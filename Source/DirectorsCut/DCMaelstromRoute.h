#pragma once

#include "CoreMinimal.h"

/** Deterministic centreline in basin centimetres. Positive offset is starboard/outward.
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
        // The feeding river is already moving tangentially before the reveal.
        // Decreasing basin angle places the abyss to PORT: a continuous left
        // turn rather than a stationary drop followed by sideways acceleration.
        return UE_PI - 0.018f * FMath::Max(0.f, Seconds + 8.f);
    }

    inline float RadialFraction(float Seconds)
    {
        // Long battle descent, followed by a tightening final approach. The
        // old route stopped at 66% of the crater radius and never reached its foot.
        return 0.985f - 0.035f * FMath::SmoothStep(-8.f, 8.f, Seconds)
            - 0.700f * FMath::SmoothStep(8.f, 165.f, Seconds)
            - 0.130f * FMath::SmoothStep(165.f, 180.f, Seconds);
    }

    inline float SecondsAtAngle(float A)
    {
        const float Delta = FMath::Fmod(UE_PI - A + UE_TWO_PI, UE_TWO_PI);
        return Delta / 0.018f - 8.f;
    }

    inline FVector2D Point(float Seconds, float Radius, float Offset = 0.f)
    {
        const float A = Angle(Seconds);
        const float R = Radius * RadialFraction(Seconds) + Offset;
        return FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
    }

    inline FVector2D Tangent(float Seconds, float Radius)
    {
        return (Point(Seconds+.05f,Radius)-Point(Seconds-.05f,Radius)).GetSafeNormal();
    }

    // Geometry and steering share this width, including the narrowing throat.
    inline float WidthScale(float Seconds)
    {
        return 1.f-.40f*FMath::SmoothStep(125.f,180.f,Seconds);
    }

    inline float BankedHeight(float Across, float Seconds)
    {
        const float Width=WidthScale(Seconds);
        const float X=Across/Width;
        const float Variation=1.f+.16f*FMath::Sin(Seconds*.061f)+.09f*FMath::Sin(Seconds*.143f+1.2f);
        // Broad asymmetric hydraulic cross-section, not two raised rails.
        // The inner spill is low enough to retain the view into the abyss.
        const float Outer=2400.f*Variation*FMath::Exp(-FMath::Square((X-14500.f)/6500.f));
        const float Inner=320.f*(1.f+.2f*FMath::Sin(Seconds*.09f))*FMath::Exp(-FMath::Square((X+8500.f)/3800.f));
        const float AtCenter=2400.f*Variation*FMath::Exp(-FMath::Square(14500.f/6500.f))
            +320.f*(1.f+.2f*FMath::Sin(Seconds*.09f))*FMath::Exp(-FMath::Square(8500.f/3800.f));
        return Across*.10f+(Outer+Inner-AtCenter)*Width;
    }
}
