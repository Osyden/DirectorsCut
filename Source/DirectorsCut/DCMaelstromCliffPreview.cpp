#include "DCMaelstrom.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"

namespace
{
bool ReadPreviewJson(const FString& Path,TSharedPtr<FJsonObject>& Object)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text,*Path)
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object) && Object.IsValid();
}
bool PreviewVector(const TArray<TSharedPtr<FJsonValue>>& Values,FVector& Out)
{
    if(Values.Num()!=3) { return false; }
    double X,Y,Z;
    if(!Values[0]->TryGetNumber(X)||!Values[1]->TryGetNumber(Y)||!Values[2]->TryGetNumber(Z)) { return false; }
    Out=FVector(X,Y,Z);
    return !Out.ContainsNaN();
}
bool CompleteFallsReady(const FString& Directory,const TSharedPtr<FJsonObject>& Import,const FString& FBXHash)
{
    TSharedPtr<FJsonObject> Data;
    const TSharedPtr<FJsonObject>* Core=nullptr;
    FString Schema,FallFBX,FallSource,ActiveSource,Asset;
    if(!ReadPreviewJson(Directory/TEXT("CliffPreviewWaterfall.json"),Data)
        ||!Data->TryGetStringField(TEXT("schema"),Schema)||Schema!=TEXT("directorscut.cliff_preview_waterfall.v1")
        ||!Data->TryGetStringField(TEXT("cliff_fbx_sha256"),FallFBX)||FallFBX!=FBXHash||FallFBX.Len()!=64
        ||!Data->TryGetStringField(TEXT("source_candidate_sha256"),FallSource)||FallSource.Len()!=64
        ||!Import->TryGetStringField(TEXT("source_candidate_sha256"),ActiveSource)||FallSource!=ActiveSource
        ||!Data->TryGetObjectField(TEXT("core"),Core)
        ||!(*Core)->TryGetStringField(TEXT("asset"),Asset)
        ||!Asset.StartsWith(TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/"))) { return false; }
    const auto Vector=[&](const TCHAR* Key,FVector& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        return (*Core)->TryGetArrayField(Key,Values)&&PreviewVector(*Values,Out);
    };
    FVector Origin,U,V,Z;
    if(!Vector(TEXT("origin_arena_m"),Origin)||!Vector(TEXT("basis_u_arena"),U)
        ||!Vector(TEXT("basis_v_arena"),V)||!Vector(TEXT("basis_z_arena"),Z)
        ||!FMath::IsNearlyEqual(U.SizeSquared(),1.,1.e-6)||!FMath::IsNearlyEqual(V.SizeSquared(),1.,1.e-6)
        ||!FMath::IsNearlyEqual(Z.SizeSquared(),1.,1.e-6)||FMath::Abs(U|V)>=1.e-6
        ||FMath::Abs(U|Z)>=1.e-6||FMath::Abs(V|Z)>=1.e-6
        ||FVector::DotProduct(FVector::CrossProduct(U,V),Z)<=.999999) { return false; }
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);
    if(!Mesh||(Mesh->GetStaticMaterials().Num()!=3&&Mesh->GetStaticMaterials().Num()!=4)) { return false; }
    for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot)
    { if(!Mesh->GetMaterial(Slot)) { return false; } }
    return true;
}
}

