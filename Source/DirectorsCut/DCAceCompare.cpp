#include "DCAceCompare.h"
#include "DCAceFace.h"
#include "ACEAudioCurveSourceComponent.h"
#include "ACEBlueprintLibrary.h"
#include "AsyncActionAnimateCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"
#include "Audio.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogDCAceCompare, Log, All);

ADCAceCompareGameMode::ADCAceCompareGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;
}

void ADCAceCompareGameMode::StartPlay()
{
    Super::StartPlay();
    const TCHAR* Cmd = FCommandLine::Get();
    int32 CIGOverride = 0;
    if (FParse::Value(Cmd, TEXT("useCIGforAI="), CIGOverride) && CIGOverride != 0)
    {
        UE_LOG(LogDCAceCompare, Error, TEXT("Shared CUDA/graphics context is disabled following the 15 Nov GPU hang."));
        FGenericPlatformMisc::RequestExitWithStatus(false, 1);
        return;
    }
    if (!FParse::Param(Cmd, TEXT("ACEBaseline")) && !FParse::Param(Cmd, TEXT("DCEnableACE")))
    {
        UE_LOG(LogDCAceCompare, Error, TEXT("Local inference requires explicit -DCEnableACE; see the crash report before testing."));
        FGenericPlatformMisc::RequestExitWithStatus(false, 1);
        return;
    }
    if (!FParse::Value(Cmd, TEXT("useCIGforAI="), CIGOverride)) { FCommandLine::Append(TEXT(" -useCIGforAI=0")); }
    Cmd = FCommandLine::Get();
    bBaseline = FParse::Param(Cmd, TEXT("ACEBaseline"));
    bAutoQuit = FParse::Param(Cmd, TEXT("ACEAutoQuit"));
    FParse::Value(Cmd, TEXT("ACERepeats="), RepeatCount);
    RepeatCount = FMath::Clamp(RepeatCount, 1, 9);
    FString ProviderText;
    if (FParse::Value(Cmd, TEXT("ACEProvider="), ProviderText)) { Provider = FName(*ProviderText); }
    WavPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("ACE/Fixtures/finch_confirmation.wav"));
    FParse::Value(Cmd, TEXT("ACEWav="), WavPath);
    OutputDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("ACE/Runs") /
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")) + (bBaseline ? TEXT("_baseline") : TEXT("_ace"))));
    FParse::Value(Cmd, TEXT("ACEOutput="), OutputDir);
    IFileManager::Get().MakeDirectory(*OutputDir, true);
    FParse::Value(Cmd, TEXT("ACECharacter="), CharacterPath);

    UClass* AvatarClass = CharacterPath.IsEmpty() ? AActor::StaticClass() : LoadClass<AActor>(nullptr, *CharacterPath);
    if (!AvatarClass) { FinishTest(TEXT("character_load_failed")); return; }
    Avatar = GetWorld()->SpawnActor<AActor>(AvatarClass, FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Avatar) { FinishTest(TEXT("character_spawn_failed")); return; }
    if (!Avatar->GetRootComponent())
    {
        USceneComponent* Root = NewObject<USceneComponent>(Avatar);
        Avatar->SetRootComponent(Root);
        Root->RegisterComponent();
    }
    if (CharacterPath.IsEmpty() && !FParse::Param(Cmd, TEXT("ACECurvesOnly")))
    {
        USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/MetaHumans/Common/Face/Face_Archetype.Face_Archetype"));
        UClass* AnimClass = LoadClass<UAnimInstance>(nullptr,
            TEXT("/Game/MetaHumans/Common/Face/Face_AnimBP.Face_AnimBP_C"));
        if (!Mesh || !AnimClass) { FinishTest(TEXT("reference_face_missing")); return; }
        USkeletalMeshComponent* Face = NewObject<USkeletalMeshComponent>(Avatar, TEXT("Face"));
        Avatar->AddInstanceComponent(Face);
        Face->SetupAttachment(Avatar->GetRootComponent());
        Face->SetSkeletalMesh(Mesh);
        // The reference skeletal mesh faces +Y; the test camera sits on +X.
        Face->SetRelativeRotation(FRotator(0, -90, 0));
        Face->SetAnimInstanceClass(AnimClass);
        Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Face->RegisterComponent();
        ReferenceFace = Face;
        CharacterPath = TEXT("NVIDIA sample MetaHuman Face_Archetype (evaluation)");
    }
    if (!CharacterPath.IsEmpty() && !ReferenceFace)
    {
        DCAceFace::Bind(Avatar);
        Avatar->SetActorRotation(FRotator(0, -90, 0));
        TInlineComponentArray<USkeletalMeshComponent*> Meshes(Avatar);
        for (USkeletalMeshComponent* Mesh : Meshes)
        {
            if (Mesh->GetName() == TEXT("Face")) { ReferenceFace = Mesh; }
        }
    }
    Curves = Avatar->FindComponentByClass<UACEAudioCurveSourceComponent>();
    if (!Curves)
    {
        Curves = NewObject<UACEAudioCurveSourceComponent>(Avatar);
        Avatar->AddInstanceComponent(Curves);
        Curves->SetupAttachment(Avatar->GetRootComponent());
        Curves->RegisterComponent();
    }
    Curves->OnAnimationStarted.AddDynamic(this, &ADCAceCompareGameMode::AnimationStarted);
    Curves->OnAnimationEnded.AddDynamic(this, &ADCAceCompareGameMode::AnimationEnded);

    ACameraActor* Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(140, 0, 145), FRotator(0, 180, 0));
    Camera->GetCameraComponent()->FieldOfView = 40;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->SetViewTarget(Camera); }
    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-30, 160, 0));
    Sun->GetLightComponent()->SetIntensity(5);
    APointLight* Fill = GetWorld()->SpawnActor<APointLight>(FVector(140, -100, 180), FRotator::ZeroRotator);
    Fill->PointLightComponent->SetIntensity(2000);
    Fill->PointLightComponent->SetAttenuationRadius(600);

    CurveCsv = TEXT("seconds,frame_ms,face_jaw_curve,jaw_rotation_delta");
    for (const FName& Name : UACEAudioCurveSourceComponent::CurveNames) { CurveCsv += TEXT(",") + Name.ToString(); }
    CurveCsv += TEXT("\n");
    UE_LOG(LogDCAceCompare, Display, TEXT("Mode=%s Provider=%s Character=%s WAV=%s"),
        bBaseline ? TEXT("baseline") : TEXT("ACE"), *Provider.ToString(),
        CharacterPath.IsEmpty() ? TEXT("none (curve-only test)") : *CharacterPath, *WavPath);
    if (!bBaseline)
    {
        const TArray<FName> Providers = UACEBlueprintLibrary::GetAvailableA2FProviderNames();
        for (const FName& Name : Providers) { UE_LOG(LogDCAceCompare, Display, TEXT("AvailableProvider=%s"), *Name.ToString()); }
        // Never silently select a remote/default provider for this local-only experiment.
        if (!Provider.ToString().StartsWith(TEXT("LocalA2F-")) || !Providers.Contains(Provider))
        { FinishTest(TEXT("local_provider_missing")); return; }
        UACEBlueprintLibrary::OverrideA2F3DInferenceMode(false);
    }
}

