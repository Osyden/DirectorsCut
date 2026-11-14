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
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector.SM_CliffSector"));
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
    for(AActor* Landmark:Landmarks) { if(Landmark) { Landmark->SetActorHiddenInGame(true); } }
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
