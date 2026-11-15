#pragma once
#include <algorithm>
#include <cmath>

// Presentation only. Attack owners decide visibility using their real windows.
namespace DCCombatCuePolicy
{
struct FLook { float RadiusScale, Glow, ArcFraction; };
inline FLook Look(float Charge, float Age, float HitFlash=0.f)
{
    Charge=std::clamp(Charge,0.f,1.f);
    HitFlash=std::clamp(HitFlash,0.f,1.f);
    const float Breath=.5f+.5f*std::sin(Age*(3.f+Charge*2.f));
    return {1.f+.025f*Breath, .62f+.14f*Breath+.20f*Charge+.32f*HitFlash,
        .18f+.68f*Charge};
}
inline bool AlertVisible(int Stage,bool HasTarget,bool Concealed,bool BodyHidden)
{ return Stage==1 && HasTarget && !Concealed && !BodyHidden; }
inline bool WeakPointVisible(bool WindowOpen,bool IsMouth,bool HasTarget,bool Interrupted,bool Cancelled,bool Applied)
{ return WindowOpen && !IsMouth && HasTarget && !Interrupted && !Cancelled && !Applied; }
}