void ADCMaelstrom::BuildCliffPreview()
{
    const bool ExplicitPreview=FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreview"));
    const FString Installed=FPaths::ProjectContentDir()/TEXT("DC/Maelstrom/GeneratedCliffPreview/Runtime");
    TSharedPtr<FJsonObject> Complete;
    FString CompleteSchema,CompleteHash;
    const bool InstalledComplete=!FParse::Param(FCommandLine::Get(),TEXT("DCLegacyCrater"))
        &&ReadPreviewJson(Installed/TEXT("CompleteArena.json"),Complete)
        &&Complete->TryGetStringField(TEXT("schema"),CompleteSchema)
        &&CompleteSchema==TEXT("directorscut.complete_crater.v1")
        &&Complete->TryGetStringField(TEXT("cliff_fbx_sha256"),CompleteHash);
    if(!ExplicitPreview&&!InstalledComplete) { return; }
    if((ExplicitPreview&&!FParse::Param(FCommandLine::Get(),TEXT("DCSceneReview")))
        || FParse::Param(FCommandLine::Get(),TEXT("DCFallCamera"))
        || FParse::Param(FCommandLine::Get(),TEXT("DCProceduralFalls")))
    {
        UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW requires DCSceneReview and excludes DCFallCamera/DCProceduralFalls"));
        return;
    }
    // Complete installed terrain is the playable scene, not a camera-only
    // replacement. Explicit preview continues to use the authoring data.
    TSharedPtr<FJsonObject> Audit,Import;
    const FString Saved=ExplicitPreview?FPaths::ProjectSavedDir():Installed;
    CliffArenaDataDirectory=Saved;
    if(!ReadPreviewJson(Saved/TEXT("CliffPreviewSourceAudit.json"),Audit)
        || !ReadPreviewJson(Saved/TEXT("CliffLandformPreviewImport.json"),Import))
    { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW missing import/source audit; preview disabled")); return; }
    FString Schema,SourceHash,ImportHash;
    bool GeometryPassed=false,MaskPassed=false;
    if(!Audit->TryGetStringField(TEXT("schema"),Schema)||Schema!=TEXT("directorscut.cliff_import_audit.v1")
        || !Audit->TryGetBoolField(TEXT("geometry_passed"),GeometryPassed)||!GeometryPassed
        || !Audit->TryGetBoolField(TEXT("mask_fidelity_passed"),MaskPassed)||!MaskPassed
        || !Audit->TryGetStringField(TEXT("source_fbx_sha256"),SourceHash)
        || !Import->TryGetStringField(TEXT("source_sha256"),ImportHash)||SourceHash!=ImportHash)
    { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW audit invalid or mismatched; preview disabled")); return; }
    if(!ExplicitPreview&&CompleteHash!=ImportHash) { return; }
    const TSharedPtr<FJsonObject>* Geometry=nullptr,*Frame=nullptr;
    const TArray<TSharedPtr<FJsonValue>>* OriginValues=nullptr,*Basis=nullptr;
    if(!Audit->TryGetObjectField(TEXT("geometry"),Geometry)
        || !(*Geometry)->TryGetObjectField(TEXT("arena_reconstruction"),Frame)
        || !(*Frame)->TryGetArrayField(TEXT("origin_cm"),OriginValues)
        || !(*Frame)->TryGetArrayField(TEXT("basis_for_imported_xyz"),Basis)||Basis->Num()!=3) { return; }
    FVector Origin,X,Y,Z;
    if(!PreviewVector(*OriginValues,Origin)
        || !PreviewVector((*Basis)[0]->AsArray(),X)||!PreviewVector((*Basis)[1]->AsArray(),Y)
        || !PreviewVector((*Basis)[2]->AsArray(),Z)) { return; }
    if(!FMath::IsNearlyEqual(X.SizeSquared(),1.,1.e-6)||!FMath::IsNearlyEqual(Y.SizeSquared(),1.,1.e-6)
        || !FMath::IsNearlyEqual(Z.SizeSquared(),1.,1.e-6)||FMath::Abs(X|Y)>1.e-6
        ||FMath::Abs(X|Z)>1.e-6||FMath::Abs(Y|Z)>1.e-6) { return; }
    FString MeshPath,AuditedPath;
    if(!Import->TryGetStringField(TEXT("asset"),MeshPath)||!Audit->TryGetStringField(TEXT("asset"),AuditedPath)
        ||!MeshPath.StartsWith(TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/"))) { return; }
    FString PackagePath=MeshPath; int32 Dot;
    if(PackagePath.FindChar(TEXT('.'),Dot)) { PackagePath.LeftInline(Dot); }
    if(AuditedPath!=MeshPath&&AuditedPath!=PackagePath) { return; }
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath);
    const bool Rock=FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewRock"));
    const bool Scan=!ExplicitPreview||FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewScan"));
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,Scan
        ? TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffScanSurface.M_CliffScanSurface") : Rock
        ? TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffRock.M_CliffRock")
        : TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffClay.M_CliffClay"));
    if(!Mesh||!Material) { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW missing isolated assets")); return; }
    if(!ExplicitPreview&&!IsCalderaLake()&&!CompleteFallsReady(Saved,Import,ImportHash))
    {
        UE_LOG(LogTemp,Error,TEXT("DC_CLIFF_ARENA replacement waterfall/core materials unavailable; retaining legacy scene"));
        return;
    }
    bCompleteCliffArena=!ExplicitPreview;
    CliffPreviewMesh=NewObject<UStaticMeshComponent>(this,TEXT("IsolatedCliffPreview"));
    CliffPreviewMesh->SetupAttachment(Arena);
    CliffPreviewMesh->SetMobility(EComponentMobility::Movable);
    CliffPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CliffPreviewMesh->SetStaticMesh(Mesh);
    CliffPreviewMesh->SetMaterial(0,Material);
    // Imported FBX Y is reversed. The audited matrix includes this reflection;
    // FTransform decomposes it into a proper rotation and signed unit scale.
    FMatrix Placement=FMatrix::Identity;
    Placement.SetAxes(&X,&Y,&Z,&Origin);
    const FTransform Pose(Placement);
    for(int32 Axis=0;Axis<3;++Axis)
    {
        const FVector Unit=Axis==0?FVector::ForwardVector:Axis==1?FVector::RightVector:FVector::UpVector;
        const FVector Expected=Axis==0?X:Axis==1?Y:Z;
        if(!Pose.TransformVector(Unit).Equals(Expected,1.e-6))
        { CliffPreviewMesh->DestroyComponent(); CliffPreviewMesh=nullptr; return; }
    }
    CliffPreviewMesh->SetRelativeTransform(Pose);
    CliffPreviewMesh->SetVisibility(false);
    CliffPreviewMesh->RegisterComponent();
    if(bCompleteCliffArena||FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewSupports")))
    {
        TSharedPtr<FJsonObject> Supports;
        FString SupportSchema,SupportHash;
        const TArray<TSharedPtr<FJsonValue>>* Actors=nullptr;
        if(ReadPreviewJson(Saved/TEXT("CliffPreviewSupports.json"),Supports)
            &&Supports->TryGetStringField(TEXT("schema"),SupportSchema)&&SupportSchema==TEXT("directorscut.cliff_preview_supports.v1")
            &&Supports->TryGetStringField(TEXT("source_fbx_sha256"),SupportHash)&&SupportHash==SourceHash
            &&(bCompleteCliffArena||Supports->TryGetArrayField(TEXT("actors"),Actors)))
        {
            // Sample the actual imported triangles, including new overhangs.
            // Angular bins accelerate queries without assuming polar topology.
            TArray<FVector3f> Surface;
            TArray<FIntVector> SurfaceTriangles;
            constexpr int32 Sectors=1536;
            TArray<TArray<int32>> TriangleBins;
            TriangleBins.SetNum(Sectors);
            const auto SectorAt=[](const FVector3f& P)
            {
                double A=FMath::Atan2(static_cast<double>(P.Y),static_cast<double>(P.X));
                if(A<0.) { A+=UE_TWO_PI; }
                return FMath::Min(FMath::FloorToInt(A/UE_TWO_PI*Sectors),Sectors-1);
            };
            const TSharedPtr<FJsonObject>* SurfaceData=nullptr;
            if(Supports->TryGetObjectField(TEXT("surface_mesh"),SurfaceData))
            {
                FString Digest; int32 Count=0,FaceCount=0; TArray<uint8> Bytes;
                if((*SurfaceData)->TryGetNumberField(TEXT("vertex_count"),Count)&&Count>0&&Count<=8000000
                    &&(*SurfaceData)->TryGetNumberField(TEXT("triangle_count"),FaceCount)&&FaceCount>0&&FaceCount<=16000000
                    &&(*SurfaceData)->TryGetStringField(TEXT("md5"),Digest)
                    &&FFileHelper::LoadFileToArray(Bytes,*(Saved/TEXT("CliffPreviewSurface.bin")))
                    &&Bytes.Num()==int64(Count)*12+int64(FaceCount)*12&&FMD5::HashBytes(Bytes.GetData(),Bytes.Num())==Digest)
                {
                    static_assert(sizeof(FVector3f)==3*sizeof(float));
                    static_assert(sizeof(FIntVector)==3*sizeof(int32));
                    Surface.SetNumUninitialized(Count);
                    SurfaceTriangles.SetNumUninitialized(FaceCount);
                    FMemory::Memcpy(Surface.GetData(),Bytes.GetData(),Count*12);
                    FMemory::Memcpy(SurfaceTriangles.GetData(),Bytes.GetData()+Count*12,FaceCount*12);
                    for(const FVector3f& P:Surface) { if(P.ContainsNaN()) { Surface.Reset(); break; } }
                    for(int32 I=0;I<SurfaceTriangles.Num()&&!Surface.IsEmpty();++I)
                    {
                        const FIntVector T=SurfaceTriangles[I];
                        if(T.X<0||T.Y<0||T.Z<0||T.X>=Count||T.Y>=Count||T.Z>=Count)
                        { Surface.Reset(); break; }
                        int32 A=SectorAt(Surface[T.X]),B=SectorAt(Surface[T.Y]),C=SectorAt(Surface[T.Z]);
                        int32 Low=FMath::Min3(A,B,C),High=FMath::Max3(A,B,C);
                        if(High-Low>Sectors/2)
                        {
                            if(A<Sectors/2) { A+=Sectors; }
                            if(B<Sectors/2) { B+=Sectors; }
                            if(C<Sectors/2) { C+=Sectors; }
                            Low=FMath::Min3(A,B,C); High=FMath::Max3(A,B,C);
                        }
                        // The crater wall stays away from the arena origin.
                        if(High-Low>Sectors/2) { Surface.Reset(); break; }
                        for(int32 Bin=Low;Bin<=High;++Bin) { TriangleBins[Bin%Sectors].Add(I); }
                    }
                }
            }
            if(bCompleteCliffArena&&Surface.IsEmpty())
            {
                UE_LOG(LogTemp,Error,TEXT("DC_CLIFF_ARENA actual ruin support surface unavailable; retaining legacy scene"));
                CliffPreviewMesh->DestroyComponent(); CliffPreviewMesh=nullptr; bCompleteCliffArena=false;
                return;
            }
            const auto GroundAt=[&](const FVector2D& P)
            {
                double Ground=-DBL_MAX;
                if(Surface.IsEmpty()) { return Ground; }
                const int32 CentreSide=SectorAt(FVector3f(P.X,P.Y,0.f));
                const auto Triangle=[&](int32 IA,int32 IB,int32 IC)
                {
                    const FVector A3(Surface[IA]),B3(Surface[IB]),C3(Surface[IC]);
                    const FVector2D AB(B3-A3),AC(C3-A3),AP=P-FVector2D(A3);
                    const double D=AB.X*AC.Y-AB.Y*AC.X;
                    if(FMath::Abs(D)<.0001) { return; }
                    const double U=(AP.X*AC.Y-AP.Y*AC.X)/D,V=(AB.X*AP.Y-AB.Y*AP.X)/D;
                    if(U>=-.000001&&V>=-.000001&&U+V<=1.000001)
                    { Ground=FMath::Max(Ground,A3.Z+U*(B3.Z-A3.Z)+V*(C3.Z-A3.Z)); }
                };
                for(const int32 Index:TriangleBins[CentreSide])
                {
                    const FIntVector T=SurfaceTriangles[Index];
                    Triangle(T.X,T.Y,T.Z);
                }
                return Ground;
            };
            for(AActor* Landmark:Landmarks)
            {
                if(!Landmark) { continue; }
                const TSharedPtr<FJsonObject>* A=nullptr;
                const FString Id=Landmark->GetName(); double Bottom=0.;
                const TArray<TSharedPtr<FJsonValue>>* Centre=nullptr;
                if(!bCompleteCliffArena)
                {
                    for(const TSharedPtr<FJsonValue>& Value:*Actors)
                    {
                        const TSharedPtr<FJsonObject>* Candidate=nullptr; FString CandidateId;
                        if(Value->TryGetObject(Candidate)&&(*Candidate)->TryGetStringField(TEXT("id"),CandidateId)&&CandidateId==Id)
                        { A=Candidate; break; }
                    }
                    if(!A||!(*A)->TryGetNumberField(TEXT("footing_bottom_m"),Bottom)
                        ||!(*A)->TryGetArrayField(TEXT("centre_xy_m"),Centre)||Centre->Num()!=2) { continue; }
                }
                {
                    // Recreate the pre-placement local bounds used by the
                    // support exporter. A yawed asymmetric group has a different
                    // world-AABB centre even when its transform is unchanged.
                    const FBox Bounds=Landmark->CalculateComponentsBoundingBoxInLocalSpace(true);
                    const FTransform LandmarkPose=Landmark->GetActorTransform();
                    const FVector Mid=LandmarkPose.TransformPosition(Bounds.GetCenter());
                    // Build-time arena is unpitched. Check identity and actual
                    // placement, not merely a reusable runtime actor name.
                    const FVector LocalMid=GetArenaTransform().InverseTransformPosition(Mid);
                    const FVector LocalBottom=GetArenaTransform().InverseTransformPosition(
                        LandmarkPose.TransformPosition(FVector(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Min.Z)));
                    const FVector LocalTop=GetArenaTransform().InverseTransformPosition(
                        LandmarkPose.TransformPosition(FVector(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Max.Z)));
                    if(!Surface.IsEmpty())
                    {
                        if(!Landmark->GetRootComponent()||Landmark->GetRootComponent()->GetAttachParent()!=Arena) { continue; }
                        double Low=DBL_MAX,High=-DBL_MAX; bool Covered=true;
                        // Nine footprint samples match production placement's
                        // granularity. This is not an exhaustive volume test.
                        for(int32 FootX=-1;FootX<=1;++FootX) for(int32 FootY=-1;FootY<=1;++FootY)
                        {
                            const FVector Corner=Bounds.GetCenter()+FVector(FootX*Bounds.GetExtent().X,FootY*Bounds.GetExtent().Y,0.);
                            const FVector P=GetArenaTransform().InverseTransformPosition(LandmarkPose.TransformPosition(Corner));
                            const double FootZ=GroundAt(FVector2D(P));
                            Covered&=FootZ>-DBL_MAX;
                            Low=FMath::Min(Low,FootZ); High=FMath::Max(High,FootZ);
                        }
                        const double Offset=Low-100.-LocalBottom.Z;
                        if(!Covered||FMath::Abs(Offset)>10000.||High-Low>1500.) { continue; }
                        CliffPreviewLandmarkOrigins.Add(Landmark,Landmark->GetRootComponent()->GetRelativeLocation());
                        CliffPreviewLandmarkOffsets.Add(Landmark,FVector(0.,0.,Offset));
                        CliffPreviewSupportedLandmarks.Add(Landmark);
                        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW current footprint=%s vertical_move_m=%.3f ground_range_m=%.3f samples=9"),*Id,Offset*.01,(High-Low)*.01);
                        continue;
                    }
                    const double DX=LocalMid.X*.01-(*Centre)[0]->AsNumber(),DY=LocalMid.Y*.01-(*Centre)[1]->AsNumber();
                    if(FMath::Abs(DX)>.03||FMath::Abs(DY)>.03||FMath::Abs(LocalBottom.Z*.01-Bottom)>.03)
                    { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW support identity/placement mismatch %s delta_m=(%.5f,%.5f,%.5f)"),*Id,DX,DY,LocalBottom.Z*.01-Bottom); continue; }
                    const TArray<TSharedPtr<FJsonValue>>* OffsetValues=nullptr;
                    FVector Offset=FVector::ZeroVector;
                    if((*A)->TryGetArrayField(TEXT("offset_arena_m"),OffsetValues))
                    {
                        if(!PreviewVector(*OffsetValues,Offset)||Offset.Size()>100.) { continue; }
                        if(!Landmark->GetRootComponent()||Landmark->GetRootComponent()->GetAttachParent()!=Arena) { continue; }
                        if(!Offset.IsNearlyZero())
                        {
                            CliffPreviewLandmarkOrigins.Add(Landmark,Landmark->GetRootComponent()->GetRelativeLocation());
                            CliffPreviewLandmarkOffsets.Add(Landmark,Offset*100.);
                        }
                    }
                    CliffPreviewSupportedLandmarks.Add(Landmark);
                    UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW supported landmark=%s bottom_m=%.3f top_m=%.3f"),*Id,Bottom,LocalTop.Z*.01);
                }
            }
        }
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW verified landmark count=%d; partial/unverified footprints hidden"),CliffPreviewSupportedLandmarks.Num());
    }
    UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_ARENA ready material=%s source=%s complete=%d shoreline_preserved=1"),*Material->GetName(),*SourceHash,bCompleteCliffArena);
}

void ADCMaelstrom::UpdateCliffPreview()
{
    if(!CliffPreviewMesh) { return; }
    if(!bActive||Elapsed<(bCompleteCliffArena?-7.f:0.f)) { ClearCliffPreview(); return; }
    CliffPreviewMesh->SetVisibility(true);
    UpdateCliffPreviewFalls();
    UpdateCliffDetailPreview();
    if(CliffPreviewHidden.IsEmpty())
    {
        const auto Remember=[this](USceneComponent* Mesh)
        { if(Mesh) { CliffPreviewHidden.Add({Mesh,Mesh->IsVisible()}); } };
        Remember(Cliffs); Remember(RimLand); Remember(Whitewater); Remember(Falls);
        for(UInstancedStaticMeshComponent* Mesh:CliffRocks) { Remember(Mesh); }
        for(UInstancedStaticMeshComponent* Mesh:CliffShadowRocks) { Remember(Mesh); }
    }
    // Never present an overlapping old wall as a successfully integrated tile.
    // This flag owns a close inspection camera and hides legacy terrain/falls.
    for(UProceduralMeshComponent* Mesh:{Cliffs.Get(),RimLand.Get(),Whitewater.Get(),Falls.Get()})
    { Mesh->SetVisibility(false); }
    for(UInstancedStaticMeshComponent* Mesh:CliffRocks) { if(Mesh) { Mesh->SetVisibility(false); } }
    for(UInstancedStaticMeshComponent* Mesh:CliffShadowRocks) { if(Mesh) { Mesh->SetVisibility(false); } }
    for(AActor* Landmark:Landmarks)
    {
        if(!Landmark) { continue; }
        Landmark->SetActorHiddenInGame(!CliffPreviewSupportedLandmarks.Contains(Landmark));
        if(const FVector* Offset=CliffPreviewLandmarkOffsets.Find(Landmark))
        {
            if(const FVector* Origin=CliffPreviewLandmarkOrigins.Find(Landmark))
            { Landmark->SetActorRelativeLocation(*Origin+*Offset); }
        }
    }
    // A complete candidate ring needs to be judged from the playable ship,
    // including its changing rim silhouette during descent. The close tile
    // camera remains the default for individual authoring patches.
    if(bCompleteCliffArena||FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewPlayerView"))) { return; }
    APlayerController* PC=GetWorld()->GetFirstPlayerController();
    if(!CliffPreviewCamera.IsValid())
    {
        FActorSpawnParameters Params; Params.Owner=this;
        CliffPreviewCamera=GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),FTransform::Identity,Params);
        if(ACameraActor* Camera=CliffPreviewCamera.Get())
        {
            Camera->GetCameraComponent()->SetFieldOfView(60.f);
            Camera->GetCameraComponent()->bConstrainAspectRatio=false;
            if(PC) { CliffPreviewPreviousTarget=PC->GetViewTarget(); PC->SetViewTarget(Camera); }
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW camera active; imported authoring surface, gameplay contacts unchanged"));
        }
    }
    if(ACameraActor* Camera=CliffPreviewCamera.Get())
    {
        const FTransform Frame=CliffPreviewMesh->GetComponentTransform();
        FVector LocalTarget(0,1500,-7500);
        if(FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewFrameMesh")))
        {
            if(const UStaticMesh* Mesh=CliffPreviewMesh->GetStaticMesh())
            { LocalTarget=Mesh->GetBoundingBox().GetCenter(); }
        }
        FVector TargetMetres;
        if(FParse::Value(FCommandLine::Get(),TEXT("DCCliffPreviewTargetX="),TargetMetres.X)
            &&FParse::Value(FCommandLine::Get(),TEXT("DCCliffPreviewTargetY="),TargetMetres.Y)
            &&FParse::Value(FCommandLine::Get(),TEXT("DCCliffPreviewTargetZ="),TargetMetres.Z)
            &&!TargetMetres.ContainsNaN())
        { LocalTarget=TargetMetres*100.; }
        FVector LocalEye=LocalTarget+FVector(9000,47500,6000);
        float AzimuthDeg=0.f;
        if(FParse::Value(FCommandLine::Get(),TEXT("DCCliffPreviewAzimuth="),AzimuthDeg)&&FMath::IsFinite(AzimuthDeg))
        {
            const float A=FMath::DegreesToRadians(AzimuthDeg);
            LocalEye=LocalTarget+FVector(FMath::Cos(A)*48000.,FMath::Sin(A)*48000.,6000.);
        }
        // Opt-in metric distance check, using the same target, angle and FOV.
        // The default authoring camera remains unchanged for matched reviews.
        float ReviewDistanceM=0.f;
        if(FParse::Value(FCommandLine::Get(),TEXT("DCCliffPreviewDistance="),ReviewDistanceM)
            &&FMath::IsFinite(ReviewDistanceM)&&ReviewDistanceM>=50.f&&ReviewDistanceM<=4000.f)
        {
            LocalEye=LocalTarget+(LocalEye-LocalTarget).GetSafeNormal()*(ReviewDistanceM*100.f);
        }
        const FVector Eye=Frame.TransformPosition(LocalEye);
        const FVector Target=Frame.TransformPosition(LocalTarget);
        Camera->SetActorLocationAndRotation(Eye,(Target-Eye).Rotation());
    }
}

void ADCMaelstrom::ClearCliffPreview()
{
    if(CliffPreviewMesh) { CliffPreviewMesh->SetVisibility(false); }
    ClearCliffPreviewFalls();
    ClearCliffDetailPreview();
    for(const auto& Entry:CliffPreviewLandmarkOrigins)
    { if(AActor* Landmark=Entry.Key.Get()) { Landmark->SetActorRelativeLocation(Entry.Value); } }
    for(const FCliffPreviewHidden& Saved:CliffPreviewHidden)
    {
        if(USceneComponent* Mesh=Saved.Component.Get())
        {
            const bool StageVisible=bActive&&((Mesh!=RimLand&&Mesh!=Falls)||Elapsed>=-7.f);
            Mesh->SetVisibility(Saved.bVisible&&StageVisible);
        }
    }
    if(!CliffPreviewHidden.IsEmpty())
    {
        for(AActor* Landmark:Landmarks)
        { if(Landmark) { Landmark->SetActorHiddenInGame(!bActive||Elapsed<-7.f); } }
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW terrain restored active=%d elapsed=%.2f cliff_visible=%d"),bActive,Elapsed,Cliffs->IsVisible());
    }
    CliffPreviewHidden.Reset();
    if(ACameraActor* Camera=CliffPreviewCamera.Get())
    {
        if(APlayerController* PC=GetWorld()->GetFirstPlayerController())
        {
            if(PC->GetViewTarget()==Camera)
            {
                AActor* Previous=CliffPreviewPreviousTarget.Get();
                PC->SetViewTarget(Previous?Previous:PC->GetPawn());
            }
        }
        Camera->Destroy();
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW camera restored"));
    }
    CliffPreviewCamera.Reset(); CliffPreviewPreviousTarget.Reset();
}
