"""Compile the real pure C++ query header and compare against exported triangles.

No Unreal or GPU. --mesh accepts export_cliff_query_mesh.py's original mesh JSON.
Generated standalone test source/binary/results stay under ignored Saved.
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
#include "DCCliffQueryMesh.h"
#include <cstdio>
#include <fstream>
#include <vector>
using namespace DCCliffQueryMesh;
int Failed=0;
void Check(bool B,const char* Label){std::printf("%s %s\n",B?"PASS":"FAIL",Label);Failed+=!B;}
FPoint At(const FGridView& G,double U,double V,double Z=0){return {
 G.Origin.X+G.CellSizeCm*(G.BasisU.X*U+G.BasisV.X*V),
 G.Origin.Y+G.CellSizeCm*(G.BasisU.Y*U+G.BasisV.Y*V),Z};}
bool Query(const FGridView& G,double U,double V,FHit& H){auto P=At(G,U,V);return QueryHeight(G,P.X,P.Y,H);}
template<class T> bool Read(std::ifstream& F,T* V,std::size_t N=1){return bool(F.read(reinterpret_cast<char*>(V),sizeof(T)*N));}
int main(int argc,char** argv){
 FGridView G;G.Rows=2;G.Columns=2;G.CellSizeCm=200;G.Origin={-34000,-51000,0};
 G.BasisU={std::cos(-.73),std::sin(-.73),0};G.BasisV={-G.BasisU.Y,G.BasisU.X,0};
 std::vector<FPoint> P={At(G,0,0),At(G,1,0),At(G,0,1),At(G,1,1,400)};
 std::vector<std::int32_t> M={0,1,2,3};G.Vertices=P.data();G.VertexCount=P.size();G.GridToVertex=M.data();G.GridCount=M.size();
 FHit H;Check(Validate(G),"rotated negative-arena frame validates in centimetres");
 Check(Query(G,.5,.5,H)&&std::abs(H.HeightCm-200)<1.e-8,"a-d diagonal centre uses triangles, not bilinear height");
 Check(Query(G,.75,.25,H)&&std::abs(H.HeightCm-100)<1.e-8&&H.Triangle==0,"ABD interior barycentric height");
 Check(Query(G,.25,.75,H)&&std::abs(H.HeightCm-100)<1.e-8&&H.Triangle==1,"ADC interior barycentric height");
 Check(H.Normal.Z>0&&std::abs(H.Normal.X*H.Normal.X+H.Normal.Y*H.Normal.Y+H.Normal.Z*H.Normal.Z-1)<1.e-12,"upward unit triangle normal");
 M[1]=-1;
 Check(!Query(G,.75,.25,H)&&Query(G,.25,.75,H),"missing corner removes only its triangle; no padding bridge");
 Check(Query(G,.5,.5,H)&&std::abs(H.HeightCm-200)<1.e-8,"shared diagonal stays queryable when one triangle is missing");
 Check(!Query(G,1,0,H)&&Query(G,1,1,H),"invalid vertex rejected; valid outer corner retained");
 const double Old=H.HeightCm;
 Check(!Query(G,-.001,.3,H)&&H.HeightCm==Old&&!QueryHeight(G,std::numeric_limits<double>::quiet_NaN(),0,H),"outside/NaN rejected without modifying hit");
 M[1]=99;Check(!Validate(G),"corrupt compact vertex index rejected");M[1]=1;
 P[0].Z=std::numeric_limits<double>::infinity();Check(!Validate(G),"non-finite source vertex rejected");P[0].Z=0;
 G.Columns=3;P={At(G,0,0),At(G,1,0),At(G,2,0),At(G,0,1),At(G,1,1,400),At(G,2,1)};
 M={0,1,-1,3,4,-1};G.Vertices=P.data();G.VertexCount=P.size();G.GridToVertex=M.data();G.GridCount=M.size();
 Check(Validate(G)&&Query(G,1,.5,H)&&std::abs(H.HeightCm-200)<1.e-7,"grid edge belongs to valid neighbour when forward cell is absent");
 Check(!Query(G,1.00001,.5,H),"edge tolerance cannot extrapolate into missing adjacent cell");
 if(argc<2){Check(false,"actual mesh fixture required");return 1;}
 std::ifstream F(argv[1],std::ios::binary);std::uint32_t Rows=0,Cols=0,Count=0,FaceCount=0;
 if(!Read(F,&Rows)||!Read(F,&Cols)||!Read(F,&Count)||!Read(F,&FaceCount))return 2;
 G.Rows=Rows;G.Columns=Cols;Read(F,&G.CellSizeCm);Read(F,&G.Origin);Read(F,&G.BasisU);Read(F,&G.BasisV);
 P.resize(Count);M.resize(std::size_t(Rows)*Cols);std::vector<std::uint32_t> T(std::size_t(FaceCount)*3);
 if(!Read(F,P.data(),P.size())||!Read(F,M.data(),M.size())||!Read(F,T.data(),T.size()))return 3;
 G.Vertices=P.data();G.VertexCount=P.size();G.GridToVertex=M.data();G.GridCount=M.size();
 Check(Validate(G),"actual exported original FBX grid validates");
 double MaxError=0;unsigned Missing=0,NormalErrors=0;
 for(std::size_t Face=0;Face<FaceCount;++Face){
  const auto& A=P[T[Face*3]];const auto& B=P[T[Face*3+1]];const auto& C=P[T[Face*3+2]];
  for(int Sample=0;Sample<2;++Sample){
   const double W0=Sample==0?1./3.:.2,W1=Sample==0?1./3.:.3,W2=1-W0-W1;
   const double X=A.X*W0+B.X*W1+C.X*W2,Y=A.Y*W0+B.Y*W1+C.Y*W2,Z=A.Z*W0+B.Z*W1+C.Z*W2;
   if(!QueryHeight(G,X,Y,H)){++Missing;continue;}
   MaxError=std::max(MaxError,std::abs(H.HeightCm-Z));
   const FPoint U={B.X-A.X,B.Y-A.Y,B.Z-A.Z},V={C.X-A.X,C.Y-A.Y,C.Z-A.Z};
   FPoint N={U.Y*V.Z-U.Z*V.Y,U.Z*V.X-U.X*V.Z,U.X*V.Y-U.Y*V.X};
   const double L=std::sqrt(N.X*N.X+N.Y*N.Y+N.Z*N.Z);
   const double Dot=(N.X*H.Normal.X+N.Y*H.Normal.Y+N.Z*H.Normal.Z)/L;
   NormalErrors+=Dot<1-1.e-10;
  }
 }
 Check(Missing==0&&MaxError<1.e-6,"all original triangle centres/interiors match exact source height");
 Check(NormalErrors==0,"all original triangle interior normals match source face");
 unsigned VertexMissing=0;
 for(const auto& Q:P)VertexMissing+=!QueryHeight(G,Q.X,Q.Y,H)||std::abs(H.HeightCm-Q.Z)>1.e-5;
 Check(VertexMissing==0,"all original used vertices including ragged boundary remain queryable");
 unsigned PaddingWrong=0,PaddingTests=0;
 for(unsigned R=0;R<Rows-1;++R)for(unsigned C=0;C<Cols-1;++C){
  auto A=std::size_t(R)*Cols+C,B=A+1,D=A+Cols+1,CC=A+Cols;
  if(M[A]<0||M[B]<0||M[D]<0){++PaddingTests;PaddingWrong+=Query(G,C+2./3.,R+1./3.,H);}
  if(M[A]<0||M[D]<0||M[CC]<0){++PaddingTests;PaddingWrong+=Query(G,C+1./3.,R+2./3.,H);}
 }
 Check(PaddingWrong==0&&PaddingTests>0,"all absent-triangle centres reject rather than bridge padding");
 std::printf("ACTUAL vertices=%u triangles=%u samples=%u max_height_error_cm=%.12g missing=%u normal_errors=%u boundary_vertex_failures=%u absent_triangle_checks=%u\n",Count,FaceCount,FaceCount*2,MaxError,Missing,NormalErrors,VertexMissing,PaddingTests);
 return Failed?1:0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mesh", required=True, type=Path)
    parser.add_argument("--vcvars", type=Path, default=Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = root / "Saved/CliffQuery/Tests"
    out.mkdir(parents=True, exist_ok=True)
    mesh = json.loads(args.mesh.read_text(encoding="utf-8"))
    if mesh["schema"] != "directorscut.cliff_query_mesh.v1":
        raise ValueError("Unsupported query mesh schema")
    values = {}
    for key, dtype in (("vertices_arena_cm", "<f8"), ("triangles", "<u4"), ("uv0", "<f4"), ("grid_to_vertex", "<i4")):
        values[key] = np.asarray(mesh[key], dtype=dtype)
        if hashlib.sha256(values[key].tobytes()).hexdigest() != mesh["array_sha256"][key]:
            raise ValueError("JSON roundtrip changed numeric array: "+key)
    if hashlib.sha256(b"".join(a.tobytes() for a in values.values())).hexdigest() != mesh["geometry_sha256"]:
        raise ValueError("Combined geometry hash mismatch")
    binary = out / "source_mesh.bin"
    with binary.open("wb") as stream:
        stream.write(struct.pack("<IIIId", mesh["rows"], mesh["columns"], mesh["vertices"], mesh["triangle_count"], mesh["cell_size_cm"]))
        for key in ("frame_origin_arena_cm", "basis_u_arena", "basis_v_arena"):
            stream.write(np.asarray(mesh[key], dtype="<f8").tobytes())
        for key in ("vertices_arena_cm", "grid_to_vertex", "triangles"):
            stream.write(values[key].tobytes())
    cpp, exe = out / "query_tests.cpp", out / "query_tests.exe"
    cpp.write_text(HARNESS, encoding="utf-8")
    batch = out / "build_query_tests.cmd"
    batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\n'
                     f'cl /nologo /std:c++17 /EHsc /O2 /I"{root / "Source/DirectorsCut"}" "{cpp}" /Fe:"{exe}" /Fo:"{out / "query_tests.obj"}"\n')
    subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
    result = subprocess.run([str(exe), str(binary)], cwd=out, capture_output=True, text=True)
    (out / "results.txt").write_text(result.stdout+result.stderr, encoding="utf-8")
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
