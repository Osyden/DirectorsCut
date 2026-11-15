"""Compile and exercise the real original-triangle BVH, without Unreal or GPU.

Optional --mesh is an NPZ with vertices_cm[N,3] or vertices_m[N,3], plus
triangles[M,3] (uint indices). The named metre key is explicitly scaled by100.
Those source triangles are also compared against an independent brute-force
plane/barycentric oracle. Arrays/transforms are not reconstructed as a grid.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

import numpy as np

sys.dont_write_bytecode = True

HARNESS = r'''
#include "DCCliffTriangleSurface.h"
#include <cstdio>
#include <fstream>
#include <random>
using namespace DCCliffTriangleSurface;
int Failures=0;
void Check(bool Value,const char* Label){std::printf("%s %s\n",Value?"PASS":"FAIL",Label);Failures+=!Value;}
bool Close(double A,double B,double E=1.e-7){return std::abs(A-B)<E;}
// Independent plane intersection and dot-product barycentrics, using long
// double arithmetic; no calls to production TriangleHit or BVH routines.
struct L3 {long double X,Y,Z;L3 operator-(L3 B)const{return {X-B.X,Y-B.Y,Z-B.Z};}
 L3 operator+(L3 B)const{return {X+B.X,Y+B.Y,Z+B.Z};} L3 operator*(long double V)const{return {X*V,Y*V,Z*V};}};
L3 L(FVec3 A){return {A.X,A.Y,A.Z};}
long double DotL(L3 A,L3 B){return A.X*B.X+A.Y*B.Y+A.Z*B.Z;}
L3 CrossL(L3 A,L3 B){return {A.Y*B.Z-A.Z*B.Y,A.Z*B.X-A.X*B.Z,A.X*B.Y-A.Y*B.X};}
bool Oracle(const std::vector<FVec3>& P,const std::vector<std::uint32_t>& T,FVec3 Origin,FVec3 Direction,double& Best){
 const auto O=L(Origin),Raw=L(Direction);const auto D=Raw*(1/std::sqrt(DotL(Raw,Raw)));
 bool Found=false;long double Nearest=INFINITY;
 for(std::size_t I=0;I<T.size();I+=3){
  const auto A=L(P[T[I]]),B=L(P[T[I+1]]),C=L(P[T[I+2]]),AB=B-A,AC=C-A,N=CrossL(AB,AC);
  const long double NN=DotL(N,N),Den=DotL(N,D);if(NN<=1.e-20||std::abs(Den)<1.e-18*std::sqrt(NN))continue;
  const long double Dist=DotL(N,A-O)/Den;if(Dist<0||Dist>Nearest)continue;
  const auto AP=O+D*Dist-A;
  const long double AA=DotL(AB,AB),BB=DotL(AC,AC),CC=DotL(AB,AC),D0=DotL(AP,AB),D1=DotL(AP,AC),Det=AA*BB-CC*CC;
  if(Det<=1.e-20)continue;
  const long double U=(BB*D0-CC*D1)/Det,V=(AA*D1-CC*D0)/Det;
  if(U>=-1.e-12&&V>=-1.e-12&&U+V<=1+1.e-12){Nearest=Dist;Found=true;}
 }
 Best=double(Nearest);return Found;
}
void Compare(FSurface& S,const std::vector<FVec3>& P,const std::vector<std::uint32_t>& T,unsigned Count,std::uint32_t Seed,const char* Label){
 std::mt19937 Rng(Seed);std::uniform_real_distribution<double> U(0,1);
 FVec3 Min={INFINITY,INFINITY,INFINITY},Max={-INFINITY,-INFINITY,-INFINITY};
 for(auto V:P){Min={std::min(Min.X,V.X),std::min(Min.Y,V.Y),std::min(Min.Z,V.Z)};Max={std::max(Max.X,V.X),std::max(Max.Y,V.Y),std::max(Max.Z,V.Z)};}
 const auto Centre=(Min+Max)*.5,Span=Max-Min;const double Scale=std::max({Span.X,Span.Y,Span.Z,1.});
 unsigned Wrong=0,Hits=0;double Error=0;
 for(unsigned I=0;I<Count;++I){
  FVec3 O=Centre+FVec3{U(Rng)-.5,U(Rng)-.5,U(Rng)-.5}*(Scale*2.5);
  const auto Face=std::size_t(Rng()%(T.size()/3))*3;
  const auto Target=(P[T[Face]]+P[T[Face+1]]+P[T[Face+2]])*(1./3.);
  FVec3 D=I%3==0?FVec3{U(Rng)-.5,U(Rng)-.5,U(Rng)-.5}:Target-O;
  FHit H;double Expected=0;const bool A=S.TraceNearest(O,D,H),B=Oracle(P,T,O,D,Expected);
  if(A!=B){++Wrong;continue;}if(A){++Hits;Error=std::max(Error,std::abs(H.DistanceCm-Expected));
   const auto Rebuilt=P[H.Vertices[0]]*H.Barycentric[0]+P[H.Vertices[1]]*H.Barycentric[1]+P[H.Vertices[2]]*H.Barycentric[2];
   Wrong+=std::hypot(Rebuilt.X-H.Position.X,Rebuilt.Y-H.Position.Y,Rebuilt.Z-H.Position.Z)>1.e-5;
  }
 }
 Check(Wrong==0&&Error<1.e-5,Label);std::printf("ORACLE rays=%u hits=%u mismatches=%u max_distance_error_cm=%.12g nodes=%zu triangles=%zu\n",Count,Hits,Wrong,Error,S.NodeCount(),S.TriangleCount());
}
template<class T>bool Read(std::ifstream& F,T* P,std::size_t N=1){return bool(F.read(reinterpret_cast<char*>(P),sizeof(T)*N));}
int main(int argc,char** argv){
 // Two stacked complete square surfaces plus a vertical wall. Every one has
 // source-winding +Z/+X; culling decisions remain explicit even underneath.
 std::vector<FVec3> P={{-100,-100,0},{100,-100,0},{100,100,0},{-100,100,0},
 {-100,-100,300},{100,-100,300},{100,100,300},{-100,100,300},
 {150,-100,0},{150,100,0},{150,100,300},{150,-100,300}, {0,0,50}};
 std::vector<std::uint32_t> T={0,1,2,0,2,3,4,5,6,4,6,7,8,9,10,8,10,11,12,12,12};
 FSurface S;Check(S.Build({P.data(),P.size(),T.data(),T.size()/3})&&S.DegenerateCount()==1,"skip degenerate face while retaining original triangle IDs");
 FHit H;Check(S.TraceNearest({13,7,500},{0,0,-2},H)&&Close(H.DistanceCm,200)&&H.FrontFace&&H.Normal.Z>.99,"top overhang nearest hit and nonunit ray centimetre distance");
 Check(S.TraceNearest({13,7,-200},{0,0,1},H)&&Close(H.DistanceCm,200)&&!H.FrontFace&&H.Normal.Z>.99,"underside two-sided hit preserves source normal");
 Check(S.TraceNearest({13,7,150},{0,0,1},H)&&Close(H.Position.Z,300),"ray between stacked layers reaches underside without XY projection");
 Check(S.TraceNearest({13,7,500},{0,0,-1},H,200.001)&&Close(H.Position.Z,0),"minimum distance can select deeper independent surface");
 Check(!S.TraceNearest({13,7,-200},{0,0,1},H,0,INFINITY,ECullMode::CullBackFaces),"explicit back-face culling rejects underside");
 Check(!S.TraceNearest({13,7,500},{0,0,-1},H,0,INFINITY,ECullMode::CullFrontFaces),"explicit front-face culling rejects top faces");
 Check(S.TraceNearest({500,13,100},{-1,0,0},H)&&Close(H.Position.X,150)&&H.Normal.X>.99,"horizontal ray intersects vertical wall");
 Check(S.TraceNearest({0,0,500},{0,0,-1},H)&&Close(H.Position.Z,300),"shared diagonal edge remains hittable");
 Check(S.TraceNearest({100,100,500},{0,0,-1},H)&&Close(H.Position.Z,300),"outer corner remains hittable");
 Check(!S.TraceNearest({100.01,100.01,500},{0,0,-1},H),"no artificial expansion across open boundary");
 Check(S.TraceNearest({13,7,500},{0,0,-1.e300},H)&&Close(H.DistanceCm,200),"finite huge direction normalizes without overflow");
 Check(S.TraceNearest({13,7,500},{0,0,-1.e-310},H)&&Close(H.DistanceCm,200),"finite subnormal direction normalizes without reciprocal overflow");
 const double Previous=H.DistanceCm;
 Check(!S.TraceNearest({0,0,500},{0,0,0},H)&&!S.TraceNearest({NAN,0,500},{0,0,-1},H)&&!S.TraceNearest({0,0,500},{0,0,-1},H,-1)&&H.DistanceCm==Previous,"bad rays rejected without mutating hit");
 Check(!S.TraceNearest({13,7,500},{0,0,-1},H,0,199.9),"maximum distance excludes later surface");
 Compare(S,P,T,3000,613,"stacked walls BVH matches independent plane oracle");
 auto Bad=T;Bad[0]=99999;Check(!S.Build({P.data(),P.size(),Bad.data(),Bad.size()/3})&&!S.TraceNearest({0,0,500},{0,0,-1},H),"invalid rebuild clears old query surface");
 P[0].X=NAN;Check(!S.Build({P.data(),P.size(),T.data(),T.size()/3}),"nonfinite source rejected");P[0].X=-100;
 // Force a nontrivial tree with disconnected arbitrary 3D triangles.
 P.clear();T.clear();std::mt19937 Rng(129);std::uniform_real_distribution<double> U(-300,300);
 for(unsigned I=0;I<900;++I){P.push_back({U(Rng)-140000,U(Rng)+130000,U(Rng)});T.push_back(I);}
 Check(S.Build({P.data(),P.size(),T.data(),T.size()/3})&&S.NodeCount()>1,"arbitrary negative-coordinate triangle BVH builds");
 Compare(S,P,T,2000,713,"arbitrary 3D BVH matches independent plane oracle");
 if(argc>1){
  std::ifstream F(argv[1],std::ios::binary);std::uint32_t VC=0,TC=0;if(!Read(F,&VC)||!Read(F,&TC))return 2;
  P.resize(VC);T.resize(std::size_t(TC)*3);if(!Read(F,P.data(),VC)||!Read(F,T.data(),T.size()))return 3;
  Check(S.Build({P.data(),P.size(),T.data(),TC}),"actual original outcrop triangles build");
  Compare(S,P,T,256,927,"actual original outcrop BVH matches independent plane oracle");
  double MinX=INFINITY,MaxX=-INFINITY,MinY=INFINITY,MaxY=-INFINITY,MinZ=INFINITY,MaxZ=-INFINITY;
  for(auto Q:P){MinX=std::min(MinX,Q.X);MaxX=std::max(MaxX,Q.X);MinY=std::min(MinY,Q.Y);MaxY=std::max(MaxY,Q.Y);MinZ=std::min(MinZ,Q.Z);MaxZ=std::max(MaxZ,Q.Z);}
  unsigned Multiple=0,ThreeOrMore=0,MaxHits=0,Sampled=0,MaxFrontFaces=0;double MaxSeparation=0;
  for(unsigned Y=0;Y<64;++Y)for(unsigned X=0;X<64;++X){
   const FVec3 O={MinX+(MaxX-MinX)*(X+.371)/64,MinY+(MaxY-MinY)*(Y+.619)/64,MaxZ+100};
   double Min=0,FirstZ=0;unsigned Hits=0,FrontFaces=0;
   for(unsigned I=0;I<32;++I){if(!S.TraceNearest(O,{0,0,-1},H,Min,MaxZ-MinZ+200))break;
    if(!Hits)FirstZ=H.Position.Z;else MaxSeparation=std::max(MaxSeparation,FirstZ-H.Position.Z);
    ++Hits;FrontFaces+=H.FrontFace;Min=H.DistanceCm+.01; // deduplicate coplanar/shared-edge intersections within0.1mm
   }
   Sampled+=Hits>0;Multiple+=Hits>1;ThreeOrMore+=Hits>=3;MaxHits=std::max(MaxHits,Hits);MaxFrontFaces=std::max(MaxFrontFaces,FrontFaces);
  }
  std::printf("ACTUAL_MULTIHIT vertical_grid=4096 covered_rays=%u multiple_surface_rays=%u rays_three_or_more_hits=%u max_hits=%u max_front_faces=%u max_vertical_separation_cm=%.9g degenerates=%zu\n",Sampled,Multiple,ThreeOrMore,MaxHits,MaxFrontFaces,MaxSeparation,S.DegenerateCount());
 }
 return Failures?1:0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mesh", type=Path)
    parser.add_argument("--output", type=Path, help="Isolated results directory; defaults to Saved/CliffTriangleSurfaceTests")
    parser.add_argument("--vcvars", type=Path, default=Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = (args.output or root/"Saved/CliffTriangleSurfaceTests").resolve()
    out.mkdir(parents=True, exist_ok=True)
    binary = None
    provenance = None
    if args.mesh:
        with np.load(args.mesh, allow_pickle=False) as data:
            key = "vertices_cm" if "vertices_cm" in data else "vertices_m"
            vertices = np.asarray(data[key], dtype="<f8")*(100 if key == "vertices_m" else 1)
            raw_triangles = data["triangles"]
            if raw_triangles.dtype.kind not in "iu" or np.any(raw_triangles < 0) or np.any(raw_triangles >= len(vertices)):
                raise ValueError("Triangles must contain integer indices into the original vertex array")
            triangles = np.asarray(raw_triangles, dtype="<u4")
        if vertices.ndim != 2 or vertices.shape[1] != 3 or triangles.ndim != 2 or triangles.shape[1] != 3:
            raise ValueError("Expected vertices_cm[N,3] and triangles[M,3]")
        binary = out/"original_mesh.bin"
        binary.write_bytes(struct.pack("<II",len(vertices),len(triangles))+vertices.tobytes()+triangles.tobytes())
        provenance = {"source_npz": str(args.mesh.resolve()), "source_sha256": hashlib.sha256(args.mesh.read_bytes()).hexdigest(),
                      "source_position_key": key, "scale_to_centimetres": 100 if key == "vertices_m" else 1,
                      "vertices": len(vertices), "triangles": len(triangles),
                      "query_vertices_cm_sha256": hashlib.sha256(vertices.tobytes()).hexdigest(),
                      "source_index_sha256": hashlib.sha256(triangles.tobytes()).hexdigest()}
    cpp, exe = out/"triangle_tests.cpp", out/"triangle_tests.exe"
    cpp.write_text(HARNESS, encoding="utf-8")
    batch = out/"build_triangle_tests.cmd"
    batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\n'
                     f'cl /nologo /std:c++17 /EHsc /O2 /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{exe}" /Fo:"{out / "triangle_tests.obj"}"\n')
    subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
    result = subprocess.run([str(exe)]+([str(binary)] if binary else []), cwd=out, text=True, capture_output=True)
    (out/"results.txt").write_text(result.stdout+result.stderr, encoding="utf-8")
    if provenance:
        (out/"source_provenance.json").write_text(json.dumps(provenance, indent=2)+"\n", encoding="utf-8")
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
