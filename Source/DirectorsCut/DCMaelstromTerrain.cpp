#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

FVector ADCMaelstrom::GetShorelinePoint(float Angle) const
{
    const FVector2D P=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*WaterEdgeRadius(Angle);
    return FVector(P,HeightAt(P));
}

float ADCMaelstrom::WaterfallAngle(int32 Index) const
{
    // Nine drainage notches with unequal spacing: three groups separated by
    // hundreds of metres of exposed rock. Periodic falls make a vast crater
    // read as a garden fountain, whatever its world-space dimensions.
    static const float Angles[]={-2.77f,-2.63f,-1.81f,-.93f,-.82f,.12f,1.09f,1.24f,2.35f};
    return Index<UE_ARRAY_COUNT(Angles) ? Angles[Index] : 0.f;
}

float ADCMaelstrom::WaterfallWidth(int32 Index) const
{
    // Half-width in cm; distant falls must be fine features against a 3.2km rim.
    static const float Widths[]={1700.f,600.f,1100.f,2200.f,700.f,950.f,1600.f,550.f,1250.f};
    return Index<UE_ARRAY_COUNT(Widths) ? Widths[Index] : 0.f;
}

float ADCMaelstrom::WaterEdgeRadius(float Angle) const
{
    const FVector2D Direction(FMath::Cos(Angle),FMath::Sin(Angle));
    const float Lower=Radius*(.66f+.035f*FMath::PerlinNoise2D(Direction*5.3f+FVector2D(7,13)));
    const float T=DCMaelstromRoute::SecondsAtAngle(Angle);
    const float Channel=Radius*DCMaelstromRoute::RadialFraction(T)+16000.f*DCMaelstromRoute::WidthScale(T);
    // Keep the incoming river and full steering envelope. The rest of the
    // shoreline lies hundreds of metres inside the rock rim.
    const float Inlet=1.f-FMath::SmoothStep(20.f,45.f,T);
    return FMath::Min(GetRimRadius(Angle)*(1.f+Inlet*.08f),FMath::Max(Lower,Channel));
}

float ADCMaelstrom::CliffHeightAt(const FVector2D& P) const
{
    const float R=P.Size(),A=FMath::Atan2(P.Y,P.X);
    const float Toe=WaterEdgeRadius(A)*.995f;
    const float Rim=FMath::Max(GetRimRadius(A)*1.055f,Toe+13000.f);
    const float U=FMath::Clamp((R-Toe)/(Rim-Toe),0.f,1.f);
    const FVector2D D=P/FMath::Max(R,1.f);
    const float WaterZ=HeightAt(D*Toe,false)-650.f;
    const float Ridge=RimGroundHeight(Rim,A);
    // Broken geological benches rather than full-height inflated flutes.
    // Each formation has a different shoulder and width, with an unbroken
    // low talus connecting the face to its submerged toe.
    const float Fault=FMath::PerlinNoise2D(D*5.f+FVector2D(2,13));
    const float LocalFault=FMath::PerlinNoise2D(D*13.f+FVector2D(U*3.f,7));
    const float Shift=Fault*.065f+LocalFault*.025f;
    const float Profile=.28f*U
        +.21f*FMath::SmoothStep(.12f+Shift,.29f+Shift,U)
        +.26f*FMath::SmoothStep(.34f-Shift,.54f-Shift,U)
        +.25f*FMath::SmoothStep(.63f+Shift,.87f+Shift,U);
    const float Band=FMath::SmoothStep(0.f,.09f,U)*(1.f-FMath::SmoothStep(.84f,1.f,U));
    const FVector2D Warp=P+D*(FMath::PerlinNoise2D(P/26000.f)*2400.f);
    const float Fracture=2400.f*FMath::PerlinNoise2D(Warp/14500.f+FVector2D(3,17))
        +1500.f*FMath::PerlinNoise2D(Warp/5200.f+FVector2D(13,27))
        +450.f*FMath::PerlinNoise2D(Warp/1700.f+FVector2D(5,9));
    const float BenchPosition=.35f+.22f*FMath::PerlinNoise2D(D*8.f+FVector2D(11,3));
    const float Bench=1500.f*FMath::SmoothStep(BenchPosition-.06f,BenchPosition,U)
        *(1.f-FMath::SmoothStep(BenchPosition+.05f,BenchPosition+.14f,U));
    float Cliff=FMath::Lerp(WaterZ,Ridge,Profile)+(Fracture+Bench)*Band;
    // Carve only actual drainage channels; inactive entries do not cut gullies.
    // Locations are irregular now, so a nearest periodic-cell lookup is invalid.
    for(int32 Index=0;Index<WaterfallCount;++Index)
    {
        const float Width=WaterfallWidth(Index);
        if(Width<=0.f) { continue; }
        const float Along=FMath::Abs(FMath::UnwindRadians(A-WaterfallAngle(Index)))*R;
        const float Gully=1.f-FMath::SmoothStep(Width*1.5f,Width*3.5f,Along);
        const float FallBed=FMath::Lerp(WaterZ+650.f,Ridge,2.f*U-U*U)-3500.f;
        Cliff=FMath::Lerp(Cliff,FMath::Min(Cliff,FallBed),Gully*FMath::SmoothStep(0.f,.06f,U));
    }
    if(R<=Rim) { return Cliff; }
    const float Outer=FMath::Clamp((R-Rim)/(Radius*.24f),0.f,1.f);
    const float Spines=FMath::Abs(FMath::PerlinNoise2D(D*9.f+FVector2D(Outer*.6f,21.f)));
    return RimGroundHeight(R,A)+FMath::SmoothStep(.20f,.60f,Outer)
        *(4500.f+18500.f*Spines+1200.f*FMath::PerlinNoise2D(P/9000.f));
}

