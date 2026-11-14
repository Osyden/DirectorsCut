#include "DCMaelstrom.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SparseVolumeTexture/SparseVolumeTexture.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameViewportClient.h"
#include "ProceduralMeshComponent.h"
#include "DCMaelstromRoute.h"
#include "DCCliffGenerator.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"

namespace
{
TArray<TSharedPtr<FJsonValue>> FallVectorJson(const FVector& V)
{
    return {MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y), MakeShared<FJsonValueNumber>(V.Z)};
}
}

// First measured prototype: one physically sized lower falling reach and its
// impact. The full cliff's thin water core and advected surface foam remain.
// Enable explicitly until captures establish quality: -DCProceduralFalls.
void ADCMaelstrom::UpdateProceduralFalls()
{
    const bool bRenderFalls = FParse::Param(FCommandLine::Get(), TEXT("DCProceduralFalls"));
    const bool bDensityDiagnostic = FParse::Param(FCommandLine::Get(), TEXT("DCFallDensityDiagnostic"));
    FString ExportPath;
    const bool bExportProfile = FParse::Value(FCommandLine::Get(), TEXT("DCExportWaterfallProfile="), ExportPath)
        || FParse::Param(FCommandLine::Get(), TEXT("DCExportWaterfallProfile"));
    if (!bRenderFalls && !bExportProfile) { return; }
    if (!bActive || Elapsed < 0.f)
    {
        ClearProceduralFalls();
        return;
    }
    if (!bProceduralFallsTried)
    {
        bProceduralFallsTried = true;
        int32 FeedIndex = INDEX_NONE;
        float Best = TNumericLimits<float>::Max();
        const FTransform Frame = GetArenaTransform();
        for (int32 I = 0; I < WaterFeeds.Num(); ++I)
        {
            const FWaterFeed& Feed = WaterFeeds[I];
            if (Feed.Lip.Z - Feed.Foot.Z < 8500.f) { continue; }
            const float Distance = Frame.TransformPosition(Feed.Foot).SizeSquared();
            if (Distance < Best) { Best = Distance; FeedIndex = I; }
        }
        int32 RequestedSource = INDEX_NONE;
        if (FParse::Value(FCommandLine::Get(), TEXT("DCFallFeed="), RequestedSource))
        {
            FeedIndex = WaterFeeds.IndexOfByPredicate([RequestedSource](const FWaterFeed& Candidate)
            { return Candidate.SourceIndex == RequestedSource; });
        }
        if (!WaterFeeds.IsValidIndex(FeedIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("DC_PROCEDURAL_FALL no suitable 80m receiving reach"));
            return;
        }
        const FWaterFeed& Feed = WaterFeeds[FeedIndex];
        const FVector Inward = -FVector(Feed.Foot.X, Feed.Foot.Y, 0.f).GetSafeNormal();
        const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Inward.Y, Inward.X)) - 90.f;
        // Generator contract: centimetres, +X across, +Y out from cliff, +Z up;
        // the contact is (0, 1000, 0). Preserve voxel scale and imported pivot.
        const FVector Origin = Feed.Foot - Inward * 1000.f;
        if (bExportProfile)
        {
            // Match the centre of the existing procedural water core. Export in
            // the same yaw-only, physical-centimetre frame as the runtime SVT.
            const FTransform LocalFrame(FRotator(0.f, Yaw, 0.f), Origin);
            // Use the stable source angle, not atan2 of a barycentrically
            // interpolated lip: that introduces centimetres of contact drift.
            const float Angle = WaterfallAngle(Feed.SourceIndex);
            auto CorePoint = [&](float U, float Across)
            {
                const float V = FMath::Lerp(.84f, Feed.ContactV, U);
                return SampleWaterfallCorePoint(Angle, V, Across);
            };
            TArray<FVector> Complete;
            constexpr int32 Samples = 512;
            for (int32 I = 0; I <= Samples; ++I)
            {
                const float U = static_cast<float>(I) / Samples;
                Complete.Add(CorePoint(U, 0));
            }
            float LowerHeight = 8000.f;
            FParse::Value(FCommandLine::Get(), TEXT("DCWaterfallProfileHeight="), LowerHeight);
            const float CutZ = Feed.Foot.Z + FMath::Max(100.f, LowerHeight);
            TArray<FVector> Lower;
            TArray<float> LowerU;
            for (int32 I = 0; I < Complete.Num(); ++I)
            {
                if (Lower.IsEmpty() && Complete[I].Z > CutZ) { continue; }
                if (Lower.IsEmpty() && I > 0 && Complete[I - 1].Z > CutZ)
                {
                    const float Blend = (Complete[I - 1].Z - CutZ) / FMath::Max(.01, Complete[I - 1].Z - Complete[I].Z);
                    Lower.Add(FMath::Lerp(Complete[I - 1], Complete[I], Blend));
                    LowerU.Add((I - 1.f + Blend) / Samples);
                }
                Lower.Add(Complete[I]);
                LowerU.Add(static_cast<float>(I) / Samples);
            }
            if (Lower.Num() >= 2)
            {
                TSharedRef<FJsonObject> Profile = MakeShared<FJsonObject>();
                Profile->SetStringField(TEXT("schema"), TEXT("directorscut.waterfall_profile.v1"));
                Profile->SetStringField(TEXT("units"), TEXT("centimeters"));
                Profile->SetNumberField(TEXT("feed_index"), Feed.SourceIndex);
                Profile->SetNumberField(TEXT("feed_array_index"), FeedIndex);
                Profile->SetNumberField(TEXT("contact_v"), Feed.ContactV);
                Profile->SetArrayField(TEXT("contact_core_local_cm"), FallVectorJson(LocalFrame.InverseTransformPosition(Feed.ContactPoint)));
                Profile->SetNumberField(TEXT("source_half_width_cm"), Feed.Width);
                Profile->SetArrayField(TEXT("local_origin_arena_cm"), FallVectorJson(Origin));
                Profile->SetNumberField(TEXT("local_yaw_degrees"), Yaw);
                Profile->SetArrayField(TEXT("basis_x_arena"), FallVectorJson(LocalFrame.GetUnitAxis(EAxis::X)));
                Profile->SetArrayField(TEXT("basis_y_arena"), FallVectorJson(LocalFrame.GetUnitAxis(EAxis::Y)));
                Profile->SetArrayField(TEXT("basis_z_arena"), FallVectorJson(LocalFrame.GetUnitAxis(EAxis::Z)));
                Profile->SetArrayField(TEXT("source_local_cm"), FallVectorJson(LocalFrame.InverseTransformPosition(Lower[0])));
                Profile->SetArrayField(TEXT("foot_local_cm"), FallVectorJson(LocalFrame.InverseTransformPosition(Feed.Foot)));
                const float RouteT = DCMaelstromRoute::SecondsAtAngle(Angle);
                const FVector2D Current = DCMaelstromRoute::Tangent(FMath::Clamp(RouteT, -7.f, 179.f), Radius) * 1800.f;
                Profile->SetArrayField(TEXT("current_local_cm_s"), FallVectorJson(LocalFrame.InverseTransformVectorNoScale(FVector(Current, 0.f))));
                TArray<TSharedPtr<FJsonValue>> Points;
                float PathLength = 0.f;
                for (int32 I = 0; I < Lower.Num(); ++I)
                {
                    if (I) { PathLength += FVector::Distance(Lower[I - 1], Lower[I]); }
                    Points.Add(MakeShared<FJsonValueArray>(FallVectorJson(LocalFrame.InverseTransformPosition(Lower[I]))));
                }
                Profile->SetArrayField(TEXT("samples_local_cm"), Points);
                Profile->SetNumberField(TEXT("path_length_cm"), PathLength);
                // Nine lateral tracks preserve local ledges and creases across
                // the source width instead of extruding one centreline through
                // adjacent rock. They use the identical rendered-triangle query.
                TArray<TSharedPtr<FJsonValue>> Offsets, CrossSections, TrackContacts, TrackContactV;
                TArray<float> ContactVByTrack;
                const float LowerStartV = FMath::Lerp(.84f, Feed.ContactV, LowerU[0]);
                for (int32 X = 0; X < 9; ++X)
                {
                    const float Across = Feed.Width * (X / 4.f - 1.f);
                    Offsets.Add(MakeShared<FJsonValueNumber>(Across));
                    const DCCliffGenerator::FFirstWaterContact Hit = DCCliffGenerator::FirstWaterContact(
                        [&](float T) { return SampleWaterfallCorePoint(Angle, .84f * (1.f - T), Across); },
                        [&](const FVector2D& P, float& Height) { return SampleReceivingWater(P, Height); });
                    if (!Hit.bFound)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("DC_WATERFALL_PROFILE source=%d lateral track=%d has no valid water contact"), Feed.SourceIndex, X);
                        return;
                    }
                    const float TrackV = .84f * (1.f - Hit.Progress);
                    ContactVByTrack.Add(TrackV);
                    TrackContactV.Add(MakeShared<FJsonValueNumber>(TrackV));
                    TrackContacts.Add(MakeShared<FJsonValueArray>(FallVectorJson(LocalFrame.InverseTransformPosition(Hit.Position))));
                }
                for (float U : LowerU)
                {
                    TArray<TSharedPtr<FJsonValue>> Section;
                    const float Progress = (U - LowerU[0]) / FMath::Max(.0001f, 1.f - LowerU[0]);
                    for (int32 X = 0; X < 9; ++X)
                    {
                        Section.Add(MakeShared<FJsonValueArray>(FallVectorJson(LocalFrame.InverseTransformPosition(
                            SampleWaterfallCorePoint(Angle, FMath::Lerp(LowerStartV, ContactVByTrack[X], Progress),
                                Feed.Width * (X / 4.f - 1.f))))));
                    }
                    CrossSections.Add(MakeShared<FJsonValueArray>(Section));
                }
                Profile->SetArrayField(TEXT("cross_offsets_cm"), Offsets);
                Profile->SetArrayField(TEXT("cross_sections_local_cm"), CrossSections);
                Profile->SetArrayField(TEXT("cross_contact_v"), TrackContactV);
                Profile->SetArrayField(TEXT("cross_contact_core_local_cm"), TrackContacts);
                Profile->SetStringField(TEXT("surface_contract"), TEXT("shared rendered-triangle core sampler; ends at first valid receiving-water ContactV; no upward contact blend"));
                // Surface samples, not a horizontal impact plane: the local
                // receiving water slopes into the spiral and changes across it.
                constexpr int32 GridCount = 49;
                constexpr float GridStep = 200.f;
                TSharedRef<FJsonObject> Water = MakeShared<FJsonObject>();
                const FVector FootLocal = LocalFrame.InverseTransformPosition(Feed.Foot);
                const FVector GridOrigin = FootLocal - FVector(GridStep * 24, GridStep * 24, 0);
                Water->SetNumberField(TEXT("count"), GridCount);
                Water->SetNumberField(TEXT("step_cm"), GridStep);
                Water->SetArrayField(TEXT("origin_local_cm"), FallVectorJson(GridOrigin));
                TArray<TSharedPtr<FJsonValue>> Heights, Coverage, ShoreClearance;
                for (int32 Y = 0; Y < GridCount; ++Y)
                {
                    for (int32 X = 0; X < GridCount; ++X)
                    {
                        const FVector P = LocalFrame.TransformPosition(GridOrigin + FVector(X * GridStep, Y * GridStep, 0));
                        float MeanHeight = HeightAt(FVector2D(P), false);
                        const bool bWater = SampleReceivingWater(FVector2D(P), MeanHeight);
                        Heights.Add(MakeShared<FJsonValueNumber>(MeanHeight - Origin.Z));
                        Coverage.Add(MakeShared<FJsonValueBoolean>(bWater));
                        const float R = P.Size2D();
                        const float A = FMath::Atan2(P.Y, P.X);
                        ShoreClearance.Add(MakeShared<FJsonValueNumber>(FMath::Min(R - 120.f, WaterEdgeRadius(A) - R)));
                    }
                }
                Water->SetArrayField(TEXT("heights_local_cm_y_major"), Heights);
                Water->SetArrayField(TEXT("coverage_y_major"), Coverage);
                Water->SetArrayField(TEXT("shore_clearance_cm_y_major"), ShoreClearance);
                Water->SetBoolField(TEXT("mean_surface"), true);
                Profile->SetObjectField(TEXT("receiving_water"), Water);
                FString Payload;
                FJsonSerializer::Serialize(Profile, TJsonWriterFactory<>::Create(&Payload));
                const FTCHARToUTF8 Bytes(*Payload);
                FSHAHash Hash;
                FSHA1::HashBuffer(Bytes.Get(), Bytes.Length(), Hash.Hash);
                Profile->SetStringField(TEXT("profile_sha1"), Hash.ToString());
                Payload.Reset();
                FJsonSerializer::Serialize(Profile, TJsonWriterFactory<>::Create(&Payload));
                if (ExportPath.IsEmpty()) { ExportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("WaterfallProfile.json")); }
                ExportPath = FPaths::ConvertRelativePathToFull(ExportPath);
                IFileManager::Get().MakeDirectory(*FPaths::GetPath(ExportPath), true);
                const bool bSaved = FFileHelper::SaveStringToFile(Payload, *ExportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
                UE_LOG(LogTemp, Display, TEXT("DC_WATERFALL_PROFILE saved=%d path=%s feed=%d samples=%d length=%.1fm hash=%s"),
                    bSaved, *ExportPath, Feed.SourceIndex, Lower.Num(), PathLength * .01f, *Hash.ToString());
            }
        }
        if (!bRenderFalls) { return; }
        if (IConsoleVariable* Range = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance")))
        {
            // The default 300m trace range omits physically sized falls across
            // this kilometre-scale basin. Scope our change to this prototype.
            const float Required = Radius * 2.2f;
            if (Range->GetFloat() < Required)
            {
                PreviousFallTraceDistance = Range->GetFloat();
                AppliedFallTraceDistance = Required;
                Range->Set(Required, ECVF_SetByCode);
            }
        }
        for (int32 Layer = 0; Layer < 2; ++Layer)
        {
            if (Layer == 0 && Feed.Lip.Z - Feed.Foot.Z < 8500.f) { continue; }
            const TCHAR* Name = Layer == 0 ? TEXT("Falling") : TEXT("Impact");
            // A fitted cache belongs to one measured feed. Direct late seeks
            // choose a different nearest feed and must never reuse feed 8's
            // geometry there. Generic ballistic data requires its own flag.
            const bool bGenericPrototype = FParse::Param(FCommandLine::Get(), TEXT("DCGenericWaterfallPrototype"));
            const FString Root = bGenericPrototype ? TEXT("/Game/DC/Maelstrom/ProceduralWaterfalls/")
                : FString::Printf(TEXT("/Game/DC/Maelstrom/ProceduralWaterfalls/Feed%02d/"), Feed.SourceIndex);
            const FString VolumePath = Root + Name + TEXT(".") + Name;
            const FString MaterialPath = Root + TEXT("M_") + Name + TEXT(".M_") + Name;
            USparseVolumeTexture* Volume = LoadObject<USparseVolumeTexture>(nullptr, *VolumePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
            UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, *MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
            if (!Volume || !Base)
            {
                UE_LOG(LogTemp, Warning, TEXT("DC_PROCEDURAL_FALL %s feed=%d missing at %s; no cross-feed fitted fallback"), Name, Feed.SourceIndex, *Root);
                continue;
            }
            const FTransform NativeFrame = Volume->GetFrameTransform();
            const FVector NativeVoxel = NativeFrame.GetScale3D();
            float MaterialVoxelCm = 0.f;
            const bool bHasVoxelUnits = Base->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VoxelSizeCm")), MaterialVoxelCm);
            if (NativeVoxel.X <= 0.f || !FMath::IsNearlyEqual(NativeVoxel.X, NativeVoxel.Y, .001)
                || !FMath::IsNearlyEqual(NativeVoxel.X, NativeVoxel.Z, .001)
                || !bHasVoxelUnits || !FMath::IsNearlyEqual(static_cast<float>(NativeVoxel.X), MaterialVoxelCm, .001f))
            {
                UE_LOG(LogTemp, Error, TEXT("DC_FALL_UNITS rejected %s nativeVoxel=%s materialVoxelCm=%.6f present=%d; require matching isotropic import and refreshed waterfall materials"),
                    *VolumePath, *NativeVoxel.ToString(), MaterialVoxelCm, bHasVoxelUnits);
                continue;
            }
            FActorSpawnParameters Params;
            Params.Owner = this;
            AHeterogeneousVolume* Actor = GetWorld()->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(), FTransform::Identity, Params);
            UHeterogeneousVolumeComponent* C = Actor ? Actor->FindComponentByClass<UHeterogeneousVolumeComponent>() : nullptr;
            if (!C) { if (Actor) { Actor->Destroy(); } continue; }
            Actor->AttachToComponent(Arena, FAttachmentTransformRules::KeepRelativeTransform);
            Actor->SetActorRelativeTransform(FTransform(FRotator(0.f, Yaw, 0.f), Origin));
            UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Actor);
            MID->SetSparseVolumeTextureParameterValue(TEXT("SparseVolumeTexture"), Volume);
            float Density = Layer == 0 ? .004f : .0015f;
            // CLI/material Density is extinction per world centimetre. The
            // generated material multiplies by verified VoxelSizeCm because
            // native HV integration uses voxel-local marching distance.
            FParse::Value(FCommandLine::Get(), Layer == 0 ? TEXT("DCFallDensity=") : TEXT("DCImpactDensity="), Density);
            // Pure sampled-density emission isolates native SVT sampling from
            // illumination/extinction. Never enable this for normal gameplay.
            float DiagnosticEmission = bDensityDiagnostic ? 1.f : 0.f;
            if (bDensityDiagnostic) { FParse::Value(FCommandLine::Get(), TEXT("DCFallDiagnosticEmission="), DiagnosticEmission); }
            float AppliedDensity = FMath::Max(0.f, Density);
            float DiagnosticConstant = 0.f;
            if (bDensityDiagnostic)
            {
                AppliedDensity = 0.f;
                FParse::Value(FCommandLine::Get(), TEXT("DCFallDiagnosticExtinction="), AppliedDensity);
                FParse::Value(FCommandLine::Get(), TEXT("DCFallConstantDensity="), DiagnosticConstant);
            }
            MID->SetScalarParameterValue(TEXT("Density"), FMath::Max(0.f, AppliedDensity));
            MID->SetScalarParameterValue(TEXT("DiagnosticEmission"), FMath::Max(0.f, DiagnosticEmission));
            MID->SetScalarParameterValue(TEXT("DiagnosticConstantDensity"), FMath::Max(0.f, DiagnosticConstant));
            if (bDensityDiagnostic)
            {
                float Available = 0.f;
                if (!Base->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DiagnosticEmission")), Available))
                { UE_LOG(LogTemp, Error, TEXT("DC_FALL_DIAGNOSTIC material %s lacks DiagnosticEmission; rebuild waterfall materials before interpreting the capture"), *MaterialPath); }
                if (DiagnosticConstant > 0.f && !Base->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DiagnosticConstantDensity")), Available))
                { UE_LOG(LogTemp, Error, TEXT("DC_FALL_DIAGNOSTIC material %s lacks DiagnosticConstantDensity; constant-box result is invalid"), *MaterialPath); }
            }
            C->SetMaterial(0, MID);
            C->bPivotAtCentroid = false;
            C->bIssueBlockingRequests = false;
            C->StepFactor = 1.f;
            C->ShadowStepFactor = 2.f;
            C->SetPlaying(false);
            C->SetLooping(true);
            C->SetStartFrame(0.f);
            C->SetEndFrame(FMath::Max(0, Volume->GetNumFrames() - 1));
            ProceduralFalls.Add({Actor, FMath::Max(1, Volume->GetNumFrames())});
            UE_LOG(LogTemp, Display, TEXT("DC_PROCEDURAL_FALL %s feed=%d frames=%d resolution=%s foot=%s lip=%s originWorld=%s"),
                Name, Feed.SourceIndex, Volume->GetNumFrames(), *Volume->GetVolumeResolution().ToString(),
                *Feed.Foot.ToCompactString(), *Feed.Lip.ToCompactString(), *Actor->GetActorLocation().ToCompactString());
            UE_LOG(LogTemp, Display, TEXT("DC_FALL_NATIVE actor=%s layer=%s asset=%s frameTranslation=%s frameScale=%s frameRotation=%s actorRelative=%s density=%.6f emission=%.6f"),
                *Actor->GetName(), Name, *Volume->GetPathName(), *NativeFrame.GetTranslation().ToString(),
                *NativeFrame.GetScale3D().ToString(), *NativeFrame.Rotator().ToString(),
                *C->GetRelativeTransform().ToString(), AppliedDensity, DiagnosticEmission);
        }
        if (ProceduralFalls.Num() && FParse::Param(FCommandLine::Get(), TEXT("DCFallCamera")))
        {
            FallReviewCamera = GetWorld()->SpawnActor<ACameraActor>();
            if (ACameraActor* Camera = FallReviewCamera.Get())
            {
                Camera->GetCameraComponent()->SetFieldOfView(60.f);
                Camera->GetCameraComponent()->bConstrainAspectRatio = false;
                if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->SetViewTarget(Camera); }
            }
        }
    }
    // Both the coarse 48-frame and fine 96-frame generated loops last four
    // seconds. Derive playback from frame count instead of slowing the fine
    // sequence to half speed. Custom-duration exports can override this.
    float LoopSeconds = 4.f;
    FParse::Value(FCommandLine::Get(), TEXT("DCWaterfallLoopSeconds="), LoopSeconds);
    float FPS = 0.f;
    FParse::Value(FCommandLine::Get(), TEXT("DCFallFPS="), FPS);
    float FixedFrame = 0.f;
    const bool bFixedFrame = FParse::Value(FCommandLine::Get(), TEXT("DCFallFixedFrame="), FixedFrame);
    for (FProceduralFall& Fall : ProceduralFalls)
    {
        if (AHeterogeneousVolume* Actor = Fall.Actor.Get())
        {
            if (UHeterogeneousVolumeComponent* C = Actor->FindComponentByClass<UHeterogeneousVolumeComponent>())
            {
                const float PlaybackFPS = FPS > 0.f ? FPS : Fall.Frames / FMath::Max(.1f, LoopSeconds);
                C->SetFrame(bFixedFrame ? FMath::Clamp(FixedFrame, 0.f, Fall.Frames - 1.f)
                    : FMath::Fmod(FMath::Max(0.f, Elapsed) * PlaybackFPS, static_cast<float>(Fall.Frames)));
                // Wait for the component's native frame update and bounds
                // refresh; a spawn-time component still has default bounds.
                if (++Fall.DiagnosticTicks == 4)
                {
                    UE_LOG(LogTemp, Display, TEXT("DC_FALL_COMPONENT actor=%s frameTranslation=%s frameScale=%s frameRotation=%s worldScale=%s boundsOrigin=%s boundsExtent=%s resolution=%s pivotCentroid=%d visible=%d"),
                        *Actor->GetName(), *C->FrameTransform.GetTranslation().ToString(), *C->FrameTransform.GetScale3D().ToString(),
                        *C->FrameTransform.Rotator().ToString(), *C->GetComponentScale().ToString(),
                        *C->Bounds.Origin.ToString(), *C->Bounds.BoxExtent.ToString(), *C->VolumeResolution.ToString(),
                        C->bPivotAtCentroid, C->IsVisible());
                    UMaterialInterface* Material = C->GetMaterial(0);
                    USparseVolumeTexture* BoundVolume = nullptr;
                    float MaterialDensity = -1.f, MaterialEmission = -1.f, MaterialVoxel = -1.f, Constant = -1.f;
                    if (Material)
                    {
                        Material->GetSparseVolumeTextureParameterValue(FMaterialParameterInfo(TEXT("SparseVolumeTexture")), BoundVolume);
                        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")), MaterialDensity);
                        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DiagnosticEmission")), MaterialEmission);
                        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VoxelSizeCm")), MaterialVoxel);
                        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DiagnosticConstantDensity")), Constant);
                    }
                    auto CVarValue = [](const TCHAR* Name)
                    { IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name); return V ? V->GetFloat() : -1.f; };
                    const UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
                    UE_LOG(LogTemp, Display, TEXT("DC_FALL_BINDING actor=%s hidden=%d material=%s svt=%s density=%.6f emission=%.6f constant=%.6f voxelCm=%.3f showHV=%d enabled=%.0f allowed=%.0f sparse=%.0f sparseVoxel=%.0f preshading=%.0f cameraAVSM=%.0f"),
                        *Actor->GetName(), Actor->IsHidden(), *GetNameSafe(Material), *GetPathNameSafe(BoundVolume),
                        MaterialDensity, MaterialEmission, Constant, MaterialVoxel,
                        Viewport ? Viewport->EngineShowFlags.HeterogeneousVolumes : -1,
                        CVarValue(TEXT("r.HeterogeneousVolumes")), CVarValue(TEXT("r.HeterogeneousVolumes.Allowed")),
                        CVarValue(TEXT("r.SparseVolumeTexture")), CVarValue(TEXT("r.HeterogeneousVolumes.SparseVoxel")),
                        CVarValue(TEXT("r.HeterogeneousVolumes.Preshading")), CVarValue(TEXT("r.HeterogeneousVolumes.LightingCache.UseAVSM")));
                }
            }
        }
    }
    if (ProceduralFalls.Num())
    {
        auto HideForReview = [&](USceneComponent* Component)
        {
            if (!Component) { return; }
            if (!FallHiddenComponents.ContainsByPredicate([Component](const FFallHiddenComponent& Entry)
                { return Entry.Component.Get() == Component; }))
            { FallHiddenComponents.Add({Component, Component->IsVisible()}); }
            Component->SetVisibility(false);
        };
        if (FParse::Param(FCommandLine::Get(), TEXT("DCFallVolumesOnly"))) { HideForReview(Whitewater); }
        if (FParse::Param(FCommandLine::Get(), TEXT("DCFallHideCliffs")))
        {
            HideForReview(Cliffs);
            for (UInstancedStaticMeshComponent* Rock : CliffRocks) { HideForReview(Rock); }
            for (UInstancedStaticMeshComponent* Rock : CliffShadowRocks) { HideForReview(Rock); }
        }
    }
    if (ACameraActor* Camera = FallReviewCamera.Get())
    {
        if (ProceduralFalls.Num())
        {
            if (AActor* Fall = ProceduralFalls[0].Actor.Get())
            {
                const FTransform Frame = Fall->GetActorTransform();
                const FVector Eye = Frame.TransformPosition(FVector(7000.f, 14500.f, 4300.f));
                const FVector Target = Frame.TransformPosition(FVector(0.f, 700.f, 3400.f));
                Camera->SetActorLocationAndRotation(Eye, (Target-Eye).Rotation());
            }
        }
    }
}

