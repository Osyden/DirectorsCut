"""CPU regression for actual cliff/receiving-water and waterfall core contacts.

Extracts production C++ functions verbatim; reuses the basin probe's small UE
math shim. Engine Perlin source is read locally into ignored Saved output only.
Requires MSVC and installed Unreal source. Does not build/run Unreal or use GPU.

Checks every enabled feed at its centre: rendered-rock support at the receiving
water edge, first covered rock intersection, and first covered core intersection.
Uses default actor dimensions and mean water. This does not test shader WPO,
individual lateral branch clipping, generated VDBs, or visual quality.

Example: python Tools/probe_waterfall_contacts.py
Compare a previous commit with --baseline <commit>; a bad join returns exit 1.
"""

import argparse
import pathlib
import re
import subprocess
import sys

sys.dont_write_bytecode = True
from probe_maelstrom_geometry import SHIM, block


VECTOR_SHIM = r"""
struct FVector {
 double X=0,Y=0,Z=0;
 FVector()=default;
 FVector(double x,double y,double z):X(x),Y(y),Z(z){}
 FVector(FVector2D p,double z):X(p.X),Y(p.Y),Z(z){}
 FVector operator+(FVector p)const{return {X+p.X,Y+p.Y,Z+p.Z};}
 FVector operator-(FVector p)const{return {X-p.X,Y-p.Y,Z-p.Z};}
 FVector operator*(double t)const{return {X*t,Y*t,Z*t};}
 double Size2D()const{return std::hypot(X,Y);}
 static const FVector ZeroVector;
};
const FVector FVector::ZeroVector{};
"""

MESH_SHIM = r"""
struct FProcMeshVertex { FVector Position; };
struct FProcMeshSection { TArray<FProcMeshVertex> ProcVertexBuffer; };
struct ProbeCliffs {
 FProcMeshSection Section;
 const FProcMeshSection* GetProcMeshSection(int)const{return &Section;}
};
"""

MAIN = r"""
int main(){
 ADCMaelstrom m;
 ProbeCliffs cliffs;
 m.Cliffs=&cliffs;
 // Same vertex grid as BuildTerrain; surface sampling and triangle diagonal
 // come from the actual DCCliffGenerator::SampleRenderedSurface implementation.
 for(int row=0;row<=DCCliffGenerator::MeshRows;++row)
  for(int side=0;side<=DCCliffGenerator::MeshSectors;++side)
   cliffs.Section.ProcVertexBuffer.Add({m.CliffSurfacePoint(
    side*UE_TWO_PI/DCCliffGenerator::MeshSectors,
    row/float(DCCliffGenerator::MeshRows))});
 const auto vertex=[&](int side,int row){return cliffs.Section.ProcVertexBuffer[
  row*(DCCliffGenerator::MeshSectors+1)+side].Position;};
 const auto water=[&](FVector2D p,float& h){return m.SampleReceivingWater(p,h);};
 const auto margin=[&](FVector p){return m.WaterEdgeRadius(FMath::Atan2(p.Y,p.X))-p.Size2D();};
 bool pass=true;
 int feeds=0;
 for(int feed=0;feed<m.WaterfallCount;++feed){
  if(m.WaterfallWidth(feed)<=0.f)continue;
  ++feeds;
  const float angle=m.WaterfallAngle(feed);
  const auto corePath=[&](float t){return m.SampleWaterfallCorePoint(angle,.84f*(1.f-t));};
  const auto rockPath=[&](float t){return DCCliffGenerator::SampleRenderedSurface(angle,.2f*(1.f-t),vertex);};
  const auto core=DCCliffGenerator::FirstWaterContact(corePath,water);
  const auto rock=DCCliffGenerator::FirstWaterContact(rockPath,water);
  float coreHeight=0,rockHeight=0;
  const bool coreCovered=core.bFound && water(FVector2D(core.Position),coreHeight);
  const bool rockCovered=rock.bFound && water(FVector2D(rock.Position),rockHeight);
  const double coreDepth=coreHeight-core.Position.Z,rockDepth=rockHeight-rock.Position.Z;
  bool feedPass=coreCovered && rockCovered
   && std::abs(coreDepth)<=1. && std::abs(rockDepth)<=1.;

  // Locate the actual rendered rock where it crosses the radial water boundary.
  // Keep the inner bracket for a covered mean-water query, never extrapolate.
  float lo=0.f,hi=.2f,unused=0.f;
  const auto rendered=[&](float v){return DCCliffGenerator::SampleRenderedSurface(angle,v,vertex);};
  const bool bracketed=water(FVector2D(rendered(lo)),unused) && !water(FVector2D(rendered(hi)),unused);
  for(int k=0;k<30 && bracketed;++k){
   const float mid=(lo+hi)*.5f;
   if(water(FVector2D(rendered(mid)),unused))lo=mid;else hi=mid;
  }
  const FVector edge=rendered(lo);
  float edgeHeight=0.f;
  const bool edgeCovered=bracketed && water(FVector2D(edge),edgeHeight);
  const double rockHeadroom=edge.Z-edgeHeight;
  const bool edgeSupported=edgeCovered && std::isfinite(rockHeadroom) && rockHeadroom>=0.;
  feedPass&=edgeSupported;
  pass&=feedPass;
  std::printf("feed=%d %s core_found=%d covered=%d V=%.7f depth=%.3fcm margin=%.3fcm "
   "rock_found=%d covered=%d depth=%.3fcm margin=%.3fcm edge_supported=%d headroom=%.3fcm\n",
   feed,feedPass?"PASS":"FAIL",core.bFound,coreCovered,.84f*(1.f-core.Progress),coreDepth,
   margin(core.Position),rock.bFound,rockCovered,rockDepth,margin(rock.Position),
   edgeSupported,rockHeadroom);
 }
 pass&=feeds>0;
 std::printf("CPU actual-rock/core contact regression: %s (%d enabled feeds; 1cm depth tolerance)\n",
  pass?"PASS":"FAIL",feeds);
 return pass?0:1;
}
"""


