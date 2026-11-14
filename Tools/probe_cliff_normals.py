"""CPU-test the actual BuildTerrain crease split on the production cliff grid.

Checks identical triangle positions/winding, unchanged canonical sampler grid,
seam and cap/shore smoothing, valid normals, and measured vertex overhead.
Reuses the production-source contact harness; no Unreal or GPU work.
"""
import pathlib
import sys

sys.dont_write_bytecode = True
import probe_waterfall_contacts as contact

root = pathlib.Path(__file__).resolve().parents[1]
terrain = (root / "Source/DirectorsCut/DCMaelstromTerrain.cpp").read_text()
algorithm = terrain.split("// BEGIN_CLIFF_CREASE_NORMALS", 1)[1].split("\n", 1)[1].split("// END_CLIFF_CREASE_NORMALS", 1)[0]
make_base_shim = contact.make_shim


def make_shim(header):
    shim = make_base_shim(header)
    shim = contact.replace_once(shim, "void Add(T value){this->push_back(value);}",
        "int Add(T value){this->push_back(value);return int(this->size())-1;} "
        "void SetNum(int n){this->resize(n);} void Init(T value,int n){this->assign(n,value);}")
    return contact.replace_once(shim, " static const FVector ZeroVector;", r"""
 FVector& operator+=(FVector p){X+=p.X;Y+=p.Y;Z+=p.Z;return *this;}
 double Size()const{return std::sqrt(X*X+Y*Y+Z*Z);}
 FVector GetSafeNormal()const{return Size()>1e-8?*this*(1/Size()):FVector();}
 void Normalize(){*this=GetSafeNormal();}
 static double DotProduct(FVector a,FVector b){return a.X*b.X+a.Y*b.Y+a.Z*b.Z;}
 static FVector CrossProduct(FVector a,FVector b){return {a.Y*b.Z-a.Z*b.Y,a.Z*b.X-a.X*b.Z,a.X*b.Y-a.Y*b.X};}
 static const FVector ZeroVector;
""") + '\n#include <cassert>\n#define check(x) assert(x)\nconstexpr int INDEX_NONE=-1;\n'


contact.make_shim = make_shim
contact.MAIN = contact.MAIN.split(" const auto vertex=", 1)[0] + r"""
 constexpr int32 Sectors=DCCliffGenerator::MeshSectors,Rows=DCCliffGenerator::MeshRows;
 TArray<FVector> V,N; TArray<int32> I;
 for(const auto& vertex:cliffs.Section.ProcVertexBuffer){V.Add(vertex.Position);N.Add(FVector::ZeroVector);}
 for(int row=0;row<Rows;++row)for(int side=0;side<Sectors;++side){
  const int b=row*(Sectors+1)+side,d=b+Sectors+1;
  for(int index:{b,b+1,d,b+1,d+1,d})I.Add(index);
 }
""" + algorithm + r"""
 bool exact=true,unit=true,seam=true,smooth=true;
 int sc=0,cc=0;
 for(int f=0;f<FaceCount;++f){
  bool split=Resolved[f*3]>=GridCount||Resolved[f*3+1]>=GridCount||Resolved[f*3+2]>=GridCount;
  for(int c=0;c<3;++c){
   int original=split?CreaseSources[CreaseIndices[cc++]]:SmoothIndices[sc++];
   exact&=original==I[f*3+c];
  }
 }
 for(int v=0;v<GridCount;++v){
  exact&=(V[v]-cliffs.Section.ProcVertexBuffer[v].Position).Size()==0;
  unit&=std::abs(N[v].Size()-1.)<1e-6;
  const float t=float(v/(Sectors+1))/Rows;
  if((t<=.20f||t>=.84f)&&Fans[v].Count){
   FVector original=FVector::ZeroVector;
   for(int k=0;k<Fans[v].Count;++k)original+=Weighted[Fans[v].Faces[k]];
   smooth&=(original.GetSafeNormal()-N[v]).Size()<1e-6;
  }
 }
 for(auto n:CreaseNormals)unit&=std::abs(n.Size()-1.)<1e-6;
 for(int r=0;r<=Rows;++r)seam&=(N[r*(Sectors+1)]-N[r*(Sectors+1)+Sectors]).Size()<1e-9;
 double overhead=100.*CreaseSources.Num()/GridCount;
 bool pass=exact&&unit&&seam&&smooth&&SplitFans>0&&overhead<25.;
 std::printf("canonical_vertices=%d total_triangles=%d split_fans=%d extra_groups=%d section1_vertices=%d overhead=%.3f%% section1_triangles=%d\n",
  GridCount,FaceCount,SplitFans,ExtraOriginal.Num(),CreaseSources.Num(),overhead,CreaseIndices.Num()/3);
 std::printf("exact_triangle_positions_winding=%d canonical_grid_unchanged=%d unit_normals=%d seam=%d cap_shore_smoothing=%d\n",
  exact,V.Num()==(Sectors+1)*(Rows+1),unit,seam,smooth);
 std::printf("CPU crease normals regression: %s\n",pass?"PASS":"FAIL");
 return pass?0:1;
}
"""

if __name__ == "__main__":
    if "--output" not in sys.argv:
        sys.argv += ["--output", str(root / "Saved/GeometryProbe/CliffNormals")]
    contact.main()
