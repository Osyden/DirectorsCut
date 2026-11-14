#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
#include "DCCliffGenerator.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "KismetProceduralMeshLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

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

bool ADCMaelstrom::SampleReceivingWater(const FVector2D& Point,float& OutMeanHeight) const
{
    const float R=Point.Size();
    if(R<120.f || R>WaterEdgeRadius(FMath::Atan2(Point.Y,Point.X))) { return false; }
    OutMeanHeight=HeightAt(Point,false);
    return true;
}

FVector ADCMaelstrom::SampleAttachedWaterfallPoint(float Angle,float V,float Across) const
{
    const FProcMeshSection* Section=Cliffs ? Cliffs->GetProcMeshSection(0) : nullptr;
    if(!Section || Section->ProcVertexBuffer.Num()!=(DCCliffGenerator::MeshSectors+1)*(DCCliffGenerator::MeshRows+1))
    { return FVector::ZeroVector; }
    const auto VertexAt=[Section](int32 Side,int32 Row)
    { return Section->ProcVertexBuffer[Row*(DCCliffGenerator::MeshSectors+1)+Side].Position; };
    const FVector Center=DCCliffGenerator::SampleRenderedSurface(Angle,V,VertexAt);
    const float SampleAngle=Angle+Across/FMath::Max(Center.Size2D(),1.0);
    return DCCliffGenerator::SampleRenderedSurface(SampleAngle,V,VertexAt)
        +DCCliffGenerator::WaterContactOffset(SampleAngle,V);
}

