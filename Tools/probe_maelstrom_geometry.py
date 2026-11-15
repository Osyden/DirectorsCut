"""CPU-only probe of the actual basin source functions and sampled mesh edges.

Extracts project functions verbatim into a temporary standalone C++ harness.
Reads the installed engine's Perlin implementation into that temporary file;
no engine/vendor source is copied into the repository. Requires MSVC and the
local Unreal source installation, but does not launch/build Unreal or use GPU.
"""
import argparse
import ast
import pathlib
import re
import subprocess


def block(text, marker):
    start = text.index(marker)
    brace = text.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


SHIM = r"""
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <type_traits>
using int32=int32_t;
#define FORCEINLINE inline
constexpr float UE_PI=3.14159265358979323846f, UE_TWO_PI=6.28318530717958647692f;
struct FVector2D {
 double X=0,Y=0;
 FVector2D()=default;
 FVector2D(double x,double y):X(x),Y(y){}
 FVector2D operator+(FVector2D p)const{return {X+p.X,Y+p.Y};}
 FVector2D operator-(FVector2D p)const{return {X-p.X,Y-p.Y};}
 FVector2D operator*(double s)const{return {X*s,Y*s};}
 FVector2D operator/(double s)const{return {X/s,Y/s};}
 double SizeSquared()const{return X*X+Y*Y;}
 double Size()const{return std::sqrt(SizeSquared());}
 FVector2D GetSafeNormal()const{return Size()>1e-8 ? *this/Size():FVector2D();}
 static double DotProduct(FVector2D a,FVector2D b){return a.X*b.X+a.Y*b.Y;}
};
struct FMath {
 template<class A,class B> static auto Min(A a,B b){using T=std::common_type_t<A,B>;return std::min(T(a),T(b));}
 template<class A,class B> static auto Max(A a,B b){using T=std::common_type_t<A,B>;return std::max(T(a),T(b));}
 template<class A,class B,class C> static auto Clamp(A x,B a,C b){return Min(Max(x,a),b);}
 template<class A,class B,class C> static auto Lerp(A a,B b,C t){return a+(b-a)*t;}
 template<class A> static auto Abs(A x){return std::abs(x);}
 template<class A> static auto Square(A x){return x*x;}
 static float Sin(float x){return std::sin(x);}
 static float Cos(float x){return std::cos(x);}
 static float Exp(float x){return std::exp(x);}
 static float Pow(float x,float y){return std::pow(x,y);}
 static float Sqrt(float x){return std::sqrt(x);}
 static float Atan2(float y,float x){return std::atan2(y,x);}
 static float Fmod(float x,float y){return std::fmod(x,y);}
 static float FloorToFloat(float x){return std::floor(x);}
 static float SmoothStep(float a,float b,float x){float t=Clamp((x-a)/(b-a),0.f,1.f);return t*t*(3-2*t);}
 static float UnwindRadians(float a){while(a>UE_PI)a-=UE_TWO_PI;while(a<-UE_PI)a+=UE_TWO_PI;return a;}
 static float RadiansToDegrees(float a){return a*(180.f/UE_PI);}
 static float FindDeltaAngleDegrees(float a,float b){return RadiansToDegrees(UnwindRadians((b-a)*UE_PI/180.f));}
 static float Sign(float a){return a>0?1.f:a<0?-1.f:0.f;}
 static float FInterpTo(float a,float b,float dt,float speed){return a+(b-a)*Clamp(dt*speed,0.f,1.f);}
 static float PerlinNoise2D(const FVector2D&);
};
template<class A,class B> auto max(A a,B b){return FMath::Max(a,b);}
template<class A,class B> auto min(A a,B b){return FMath::Min(a,b);}
template<class A,class B,class C> auto lerp(A a,B b,C t){return FMath::Lerp(a,b,t);}
float smoothstep(float a,float b,float x){return FMath::SmoothStep(a,b,x);}
float saturate(float x){return FMath::Clamp(x,0.f,1.f);}
float frac(float x){return x-std::floor(x);}
double length(FVector2D p){return p.Size();}
double dot(FVector2D a,FVector2D b){return FVector2D::DotProduct(a,b);}
FVector2D normalize(FVector2D p){return p.GetSafeNormal();}
using float2=FVector2D;
struct FRandomStream {
 uint32_t seed;
 explicit FRandomStream(uint32_t s):seed(s){}
 float FRandRange(float a,float b){seed=seed*196314165U+907633515U;uint32_t bits=0x3f800000U|(seed&0x007fffffU);float f;std::memcpy(&f,&bits,4);return a+(b-a)*(f-1.f);}
};
template<class T> struct TArray:std::vector<T>{void Add(T value){this->push_back(value);}};
struct ADCMaelstrom {
 float Radius=160000.f,Depth=90000.f;
 struct FBreakingWave{FVector2D Center,Along,Across;float Length,Width,Height,Seed;};
 TArray<FBreakingWave> BreakingWaves;
 float GetRimRadius(float)const;
 float GetOverflowAmount(float)const;
 float GetRimJoin(float)const;
 float RimElevation(float)const;
 float BaseHeightAt(const FVector2D&)const;
 float HeightAt(const FVector2D&,bool=true)const;
 float BreakerHeight(const FVector2D&,float* =nullptr)const;
 float WaterEdgeRadius(float)const;
 void BuildBreakers();
};
"""