void ADCMaelstrom::BuildTerrain()
{
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<FColor> C;
    TArray<int32> I; TArray<FProcMeshTangent> Tangents;
    constexpr int32 Sectors=1024,Rows=112;
    for(int32 Row=0;Row<=Rows;++Row) for(int32 Side=0;Side<=Sectors;++Side)
    {
        const float A=Side*UE_TWO_PI/Sectors;
        const float R=FMath::Lerp(WaterEdgeRadius(A)*.995f,GetRimRadius(A)*1.32f,static_cast<float>(Row)/Rows);
        const FVector2D P(FMath::Cos(A)*R,FMath::Sin(A)*R);
        const float Z=CliffHeightAt(P);
        const float DX=(CliffHeightAt(P+FVector2D(100,0))-CliffHeightAt(P-FVector2D(100,0)))/200.f;
        const float DY=(CliffHeightAt(P+FVector2D(0,100))-CliffHeightAt(P-FVector2D(0,100)))/200.f;
        V.Add(FVector(P,Z)); N.Add(FVector(-DX,-DY,1).GetSafeNormal());
        Tangents.Add(FProcMeshTangent(FVector(1,0,DX).GetSafeNormal(),false));
        UV.Add(P/10000.f);
        const float ToeZ=HeightAt(FVector2D(FMath::Cos(A),FMath::Sin(A))*WaterEdgeRadius(A),false);
        C.Add(FLinearColor(1.f-FMath::SmoothStep(300.f,6000.f,Z-ToeZ),0,0,1).ToFColor(false));
        if(Row<Rows && Side<Sectors)
        { const int32 B=Row*(Sectors+1)+Side,D=B+Sectors+1; I.Append({B,B+1,D,B+1,D+1,D}); }
    }
    Cliffs->CreateMeshSection(0,V,I,N,UV,C,Tangents,false);
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_CraterRock.M_CraterRock")))
    { Cliffs->SetMaterial(0,M); }
    // Authored rock normals and silhouettes add metre-scale fracture detail
    // to the connected original terrain. Reference installed assets in place.
    const TCHAR* Meshes[]={TEXT("/Game/AncientRuins/Meshes/Cliffs/SM_Cliff_01.SM_Cliff_01"),
        TEXT("/Game/AncientRuins/Meshes/Cliffs/SM_Cliff_02.SM_Cliff_02"),
        TEXT("/Game/AncientRuins/Meshes/Rocks/SM_Rock_01.SM_Rock_01")};
    FRandomStream Random(89714);
    for(const TCHAR* Path:Meshes)
    {
        UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,Path);
        if(!Mesh) { continue; }
        UInstancedStaticMeshComponent* Rocks=NewObject<UInstancedStaticMeshComponent>(this);
        Rocks->SetMobility(EComponentMobility::Movable);
        Rocks->SetupAttachment(Arena); Rocks->SetStaticMesh(Mesh);
        // Preserve authored rock materials; substrate vertex wetness and local
        // texture coordinates are not valid for scaled static-mesh instances.
        Rocks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rocks->SetCastShadow(false); Rocks->RegisterComponent(); CliffRocks.Add(Rocks);
        UInstancedStaticMeshComponent* Shadows=NewObject<UInstancedStaticMeshComponent>(this);
        Shadows->SetMobility(EComponentMobility::Movable); Shadows->SetupAttachment(Arena);
        Shadows->SetStaticMesh(Mesh); Shadows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shadows->SetRenderInMainPass(false); Shadows->SetRenderInDepthPass(false);
        Shadows->SetCastShadow(true); Shadows->RegisterComponent(); CliffShadowRocks.Add(Shadows);
        const FBox Box=Mesh->GetBoundingBox();
        const float Extent=FMath::Max3(Box.GetSize().X,Box.GetSize().Y,Box.GetSize().Z);
        for(int32 Rock=0;Rock<96;++Rock)
        {
            // Small, deeply embedded clusters follow a common fracture plane.
            // No isolated 160m boulders perched on otherwise featureless slopes.
            const int32 Cluster=Rock/6;
            const float ClusterA=-UE_PI+UE_TWO_PI*FMath::Frac(Cluster*.61803399f+.17f);
            const float A=ClusterA+Random.FRandRange(-.012f,.012f);
            const float Edge=WaterEdgeRadius(A),Rim=GetRimRadius(A);
            if(Edge>Rim*.94f) { continue; } // keep the incoming river clear
            const float ClusterU=.18f+.66f*FMath::Frac(Cluster*.381966f+.23f);
            const float R=FMath::Lerp(Edge+3500.f,Rim*1.03f,FMath::Clamp(ClusterU+Random.FRandRange(-.08f,.08f),.08f,.95f));
            const FVector2D P(FMath::Cos(A)*R,FMath::Sin(A)*R);
            const float Size=Random.FRandRange(800.f,4500.f),Scale=Size/FMath::Max(Extent,1.f);
            bool InWaterfall=false;
            for(int32 Fall=0;Fall<WaterfallCount;++Fall)
            {
                if(WaterfallWidth(Fall)>0.f && FMath::Abs(FMath::UnwindRadians(A-WaterfallAngle(Fall)))*R<WaterfallWidth(Fall)*2.f+Size*.65f)
                { InWaterfall=true; break; }
            }
            if(InWaterfall) { continue; }
            const float RouteT=DCMaelstromRoute::SecondsAtAngle(A);
            const float ProtectedRadius=Radius*DCMaelstromRoute::RadialFraction(RouteT)
                +13000.f*DCMaelstromRoute::WidthScale(RouteT);
            if(RouteT<180.f && R-Size*.9f<ProtectedRadius) { continue; }
            const FRotator Rotation(Random.FRandRange(-12.f,12.f),FMath::RadiansToDegrees(ClusterA)+Random.FRandRange(-12.f,12.f),Random.FRandRange(-10.f,10.f));
            const FVector Foot(Box.GetCenter().X,Box.GetCenter().Y,Box.Min.Z+Box.GetSize().Z*.68f);
            const FVector Location=FVector(P,CliffHeightAt(P))-Rotation.RotateVector(Foot*Scale);
            Rocks->AddInstance(FTransform(Rotation,Location,FVector(Scale)));
        }
    }
    // The connected terrain replaces the old rim strip and the water-coated
    // wall. Discrete cliff falls are built by BuildWhitewater instead.
    RimLand->ClearAllMeshSections();
    Falls->ClearAllMeshSections();
    float MinimumClearance=MAX_flt;
    for(int32 T=0;T<=178;++T)
    {
        const float Required=Radius*DCMaelstromRoute::RadialFraction(T)
            +11000.f*DCMaelstromRoute::WidthScale(T)+1500.f;
        MinimumClearance=FMath::Min(MinimumClearance,WaterEdgeRadius(DCMaelstromRoute::Angle(T))-Required);
    }
    ensureMsgf(MinimumClearance>0.f,TEXT("Rock shoreline enters the ship steering envelope"));
    UE_LOG(LogTemp,Display,TEXT("DC_TERRAIN minimum full-steer shoreline clearance %.1fm"),MinimumClearance*.01f);
    UE_LOG(LogTemp,Display,TEXT("DC_TERRAIN irregular inner shoreline, connected rock walls: %d vertices"),V.Num());
}

