#pragma once

#include "CoreMinimal.h"

/** Deterministic metre-scale geology. Circumferential cells wrap exactly;
 * piecewise-planar fractures produce ledges rather than rounded noise hills. */
namespace DCCliffGenerator
{
    inline constexpr int32 MeshSectors=1536,MeshRows=160;
    inline constexpr float TempleAngles[]={-143.f,-109.f,-76.f,-38.f,2.f,38.f,77.f,112.f,144.f};

    /** Interpolate the same diagonal as BuildTerrain, not the analytic surface
     * between its vertices. VertexAt(Side,Row) may read the rendered section. */
    template<typename VertexSampler>
    inline FVector SampleRenderedSurface(float Angle,float V,const VertexSampler& VertexAt)
    {
        float Turns=FMath::Fmod(Angle/UE_TWO_PI,1.f);
        if(Turns<0.f) { Turns+=1.f; }
        const float X=Turns*MeshSectors,Y=FMath::Clamp(V,0.f,1.f)*MeshRows;
        const int32 Side=FMath::Min(FMath::FloorToInt(X),MeshSectors-1);
        const int32 Row=FMath::Min(FMath::FloorToInt(Y),MeshRows-1);
        const float U=X-Side,W=Y-Row;
        const FVector P10=VertexAt(Side+1,Row),P01=VertexAt(Side,Row+1);
        if(U+W<=1.f)
        {
            const FVector P00=VertexAt(Side,Row);
            return P00+(P10-P00)*U+(P01-P00)*W;
        }
        const FVector P11=VertexAt(Side+1,Row+1);
        return P11+(P01-P11)*(1.f-U)+(P10-P11)*(1.f-W);
    }

    inline FVector WaterContactOffset(float Angle,float V)
    {
        // A +Z offset enters downward-facing ledges. Radial inward offset
        // remains on the basin side of every monotone-height wall triangle.
        // The low rubble apron can slope upward/outward. Lift that region only;
        // extending the lift above V=.20 would reintroduce the overhang defect.
        const float TalusLift=120.f*(1.f-FMath::SmoothStep(.10f,.20f,V));
        return FVector(-FMath::Cos(Angle)*180.f,-FMath::Sin(Angle)*180.f,TalusLift);
    }
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

    // The underlying escarpment is continuous. Large smooth-step retreats made
    // hundred-metre clay shelves; fracture volumes now establish the faces.
    inline float FormationProfile(float A,float T)
    {
        return .72f*T+.28f*T*T+.055f*Facet(A,1.9f,17)*FMath::Sin(T*UE_PI);
    }

    inline float FractureMass(float A,float T,int32 Period,float HeightCells,float Amplitude)
    {
        const float X=A*Period,Y=T*HeightCells;
        const int32 CellX=FMath::FloorToInt(X),CellY=FMath::FloorToInt(Y);
        float Mass=0.f;
        for(int32 I=CellX-1;I<=CellX+1;++I) for(int32 J=CellY-1;J<=CellY+1;++J)
        {
            const float H0=Hash(I,J*7+13,Period),H1=Hash(I,J*7+29,Period),H2=Hash(I,J*7+43,Period);
            const float DX=X-(I+.18f+.64f*H0),DY=Y-(J+.18f+.64f*H1);
            const float Width=.46f+.30f*H1,Height=.48f+.28f*H2;
            // Intersections of three oblique planes form actual rock fronts.
            // Their union is continuous but has sharp fractured arrises. No
            // bilinear or smooth-step blending rounds these into pillows.
            const float Front=.42f+(H2-.5f)*.45f*DX+(H0-.5f)*.50f*DY;
            const float Side=1.f-FMath::Abs(DX+.16f*DY)/Width;
            const float Cap=1.f-FMath::Abs(DY-.12f*DX)/Height;
            const float Solid=FMath::Max(0.f,FMath::Min(Front,FMath::Min(Side,Cap)));
            Mass=FMath::Max(Mass,Solid*Amplitude*(.65f+.70f*H0));
        }
        return Mass;
    }

    inline FFaceRelief FaceRelief(float A,float T)
    {
        const float Major=FractureMass(A,T,23,3.7f,21000.f);
        const float Blocks=FractureMass(A+.0021f,T+.17f,97,11.f,5200.f);
        const float Chips=FractureMass(A+.0013f,T+.31f,257,31.f,1450.f);
        // Shadow tint follows exposed fissures between the middle-scale masses.
        const float Recess=1.f-FMath::SmoothStep(250.f,1700.f,Blocks);
        return {-Major-Blocks-Chips,Recess};
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
            // No wholesale radial jumps or terraces: broken angular masses
            // supply the local walls and ledges over the connected escarpment.
            const float RadialT=FormationProfile(A,T);
            R=FMath::Lerp(FaceFoot,S.LipRadius,RadialT);
            const float Envelope=FMath::SmoothStep(0.f,.06f,T)*(1-FMath::SmoothStep(.90f,1.f,T));
            const float DetailScale=FMath::Min(1.f,Gap/30000.f);
            const float Raw=FaceRelief(A,T).Offset*DetailScale;
            // Smoothly bound relief above the shoreline even on the narrow
            // inlet. A hard radial clamp would create obvious flat cut planes.
            const float Limit=FMath::Max(100.f,FMath::Min(24000.f,(R-S.ToeRadius)*.85f));
            // A piecewise-linear safety bound preserves the rock planes. The previous
            // nonlinear limiter visibly rounded every protruding formation.
            R+=Envelope*FMath::Max(Raw,-Limit);
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
