#include "DCVoice.h"
#include "DCSpeech.h"
#include "DCACEDialogue.h"
#include "DCLanguageModelResponse.h"
#include "DCDialoguePrompts.h"
#include "ACEAudioCurveSourceComponent.h"
#include "ACEBlueprintLibrary.h"
#include "AsyncActionAnimateCharacter.h"
#include "Components/ChildActorComponent.h"
#include "Audio.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "DCTypes.h"
#include "DCCrew.h"
#include "DCRideDirector.h"
#include "DCGame.h"
#include "AudioCaptureCore.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Modules/ModuleManager.h"

namespace
{
	void AppendStr(TArray<uint8>& Out, const FString& S)
	{
		FTCHARToUTF8 Utf8(*S);
		Out.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	}

	/** 16-bit mono WAV from float samples. */
	TArray<uint8> MakeWav(const TArray<float>& Mono, int32 SampleRate)
	{
		TArray<uint8> Out;
		const int32 DataBytes = Mono.Num() * 2;
		auto W32 = [&Out](uint32 V) { Out.Append(reinterpret_cast<const uint8*>(&V), 4); };
		auto W16 = [&Out](uint16 V) { Out.Append(reinterpret_cast<const uint8*>(&V), 2); };
		Out.Append(reinterpret_cast<const uint8*>("RIFF"), 4); W32(36 + DataBytes);
		Out.Append(reinterpret_cast<const uint8*>("WAVEfmt "), 8); W32(16); W16(1); W16(1);
		W32(SampleRate); W32(SampleRate * 2); W16(2); W16(16);
		Out.Append(reinterpret_cast<const uint8*>("data"), 4); W32(DataBytes);
		Out.Reserve(Out.Num() + DataBytes);
		for (float S : Mono)
		{
			W16(static_cast<uint16>(static_cast<int16>(FMath::Clamp(S, -1.f, 1.f) * 32767.f)));
		}
		return Out;
	}

	TArray<float> Resample(const TArray<float>& In, int32 InRate, int32 OutRate)
	{
		if (InRate == OutRate || In.Num() < 2) { return In; }
		const double Step = static_cast<double>(InRate) / OutRate;
		const int32 N = FMath::FloorToInt32((In.Num() - 1) / Step);
		TArray<float> Out;
		Out.SetNumUninitialized(N);
		for (int32 i = 0; i < N; ++i)
		{
			const double P = i * Step;
			const int32 I0 = static_cast<int32>(P);
			const float F = static_cast<float>(P - I0);
			Out[i] = FMath::Lerp(In[I0], In[I0 + 1], F);
		}
		return Out;
	}

	/** Minimal WAV reader: 16-bit PCM only. */
	bool ReadWav(const FString& Path, TArray<uint8>& OutPcm, int32& OutRate, int32& OutChannels)
	{
		TArray<uint8> Bytes;
		FWaveModInfo Info;
		if (!FFileHelper::LoadFileToArray(Bytes, *Path) || !Info.ReadWaveInfo(Bytes.GetData(), Bytes.Num()) ||
			!Info.pFormatTag || *Info.pFormatTag != 1 || !Info.pBitsPerSample || *Info.pBitsPerSample != 16 ||
			!Info.pChannels || *Info.pChannels < 1 || *Info.pChannels > 2 ||
			!Info.pSamplesPerSec || *Info.pSamplesPerSec < 8000 || *Info.pSamplesPerSec > 192000) { return false; }
		OutChannels = *Info.pChannels;
		OutRate = *Info.pSamplesPerSec;
		OutPcm.Append(Info.SampleDataStart, Info.SampleDataSize);
		return OutPcm.Num() > 0 && OutPcm.Num() % (2 * OutChannels) == 0;
	}
}

FProcHandle ADCVoice::LLMProc;
FProcHandle ADCVoice::STTProc;

ADCVoice::ADCVoice()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ADCVoice* ADCVoice::Get(const UObject* WorldContext)
{
	if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCVoice> It(World); It; ++It) { return *It; }
	}
	return nullptr;
}

FString ADCVoice::SidecarDir() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Sidecars"));
}

bool ADCVoice::IsMenuOpen() const
{
	const auto* Game = GetWorld() ? GetWorld()->GetAuthGameMode<ADCGameMode>() : nullptr;
	return Game && Game->IsAwaitingStart();
}

bool ADCVoice::IsSpeaking() const
{
	return (ActiveCurves && bACEStarted && !bACEEnded) || !Playing.IsEmpty();
}

float ADCVoice::GetSpeechRemainingSeconds() const
{
	return float(FMath::Max(0.0, SpeechVisibleUntil - FPlatformTime::Seconds()));
}

void ADCVoice::ShowSpeech(const FString& Speaker, const FString& Text, double Duration)
{
	SpeakingSpeaker = Speaker;
	SpeakingText = Text;
	SpeechVisibleUntil = FPlatformTime::Seconds() + FMath::Max(3.0, Duration + .7);
}

void ADCVoice::ResetPendingSpeech()
{
	if (ACEDialogue) { ACEDialogue->CancelPending(); }
	++SpeechEpoch;
	SpeechQueue.Empty();
	ClearACE();
	for (FPendingTTS& Pending : PendingTTS)
	{
		if (FPlatformProcess::IsProcRunning(Pending.Proc)) { FPlatformProcess::TerminateProc(Pending.Proc, true); }
		FPlatformProcess::CloseProc(Pending.Proc);
	}
	PendingTTS.Empty();
	for (FPlaying& Voice : Playing) { if (Voice.Component.IsValid()) { Voice.Component->Stop(); Voice.Component->DestroyComponent(); } }
	Playing.Empty(); LiveWaves.Empty();
	SpeakingSpeaker.Empty(); SpeakingText.Empty(); SpeechVisibleUntil = 0;
	if (GetWorld())
	{
		if (auto* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->RecentLines.Empty(); }
	}
}

