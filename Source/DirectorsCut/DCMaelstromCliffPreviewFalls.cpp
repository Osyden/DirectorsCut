#include "DCMaelstrom.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SparseVolumeTexture/SparseVolumeTexture.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadFallPreview(const FString& Path,TSharedPtr<FJsonObject>& Out)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text,*Path)
        &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out)&&Out.IsValid();
}
bool FallPreviewVector(const TSharedPtr<FJsonObject>& Object,const TCHAR* Name,FVector& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* A=nullptr;
    if(!Object->TryGetArrayField(Name,A)||A->Num()!=3) { return false; }
    double X,Y,Z;
    if(!(*A)[0]->TryGetNumber(X)||!(*A)[1]->TryGetNumber(Y)||!(*A)[2]->TryGetNumber(Z)) { return false; }
    Out=FVector(X,Y,Z); return !Out.ContainsNaN();
}
bool FallPreviewHash(const FString& Hash)
{
    if(Hash.Len()!=64) { return false; }
    for(TCHAR C:Hash) { if(!FChar::IsHexDigit(C)) { return false; } }
    return true;
}
}

void ADCMaelstrom::UpdateCliffPreviewFalls()
{
    if((!bCompleteCliffArena&&!FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewFall")))||!CliffPreviewMesh||!bActive||Elapsed<0.f)
    { ClearCliffPreviewFalls(); return; }
    if(!bCliffPreviewFallsTried)
    {
        bCliffPreviewFallsTried=true;
        const FString Saved=FPaths::ConvertRelativePathToFull(CliffArenaDataDirectory.IsEmpty()?FPaths::ProjectSavedDir():CliffArenaDataDirectory);
        TSharedPtr<FJsonObject> Data,Active,Sidecar;
        FString Schema,ExpectedFBX,ExpectedSource,ActiveFBX,SidecarPath,ActualSource;
        if(!ReadFallPreview(Saved/TEXT("CliffPreviewWaterfall.json"),Data)
            ||!ReadFallPreview(Saved/TEXT("CliffLandformPreviewImport.json"),Active)
            ||!Data->TryGetStringField(TEXT("schema"),Schema)||Schema!=TEXT("directorscut.cliff_preview_waterfall.v1")
            ||!Data->TryGetStringField(TEXT("cliff_fbx_sha256"),ExpectedFBX)||!FallPreviewHash(ExpectedFBX)
            ||!Data->TryGetStringField(TEXT("source_candidate_sha256"),ExpectedSource)||!FallPreviewHash(ExpectedSource)
            ||!Active->TryGetStringField(TEXT("source_sha256"),ActiveFBX)||ExpectedFBX!=ActiveFBX
            ||!(bCompleteCliffArena
                ? Active->TryGetStringField(TEXT("source_candidate_sha256"),ActualSource)
                : (Active->TryGetStringField(TEXT("export_sidecar"),SidecarPath)&&ReadFallPreview(SidecarPath,Sidecar)
                    &&Sidecar->TryGetStringField(TEXT("input_sha256"),ActualSource)))
            ||ActualSource!=ExpectedSource)
        {
            UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW_FALL rejected missing/stale active FBX or source candidate contract")); return;
        }
        const TSharedPtr<FJsonObject>* Core=nullptr;
        if(Data->TryGetObjectField(TEXT("core"),Core))
        {
            FString Asset;
            FVector Origin,U,V,Z;
            if((*Core)->TryGetStringField(TEXT("asset"),Asset)
                &&Asset.StartsWith(TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/"))
                &&FallPreviewVector(*Core,TEXT("origin_arena_m"),Origin)
                &&FallPreviewVector(*Core,TEXT("basis_u_arena"),U)
                &&FallPreviewVector(*Core,TEXT("basis_v_arena"),V)
                &&FallPreviewVector(*Core,TEXT("basis_z_arena"),Z)
                &&FMath::IsNearlyEqual(U.SizeSquared(),1.,1.e-6)&&FMath::IsNearlyEqual(V.SizeSquared(),1.,1.e-6)
                &&FMath::IsNearlyEqual(Z.SizeSquared(),1.,1.e-6)&&FMath::Abs(U|V)<1.e-6
                &&FMath::Abs(U|Z)<1.e-6&&FMath::Abs(V|Z)<1.e-6&&FVector::DotProduct(FVector::CrossProduct(U,V),Z)>.999999)
            {
                if(UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset))
                {
                    const int32 MaterialCount=Mesh->GetStaticMaterials().Num();
                    if(MaterialCount==3||MaterialCount==4)
                    {
                        CliffPreviewFallCore=NewObject<UStaticMeshComponent>(this,TEXT("CliffPreviewFallCore"));
                        CliffPreviewFallCore->SetupAttachment(Arena);
                        CliffPreviewFallCore->SetMobility(EComponentMobility::Movable);
                        CliffPreviewFallCore->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                        CliffPreviewFallCore->SetStaticMesh(Mesh);
                        CliffPreviewFallCore->SetCastShadow(false);
                        // Core FBX reflects chart Y; native VDB below does not.
                        const FVector ImportedY=-V,OriginCm=Origin*100.;
                        FMatrix Placement=FMatrix::Identity; Placement.SetAxes(&U,&ImportedY,&Z,&OriginCm);
                        CliffPreviewFallCore->SetRelativeTransform(FTransform(Placement));
                        for(int32 I=0;I<MaterialCount;++I) { CliffPreviewFallCore->CreateAndSetMaterialInstanceDynamic(I); }
                        CliffPreviewFallCore->RegisterComponent();
                    }
                }
            }
        }
        if(!CliffPreviewFallCore)
        { UE_LOG(LogTemp,Warning,TEXT("DC_CLIFF_PREVIEW_FALL core rejected; volume-only capture is not the complete waterfall")); }
        const TArray<TSharedPtr<FJsonValue>>* Volumes=nullptr;
        if(Data->TryGetArrayField(TEXT("volumes"),Volumes)&&Volumes->Num()<=8)
        {
            for(const TSharedPtr<FJsonValue>& Item:*Volumes)
            {
                const TSharedPtr<FJsonObject>* Entry=nullptr;
                FString Asset,Material; FVector Origin;
                double Yaw=0.,FPS=12.,Density=.002; int32 Frames=48;
                if(!Item->TryGetObject(Entry)||!(*Entry)->TryGetStringField(TEXT("asset"),Asset)
                    ||!(*Entry)->TryGetStringField(TEXT("material"),Material)
                    ||!Asset.StartsWith(TEXT("/Game/DC/Maelstrom/ProceduralWaterfalls/Feed"))
                    ||!Material.StartsWith(TEXT("/Game/DC/Maelstrom/ProceduralWaterfalls/Feed"))
                    ||!FallPreviewVector(*Entry,TEXT("origin_arena_cm"),Origin)
                    ||!(*Entry)->TryGetNumberField(TEXT("yaw_degrees"),Yaw)||!FMath::IsFinite(Yaw)
                    ||!(*Entry)->TryGetNumberField(TEXT("frames"),Frames)||Frames!=48
                    ||!(*Entry)->TryGetNumberField(TEXT("fps"),FPS)||!FMath::IsNearlyEqual(FPS,12.,.0001)
                    ||!(*Entry)->TryGetNumberField(TEXT("density_per_cm"),Density)||!FMath::IsFinite(Density)||Density<0.||Density>.02) { continue; }
                USparseVolumeTexture* Volume=LoadObject<USparseVolumeTexture>(nullptr,*Asset);
                UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,*Material);
                if(!Volume||!Base||Volume->GetNumFrames()!=Frames) { continue; }
                const FVector Voxel=Volume->GetFrameTransform().GetScale3D();
                float MaterialVoxel=0.f;
                if(Voxel.X<=0.||!FMath::IsNearlyEqual(Voxel.X,Voxel.Y,.001)||!FMath::IsNearlyEqual(Voxel.X,Voxel.Z,.001)
                    ||!Base->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VoxelSizeCm")),MaterialVoxel)
                    ||!FMath::IsNearlyEqual(static_cast<float>(Voxel.X),MaterialVoxel,.001f))
                { UE_LOG(LogTemp,Error,TEXT("DC_CLIFF_PREVIEW_FALL native voxel/material unit mismatch %s"),*Asset); continue; }
                FActorSpawnParameters Params; Params.Owner=this;
                AHeterogeneousVolume* Actor=GetWorld()->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(),FTransform::Identity,Params);
                UHeterogeneousVolumeComponent* C=Actor?Actor->FindComponentByClass<UHeterogeneousVolumeComponent>():nullptr;
                if(!C) { if(Actor) { Actor->Destroy(); } continue; }
                Actor->AttachToComponent(Arena,FAttachmentTransformRules::KeepRelativeTransform);
                Actor->SetActorRelativeTransform(FTransform(FRotator(0.f,Yaw,0.f),Origin));
                UMaterialInstanceDynamic* MID=UMaterialInstanceDynamic::Create(Base,Actor);
                MID->SetSparseVolumeTextureParameterValue(TEXT("SparseVolumeTexture"),Volume);
                const bool bDensityDiagnostic=FParse::Param(FCommandLine::Get(),TEXT("DCCliffFallDiagnostic"));
                float AppliedDensity=static_cast<float>(Density),Emission=bDensityDiagnostic?1.f:0.f;
                FParse::Value(FCommandLine::Get(),Asset.Contains(TEXT("/Falling."))?TEXT("DCCliffFallDensity="):TEXT("DCCliffImpactDensity="),AppliedDensity);
                if(bDensityDiagnostic) { AppliedDensity=0.f; FParse::Value(FCommandLine::Get(),TEXT("DCCliffFallDiagnosticEmission="),Emission); }
                MID->SetScalarParameterValue(TEXT("Density"),FMath::Max(0.f,AppliedDensity));
                MID->SetScalarParameterValue(TEXT("DiagnosticEmission"),FMath::Max(0.f,Emission));
                MID->SetScalarParameterValue(TEXT("DiagnosticConstantDensity"),0.f);
                C->SetMaterial(0,MID); C->bPivotAtCentroid=false;
                // The opt-in blocking mode distinguishes sparse residency from
                // placement/lighting after direct screenshot seeks.
                C->bIssueBlockingRequests=FParse::Param(FCommandLine::Get(),TEXT("DCCliffFallBlocking"));
                C->SetFrameRate(static_cast<float>(FPS));
                C->ComponentTags.Add(TEXT("DCCliffFallAwaitBounds"));
                C->StepFactor=1.f; C->ShadowStepFactor=2.f;
                C->SetPlaying(false); C->SetLooping(true); C->SetStartFrame(0.f); C->SetEndFrame(Frames-1.f);
                CliffPreviewVolumes.Add({Actor,Frames,static_cast<float>(FPS)});
                UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW_FALL native asset=%s voxelCm=%.4f frames=%d fps=%.1f origin=%s yaw=%.3f"),*Asset,MaterialVoxel,Frames,FPS,*Origin.ToString(),Yaw);
            }
        }
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW_FALL ready core=%d volumes=%d activeFBX=%s source=%s"),CliffPreviewFallCore?1:0,CliffPreviewVolumes.Num(),*ExpectedFBX,*ExpectedSource);
    }
    if(CliffPreviewFallCore)
    {
        CliffPreviewFallCore->SetVisibility(!FParse::Param(FCommandLine::Get(),TEXT("DCCliffFallHideCore")));
        CliffPreviewFallCore->SetScalarParameterValueOnMaterials(TEXT("PreviewTime"),FMath::Max(0.f,Elapsed));
    }
    if(!CliffPreviewVolumes.IsEmpty())
    {
        if(IConsoleVariable* Range=IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance")))
        {
            const float Required=Radius*2.2f;
            if(Range->GetFloat()<Required)
            {
                if(CliffPreviewPreviousTraceDistance<0.f) { CliffPreviewPreviousTraceDistance=Range->GetFloat(); }
                CliffPreviewAppliedTraceDistance=Required; Range->Set(Required,ECVF_SetByCode);
            }
        }
    }
    for(FCliffPreviewVolume& Fall:CliffPreviewVolumes)
    {
        if(AHeterogeneousVolume* Actor=Fall.Actor.Get())
        {
            Actor->SetActorHiddenInGame(false);
            if(UHeterogeneousVolumeComponent* C=Actor->FindComponentByClass<UHeterogeneousVolumeComponent>())
            {
                C->SetVisibility(true);
                float Frame=FMath::Fmod(FMath::Max(0.f,Elapsed)*Fall.FPS,static_cast<float>(Fall.Frames));
                FParse::Value(FCommandLine::Get(),TEXT("DCCliffFallFixedFrame="),Frame);
                C->SetFrame(FMath::Clamp(Frame,0.f,Fall.Frames-1.f));
                // Wait for the component's native tick: spawn-time frame and
                // bounds are placeholders and cannot diagnose VDB placement.
                if(C->ComponentHasTag(TEXT("DCCliffFallAwaitBounds"))&&C->FrameTransform.GetScale3D().X>1.)
                {
                    C->ComponentTags.Remove(TEXT("DCCliffFallAwaitBounds"));
                    UMaterialInterface* Material=C->GetMaterial(0);
                    USparseVolumeTexture* BoundVolume=nullptr;
                    float Density=-1.f,Emission=-1.f,Voxel=-1.f;
                    if(Material)
                    {
                        Material->GetSparseVolumeTextureParameterValue(FMaterialParameterInfo(TEXT("SparseVolumeTexture")),BoundVolume);
                        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),Density);
                        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DiagnosticEmission")),Emission);
                        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VoxelSizeCm")),Voxel);
                    }
                    const FVector Foot=Actor->GetActorTransform().TransformPosition(FVector(0,1000,0));
                    const UGameViewportClient* Viewport=GetWorld()->GetGameViewport();
                    IConsoleVariable* Range=IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance"));
                    UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_FALL_BOUNDS actor=%s relative=%s component=%s frame=%s resolution=%s boundsOrigin=%s boundsExtent=%s expectedFootWorld=%s coreWorld=%s svt=%s density=%.6f emission=%.6f voxelCm=%.3f showHV=%d traceCm=%.1f blocking=%d"),
                        *Actor->GetName(),*Actor->GetRootComponent()->GetRelativeTransform().ToString(),*C->GetComponentTransform().ToString(),
                        *C->FrameTransform.ToString(),*C->VolumeResolution.ToString(),*C->Bounds.Origin.ToString(),*C->Bounds.BoxExtent.ToString(),
                        *Foot.ToString(),CliffPreviewFallCore?*CliffPreviewFallCore->GetComponentTransform().ToString():TEXT("none"),
                        *GetPathNameSafe(BoundVolume),Density,Emission,Voxel,Viewport?Viewport->EngineShowFlags.HeterogeneousVolumes:-1,Range?Range->GetFloat():-1.f,C->bIssueBlockingRequests);
                }
            }
        }
    }
}

void ADCMaelstrom::ClearCliffPreviewFalls()
{
    // Preserve loaded assets/actors for backward seeks; no repeated import or spawn.
    if(CliffPreviewFallCore) { CliffPreviewFallCore->SetVisibility(false); }
    for(FCliffPreviewVolume& Fall:CliffPreviewVolumes)
    {
        if(AHeterogeneousVolume* Actor=Fall.Actor.Get())
        {
            Actor->SetActorHiddenInGame(true);
            if(UHeterogeneousVolumeComponent* C=Actor->FindComponentByClass<UHeterogeneousVolumeComponent>()) { C->SetVisibility(false); }
        }
    }
    if(CliffPreviewPreviousTraceDistance>=0.f)
    {
        if(IConsoleVariable* Range=IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance")))
        {
            if(FMath::IsNearlyEqual(Range->GetFloat(),CliffPreviewAppliedTraceDistance))
            { Range->Set(CliffPreviewPreviousTraceDistance,ECVF_SetByCode); UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_PREVIEW_FALL restored traceCm=%.1f"),CliffPreviewPreviousTraceDistance); }
        }
        CliffPreviewPreviousTraceDistance=CliffPreviewAppliedTraceDistance=-1.f;
    }
}
