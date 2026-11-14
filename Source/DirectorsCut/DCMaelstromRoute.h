#pragma once

#include "CoreMinimal.h"

/** Deterministic centreline in basin centimetres. Positive offset is starboard/inward.
 * The ship remains at the world origin; the arena's inverse pose expresses travel. */
namespace DCMaelstromRoute
{
    inline void AdvanceSteering(float Input, float DT, float Riding, float& Helm, float& Offset, float& Velocity)
    {
        Helm = FMath::FInterpTo(Helm,Input,DT,1.5f);
        const float Bank = FMath::SmoothStep(5000.f,9500.f,FMath::Abs(Offset));
        const float Acceleration = Helm*850.f - Velocity*.85f - FMath::Sign(Offset)*Bank*1900.f;
        Velocity = FMath::Clamp(Velocity+Acceleration*DT,-1300.f,1300.f);
        Offset += Velocity*DT*Riding;
        if (FMath::Abs(Offset)>11000.f) { Offset=FMath::Clamp(Offset,-11000.f,11000.f); Velocity*=.25f; }
    }

    inline float Angle(float Seconds)
    {
        const float T = FMath::Max(0.f, Seconds - 22.f);
        // Ramp to about 12 m/s at a 1.2 km radius, without a heading discontinuity.
        return UE_PI + 0.010f * (T < 12.f ? T * T / 24.f : T - 6.f);
    }

    inline float RadialFraction(float Seconds)
    {
        return 0.985f - 0.245f * FMath::SmoothStep(12.f, 40.f, Seconds)
            - 0.08f * FMath::SmoothStep(40.f, 158.f, Seconds);
    }

    inline float SecondsAtAngle(float A)
    {
        float Delta = FMath::Fmod(A - UE_PI + UE_TWO_PI, UE_TWO_PI);
        const float Travel = Delta / 0.010f;
        return 22.f + (Travel < 6.f ? FMath::Sqrt(Travel * 24.f) : Travel + 6.f);
    }

    inline FVector2D Point(float Seconds, float Radius, float Offset = 0.f)
    {
        const float A = Angle(Seconds);
        const float R = Radius * RadialFraction(Seconds) - Offset;
        return FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
    }
}