void ADCVoice::SynchronizeScene()
{
	const auto* Director = ADCRideDirector::Get(this);
	if (!Director) { return; }
	const int32 Key = int32(Director->GetPhase()) * 2 + (Director->GetPhase() == EDCPhase::Fog && Director->RideTime >= 76.f ? 1 : 0);
	if ((SceneKey != INDEX_NONE && Key != SceneKey) || (LastRideTime >= 0.f && Director->RideTime < LastRideTime - .1f))
	{
		ResetPendingSpeech();
		UE_LOG(LogDC, Log, TEXT("Voice: cleared previous-scene dialogue at ride %.1f"), Director->RideTime);
	}
	SceneKey = Key; LastRideTime = Director->RideTime;
}

void ADCVoice::BeginPlay()
{
	Super::BeginPlay();
	// ACE 2.5's shared D3D12/CUDA context creation stalled the UE 5.8 voyage's
	// graphics and compute queues on Blackwell. Use its supported separate-context
	// path only, and refuse the shared-context command-line override below.
	int32 CIGOverride = 0;
	if (!FParse::Value(FCommandLine::Get(), TEXT("useCIGforAI="), CIGOverride))
	{
		FCommandLine::Append(TEXT(" -useCIGforAI=0"));
	}
	UACEBlueprintLibrary::OverrideA2F3DInferenceMode(false);
	if (FParse::Param(FCommandLine::Get(), TEXT("DCEnableACE"))) { bUseACE = true; }
	if (FParse::Param(FCommandLine::Get(), TEXT("DCNoACE"))) { bUseACE = false; }
	if (CIGOverride != 0)
	{
		UE_LOG(LogDC, Warning, TEXT("Voice: refusing live ACE with shared CUDA/graphics context after the 15 Nov GPU hang"));
		bUseACE = false;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("DCNoLLM"))) { bUseLLM = false; }
	bSmokeTest = FParse::Param(FCommandLine::Get(), TEXT("DCVoiceSmokeTest"));
	FParse::Value(FCommandLine::Get(), TEXT("DCVoiceSmokeRepeats="), SmokeRepeats);
	SmokeRepeats = FMath::Clamp(SmokeRepeats, 1, 10);
	if (bSmokeTest) { bUseLLM = false; }
	InitializeDialogueBackend();
	if (bUseACE && (!ACEProvider.ToString().StartsWith(TEXT("LocalA2F-")) ||
		!UACEBlueprintLibrary::GetAvailableA2FProviderNames().Contains(ACEProvider)))
	{
		UE_LOG(LogDC, Warning, TEXT("Voice: local ACE provider unavailable; using audio-only playback"));
		bUseACE = false;
	}
	const FString Dir = SidecarDir();
	bTTSReady = FPaths::FileExists(Dir / TEXT("bin/piper/piper/piper.exe"));
	UE_LOG(LogDC, Log, TEXT("Voice: sidecars at %s, piper %s"), *Dir, bTTSReady ? TEXT("found") : TEXT("missing"));

	// The capture backends register when this plugin module loads.
	FModuleManager::Get().LoadModule(TEXT("AudioCapture"));
	{
		Audio::FAudioCapture Probe;
		Audio::FCaptureDeviceInfo Info;
		if (Probe.GetCaptureDeviceInfo(Info)) { UE_LOG(LogDC, Log, TEXT("Voice: microphone \"%s\" %d Hz %d ch"), *Info.DeviceName, Info.PreferredSampleRate, Info.InputChannels); }
		else { UE_LOG(LogDC, Warning, TEXT("Voice: no microphone detected; use typed or key orders")); }
	}
	CheckHealth();
}

void ADCVoice::EndPlay(const EEndPlayReason::Type Reason)
{
	bEnding = true;
	SpeechQueue.Empty();
	ClearACE();
	for (FPendingTTS& Pending : PendingTTS)
	{
		if (FPlatformProcess::IsProcRunning(Pending.Proc)) { FPlatformProcess::TerminateProc(Pending.Proc, true); }
		FPlatformProcess::CloseProc(Pending.Proc);
	}
	PendingTTS.Empty();
	for (FPlaying& Voice : Playing) { if (Voice.Component.IsValid()) { Voice.Component->Stop(); Voice.Component->DestroyComponent(); } }
	Playing.Empty();
	LiveWaves.Empty();
	if (Capture.IsValid())
	{
		bRecording = false;
		Capture->AbortStream();
		Capture->CloseStream();
		Capture.Reset();
	}
	// Only stop servers this run started; ones already running (dev) are left alone.
	if (Reason == EEndPlayReason::Quit || Reason == EEndPlayReason::EndPlayInEditor)
	{
		if (LLMProc.IsValid()) { FPlatformProcess::TerminateProc(LLMProc, true); FPlatformProcess::CloseProc(LLMProc); LLMProc.Reset(); }
		if (STTProc.IsValid()) { FPlatformProcess::TerminateProc(STTProc, true); FPlatformProcess::CloseProc(STTProc); STTProc.Reset(); }
	}
	Super::EndPlay(Reason);
}

