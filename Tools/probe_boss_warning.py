"""Compile the actual boss warning callback with small CPU-only lifetime mocks."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'Saved/BossWarningProbe'; out.mkdir(parents=True,exist_ok=True)
source=(root/'Source/DirectorsCut/DCBossCombat.cpp').read_text()
guard=source.split('auto StillRelevant=',1)[1].split('    if(!StillRelevant())',1)[0].strip().rstrip(';')
cpp=r'''
#include <functional>
#include <cstdio>
#include <cmath>
#include "DCBossTimeline.h"
using int32=int;
struct FVector {float X=0,Y=0;};
struct Transform {float Yaw=0; FVector InverseTransformPosition(FVector P) const {return {0,-std::sin(Yaw)*P.X+std::cos(Yaw)*P.Y};}};
enum class EDCPhase {Maelstrom,Ending};
struct AActor {bool Alive=true; FVector Position{0,-3}; FVector GetActorLocation() const{return Position;}};
template<class T> struct TWeakObjectPtr {T* P=nullptr; TWeakObjectPtr()=default; TWeakObjectPtr(T* In):P(In){} T* Get()const{return IsValid()?P:nullptr;} bool IsValid()const{return P&&P->Alive;} T* operator->()const{return Get();} bool operator==(const TWeakObjectPtr& B)const{return P==B.P;} bool operator!=(const TWeakObjectPtr& B)const{return P!=B.P;}};
namespace DCBossFireball {constexpr float Flight=3.f;}
struct DirectorType {float RideTime=345; Transform DeckTransform; EDCPhase Phase=EDCPhase::Maelstrom; EDCPhase GetPhase()const{return Phase;}};
struct FAttack {int Index=1;float Start=70,Strike=82;DCBossTimeline::EAttack Kind=DCBossTimeline::EAttack::Slam;bool Cancelled=false,Applied=false,Interrupted=false,Released=false;TWeakObjectPtr<AActor> Target,Projectile;};
struct ADCBossCombat {bool Alive=true,Running=true,Finished=false;DirectorType* Director=nullptr;float RideOrigin=270,Deadline=160;FAttack Attack;};
auto MakeGuard(TWeakObjectPtr<ADCBossCombat> Weak,TWeakObjectPtr<AActor> Subject,int Event,float Start,DCBossTimeline::EAttack Kind,bool Incoming,bool Port) {return GUARD;}
int main(){int Count=0,Failed=0;auto Check=[&](bool OK,const char* Name){++Count;if(!OK){++Failed;std::printf("FAIL %s\n",Name);}};
 DirectorType D;ADCBossCombat B;B.Director=&D;AActor Target,Other,Projectile;B.Attack.Target=&Target;
 auto G=MakeGuard(&B,&Target,1,70,DCBossTimeline::EAttack::Slam,false,true);
 Check(G(),"live slam accepted");
 for(bool* Flag:{&B.Attack.Interrupted,&B.Attack.Cancelled,&B.Attack.Applied,&B.Finished}){*Flag=true;Check(!G(),"expired attack rejected");*Flag=false;}
 B.Running=false;Check(!G(),"inactive encounter rejected");B.Running=true;
 B.Alive=false;Check(!G(),"destroyed owner rejected");B.Alive=true;
 Target.Alive=false;Check(!G(),"destroyed target rejected");Target.Alive=true;
 B.Attack.Target=&Other;Check(!G(),"replacement target never inherits callback");B.Attack.Target=&Target;
 B.Attack.Index=2;Check(!G(),"next event rejected");B.Attack.Index=1;
 B.Attack.Start=71;Check(!G(),"reconstructed identity rejected");B.Attack.Start=70;
 D.RideTime=352;Check(!G(),"strike boundary rejected");D.RideTime=345;
 D.Phase=EDCPhase::Ending;Check(!G(),"phase transition rejected");D.Phase=EDCPhase::Maelstrom;
 D.DeckTransform.Yaw=3.14159265f;Check(!G(),"changed ship-relative battery invalidates spoken side");D.DeckTransform.Yaw=0;
 B.Attack.Kind=DCBossTimeline::EAttack::Obstacle;auto O=MakeGuard(&B,&Target,1,70,B.Attack.Kind,false,true);
 D.DeckTransform.Yaw=3.14159265f;Check(O(),"committed obstacle safe lane is independent of ship yaw");
 B.Attack.Kind=DCBossTimeline::EAttack::Mouth;B.Attack.Released=true;B.Attack.Target=nullptr;Target.Alive=false;B.Attack.Projectile=&Projectile;D.RideTime=353;
 auto F=MakeGuard(&B,&Projectile,1,70,B.Attack.Kind,true,false);
 Check(F(),"released fire remains relevant without original weakpoint");
 D.RideTime=355;Check(!F(),"incoming expires exactly at impact time");D.RideTime=353;
 B.Attack.Projectile=&Other;Check(!F(),"incoming requires exact projectile identity");B.Attack.Projectile=&Projectile;
 Projectile.Alive=false;Check(!F(),"destroyed projectile invalidates incoming warning");
 std::printf("BOSS_WARNING_CPU checks=%d failed=%d\n",Count,Failed);return Failed?1:0;}
'''.replace('GUARD',guard)
(out/'probe.cpp').write_text(cpp)
vcvars=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat')
(out/'build.cmd').write_text(f'@echo off\ncall "{vcvars}" >nul\ncl /nologo /std:c++17 /EHsc /O2 /I"{root / "Source/DirectorsCut"}" "{out / "probe.cpp"}" /Fe:"{out / "probe.exe"}" /Fo:"{out / "probe.obj"}"\n')
subprocess.run(['cmd','/d','/c',str(out/'build.cmd')],cwd=out,check=True)
raise SystemExit(subprocess.run([str(out/'probe.exe')],cwd=out).returncode)