void ADCAceCompareGameMode::BeginClip()
{
    bStarted = true;
    bAudioStarted = bAudioEnded = bSendDone = bSendSucceeded = bScreenshotTaken = bSkipTimingSample = false;
    StartDelayMs = SendMs = -1;
    SampleAt = SumFrameMs = MaxFrameMs = PeakJawBoneChange = 0;
    FrameCount = CurveFrames = 0;
    PeakCurve = PeakFaceJawCurve = 0;
    CurveCsv = TEXT("seconds,frame_ms,face_jaw_curve,jaw_rotation_delta");
    for (const FName& Name : UACEAudioCurveSourceComponent::CurveNames) { CurveCsv += TEXT(",") + Name.ToString(); }
    CurveCsv += TEXT("\n");
    TArray<uint8> Bytes;
    FWaveModInfo Info;
    if (!FFileHelper::LoadFileToArray(Bytes, *WavPath) || !Info.ReadWaveInfo(Bytes.GetData(), Bytes.Num()) ||
        !Info.pBitsPerSample || *Info.pBitsPerSample != 16 || *Info.pChannels != 1 || *Info.pSamplesPerSec == 0)
    { FinishTest(TEXT("invalid_pcm16_mono_wav")); return; }
    Duration = static_cast<float>(Info.SampleDataSize) / (2.f * *Info.pSamplesPerSec);
    SentAt = FPlatformTime::Seconds();
    if (ReferenceFace) { InitialJawRotation = ReferenceFace->GetBoneQuaternion(TEXT("FACIAL_C_Jaw"), EBoneSpaces::ComponentSpace); }
    if (bBaseline)
    {
        BaselineWave = NewObject<USoundWaveProcedural>(this);
        BaselineWave->SetSampleRate(*Info.pSamplesPerSec);
        BaselineWave->NumChannels = 1;
        BaselineWave->Duration = INDEFINITELY_LOOPING_DURATION;
        BaselineWave->SoundGroup = SOUNDGROUP_Voice;
        BaselineWave->QueueAudio(Info.SampleDataStart, Info.SampleDataSize);
        BaselineAudio = UGameplayStatics::SpawnSound2D(this, BaselineWave);
        if (!BaselineAudio) { FinishTest(TEXT("baseline_audio_failed")); return; }
        // Baseline timestamp is the playback request, not measured audio-device latency.
        AnimationStarted();
        bSendDone = bSendSucceeded = true;
        SendMs = 0;
    }
    else
    {
        Request = UAsyncActionAnimateCharacter::AnimateCharacterFromWavFileAsync(this, Avatar, WavPath,
            FAudio2FaceEmotion(), nullptr, Provider);
        if (!Request) { FinishTest(TEXT("request_creation_failed")); return; }
        Request->AudioSendCompleted.AddDynamic(this, &ADCAceCompareGameMode::SendCompleted);
        Request->Activate();
    }
}

