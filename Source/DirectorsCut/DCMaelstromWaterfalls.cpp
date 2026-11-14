#include "DCMaelstrom.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SparseVolumeTexture/SparseVolumeTexture.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

// First measured prototype: one physically sized lower falling reach and its
// impact. The full cliff's thin water core and advected surface foam remain.
// Enable explicitly until captures establish quality: -DCProceduralFalls.
void ADCMaelstrom::UpdateProceduralFalls()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("DCProceduralFalls"))) { return; }
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
        FParse::Value(FCommandLine::Get(), TEXT("DCFallFeed="), FeedIndex);
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
            const FString Root = TEXT("/Game/DC/Maelstrom/ProceduralWaterfalls/");
            const FString VolumePath = Root + Name + TEXT(".") + Name;
            const FString MaterialPath = Root + TEXT("M_") + Name + TEXT(".M_") + Name;
            USparseVolumeTexture* Volume = LoadObject<USparseVolumeTexture>(nullptr, *VolumePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
            UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, *MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
            if (!Volume || !Base)
            {
                UE_LOG(LogTemp, Warning, TEXT("DC_PROCEDURAL_FALL %s missing; run generator and import_procedural_waterfalls.py"), Name);
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
            FParse::Value(FCommandLine::Get(), Layer == 0 ? TEXT("DCFallDensity=") : TEXT("DCImpactDensity="), Density);
            MID->SetScalarParameterValue(TEXT("Density"), FMath::Max(0.f, Density));
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
                Name, FeedIndex, Volume->GetNumFrames(), *Volume->GetVolumeResolution().ToString(),
                *Feed.Foot.ToCompactString(), *Feed.Lip.ToCompactString(), *Actor->GetActorLocation().ToCompactString());
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
    for (FProceduralFall& Fall : ProceduralFalls)
    {
        if (AHeterogeneousVolume* Actor = Fall.Actor.Get())
        {
            if (UHeterogeneousVolumeComponent* C = Actor->FindComponentByClass<UHeterogeneousVolumeComponent>())
            {
                const float PlaybackFPS = FPS > 0.f ? FPS : Fall.Frames / FMath::Max(.1f, LoopSeconds);
                C->SetFrame(FMath::Fmod(FMath::Max(0.f, Elapsed) * PlaybackFPS, static_cast<float>(Fall.Frames)));
            }
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
