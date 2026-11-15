#pragma once

#include "CoreMinimal.h"

/** Deterministic centreline in basin centimetres. Positive offset is starboard/outward.
 * The ship remains at the world origin; the arena's inverse pose expresses travel. */
namespace DCMaelstromRoute
{
    constexpr float SteeringLimit=11000.f;
    inline float SteeringAuthority(float RiggingAuthority) { return FMath::Clamp(RiggingAuthority,.75f,1.f); }
    constexpr float MaximumSteeringYaw=40.f; // Fixed slam joint remains inside the cannon's broadside arc.
    inline void AdvanceSteering(float Input, float DT, float Riding, float& Helm, float& Offset, float& Velocity,
        float OutwardLimit=SteeringLimit)
    {
        // A real helm changes carried velocity, not position. A short fixed step
        // keeps braking/reversal identical at ordinary and hitch frame rates.
        OutwardLimit=FMath::Clamp(OutwardLimit,0.f,SteeringLimit);
        const int32 Steps=FMath::Max(1,FMath::CeilToInt(FMath::Max(0.f,DT)*120.f));
        const float H=FMath::Max(0.f,DT)/Steps;
        for(int32 I=0;I<Steps;++I)
        {
            const float PreviousOffset=Offset;
            Helm=FMath::FInterpTo(Helm,FMath::Clamp(Input,-1.f,1.f),H,5.f);
            constexpr float AccelerationLimit=1900.f, MaximumSpeed=1250.f;
            float Target=Helm*MaximumSpeed;
            // Brake only at the real boundary. The previous restoring spring
            // began halfway across the lane and fought steering/cancelled dodges.
            const float Braking=Velocity*FMath::Abs(Velocity)/(2.f*AccelerationLimit);
            const float Predicted=Offset+Velocity*.30f+Braking;
            if(Target>0.f)
            {
                const float Edge=FMath::Max(0.f,OutwardLimit-300.f);
                Target*=1.f-FMath::SmoothStep(FMath::Max(0.f,Edge-1800.f),FMath::Max(1.f,Edge),Predicted);
            }
            else if(Target<0.f)
            { Target*=1.f-FMath::SmoothStep(SteeringLimit-2100.f,SteeringLimit-300.f,-Predicted); }
            const float Response=FMath::Abs(Input)<.05f ? .70f : .45f;
            const float Acceleration=FMath::Clamp((Target-Velocity)/Response,-AccelerationLimit,AccelerationLimit);
            Velocity=FMath::Clamp(Velocity+Acceleration*H,-MaximumSpeed,MaximumSpeed);
            Offset+=Velocity*H*Riding;
            if(Offset<-SteeringLimit) { Offset=-SteeringLimit; Velocity=FMath::Max(0.f,Velocity); }
            if(Offset>OutwardLimit)
            {
                Offset=OutwardLimit;
                // Keep the existing inlet guard: a closing bank carries the
                // ship inward and its visible heading follows that actual motion.
                Velocity=DT>0.f ? FMath::Min(0.f,(Offset-PreviousOffset)/DT) : 0.f;
            }
        }
    }

    constexpr float EntryStartSeconds=-8.f;
    constexpr float LakeEntrySeconds=32.f;
    constexpr float LakeRadiusFraction=.58f;
    constexpr float OrbitAngularSpeed=.009f;
    constexpr float LakeHeight=-40000.f;

    inline float MeanSurfaceHeight(const FVector2D& P,float Radius)
    {
        const float R=P.Size(),Q=FMath::SmoothStep(LakeRadiusFraction,.985f,R/Radius);
        const float A=UE_PI-.40f*(1.f-Q);
        const float Across=FMath::Abs(FMath::UnwindRadians(FMath::Atan2(P.Y,P.X)-A))*R;
        return R>=Radius*LakeRadiusFraction && Across<=22000.f && R<=Radius*1.08f
            ? FMath::Lerp(LakeHeight,0.f,Q) : LakeHeight;
    }

    inline float BowLakeClearance(float CentreHeight,float PhysicalPitch,float BowReach)
    {
        // Bow waterline, rotated about the same amidships waterline as DCGame.
        // A sloping hull touches the lake before its centre finishes descending.
        return CentreHeight+FMath::Sin(PhysicalPitch*UE_PI/180.f)*BowReach-LakeHeight;
    }

    // The two foam crests are inset from the same enforced inner orbit limit.
    // Kraken swims on the smaller authored inner orbit; the band encloses it.
    inline float InnerBoundaryRadius(float Radius, float HullClearance=2500.f)
    { return Radius*LakeRadiusFraction-SteeringLimit-HullClearance; }

    inline float DeckPitch(float GradeDegrees, float Seconds)
    {
        return FMath::Clamp(GradeDegrees,-34.f,4.f)*FMath::SmoothStep(-8.f,-2.f,Seconds);
    }

    // The waterfall foot throws a standing wave above the flat basin. Start
    // the deck wash here, then carry it through the subsequent flat-water touch.
    constexpr float LandingWavefrontClearance=850.f;
    constexpr float LandingWashSeconds=3.f;

    inline float LandingHeave(float Age)
    {
        // One downward compression and a smaller rebound, fully settled at 3s.
        if(Age<0.f || Age>=3.f) { return 0.f; }
        return -160.f*FMath::Sin(Age*9.f)*FMath::Exp(-Age*2.2f)
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

    inline float InletOutwardLimit(float Seconds,float Radius)
    {
        // Outward offset still sits uphill on the inlet when the centreline
        // reaches the lake. Let the river carry it back before the lake turn;
        // otherwise it rides a nonexistent side shelf and drops 13m at its end.
        const float Approach=1.f-FMath::SmoothStep(16.f,30.f,Seconds);
        // Restore full helm only once even the innermost part of that outward
        // band has passed the inlet's 220m angular footprint plus bow margin.
        const float Across=FMath::Abs(FMath::UnwindRadians(Angle(Seconds)-(UE_PI-.40f)))
            *Radius*LakeRadiusFraction;
        const float Clear=Seconds>=LakeEntrySeconds ? FMath::SmoothStep(25000.f,35000.f,Across) : 0.f;
        return 11000.f*FMath::Max(Approach,Clear);
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
        return FMath::Clamp(FMath::FindDeltaAngleDegrees(CenterYaw, TravelYaw), -MaximumSteeringYaw, MaximumSteeringYaw);
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
