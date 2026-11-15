"""CPU-only behavioral checks of the production helm helper; no Unreal/GPU."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'Saved/SteeringProbe'
out.mkdir(parents=True, exist_ok=True)
cpp = out / 'probe.cpp'
cpp.write_text(r'''
#include "DCSteering.h"
#include "DCOceanCoordinates.h"
#include <cstdio>
#include <cstdlib>
using namespace DCSteering;
int Failures=0;
void Check(bool Pass,const char* Name){std::printf("%s: %s\n",Pass?"PASS":"FAIL",Name);Failures+=!Pass;}
void Run(State& S,float Input,float Seconds,float Dt=1.f/120,float Limit=5000,float Drift=0){
 const int Frames=int(std::lround(Seconds/Dt));
 for(int I=0;I<Frames;++I)Advance(S,Input,900,Limit,Drift,0,Dt);
}
int main(){
 State S;Run(S,1,.1f);
 Check(S.Velocity>0&&S.Velocity<30,"initial turn accelerates without an immediate strafe");
 Run(S,1,1.9f);float Moving=S.Velocity,At=S.Position;
 Run(S,0,.5f);Check(S.Velocity>Moving*.65f&&S.Position>At+Moving*.35f,"release carries forward momentum");
 float Before=S.Velocity;Run(S,-1,.1f);Check(S.Velocity>0&&S.Velocity<Before,"countersteering brakes before reversing");
 Run(S,-1,2.f);Check(S.Velocity<0,"held countersteering reverses travel");
 State Coast{0,700,0};Run(Coast,0,15);Check(std::abs(Coast.Velocity)<2,"uncommanded drift gradually settles");
 float MaxEdge=0;bool Returned=false;
 State Edge{3800,750,0};
 for(int I=0;I<120*30;++I){Advance(Edge,1,900,5000,120,0,1.f/120);MaxEdge=std::max(MaxEdge,std::abs(Edge.Position));Returned|=Edge.Velocity<0;}
 Check(MaxEdge<4990&&Returned,"held outward helm recovers inward before hard channel boundary");
 std::printf("Boundary maximum %.2fcm, final %.2fcm\n",MaxEdge,Edge.Position);
 State Left,Right;Run(Left,-1,8);Run(Right,1,8);
 Check(std::abs(Left.Position+Right.Position)<.01f&&std::abs(Left.Velocity+Right.Velocity)<.01f,"port and starboard dynamics are symmetric");
 State Runs[3];const int FPS[3]={30,60,144};
 for(int I=0;I<3;++I){Run(Runs[I],1,2,1.f/FPS[I]);Run(Runs[I],0,1,1.f/FPS[I]);Run(Runs[I],-1,2,1.f/FPS[I]);}
 float Error=0;for(int I=1;I<3;++I)Error=std::max(Error,std::abs(Runs[I].Position-Runs[0].Position));
 Check(Error<8,"30/60/144fps replay differs by less than eight centimetres");
 std::printf("Frame-rate position spread %.4fcm\n",Error);
 State Predict,Live;
 for(int I=0;I<40;++I)Advance(Predict,.6f,1350,9000,50,10,.1f);
 for(int I=0;I<480;++I)Advance(Live,.6f,1350,9000,50,10,1.f/120);
 Check(std::abs(Predict.Position-Live.Position)<.1f,"navigation forecast and live simulation share the same carried trajectory");
 Check(VisualTurn({0,600,200},0,900)>0&&VisualTurn({0,-600,-200},0,900)<0,"visual yaw follows actual turn including momentum");
 double WaterError=0;
 for(double Lane:{-9000.,-4000.,0.,3500.,9000.})for(double WorldY:{-15000.,-200.,12000.}){
   const double StageY=WorldY-Lane;
   const double CPU=DCOcean::WaveY(StageY,Lane);
   const double Shader=StageY-(-Lane); // MPC Trough.x, read by every patched wave path.
   WaterError=std::max(WaterError,std::abs(CPU-WorldY));
   WaterError=std::max(WaterError,std::abs(Shader-CPU));
 }
 Check(WaterError<1e-9,"world-fixed water phase and CPU surface queries remain aligned while steering");
 return Failures?1:0;
}
''')
vcvars = Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat')
batch = out / 'build.cmd'
batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{out / "probe.exe"}" /Fo:"{out / "probe.obj"}"\n')
subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
result = subprocess.run([str(out/'probe.exe')],cwd=out,capture_output=True,text=True)
(out/'results.txt').write_text(result.stdout+result.stderr)
print(result.stdout,end='')
raise SystemExit(result.returncode)