def replace_once(text, old, new):
    """Fail explicitly if the shared shim changes instead of silently diverging."""
    if text.count(old) != 1:
        raise ValueError(f"Shared probe shim changed; expected one {old!r}")
    return text.replace(old, new, 1)


def make_shim(header):
    shim = replace_once(SHIM, "using int32=int32_t;", "using int32=int32_t; using uint32=uint32_t;\n#define UE_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))")
    shim = replace_once(shim, "FVector2D()=default;", "FVector2D()=default; template<class T> explicit FVector2D(const T& p):X(p.X),Y(p.Y){}")
    shim = replace_once(shim, "struct FMath {", VECTOR_SHIM + "struct FMath {\n static int FloorToInt(float x){return int(std::floor(x));} static float Frac(float x){return x-std::floor(x);} static float DegreesToRadians(float x){return x*UE_PI/180.f;}")
    shim = replace_once(shim, "void Add(T value)", "int Num()const{return int(this->size());} void Add(T value)")
    shim = replace_once(shim, "struct ADCMaelstrom {", MESH_SHIM + "struct ADCMaelstrom {")
    # Keep actor defaults and enabled-feed bound sourced from the real header.
    defaults = [re.search(r"\bfloat\s+" + name + r"\s*=\s*([^;]+);", header).group(1)
                for name in ("Radius", "Depth")]
    count = re.search(r"static constexpr int32 WaterfallCount\s*=\s*[^;]+;", header).group(0)
    shim = replace_once(shim, "float Radius=160000.f,Depth=90000.f;", f"float Radius={defaults[0]},Depth={defaults[1]};")
    return replace_once(shim, " void BuildBreakers();", " " + count + r"""
 ProbeCliffs* Cliffs=nullptr;
 float WaterfallAngle(int32)const;
 float WaterfallWidth(int32)const;
 float RimGroundHeight(float,float)const;
 FVector CliffSurfacePoint(float,float)const;
 bool SampleReceivingWater(const FVector2D&,float&)const;
 FVector SampleWaterfallCorePoint(float,float,float=0.f)const;
 void BuildBreakers();
""")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=pathlib.Path, default=pathlib.Path("E:/Utilities/UE_5.8"))
    parser.add_argument("--vcvars", type=pathlib.Path, default=pathlib.Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"))
    parser.add_argument("--baseline", help="Probe a 7-40 character Git commit hash instead of working source")
    args = parser.parse_args()
    if args.baseline and not re.fullmatch(r"[0-9a-fA-F]{7,40}", args.baseline):
        parser.error("--baseline must be a Git commit hash")
    engine_path = args.engine / "Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp"
    for required in (engine_path, args.vcvars):
        if not required.is_file():
            parser.error(f"Required local tool/source missing: {required}")
    root = pathlib.Path(__file__).resolve().parents[1]

    def read(relative):
        if args.baseline:
            return subprocess.check_output(["git", "show", args.baseline + ":" + relative], cwd=root).decode("utf-8")
        return (root / relative).read_text(encoding="utf-8")

    source = read("Source/DirectorsCut/DCMaelstrom.cpp")
    terrain = read("Source/DirectorsCut/DCMaelstromTerrain.cpp")
    whitewater = read("Source/DirectorsCut/DCMaelstromWhitewater.cpp")
    engine = engine_path.read_text(encoding="utf-8")
    code = make_shim(read("Source/DirectorsCut/DCMaelstrom.h"))
    code += block(engine, "namespace FMathPerlinHelpers") + ";\n" + block(engine, "float FMath::PerlinNoise2D")
    code += block(read("Source/DirectorsCut/DCMaelstromRoute.h"), "namespace DCMaelstromRoute")
    code += block(read("Source/DirectorsCut/DCCliffGenerator.h"), "namespace DCCliffGenerator")
    code += block(source, "float RimVariation")
    for name in ("GetRimRadius", "GetOverflowAmount", "GetRimJoin", "RimElevation", "BaseHeightAt", "HeightAt", "RimGroundHeight"):
        code += block(source, "float ADCMaelstrom::" + name)
    for name in ("WaterEdgeRadius", "WaterfallAngle", "WaterfallWidth"):
        code += block(terrain, "float ADCMaelstrom::" + name)
    code += block(terrain, "FVector ADCMaelstrom::CliffSurfacePoint")
    code += block(whitewater, "bool ADCMaelstrom::SampleReceivingWater")
    code += block(whitewater, "FVector ADCMaelstrom::SampleWaterfallCorePoint")
    code += block(read("Source/DirectorsCut/DCMaelstromBreakers.cpp"), "float ADCMaelstrom::BreakerHeight")
    code += MAIN
    out = root / "Saved/GeometryProbe/WaterfallContacts"
    out.mkdir(parents=True, exist_ok=True)
    cpp, exe = out / "waterfall_contacts.cpp", out / "waterfall_contacts.exe"
    cpp.write_text(code, encoding="utf-8")
    batch = out / "build_probe.cmd"
    batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\n'
                     f'cl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "waterfall_contacts.obj"}"\n', encoding="utf-8")
    print(f"Contact source: {args.baseline or 'working tree'}; generated files: {out}", flush=True)
    subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
    result = subprocess.run([str(exe)], cwd=out, text=True, capture_output=True)
    (out / "results.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