void ADCVoice::CheckHealth()
{
	TWeakObjectPtr<ADCVoice> Weak(this);

	if (bUseACEDialogue && bUseLLM)
	{
		const bool bReady = ACEDialogue && ACEDialogue->IsLLMReady();
		if (bReady && !bLLMReady) { UE_LOG(LogDC, Display, TEXT("DIALOGUE_READY backend=ACE")); }
		bLLMReady = bReady;
	}
	else if (!bLLMReady && bUseLLM)
	{
		auto Req = FHttpModule::Get().CreateRequest();
		Req->SetURL(FString::Printf(TEXT("http://127.0.0.1:%d/health"), LLMPort));
		Req->SetVerb(TEXT("GET"));
		Req->SetTimeout(2.f);
		Req->OnProcessRequestComplete().BindLambda([Weak](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
		{
			if (ADCVoice* V = Weak.Get())
			{
				const bool bUp = bOk && Resp.IsValid() && Resp->GetResponseCode() == 200;
				if (bUp && !V->bLLMReady) { UE_LOG(LogDC, Log, TEXT("Voice: LLM ready")); }
				V->bLLMReady = bUp;
				if (!bUp) { V->LaunchSidecars(); }
			}
		});
		Req->ProcessRequest();
	}

	if (!bSTTReady && !FParse::Param(FCommandLine::Get(), TEXT("DCDialogueProbe")))
	{
		auto Req = FHttpModule::Get().CreateRequest();
		Req->SetURL(FString::Printf(TEXT("http://127.0.0.1:%d/"), STTPort));
		Req->SetVerb(TEXT("GET"));
		Req->SetTimeout(2.f);
		Req->OnProcessRequestComplete().BindLambda([Weak](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
		{
			if (ADCVoice* V = Weak.Get())
			{
				const bool bUp = bOk && Resp.IsValid() && Resp->GetResponseCode() == 200;
				if (bUp && !V->bSTTReady) { UE_LOG(LogDC, Log, TEXT("Voice: speech-to-text ready")); }
				V->bSTTReady = bUp;
				if (!bUp) { V->LaunchSidecars(); }
			}
		});
		Req->ProcessRequest();
	}
}

void ADCVoice::LaunchSidecars()
{
	if (bLaunched) { return; }
	bLaunched = true;
	const FString Dir = SidecarDir();

	const FString LLMExe = Dir / TEXT("bin/llama/llama-server.exe");
	const FString LLMModelPath = Dir / TEXT("models") / LLMModel;
	if (bUseLLM && !bUseACEDialogue && !bLLMReady && FPaths::FileExists(LLMExe) && FPaths::FileExists(LLMModelPath))
	{
		const FString Args = FString::Printf(TEXT("-m \"%s\" --host 127.0.0.1 --port %d -ngl 99 -c 4096"), *LLMModelPath, LLMPort);
		LLMProc = FPlatformProcess::CreateProc(*LLMExe, *Args, false, true, true, nullptr, 0, *(Dir / TEXT("bin/llama")), nullptr);
		UE_LOG(LogDC, Log, TEXT("Voice: started llama-server (%s)"), LLMProc.IsValid() ? TEXT("ok") : TEXT("failed"));
	}

	const FString STTExe = Dir / TEXT("bin/whisper/Release/whisper-server.exe");
	const FString STTModelPath = Dir / TEXT("models") / STTModel;
	if (!bSTTReady && !FParse::Param(FCommandLine::Get(), TEXT("DCDialogueProbe")) && FPaths::FileExists(STTExe) && FPaths::FileExists(STTModelPath))
	{
		const FString Args = FString::Printf(TEXT("-m \"%s\" --host 127.0.0.1 --port %d -t 8"), *STTModelPath, STTPort);
		STTProc = FPlatformProcess::CreateProc(*STTExe, *Args, false, true, true, nullptr, 0, *(Dir / TEXT("bin/whisper/Release")), nullptr);
		UE_LOG(LogDC, Log, TEXT("Voice: started whisper-server (%s)"), STTProc.IsValid() ? TEXT("ok") : TEXT("failed"));
	}
}

// ---------------------------------------------------------------- Tick

void ADCVoice::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SynchronizeScene();

	if (!bLLMReady || !bSTTReady)
	{
		HealthTimer -= DeltaSeconds;
		if (HealthTimer <= 0.f)
		{
			HealthTimer = 2.f;
			CheckHealth();
		}
	}

	if (IsMenuOpen()) { return; }
	// Push-to-talk.
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		const bool bHeld = PC->IsInputKeyDown(EKeys::V) || PC->IsInputKeyDown(EKeys::MiddleMouseButton);
		if (bHeld && !bRecording) { StartRecording(); }
		else if (!bHeld && bRecording) { StopRecordingAndTranscribe(); }
	}

	// Finished speech synthesis -> play.
	for (int32 i = PendingTTS.Num() - 1; i >= 0; --i)
	{
		FPendingTTS& P = PendingTTS[i];
		const bool bDone = !FPlatformProcess::IsProcRunning(P.Proc);
		const bool bTimedOut = FPlatformTime::Seconds() - P.StartTime > 8.0;
		if (bDone || bTimedOut)
		{
			if (bTimedOut && !bDone) { FPlatformProcess::TerminateProc(P.Proc, true); }
			FPlatformProcess::CloseProc(P.Proc);
			if (P.Epoch == SpeechEpoch)
			{
				ActiveText = P.Text;
				TArray<uint8> Pcm; int32 Rate = 0, Channels = 0;
				if (bDone && ReadWav(P.WavPath, Pcm, Rate, Channels) && Pcm.Num() > 0 && Rate > 0 && Channels > 0)
				{
					if (!TryACE(P.Speaker, P.WavPath, double(Pcm.Num()) / (2.0 * Rate * Channels))) { PlayWav(P.Speaker, P.WavPath); }
				}
				else
				{
					// Keep the current line readable even if Piper fails. This does not
					// mark speech active, so the ambience is not ducked for silent text.
					ShowSpeech(P.Speaker, P.Text, FMath::Clamp(P.Text.Len() * .055, 4.0, 9.0));
					UE_LOG(LogDC, Warning, TEXT("Voice: synthesis failed or timed out for %s; displaying subtitle"), *P.Speaker);
				}
			}
			PendingTTS.RemoveAt(i);
		}
	}

	// Retire finished voices.
	const double Now = FPlatformTime::Seconds();
	for (int32 i = Playing.Num() - 1; i >= 0; --i)
	{
		if (Now > Playing[i].EndTime)
		{
			if (UAudioComponent* C = Playing[i].Component.Get())
			{
				LiveWaves.Remove(Cast<USoundWaveProcedural>(C->Sound));
				C->Stop();
				C->DestroyComponent();
			}
			Playing.RemoveAt(i);
		}
	}
	if (ActiveCurves && ((bACESent && bACEEnded) || Now > ACEDeadline))
	{
		const bool bFallback = !bACEStarted;
		const FString Speaker = ActiveSpeaker, Path = ActiveWav;
		if (Now > ACEDeadline) { UE_LOG(LogDC, Warning, TEXT("Voice: ACE playback timed out; disabling ACE for this run")); bUseACE = false; }
		ClearACE();
		if (bFallback) { PlayWav(Speaker, Path); }
	}
	if (PendingTTS.IsEmpty() && Playing.IsEmpty() && !ActiveCurves) { StartNextLine(); }
	if (bSmokeTest)
	{
		if (!bSmokeStarted && GetWorld()->GetTimeSeconds() > 5)
		{
			bSmokeStarted = true;
			Speak(TEXT("Rex"), TEXT("Aye, Captain. Bringing her hard to starboard."));
			Speak(TEXT("Duncan"), TEXT("Port guns ready. Waiting for your order."));
			Speak(TEXT("Ruby"), TEXT("The fire is out. Checking the rigging now."));
		}
		if (bSmokeStarted && SmokeCompleted.Num() == 3 && PendingTTS.IsEmpty() && Playing.IsEmpty() && !ActiveCurves && SpeechQueue.IsEmpty())
		{
			++SmokeCycle;
			UE_LOG(LogDC, Display, TEXT("VOICE_SMOKE_CYCLE mode=%s cycle=%d completed=%d"), bUseACE ? TEXT("ACE") : TEXT("audio"), SmokeCycle, SmokeCompleted.Num());
			if (SmokeCycle >= SmokeRepeats)
			{
				const bool bExpectedACE = (GetDefault<ADCVoice>()->bUseACE || FParse::Param(FCommandLine::Get(), TEXT("DCEnableACE"))) && !FParse::Param(FCommandLine::Get(), TEXT("DCNoACE"));
				const bool bSuccess = bUseACE == bExpectedACE;
				UE_LOG(LogDC, Display, TEXT("VOICE_SMOKE_%s mode=%s cycles=%d"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"), bUseACE ? TEXT("ACE") : TEXT("audio"), SmokeCycle);
				FGenericPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
			}
			else { bSmokeStarted = false; SmokeCompleted.Reset(); }
		}
		else if ((bSmokeStarted && SpeechQueue.IsEmpty() && PendingTTS.IsEmpty() && Playing.IsEmpty() && !ActiveCurves)
			|| GetWorld()->GetTimeSeconds() > 45 + 30 * SmokeRepeats)
		{
			UE_LOG(LogDC, Error, TEXT("VOICE_SMOKE_FAILED completed=%d"), SmokeCompleted.Num());
			FGenericPlatformMisc::RequestExitWithStatus(false, 1);
		}
	}
}

// ---------------------------------------------------------------- Speech to text

void ADCVoice::StartRecording()
{
	if (!Capture.IsValid())
	{
		Capture = MakeUnique<Audio::FAudioCapture>();
	}
	if (!bStreamOpen)
	{
		Audio::FCaptureDeviceInfo Info;
		if (!Capture->GetCaptureDeviceInfo(Info))
		{
			UE_LOG(LogDC, Warning, TEXT("Voice: no microphone found"));
			return;
		}
		CaptureRate = Info.PreferredSampleRate;
		Audio::FAudioCaptureDeviceParams Params;
		Params.PCMAudioEncoding = Audio::EPCMAudioEncoding::FLOATING_POINT_32;
		Audio::FOnAudioCaptureFunction OnCapture = [this](const void* InAudio, int32 NumFrames, int32 NumChannels, int32 SampleRate, double, bool)
		{
			if (!bRecording) { return; }
			const float* Samples = static_cast<const float*>(InAudio);
			FScopeLock Lock(&CaptureLock);
			CaptureRate = SampleRate;
			const int32 Start = CaptureBuffer.AddUninitialized(NumFrames);
			for (int32 f = 0; f < NumFrames; ++f)
			{
				float Sum = 0.f;
				for (int32 c = 0; c < NumChannels; ++c) { Sum += Samples[f * NumChannels + c]; }
				CaptureBuffer[Start + f] = Sum / FMath::Max(1, NumChannels);
			}
		};
		if (!Capture->OpenAudioCaptureStream(Params, MoveTemp(OnCapture), 1024) || !Capture->StartStream())
		{
			UE_LOG(LogDC, Warning, TEXT("Voice: could not open the microphone"));
			return;
		}
		bStreamOpen = true;
		UE_LOG(LogDC, Log, TEXT("Voice: microphone open (%s, %d Hz)"), *Info.DeviceName, CaptureRate);
	}
	{
		FScopeLock Lock(&CaptureLock);
		CaptureBuffer.Reset();
	}
	bRecording = true;
}

void ADCVoice::StopRecordingAndTranscribe()
{
	bRecording = false;
	TArray<float> Samples;
	int32 Rate = 0;
	{
		FScopeLock Lock(&CaptureLock);
		Samples = MoveTemp(CaptureBuffer);
		CaptureBuffer.Reset();
		Rate = CaptureRate;
	}
	if (Samples.Num() < Rate / 4)
	{
		return; // Under a quarter second: a stray key tap.
	}
	SendToWhisper(Samples, Rate);
}

void ADCVoice::TranscribeWavFile(const FString& Path)
{
	TArray<uint8> Pcm;
	int32 Rate = 0, Channels = 0;
	if (!ReadWav(Path, Pcm, Rate, Channels))
	{
		UE_LOG(LogDC, Warning, TEXT("Voice: cannot read wav %s"), *Path);
		return;
	}
	const int16* S = reinterpret_cast<const int16*>(Pcm.GetData());
	const int32 Frames = Pcm.Num() / 2 / FMath::Max(1, Channels);
	TArray<float> Mono;
	Mono.SetNumUninitialized(Frames);
	for (int32 i = 0; i < Frames; ++i) { Mono[i] = S[i * Channels] / 32768.f; }
	SendToWhisper(Mono, Rate);
}

void ADCVoice::SendToWhisper(const TArray<float>& Mono, int32 SampleRate)
{
	const TArray<uint8> Wav = MakeWav(Resample(Mono, SampleRate, 16000), 16000);
	FFileHelper::SaveArrayToFile(Wav, *(FPaths::ProjectSavedDir() / TEXT("TTS/last_order.wav")));

	const FString Boundary = TEXT("----DCBoundary7MA4YWxk");
	TArray<uint8> Body;
	auto Field = [&](const FString& Name, const FString& Value)
	{
		AppendStr(Body, FString::Printf(TEXT("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n"), *Boundary, *Name, *Value));
	};
	Field(TEXT("response_format"), TEXT("json"));
	Field(TEXT("temperature"), TEXT("0.0"));
	// Bias the recogniser toward the words the crew understands.
	Field(TEXT("prompt"), STTVocabulary);
	Field(TEXT("language"), TEXT("en"));
	AppendStr(Body, FString::Printf(TEXT("--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"order.wav\"\r\nContent-Type: audio/wav\r\n\r\n"), *Boundary));
	Body.Append(Wav);
	AppendStr(Body, FString::Printf(TEXT("\r\n--%s--\r\n"), *Boundary));

	const double Sent = FPlatformTime::Seconds();
	TWeakObjectPtr<ADCVoice> Weak(this);
	auto Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(FString::Printf(TEXT("http://127.0.0.1:%d/inference"), STTPort));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary));
	Req->SetContent(MoveTemp(Body));
	Req->SetTimeout(15.f);
	Req->OnProcessRequestComplete().BindLambda([Weak, Sent](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
	{
		ADCVoice* V = Weak.Get();
		if (!V || V->bEnding) { return; }
		if (!bOk || !Resp.IsValid() || Resp->GetResponseCode() != 200)
		{
			UE_LOG(LogDC, Warning, TEXT("Voice: speech-to-text request failed"));
			return;
		}
		TSharedPtr<FJsonObject> Json;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
		FString Text;
		if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid()) { Json->TryGetStringField(TEXT("text"), Text); }
		Text = Text.TrimStartAndEnd().Replace(TEXT("\n"), TEXT(" "));
		UE_LOG(LogDC, Log, TEXT("Voice: heard \"%s\" (%.0f ms)"), *Text, (FPlatformTime::Seconds() - Sent) * 1000.0);
		V->OnHeard(Text);
	});
	Req->ProcessRequest();
}

