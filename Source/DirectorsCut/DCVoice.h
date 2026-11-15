#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformProcess.h"
#include "AudioCaptureCore.h"
#include "DCVoice.generated.h"


class UAudioComponent;
class USoundWaveProcedural;
class UACEAudioCurveSourceComponent;
class UAsyncActionAnimateCharacter;
class UDCACEDialogue;

/**
 * The voice loop: microphone -> speech-to-text -> order, and crew line -> (LLM) -> text-to-speech.
 *
 * Three local sidecar processes do the heavy work, each optional:
 *   llama-server   (LLM, OpenAI-style HTTP)   - gives crew replies their own words
 *   whisper-server (speech-to-text HTTP)      - turns push-to-talk audio into text
 *   piper          (text-to-speech, per line) - speaks crew replies
 * If one is missing the game falls back: typed/key orders, canned lines, subtitles only.
 */
UCLASS(Config = Game)
class ADCVoice : public AActor
{
	GENERATED_BODY()
	friend class ADCDialogueReviewGameMode;
	friend class UDCACEIntegrationReview;

public:
	ADCVoice();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	static ADCVoice* Get(const UObject* WorldContext);

	/** Reports (empty Context) use exact authored wording; order acknowledgements can use the LLM. */
	void CrewReply(const FString& Speaker, const FString& Fallback, const FString& Context);
	/** Speak a finished line (no LLM). */
	void Speak(const FString& Speaker, const FString& Text, bool bReport = false);
	/** Drop dialogue belonging to the previous scene/menu without restarting sidecars. */
	void ResetPendingSpeech();
	bool IsSpeaking() const;
	const FString& GetSpeakingSpeaker() const { return SpeakingSpeaker; }
	const FString& GetSpeakingText() const { return SpeakingText; }
	float GetSpeechRemainingSeconds() const;
	/** Asks the language model to make sense of an order the keyword grammar could not, then carries it out. */
	void ResolveOrder(const FString& Text);
	/** Transcribe a wav file from disk as if it had been spoken. For unattended tests. */
	void TranscribeWavFile(const FString& Path);

	bool IsListening() const { return bRecording; }
	bool IsLLMReady() const { return bLLMReady; }
	bool IsSTTReady() const { return bSTTReady; }
	bool IsTTSReady() const { return bTTSReady; }
	const FString& GetLastHeard() const { return LastHeard; }

	UPROPERTY(Config, EditAnywhere) int32 LLMPort = 8791;
	UPROPERTY(Config, EditAnywhere) int32 STTPort = 8792;
	UPROPERTY(Config, EditAnywhere) FString LLMModel = TEXT("Qwen3.5-4B-Q5_K_M.gguf");
	UPROPERTY(Config, EditAnywhere) FString STTModel = TEXT("ggml-base.en.bin");
	UPROPERTY(Config, EditAnywhere) bool bUseLLM = true;
	/** LlamaCpp (external server) or ACE (local NVIDIA runtime). Selected per launch for comparison. */
	UPROPERTY(Config, EditAnywhere) FString DialogueBackend = TEXT("LlamaCpp");
	/** Whisper vocabulary/context hint, not a strict grammar or guaranteed whitelist. */
	UPROPERTY(Config, EditAnywhere) FString STTVocabulary = TEXT("Rex, Duncan, Ruby. Rex, take the helm. Duncan, fire at will. Ruby, put out the fire. Hard to port. Hard to starboard. Steady as she goes. Man the cannons. Hold fire. Repair the rigging. Report.");
	/** Seconds to wait for the LLM before speaking the canned line instead. */
	UPROPERTY(Config, EditAnywhere) float LLMTimeout = 1.5f;
	/** Uses a separate CUDA context; shared graphics contexts remain blocked. */
	UPROPERTY(Config, EditAnywhere) bool bUseACE = true;
	UPROPERTY(Config, EditAnywhere) FName ACEProvider = TEXT("LocalA2F-Mark-AR");
	UPROPERTY(Config, EditAnywhere) FString FinchVoice = TEXT("en_GB-vctk-medium.onnx");
	UPROPERTY(Config, EditAnywhere) int32 FinchSpeakerId = 95; // VCTK p226
	UPROPERTY(Config, EditAnywhere) FString MarlowVoice = TEXT("en_GB-northern_english_male-medium.onnx");
	UPROPERTY(Config, EditAnywhere) FString OdessaVoice = TEXT("en_GB-alba-medium.onnx");
	UPROPERTY(Config, EditAnywhere) float FinchLengthScale = 1.04f;
	UPROPERTY(Config, EditAnywhere) float MarlowLengthScale = .95f;
	UPROPERTY(Config, EditAnywhere) float OdessaLengthScale = 1.f;

protected:
	void InitializeDialogueBackend();
	void RequestDialogue(const FString& System, const FString& User, int32 MaxTokens, float Temperature,
		bool bJson, float Timeout, TFunction<void(bool, const FString&)> Completion);
	UPROPERTY(Transient) TObjectPtr<UDCACEDialogue> ACEDialogue;
	bool bUseACEDialogue = false;
	FString SidecarDir() const;
	void CheckHealth();
	void LaunchSidecars();

