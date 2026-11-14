#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "KismetProceduralMeshLibrary.h"

namespace
{
FVector2D CurrentAt(const FVector2D& P,float Radius)
{
    const float T=DCMaelstromRoute::SecondsAtAngle(FMath::Atan2(P.Y,P.X));
    return DCMaelstromRoute::Tangent(FMath::Clamp(T,-7.f,179.f),Radius)*1800.f;
}
}

void ADCMaelstrom::BuildWhitewater()
{
    FRandomStream Random(71421);
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<FColor> C;
    TArray<int32> I; TArray<FProcMeshTangent> Tangents;
    // Narrow, unequal tributaries enter at distinct points along the course.
    // Their width remains metres even where the fall is hundreds of metres tall.
    constexpr float FeedTimes[]={8.f,21.f,27.f,41.f,50.f,68.f,75.f,92.f,109.f,118.f,134.f,141.f};
    for(int32 Feed=0;Feed<UE_ARRAY_COUNT(FeedTimes);++Feed)
    {
        const float T=FeedTimes[Feed]+Random.FRandRange(-1.5f,1.5f);
        const float Angle=DCMaelstromRoute::Angle(T);
        const float W=Random.FRandRange(1000.f,3200.f);
        const FVector2D End=DCMaelstromRoute::Point(T,Radius,(Feed%3 ? 6500.f : -4500.f)*DCMaelstromRoute::WidthScale(T));
        const float StartAngle=Angle+Random.FRandRange(.025f,.10f);
        const FVector2D Start=FVector2D(FMath::Cos(StartAngle),FMath::Sin(StartAngle))*GetRimRadius(StartAngle)*.997f;
        const FVector2D Direction=(End-Start).GetSafeNormal();
        const FVector2D Across(-Direction.Y,Direction.X);
        const float Seed=Random.FRand();
        FWaterFeed& Source=WaterFeeds.AddDefaulted_GetRef();
        Source.Foot=FVector(End,HeightAt(End)); Source.Width=W; Source.Seed=Seed;
        const float LipZ=HeightAt(Start);
        FVector2D Drift=End;
        for(int32 Step=0;Step<24;++Step)
        {
            Source.Trail.Add(Drift);
            Drift+=CurrentAt(Drift,Radius)*1.2f;
        }
        const int32 First=V.Num();
        constexpr int32 Rows=100,Cols=12;
        float Distance=0.f;
        FVector Previous(Start,HeightAt(Start));
        for(int32 Row=0;Row<=Rows;++Row)
        {
            const float U=static_cast<float>(Row)/Rows;
            const float Bend=FMath::Sin(U*UE_PI)*W*FMath::Sin(U*5.f+Seed*10.f)*.8f;
            const FVector2D Centre=FMath::Lerp(Start,End,U)+Across*Bend;
            const FVector Center3(Centre,HeightAt(Centre)+45.f);
            Distance+=(Center3-Previous).Size(); Previous=Center3;
            const float Width=W*(.55f+1.05f*U)*(1.f+.40f*FMath::PerlinNoise1D(U*19.f+Seed*41.f));
            for(int32 Col=0;Col<=Cols;++Col)
            {
                const float X=static_cast<float>(Col)/Cols;
                const FVector2D P=Centre+Across*((X-.5f)*Width*2.f);
                // A falling sheet separates from the wall, then reconnects
                // at the real contact. Surface-only overlays read as paint.
                const float FallZ=FMath::Lerp(LipZ,Source.Foot.Z,U*U);
                const float SurfaceZ=HeightAt(P);
                const float Lift=400.f*(1.f-FMath::SmoothStep(.65f,1.f,U))+25.f;
                V.Add(FVector(P,FMath::Max(SurfaceZ,FallZ)+Lift));
                UV.Add(FVector2D(X,Distance*.01f));
                C.Add(FLinearColor(Width/10000.f,Seed,U,1.f).ToFColor(false));
                if(Row<Rows && Col<Cols)
                { const int32 A=First+Row*(Cols+1)+Col,B=A+Cols+1; I.Append({A,B,A+1,A+1,B,B+1}); }
            }
        }
    }
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V,I,UV,N,Tangents);
    Whitewater->CreateMeshSection(0,V,I,N,UV,C,Tangents,false);
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_MaelstromWhitewater.M_MaelstromWhitewater")))
    {
        WhitewaterMID=UMaterialInstanceDynamic::Create(M,this);
        Whitewater->SetMaterial(0,WhitewaterMID);
    }
    V.Reset(); N.Reset(); UV.Reset(); C.Reset(); I.Reset(); Tangents.Reset();
    for(const FWaterFeed& Feed:WaterFeeds)
    {
        const FVector2D Direction=CurrentAt(FVector2D(Feed.Foot),Radius).GetSafeNormal();
        const FVector2D Across(-Direction.Y,Direction.X);
        for(int32 Fan=0;Fan<11;++Fan)
        {
            // Stagger small spray lobes across the contact. Three full-width
            // parallel sheets read as horizontal shelves from the deck.
            const float H=Random.FRandRange(280.f,850.f),Width=Random.FRandRange(350.f,950.f);
            const FVector2D Offset=Across*Random.FRandRange(-Feed.Width*.8f,Feed.Width*.8f)
                +Direction*Random.FRandRange(-350.f,900.f);
            const int32 First=V.Num();
            constexpr int32 Rows=8,Cols=12;
            for(int32 Y=0;Y<=Rows;++Y) for(int32 X=0;X<=Cols;++X)
            {
                const float U=static_cast<float>(X)/Cols,T=static_cast<float>(Y)/Rows;
                const FVector2D P=FVector2D(Feed.Foot)+Offset+Across*((U-.5f)*Width*2.f)
                    +Direction*(T*H*1.4f);
                const float Crown=.45f+.55f*FMath::Sin(U*UE_PI);
                V.Add(FVector(P,HeightAt(P)+40.f+FMath::Sin(T*UE_PI)*H*Crown));
                UV.Add(FVector2D(U,T)); C.Add(FLinearColor(1.f,FMath::Frac(Feed.Seed+Fan*.173f),0.f,.65f).ToFColor(false));
                if(Y<Rows && X<Cols) { const int32 A=First+Y*(Cols+1)+X,B=A+Cols+1; I.Append({A,B,A+1,A+1,B,B+1}); }
            }
        }
    }
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V,I,UV,N,Tangents);
    Whitewater->CreateMeshSection(1,V,I,N,UV,C,Tangents,false);
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_MaelstromImpactMist.M_MaelstromImpactMist")))
    { Whitewater->SetMaterial(1,M); }
    UE_LOG(LogTemp,Display,TEXT("DC_WHITEWATER %d surface-connected feeds and downstream trails"),WaterFeeds.Num());
}

