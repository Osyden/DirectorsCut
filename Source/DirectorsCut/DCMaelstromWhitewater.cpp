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
    for(int32 Feed=0;Feed<WaterfallCount;++Feed)
    {
        const float Angle=WaterfallAngle(Feed);
        const float W=WaterfallWidth(Feed);
        if(W<=0.f) { continue; }
        const float Edge=WaterEdgeRadius(Angle);
        if(Edge>GetRimRadius(Angle)*.97f) { continue; }
        const FVector2D End=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*(Edge-900.f);
        const float StartAngle=Angle;
        const FVector StartSurface=CliffSurfacePoint(StartAngle,.84f);
        const FVector2D Start(StartSurface.X,StartSurface.Y);
        const FVector2D Direction=(End-Start).GetSafeNormal();
        const FVector2D Across(-Direction.Y,Direction.X);
        const float Seed=Random.FRand();
        FWaterFeed& Source=WaterFeeds.AddDefaulted_GetRef();
        Source.Foot=FVector(End,HeightAt(End)); Source.Width=W; Source.Seed=Seed;
        Source.Lip=StartSurface;
        const float LipZ=StartSurface.Z+80.f;
        FVector2D Drift=End;
        for(int32 Step=0;Step<24;++Step)
        {
            Source.Trail.Add(Drift);
            Drift+=CurrentAt(Drift,Radius)*1.2f;
        }
        // Split the supply into separate, unequal runs. A single uninterrupted
        // sheet spanning hundreds of metres reads as a painted ribbon.
        for(int32 Branch=0;Branch<3;++Branch)
        {
            const float BranchSeed=FMath::Frac(Seed+Branch*.217f);
            const float BranchOffset=(Branch-1.f)*W*.57f;
            const float BranchWidth=W*Random.FRandRange(.20f,.34f);
            const int32 First=V.Num();
            constexpr int32 Rows=144,Cols=8;
            float Distance=0.f;
            FVector Previous(Start,LipZ);
            for(int32 Row=0;Row<=Rows;++Row)
            {
                const float U=static_cast<float>(Row)/Rows;
                // Gravity accelerates the stream downwards. Keep its centreline
                // coherent; large sine bends made narrow falls look like ropes.
                const float Bend=BranchOffset*(.7f+.3f*U)+FMath::Sin(U*UE_PI)*W*
                    FMath::PerlinNoise1D(U*4.f+BranchSeed*57.f)*.12f;
                FVector Center3=CliffSurfacePoint(Angle,.84f*(1.f-U));
                const FVector Inward(-FMath::Cos(Angle),-FMath::Sin(Angle),0);
                Center3+=Inward*180.f+FVector(Across*Bend,60.f);
                Center3=FMath::Lerp(Center3,Source.Foot+FVector(0,0,40),FMath::SmoothStep(.94f,1.f,U));
                Distance+=(Center3-Previous).Size(); Previous=Center3;
                // A thin core loses cohesion gradually into aerated water at its
                // foot. Width breakup is small-scale, not a repeating zigzag.
                const float Width=BranchWidth*(.8f+.45f*U)*(1.f+.22f*FMath::PerlinNoise1D(U*37.f+BranchSeed*41.f));
                for(int32 Col=0;Col<=Cols;++Col)
                {
                    const float X=static_cast<float>(Col)/Cols;
                    const float SideOffset=(X-.5f)*Width*2.f;
                    const float SampleAngle=Angle+(Bend+SideOffset)/FMath::Max(Center3.Size2D(),1.f);
                    FVector Sample=CliffSurfacePoint(SampleAngle,.84f*(1.f-U));
                    Sample+=FVector(-FMath::Cos(SampleAngle)*180.f,-FMath::Sin(SampleAngle)*180.f,60.f);
                    Sample=FMath::Lerp(Sample,Source.Foot+FVector(Across*(Bend+SideOffset),40.f),FMath::SmoothStep(.94f,1.f,U));
                    V.Add(Sample);
                    UV.Add(FVector2D(X,Distance*.01f));
                    C.Add(FLinearColor(Width/10000.f,BranchSeed,U,1.f).ToFColor(false));
                    if(Row<Rows && Col<Cols)
                    { const int32 A=First+Row*(Cols+1)+Col,B=A+Cols+1; I.Append({A,B,A+1,A+1,B,B+1}); }
                }
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
        for(int32 Fan=0;Fan<17;++Fan)
        {
            // Stagger small spray lobes across the contact. Three full-width
            // parallel sheets read as horizontal shelves from the deck.
            const float H=Random.FRandRange(250.f,750.f),Width=Random.FRandRange(200.f,650.f);
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
                UV.Add(FVector2D(U,T)); C.Add(FLinearColor(1.f,FMath::Frac(Feed.Seed+Fan*.173f),0.f,.48f).ToFColor(false));
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
        for(int32 Drop=0;Drop<28 && ContactParticles.Num()<1600;++Drop)
        {
            const float A=FMath::FRand()*UE_TWO_PI;
            const float R=FMath::FRandRange(0.f,Feed.Width*.75f);
            const FVector2D P=FVector2D(Feed.Foot)+FVector2D(FMath::Cos(A),FMath::Sin(A))*R;
            ContactParticles.Add({FVector(P,HeightAt(P)+80.f),
                FVector(Velocity*.45f+FVector2D(FMath::FRandRange(-400.f,400.f),FMath::FRandRange(-400.f,400.f)),FMath::FRandRange(650.f,1550.f)),
                0.f,FMath::FRandRange(1.f,2.5f),FMath::FRandRange(3.f,16.f)});
        }
    }
}