void ADCMaelstrom::UpdateTerrainShadows()
{
    if(Elapsed<NextTerrainShadowUpdate && NextTerrainShadowUpdate-Elapsed<2.f) { return; }
    NextTerrainShadowUpdate=Elapsed+1.f;
    // Budget shadow-only instances close to the ship. Baseline reviews already
    // hit the VSM marking limit on startup; avoid adding distant shadow work.
    struct FCandidate { int32 Group,Index; float Distance; };
    TArray<FCandidate> Candidates;
    for(int32 Group=0;Group<CliffRocks.Num();++Group)
    {
        CliffShadowRocks[Group]->ClearInstances();
        for(int32 Index=0;Index<CliffRocks[Group]->GetInstanceCount();++Index)
        {
            FTransform World; CliffRocks[Group]->GetInstanceTransform(Index,World,true);
            const float Distance=World.GetLocation().Size();
            if(Distance<60000.f) { Candidates.Add({Group,Index,Distance}); }
        }
    }
    Candidates.Sort([](const FCandidate& A,const FCandidate& B){ return A.Distance<B.Distance; });
    for(int32 C=0;C<FMath::Min(12,Candidates.Num());++C)
    {
        const FCandidate& Choice=Candidates[C]; FTransform Local;
        CliffRocks[Choice.Group]->GetInstanceTransform(Choice.Index,Local,false);
        CliffShadowRocks[Choice.Group]->AddInstance(Local);
    }
}
