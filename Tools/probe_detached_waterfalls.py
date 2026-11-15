"""CPU-only test of production ballistic helper against actual generated cliff.

Uses the existing contact probe/MSVC shim, never Unreal or the GPU. Executes the
real DCWaterfallTrajectory.h functions, checks all accepted gaps and preserved
first contact, and writes ignored Saved/GeometryProbe output. Existing supported
shoreline/default-contact tests also run. Optional --baseline follows that probe.
"""
import pathlib
import sys
sys.dont_write_bytecode=True
import probe_waterfall_contacts as contact

original_make_shim=contact.make_shim

def make_shim(header):
    code=original_make_shim(header)
    code=code.replace("double Size2D()const", """FVector operator/(double t)const{return {X/t,Y/t,Z/t};}
 FVector GetSafeNormal()const{double l=std::sqrt(X*X+Y*Y+Z*Z);return l>1e-9?*this/l:FVector{};}
 bool ContainsNaN()const{return !std::isfinite(X)||!std::isfinite(Y)||!std::isfinite(Z);}
 static double DotProduct(FVector a,FVector b){return a.X*b.X+a.Y*b.Y+a.Z*b.Z;}
 double Size2D()const""")
    code=code.replace("void Add(T value)", """template<class F> void Sort(F f){std::sort(this->begin(),this->end(),f);}
 template<class F> bool ContainsByPredicate(F f)const{for(const auto& p:*this)if(f(p))return true;return false;}
 template<class F> const T* FindByPredicate(F f)const{for(const auto& p:*this)if(f(p))return &p;return nullptr;}
 void Add(T value)""")
    code="#include <algorithm>\n#include <cfloat>\n#define MAX_flt FLT_MAX\n"+code
    return code

contact.make_shim=make_shim
helper=(pathlib.Path(__file__).resolve().parents[1]/"Source/DirectorsCut/DCWaterfallTrajectory.h").read_text()
extra=r"""
 if(feed==8){
  const auto surface=[&](float v,float side){return m.SampleWaterfallCorePoint(angle,v,side);};
  const auto gaps=DCWaterfallTrajectory::Build(surface,water,m.WaterfallWidth(feed),core.WaterHeight);
  bool physics=!gaps.empty();
  for(auto gap:gaps){
   physics &= DCWaterfallTrajectory::Validate(gap,surface,water,m.WaterfallWidth(feed));
   std::printf("BALLISTIC V=%.6f..%.6f time=%.3fs down=%.3fm/s peak_gap=%.3fm minimum=%.4fcm\n",
    gap.StartV,gap.EndV,gap.Duration,gap.InitialDown*.01f,gap.PeakSeparation*.01f,gap.MinClearance);
   for(int lane=0;lane<9;++lane){
    const float across=m.WaterfallWidth(feed)*(lane/4.f-1.f);
    const FVector a=surface(gap.StartV,across),b=surface(gap.EndV,across);
    const float duration=DCWaterfallTrajectory::TimeForDrop(a.Z-b.Z,gap.InitialDown);
    const double vx=(b.X-a.X)/duration,vy=(b.Y-a.Y)/duration;
    for(int i=0;i<=128;++i){
     float v=gap.StartV+(gap.EndV-gap.StartV)*i/128.f;
     const FVector p=DCWaterfallTrajectory::Sample(gap,surface,v,across);
     const float t=DCWaterfallTrajectory::TimeForDrop(a.Z-p.Z,gap.InitialDown);
     const double residual=std::abs(p.Z-(a.Z-gap.InitialDown*t-.5*980*t*t));
     const double xy=std::hypot(p.X-(a.X+vx*t),p.Y-(a.Y+vy*t));
     physics &= residual<.01 && xy<.05;
    }
   }
  }
  const auto detached=[&](float t){
   const float v=.84f*(1.f-t);
   if(const auto* gap=DCWaterfallTrajectory::Find(gaps,v))return DCWaterfallTrajectory::Sample(*gap,surface,v,0.f);
   return surface(v,0.f);
  };
  const auto newContact=DCCliffGenerator::FirstWaterContact(detached,water);
  const double drift=std::hypot(std::hypot(core.Position.X-newContact.Position.X,
   core.Position.Y-newContact.Position.Y),core.Position.Z-newContact.Position.Z);
  physics &= newContact.bFound && drift<.01;
  pass &= physics;
  std::printf("Detached production helper: %s gaps=%zu contact_drift=%.6fcm\n",physics?"PASS":"FAIL",gaps.size(),drift);
 }
"""
contact.MAIN=contact.block(helper,"namespace DCWaterfallTrajectory")+contact.MAIN.replace("  pass&=feedPass;", "  pass&=feedPass;\n"+extra)

if __name__=="__main__":
    contact.main()
