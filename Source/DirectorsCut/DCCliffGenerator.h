#pragma once

#include "CoreMinimal.h"

/** Deterministic metre-scale geology. Circumferential cells wrap exactly;
 * piecewise-planar fractures produce ledges rather than rounded noise hills. */
namespace DCCliffGenerator
{
    inline constexpr float TempleAngles[]={-143.f,-109.f,-76.f,-38.f,2.f,38.f,77.f,112.f,144.f};
    inline float Hash(int32 X,int32 Y,int32 Period)
    {
        const uint32 Wrapped=static_cast<uint32>((X%Period+Period)%Period);
        uint32 H=Wrapped*374761393u+static_cast<uint32>(Y)*668265263u+92837111u;
        H=(H^(H>>13))*1274126177u;
        return static_cast<float>((H^(H>>16))&0x00ffffffu)/16777215.f;
    }

    inline float Facet(float Around,float Height,int32 Period)
    {
        const float X=Around*Period;
        const int32 I=FMath::FloorToInt(X),J=FMath::FloorToInt(Height);
        const float U=X-I,V=Height-J;
        const float A=Hash(I,J,Period),B=Hash(I+1,J,Period);
        const float C=Hash(I,J+1,Period),D=Hash(I+1,J+1,Period);
        return (U+V<1.f ? A+(B-A)*U+(C-A)*V : D+(C-D)*(1-U)+(B-D)*(1-V))*2.f-1.f;
    }

    struct FSection
    {
        float ToeRadius, LipRadius, OuterRadius, WaterZ, TopZ;
    };

    inline float Joint(float Around,float Height)
    {
        const float Phase=Around*96.f+Facet(Around,2.1f,24)*.8f+Height*.7f;
        const float F=FMath::Frac(Phase);
        const float Cut=1.f-FMath::SmoothStep(.015f,.10f,FMath::Min(F,1-F));
        return Cut*(.45f+.55f*FMath::SmoothStep(-.4f,.4f,Facet(Around,Height*4.f,48)));
    }

    inline FVector Point(const FSection& S,float Angle,float V,float Drainage)
    {
        const float A=Angle/UE_TWO_PI;
        const float Gap=FMath::Max(1000.f,S.LipRadius-S.ToeRadius);
        const float FaceFoot=S.LipRadius-FMath::Min(6500.f,Gap*.38f);
        const float BottomZ=FMath::Lerp(S.WaterZ,S.TopZ,.13f);
        float R,Z;
        if(V<.20f)
        {
            const float T=V/.20f;
            R=FMath::Lerp(S.ToeRadius,FaceFoot,T);
            Z=FMath::Lerp(S.WaterZ,BottomZ,T*T);
            const float Envelope=FMath::Sin(T*UE_PI);
            Z+=Envelope*(330.f*Facet(A,T*7.f,384)+90.f*Facet(A,T*24.f,1024));
        }
        else if(V<.84f)
        {
            const float T=(V-.20f)/.64f;
            // The face rises mostly vertically. Outward lean and recessed beds
            // are geometric; detail is not obtained by stretching a rock prop.
            Z=FMath::Lerp(BottomZ,S.TopZ,T);
            R=FMath::Lerp(FaceFoot,S.LipRadius,T);
            const float Envelope=FMath::SmoothStep(0.f,.06f,T)*(1-FMath::SmoothStep(.90f,1.f,T));
            const float WarpA=A+Facet(A,T*.4f,16)*.003f;
            const float Mass=Facet(WarpA+.001f,T*1.3f,24);
            const float Fault=Facet(WarpA,T*6.f,96);
            const float Broken=Facet(WarpA+.002f,T*26.f,288);
            const float Chips=Facet(WarpA,T*96.f,1152);
            // Unequal beds, interrupted by oblique faults; no repeated rings.
            const float Bed=T*7.3f+Facet(A,1.7f,32)*.65f;
            const float BedFraction=FMath::Frac(Bed);
            const float Recess=1.f-FMath::SmoothStep(.025f,.13f,FMath::Min(BedFraction,1-BedFraction));
            const float BedBreak=FMath::SmoothStep(.15f,.6f,Facet(A,T*3.f,48));
            const float DetailScale=FMath::Min(1.f,Gap/30000.f);
            R+=Envelope*DetailScale*(Mass*8200.f+Fault*4200.f+Broken*1800.f+Chips*300.f
                +Recess*BedBreak*120.f);
            R+=Drainage*Envelope*1500.f;
        }
        else
        {
            const float T=(V-.84f)/.16f;
            R=FMath::Lerp(S.LipRadius,S.OuterRadius,T);
            // A low plateau leaves architecture against the sky. Small chips
            // at its edge do not turn into a wall of hills behind the temples.
            Z=S.TopZ-220.f*FMath::SmoothStep(.05f,.5f,T)
                +55.f*Facet(A,T*8.f,384)*FMath::SmoothStep(0.f,.18f,T);
        }
        return FVector(FMath::Cos(Angle)*R,FMath::Sin(Angle)*R,Z);
    }
}
