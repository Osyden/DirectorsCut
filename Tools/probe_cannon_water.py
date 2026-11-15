"""CPU tests of production surface-entry sweep, latency correction and wheel direction."""
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[1]
out=root/'Saved/CannonWaterProbe'
out.mkdir(parents=True,exist_ok=True)
source=r'''
#include "DCShotWater.h"
#include "DCWheelVisual.h"
#include <cstdio>
#include <cstdlib>
int Checks=0;
void Check(bool Good,const char* What) { ++Checks;if(!Good){std::printf("FAIL: %s\n",What);std::exit(1);} }
bool Near(double A,double B,double E=.05){return std::abs(A-B)<E;}
int main()
{
 double F=-1;
 auto Flat=[](double,double){return 230.;};
 Check(DCShotWater::FirstEntry(0,0,1230,0,5000,-770,Flat,F)&&Near(F,.5,.0001),"flat water first entry");
 Check(!DCShotWater::FirstEntry(0,0,1230,0,5000,900,Flat,F),"clear air remains flying");
 Check(DCShotWater::FirstEntry(0,0,229,0,1000,900,Flat,F)&&F==0,"wave engulfed start contacts immediately");
 auto Slope=[](double X,double){return X*.3;};
 Check(DCShotWater::FirstEntry(0,0,100,1000,0,200,Slope,F)&&Near(F,.5,.0001),"rising wave face can intercept rising shot");
 auto Crest=[](double X,double){return 800.*std::exp(-std::pow((X-2000.)/450.,2));};
 Check(DCShotWater::FirstEntry(0,0,300,5000,0,300,Crest,F)&&F>.2&&F<.4,"long sweep finds crest with both endpoints above water");
 const double FirstCrest=F;
 Check(Crest(5000*FirstCrest,0)>=299.9,"intersection refines to actual crest surface");
 auto DoubleCrest=[](double X,double){return 800.*std::exp(-std::pow((X-1000.)/300.,2))+900.*std::exp(-std::pow((X-4000.)/300.,2));};
 Check(DCShotWater::FirstEntry(0,0,300,6000,0,300,DoubleCrest,F)&&F<.2,"first of multiple wave crossings wins");
 // Visible channel banks are WPO added once above the delayed raw FFT sample.
 const double Raw=DCShotWater::ProjectHeight(200,-.1,-.2,1,300,200,80,.05);
 Check(Near(Raw,274),"capture-position slope and bounded age correction");
 auto Trough=[Raw](double,double Y){return Raw+.00002*Y*Y;};
 Check(DCShotWater::FirstEntry(0,2814,1200,0,2814,0,Trough,F)&&Near(1200*(1-F),Raw+.00002*2814*2814),"trough offset at world lane coordinates, once");
 Check(Near(DCShotWater::ProjectHeight(0,0,0,1,0,0,10000,.2),100),"vertical latency prediction bounded");
 Check(!DCShotWater::FreshCapture(10,.08,10),"reused channel rejects prior projectile capture");
 Check(DCShotWater::FreshCapture(10.1,.05,10),"current projectile capture accepted");
 Check(!DCShotWater::FreshCapture(10.4,.3,10),"stale GPU sample falls back");
 for(double VZ : {-1500.,0.,1800.,6000.})
 {
   const double Life=DCShotWater::Lifetime(1000,230,VZ,1400,3);
   const double WaterTime=(VZ+std::sqrt(VZ*VZ+2*1400*770))/1400;
   Check(Life>WaterTime&&Life<=15,"flight lives through descending water entry");
   double Z=1000,V=VZ,Time=0;int Contacts=0;
   while(Time<Life)
   {
      const double DT=.137,Next=Z+V*DT-.5*1400*DT*DT;
      if(DCShotWater::FirstEntry(0,0,Z,0,0,Next,Flat,F)){++Contacts;break;}
      Z=Next;V-=1400*DT;Time+=DT;
   }
   Check(Contacts==1,"coarse frame downward flight terminates once at water");
 }
 constexpr double Radians=3.141592653589793/180;
 Check(-std::sin(DCWheelVisual::Degrees(1)*Radians)>0,"D rotates upper rim toward starboard from helm and aft cameras");
 Check(-std::sin(DCWheelVisual::Degrees(-1)*Radians)<0,"A rotates upper rim toward port");
 Check(DCWheelVisual::Degrees(0)==0,"released wheel returns to centered rest");
 std::printf("PASS: %d cannon water / wheel checks\n",Checks);
}
'''
cpp=out/'probe.cpp';cpp.write_text(source)
vcvars=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat')
batch=out/'build.cmd'
batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{out / "probe.exe"}" /Fo:"{out / "probe.obj"}"\n')
subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
result=subprocess.run([str(out/'probe.exe')],cwd=out,capture_output=True,text=True)
(out/'results.txt').write_text(result.stdout+result.stderr)
print(result.stdout,end='')
raise SystemExit(result.returncode)