FVector ADCMaelstrom::SampleWaterfallCorePoint(float Angle,float V,float Across) const
{
    if(DetachedFallSource!=INDEX_NONE && FMath::Abs(FMath::FindDeltaAngleRadians(Angle,WaterfallAngle(DetachedFallSource)))<.00001f)
    {
        if(const DCWaterfallTrajectory::FSegment* Segment=DCWaterfallTrajectory::Find(DetachedFallSegments,V))
        {
            return DCWaterfallTrajectory::Sample(*Segment,
                [&](float Parameter,float Side){return SampleAttachedWaterfallPoint(Angle,Parameter,Side);},V,Across);
        }
    }
    return SampleAttachedWaterfallPoint(Angle,V,Across);
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
    const auto ReceivingWater=[this](const FVector2D& P,float& H) { return SampleReceivingWater(P,H); };
    DetachedFallSegments.Reset(); DetachedFallSource=INDEX_NONE;
    if(FParse::Param(FCommandLine::Get(),TEXT("DCDetachedFalls")))
    {
        int32 Requested=8;
        FParse::Value(FCommandLine::Get(),TEXT("DCDetachedFallFeed="),Requested);
        if(Requested>=0 && Requested<WaterfallCount && WaterfallWidth(Requested)>0.f)
        {
            const float A=WaterfallAngle(Requested);
            const auto Surface=[&](float Parameter,float Side){return SampleAttachedWaterfallPoint(A,Parameter,Side);};
            const auto Contact=DCCliffGenerator::FirstWaterContact([&](float T){return Surface(.84f*(1.f-T),0.f);},ReceivingWater);
            if(Contact.bFound)
            {
                DetachedFallSource=Requested;
                DetachedFallSegments=DCWaterfallTrajectory::Build(Surface,ReceivingWater,WaterfallWidth(Requested),Contact.WaterHeight);
                for(const DCWaterfallTrajectory::FSegment& Segment:DetachedFallSegments)
                {
                    UE_LOG(LogTemp,Display,TEXT("DC_DETACHED_FALL source=%d V=%.6f..%.6f duration=%.3fs down=%.2fm/s peak_gap=%.2fm min_clearance=%.3fcm"),
                        Requested,Segment.StartV,Segment.EndV,Segment.Duration,Segment.InitialDown*.01f,Segment.PeakSeparation*.01f,Segment.MinClearance);
                }
                UE_LOG(LogTemp,Display,TEXT("DC_DETACHED_FALL source=%d accepted=%d; protected toe and first receiving-water contact unchanged"),Requested,DetachedFallSegments.Num());
            }
        }
    }
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
        const float StartAngle=Angle;
        const FVector StartSurface=DCCliffGenerator::SampleRenderedSurface(StartAngle,.84f,CliffVertex);
        // Increasing sample angle moves in the positive circumferential direction.
        // The old endpoint used the opposite vector and twisted the last6%.
        const FVector2D Across(-FMath::Sin(Angle),FMath::Cos(Angle));
        const float Seed=Random.FRand();
        const auto Contact=DCCliffGenerator::FirstWaterContact(
            [&](float T){return SampleWaterfallCorePoint(Angle,.84f*(1.f-T));},ReceivingWater);
        if(!ensureMsgf(Contact.bFound,TEXT("Waterfall source has no covered receiving-water contact"))) { continue; }
        FWaterFeed& Source=WaterFeeds.AddDefaulted_GetRef();
        Source.SourceIndex=Feed;
        Source.ContactV=.84f*(1.f-Contact.Progress); Source.ContactPoint=Contact.Position;
        Source.Foot=FVector(FVector2D(Contact.Position),Contact.WaterHeight); Source.Width=W; Source.Seed=Seed;
        Source.Lip=StartSurface;
        UE_LOG(LogTemp,Display,TEXT("DC_WATERFALL_RECEIVING_CONTACT feed=%d V=%.7f core=%s surface=%s submerged_depth_cm=%.2f coverage_margin_cm=%.3f"),
            Feed,Source.ContactV,*Source.ContactPoint.ToCompactString(),*Source.Foot.ToCompactString(),
            Source.Foot.Z-Source.ContactPoint.Z,WaterEdgeRadius(FMath::Atan2(Source.Foot.Y,Source.Foot.X))-Source.Foot.Size2D());
        FVector2D Drift(Source.Foot);
        for(int32 Step=0;Step<24;++Step)
        {
            float MeanHeight=0.f;
            if(!SampleReceivingWater(Drift,MeanHeight)) { break; }
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
            // Track first contact across the complete branch width, not just
            // its centre. A sloping shoreline reaches each part at a different V.
            constexpr int32 ContactTracks=33;
            TArray<float,TInlineAllocator<ContactTracks>> ContactVs;
            bool bAllContactsValid=true;
            float LowestEndpointClearance=MAX_flt,HighestEndpointClearance=-MAX_flt;
            for(int32 Track=0;Track<ContactTracks;++Track)
            {
                const float Side=2.f*Track/(ContactTracks-1)-1.f;
                const auto Hit=DCCliffGenerator::FirstWaterContact([&](float T)
                {
                    const float SurfaceV=.84f*(1.f-T);
                    const FStreamSection S=StreamSection(SurfaceV);
                    return SampleWaterfallCorePoint(Angle,SurfaceV,S.Bend+Side*S.Width);
                },ReceivingWater);
                ContactVs.Add(Hit.bFound ? .84f*(1.f-Hit.Progress) : .84f);
                float MeanHeight=0.f;
                const bool bCovered=Hit.bFound && SampleReceivingWater(FVector2D(Hit.Position),MeanHeight);
                const float Clearance=bCovered ? Hit.Position.Z-MeanHeight : MAX_flt;
                bAllContactsValid &= bCovered && Clearance<=.1f;
                if(bCovered)
                {
                    LowestEndpointClearance=FMath::Min(LowestEndpointClearance,Clearance);
                    HighestEndpointClearance=FMath::Max(HighestEndpointClearance,Clearance);
                }
            }
            if(!ensureMsgf(bAllContactsValid,TEXT("Waterfall branch endpoint lies outside covered receiving water"))) { continue; }
            const auto ContactVAt=[&](const FVector2D& Parameter)
            {
                const FStreamSection S=StreamSection(Parameter.Y);
                const float Side=(Parameter.X-S.Angle)*S.Radius/FMath::Max(2.f*S.Width,1.f)+.5f;
                const float Track=FMath::Clamp(Side,0.f,1.f)*(ContactTracks-1);
                const int32 Index=FMath::Min(FMath::FloorToInt(Track),ContactTracks-2);
                return FMath::Lerp(ContactVs[Index],ContactVs[Index+1],Track-Index);
            };
            UE_LOG(LogTemp,Display,TEXT("DC_WATERFALL_FIRST_CONTACT feed=%d branch=%d tracks=%d V=%.6f..%.6f endpoint_mean_clearance_cm=%.2f..%.2f covered=1"),
                Feed,Branch,ContactTracks,FMath::Min(ContactVs[0],ContactVs.Last()),FMath::Max(ContactVs[0],ContactVs.Last()),
                LowestEndpointClearance,HighestEndpointClearance);
            constexpr int32 PathSteps=144;
            TArray<float,TInlineAllocator<PathSteps+1>> Distances;
            FVector Previous=FVector::ZeroVector;
            for(int32 Step=0;Step<=PathSteps;++Step)
            {
                const float SurfaceV=.84f*(1.f-static_cast<float>(Step)/PathSteps);
                const FStreamSection Stream=StreamSection(SurfaceV);
                const FVector P=SampleWaterfallCorePoint(Angle,SurfaceV,Stream.Bend);
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
                    const float Path=Stream.U*PathSteps;
                    const int32 Step=FMath::Min(FMath::FloorToInt(Path),PathSteps-1);
                    const float Distance=FMath::Lerp(Distances[Step],Distances[Step+1],Path-Step);
                    V.Add(Sample);
                    UV.Add(FVector2D(X,Distance*.01f));
                    const float Progress=FMath::Clamp(Stream.U/FMath::Max(.001f,1.f-ContactVAt(Corner.Parameter)/.84f),0.f,1.f);
                    C.Add(FLinearColor(Stream.Width/10000.f,BranchSeed,Progress,1.f).ToFColor(false));
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
                    // Discard everything beyond first contact, including any
                    // later upward curl or re-emergence of the underlying rock.
                    ClipCorePolygon(Polygon,[&](const FVector2D& P){return static_cast<float>(P.Y)-ContactVAt(P);});
                    // Remove attached water exactly where the detached core
                    // replaces it. Leaving the old strip creates a double fall.
                    TArray<FCorePolygon> Remaining; Remaining.Add(MoveTemp(Polygon));
                    if(Feed==DetachedFallSource) for(const DCWaterfallTrajectory::FSegment& Segment:DetachedFallSegments)
                    {
                        TArray<FCorePolygon> Split;
                        for(const FCorePolygon& Piece:Remaining)
                        {
                            FCorePolygon Above=Piece,Below=Piece;
                            ClipCorePolygon(Above,[&](const FVector2D& P){return static_cast<float>(P.Y)-Segment.StartV;});
                            ClipCorePolygon(Below,[&](const FVector2D& P){return Segment.EndV-static_cast<float>(P.Y);});
                            if(Above.Num()>=3) { Split.Add(MoveTemp(Above)); }
                            if(Below.Num()>=3) { Split.Add(MoveTemp(Below)); }
                        }
                        Remaining=MoveTemp(Split);
                    }
                    for(const FCorePolygon& Piece:Remaining) for(int32 Corner=1;Corner+1<Piece.Num();++Corner)
                    { AddTriangle(Piece[0],Piece[Corner],Piece[Corner+1]); }
                }
            }
            if(Feed==DetachedFallSource) for(const DCWaterfallTrajectory::FSegment& Segment:DetachedFallSegments)
            {
                constexpr int32 Rows=64,Columns=12;
                const auto Surface=[&](float Parameter,float Side){return SampleAttachedWaterfallPoint(Angle,Parameter,Side);};
                const FVector Launch=Surface(Segment.StartV,0.f);
                const int32 First=V.Num();
                for(int32 Row=0;Row<=Rows;++Row)
                {
                    const float Time=Segment.Duration*Row/Rows;
                    const float Z=Launch.Z-Segment.InitialDown*Time-.5f*DCWaterfallTrajectory::Gravity*Time*Time;
                    const float Parameter=Row==0 ? Segment.StartV : Row==Rows ? Segment.EndV
                        : DCWaterfallTrajectory::ParameterAtHeight(Surface,Z,Segment.EndV,Segment.StartV);
                    const FStreamSection S=StreamSection(Parameter);
                    const float Path=S.U*PathSteps;
                    const int32 Step=FMath::Min(FMath::FloorToInt(Path),PathSteps-1);
                    const float Distance=FMath::Lerp(Distances[Step],Distances[Step+1],Path-Step);
                    for(int32 Column=0;Column<=Columns;++Column)
                    {
                        const float Side=2.f*Column/Columns-1.f;
                        const FVector P=SampleWaterfallCorePoint(Angle,Parameter,S.Bend+Side*S.Width);
                        V.Add(P); UV.Add(FVector2D(static_cast<float>(Column)/Columns,Distance*.01f));
                        N.Add(FVector::UpVector); Tangents.Add(FProcMeshTangent(FVector(Across,0.f),false));
                        C.Add(FLinearColor(S.Width/10000.f,BranchSeed,S.U,1.f).ToFColor(false));
                    }
                }
                for(int32 Row=0;Row<Rows;++Row) for(int32 Column=0;Column<Columns;++Column)
                {
                    const int32 A=First+Row*(Columns+1)+Column,B=A+Columns+1;
                    I.Append({A,B,A+1,A+1,B,B+1});
                }
            }
        }
    }
    ensureMsgf(MinimumCoreClearance>0.f,TEXT("Waterfall core enters the rendered cliff"));
    UE_LOG(LogTemp,Display,TEXT("DC_WHITEWATER_CONTACT clipped %d triangles to rendered cliff; minimum face clearance %.2fcm"),I.Num()/3,MinimumCoreClearance);
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
        // The hero trial gets compact gravity splash sheets below. Do not
        // layer the previous broad mist fans over the same receiving contact.
        if(Feed.SourceIndex==DetachedFallSource && !DetachedFallSegments.IsEmpty()) { continue; }
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
            TArray<bool,TInlineAllocator<(Rows+1)*(Cols+1)>> Covered;
            for(int32 Y=0;Y<=Rows;++Y) for(int32 X=0;X<=Cols;++X)
            {
                const float U=static_cast<float>(X)/Cols,T=static_cast<float>(Y)/Rows;
                const FVector2D P=FVector2D(Feed.Foot)+Offset+Across*((U-.5f)*Width*2.f)
                    +Direction*(T*H*1.4f);
                const float Crown=.45f+.55f*FMath::Sin(U*UE_PI);
                float MeanHeight=Feed.Foot.Z;
                Covered.Add(SampleReceivingWater(P,MeanHeight));
                V.Add(FVector(P,MeanHeight+40.f+FMath::Sin(T*UE_PI)*H*Crown));
                UV.Add(FVector2D(U,T)); C.Add(FLinearColor(1.f,FMath::Frac(Feed.Seed+Fan*.173f),0.f,.48f).ToFColor(false));
            }
            for(int32 Y=0;Y<Rows;++Y) for(int32 X=0;X<Cols;++X)
            {
                const int32 A=Y*(Cols+1)+X,B=A+Cols+1;
                if(Covered[A] && Covered[B] && Covered[A+1] && Covered[B+1])
                { I.Append({First+A,First+B,First+A+1,First+A+1,First+B,First+B+1}); }
            }
        }
    }
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V,I,UV,N,Tangents);
    Whitewater->CreateMeshSection(1,V,I,N,UV,C,Tangents,false);
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_MaelstromImpactMist.M_MaelstromImpactMist")))
    { Whitewater->SetMaterial(1,M); }
    V.Reset(); N.Reset(); UV.Reset(); C.Reset(); I.Reset(); Tangents.Reset();
    for(const FWaterFeed& Feed:WaterFeeds)
    {
        if(Feed.SourceIndex!=DetachedFallSource || DetachedFallSegments.IsEmpty()) { continue; }
        FRandomStream SplashRandom(51971+Feed.SourceIndex);
        const FVector2D Current=CurrentAt(FVector2D(Feed.Foot),Radius).GetSafeNormal();
        const FVector2D Lateral(-Current.Y,Current.X);
        for(int32 Jet=0;Jet<27;++Jet)
        {
            const FVector2D StartXY=FVector2D(Feed.Foot)+Lateral*SplashRandom.FRandRange(-Feed.Width*.65f,Feed.Width*.65f)
                +Current*SplashRandom.FRandRange(30.f,240.f);
            float StartZ=0.f;
            if(!SampleReceivingWater(StartXY,StartZ)) { continue; }
            const FVector Start(StartXY,StartZ+15.f);
            const FVector2D Horizontal=Current*SplashRandom.FRandRange(350.f,850.f)+Lateral*SplashRandom.FRandRange(-380.f,380.f);
            const FVector Velocity(Horizontal,SplashRandom.FRandRange(650.f,1250.f));
            const auto At=[&](float T){return Start+Velocity*T-FVector(0,0,.5f*DCWaterfallTrajectory::Gravity*T*T);};
            float EndTime=0.f,PreviousTime=0.f;
            for(int32 Step=1;Step<=100;++Step)
            {
                const float T=Step*.04f; const FVector P=At(T); float H=0.f;
                if(!SampleReceivingWater(FVector2D(P),H)) { break; }
                if(P.Z<=H)
                {
                    float Low=PreviousTime,High=T;
                    for(int32 Iter=0;Iter<18;++Iter)
                    {
                        const float Mid=(Low+High)*.5f; const FVector Q=At(Mid); float Mean=0.f;
                        if(SampleReceivingWater(FVector2D(Q),Mean) && Q.Z<=Mean) { High=Mid; } else { Low=Mid; }
                    }
                    EndTime=High; break;
                }
                PreviousTime=T;
            }
            if(EndTime<.1f) { continue; }
            constexpr int32 Rows=32,Columns=4;
            const int32 First=V.Num();
            const FVector2D AcrossJet(-Horizontal.Y,Horizontal.X);
            const FVector WidthDirection(AcrossJet.GetSafeNormal(),0.f);
            const float Width=SplashRandom.FRandRange(35.f,95.f),JetSeed=SplashRandom.FRand();
            float Distance=0.f; FVector Previous=Start;
            TArray<bool,TInlineAllocator<(Rows+1)*(Columns+1)>> Covered;
            for(int32 Row=0;Row<=Rows;++Row)
            {
                const float U=static_cast<float>(Row)/Rows;
                const FVector Center=At(EndTime*U);
                Distance+=FVector::Dist(Center,Previous); Previous=Center;
                const float LocalWidth=Width*(.4f+.6f*FMath::Sin(U*UE_PI));
                for(int32 Column=0;Column<=Columns;++Column)
                {
                    const float X=static_cast<float>(Column)/Columns;
                    const FVector P=Center+WidthDirection*((X*2.f-1.f)*LocalWidth);
                    float H=0.f; Covered.Add(SampleReceivingWater(FVector2D(P),H) && P.Z>=H-.1f);
                    V.Add(P); UV.Add(FVector2D(X,Distance*.01f));
                    C.Add(FLinearColor(LocalWidth/10000.f,JetSeed,U,1.f).ToFColor(false));
                }
            }
            for(int32 Row=0;Row<Rows;++Row) for(int32 Column=0;Column<Columns;++Column)
            {
                const int32 A=Row*(Columns+1)+Column,B=A+Columns+1;
                if(Covered[A] && Covered[B] && Covered[A+1] && Covered[B+1])
                { I.Append({First+A,First+B,First+A+1,First+A+1,First+B,First+B+1}); }
            }
        }
    }
    if(!I.IsEmpty())
    {
        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V,I,UV,N,Tangents);
        Whitewater->CreateMeshSection(2,V,I,N,UV,C,Tangents,false);
        Whitewater->SetMaterial(2,WhitewaterMID);
        UE_LOG(LogTemp,Display,TEXT("DC_DETACHED_SPLASH source=%d gravity-driven receiving sheets triangles=%d"),DetachedFallSource,I.Num()/3);
    }
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
            float MeanHeight=0.f;
            if(!SampleReceivingWater(P,MeanHeight)) { continue; }
            ContactParticles.Add({FVector(P,MeanHeight+80.f),
                FVector(Velocity*.45f+FVector2D(FMath::FRandRange(-400.f,400.f),FMath::FRandRange(-400.f,400.f)),FMath::FRandRange(650.f,1550.f)),
                0.f,FMath::FRandRange(1.f,2.5f),FMath::FRandRange(3.f,16.f)});
        }
    }
}
