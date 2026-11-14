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

    inline float TalusWidth(const FSection& S,float Angle)
    {
        const float A=Angle/UE_TWO_PI;
        const float Gap=FMath::Max(1000.f,S.LipRadius-S.ToeRadius);
        // Local fans of debris, not one 500m-wide illuminated apron. Preserve
        // the inlet clearance by reducing the fan with the available gap.
        const float Fan=FMath::SmoothStep(-.45f,.65f,Facet(A,1.8f,32));
        return FMath::Min(Gap*.26f,4200.f+8500.f*Fan);
    }

    struct FFaceRelief { float Offset; float Recess; };

    inline FFaceRelief FaceRelief(float A,float T)
    {
        const float WarpA=A+Facet(A,T*.7f,16)*.0023f;
        const float Mass=Facet(WarpA+.001f,T*1.8f,24);
        const float Fault=Facet(WarpA,T*7.f,96);
        const float Broken=Facet(WarpA+.002f,T*29.f,288);
        const float Chips=Facet(WarpA,T*97.f,1152);
        // Sparse oblique cleaves follow the generated fracture field. They
        // stop across formations rather than continuing as rings or panels.
        const float Cleave=FMath::SmoothStep(.42f,.88f,FMath::Abs(Fault))
            *FMath::SmoothStep(.05f,.65f,Facet(A+.007f,T*3.2f+11.f,40));
        return {Mass*6200.f+Fault*3100.f+Broken*1150.f+Chips*220.f+Cleave*950.f,Cleave};
    }

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
        const float FaceFoot=S.ToeRadius+TalusWidth(S,Angle);
        const float DebrisHeight=.08f+.06f*FMath::SmoothStep(-.5f,.65f,Facet(A,2.7f,24));
        const float BottomZ=FMath::Lerp(S.WaterZ,S.TopZ,DebrisHeight);
        float R,Z;
        if(V<.20f)
        {
            const float T=V/.20f;
            R=FMath::Lerp(S.ToeRadius,FaceFoot,T);
            Z=FMath::Lerp(S.WaterZ,BottomZ,T*T);
            const float Envelope=FMath::Sin(T*UE_PI);
            const float DetailScale=FMath::Min(1.f,TalusWidth(S,Angle)/7000.f);
            Z+=Envelope*DetailScale*(260.f*Facet(A,T*7.f,384)+70.f*Facet(A,T*24.f,1024));
        }
        else if(V<.84f)
        {
            const float T=(V-.20f)/.64f;
            // The face rises mostly vertically. Outward lean and recessed beds
            // are geometric; detail is not obtained by stretching a rock prop.
            Z=FMath::Lerp(BottomZ,S.TopZ,T);
            // Lower and upper steep faces are separated by an unequal recessed
            // shoulder. Its height varies around the rim, preventing a new ring.
            const float Shoulder=.58f+.12f*Facet(A,1.2f,20);
            const float RadialT=.30f*T+.70f*FMath::SmoothStep(.12f,Shoulder,T);
            R=FMath::Lerp(FaceFoot,S.LipRadius,RadialT);
            const float Envelope=FMath::SmoothStep(0.f,.06f,T)*(1-FMath::SmoothStep(.90f,1.f,T));
            const float DetailScale=FMath::Min(1.f,Gap/30000.f);
            const float Raw=FaceRelief(A,T).Offset*DetailScale;
            // Smoothly bound relief above the shoreline even on the narrow
            // inlet. A hard radial clamp would create obvious flat cut planes.
            const float Limit=FMath::Max(100.f,FMath::Min(10000.f,(R-S.ToeRadius)*.78f));
            R+=Envelope*Raw/(1.f+FMath::Abs(Raw)/Limit);
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