void ADCVoice::OnHeard(const FString& Text)
{
	LastHeard = Text;
	if (Text.IsEmpty() || Text.StartsWith(TEXT("[")) || Text.StartsWith(TEXT("(")))
	{
		return; // Silence or a noise tag such as [BLANK_AUDIO].
	}
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->ExecuteOrder(Text);
	}
}

// ---------------------------------------------------------------- Replies

FString ADCVoice::VoiceFor(const FString& Speaker) const
{
	if (Speaker == TEXT("Duncan")) { return MarlowVoice; }
	if (Speaker == TEXT("Ruby")) { return OdessaVoice; }
	return FinchVoice;
}

FString ADCVoice::PersonaFor(const FString& Speaker) const
{
	if (Speaker == TEXT("Duncan")) { return TEXT("Duncan, the young gunner: eager, cocky, loves the cannons"); }
	if (Speaker == TEXT("Ruby")) { return TEXT("Ruby, the bosun: sharp-tongued, dry, fearless"); }
	return TEXT("Rex, the old helmsman: gruff, superstitious, loyal");
}

void ADCVoice::CrewReply(const FString& Speaker, const FString& Fallback, const FString& Context)
{
	SynchronizeScene();
	if (bEnding || bSmokeTest || IsMenuOpen()) { return; }
	// Hazard calls must describe what is happening now, without an asynchronous
	// paraphrase inventing directions or retaining the previous encounter.
	if (Context.IsEmpty())
	{
		const auto* D = ADCRideDirector::Get(this);
		if (D && D->GetPhase() >= EDCPhase::Slide &&
			(Fallback.Contains(TEXT("giant")) || Fallback.Contains(TEXT("Rocks")) || Fallback.Contains(TEXT("stone")) || Fallback.Contains(TEXT("Ghost ship")))) { return; }
		FinishLine(Speaker, Fallback, true);
		return;
	}
	if (!bUseLLM || !bLLMReady)
	{
		FinishLine(Speaker, Fallback);
		return;
	}

	FString Situation = TEXT("open sea in sunshine, broken ruins standing in the water, a wall of storm cloud ahead");
	if (ADCRideDirector* D = ADCRideDirector::Get(this))
	{
		switch (D->GetPhase())
		{
		case EDCPhase::OpenSea: if (D->RideTime < 26.f) { Situation = TEXT("open sea in sunshine, a fair wind, a pale bank of cloud low on the horizon ahead"); } break;
		case EDCPhase::Fog: Situation = D->RideTime < 76.f ? TEXT("sailing through a tunnel of rolling storm cloud, dark, rain, lightning inside the cloud walls, a pale light at the far end") : TEXT("mist among the vast ruins of a drowned city, temples and statues looming, rocks and fallen columns in the water, stone giants wading in from the banks to hurl burning rock"); break;
		case EDCPhase::Slide: Situation = TEXT("inside a water tunnel descending toward a caldera, no stone giants or city obstacles here"); break;
		case EDCPhase::Hell: Situation = TEXT("a burning channel of fire and demons"); break;
		case EDCPhase::Breach: Situation = TEXT("the ship bursts out into a vast storm"); break;
		case EDCPhase::Maelstrom: Situation = TEXT("a vast caldera lake, an enormous Kraken attacking with tentacle obstacles, tentacle slams and fireballs from its mouth"); break;
		case EDCPhase::Ending: Situation = TEXT("the storm is behind us: calm open sea in low golden sunlight, the ship battered but afloat, the crew relieved"); break;
		default: break;
		}
		if (!Context.IsEmpty()) { Situation += FString::Printf(TEXT("; hull at %d percent"), FMath::RoundToInt(D->ShipHealth)); }
	}

	// An empty Context means nobody asked: the crew member is reporting something they have seen.
	const bool bReport = Context.IsEmpty();
	const FString System = bReport
		? FString::Printf(
			TEXT("You are %s, crew on a pirate brigantine. You have just seen something the captain must know. ")
			TEXT("Shout it in one short urgent line, in character. At most 8 words. Repeat any direction exactly as given (port, starboard, ahead) and add no other direction or order. Spoken words only: no quotes, no actions, no narration."),
			*PersonaFor(Speaker))
		: DCDialoguePrompts::AcknowledgementSystem(PersonaFor(Speaker));
	const FString User = bReport
		? FString::Printf(TEXT("What you saw: %s\nSituation: %s"), *Fallback, *Situation)
		: FString::Printf(TEXT("Captain's order: \"%s\"\nWhat you are doing: %s\nSituation: %s"), *Context, *Fallback, *Situation);

	TWeakObjectPtr<ADCVoice> Weak(this);
	const double Sent = FPlatformTime::Seconds();
	const int32 Epoch = SpeechEpoch;
	RequestDialogue(System, User, 32, .6f, false, LLMTimeout,
		[Weak, Speaker, Fallback, Sent, Epoch](bool bOK, const FString& Content)
	{
		ADCVoice* V = Weak.Get();
		if (!V || V->bEnding) { return; }
		V->SynchronizeScene();
		if (Epoch != V->SpeechEpoch || V->IsMenuOpen() || FPlatformTime::Seconds() - Sent > 12.0) { return; }
		FString Line = bOK ? Content : FString();
		// Keep only a clean single spoken line.
		int32 ThinkEnd = Line.Find(TEXT("</think>"));
		if (ThinkEnd != INDEX_NONE) { Line = Line.Mid(ThinkEnd + 8); }
		Line = Line.TrimStartAndEnd().Replace(TEXT("\""), TEXT("")).Replace(TEXT("*"), TEXT(""));
		int32 NewLine = INDEX_NONE;
		if (Line.FindChar(TEXT('\n'), NewLine)) { Line = Line.Left(NewLine).TrimEnd(); }
		if (Line.IsEmpty() || Line.Len() > 120)
		{
			Line = Fallback;
		}
		// An acknowledgement must never reverse the actual order's direction.
		if ((Fallback.Contains(TEXT("starboard")) && Line.Contains(TEXT("port"))) ||
			(Fallback.Contains(TEXT("port")) && Line.Contains(TEXT("starboard")))) { Line = Fallback; }
		UE_LOG(LogDC, Verbose, TEXT("Voice: LLM line in %.0f ms"), (FPlatformTime::Seconds() - Sent) * 1000.0);
		V->FinishLine(Speaker, Line);
	});
}

