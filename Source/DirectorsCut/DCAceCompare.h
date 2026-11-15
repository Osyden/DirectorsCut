#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCAceCompare.generated.h"

class UACEAudioCurveSourceComponent;
class UAsyncActionAnimateCharacter;
class UAudioComponent;
class USoundWaveProcedural;
class USkeletalMeshComponent;

/** Isolated comparison scene. Does not start the voyage or its AI sidecars. */
UCLASS()
class ADCAceCompareGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ADCAceCompareGameMode();
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void BeginClip();
    void FinishTest(const FString& Result);
    UFUNCTION() void AnimationStarted();
    UFUNCTION() void AnimationEnded();
    UFUNCTION() void SendCompleted(bool bSuccess);

    UPROPERTY() TObjectPtr<AActor> Avatar;
    UPROPERTY() TObjectPtr<UACEAudioCurveSourceComponent> Curves;
    UPROPERTY() TObjectPtr<UAsyncActionAnimateCharacter> Request;
    UPROPERTY() TObjectPtr<UAudioComponent> BaselineAudio;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> BaselineWave;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> ReferenceFace;
    FString WavPath, OutputDir, CharacterPath;
    FName Provider = TEXT("LocalA2F-Mark-AR");
    bool bBaseline = false, bAutoQuit = false, bStarted = false, bFinished = false;
    bool bAudioStarted = false, bAudioEnded = false, bSendDone = false, bSendSucceeded = false;
    bool bScreenshotTaken = false;
    bool bSkipTimingSample = false;
    double SentAt = 0, PlaybackStartedAt = 0, StartDelayMs = -1, SendMs = -1;
    double SampleAt = 0, SumFrameMs = 0, MaxFrameMs = 0;
    int32 FrameCount = 0, CurveFrames = 0;
    int32 RepeatCount = 1, CompletedRuns = 0;
    double NextRunAt = 0;
    float Duration = 0, PeakCurve = 0;
    float PeakFaceJawCurve = 0;
    double PeakJawBoneChange = 0;
    FQuat InitialJawRotation = FQuat::Identity;
    FString CurveCsv;
};
