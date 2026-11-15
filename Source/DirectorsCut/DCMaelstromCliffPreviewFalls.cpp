#include "DCMaelstrom.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Components/LocalFogVolumeComponent.h"
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
    if(IsCalderaLake())
    {
        // The six imported sidefalls target the retired bowl. The lake is
        // supplied only by the new tunnel cataract; retain its local mist.
        ClearCliffPreviewFalls();
        if(!bActive||Elapsed<-7.f) { return; }
        const FName CataractMistTag(TEXT("DCCalderaCataractMist"));
        const FName ShoreMistTag(TEXT("DCCalderaShoreMist"));
        const FName SlopeMistTag(TEXT("DCCalderaSlopeMist"));
        const bool bHasCataractMist=CliffImpactMist.ContainsByPredicate(
            [CataractMistTag](const TObjectPtr<ULocalFogVolumeComponent>& Mist)
            { return Mist&&Mist->ComponentHasTag(CataractMistTag); });
        if(!bHasCataractMist)
        {
            const FVector FootM(-855.f,361.f,-400.f);
            const FVector IntoLake=FVector(855.f,-361.f,0.f).GetSafeNormal();
            for(int32 Layer=0;Layer<2;++Layer)
            {
                ULocalFogVolumeComponent* Mist=NewObject<ULocalFogVolumeComponent>(this);
                Mist->SetupAttachment(Arena); Mist->SetMobility(EComponentMobility::Movable);
                const float RadiusM=Layer?78.f:55.f;
                Mist->SetRelativeLocation((FootM+IntoLake*(Layer?45.f:0.f)
                    +FVector(0.f,0.f,Layer?26.f:18.f))*100.f);
                Mist->SetRelativeScale3D(FVector(RadiusM*100.f/ULocalFogVolumeComponent::GetBaseVolumeSize()));
                Mist->RadialFogExtinction=Layer?.065f:.10f;
                Mist->HeightFogExtinction=0.f;
                Mist->FogAlbedo=FLinearColor(.82f,.88f,.91f);
                Mist->FogEmissive=FLinearColor::Black; Mist->FogPhaseG=.45f;
                Mist->FogSortPriority=20;
                Mist->ComponentTags.Add(CataractMistTag);
                Mist->RegisterComponent(); CliffImpactMist.Add(Mist);
            }
            UE_LOG(LogTemp,Display,TEXT("DC_CALDERA_CATARACT_MIST scene_lit_volumes=2 foot_m=-855,361,-400"));
            auto AddMist=[&](const FVector& PositionM,float RadiusM,FName Tag,float Density)
            {
                ULocalFogVolumeComponent* Mist=NewObject<ULocalFogVolumeComponent>(this);
                Mist->SetupAttachment(Arena); Mist->SetMobility(EComponentMobility::Movable);
                Mist->SetRelativeLocation(PositionM*100.f);
                Mist->SetRelativeScale3D(FVector(RadiusM*100.f/ULocalFogVolumeComponent::GetBaseVolumeSize()));
                Mist->RadialFogExtinction=Density;
                Mist->HeightFogExtinction=Tag==ShoreMistTag?.10f:0.f;
                Mist->HeightFogFalloff=12.f;
                Mist->FogAlbedo=FLinearColor(.72f,.79f,.83f);
                Mist->FogEmissive=FLinearColor::Black; Mist->FogPhaseG=.35f;
                Mist->FogSortPriority=Tag==ShoreMistTag?-20:100;
                Mist->ComponentTags.Add(Tag); Mist->RegisterComponent(); CliffImpactMist.Add(Mist);
            };
            // A shallow, scene-lit veil breaks the hard perimeter waterline.
            // Architecture remains clear above it; no changes to the shared sky.
            TSharedPtr<FJsonObject> ArenaData;
            const TArray<TSharedPtr<FJsonValue>>* ShoreCenters=nullptr;
            if(ReadFallPreview(CliffArenaDataDirectory/TEXT("CompleteArena.json"),ArenaData))
            { ArenaData->TryGetArrayField(TEXT("shore_mist_centers_m"),ShoreCenters); }
            // Stay below the renderer's 32 visible-volume budget including
            // the slope, foot and exit banks; prioritize the nearby spray.
            for(int32 Sector=0;Sector<16;++Sector)
            {
                const float A=UE_TWO_PI*(Sector+.23f)/16.f;
                const float R=GetRimRadius(A)*.01f*.67f;
                FVector Center(FMath::Cos(A)*R,FMath::Sin(A)*R,-410.f);
                if(ShoreCenters&&ShoreCenters->IsValidIndex(Sector*2))
                {
                    const TArray<TSharedPtr<FJsonValue>>* XYZ=nullptr;
                    if((*ShoreCenters)[Sector*2]->TryGetArray(XYZ)&&XYZ->Num()==3)
                    { Center=FVector((*XYZ)[0]->AsNumber(),(*XYZ)[1]->AsNumber(),(*XYZ)[2]->AsNumber()); }
                }
                AddMist(Center,
                    300.f+25.f*FMath::Sin(Sector*1.7f),ShoreMistTag,.055f);
            }
            // After the descent, wind-blown mist gathers behind the ship.
            for(int32 Layer=0;Layer<8;++Layer)
            {
                const float R=FMath::Lerp(1570.f,950.f,Layer/7.f);
                const float Q=FMath::SmoothStep(.58f,.985f,R/1600.f);
                const float A=UE_PI-.40f*(1.f-Q);
                AddMist(FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,-400.f+400.f*Q+25.f),
                    160.f+15.f*FMath::Sin(Layer*2.f),SlopeMistTag,0.f);
            }
        }
        for(ULocalFogVolumeComponent* Mist:CliffImpactMist)
        {
            if(!Mist) { continue; }
            if(Mist->ComponentHasTag(CataractMistTag)||Mist->ComponentHasTag(ShoreMistTag))
            { Mist->SetVisibility(true); }
            if(Mist->ComponentHasTag(SlopeMistTag))
            {
                const float Gather=FMath::SmoothStep(26.f,42.f,Elapsed);
                Mist->SetRadialFogExtinction(1.35f*Gather);
                Mist->SetVisibility(Gather>0.f);
            }
        }
        return;
    }
    if((!bCompleteCliffArena&&!FParse::Param(FCommandLine::Get(),TEXT("DCCliffPreviewFall")))||!CliffPreviewMesh||!bActive||Elapsed<(bCompleteCliffArena?-7.f:0.f))
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
        // Scene-lit participating mist gives the fitted impact spray actual
        // depth. Keep it local to receiving water, with no emissive cloud mass.
        const TArray<TSharedPtr<FJsonValue>>* Outlets=nullptr;
        if(bCompleteCliffArena&&CliffPreviewFallCore&&Data->TryGetArrayField(TEXT("outlets"),Outlets))
        {
            for(const TSharedPtr<FJsonValue>& Value:*Outlets)
            {
                const TSharedPtr<FJsonObject>* Outlet=nullptr;
                FVector Foot; double Width=40.;
                if(!Value->TryGetObject(Outlet)||!FallPreviewVector(*Outlet,TEXT("contact_arena_m"),Foot)) { continue; }
                (*Outlet)->TryGetNumberField(TEXT("width_m"),Width);
                const FVector Drift=FVector(Foot.Y,-Foot.X,0.).GetSafeNormal();
                for(int32 Layer=0;Layer<2;++Layer)
                {
                    ULocalFogVolumeComponent* Mist=NewObject<ULocalFogVolumeComponent>(this);
                    Mist->SetupAttachment(Arena); Mist->SetMobility(EComponentMobility::Movable);
                    const float RadiusM=FMath::Clamp(static_cast<float>(Width)*.65f+18.f,45.f,90.f)*(Layer?1.15f:1.f);
                    Mist->SetRelativeLocation((Foot+Drift*(Layer?RadiusM*.8f:0.f)+FVector(0.,0.,RadiusM*.28f))*100.);
                    Mist->SetRelativeScale3D(FVector(RadiusM*100.f/ULocalFogVolumeComponent::GetBaseVolumeSize()));
                    Mist->RadialFogExtinction=Layer?.12f:.22f;
                    Mist->HeightFogExtinction=0.f;
                    Mist->FogAlbedo=FLinearColor(.82f,.87f,.90f);
                    Mist->FogEmissive=FLinearColor::Black; Mist->FogPhaseG=.55f;
                    Mist->RegisterComponent(); CliffImpactMist.Add(Mist);
                }
            }
            UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_IMPACT_MIST scene_lit_volumes=%d"),CliffImpactMist.Num());
        }
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
    for(ULocalFogVolumeComponent* Mist:CliffImpactMist) { if(Mist) { Mist->SetVisibility(true); } }
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
    for(ULocalFogVolumeComponent* Mist:CliffImpactMist) { if(Mist) { Mist->SetVisibility(false); } }
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
