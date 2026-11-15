"""CPU check of actual cue topology/policy and authoritative boss windows. No Unreal/GPU."""
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[1]
out=root/'Saved/CombatCueProbe'
out.mkdir(parents=True,exist_ok=True)
component=(root/'Source/DirectorsCut/DCCombatCue.cpp').read_text()
geometry=component[component.index('namespace\n{'):component.index('UDCCombatCueComponent* UDCCombatCueComponent::Create')]
stub=r'''
#include "DCCombatCuePolicy.h"
#include "DCBossTimeline.h"
#include <vector>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
using int32=int;
struct FVector {float X,Y,Z; FVector(float x,float y,float z):X(x),Y(y),Z(z){} };
struct FLinearColor {float R,G,B,A; FLinearColor(float r,float g,float b,float a):R(r),G(g),B(b),A(a){} };
template<class T> struct TArray:std::vector<T>{
 int Num()const{return int(this->size());}
 void Add(const T& v){this->push_back(v);}
 void Append(std::initializer_list<T> v){this->insert(this->end(),v);}
};
struct FMath {
 static float Lerp(float a,float b,float t){return a+(b-a)*t;}
 static float Cos(float t){return std::cos(t);}
 static float Sin(float t){return std::sin(t);}
};
'''
tests=r'''
int Checks=0;
void Check(bool b,const char* Message){++Checks;if(!b){std::printf("FAIL: %s\n",Message);std::exit(1);}}
void MeshCheck(const FDCCombatCueGeometry& G){
 Check(G.V.Num()==G.C.Num()&&G.I.Num()%3==0,"matching vertex/color and triangle counts");
 for(int n:G.I)Check(n>=0&&n<G.V.Num(),"triangle references actual vertex");
 for(auto c:G.C)Check(c.A>=0&&c.A<=1,"opacity bounded");
 for(int i=0;i<G.I.Num();i+=3){
  auto a=G.V[G.I[i]],b=G.V[G.I[i+1]],c=G.V[G.I[i+2]];
  float x=b.X-a.X,y=b.Y-a.Y,z=b.Z-a.Z,u=c.X-a.X,v=c.Y-a.Y,w=c.Z-a.Z;
  float area=std::pow(y*w-z*v,2)+std::pow(z*u-x*w,2)+std::pow(x*v-y*u,2);
  Check(area>1e-4f,"no zero-area triangles");
 }
}
int main(){
 using namespace DCCombatCuePolicy;
 for(int stage=0;stage<5;++stage)for(int target=0;target<2;++target)for(int hidden=0;hidden<2;++hidden)for(int concealed=0;concealed<2;++concealed)
  Check(AlertVisible(stage,target,concealed,hidden)==(stage==1&&target&&!hidden&&!concealed),"hall cue shows only actual visible charge and disappears on death/release/concealment");
 for(const auto& event:DCBossTimeline::Events){
  float end=event.At+DCBossTimeline::Tell(event.Kind);
  for(float t:{event.At-.001f,event.At,end-.001f,end,160.f}){
   bool open=DCBossTimeline::Open(t,event.At,end,160);
   bool mouth=event.Kind==DCBossTimeline::EAttack::Mouth;
   Check(WeakPointVisible(open,mouth,true,false,false,false)==(open&&!mouth),"vulnerability presentation uses exact authoritative attack bounds");
   Check(!WeakPointVisible(open,mouth,false,false,false,false),"missing target hides cue");
   Check(!WeakPointVisible(open,mouth,true,true,false,false),"interruption hides cue");
   Check(!WeakPointVisible(open,mouth,true,false,true,false),"cancel hides cue");
   Check(!WeakPointVisible(open,mouth,true,false,false,true),"applied strike hides cue");
  }
 }
 for(float charge:{-1.f,0.f,.25f,.5f,1.f,2.f})for(int frame=0;frame<1200;++frame){
  auto look=Look(charge,frame/60.f,frame<12?1.f:0.f);
  Check(look.Glow>=.62f&&look.Glow<=1.281f&&look.RadiusScale>=1&&look.RadiusScale<=1.026f,"subtle bounded glow and breathing (no strobe or size jump)");
 }
 for(float arc:{.18f,.5f,.86f}){
  FDCCombatCueGeometry G;G.Arc(220,15,0,6.283185f*arc,0,FLinearColor(.85,.49,.15,.85));MeshCheck(G);
  Check(G.C[0].A==0&&G.C[1].A>0&&G.C[2].A==0,"halo edges fade on both sides");
 }
 FDCCombatCueGeometry Band;Band.Band(180,58,.1f,1.47f,FLinearColor(.86,.54,.18,.8));MeshCheck(Band);
 Check(Band.C[0].A==0&&Band.C[1].A>0&&Band.C[2].A==0,"joint band has soft axial edges");
 FDCCombatCueGeometry Contrast;Contrast.ContrastBand(180,99,.1f,1.47f,FLinearColor(.9,.62,.24,.96));MeshCheck(Contrast);
 for(int row=0;row<8;++row){
  auto p=Contrast.V[row];
  Check(std::abs(std::sqrt(p.X*p.X+p.Y*p.Y)-180)<.01,"contrast change preserves target radius");
 }
 Check(Contrast.C[0].A==0&&Contrast.C[7].A==0,"contrast band fades only at outer edges");
 Check(Contrast.C[3].A>.95&&Contrast.C[4].A>.95&&Contrast.V[3].Z<-43&&Contrast.V[4].Z>43,"filled core retains measurable axial width and coverage");
 auto Luma=[](FLinearColor c){return .2126*c.R+.7152*c.G+.0722*c.B;};
 for(double background:{.03,.3,1.,3.})for(double glow:{.62,.9,1.28}){
  auto core=Contrast.C[3],edge=Contrast.C[1];
  double corePixel=Luma(core)*glow*core.A+background*(1-core.A);
  double edgePixel=Luma(edge)*glow*edge.A+background*(1-edge.A);
  Check(corePixel-edgePixel>.28,"translucent core/shoulder contrast survives bright and dark backgrounds without bloom boost");
  Check(edgePixel<background||background<.1,"non-additive shoulder can darken a bright background");
 }
 std::printf("PASS: %d cue geometry, bounded appearance and authoritative lifecycle checks\n",Checks);
}
'''
cpp=out/'probe.cpp'
cpp.write_text(stub+geometry+tests)
vcvars=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat')
batch=out/'build.cmd'
batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{out / "probe.exe"}" /Fo:"{out / "probe.obj"}"\n')
subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
result=subprocess.run([str(out/'probe.exe')],cwd=out,capture_output=True,text=True)
(out/'results.txt').write_text(result.stdout+result.stderr)
print(result.stdout,end='')
raise SystemExit(result.returncode)