void ADCAceCompareGameMode::AnimationStarted()
{
    if (bAudioStarted) { return; }
    bAudioStarted = true;
    PlaybackStartedAt = FPlatformTime::Seconds();
    StartDelayMs = (PlaybackStartedAt - SentAt) * 1000;
    UE_LOG(LogDCAceCompare, Display, TEXT("PlaybackStarted delay_ms=%.2f"), StartDelayMs);
}

void ADCAceCompareGameMode::AnimationEnded() { bAudioEnded = true; }
void ADCAceCompareGameMode::SendCompleted(bool bSuccess)
{
    bSendDone = true;
    bSendSucceeded = bSuccess;
    SendMs = (FPlatformTime::Seconds() - SentAt) * 1000;
    UE_LOG(LogDCAceCompare, Display, TEXT("AudioSendCompleted success=%d elapsed_ms=%.2f"), bSuccess, SendMs);
}

void ADCAceCompareGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) { return; }
    if (!bStarted)
    {
        if (GetWorld()->GetTimeSeconds() > 5 && FPlatformTime::Seconds() >= NextRunAt) { BeginClip(); }
        return;
    }
    const double Now = FPlatformTime::Seconds();
    if (bBaseline && bAudioStarted && Now - PlaybackStartedAt > Duration + 0.2) { bAudioEnded = true; }
    if (bSendDone && !bSendSucceeded) { FinishTest(TEXT("inference_failed")); return; }
    if (bAudioStarted && !bAudioEnded && !bSkipTimingSample)
    {
        ++FrameCount;
        SumFrameMs += DeltaSeconds * 1000;
        MaxFrameMs = FMath::Max(MaxFrameMs, static_cast<double>(DeltaSeconds * 1000));
    }
    bSkipTimingSample = false;
    if (Now >= SampleAt)
    {
        SampleAt = Now + 0.05;
        TArray<float> Weights;
        Curves->GetCurveOutputsInterp(Weights);
        float FramePeak = 0;
        float FaceJaw = 0;
        double BoneChange = 0;
        if (ReferenceFace)
        {
            if (UAnimInstance* Anim = ReferenceFace->GetAnimInstance()) { FaceJaw = Anim->GetCurveValue(TEXT("JawOpen")); }
            BoneChange = InitialJawRotation.AngularDistance(ReferenceFace->GetBoneQuaternion(TEXT("FACIAL_C_Jaw"), EBoneSpaces::ComponentSpace));
            PeakFaceJawCurve = FMath::Max(PeakFaceJawCurve, FaceJaw);
            PeakJawBoneChange = FMath::Max(PeakJawBoneChange, BoneChange);
        }
        CurveCsv += FString::Printf(TEXT("%.4f,%.3f,%.5f,%.6f"), Now - SentAt, DeltaSeconds * 1000, FaceJaw, BoneChange);
        for (int32 i = 0; i < 55; ++i)
        {
            const float Weight = Weights.IsValidIndex(i) ? Weights[i] : 0;
            FramePeak = FMath::Max(FramePeak, FMath::Abs(Weight));
            CurveCsv += FString::Printf(TEXT(",%.5f"), Weight);
        }
        CurveCsv += TEXT("\n");
        if (FramePeak > 0.001f) { ++CurveFrames; }
        PeakCurve = FMath::Max(PeakCurve, FramePeak);
        if (GEngine) { GEngine->AddOnScreenDebugMessage(710, 1, FColor::Cyan,
            FString::Printf(TEXT("%s | %s | start %.0f ms | curve peak %.3f\n%s"),
                bBaseline ? TEXT("BASELINE: Piper audio") : TEXT("ACE: local Audio2Face"),
                *Provider.ToString(), StartDelayMs, FramePeak,
                CharacterPath.IsEmpty() ? TEXT("Curve-only diagnostic: no character loaded") : TEXT("Character comparison"))); }
    }
    if (bAudioStarted && !bScreenshotTaken && Now - PlaybackStartedAt > 1)
    {
        FScreenshotRequest::RequestScreenshot(OutputDir / FString::Printf(TEXT("playback_%02d.png"), CompletedRuns + 1), true, false);
        bScreenshotTaken = true;
        bSkipTimingSample = true; // Screenshot readback/write stalls are test tooling, not inference cost.
    }
    if (bAudioEnded && bSendDone)
    {
        if (!bBaseline && ReferenceFace && PeakJawBoneChange < 0.0001) { FinishTest(TEXT("no_reference_jaw_motion")); }
        else { FinishTest(bBaseline || CurveFrames > 0 ? TEXT("success") : TEXT("no_facial_curves")); }
    }
    else if (Now - SentAt > 180) { FinishTest(TEXT("timeout")); }
}

