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
 int FirstKills=0;
 for(int Roll=0;Roll<100;++Roll){
  float Health=CreatureHealth;int Hits=0,Score=0;
  float Cost=CreatureHitCost(Hits++,Roll);
  Health-=std::min(Health,Cost);
  if(Health<=0){++FirstKills;Score+=100;}
  Check((Health<=0)==(Roll<33),"exact first-hit percentile boundary");
  if(Health>0){
   Check(Health==.5f,"first nonfatal hit leaves exactly one cannon hit");
   for(int IgnoredRoll=0;IgnoredRoll<100;++IgnoredRoll)
    Check(CreatureHitCost(Hits,IgnoredRoll)==1.f,"second hit guaranteed irrespective of random input");
   Health-=std::min(Health,CreatureHitCost(Hits++,99));Score+=100;
  }
  Check(Health==0 && Hits>=1 && Hits<=2,"normalized treadmill clamp kills in one or two hits");
  Check(Score==100,"one kill score per creature");
 }
 Check(FirstKills==33,"exactly 33 percent first-hit outcomes");
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
