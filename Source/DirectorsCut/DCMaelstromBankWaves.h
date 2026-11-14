#pragma once

#include "CoreMinimal.h"
#include "DCMaelstromRoute.h"

/** Finite standing-wave packets on the outer hydraulic bank. Centimetres.
 * The existing material animates smaller water detail over this geometric
 * relief. No circular phase, global wave train, or new foam ring is added. */
namespace DCMaelstromBankWaves
{
    inline constexpr float CellSize=4800.f,MaximumRelief=200.f;

    inline uint32 Hash(int32 X,int32 Y,uint32 Salt)
    {
        uint32 H=static_cast<uint32>(X)*1597334677u+static_cast<uint32>(Y)*3812015801u+Salt;
        H=(H^(H>>16))*2246822519u; H=(H^(H>>13))*3266489917u;
        return H^(H>>16);
    }

    inline float Random(int32 X,int32 Y,uint32 Salt)
    {
        return static_cast<float>(Hash(X,Y,Salt)&0x00ffffffu)/16777215.f;
    }

    inline float Packet(float Travel,float Crest,float HalfTravel,float HalfCrest,float Wavelength,float Phase)
    {
        const float U=Travel/HalfTravel,V=Crest/HalfCrest;
        if(FMath::Abs(U)>=1.f || FMath::Abs(V)>=1.f) { return 0.f; }
        const float A=1.f-U*U,B=1.f-V*V;
        const float E=A*A*A,DE=-6.f*U*A*A/HalfTravel;
        const float K=UE_TWO_PI/Wavelength,Theta=K*Travel+Phase;
        // d/dx[E*sin(kx+phase)]/k has exactly zero integrated displacement.
        // A smaller second harmonic makes a sharper crest without a broad
        // positive Gaussian mound. Both height and slope vanish at each edge.
        const float First=E*FMath::Cos(Theta)+DE/K*FMath::Sin(Theta);
        const float Second=E*FMath::Cos(2.f*Theta+.73f)+DE/(2.f*K)*FMath::Sin(2.f*Theta+.73f);
        return (First+.18f*Second)*B*B*B;
    }

    template<typename ShoreSampler>
    inline float Height(const FVector2D& Point,float Radius,const ShoreSampler& ShoreAt)
    {
        const float R=Point.Size(),Angle=FMath::Atan2(Point.Y,Point.X);
        const float T=DCMaelstromRoute::SecondsAtAngle(Angle);
        const float Active=FMath::SmoothStep(22.f,40.f,T)*(1.f-FMath::SmoothStep(165.f,178.f,T));
        if(Active<=0.f) { return 0.f; }
        const float Width=DCMaelstromRoute::WidthScale(T);
        const float Across=R-Radius*DCMaelstromRoute::RadialFraction(T);
        // Full steering is +/-110m. Keep another15m for the hull even when the
        // late route narrows. This candidate cannot change ship position/grade.
        const float Inner=FMath::Max(12500.f,15000.f*Width);
        const float Bank=FMath::SmoothStep(Inner,Inner+2500.f,Across)
            *(1.f-FMath::SmoothStep(33000.f*Width+1000.f,38000.f*Width+1000.f,Across));
        if(Bank<=0.f) { return 0.f; }
        // Leave the shore and waterfall receiving strip exactly unchanged.
        const float Coast=FMath::SmoothStep(3500.f,6500.f,ShoreAt(Angle)-R);
        if(Coast<=0.f) { return 0.f; }

        const int32 CellX=FMath::FloorToInt(Point.X/CellSize),CellY=FMath::FloorToInt(Point.Y/CellSize);
        float Raw=0.f;
        // A packet reaches at most sqrt(44^2+28^2)=52.2m from its centre.
        // A non-neighbour centre is at least57.6m away (.2-cell jitter margin),
        // so exactly these nine cells are sufficient, including at cell seams.
        for(int32 Y=CellY-1;Y<=CellY+1;++Y) for(int32 X=CellX-1;X<=CellX+1;++X)
        {
            const FVector2D Center((X+.2f+.6f*Random(X,Y,1013904223u))*CellSize,
                (Y+.2f+.6f*Random(X,Y,2246822519u))*CellSize);
            const FVector2D Delta=Point-Center;
            if(Delta.SizeSquared()>27200000.f) { continue; }
            const float CenterT=DCMaelstromRoute::SecondsAtAngle(FMath::Atan2(Center.Y,Center.X));
            const FVector2D Current=DCMaelstromRoute::Tangent(FMath::Clamp(CenterT,-7.f,179.f),Radius);
            const float Skew=(Random(X,Y,3266489917u)-.5f)*.42f;
            const FVector2D AcrossCurrent(-Current.Y,Current.X);
            const FVector2D Along=Current*FMath::Cos(Skew)+AcrossCurrent*FMath::Sin(Skew);
            const FVector2D Cross(-Along.Y,Along.X);
            // Actual448x1536 bank grid has up to4.40m planar neighbour spacing,
            // including shoreline-induced skew. The conservative4.46m floor
            // keeps at least six planar mesh samples
            // per wavelength; 12m waves belong to the material at this density.
            const float MeshStep=FMath::Max(Radius*1.25f/448.f,static_cast<float>(Center.Size())*UE_TWO_PI/1536.f);
            const float Wavelength=FMath::Max(2400.f+1400.f*Random(X,Y,668265263u),6.f*MeshStep);
            const float HalfTravel=FMath::Min(4400.f,Wavelength*(.95f+.20f*Random(X,Y,374761393u)));
            const float HalfCrest=1800.f+1000.f*Random(X,Y,1274126177u);
            const float Amplitude=90.f+100.f*Random(X,Y,2048419325u);
            Raw+=Amplitude*Packet(FVector2D::DotProduct(Delta,Along),FVector2D::DotProduct(Delta,Cross),
                HalfTravel,HalfCrest,Wavelength,UE_TWO_PI*Random(X,Y,2654435761u));
        }
        // Odd smooth limiting keeps signed troughs and bounds every superposed
        // packet to2m. No permanent positive-height shelf is added to the bank.
        return Raw/FMath::Sqrt(1.f+FMath::Square(Raw/MaximumRelief))*Active*Bank*Coast;
    }
}
