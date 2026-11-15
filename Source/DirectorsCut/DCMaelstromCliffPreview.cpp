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
}

void ADCMaelstrom::BuildCliffPreview()
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreview"))) { return; }
    if(!FParse::Param(FCommandLine::Get(),TEXT("DCSceneReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("DCFallCamera"))
        || FParse::Param(FCommandLine::Get(),TEXT("DCProceduralFalls")))
    {
        UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW requires DCSceneReview and excludes DCFallCamera/DCProceduralFalls"));
        return;
    }
    // This close authoring view deliberately isolates one tile. The normal
    // cliff/contact/temple system remains unchanged until all share a mesh.
    TSharedPtr<FJsonObject> Audit,Import;
    const FString Saved=FPaths::ProjectSavedDir();
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
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,Rock
        ? TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffRock.M_CliffRock")
        : TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffClay.M_CliffClay"));
    if(!Mesh||!Material) { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW missing isolated assets")); return; }
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
    if(FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewSupports")))
    {
        TSharedPtr<FJsonObject> Supports;
        FString SupportSchema,SupportHash;
        const TArray<TSharedPtr<FJsonValue>>* Actors=nullptr;
        if(ReadPreviewJson(Saved/TEXT("CliffPreviewSupports.json"),Supports)
            &&Supports->TryGetStringField(TEXT("schema"),SupportSchema)&&SupportSchema==TEXT("directorscut.cliff_preview_supports.v1")
            &&Supports->TryGetStringField(TEXT("source_fbx_sha256"),SupportHash)&&SupportHash==SourceHash
            &&Supports->TryGetArrayField(TEXT("actors"),Actors))
        {
            for(const TSharedPtr<FJsonValue>& Value:*Actors)
            {
                const TSharedPtr<FJsonObject>* A=nullptr;
                FString Id; double Bottom;
                const TArray<TSharedPtr<FJsonValue>>* Centre=nullptr;
                if(!Value->TryGetObject(A)||!(*A)->TryGetStringField(TEXT("id"),Id)
                    ||!(*A)->TryGetNumberField(TEXT("footing_bottom_m"),Bottom)
                    ||!(*A)->TryGetArrayField(TEXT("centre_xy_m"),Centre)||Centre->Num()!=2) { continue; }
                for(AActor* Landmark:Landmarks)
                {
                    if(!Landmark||Landmark->GetName()!=Id) { continue; }
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
                    const double DX=LocalMid.X*.01-(*Centre)[0]->AsNumber(),DY=LocalMid.Y*.01-(*Centre)[1]->AsNumber();
                    if(FMath::Abs(DX)>.03||FMath::Abs(DY)>.03||FMath::Abs(LocalBottom.Z*.01-Bottom)>.03)
                    { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW support identity/placement mismatch %s"),*Id); continue; }
                    CliffPreviewSupportedLandmarks.Add(Landmark);
                    UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW supported landmark=%s bottom_m=%.3f top_m=%.3f"),*Id,Bottom,LocalTop.Z*.01);
                }
            }
        }
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW verified landmark count=%d; partial/unverified footprints hidden"),CliffPreviewSupportedLandmarks.Num());
    }
    UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW ready material=%s source=%s isolated=1 runtime_contacts_unchanged=1"),*Material->GetName(),*SourceHash);
}

void ADCMaelstrom::UpdateCliffPreview()
{
    if(!CliffPreviewMesh) { return; }
    if(!bActive||Elapsed<0.f) { ClearCliffPreview(); return; }
    CliffPreviewMesh->SetVisibility(true);
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
    { if(Landmark) { Landmark->SetActorHiddenInGame(!CliffPreviewSupportedLandmarks.Contains(Landmark)); } }
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
            UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW camera active; 500m authoring tile, no waterfall/temple integration"));
        }
    }
    if(ACameraActor* Camera=CliffPreviewCamera.Get())
    {
        const FTransform Frame=CliffPreviewMesh->GetComponentTransform();
        const FVector Eye=Frame.TransformPosition(FVector(9000,49000,-1500));
        const FVector Target=Frame.TransformPosition(FVector(0,1500,-7500));
        Camera->SetActorLocationAndRotation(Eye,(Target-Eye).Rotation());
    }
}

void ADCMaelstrom::ClearCliffPreview()
{
    if(CliffPreviewMesh) { CliffPreviewMesh->SetVisibility(false); }
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
