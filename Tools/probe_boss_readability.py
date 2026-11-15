"""CPU checks for exact target-marker windows, screen placement and authored bone/curve mapping."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'Saved/BossReadabilityProbe';out.mkdir(parents=True,exist_ok=True)
(out/'CoreMinimal.h').write_text(r'''
#pragma once
#include <cmath>
#include <algorithm>
struct FVector {
 double X,Y,Z;FVector(double x=0,double y=0,double z=0):X(x),Y(y),Z(z){}
 FVector operator+(FVector b)const{return {X+b.X,Y+b.Y,Z+b.Z};}
 FVector operator-(FVector b)const{return {X-b.X,Y-b.Y,Z-b.Z};}
 FVector operator*(double s)const{return {X*s,Y*s,Z*s};}
 double Size()const{return std::sqrt(X*X+Y*Y+Z*Z);}
 FVector GetSafeNormal()const{double l=Size();return l>1e-8?*this*(1/l):FVector();}
};
struct FMath {
 static float Clamp(float v,float a,float b){return std::clamp(v,a,b);}
 static FVector Lerp(FVector a,FVector b,float t){return a+(b-a)*t;}
};
''')
source=r'''
#include "DCTentacleCurve.h"
#include "DCBossMarkerPolicy.h"
#include "DCAuthoredLimbPolicy.h"
#include "DCBossTimeline.h"
#include <cstdio>
#include <cstdlib>
int Checks=0;
void Check(bool Good,const char* What){++Checks;if(!Good){std::printf("FAIL %s\n",What);std::exit(1);}}
int main(){
 using namespace DCBossMarkerPolicy;
 for(float width:{800.f,1280.f,1600.f,1920.f})for(float height:{600.f,720.f,900.f,1080.f}){
  const float scale=std::clamp(height/900.f,.7f,1.5f);
  for(float x:{-2000.f,0.f,width*.5f,width,width+2000})for(float y:{-1000.f,0.f,height*.5f,height,height+1000}){
   auto p=Place(x,y,true,width,height,scale);
   Check(p.X>=0&&p.X<=width&&p.Y>=0&&p.Y<=height,"marker stays on viewport");
   Check(p.LabelX>=0&&p.LabelY>=0&&p.LabelX+288*scale<=width&&p.LabelY+66*scale<=height,"readable label stays in viewport");
   Check(p.AtEdge==(std::abs(p.X-x)>1||std::abs(p.Y-y)>1),"clamped target gets directional arrow");
   for(bool urgent:{false,true})for(bool projected:{false,true}){
    auto b=PlaceBody(x,y,projected,width,height,scale,urgent);
    Check(b.X>=0&&b.X<=width&&b.Y>=0&&b.Y+47*scale<=height,"body awareness stays visible at all view edges");
    Check(projected||b.AtEdge,"body behind any station camera retains an edge arrow");
    Check(!urgent||!b.AtEdge||std::abs(b.Y-(height-260*scale))<.01f,"quiet body arrow moves below urgent attack guidance");
   }
  }
 }
 Check(Place(800,450,false,1600,900,1).AtEdge,"behind-camera target retains directional cue");
 Check(Urgency(12)==0&&Urgency(3)==0&&Urgency(0)==1,"urgent tint only in final three seconds");
 for(const auto& e:DCBossTimeline::Events){
  const float strike=e.At+DCBossTimeline::Tell(e.Kind);
  for(float t:{e.At-.001f,e.At,strike-.001f,strike,160.f}){
   const bool open=DCBossTimeline::Open(t,e.At,strike,160);
   Check(ShowTarget(true,open,false,false,false)==open,"marker uses actual target hit window including mouth");
   Check(!ShowTarget(true,open,true,false,false)&&!ShowTarget(true,open,false,true,false)&&!ShowTarget(true,open,false,false,true)&&!ShowTarget(false,open,false,false,false),"no shoot marker after interrupt/cancel/strike/removal");
  }
 }
 for(float sourceJoint:{.61f,.68f,.72f,.77f,.84f}){
  Check(std::abs(DCAuthoredLimbPolicy::CurveU(sourceJoint,sourceJoint)-.72f)<.00001,"authored joint bone pins exactly to cannon target");
  Check(DCAuthoredLimbPolicy::CurveU(0,sourceJoint)==0&&DCAuthoredLimbPolicy::CurveU(1,sourceJoint)==1,"root and tip remain pinned");
  float before=-1;
  for(int i=0;i<=1000;++i){float u=DCAuthoredLimbPolicy::CurveU(i/1000.f,sourceJoint);Check(u>=before&&u>=0&&u<=1,"authored bone ordering never folds or leaves curve");before=u;}
 }
 for(float side:{-1.f,1.f})for(float tension:{0.f,.5f,1.f}){
  FVector base(2000,side*10000,-5000),bend(-1500,side*8500,3300+tension*1600);
  FVector joint(2200,side*7000,1400),tip(800-1800*tension,side*(6500+350*tension),4200+1600*tension);
  Check((DCTentacleCurve::Point(0,base,bend,joint,tip)-base).Size()<.01,"authored root remains submerged at committed root");
  Check((DCTentacleCurve::Point(.72f,base,bend,joint,tip)-joint).Size()<.01,"strong windup never moves vulnerable joint");
  Check((DCTentacleCurve::Point(1,base,bend,joint,tip)-tip).Size()<.01,"authored tip follows windup/contact control");
  auto a=DCTentacleCurve::Tangent(.7199f,base,bend,joint,tip),b=DCTentacleCurve::Tangent(.7201f,base,bend,joint,tip);
  Check(a.X*b.X+a.Y*b.Y+a.Z*b.Z>.999,"joint tangent remains continuous for skinning");
  for(int i=0;i<=100;++i){auto p=DCTentacleCurve::Point(i/100.f,base,bend,joint,tip);Check(std::isfinite(p.X)&&std::isfinite(p.Y)&&std::isfinite(p.Z),"all bone curve samples remain finite");}
 }
 std::printf("PASS %d boss marker, authored mapping and curve checks\n",Checks);
}
'''
cpp=out/'probe.cpp';cpp.write_text(source)
vcvars=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat')
batch=out/'build.cmd'
batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 /I"{out}" /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{out / "probe.exe"}" /Fo:"{out / "probe.obj"}"\n')
subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
result=subprocess.run([str(out/'probe.exe')],cwd=out,capture_output=True,text=True)
(out/'results.txt').write_text(result.stdout+result.stderr)
print(result.stdout,end='');raise SystemExit(result.returncode)