void ADCVoice::FinishLine(const FString& Speaker, const FString& Text, bool bReport)
{
	if (bEnding) { return; }
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Say(Speaker, Text);
	}
	Speak(Speaker, Text, bReport);
}

void ADCVoice::Speak(const FString& Speaker, const FString& Text, bool bReport)
{
	SynchronizeScene();
	if (bEnding || IsMenuOpen() || Text.TrimStartAndEnd().IsEmpty()) { return; }
	if (!bTTSReady) { ShowSpeech(Speaker, Text, 4); return; }
	const FQueuedLine Line { Speaker, Text.Left(240), FPlatformTime::Seconds(), bReport };
	if (bReport)
	{
		SpeechQueue.RemoveAll([](const FQueuedLine& Queued) { return Queued.bReport; });
		SpeechQueue.Insert(Line, 0);
	}
	else
	{
		if (SpeechQueue.Num() >= 3 && !bSmokeTest) { SpeechQueue.RemoveAt(SpeechQueue.Num() - 1); }
		SpeechQueue.Add(Line);
	}
}

void ADCVoice::StartNextLine()
{
	while (!SpeechQueue.IsEmpty() && !bSmokeTest && FPlatformTime::Seconds() - SpeechQueue[0].QueuedAt > (SpeechQueue[0].bReport ? 6.0 : 12.0))
	{
		UE_LOG(LogDC, Log, TEXT("Voice: expired stale queued line for %s"), *SpeechQueue[0].Speaker);
		SpeechQueue.RemoveAt(0);
	}
	if (SpeechQueue.IsEmpty() || bEnding) { return; }
	const FQueuedLine Line = SpeechQueue[0];
	SpeechQueue.RemoveAt(0);
	const FString& Speaker = Line.Speaker;
	const FString& Text = Line.Text;
	const FString Dir = SidecarDir();
	const FString OutDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("TTS"));
	IFileManager::Get().MakeDirectory(*OutDir, true);
	const int32 Id = LineCounter++ % 32;
	const FString WavPath = OutDir / FString::Printf(TEXT("line_%02d.wav"), Id);
	IFileManager::Get().Delete(*WavPath, false, true, true);

	const FString Piper = Dir / TEXT("bin/piper/piper/piper.exe");
	const FString Model = Dir / TEXT("models/voices") / VoiceFor(Speaker);
	if (!FPaths::FileExists(Model)) { UE_LOG(LogDC, Warning, TEXT("Voice: missing model for %s: %s"), *Speaker, *Model); ShowSpeech(Speaker, Text, FMath::Clamp(Text.Len() * .055, 4.0, 9.0)); return; }
	const float Pace = Speaker == TEXT("Duncan") ? MarlowLengthScale : Speaker == TEXT("Ruby") ? OdessaLengthScale : FinchLengthScale;
	const FString Cmd = FString::Printf(TEXT("--model \"%s\" --output_file \"%s\" --speaker %d --length_scale %.3f --noise_scale 0.5 --noise_w %.3f --sentence_silence 0.12 --quiet"),
		*Model, *WavPath, Speaker == TEXT("Rex") ? FinchSpeakerId : 0, FMath::Clamp(Pace, .75f, 1.3f), Speaker == TEXT("Ruby") ? .333f : .65f);
	void* ReadPipe = nullptr; void* WritePipe = nullptr;
	if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe, true)) { ShowSpeech(Speaker, Text, FMath::Clamp(Text.Len() * .055, 4.0, 9.0)); return; }

	FPendingTTS P;
	P.Proc = FPlatformProcess::CreateProc(*Piper, *Cmd, false, true, true, nullptr, 0, nullptr, nullptr, ReadPipe);
	if (P.Proc.IsValid()) { FPlatformProcess::WritePipe(WritePipe, Text + TEXT("\n")); }
	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
	if (!P.Proc.IsValid())
	{
		UE_LOG(LogDC, Warning, TEXT("Voice: could not start piper"));
		ShowSpeech(Speaker, Text, FMath::Clamp(Text.Len() * .055, 4.0, 9.0));
		return;
	}
	P.Speaker = Speaker;
	P.Text = Text;
	P.Epoch = SpeechEpoch;
	P.WavPath = WavPath;
	P.StartTime = FPlatformTime::Seconds();
	PendingTTS.Add(MoveTemp(P));
}

