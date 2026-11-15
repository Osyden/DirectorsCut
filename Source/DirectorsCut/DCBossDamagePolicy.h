#pragma once
#include <algorithm>
#include <cmath>

namespace DCBossDamagePolicy
{
    constexpr float BodyRadius=700.f, MouthRadius=450.f, BodyAboveMouth=1700.f;
    constexpr float SlamDamage=10.f, SlamStrength=(SlamDamage-1.f)/3.f;
    constexpr float SlamJointForward=1800.f, SlamJointSide=7000.f;
    inline float BodyDamage(float Requested,float Time,float Start,float Deadline,bool Running,bool Finished,bool Paused)
    { return Running && !Finished && !Paused && Time>=Start && Time<Deadline && std::isfinite(Requested)?std::max(0.f,Requested):0.f; }
    inline bool ShowHitInsteadOfTarget(bool HasTarget,bool HasHit,bool Interrupted)
    { return HasHit && (!HasTarget || Interrupted); }
    inline bool SlamContact(bool Applied,bool Interrupted,bool Cancelled,float DT,float Time,float Strike)
    { return !Applied && !Interrupted && !Cancelled && DT>0.f && Time>=Strike; }
}
