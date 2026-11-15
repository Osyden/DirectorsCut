#pragma once

namespace DCEarlyEncounterPolicy
{
    constexpr float StatueHealth=4.f;
    constexpr float StatueFirstExposure=11.f;
    constexpr float StatueCharge=3.5f;
    constexpr float CreatureCharge=3.f;
    constexpr float ChargeSpacing=4.f;

    inline bool ShowStatueMarker(bool Statue,bool Target,bool Hidden,bool CanShoot,int Stage)
    { return Statue && Target && !Hidden && CanShoot && Stage!=4; }

    inline bool CanBeginCharge(int Stage,float Exposure,float Wait,bool CanShoot,float Spacing)
    { return Stage==0 && Exposure>=Wait && CanShoot && Spacing<=0.f; }
}
