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

    // A formation's failure planes end at different elevations. Sampling the
    // complete profile at jittered centres avoids one continuous curved bed
    // around the basin; neighbouring buttresses expose different lit faces.
    inline float FormationProfile(float A,float T)
    {
        constexpr int32 Period=32;
        const float X=(A+.007f*Facet(A,1.7f,11))*Period;
        const int32 Cell=FMath::FloorToInt(X);
        float Total=0.f,Weight=0.f;
        for(int32 I=Cell-1;I<=Cell+1;++I)
        {
            const float Centre=I+.18f+.64f*Hash(I,71,Period);
            const float D=FMath::Abs(X-Centre);
            // Broad planar fronts and unequal transition widths. This is not
            // a regular grid of giant cliff panels or a sinusoidal wave wall.
            const float W=FMath::Max(0.f,1.f-D/(.68f+.30f*Hash(I,72,Period)));
            const float W2=W*W;
            const float Break=.13f+.57f*Hash(I,73,Period);
            const float Width=.075f+.10f*Hash(I,74,Period);
            const float Upper=.50f+.30f*Hash(I,75,Period);
            const float Profile=.07f*T+.68f*FMath::SmoothStep(Break,Break+Width,T)
                +.25f*FMath::SmoothStep(Upper,Upper+.13f,T);
            Total+=W2*Profile; Weight+=W2;
        }
        return Weight>SMALL_NUMBER ? Total/Weight : T;
    }

    inline FFaceRelief FaceRelief(float A,float T)
    {
        constexpr int32 Period=57;
        const float Warp=A+.0026f*Facet(A,T*1.7f,19);
        const float X=Warp*Period;
        const int32 Cell=FMath::FloorToInt(X);
        float Buttress=0.f,Gully=0.f;
        for(int32 I=Cell-2;I<=Cell+2;++I)
        {
            const float H=Hash(I,19,Period);
            // Each buttress leans and terminates independently. Linear flanks
            // form broad oblique rock planes instead of rounded noise pillows.
            const float Centre=I+.12f+.76f*H+(T-.5f)*(.30f-.60f*Hash(I,23,Period));
            const float D=X-Centre;
            const float Width=D<0.f ? .20f+.33f*Hash(I,29,Period) : .25f+.42f*Hash(I,31,Period);
            const float Flank=FMath::Clamp(1.f-FMath::Abs(D)/Width,0.f,1.f);
            const float Crown=.40f+.51f*Hash(I,37,Period);
            const float HeightMask=FMath::SmoothStep(.015f,.15f,T)
                *(1.f-FMath::SmoothStep(Crown-.10f,Crown+.08f,T));
            Buttress+=Flank*HeightMask*(6500.f+9500.f*H);
            const float CutCentre=I+.74f+.23f*Hash(I,41,Period)
                +.16f*Facet(A+.002f,T*3.1f,23);
            const float CutWidth=.07f+.10f*Hash(I,43,Period);
            const float Cut=FMath::Clamp(1.f-FMath::Abs(X-CutCentre)/CutWidth,0.f,1.f);
            Gully+=Cut*(.35f+.65f*FMath::SmoothStep(.06f,.72f,T));
        }
        const float Broken=Facet(Warp+.002f,T*11.f,173);
        const float Chips=Facet(Warp,T*41.f,509);
        return {-Buttress+Gully*7800.f+Broken*1800.f+Chips*360.f,FMath::Clamp(Gully,0.f,1.f)};
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
            // Faulted masses retreat at separate elevations; a shared smooth
            // cross section previously made the whole crater a concave curtain.
            const float RadialT=FormationProfile(A,T);
            R=FMath::Lerp(FaceFoot,S.LipRadius,RadialT);
            const float Envelope=FMath::SmoothStep(0.f,.06f,T)*(1-FMath::SmoothStep(.90f,1.f,T));
            const float DetailScale=FMath::Min(1.f,Gap/30000.f);
            const float Raw=FaceRelief(A,T).Offset*DetailScale;
            // Smoothly bound relief above the shoreline even on the narrow
            // inlet. A hard radial clamp would create obvious flat cut planes.
            const float Limit=FMath::Max(100.f,FMath::Min(24000.f,(R-S.ToeRadius)*.85f));
            R+=Envelope*Raw/FMath::Sqrt(1.f+FMath::Square(Raw/Limit));
            R+=Drainage*Envelope*4200.f*DetailScale;
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
