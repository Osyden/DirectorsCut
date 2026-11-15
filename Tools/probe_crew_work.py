"""CPU-only contact-timing regression for the production crew stroke policy.

No Unreal build, editor or GPU. Compiles a tiny standalone MSVC executable.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "Saved/CrewWorkProbe"
out.mkdir(parents=True, exist_ok=True)
cpp = out / "work.cpp"
cpp.write_text('#include "' + (root / 'Source/DirectorsCut/DCCrewWorkPolicy.h').as_posix() + '"\n' + r'''
#include <cstdio>
#include <cmath>
#include <initializer_list>
int main() {
    int failures=0, checks=0;
    auto check=[&](bool ok,const char* name){++checks;if(!ok){++failures;std::printf("FAIL %s\n",name);}};
    using namespace DCCrewWorkPolicy;
    for(float dt:{1.f/120,1.f/60,1.f/30,.16f,.75f,2.f}) {
        int hits=0; float age=0;
        while(age<Duration) {float next=age+dt; hits+=CrossedContact(age,next); age=next;}
        check(hits==1,"one contact per stroke, including a hitch past recovery");
        check(!CrossedContact(age,age+dt),"recovery cannot repeat contact");
    }
    check(!CrossedContact(0.f,.2f),"cancellation during windup applies no suppression");
    check(CrossedContact(.2f,.43f),"downstroke crosses contact");
    check(!CrossedContact(Contact,Contact+.1f),"boundary is consumed once");
    check(CrossedContact(Contact-.001f,Contact),"exact contact boundary counts");
    check(!CrossedContact(10.f,10.1f),"cancelled or unassigned crew cannot hit");
    int suppression=0;
    for(int stroke=0;stroke<3;++stroke) for(float age=0;age<Duration;age+=1.f/60)
        suppression+=CrossedContact(age,age+1.f/60);
    check(suppression==3,"three fresh strokes provide exactly three suppression contacts");
    check(PoseWeight(0.f)==0.f && PoseWeight(Duration)==0.f,"stroke endpoints preserve the idle pose");
    check(PoseWeight(-1.f)==0.f && PoseWeight(10.f)==0.f,"cancelled/inactive pose has no arm offset");
    check(PoseWeight(Contact)==1.f,"full visible stroke at the unchanged suppression contact");
    for(float age=0.f;age<Duration;age+=.001f) {
        const float w=PoseWeight(age), next=PoseWeight(age+.001f);
        check(w>=0.f && w<=1.f,"pose interpolation never extrapolates the joints");
        check(std::abs(next-w)<.013f,"final joint blend envelope has no frame-edge snap");
    }
    check(PoseWeight(.00001f)<.000001f && PoseWeight(Duration-.00001f)<.000001f,
          "both sides of the active/inactive branch converge to idle");
    std::printf("CREW_WORK_CPU checks=%d failed=%d\n",checks,failures);
    return failures?1:0;
}
''', encoding="utf-8")
exe = out / "work.exe"
vcvars = Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat")
batch = out / "build.cmd"
batch.write_text('@echo off\ncall "' + str(vcvars) + '" >nul\ncl /nologo /std:c++17 /EHsc /O2 "'
                 + str(cpp) + '" /Fe:"' + str(exe) + '" /Fo:"' + str(out / "work.obj") + '"\n', encoding="utf-8")
subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
raise SystemExit(subprocess.run([str(exe)], cwd=out).returncode)
