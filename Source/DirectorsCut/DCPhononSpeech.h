#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCPhononSpeech.generated.h"

class UACEAudioCurveSourceComponent;
namespace DCPhononSpeechPrivate { struct FWorker; }

/** Opt-in local Phonon PCM producer. One instance owns one persistent process.
 * The caller must exclude all legacy ACE WAV sends until IsBusy() becomes false.
 * Sent reports native input/drain completion, not the end of audible playback.
 * Actor teardown is an asynchronous fallback. World-begin-teardown drains that
 * world's workers/GT cleanup before GC; engine-pre-exit drains remaining workers
 * before ACE/module teardown. Reentrant world teardown inside a GT task fails
 * explicitly if cleanup is outstanding. A stuck native call can delay teardown
 * because the installed SDK has no safe hard abort.
 */
UCLASS()
class DIRECTORSCUT_API UDCPhononSpeech : public UActorComponent
{
    GENERATED_BODY()
public:
    UDCPhononSpeech();
    bool Initialize(const FString& PythonPath, const FString& ScriptPath,
        const FString& ModelDirectory, const FString& VoiceManifestPath);
    bool IsReady() const;
    // GT only: includes native drain and two frames after the actual GT Stop.
    bool IsBusy() const;
    bool Speak(const FString& Speaker, const FString& Text, UACEAudioCurveSourceComponent* Consumer,
        FName Provider, TFunction<void(bool, double)> Sent);
    // Cancellation suppresses Sent. Busy remains true until native drain and GT cleanup.
    void Cancel();
    // Native review only: fault admission requires -DCPhononLifecycleReview=Failure.
    // The worker terminates only its own producer; a negative applied ID means the request was too late.
    int32 RequestProducerFailureForReview();
    void GetProducerReviewState(int32& Generation, int32& AppliedFaultId) const;
    // Opaque worker-only receipt survives normal actor/world teardown without retaining a UObject.
    TFunction<bool()> CaptureReloadDrainProbeForReview() const;
    void EndPlay(const EEndPlayReason::Type Reason) override;
    void BeginDestroy() override;

private:
    TSharedPtr<DCPhononSpeechPrivate::FWorker, ESPMode::ThreadSafe> Worker;
    UPROPERTY() TObjectPtr<UACEAudioCurveSourceComponent> LastConsumer;
    TFunction<void(bool, double)> Completion;
    int32 NextId = 0, ActiveId = 0;
    bool bStopped = false;
    void Shutdown();
    void Delivered(int32 Id, bool bSuccess, double Duration);
};
