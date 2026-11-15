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
class IHttpRequest;
class UDCACESpeechCallback;
class UDCPhononSpeech;

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
	friend class UDCConversationReview;
	friend class UDCACESpeechCallback;
	friend class FDCStreamedSpeechLifecycleTest;

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
	/** Returns true when typed/voiced input is handled as conversation instead of an order. */
	bool TryConversationInput(const FString& Text);
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
	/** Experimental persistent CPU Phonon31M PCM producer; Piper remains the default. */
	UPROPERTY(Config, EditAnywhere) FString TTSBackend = TEXT("Piper");
	/** Whisper vocabulary/context hint, not a strict grammar or guaranteed whitelist. */
	UPROPERTY(Config, EditAnywhere) FString STTVocabulary = TEXT("Rex, Duncan, Ruby. Rex, take the helm. Duncan, fire at will. Ruby, put out the fire. Hard to port. Hard to starboard. Steady as she goes. Man the cannons. Hold fire. Repair the rigging. Report.");
	/** Seconds to wait for the LLM before speaking the canned line instead. */
	UPROPERTY(Config, EditAnywhere) float LLMTimeout = 1.5f;
	/** Uses a separate CUDA context; shared graphics contexts remain blocked. */
	UPROPERTY(Config, EditAnywhere) bool bUseACE = true;
	/** Isolated candidate: text chunks, one-ahead synthesis and bounded A2F prebuffer. */
	UPROPERTY(Config, EditAnywhere) bool bStreamSpeech = false;
	UPROPERTY(Config, EditAnywhere) float StreamPrebufferSeconds = 1.f;
	UPROPERTY(Config, EditAnywhere) FName ACEProvider = TEXT("LocalA2F-Mark-AR");
	UPROPERTY(Config, EditAnywhere) FString FinchVoice = TEXT("en_GB-vctk-medium.onnx");
	UPROPERTY(Config, EditAnywhere) int32 FinchSpeakerId = 95; // VCTK p226
	UPROPERTY(Config, EditAnywhere) FString MarlowVoice = TEXT("en_GB-northern_english_male-medium.onnx");
	UPROPERTY(Config, EditAnywhere) FString OdessaVoice = TEXT("en_GB-alba-medium.onnx");
	UPROPERTY(Config, EditAnywhere) float FinchLengthScale = 1.04f;
	UPROPERTY(Config, EditAnywhere) float MarlowLengthScale = .95f;
	UPROPERTY(Config, EditAnywhere) float OdessaLengthScale = 1.f;

protected:
	struct FConversationQuestion { FString Speaker, Text; double QueuedAt = 0; };
	struct FConversationTurn { FString User, Reply; };
	TArray<FConversationQuestion> ConversationQueue;
	TMap<FString, TArray<FConversationTurn>> ConversationHistory;
	FString LastConversationSpeaker;
	bool bConversationPending = false;
	int32 ConversationSerial = 0;
	void QueueConversation(const FString& Text, bool bShowCaptain);
	void StartNextConversation();
	void ResetConversationState();
	void InitializeDialogueBackend();
	void RequestDialogue(const FString& System, const FString& User, int32 MaxTokens, float Temperature,
		bool bJson, float Timeout, TFunction<void(bool, const FString&)> Completion,
		TFunction<void(const FString&)> Progress = nullptr);
	TArray<TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>> StreamingDialogueRequests;
	void CancelStreamingDialogueRequests();
	UPROPERTY(Transient) TObjectPtr<UDCACEDialogue> ACEDialogue;
	bool bUseACEDialogue = false;
	FString SidecarDir() const;
	void CheckHealth();
	void LaunchSidecars(bool bAllowLLM = false);

	void StartRecording();
	void StopRecordingAndTranscribe();
	void SendToWhisper(const TArray<float>& Mono, int32 SampleRate);
	void OnHeard(const FString& Text);

	void FinishLine(const FString& Speaker, const FString& Text, bool bReport = false);
	void SynchronizeScene();
	bool IsMenuOpen() const;
	void ShowSpeech(const FString& Speaker, const FString& Text, double Duration);
	void PlayWav(const FString& Speaker, const FString& Path, int32 BatchId = 0);
	void StartNextLine();
	void InitializePhonon();
	bool TryPhonon(const FString& Speaker, const FString& Text, int32 BatchId);
	UPROPERTY(Transient) TObjectPtr<UDCPhononSpeech> PhononSpeech;
	bool bUsePhonon = false, bActivePhonon = false;
	FString PhononReviewMode;
	int32 PhononReviewStage = 0;
	double PhononReviewStarted = 0;
	bool bPhononReviewSawFreshPlayback = false;
	bool TickPhononLifecycleReview();
	void StartReadySpeech();
	void QueueSynthesisFailure(const FString& Speaker, const FString& Text, int32 BatchId);
	bool TryACE(const FString& Speaker, const FString& Path, double Duration);
	void ClearACE();
	UFUNCTION() void ACEStarted();
	UFUNCTION() void ACEEnded();
	UFUNCTION() void ACESent(bool bSuccess);
	struct FQueuedLine { FString Speaker; FString Text; double QueuedAt; bool bReport = false; int32 BatchId = 0; };
	TArray<FQueuedLine> SpeechQueue;
	struct FReadySpeech { FString Speaker, Text, Path; double Duration = 0; int32 Epoch = 0, BatchId = 0; };
	TArray<FReadySpeech> ReadySpeech;
	double SilentSpeechUntil = 0;
	struct FSpeechBatch
	{
		int32 Id = 0, Queued = 0, Completed = 0, Faces = 0;
		FString Speaker, Text, User;
		bool bFinalized = false, bFailed = false;
	};
	FSpeechBatch StreamingBatch, LastFinishedBatch;
	int32 NextSpeechBatchId = 0, ActiveSpeechBatchId = 0;
	int32 BeginSpeechBatch(const FString& Speaker);
	void QueueSpeechChunk(int32 BatchId, const FString& Text);
	void FinalizeSpeechBatch(int32 BatchId, const FString& Text, bool bSuccess);
	void CompleteSpeechChunk(int32 BatchId, bool bSuccess, bool bFace);
	void TryFinishSpeechBatch();
	UPROPERTY() TObjectPtr<UACEAudioCurveSourceComponent> ActiveCurves;
	UPROPERTY() TObjectPtr<UAsyncActionAnimateCharacter> ActiveACERequest;
	UPROPERTY() TObjectPtr<UDCACESpeechCallback> ActiveACERelay;
	int32 ACERequestSerial = 0;
	uint64 EarliestSpeechStartFrame = 0;
	FString ActiveSpeaker, ActiveWav;
	FString ActiveText, SpeakingSpeaker, SpeakingText;
	double ActiveDuration = 0, SpeechVisibleUntil = 0;
	int32 SpeechEpoch = 0, SceneKey = INDEX_NONE;
	float LastRideTime = -1.f;
	double ACEDeadline = 0;
	bool bACEStarted = false;
	bool bACESent = false;
	bool bACESendSucceeded = false;
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
		int32 BatchId = 0;
	};
	TArray<FPendingTTS> PendingTTS;

	struct FPlaying
	{
		TWeakObjectPtr<UAudioComponent> Component;
		double EndTime = 0.0;
		int32 BatchId = 0;
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
	bool bLLMLaunched = false, bSTTLaunched = false, bModelMismatchReported = false;
	bool bLLMReady = false;
	bool bSTTReady = false;
	bool bTTSReady = false;
	float HealthTimer = 0.f;
	int32 LineCounter = 0;
	FString LastHeard;
};
