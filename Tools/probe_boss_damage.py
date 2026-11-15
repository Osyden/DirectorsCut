"""CPU-only production-policy checks for sustained boss damage and slam contact."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'Saved/BossDamageProbe';out.mkdir(parents=True,exist_ok=True)
source=r'''
#include "DCBossDamagePolicy.h"
#include "DCBossTimeline.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
int checks=0;
void Check(bool v,const char* what){++checks;if(!v){std::printf("FAIL %s\n",what);std::exit(1);}}
int main(){
 using namespace DCBossDamagePolicy;
 Check(BodyAboveMouth>BodyRadius+MouthRadius+30,"body and mouth projectile envelopes are disjoint");
 for(float time:{0.f,43.99f,44.f,70.f,81.99f,82.f,98.f,159.99f,160.f,180.f})
 for(bool running:{false,true})for(bool finished:{false,true})for(bool paused:{false,true}){
  const float d=BodyDamage(1,time,44,160,running,finished,paused);
  Check(d==(running&&!finished&&!paused&&time>=44&&time<160?1:0),"body damage active during tell/contact but closed outside encounter");
 }
 Check(BodyDamage(-1,60,44,160,true,false,false)==0,"negative damage rejected");
 Check(BodyDamage(std::numeric_limits<float>::infinity(),60,44,160,true,false,false)==0,"nonfinite damage rejected");
 Check(BodyDamage(std::numeric_limits<float>::quiet_NaN(),60,44,160,true,false,false)==0,"NaN rejected");
 float total=0;
 for(int i=0;i<38;++i)total+=BodyDamage(1,44+i*3,44,160,true,false,false);
 Check(total==38,"sustained reload cadence counts beyond the former eight-hit cap");
 Check(!ShowHitInsteadOfTarget(true,true,false),"partial hit never suppresses still-open joint marker");
 Check(ShowHitInsteadOfTarget(false,true,true),"successful interrupt replaces closed marker with confirmation");
 Check(!ShowHitInsteadOfTarget(false,false,false),"no phantom stale hit marker");
 for(float fps:{15.f,30.f,60.f,120.f})for(int scenario:{0,1,2,3}){
  bool applied=false;int impacts=0;float hull=100;
  for(float time=70;time<87;time+=1/fps){
   bool interrupted=scenario==1&&time>=76;
   bool cancelled=scenario==2;
   float dt=scenario==3?0:1/fps;
   if(SlamContact(applied,interrupted,cancelled,dt,time,82)){
    applied=true;++impacts;hull-=1+3*SlamStrength;
   }
  }
  Check(impacts==(scenario==0?1:0),"one contact only; none after interrupt/cancel or on paused seek");
  Check(std::abs(hull-(scenario==0?90:100))<.001,"physical strike strength applies exactly ten hull damage");
 }
 for(float yaw=-40;yaw<=40;yaw+=5)for(int side:{-1,1}){
  // Production rotates both joint and ship battery by the deck transform.
  const float a=yaw*3.14159265f/180;
  const float x=SlamJointForward,y=side*(SlamJointSide-650);
  const float wx=x*std::cos(a)-y*std::sin(a),wy=x*std::sin(a)+y*std::cos(a);
  const float lx=wx*std::cos(a)+wy*std::sin(a),ly=-wx*std::sin(a)+wy*std::cos(a);
  Check(side*ly>100 && std::abs(lx)<side*ly*1.9,"held joint remains in correct battery arc throughout mirrored full helm");
  Check(std::atan2(500.f,std::hypot(lx,ly))*180/3.14159265f<10,"nominal held joint lies below camera elevation margin");
 }
 // For any center-directed mouth shot at feasible ranges, the elevated body
 // envelope cannot be encountered first, including sloped cannon aim.
 for(float range=6000;range<=20500;range+=500)for(float muzzleZ:{400.f,800.f,1100.f}){
  const float dz=650-muzzleZ,bodyZ=650+BodyAboveMouth-muzzleZ;
  const float length=std::hypot(range,dz),projection=(range*range+bodyZ*dz)/length;
  const float distance2=range*range+bodyZ*bodyZ-projection*projection;
  Check(distance2>(BodyRadius+15)*(BodyRadius+15),"body sphere cannot intercept aimed mouth shot");
 }
 std::printf("BOSS_DAMAGE_CPU checks=%d failed=0\n",checks);
}
'''
cpp=out/'probe.cpp';cpp.write_text(source)
vcvars=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat')
batch=out/'build.cmd'
batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{out / "probe.exe"}" /Fo:"{out / "probe.obj"}"\n')
subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
raise SystemExit(subprocess.run([str(out/'probe.exe')],cwd=out).returncode)
