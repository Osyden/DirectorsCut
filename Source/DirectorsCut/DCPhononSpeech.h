#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCPhononSpeech.generated.h"

class UACEAudioCurveSourceComponent;
namespace DCPhononSpeechPrivate { struct FWorker; }

/** Opt-in local Phonon PCM producer. One instance owns one persistent process.
 * The caller must exclude all legacy ACE WAV sends until IsBusy() becomes false.
 * Sent reports native input/drain completion, not the end of audible playback.
 * Actor teardown is asynchronous; a global engine-pre-exit barrier drains owned
 * workers and GT cleanup before ACE/module teardown. A stuck native call can
 * delay application exit because the installed SDK has no safe hard abort.
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
