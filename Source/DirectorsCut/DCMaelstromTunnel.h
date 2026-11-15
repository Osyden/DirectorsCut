#pragma once
#include "CoreMinimal.h"

namespace DCMaelstromTunnel
{
    constexpr float Radius=9000.f;
    inline float WallShape(float Angle)
    {
        return FMath::Cos(Angle)>0.f ? FMath::SmoothStep(.20f,.50f,FMath::Abs(FMath::Sin(Angle))) : 1.f;
    }
    inline float ShapedRadius(float Angle,float Ripple,float Mouth)
    {
        return Radius+WallShape(Angle)*(Ripple+Mouth*1200.f*(.65f+.35f*FMath::Sin(Angle*3.f+.7f)));
    }
    inline float Lip(float Angle,float Mouth)
    {
        return -WallShape(Angle)*Mouth*(1900.f+1300.f*FMath::Sin(Angle*3.f+.4f)+600.f*FMath::Sin(Angle*7.f-1.2f));
    }
}
