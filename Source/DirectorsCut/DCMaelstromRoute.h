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

    constexpr float EntryStartSeconds=-8.f;
    constexpr float LakeEntrySeconds=32.f;
    constexpr float LakeRadiusFraction=.58f;
    constexpr float OrbitAngularSpeed=.009f;

    inline float DeckPitch(float GradeDegrees, float Seconds)
    {
        return FMath::Clamp(GradeDegrees,-34.f,4.f)*FMath::SmoothStep(-8.f,-2.f,Seconds);
    }

    inline float LandingHeave(float Age)
    {
        // One downward compression and a smaller rebound, fully settled at 3s.
        if(Age<0.f || Age>=3.f) { return 0.f; }
        return -90.f*FMath::Sin(Age*9.f)*FMath::Exp(-Age*2.2f)
            *(1.f-FMath::SmoothStep(2.f,3.f,Age));
    }

    inline float RadialFraction(float Seconds)
    {
        // One downhill approach, then a level, constant-radius lake orbit.
        return FMath::Lerp(.985f,LakeRadiusFraction,
            FMath::SmoothStep(EntryStartSeconds,LakeEntrySeconds,Seconds));
    }

    inline float Angle(float Seconds)
    {
        const float R=RadialFraction(Seconds);
        const float FeedingAngle=UE_PI-.40f*(1.f-FMath::SmoothStep(LakeRadiusFraction,.985f,R));
        // Integrate a smooth angular-speed ramp over the final eight seconds.
        // This 33m tangential lead stays inside the wide cataract and prevents
        // the ship stopping as the radial descent meets the level lake.
        const float U=FMath::Clamp((Seconds-24.f)/8.f,0.f,1.f);
        const float OrbitSeconds=8.f*(U*U*U-.5f*U*U*U*U)+FMath::Max(0.f,Seconds-LakeEntrySeconds);
        return FeedingAngle-OrbitAngularSpeed*OrbitSeconds;
    }

    inline float SecondsAtAngle(float A)
    {
        const float Delta=FMath::Fmod(UE_PI-A+UE_TWO_PI,UE_TWO_PI);
        const float JoinDelta=UE_PI-Angle(LakeEntrySeconds);
        if(Delta>=JoinDelta) { return LakeEntrySeconds+(Delta-JoinDelta)/OrbitAngularSpeed; }
        // The finite entry is monotone but not a linear angular orbit.
        float Low=EntryStartSeconds,High=LakeEntrySeconds;
        for(int32 I=0;I<22;++I)
        {
            const float Mid=(Low+High)*.5f;
            if(UE_PI-Angle(Mid)<Delta) { Low=Mid; } else { High=Mid; }
        }
        return (Low+High)*.5f;
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

    inline float SteeringYaw(float Seconds, float Radius, float Offset, float RadialVelocity)
    {
        const FVector2D Center = Tangent(Seconds, Radius);
        const FVector2D Travel = (Point(Seconds+.05f, Radius, Offset+RadialVelocity*.05f)
            - Point(Seconds-.05f, Radius, Offset-RadialVelocity*.05f)).GetSafeNormal();
        const float CenterYaw = FMath::RadiansToDegrees(FMath::Atan2(Center.Y, Center.X));
        const float TravelYaw = FMath::RadiansToDegrees(FMath::Atan2(Travel.Y, Travel.X));
        // Let the hull visibly turn into its actual travel, while retaining a
        // comfortable broadside camera and a stable distant horizon.
        return FMath::Clamp(FMath::FindDeltaAngleDegrees(CenterYaw, TravelYaw), -12.f, 12.f);
    }

    // The battle stays on the lake; there is no narrowing final throat.
    inline float WidthScale(float Seconds)
    {
        return 1.f;
    }

    inline float BankedHeight(float Across, float Seconds)
    {
        // Legacy callers may still ask for a cross-section, but the caldera
        // has a flat mean surface. Entry elevation is supplied by HeightAt.
        return 0.f;
    }
}