bool ADCVoice::TryACE(const FString& Speaker, const FString& Path, double Duration)
{
	if (!bUseACE || bEnding) { return false; }
	AActor* Avatar = nullptr;
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		for (ADCCrewMember* Member : Crew->GetCrew())
		{
			if (!Member || Member->CrewName != Speaker) { continue; }
			TInlineComponentArray<UChildActorComponent*> VisualChildren(Member);
			for (UChildActorComponent* Child : VisualChildren)
			{
				if (Child->GetName() == TEXT("PirateAppearance")) { Avatar = Child->GetChildActor(); break; }
			}
		}
	}
	if (!Avatar) { return false; }
	ActiveCurves = Avatar->FindComponentByClass<UACEAudioCurveSourceComponent>();
	if (!ActiveCurves) { return false; }
	ActiveSpeaker = Speaker; ActiveWav = Path;
	ActiveDuration = Duration;
	ActiveCurves->Volume = 1.6f;
	ActiveCurves->Priority = 100.f;
	// Keep critical orders readable/audible at either station rather than losing
	// one crew member behind the player or to weather attenuation.
	ActiveCurves->bOverrideAttenuation = false;
	ActiveCurves->AttenuationSettings = nullptr;
	// Short crew replies are buffered while GPU inference runs. Starting with only
	// 100 ms queued produced gaps under the fog scene's graphics load.
	ActiveCurves->BufferLengthInSeconds = 60.f;
	bACEStarted = bACESent = bACEEnded = false;
	ACEDeadline = FPlatformTime::Seconds() + Duration + 30;
	ActiveCurves->OnAnimationStarted.AddDynamic(this, &ADCVoice::ACEStarted);
	ActiveCurves->OnAnimationEnded.AddDynamic(this, &ADCVoice::ACEEnded);
	ActiveACERequest = UAsyncActionAnimateCharacter::AnimateCharacterFromWavFileAsync(this, Avatar, Path,
		FAudio2FaceEmotion(), nullptr, ACEProvider);
	if (!ActiveACERequest) { ClearACE(); return false; }
	ActiveACERequest->AudioSendCompleted.AddDynamic(this, &ADCVoice::ACESent);
	ActiveACERequest->Activate();
	UE_LOG(LogDC, Log, TEXT("Voice: ACE queued %s provider=%s"), *Speaker, *ACEProvider.ToString());
	return true;
}