	void StartRecording();
	void StopRecordingAndTranscribe();
	void SendToWhisper(const TArray<float>& Mono, int32 SampleRate);
	void OnHeard(const FString& Text);

	void FinishLine(const FString& Speaker, const FString& Text, bool bReport = false);
	void SynchronizeScene();
	bool IsMenuOpen() const;
	void ShowSpeech(const FString& Speaker, const FString& Text, double Duration);
	void PlayWav(const FString& Speaker, const FString& Path);
	void StartNextLine();
	bool TryACE(const FString& Speaker, const FString& Path, double Duration);
	void ClearACE();
	UFUNCTION() void ACEStarted();
	UFUNCTION() void ACEEnded();
	UFUNCTION() void ACESent(bool bSuccess);
	struct FQueuedLine { FString Speaker; FString Text; double QueuedAt; bool bReport = false; };
	TArray<FQueuedLine> SpeechQueue;
	UPROPERTY() TObjectPtr<UACEAudioCurveSourceComponent> ActiveCurves;
	UPROPERTY() TObjectPtr<UAsyncActionAnimateCharacter> ActiveACERequest;
	FString ActiveSpeaker, ActiveWav;
	FString ActiveText, SpeakingSpeaker, SpeakingText;
	double ActiveDuration = 0, SpeechVisibleUntil = 0;
	int32 SpeechEpoch = 0, SceneKey = INDEX_NONE;
	float LastRideTime = -1.f;
	double ACEDeadline = 0;
	bool bACEStarted = false;
	bool bACESent = false;
	bool bACEEnded = false;
	bool bEnding = false;
	bool bSmokeTest = false;
	bool bSmokeStarted = false;
	TSet<FString> SmokeCompleted;
	int32 SmokeRepeats = 1;
	int32 SmokeCycle = 0;
	FString VoiceFor(const FString& Speaker) const;
	FString PersonaFor(const FString& Speaker) const;

	struct FPendingTTS
	{
		FProcHandle Proc;
		FString Speaker;
		FString Text;
		FString WavPath;
		double StartTime = 0.0;
		int32 Epoch = 0;
	};
	TArray<FPendingTTS> PendingTTS;

	struct FPlaying
	{
		TWeakObjectPtr<UAudioComponent> Component;
		double EndTime = 0.0;
	};
	TArray<FPlaying> Playing;
	UPROPERTY() TArray<TObjectPtr<USoundWaveProcedural>> LiveWaves;

	TUniquePtr<Audio::FAudioCapture> Capture;
	FCriticalSection CaptureLock;
	TArray<float> CaptureBuffer;
	int32 CaptureRate = 48000;
	TAtomic<bool> bRecording{ false };
	bool bStreamOpen = false;

	// Shared across level reloads (R to sail again) so the models stay loaded; stopped only when the game quits.
	static FProcHandle LLMProc;
	static FProcHandle STTProc;
	bool bLaunched = false;
	bool bLLMReady = false;
	bool bSTTReady = false;
	bool bTTSReady = false;
	float HealthTimer = 0.f;
	int32 LineCounter = 0;
	FString LastHeard;
};
