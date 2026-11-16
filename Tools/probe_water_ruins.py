"""CPU geometry, actual helm response, water datum and impact-gating checks."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'Saved/WaterRuinsProbe'
out.mkdir(parents=True,exist_ok=True)
(out/'CoreMinimal.h').write_text('#pragma once\n#include <cmath>\n#include <algorithm>\nusing int32=int;\nstruct FMath { template<class T> static T Clamp(T X,T A,T B){return std::clamp(X,A,B);} template<class T> static T Max(T A,T B){return std::max(A,B);} template<class T> static T Abs(T X){return std::abs(X);} };\n')
source=r'''
#include "DCCityObstacleLayout.h"
#include "DCSteering.h"
#include "DCFogRouteLayout.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
int Checks=0;
void Check(bool Good,const char* What){++Checks;if(!Good){std::printf("FAIL: %s\n",What);std::exit(1);}}
float Rise(float Y){float U=std::clamp(std::abs(Y)/9000.f,0.f,1.f);return 1100*U*U*(3-2*U);}
int main(){
 using namespace DCCityObstacleLayout;
 const float HullRadius=std::hypot(2200.f,500.f);
 Check(Count==5,"five sparse fixed encounters");
 Check(Interval*1500-2*(2200+MaximumFootprint)>30000,"long clear water between clusters");
 for(int I=0;I<Count;++I){
  Check(PassingClearance(I,HullRadius)>500,"entire yawed hull fits opposite passing lane");
  Check(std::abs(Lane(I))+MaximumFootprint<9000,"stone never clips either bank");
  if(I+1<Count)Check(Lane(I)*Lane(I+1)<0,"staggered course never makes a cross-water wall");
  const float Top=MinimumPillarTop(230,Lane(I),9000,1100);
  for(float Offset=-MaximumFootprint;Offset<=MaximumFootprint;Offset+=50){
   for(float ShipLane=-9000;ShipLane<=9000;ShipLane+=250){
    float Water=230+Rise(Lane(I)+Offset)-Rise(ShipLane)+WaveAllowance;
    Check(AnchoredZ(Top,Rise(ShipLane))-Water>=MinimumVisiblePillar-.1f,"foot-height fit stays visible across all helm offsets");
    float LowWater=230+Rise(Lane(I)+Offset)-Rise(ShipLane)-WaveAllowance;
    Check(AnchoredZ(230-Submergence-800,Rise(ShipLane))<LowWater,"deep fixed foundation remains below wave trough");
   }
  }
  for(float Authority:{1.f,.35f})for(float Dt:{1.f/120,1.f/30,.1f}){
   DCSteering::State S;float Time=0,Min=1e9;
   for(;Time<18;Time+=Dt){
    const float Error=PassingLane(I)-S.Position-S.Velocity*.7f;
    const float Helm=Time<2?0:std::clamp(Error/700.f,-1.f,1.f);
    const float U=std::clamp(std::abs(S.Position)/9000.f,0.f,1.f);
    const float BankPull=1100*6*U*(1-U)/9000*(S.Position<0?-1:1)*1200;
    DCSteering::Advance(S,Helm,1350*Authority,9000,0,BankPull,Dt);
    // Visible obstacle starts 170m ahead: two-second reaction, then
    // sweep its whole longitudinal span against a conservative yawed beam.
    const float X=17000-1500*Time;
    if(std::abs(X)<2200+MaximumFootprint){
      const float Yaw=std::abs(DCSteering::VisualTurn(S,0,900))*8*3.14159265f/180;
      const float Beam=2200*std::sin(Yaw)+500*std::cos(Yaw);
      const float Clearance=std::abs(S.Position-Lane(I))-MaximumFootprint-Beam;
      Min=std::min(Min,Clearance);Check(Clearance>100,"two-second reaction clears every cluster even with damaged rigging");
    }
   }
   std::printf("lane %.0f authority %.2f dt %.3f minimum clearance %.0fcm\n",Lane(I),Authority,Dt,Min);
  }
 }
 // Every bank slice retains the same fixed enemy support, while its spare
 // inner edge and extended outer terrace differ between port and starboard.
 float MaxInset=0,MinInset=1e9,MaxAsymmetry=0;
 for(float X=0;X<=400000;X+=500){
  MaxAsymmetry=std::max(MaxAsymmetry,std::abs(DCFogRouteLayout::ShoreInset(X,-1)-DCFogRouteLayout::ShoreInset(X,1)));
  for(float Side:{-1.f,1.f}){
   float Inset=DCFogRouteLayout::ShoreInset(X,Side),Outer=DCFogRouteLayout::ShoreOuterExtension(X,Side);
   MaxInset=std::max(MaxInset,Inset);MinInset=std::min(MinInset,Inset);
   Check(Inset>=0 && Inset<=900,"recessed shoreline never enters reserved corridor");
   Check(Outer>=0,"outer terrace never removes the supporting bench");
   for(int Group=0;Group<4;++Group)for(int I=0;I<DCFogRouteLayout::GroupSize(Group);++I){
    const float Radius=(std::hypot(137.113953f,92.420547f)+13.226578f)*(10+.8f*I);
    const float Centre=3000+DCFogRouteLayout::WalkerOffset(I,Radius);
    Check(Centre-Radius>Inset+100,"every actual Rampage footprint remains supported over all shoreline samples");
    Check(Centre+Radius<6000+Outer,"outer bank retains each moving creature footprint");
    Check(Centre-Radius>DCFogRouteLayout::ShoreHeadlandCentre+DCFogRouteLayout::ShoreHeadlandHalfWidth+100,"headlands do not intersect walker path");
   }
   Check(3000-1780>Inset+100,"statue pedestal keeps supported inboard margin");
  }
 }
 Check(MaxInset-MinInset>850 && MaxAsymmetry>500,"bank profiles vary substantially and asymmetrically");
 Check(DCFogRouteLayout::ShoreHeadlandCentre-DCFogRouteLayout::ShoreHeadlandHalfWidth>0,"headlands stay outside full rotated hull envelope");
 Check(IsRock(1)&&IsRock(3)&&!IsRock(0)&&!IsRock(2)&&!IsRock(4),"course restores two rock hazards among three pillar clusters");
 Check(MinimumVisiblePillar==2400,"pillars have four times previous visible clearance");
 Check(std::abs(Lane(0))<MaximumFootprint+500,"centre route requires a deliberate choice");
 bool Hit=false;int Events=0;
 for(int Frame=0;Frame<600;++Frame)if(CanImpact(Hit,0)){Hit=true;++Events;}
 Check(Events==1,"sustained contact emits exactly one impact per stone");
 Check(!CanImpact(false,1.f),"global cooldown still prevents stacked impacts");
 Check(CanImpact(false,0.f),"next untouched stone retains its impact");
 std::printf("PASS: %d water ruin geometry and behavior checks\n",Checks);
}
'''
cpp=out/'probe.cpp';cpp.write_text(source)
vcvars=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat')
batch=out/'build.cmd'
batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 /I"{out}" /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{out / "probe.exe"}" /Fo:"{out / "probe.obj"}"\n')
subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
result=subprocess.run([str(out/'probe.exe')],cwd=out,capture_output=True,text=True)
(out/'results.txt').write_text(result.stdout+result.stderr)
print(result.stdout,end='')
raise SystemExit(result.returncode)
