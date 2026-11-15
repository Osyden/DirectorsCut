#pragma once
#include "CoreMinimal.h"

namespace DCCityObstacleLayout
{
    constexpr float Interval=14.f;
    constexpr float MinimumSide=2600.f;
    constexpr float MaximumSide=3400.f;
    constexpr float MaximumFootprint=900.f;
    constexpr float Submergence=350.f;
    inline float Side(int32 Serial) { return Serial%2==0 ? -1.f : 1.f; }
    inline float HullClearance(float HalfWidth) { return MinimumSide-MaximumFootprint-HalfWidth; }
}