MAIN = r"""
int main(){
 ADCMaelstrom m;m.BuildBreakers();
 constexpr int sectors=1536,rings=448;
 bool pass=true;
 auto edge=[&](float a,float f){FVector2D p=FVector2D(FMath::Cos(a),FMath::Sin(a))*FMath::Lerp(120.f,m.WaterEdgeRadius(a),f);return std::pair<FVector2D,float>(p,m.HeightAt(p));};
 for(float eps:{.00001f,UE_TWO_PI/sectors}){
   auto l=edge(UE_PI-eps,1),r=edge(UE_PI+eps,1);
   std::printf("inlet epsilon=%.8f rad: radii=%.3f/%.3fm dXY=%.3fm dZ=%.3fm\n",eps,l.first.Size()*.01,r.first.Size()*.01,(r.first-l.first).Size()*.01,std::abs(r.second-l.second)*.01);
   if(eps<.0001f){pass&=(r.first-l.first).Size()<10.f && std::abs(r.second-l.second)<1.f;}
 }
 double worst=0;int worstSide=0,worstRing=0;
 for(int side=0;side<sectors;side++)for(int ring=1;ring<=rings;ring++){
   auto a=edge(side*UE_TWO_PI/sectors,ring/float(rings));
   auto b=edge((side+1)*UE_TWO_PI/sectors,ring/float(rings));
   double length=std::hypot((a.first-b.first).Size(),a.second-b.second);
   if(length>worst){worst=length;worstSide=side;worstRing=ring;}
 }
 std::printf("full mesh longest angular edge=%.3fm at side=%d ring=%d\n",worst*.01,worstSide,worstRing);
 pass&=worst<2500.f;
 float minimum=1e20f;
 for(int t=0;t<=178;t++){
  float required=m.Radius*DCMaelstromRoute::RadialFraction(float(t))+11000*DCMaelstromRoute::WidthScale(float(t))+1500;
  minimum=FMath::Min(minimum,m.WaterEdgeRadius(DCMaelstromRoute::Angle(float(t)))-required);
 }
 std::printf("minimum route/shore clearance=%.3fm; breakers=%zu\n",minimum*.01,m.BreakingWaves.size());
 pass&=minimum>=3499.f;
 auto zero=edge(0,1),tau=edge(UE_TWO_PI,1);
 std::printf("0/2PI duplicate seam dXY=%.6fcm dZ=%.6fcm\n",(zero.first-tau.first).Size(),std::abs(zero.second-tau.second));
 for(float radius:{1.2f,10.f,50.f,100.f,250.f,1000.f}){
   double stretch=0;int stretchSide=0;
   for(float clock:{0.f,3.57f,7.14f,10.f,14.27f})for(int side=0;side<sectors;side++){
     float a=side*UE_TWO_PI/sectors,b=(side+1)*UE_TWO_PI/sectors;
     FVector2D p=FVector2D(std::cos(a),std::sin(a))*radius;
     FVector2D q=FVector2D(std::cos(b),std::sin(b))*radius;
     double ratio=(AdvectedUV(p/100.,clock)-AdvectedUV(q/100.,clock)).Size()/(p-q).Size();
     if(ratio>stretch){stretch=ratio;stretchSide=side;}
   }
   std::printf("flow radius=%.1fm maximum adjacent-sector UV stretch=%.3fx at side=%d\n",radius,stretch,stretchSide);
   // The protected throat must be well conditioned. Outside it the existing
   // prescribed final-route direction still has substantial angular strain;
   // report that separately rather than claiming a whole-basin solver.
   pass&=std::isfinite(stretch) && (radius>250.f || stretch<2.);
 }
 for(float clock:{0.f,3.57f,7.14f,10.f,14.27f}){
   pass&=AdvectedUV({0,0},clock).Size()<.0001;
   FVector2D p(230,-85),expected=p-FVector2D(-18,2)*(frac(clock*.07f)/.07);
   pass&=(AdvectedUV(p/100.,clock,1)-expected).Size()<.001;
 }
 std::printf("Protected pole and unchanged straight-tunnel flow checked across five phases\n");
 std::printf("CPU geometry/flow regression: %s\n",pass?"PASS":"FAIL");
 return pass?0:1;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=pathlib.Path, default=pathlib.Path("E:/Utilities/UE_5.8"))
    parser.add_argument("--vcvars", type=pathlib.Path, default=pathlib.Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"))
    parser.add_argument("--baseline", help="Probe a 7-40 character Git commit hash instead of the working source")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    if args.baseline and not re.fullmatch(r"[0-9a-fA-F]{7,40}", args.baseline):
        parser.error("--baseline must be a Git commit hash")
    def read(relative):
        if args.baseline:
            return subprocess.check_output(["git", "show", args.baseline + ":" + relative], cwd=root).decode("utf-8")
        return (root / relative).read_text(encoding="utf-8")
    source = read("Source/DirectorsCut/DCMaelstrom.cpp")
    terrain = read("Source/DirectorsCut/DCMaelstromTerrain.cpp")
    breakers = read("Source/DirectorsCut/DCMaelstromBreakers.cpp")
    route = read("Source/DirectorsCut/DCMaelstromRoute.h")
    engine = (args.engine / "Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp").read_text(encoding="utf-8")
    code = SHIM + block(engine, "namespace FMathPerlinHelpers") + ";\n"
    code += block(engine, "float FMath::PerlinNoise2D")
    code += block(route, "namespace DCMaelstromRoute")
    if "DCMaelstromBankWaves::" in breakers:
        code = code.replace("using int32=int32_t;", "using int32=int32_t; using uint32=uint32_t;")
        code = code.replace("struct FMath {", "struct FMath { static int32 FloorToInt(float x){return int32(std::floor(x));}")
        code += block(read("Source/DirectorsCut/DCMaelstromBankWaves.h"), "namespace DCMaelstromBankWaves")
    code += block(source, "float RimVariation")
    for name in ("GetRimRadius", "GetOverflowAmount", "GetRimJoin", "RimElevation", "BaseHeightAt", "HeightAt"):
        code += block(source, "float ADCMaelstrom::" + name)
    code += block(terrain, "float ADCMaelstrom::WaterEdgeRadius")
    code += block(breakers, "float ADCMaelstrom::BreakerHeight")
    build = breakers[breakers.index("void ADCMaelstrom::BuildBreakers()"):]
    code += build[:build.index("    for(const FBreakingWave& W:BreakingWaves)")] + "}\n"
    # Probe the actual HLSL flow expression, not a separately maintained formula.
    material = ast.parse(read("Tools/make_maelstrom_materials.py"))
    flow = next(ast.literal_eval(n.value) for n in material.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == "FLOW" for t in n.targets))
    flow = re.sub(r"\.([xy])\b", lambda m: "." + m[1].upper(), flow)
    code += "FVector2D AdvectedUV(FVector2D UV,float T,float Tunnel=0){float Handedness=-1,RadiusM=1600,Flow=1;\n" + flow + "return q;}\n"
    code += MAIN
    out = root / "Saved/GeometryProbe"
    out.mkdir(parents=True, exist_ok=True)
    cpp, exe = out / "basin_probe.cpp", out / "basin_probe.exe"
    cpp.write_text(code, encoding="utf-8")
    command = f'call "{args.vcvars}" >nul && cl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "basin_probe.obj"}"'
    batch = out / "build_probe.cmd"
    batch.write_text("@echo off\n" + command + "\n", encoding="utf-8")
    subprocess.run(f'cmd /d /c "{batch}"', cwd=out, check=True)
    raise SystemExit(subprocess.run([str(exe)], cwd=out).returncode)


if __name__ == "__main__":
    main()
