#pragma once
#include "CoreMinimal.h"

namespace DCCityObstacleLayout
{
    constexpr float Interval=14.f;
    constexpr float MinimumSide=2600.f;
    constexpr float MaximumSide=3400.f;
    constexpr float MaximumFootprint=900.f;
    constexpr float Submergence=100.f;
    constexpr float MinimumVisiblePillar=400.f;
    constexpr float WaveAllowance=250.f;
    // Evaluate the maximum future city shoulder, even when spawning in the
    // storm entry before its trough has formed. All quantities are centimetres.
    inline float MinimumPillarTop(float MeanSea,float SeaY,float HalfWidth,float PeakRise)
    {
        const float U=FMath::Clamp((FMath::Abs(SeaY)+MaximumFootprint)/FMath::Max(HalfWidth,1.f),0.f,1.f);
        return MeanSea+PeakRise*U*U*(3.f-2.f*U)+WaveAllowance+MinimumVisiblePillar;
    }
    inline float Side(int32 Serial) { return Serial%2==0 ? -1.f : 1.f; }
    inline float HullClearance(float HalfWidth) { return MinimumSide-MaximumFootprint-HalfWidth; }
}
