"""CPU-only regression of the production finite bank-wave helper and mesh grid.

Reuses the existing standalone UE math shim; requires local MSVC/engine source.
Does not run/build Unreal or use GPU. Outputs stay ignored under Saved.
"""
import argparse
import pathlib
import subprocess
import sys

sys.dont_write_bytecode = True
from probe_maelstrom_geometry import SHIM, block


MAIN = r"""
int main(){
 ADCMaelstrom m;
 auto shore=[&](float a){return m.WaterEdgeRadius(a);};
 auto height=[&](FVector2D p){return DCMaelstromBankWaves::Height(p,m.Radius,shore);};
 bool pass=true;double worstMean=0,worstEdgeSlope=0;
 for(float phase:{0.f,.7f,2.9f,5.8f})for(float length:{2400.f,3100.f,3800.f}){
  double sum=0;const float half=length*1.05f;constexpr int count=8000;
  for(int i=0;i<=count;++i){float x=-half+2*half*i/count;sum+=DCMaelstromBankWaves::Packet(x,350,half,2400,length,phase)*(i==0||i==count?.5:1.);}
  worstMean=std::max(worstMean,std::abs(sum/count));
  worstEdgeSlope=std::max(worstEdgeSlope,double(std::abs(DCMaelstromBankWaves::Packet(half-1,350,half,2400,length,phase))));
 }
 pass&=worstMean<1e-5 && worstEdgeSlope<1e-5;
 std::printf("individual packet mean=%.9f, edge height/slope residual=%.9f: %s\n",worstMean,worstEdgeSlope,pass?"PASS":"FAIL");

 double protectedHeight=0,coastHeight=0,coastSlope=0;
 for(int t=0;t<=178;++t)for(int offset=-12500;offset<=12500;offset+=500){
  protectedHeight=std::max(protectedHeight,double(std::abs(height(DCMaelstromRoute::Point(float(t),m.Radius,float(offset))))));
 }
 for(int side=0;side<=1536;++side){
  float a=side*UE_TWO_PI/1536;FVector2D d(std::cos(a),std::sin(a));
  for(float offset:{0.f,-1.f,-100.f,-1500.f})coastHeight=std::max(coastHeight,double(std::abs(height(d*(shore(a)-offset)))));
  // One-sided1cm derivative at the unchanged shore: finite smooth return,
  // not the old35m exclusion which erased all early/mid bank packets.
  coastSlope=std::max(coastSlope,double(std::abs(height(d*(shore(a)-1.f)))));
 }
 pass&=protectedHeight<.001 && coastHeight<.001 && coastSlope<.001;
 std::printf("125m protected corridor max=%.9fcm;shore/outside max=%.9fcm;1cm shore derivative=%.9f\n",protectedHeight,coastHeight,coastSlope);

 double lo=0,hi=0,sum=0,sumSq=0,area=0,maxPlanar=0,maxPhysical=0,maxSlope=0,minPacketSamples=1e9;int active=0;
 double windowSum[3]={},windowSquare[3]={},windowMin[3]={},windowMax[3]={};int windowCount[3]={};
 const float windowTimes[3]={30,80,150};
 constexpr int sectors=1536,rings=448;
 for(int side=0;side<sectors;++side){
  float a=side*UE_TWO_PI/sectors,b=(side+1)*UE_TWO_PI/sectors;
  float edge=shore(a),nextEdge=shore(b),dr=(edge-120)/rings;
  for(int ring=1;ring<rings;++ring){
   float r=120+dr*ring;FVector2D p=FVector2D(std::cos(a),std::sin(a))*r;double h=height(p);
   // Evaluate the actual production vertices in each remaining wet bank,
   // including zero heights. A globalRMS alone hid the old zero-overlap bug:
   // its late wide bowl passed while ride350 had no packets at all.
   float t=DCMaelstromRoute::SecondsAtAngle(a);
   float across=r-m.Radius*DCMaelstromRoute::RadialFraction(t);
   if(across>12500.f)for(int w=0;w<3;++w)if(std::abs(t-windowTimes[w])<4.f){
    ++windowCount[w];windowSum[w]+=h;windowSquare[w]+=h*h;
    windowMin[w]=std::min(windowMin[w],h);windowMax[w]=std::max(windowMax[w],h);
   }
   if(std::abs(h)<.00001)continue;
   ++active;double weight=r*dr;area+=weight;sum+=h*weight;sumSq+=h*h*weight;lo=std::min(lo,h);hi=std::max(hi,h);
   FVector2D q=FVector2D(std::cos(b),std::sin(b))*(120+(nextEdge-120)*ring/rings);
   maxPlanar=std::max(maxPlanar,std::max(double(dr),(q-p).Size()));
   const double neighbour=std::max(double(dr),(q-p).Size());
   int cx=int(std::floor(p.X/DCMaelstromBankWaves::CellSize)),cy=int(std::floor(p.Y/DCMaelstromBankWaves::CellSize));
   for(int y=cy-1;y<=cy+1;++y)for(int x=cx-1;x<=cx+1;++x){
    FVector2D center((x+.2f+.6f*DCMaelstromBankWaves::Random(x,y,1013904223u))*DCMaelstromBankWaves::CellSize,
     (y+.2f+.6f*DCMaelstromBankWaves::Random(x,y,2246822519u))*DCMaelstromBankWaves::CellSize);
    if((p-center).SizeSquared()>27200000.f)continue;
    double wavelength=std::max(double(2400.f+1400.f*DCMaelstromBankWaves::Random(x,y,668265263u)),
     double(6.f*DCMaelstromBankWaves::PacketMeshStep(center,m.Radius,shore)));
    minPacketSamples=std::min(minPacketSamples,wavelength/neighbour);
   }
   maxPhysical=std::max(maxPhysical,std::hypot((q-p).Size(),m.HeightAt(q)-m.HeightAt(p)));
   double dx=(height(p+FVector2D(60,0))-height(p-FVector2D(60,0)))/120.;
   double dy=(height(p+FVector2D(0,60))-height(p-FVector2D(0,60)))/120.;
   maxSlope=std::max(maxSlope,std::hypot(dx,dy));
   pass&=std::isfinite(h)&&std::isfinite(dx)&&std::isfinite(dy);
  }
 }
 double mean=sum/area,rms=std::sqrt(sumSq/area);
 pass&=active>1000 && lo>=-200 && hi<=200 && std::abs(mean)<10 && rms>15 && maxSlope<1.2;
 std::printf("actual448x1536 grid active=%d height=%.3f..%.3fm weighted mean=%.5fm RMS=%.3fm maximum added slope=%.4f\n",active,lo*.01,hi*.01,mean*.01,rms*.01,maxSlope);
 pass&=minPacketSamples>=6;
 std::printf("active grid max planar neighbour=%.3fm physical neighbour=%.3fm; minimum contributing-packet wavelength/neighbour=%.3f samples\n",maxPlanar*.01,maxPhysical*.01,minPacketSamples);
 for(int w=0;w<3;++w){
  double rms=windowCount[w]?std::sqrt(windowSquare[w]/windowCount[w]):0;
  // Early/mid banks must now contain resolved signed relief. Late depth includes
  // the broad quiet region beyond the packet window, so only its extent is used.
  pass&=windowCount[w]>100 && windowMax[w]-windowMin[w]>40 && (w==2||rms>12);
  std::printf("elapsed%.0f +/-4s wet-bank vertices=%d range=%.3f..%.3fm RMS=%.3fm mean=%.4fm\n",
   windowTimes[w],windowCount[w],windowMin[w]*.01,windowMax[w]*.01,rms*.01,
   windowCount[w]?windowSum[w]/windowCount[w]*.01:0);
 }

 double maxSeamJump=0,maxSlopeJump=0;int seamSamples=0;
 for(int t=40;t<170;t+=2)for(int offset=15000;offset<36000;offset+=700){
  FVector2D p=DCMaelstromRoute::Point(float(t),m.Radius,float(offset));
  for(int axis=0;axis<2;++axis){
   if(axis==0)p.X=std::round(p.X/DCMaelstromBankWaves::CellSize)*DCMaelstromBankWaves::CellSize;
   else p.Y=std::round(p.Y/DCMaelstromBankWaves::CellSize)*DCMaelstromBankWaves::CellSize;
   FVector2D step=axis==0?FVector2D(1,0):FVector2D(0,1);
   double a=height(p-step),b=height(p),c=height(p+step);
   if(std::abs(a)+std::abs(b)+std::abs(c)>.001)++seamSamples;
   maxSeamJump=std::max(maxSeamJump,std::abs(c-a));maxSlopeJump=std::max(maxSlopeJump,std::abs(c-2*b+a));
  }
 }
 pass&=seamSamples>100 && maxSeamJump<2 && maxSlopeJump<.03;
 std::printf("spatial cell seam probes=%d max2cm height change=%.6fcm slope jump=%.6f\n",seamSamples,maxSeamJump,maxSlopeJump);
 for(float r:{50000.f,100000.f,160000.f}){
  pass&=height(FVector2D(-r,.01))==0 && height(FVector2D(-r,-.01))==0;
 }
 std::printf("Bank packet regression: %s\n",pass?"PASS":"FAIL");return pass?0:1;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=pathlib.Path, default=pathlib.Path("E:/Utilities/UE_5.8"))
    parser.add_argument("--vcvars", type=pathlib.Path, default=pathlib.Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"))
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    source = root / "Source/DirectorsCut"
    code = SHIM.replace("using int32=int32_t;", "using int32=int32_t; using uint32=uint32_t;")
    code = code.replace("struct FMath {", "struct FMath { static int32 FloorToInt(float x){return int32(std::floor(x));}")
    engine = (args.engine / "Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp").read_text()
    code += block(engine, "namespace FMathPerlinHelpers") + ";\n" + block(engine, "float FMath::PerlinNoise2D")
    code += block((source / "DCMaelstromRoute.h").read_text(), "namespace DCMaelstromRoute")
    code += block((source / "DCMaelstromBankWaves.h").read_text(), "namespace DCMaelstromBankWaves")
    basin = (source / "DCMaelstrom.cpp").read_text()
    code += block(basin, "float RimVariation")
    for name in ("GetRimRadius", "GetOverflowAmount", "GetRimJoin", "RimElevation", "BaseHeightAt", "HeightAt"):
        code += block(basin, "float ADCMaelstrom::" + name)
    code += block((source / "DCMaelstromTerrain.cpp").read_text(), "float ADCMaelstrom::WaterEdgeRadius")
    code += block((source / "DCMaelstromBreakers.cpp").read_text(), "float ADCMaelstrom::BreakerHeight")
    code += MAIN
    out = root / "Saved/GeometryProbe/BankPackets"
    out.mkdir(parents=True, exist_ok=True)
    cpp, exe = out / "bank_packets.cpp", out / "bank_packets.exe"
    cpp.write_text(code)
    batch = out / "build_probe.cmd"
    batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\n'
                     f'cl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "bank_packets.obj"}"\n')
    subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
    result = subprocess.run([str(exe)], cwd=out, text=True, capture_output=True)
    (out / "results.txt").write_text(result.stdout + result.stderr)
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
