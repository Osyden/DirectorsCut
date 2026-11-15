"""CPU-only MSVC probe of production inlet height, route grade and landing motion."""
from pathlib import Path
import subprocess
import sys
sys.dont_write_bytecode = True
from probe_maelstrom_geometry import SHIM, block

root = Path(__file__).resolve().parents[1]
source = (root / "Source/DirectorsCut/DCMaelstrom.cpp").read_text(encoding="utf-8")
route = (root / "Source/DirectorsCut/DCMaelstromRoute.h").read_text(encoding="utf-8")
code = SHIM + block(route, "namespace DCMaelstromRoute")
code += block(source, "float ADCMaelstrom::HeightAt")
code += r'''
int main() {
    using namespace DCMaelstromRoute;
    ADCMaelstrom m;
    int failures=0,checks=0;
    auto check=[&](bool good,const char* name){++checks;if(!good){++failures;std::printf("FAIL %s\n",name);}};
    for(float offset:{-9000.f,0.f,9000.f}) {
        float steepest=0.f,lastPitch=0.f,landingTime=-1.f;
        int contacts=0;
        bool landed=false;
        for(float t=-8.f;t<80.f;t+=.02f) {
            const auto a=Point(t+.10f,m.Radius,offset),b=Point(t-.10f,m.Radius,offset);
            const float grade=FMath::RadiansToDegrees(FMath::Atan2(m.HeightAt(a)-m.HeightAt(b),FMath::Max(1.,(a-b).Size())));
            const float pitch=FMath::FInterpTo(lastPitch,DeckPitch(grade,t),.02f,5.f);
            steepest=FMath::Min(steepest,pitch);
            check(std::isfinite(pitch)&&pitch>=-34.f&&pitch<=4.f,"bounded pitch");
            check(FMath::Abs(pitch-lastPitch)<=3.8f,"bounded physical pitch step at 50Hz");
            lastPitch=pitch;
            const auto p=Point(t,m.Radius,offset);
            if(!landed&&m.HeightAt(p)<=-39975.f&&t>0.f){landed=true;++contacts;landingTime=t;}
        }
        check(steepest<-24.f,"bow visibly down on the real steered inlet");
        check(contacts==1&&landingTime>12.f&&landingTime<80.f,"one landing per steered route");
        check(FMath::Abs(lastPitch)<.01f,"level lake orbit");
        std::printf("unguarded fixed-offset control %.0f cm: steepest %.2f deg, landing %.2f seconds, contacts %d\n",offset,steepest,landingTime,contacts);
    }
    for(int fps:{30,60,120})for(float input:{-1.f,0.f,1.f}) {
        float helm=0,offset=0,velocity=0,lastHeight=0,worstDrop=0,landing=-1;
        int unsupported=0;
        for(float t=-30.f;t<90.f;t+=1.f/fps) {
            AdvanceSteering(input,1.f/fps,FMath::SmoothStep(-4.f,12.f,t),helm,offset,velocity,InletOutwardLimit(t,m.Radius));
            const auto p=Point(t,m.Radius,offset);
            const float r=p.Size(),q=FMath::SmoothStep(.58f,.985f,r/m.Radius);
            const float across=FMath::Abs(FMath::UnwindRadians(FMath::Atan2(p.Y,p.X)-(UE_PI-.40f*(1-q))))*r;
            const float width=100*(160+25*FMath::SmoothStep(1040,1105,r*.01f)+35*FMath::SmoothStep(1113,1130,r*.01f));
            const float height=m.HeightAt(p);
            if(t>0)worstDrop=FMath::Min(worstDrop,height-lastHeight);
            if(t>0&&height>-39975&&across>width-1200)++unsupported;
            if(landing<0&&t>0&&height<=-39975)landing=t;
            lastHeight=height;
        }
        check(unsupported==0,"hull margin remains over visible inlet water");
        check(worstDrop*fps>-5000.f,"no side shelf snap drop");
        check(landing>12.f&&landing<32.f,"steered inlet reaches lake before combat orbit");
        check(input==0.f||offset*input>5000.f,"full steering authority restored after inlet");
        std::printf("guard fps%d helm%.0f unsupported%d landing%.2f worstDrop%.1fcm offset%.0fcm\n",fps,input,unsupported,landing,worstDrop,offset);
    }
    check(LandingHeave(.15f)<-50.f&&LandingHeave(.5f)>0.f,"compression then rebound");
    check(LandingHeave(-1.f)==0.f&&LandingHeave(3.f)==0.f&&LandingHeave(20.f)==0.f,"settled heave");
    for(float t=0;t<3.f;t+=.01f)check(std::abs(LandingHeave(t))<=90.f,"bounded landing heave");
    std::printf("CALDERA_MOTION_CPU checks=%d failed=%d\n",checks,failures);
    return failures?1:0;
}
'''
out = root / "Saved/CalderaMotionProbe"
out.mkdir(parents=True, exist_ok=True)
cpp, exe = out / "motion.cpp", out / "motion.exe"
cpp.write_text(code, encoding="utf-8")
vcvars = Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat")
batch = out / "build.cmd"
batch.write_text('@echo off\ncall "' + str(vcvars) + '" >nul\ncl /nologo /std:c++17 /EHsc /O2 "'
                 + str(cpp) + '" /Fe:"' + str(exe) + '" /Fo:"' + str(out / "motion.obj") + '"\n', encoding="utf-8")
subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
raise SystemExit(subprocess.run([str(exe)], cwd=out).returncode)