void ADCMaelstrom::ClearProceduralFalls()
{
    for (const FFallHiddenComponent& Entry : FallHiddenComponents)
    {
        if (USceneComponent* Component = Entry.Component.Get())
        {
            // Respect the encounter's normal hidden state on exit/early seek.
            Component->SetVisibility(Entry.bWasVisible && bActive && Elapsed >= -7.f);
        }
    }
    if (FallHiddenComponents.Num()) { UE_LOG(LogTemp, Display, TEXT("DC_FALL_DIAGNOSTIC restored %d component visibility states"), FallHiddenComponents.Num()); }
    FallHiddenComponents.Reset();
    if (ACameraActor* Camera = FallReviewCamera.Get())
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (PC->GetViewTarget() == Camera) { PC->SetViewTarget(PC->GetPawn()); }
        }
        Camera->Destroy();
    }
    FallReviewCamera.Reset();
    for (FProceduralFall& Fall : ProceduralFalls)
    {
        if (AActor* Actor = Fall.Actor.Get()) { Actor->Destroy(); }
    }
    if (ProceduralFalls.Num()) { UE_LOG(LogTemp, Display, TEXT("DC_PROCEDURAL_FALL cleared %d layers"), ProceduralFalls.Num()); }
    ProceduralFalls.Reset();
    bProceduralFallsTried = false;
    if (PreviousFallTraceDistance >= 0.f)
    {
        if (IConsoleVariable* Range = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance")))
        {
            // An encounter effect may subsequently have requested a longer
            // range; restore only while the value is still the one we set.
            if (FMath::IsNearlyEqual(Range->GetFloat(), AppliedFallTraceDistance))
            {
                Range->Set(PreviousFallTraceDistance, ECVF_SetByCode);
                UE_LOG(LogTemp, Display, TEXT("DC_PROCEDURAL_FALL restored trace distance %.0f"), PreviousFallTraceDistance);
            }
        }
        PreviousFallTraceDistance = AppliedFallTraceDistance = -1.f;
    }
}
