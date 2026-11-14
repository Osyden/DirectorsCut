"""Run the actual cliff automation-test bodies in a standalone CPU harness.

Requires MSVC, not Unreal or GPU. Reuses the contact probe's small math shim;
no production geology formulas are copied. Generated files stay under Saved.
"""
import argparse
import pathlib
import subprocess
import sys

sys.dont_write_bytecode = True
from probe_waterfall_contacts import make_shim
from probe_maelstrom_geometry import block


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vcvars", type=pathlib.Path, default=pathlib.Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"))
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    source = root / "Source/DirectorsCut"
    shim = make_shim((source / "DCMaelstrom.h").read_text())
    shim = shim.replace(" static const FVector ZeroVector;", r"""
 double Size()const{return std::sqrt(X*X+Y*Y+Z*Z);}
 bool ContainsNaN()const{return !std::isfinite(X)||!std::isfinite(Y)||!std::isfinite(Z);}
 FVector GetSafeNormal()const{return Size()>1e-8?*this*(1/Size()):FVector();}
 static double Dist(FVector a,FVector b){return (a-b).Size();}
 static double DotProduct(FVector a,FVector b){return a.X*b.X+a.Y*b.Y+a.Z*b.Z;}
 static FVector CrossProduct(FVector a,FVector b){return {a.Y*b.Z-a.Z*b.Y,a.Z*b.X-a.X*b.Z,a.X*b.Y-a.Y*b.X};}
 static const FVector ZeroVector;
""")
    tests = (source / "DCCliffGeneratorTests.cpp").read_text()
    tests = "\n".join(line for line in tests.splitlines() if not line.startswith("#include"))
    code = shim + block((source / "DCCliffGenerator.h").read_text(), "namespace DCCliffGenerator")
    code += r"""
#include <cfloat>
#define MAX_flt FLT_MAX
#define WITH_DEV_AUTOMATION_TESTS 1
#define TEXT(x) x
using FString=int;
bool AllPassed=true;
struct ProbeTest {
 void TestTrue(const char* title,bool pass){std::printf("%s %s\n",pass?"PASS":"FAIL",title);AllPassed&=pass;}
 void TestFalse(const char* title,bool value){TestTrue(title,!value);}
};
#define IMPLEMENT_SIMPLE_AUTOMATION_TEST(Name,Path,Flags) struct Name:ProbeTest{bool RunTest(const FString&);};
""" + tests + r"""
int main(){
 FDCCliffGeneratorTest().RunTest(0);
 FDCCliffWaterContactTest().RunTest(0);
 FDCFirstReceivingWaterTest().RunTest(0);
 return AllPassed?0:1;
}
"""
    out = root / "Saved/GeometryProbe/CliffTests"
    out.mkdir(parents=True, exist_ok=True)
    cpp, exe = out / "cliff_tests.cpp", out / "cliff_tests.exe"
    cpp.write_text(code)
    batch = out / "build_probe.cmd"
    batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\n'
                     f'cl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "cliff_tests.obj"}"\n')
    subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
    result = subprocess.run([str(exe)], cwd=out, text=True, capture_output=True)
    (out / "results.txt").write_text(result.stdout + result.stderr)
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