void ADCAceCompareGameMode::FinishTest(const FString& Result)
{
    bFinished = true;
    const FString Report = FString::Printf(
        TEXT("result,mode,provider,has_character,start_delay_ms,send_elapsed_ms,nonzero_curve_samples,peak_curve,mean_frame_ms,max_frame_ms,peak_face_jaw_curve,peak_jaw_rotation_delta\n")
        TEXT("%s,%s,%s,%d,%.3f,%.3f,%d,%.5f,%.3f,%.3f,%.5f,%.6f\n"),
        *Result, bBaseline ? TEXT("baseline") : TEXT("ace"), *Provider.ToString(), !CharacterPath.IsEmpty(),
        StartDelayMs, SendMs, CurveFrames, PeakCurve, FrameCount ? SumFrameMs / FrameCount : 0, MaxFrameMs, PeakFaceJawCurve, PeakJawBoneChange);
    FFileHelper::SaveStringToFile(Report, *(OutputDir / TEXT("result.csv")));
    FFileHelper::SaveStringToFile(CurveCsv, *(OutputDir / TEXT("curves.csv")));
    ++CompletedRuns;
    FFileHelper::SaveStringToFile(Report, *(OutputDir / FString::Printf(TEXT("result_%02d.csv"), CompletedRuns)));
    FFileHelper::SaveStringToFile(CurveCsv, *(OutputDir / FString::Printf(TEXT("curves_%02d.csv"), CompletedRuns)));
    UE_LOG(LogDCAceCompare, Display, TEXT("RESULT=%s Output=%s"), *Result, *OutputDir);
    if (BaselineAudio) { BaselineAudio->Stop(); }
    if (Result == TEXT("success") && CompletedRuns < RepeatCount)
    {
        bFinished = bStarted = false;
        NextRunAt = FPlatformTime::Seconds() + 1.5;
        return;
    }
    if (GEngine) { GEngine->AddOnScreenDebugMessage(711, 3600, Result == TEXT("success") ? FColor::Green : FColor::Red,
        TEXT("Test finished: ") + Result + TEXT(". See Saved/ACE/Runs. Close window to exit.")); }
    if (bAutoQuit) { FGenericPlatformMisc::RequestExitWithStatus(false, Result == TEXT("success") ? 0 : 1); }
}

void ADCAceCompareGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Avatar) { UACEBlueprintLibrary::StopCharacter(Avatar); }
    if (BaselineAudio) { BaselineAudio->Stop(); }
    Super::EndPlay(Reason);
}
