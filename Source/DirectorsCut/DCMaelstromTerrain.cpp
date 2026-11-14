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
    return -UE_PI+(Index+.5f)*UE_TWO_PI/WaterfallCount+.025f*FMath::Sin(Index*2.73f);
}

float ADCMaelstrom::WaterfallWidth(int32 Index) const
{
    return Index%5==0 ? 5000.f : 1400.f+2400.f*(.5f+.5f*FMath::Sin(Index*73.1f));
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
    // A steep, connected escarpment with buttresses, incised gullies and
    // fractured ledges. It is land, not a displaced water sheet.
    const float Buttress=FMath::PerlinNoise2D(P/12500.f+FVector2D(11,3));
    const float Profile=FMath::Pow(U,FMath::Clamp(.70f+Buttress*.22f,.48f,.92f));
    const float Band=FMath::SmoothStep(0.f,.045f,U)*(1.f-FMath::SmoothStep(.80f,1.f,U));
    const FVector2D Warp=P+FVector2D(
        FMath::PerlinNoise2D(P/17000.f),FMath::PerlinNoise2D(P/19000.f+FVector2D(31,7)))*4500.f;
    const float Fracture=7500.f*FMath::PerlinNoise2D(Warp/12500.f)
        +1200.f*FMath::PerlinNoise2D(Warp/3700.f+FVector2D(13,27))
        +250.f*FMath::PerlinNoise2D(Warp/1200.f+FVector2D(5,9));
    float Cliff=FMath::Lerp(WaterZ,Ridge,Profile)+Fracture*Band;
    // Shared waterfall notches: carve receiving gullies below the airborne
    // water, instead of laying white material over every rock protrusion.
    const int32 Near=FMath::FloorToInt((A+UE_PI)*WaterfallCount/UE_TWO_PI);
    for(int32 J=-1;J<=1;++J)
    {
        const int32 Index=(Near+J+WaterfallCount)%WaterfallCount;
        const float Along=FMath::Abs(FMath::UnwindRadians(A-WaterfallAngle(Index)))*R;
        const float Gully=1.f-FMath::SmoothStep(WaterfallWidth(Index)*1.2f,WaterfallWidth(Index)*2.4f,Along);
        const float FallBed=FMath::Lerp(WaterZ+650.f,Ridge,2.f*U-U*U)-3500.f;
        Cliff=FMath::Lerp(Cliff,FMath::Min(Cliff,FallBed),Gully*FMath::SmoothStep(0.f,.06f,U));
    }
    if(R<=Rim) { return Cliff; }
    const float Outer=FMath::Clamp((R-Rim)/(Radius*.24f),0.f,1.f);
    const float Spines=1.f-FMath::Abs(FMath::PerlinNoise2D(P/21000.f+FVector2D(3,21)));
    return RimGroundHeight(R,A)+FMath::SmoothStep(.20f,.60f,Outer)
        *(3500.f+12500.f*Spines*Spines+2800.f*FMath::PerlinNoise2D(P/6500.f));
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
        Rocks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rocks->SetCastShadow(false); Rocks->RegisterComponent(); CliffRocks.Add(Rocks);
        UInstancedStaticMeshComponent* Shadows=NewObject<UInstancedStaticMeshComponent>(this);
        Shadows->SetMobility(EComponentMobility::Movable); Shadows->SetupAttachment(Arena);
        Shadows->SetStaticMesh(Mesh); Shadows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shadows->SetRenderInMainPass(false); Shadows->SetRenderInDepthPass(false);
        Shadows->SetCastShadow(true); Shadows->RegisterComponent(); CliffShadowRocks.Add(Shadows);
        const FBox Box=Mesh->GetBoundingBox();
        const float Extent=FMath::Max3(Box.GetSize().X,Box.GetSize().Y,Box.GetSize().Z);
        for(int32 Rock=0;Rock<45;++Rock)
        {
            const float A=Random.FRandRange(-UE_PI,UE_PI);
            const float Edge=WaterEdgeRadius(A),Rim=GetRimRadius(A);
            if(Edge>Rim*.94f) { continue; } // keep the incoming river clear
            const float R=FMath::Lerp(Edge+3500.f,Rim*1.03f,FMath::Pow(Random.FRand(),1.5f));
            const FVector2D P(FMath::Cos(A)*R,FMath::Sin(A)*R);
            const float Size=Random.FRandRange(5000.f,16000.f),Scale=Size/FMath::Max(Extent,1.f);
            bool InWaterfall=false;
            for(int32 Fall=0;Fall<WaterfallCount;++Fall)
            {
                if(FMath::Abs(FMath::UnwindRadians(A-WaterfallAngle(Fall)))*R<WaterfallWidth(Fall)*1.6f+Size*.5f)
                { InWaterfall=true; break; }
            }
            if(InWaterfall) { continue; }
            const float RouteT=DCMaelstromRoute::SecondsAtAngle(A);
            const float ProtectedRadius=Radius*DCMaelstromRoute::RadialFraction(RouteT)
                +13000.f*DCMaelstromRoute::WidthScale(RouteT);
            if(RouteT<180.f && R-Size*.9f<ProtectedRadius) { continue; }
            const FRotator Rotation(Random.FRandRange(-20.f,20.f),FMath::RadiansToDegrees(A)+Random.FRandRange(-70.f,70.f),Random.FRandRange(-25.f,25.f));
            const FVector Foot(Box.GetCenter().X,Box.GetCenter().Y,Box.Min.Z+Box.GetSize().Z*.35f);
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
