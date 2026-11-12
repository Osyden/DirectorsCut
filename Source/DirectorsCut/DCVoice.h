#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformProcess.h"
#include "AudioCaptureCore.h"
#include "DCVoice.generated.h"


class UAudioComponent;
class USoundWaveProcedural;

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

public:
	ADCVoice();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	static ADCVoice* Get(const UObject* WorldContext);

	/** Crew reply. Uses the LLM to rephrase Fallback in character if it is up, then speaks and subtitles it. */
	void CrewReply(const FString& Speaker, const FString& Fallback, const FString& Context);
	/** Speak a finished line (no LLM). */
	void Speak(const FString& Speaker, const FString& Text);
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
	UPROPERTY(Config, EditAnywhere) FString STTModel = TEXT("ggml-small.en.bin");
	UPROPERTY(Config, EditAnywhere) bool bUseLLM = true;
	/** Seconds to wait for the LLM before speaking the canned line instead. */
	UPROPERTY(Config, EditAnywhere) float LLMTimeout = 1.5f;

protected:
	FString SidecarDir() const;
	void CheckHealth();
	void LaunchSidecars();

	void StartRecording();
	void StopRecordingAndTranscribe();
	void SendToWhisper(const TArray<float>& Mono, int32 SampleRate);
	void OnHeard(const FString& Text);

	void FinishLine(const FString& Speaker, const FString& Text);
	void PlayWav(const FString& Speaker, const FString& Path);
	FString VoiceFor(const FString& Speaker) const;
	FString PersonaFor(const FString& Speaker) const;

	struct FPendingTTS
	{
		FProcHandle Proc;
		FString Speaker;
		FString WavPath;
		double StartTime = 0.0;
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

	FProcHandle LLMProc;
	FProcHandle STTProc;
	bool bLaunched = false;
	bool bLLMReady = false;
	bool bSTTReady = false;
	bool bTTSReady = false;
	float HealthTimer = 0.f;
	int32 LineCounter = 0;
	FString LastHeard;
};
