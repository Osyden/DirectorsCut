#pragma once

namespace DCEarlyEncounterPolicy
{
    // Treadmill damage delegates may reduce, but cannot amplify, a cannon hit.
    // A normalized one-point budget therefore represents two half-point hits.
    constexpr float CreatureHealth=1.f;
    inline float CreatureHitCost(int PreviousHits,int FirstHitPercentile)
    { return PreviousHits>0 || FirstHitPercentile<33 ? CreatureHealth : .5f; }

    constexpr float StatueHealth=4.f;
    constexpr float StatueFirstExposure=11.f;
    constexpr float StatueCharge=3.5f;
    constexpr float CreatureCharge=3.f;
    constexpr float ChargeSpacing=4.f;

    // Awareness starts when the monument is revealed, before the ship reaches
    // a usable broadside. Shot/review admission remains a separate decision.
    inline bool ShowStatueAwareness(bool Statue,bool Target,bool Hidden,int Stage)
    { return Statue && Target && !Hidden && Stage!=4; }

    inline bool ShowStatueMarker(bool Statue,bool Target,bool Hidden,bool CanShoot,int Stage)
    { return ShowStatueAwareness(Statue,Target,Hidden,Stage) && CanShoot; }

    inline bool CanBeginCharge(int Stage,float Exposure,float Wait,bool CanShoot,float Spacing)
    { return Stage==0 && Exposure>=Wait && CanShoot && Spacing<=0.f; }
}
