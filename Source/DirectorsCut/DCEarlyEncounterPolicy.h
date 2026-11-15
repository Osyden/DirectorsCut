#pragma once

namespace DCEarlyEncounterPolicy
{
    // Treadmill damage delegates may reduce, but cannot amplify, a cannon hit.
    // Three normal hits consume one normalized health budget; a fresh roll on
    // every valid contact can instead consume the remaining budget immediately.
    constexpr float CreatureHealth=1.f;
    inline float CreatureHitCost(int PreviousHits,int HitPercentile)
    { return PreviousHits>=2 || HitPercentile<25 ? CreatureHealth : CreatureHealth/3.f; }

    constexpr float StatueHealth=4.f;
    constexpr float StatueFirstExposure=11.f;
    constexpr float StatueCharge=3.5f;
    constexpr float CreatureCharge=3.f;
    constexpr float CreatureFirstExposure=12.f;
    constexpr float ChargeSpacing=4.f;
    constexpr float BeamDuration=1.2f;
    inline float BeamProgress(float Time) { return Time<=.2f?0.f:(Time>=1.f?1.f:(Time-.2f)/.8f); }
    inline int BeamContactsDue(float Time) { return (Time>=.2f?1:0)+(Time>=1.f?1:0); }

    // Awareness starts when the monument is revealed, before the ship reaches
    // a usable broadside. Shot/review admission remains a separate decision.
    inline bool ShowStatueAwareness(bool Statue,bool Target,bool Hidden,int Stage)
    { return Statue && Target && !Hidden && Stage!=4; }

    inline bool ShowStatueMarker(bool Statue,bool Target,bool Hidden,bool CanShoot,int Stage)
    { return ShowStatueAwareness(Statue,Target,Hidden,Stage) && CanShoot; }

    inline bool CanBeginCharge(int Stage,float Exposure,float Wait,bool CanShoot,float Spacing)
    { return Stage==0 && Exposure>=Wait && CanShoot && Spacing<=0.f; }
}