void ADCMaelstrom::UpdateWhitewater()
{
    if(WhitewaterMID) { WhitewaterMID->SetScalarParameterValue(TEXT("Clock"),Elapsed); }
    if(Elapsed<0.f || Elapsed<NextFeedSpray) { return; }
    NextFeedSpray=Elapsed+.08f;
    const FTransform Frame=GetArenaTransform();
    for(const FWaterFeed& Feed:WaterFeeds)
    {
        const FVector World=Frame.TransformPosition(Feed.Foot);
        if(World.Size2D()>45000.f || World.Size2D()<2600.f) { continue; }
        const FVector2D Velocity=CurrentAt(FVector2D(Feed.Foot),Radius);
        // Fine droplets and a few heavier fragments leave the actual contact,
        // follow the current, then fall. No explosion spheres or smoke cards.
        for(int32 Drop=0;Drop<18 && ContactParticles.Num()<1600;++Drop)
        {
            const float A=FMath::FRand()*UE_TWO_PI;
            const float R=FMath::FRandRange(0.f,Feed.Width*.75f);
            const FVector2D P=FVector2D(Feed.Foot)+FVector2D(FMath::Cos(A),FMath::Sin(A))*R;
            ContactParticles.Add({FVector(P,HeightAt(P)+80.f),
                FVector(Velocity*.45f+FVector2D(FMath::FRandRange(-400.f,400.f),FMath::FRandRange(-400.f,400.f)),FMath::FRandRange(650.f,1550.f)),
                0.f,FMath::FRandRange(1.f,2.8f),FMath::FRandRange(8.f,30.f)});
        }
    }
}
