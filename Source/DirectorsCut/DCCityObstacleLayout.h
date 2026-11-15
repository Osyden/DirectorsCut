#pragma once
#include "CoreMinimal.h"

namespace DCCityObstacleLayout
{
    constexpr float Interval=26.f;
    constexpr int32 Count=5;
    // Fixed sea-space course: useful steering choices, not a wall spawned
    // in the player's chosen lane. Last cluster passes well before the tunnel.
    constexpr float Lanes[Count]={-650.f,1800.f,-1400.f,1000.f,-2100.f};
    inline float Lane(int32 Serial) { return Lanes[Serial%Count]; }
    inline float PassingLane(int32 Serial) { return Lane(Serial)<0.f ? 3300.f : -3300.f; }
    constexpr float MinimumSide=650.f;
    constexpr float MaximumSide=2100.f;
    constexpr float MaximumFootprint=900.f;
    constexpr float Submergence=300.f;
    constexpr float MinimumVisiblePillar=600.f;
    constexpr float WaveAllowance=250.f;
    // Evaluate the maximum future city shoulder, even when spawning in the
    // storm entry before its trough has formed. All quantities are centimetres.
    inline float MinimumPillarTop(float MeanSea,float SeaY,float HalfWidth,float PeakRise)
    {
        const float U=FMath::Clamp((FMath::Abs(SeaY)+MaximumFootprint)/FMath::Max(HalfWidth,1.f),0.f,1.f);
        return MeanSea+PeakRise*U*U*(3.f-2.f*U)+WaveAllowance+MinimumVisiblePillar;
    }
    inline float Side(int32 Serial) { return Serial%2==0 ? -1.f : 1.f; }
    inline float PassingClearance(int32 Serial,float HullRadius)
    { return FMath::Abs(Lane(Serial)-PassingLane(Serial))-MaximumFootprint-HullRadius; }
    inline bool CanImpact(bool AlreadyHit,float Cooldown) { return !AlreadyHit && Cooldown<=0.f; }
    inline float AnchoredZ(float AbsoluteStoneZ,float ShipFrameRise) { return AbsoluteStoneZ-ShipFrameRise; }
}