void ADCVoice::ACEStarted()
{
	bACEStarted = true;
	ShowSpeech(ActiveSpeaker, ActiveText, ActiveDuration);
	UE_LOG(LogDC, Log, TEXT("Voice: ACE speaking %s"), *ActiveSpeaker);
}
void ADCVoice::ACEEnded()
{
	bACEEnded = true;
	if (bSmokeTest && bACEStarted) { SmokeCompleted.Add(ActiveSpeaker); }
	UE_LOG(LogDC, Log, TEXT("Voice: ACE finished %s"), *ActiveSpeaker);
}
void ADCVoice::ACESent(bool bSuccess)
{
	bACESent = true;
	if (bSuccess && ActiveCurves) { ActiveCurves->BufferLengthInSeconds = .04f; }
	if (!bSuccess)
	{
		UE_LOG(LogDC, Warning, TEXT("Voice: ACE inference failed for %s; disabling ACE for this run"), *ActiveSpeaker);
		bUseACE = false;
		bACEEnded = true;
	}
}
void ADCVoice::ClearACE()
{
	if (ActiveACERequest) { ActiveACERequest->AudioSendCompleted.RemoveDynamic(this, &ADCVoice::ACESent); }
	if (ActiveCurves)
	{
		ActiveCurves->OnAnimationStarted.RemoveDynamic(this, &ADCVoice::ACEStarted);
		ActiveCurves->OnAnimationEnded.RemoveDynamic(this, &ADCVoice::ACEEnded);
		ActiveCurves->Stop();
	}
	ActiveCurves = nullptr;
	ActiveACERequest = nullptr;
}

