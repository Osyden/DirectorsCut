"""Compile and exercise production creature hit policy without Unreal."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/"Saved/CreatureDamageProbe"
out.mkdir(parents=True,exist_ok=True)
source=r'''
#include "DCEarlyEncounterPolicy.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
int Checks=0;
void Check(bool Good,const char* What){++Checks;if(!Good){std::printf("FAIL: %s\n",What);std::exit(1);}}
int main(){
 using namespace DCEarlyEncounterPolicy;
 int Kills[3]={};
 // Enumerate independent percentile pairs: 25%,18.75%,56.25% kills by hit.
 for(int First=0;First<100;++First)for(int Second=0;Second<100;++Second){
  float Health=CreatureHealth;int Hits=0,Score=0;
  const int Rolls[]={First,Second,99};
  while(Health>0 && Hits<3){
   const float Cost=CreatureHitCost(Hits,Rolls[Hits]);++Hits;
   Check(Cost>0 && Cost<=1.f,"cost respects real treadmill damage clamp");
   Health-=std::min(Health,Cost);
   if(Health<=0){++Kills[Hits-1];Score+=100;}
  }
  Check(Health==0 && Hits>=1 && Hits<=3,"normalized real health budget kills within three shots");
  Check(Score==100,"one kill score per creature");
 }
 Check(Kills[0]==2500 && Kills[1]==1875 && Kills[2]==5625,"independent per-shot kill distribution");
 for(int Previous=0;Previous<2;++Previous)for(int Roll=0;Roll<100;++Roll)
  Check((CreatureHitCost(Previous,Roll)==CreatureHealth)==(Roll<25),"exact 25 percent threshold on first AND second shots");
 for(int Roll=0;Roll<100;++Roll)
  Check(CreatureHitCost(2,Roll)==CreatureHealth,"third hit guarantees death without float residual");
 Check(StatueHealth==4.f,"statues retain four cannon hits");
 std::printf("PASS: %d regular-creature damage policy checks\n",Checks);
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
