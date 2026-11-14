#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
#include "DCCliffGenerator.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "KismetProceduralMeshLibrary.h"

namespace
{
struct FCoreVertex { FVector Position; FVector2D Parameter; };
using FCorePolygon=TArray<FCoreVertex,TInlineAllocator<8>>;

template<typename DistanceFunction>
void ClipCorePolygon(FCorePolygon& Polygon,const DistanceFunction& SignedDistance)
{
    if(Polygon.IsEmpty()) { return; }
    FCorePolygon Result;
    FCoreVertex Previous=Polygon.Last();
    float PreviousD=SignedDistance(Previous.Parameter);
    for(const FCoreVertex& Current:Polygon)
    {
        const float CurrentD=SignedDistance(Current.Parameter);
        if((PreviousD>=0.f)!=(CurrentD>=0.f))
        {
            const float Alpha=PreviousD/(PreviousD-CurrentD);
            Result.Add({FMath::Lerp(Previous.Position,Current.Position,Alpha),
                FMath::Lerp(Previous.Parameter,Current.Parameter,Alpha)});
        }
        if(CurrentD>=0.f) { Result.Add(Current); }
        Previous=Current; PreviousD=CurrentD;
    }
    Polygon=MoveTemp(Result);
}

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
    const FProcMeshSection* CliffSection=Cliffs->GetProcMeshSection(0);
    constexpr int32 CliffSectors=DCCliffGenerator::MeshSectors,CliffRows=DCCliffGenerator::MeshRows;
    if(!ensureMsgf(CliffSection && CliffSection->ProcVertexBuffer.Num()==(CliffSectors+1)*(CliffRows+1),
        TEXT("Whitewater requires the generated cliff mesh"))) { return; }
    const auto CliffVertex=[&](int32 Side,int32 Row)
    { return CliffSection->ProcVertexBuffer[Row*(CliffSectors+1)+Side].Position; };
    float MinimumCoreClearance=MAX_flt;
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
        const FVector StartSurface=DCCliffGenerator::SampleRenderedSurface(StartAngle,.84f,CliffVertex);
        // Increasing sample angle moves in the positive circumferential direction.
        // The old endpoint used the opposite vector and twisted the last6%.
        const FVector2D Across(-FMath::Sin(Angle),FMath::Cos(Angle));
        const float Seed=Random.FRand();
        FWaterFeed& Source=WaterFeeds.AddDefaulted_GetRef();
        Source.Foot=FVector(End,HeightAt(End)); Source.Width=W; Source.Seed=Seed;
        Source.Lip=StartSurface;
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
            struct FStreamSection { float U,Bend,Width,Radius,Angle; };
            const auto StreamSection=[&](float SurfaceV)
            {
                const float U=FMath::Clamp(1.f-SurfaceV/.84f,0.f,1.f);
                const float Bend=BranchOffset*(.7f+.3f*U)+FMath::Sin(U*UE_PI)*W*
                    FMath::PerlinNoise1D(U*4.f+BranchSeed*57.f)*.12f;
                const float Width=BranchWidth*(.8f+.45f*U)*(1.f+.22f*FMath::PerlinNoise1D(U*37.f+BranchSeed*41.f));
                const float R=DCCliffGenerator::SampleRenderedSurface(Angle,SurfaceV,CliffVertex).Size2D();
                return FStreamSection{U,Bend,Width,R,Angle+Bend/FMath::Max(R,1.f)};
            };
            constexpr int32 PathSteps=144;
            TArray<float,TInlineAllocator<PathSteps+1>> Distances;
            FVector Previous=FVector::ZeroVector;
            for(int32 Step=0;Step<=PathSteps;++Step)
            {
                const float SurfaceV=.84f*(1.f-static_cast<float>(Step)/PathSteps);
                const FStreamSection Stream=StreamSection(SurfaceV);
                FVector P=DCCliffGenerator::SampleRenderedSurface(Stream.Angle,SurfaceV,CliffVertex)
                    +DCCliffGenerator::WaterContactOffset(Stream.Angle,SurfaceV);
                P=FMath::Lerp(P,Source.Foot+FVector(Across*Stream.Bend,40.f),FMath::SmoothStep(.94f,1.f,Stream.U));
                Distances.Add(Step ? Distances.Last()+FVector::Dist(P,Previous) : 0.f);
                Previous=P;
            }
            const auto AddTriangle=[&](const FCoreVertex& A,const FCoreVertex& B,const FCoreVertex& D)
            {
                const FCoreVertex Corners[]={A,B,D};
                const FVector RockNormal=FVector::CrossProduct(D.Position-A.Position,B.Position-A.Position).GetSafeNormal();
                if(RockNormal.IsNearlyZero()) { return; }
                const int32 First=V.Num();
                for(const FCoreVertex& Corner:Corners)
                {
                    const float SampleAngle=Corner.Parameter.X;
                    const FStreamSection Stream=StreamSection(Corner.Parameter.Y);
                    const float SideOffset=(SampleAngle-Stream.Angle)*Stream.Radius;
                    const float X=FMath::Clamp(.5f+SideOffset/FMath::Max(2.f*Stream.Width,1.f),0.f,1.f);
                    FVector Sample=Corner.Position+DCCliffGenerator::WaterContactOffset(SampleAngle,Corner.Parameter.Y);
                    if(Stream.U<.94f)
                    { MinimumCoreClearance=FMath::Min(MinimumCoreClearance,static_cast<float>(FVector::DotProduct(Sample-Corner.Position,RockNormal))); }
                    Sample=FMath::Lerp(Sample,Source.Foot+FVector(Across*(Stream.Bend+SideOffset),40.f),FMath::SmoothStep(.94f,1.f,Stream.U));
                    const float Path=Stream.U*PathSteps;
                    const int32 Step=FMath::Min(FMath::FloorToInt(Path),PathSteps-1);
                    const float Distance=FMath::Lerp(Distances[Step],Distances[Step+1],Path-Step);
                    V.Add(Sample);
                    UV.Add(FVector2D(X,Distance*.01f));
                    C.Add(FLinearColor(Stream.Width/10000.f,BranchSeed,Stream.U,1.f).ToFColor(false));
                }
                const FVector WaterNormal=FVector::CrossProduct(V[First+2]-V[First],V[First+1]-V[First]).GetSafeNormal();
                const FProcMeshTangent Tangent(FVector(Across,0.f),false);
                for(int32 Corner=0;Corner<3;++Corner) { N.Add(WaterNormal); Tangents.Add(Tangent); }
                I.Append({First,First+1,First+2});
            };
            // Clip the stream out of the rendered rock triangles. An unrelated
            // 144x8 grid cut through fracture peaks even when its vertices were
            // on the analytic cliff. Matching triangles removes that chord error.
            const float Margin=1.8f*W/FMath::Max(Edge*.9f,1.f);
            const int32 FirstSide=FMath::FloorToInt((Angle-Margin)*CliffSectors/UE_TWO_PI);
            const int32 LastSide=FMath::CeilToInt((Angle+Margin)*CliffSectors/UE_TWO_PI);
            for(int32 Row=0;Row<CliffRows*.84f;++Row) for(int32 Side=FirstSide;Side<=LastSide;++Side)
            {
                const int32 Wrapped=(Side%CliffSectors+CliffSectors)%CliffSectors;
                const float A0=Side*UE_TWO_PI/CliffSectors,A1=(Side+1)*UE_TWO_PI/CliffSectors;
                const float V0=static_cast<float>(Row)/CliffRows,V1=static_cast<float>(Row+1)/CliffRows;
                const FCoreVertex Corners[]={{CliffVertex(Wrapped,Row),FVector2D(A0,V0)},
                    {CliffVertex(Wrapped+1,Row),FVector2D(A1,V0)},
                    {CliffVertex(Wrapped,Row+1),FVector2D(A0,V1)},
                    {CliffVertex(Wrapped+1,Row+1),FVector2D(A1,V1)}};
                for(int32 Tri=0;Tri<2;++Tri)
                {
                    FCorePolygon Polygon;
                    if(Tri==0) { Polygon.Append({Corners[0],Corners[1],Corners[2]}); }
                    else { Polygon.Append({Corners[1],Corners[3],Corners[2]}); }
                    ClipCorePolygon(Polygon,[](const FVector2D& P){return .84f-static_cast<float>(P.Y);});
                    ClipCorePolygon(Polygon,[&](const FVector2D& P){const FStreamSection S=StreamSection(P.Y); return static_cast<float>(P.X-S.Angle)*S.Radius+S.Width;});
                    ClipCorePolygon(Polygon,[&](const FVector2D& P){const FStreamSection S=StreamSection(P.Y); return S.Width-static_cast<float>(P.X-S.Angle)*S.Radius;});
                    for(int32 Corner=1;Corner+1<Polygon.Num();++Corner)
                    { AddTriangle(Polygon[0],Polygon[Corner],Polygon[Corner+1]); }
                }
            }
        }
    }
    ensureMsgf(MinimumCoreClearance>0.f,TEXT("Waterfall core enters the rendered cliff"));
    UE_LOG(LogTemp,Display,TEXT("DC_WHITEWATER_CONTACT clipped %d triangles to rendered cliff; minimum face clearance %.2fcm"),I.Num()/3,MinimumCoreClearance);
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