void ADCVoice::PlayWav(const FString& Speaker, const FString& Path)
{
	TArray<uint8> Pcm;
	int32 Rate = 0, Channels = 0;
	if (!ReadWav(Path, Pcm, Rate, Channels) || Pcm.Num() == 0)
	{
		UE_LOG(LogDC, Warning, TEXT("Voice: no audio produced for %s"), *Speaker);
		return;
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(Rate);
	Wave->NumChannels = Channels;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Voice;
	Wave->Priority = 100.f;
	Wave->bLooping = false;
	Wave->QueueAudio(Pcm.GetData(), Pcm.Num());
	LiveWaves.Add(Wave);

	// Keep spoken warnings centred and intelligible from every ship station.
	UAudioComponent* Comp = UGameplayStatics::SpawnSound2D(this, Wave, 1.6f);
	if (Comp)
	{
		if (bSmokeTest && !bUseACE) { SmokeCompleted.Add(Speaker); }
		FPlaying P;
		P.Component = Comp;
		P.EndTime = FPlatformTime::Seconds() + static_cast<double>(Pcm.Num()) / (2.0 * Channels * Rate) + 0.3;
		Playing.Add(P);
		ShowSpeech(Speaker, ActiveText, double(Pcm.Num()) / (2.0 * Channels * Rate));
		UE_LOG(LogDC, Log, TEXT("Voice: %s speaking (%.1fs of audio)"), *Speaker, static_cast<double>(Pcm.Num()) / (2.0 * Channels * Rate));
	}
}

static FAutoConsoleCommandWithWorldAndArgs GDCVoiceTestWav(
	TEXT("dc.Voice.TestWav"), TEXT("Transcribe a wav file as if spoken into the microphone."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (ADCVoice* V = ADCVoice::Get(World))
		{
			if (Args.Num()) { V->TranscribeWavFile(FString::Join(Args, TEXT(" "))); }
		}
	}));

void ADCVoice::ResolveOrder(const FString& Text)
{
	// The keyword grammar did not recognise this. Ask the language model to turn it into the same
	// structured intent, so loose phrasing ("somebody get on that wheel!") still works.
	const FString System = DCDialoguePrompts::OrderSystem();

	TWeakObjectPtr<ADCVoice> Weak(this);
	const double Sent = FPlatformTime::Seconds();
	const int32 Epoch = SpeechEpoch;
	RequestDialogue(System, Text, 80, 0.f, true, 4.f,
		[Weak, Text, Sent, Epoch](bool bOK, const FString& Content)
	{
		ADCVoice* V = Weak.Get();
		if (!V || V->bEnding) { return; }
		V->SynchronizeScene();
		if (Epoch != V->SpeechEpoch || V->IsMenuOpen()) { return; }
		FDCIntent Intent;
		Intent.RawText = Text;
		if (bOK) { DCLanguageModelResponse::ParseIntent(Content, Text, Intent); }
		UDCCrewSubsystem* CrewSub = V->GetWorld()->GetSubsystem<UDCCrewSubsystem>();
		if (!Intent.bValid && CrewSub)
		{
			// Second opinion from the keyword grammar before giving up.
			TArray<FString> Names;
			for (ADCCrewMember* M : CrewSub->GetCrew()) { if (M) { Names.Add(M->CrewName); } }
			Intent = DCIntentParser::Parse(Text, Names);
		}
		UE_LOG(LogDC, Log, TEXT("Voice: language model read \"%s\" as %s (%.0f ms)"), *Text, Intent.bValid ? *Content.Replace(TEXT("\n"), TEXT(" ")) : TEXT("not an order"), (FPlatformTime::Seconds() - Sent) * 1000.0);
		if (UDCCrewSubsystem* Crew = V->GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->ExecuteIntent(Intent);
		}
	});
}
