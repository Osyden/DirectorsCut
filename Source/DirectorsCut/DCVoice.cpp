#include "DCVoice.h"
#include "DCTypes.h"
#include "DCCrew.h"
#include "DCRideDirector.h"
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
		if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() < 44) { return false; }
		OutChannels = *reinterpret_cast<const uint16*>(&Bytes[22]);
		OutRate = *reinterpret_cast<const int32*>(&Bytes[24]);
		const int32 Bits = *reinterpret_cast<const uint16*>(&Bytes[34]);
		if (Bits != 16) { return false; }
		int32 Pos = 12;
		while (Pos + 8 <= Bytes.Num())
		{
			const uint32 Size = *reinterpret_cast<const uint32*>(&Bytes[Pos + 4]);
			if (FMemory::Memcmp(&Bytes[Pos], "data", 4) == 0)
			{
				const int32 Len = FMath::Min<int64>(Size, Bytes.Num() - (Pos + 8));
				OutPcm.Append(&Bytes[Pos + 8], Len);
				return true;
			}
			Pos += 8 + Size + (Size & 1);
		}
		return false;
	}
}

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

void ADCVoice::BeginPlay()
{
	Super::BeginPlay();
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
	if (Capture.IsValid())
	{
		bRecording = false;
		Capture->AbortStream();
		Capture->CloseStream();
		Capture.Reset();
	}
	// Only stop servers this run started; ones already running (dev) are left alone.
	if (LLMProc.IsValid()) { FPlatformProcess::TerminateProc(LLMProc, true); FPlatformProcess::CloseProc(LLMProc); }
	if (STTProc.IsValid()) { FPlatformProcess::TerminateProc(STTProc, true); FPlatformProcess::CloseProc(STTProc); }
	Super::EndPlay(Reason);
}

void ADCVoice::CheckHealth()
{
	TWeakObjectPtr<ADCVoice> Weak(this);

	if (!bLLMReady && bUseLLM)
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

	if (!bSTTReady)
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
	if (bUseLLM && !bLLMReady && FPaths::FileExists(LLMExe) && FPaths::FileExists(LLMModelPath))
	{
		const FString Args = FString::Printf(TEXT("-m \"%s\" --host 127.0.0.1 --port %d -ngl 99 -c 4096"), *LLMModelPath, LLMPort);
		LLMProc = FPlatformProcess::CreateProc(*LLMExe, *Args, false, true, true, nullptr, 0, *(Dir / TEXT("bin/llama")), nullptr);
		UE_LOG(LogDC, Log, TEXT("Voice: started llama-server (%s)"), LLMProc.IsValid() ? TEXT("ok") : TEXT("failed"));
	}

	const FString STTExe = Dir / TEXT("bin/whisper/Release/whisper-server.exe");
	const FString STTModelPath = Dir / TEXT("models") / STTModel;
	if (!bSTTReady && FPaths::FileExists(STTExe) && FPaths::FileExists(STTModelPath))
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

	if (!bLLMReady || !bSTTReady)
	{
		HealthTimer -= DeltaSeconds;
		if (HealthTimer <= 0.f)
		{
			HealthTimer = 2.f;
			CheckHealth();
		}
	}

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
			if (bDone) { PlayWav(P.Speaker, P.WavPath); }
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
	Field(TEXT("prompt"), TEXT("Orders on a sailing ship. Finch, Marlow, Odessa. Hard to starboard. Hard to port. Steady as she goes. Take the helm. Man the cannons. Port battery, fire. Starboard guns, hold fire. Put out the fire. Repair the rigging."));
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
		if (!V) { return; }
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
	if (Speaker == TEXT("Marlow")) { return TEXT("en_GB-northern_english_male-medium.onnx"); }
	if (Speaker == TEXT("Odessa")) { return TEXT("en_GB-alba-medium.onnx"); }
	return TEXT("en_GB-alan-medium.onnx");
}

FString ADCVoice::PersonaFor(const FString& Speaker) const
{
	if (Speaker == TEXT("Marlow")) { return TEXT("Marlow, the young gunner: eager, cocky, loves the cannons"); }
	if (Speaker == TEXT("Odessa")) { return TEXT("Odessa, the bosun: sharp-tongued, dry, fearless"); }
	return TEXT("Finch, the old helmsman: gruff, superstitious, loyal");
}

void ADCVoice::CrewReply(const FString& Speaker, const FString& Fallback, const FString& Context)
{
	if (!bUseLLM || !bLLMReady)
	{
		FinishLine(Speaker, Fallback);
		return;
	}

	FString Situation = TEXT("calm open sea");
	if (ADCRideDirector* D = ADCRideDirector::Get(this))
	{
		switch (D->GetPhase())
		{
		case EDCPhase::Fog: Situation = TEXT("thick unnatural fog, ghost ships and ruins looming"); break;
		case EDCPhase::Slide: Situation = TEXT("the sea is dropping away beneath the ship"); break;
		case EDCPhase::Hell: Situation = TEXT("a burning channel of fire and demons"); break;
		case EDCPhase::Breach: Situation = TEXT("the ship bursts out into a vast storm"); break;
		case EDCPhase::Maelstrom: Situation = TEXT("a giant whirlpool, a kraken's tentacles rising around the ship"); break;
		default: break;
		}
		Situation += FString::Printf(TEXT("; hull at %d percent"), FMath::RoundToInt(D->ShipHealth));
	}

	// An empty Context means nobody asked: the crew member is reporting something they have seen.
	const bool bReport = Context.IsEmpty();
	const FString System = bReport
		? FString::Printf(
			TEXT("You are %s, crew on a pirate brigantine. You have just seen something the captain must know. ")
			TEXT("Shout it in one short urgent line, in character. At most 8 words. Keep any direction given (port, starboard, ahead). Spoken words only: no quotes, no actions, no narration."),
			*PersonaFor(Speaker))
		: FString::Printf(
			TEXT("You are %s, crew on a pirate brigantine. The captain has given an order and you are already carrying it out. ")
			TEXT("Say one short line aloud to the captain confirming it, in character. At most 10 words. Spoken words only: no quotes, no actions, no narration."),
			*PersonaFor(Speaker));
	const FString User = bReport
		? FString::Printf(TEXT("What you saw: %s\nSituation: %s"), *Fallback, *Situation)
		: FString::Printf(TEXT("Captain's order: \"%s\"\nWhat you are doing: %s\nSituation: %s"), *Context, *Fallback, *Situation);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Messages;
	auto Msg = [&Messages](const TCHAR* InRole, const FString& Content)
	{
		TSharedRef<FJsonObject> M = MakeShared<FJsonObject>();
		M->SetStringField(TEXT("role"), InRole);
		M->SetStringField(TEXT("content"), Content);
		Messages.Add(MakeShared<FJsonValueObject>(M));
	};
	Msg(TEXT("system"), System);
	Msg(TEXT("user"), User);
	Root->SetArrayField(TEXT("messages"), Messages);
	Root->SetNumberField(TEXT("max_tokens"), 32);
	Root->SetNumberField(TEXT("temperature"), 0.9);
	TSharedRef<FJsonObject> Kwargs = MakeShared<FJsonObject>();
	Kwargs->SetBoolField(TEXT("enable_thinking"), false);
	Root->SetObjectField(TEXT("chat_template_kwargs"), Kwargs);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Root, Writer);

	TWeakObjectPtr<ADCVoice> Weak(this);
	const double Sent = FPlatformTime::Seconds();
	auto Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(FString::Printf(TEXT("http://127.0.0.1:%d/v1/chat/completions"), LLMPort));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetContentAsString(BodyStr);
	Req->SetTimeout(LLMTimeout);
	Req->OnProcessRequestComplete().BindLambda([Weak, Speaker, Fallback, Sent](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
	{
		ADCVoice* V = Weak.Get();
		if (!V) { return; }
		FString Line;
		if (bOk && Resp.IsValid() && Resp->GetResponseCode() == 200)
		{
			TSharedPtr<FJsonObject> Json;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
			if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
				if (Json->TryGetArrayField(TEXT("choices"), Choices) && Choices && Choices->Num())
				{
					const TSharedPtr<FJsonObject>* MsgObj = nullptr;
					if ((*Choices)[0]->AsObject()->TryGetObjectField(TEXT("message"), MsgObj) && MsgObj)
					{
						(*MsgObj)->TryGetStringField(TEXT("content"), Line);
					}
				}
			}
		}
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
		UE_LOG(LogDC, Verbose, TEXT("Voice: LLM line in %.0f ms"), (FPlatformTime::Seconds() - Sent) * 1000.0);
		V->FinishLine(Speaker, Line);
	});
	Req->ProcessRequest();
}

void ADCVoice::FinishLine(const FString& Speaker, const FString& Text)
{
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Say(Speaker, Text);
	}
	Speak(Speaker, Text);
}

void ADCVoice::Speak(const FString& Speaker, const FString& Text)
{
	if (!bTTSReady) { return; }
	const FString Dir = SidecarDir();
	const FString OutDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("TTS"));
	IFileManager::Get().MakeDirectory(*OutDir, true);
	const int32 Id = LineCounter++ % 32;
	const FString TextPath = OutDir / FString::Printf(TEXT("line_%02d.txt"), Id);
	const FString WavPath = OutDir / FString::Printf(TEXT("line_%02d.wav"), Id);
	IFileManager::Get().Delete(*WavPath, false, true, true);
	FFileHelper::SaveStringToFile(Text, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	const FString Piper = Dir / TEXT("bin/piper/piper/piper.exe");
	const FString Model = Dir / TEXT("models/voices") / VoiceFor(Speaker);
	FString Cmd = FString::Printf(TEXT("/c \"type \"%s\" | \"%s\" --model \"%s\" --output_file \"%s\"\""), *TextPath, *Piper, *Model, *WavPath);
	Cmd.ReplaceInline(TEXT("/"), TEXT("\\"));
	Cmd = TEXT("/c") + Cmd.Mid(2); // keep the switch itself

	FPendingTTS P;
	P.Proc = FPlatformProcess::CreateProc(TEXT("cmd.exe"), *Cmd, false, true, true, nullptr, 0, nullptr, nullptr);
	if (!P.Proc.IsValid())
	{
		UE_LOG(LogDC, Warning, TEXT("Voice: could not start piper"));
		return;
	}
	P.Speaker = Speaker;
	P.WavPath = WavPath;
	P.StartTime = FPlatformTime::Seconds();
	PendingTTS.Add(MoveTemp(P));
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
	Wave->bLooping = false;
	Wave->QueueAudio(Pcm.GetData(), Pcm.Num());
	LiveWaves.Add(Wave);

	// Speak from the crew member's position so the voice has a place on deck.
	USceneComponent* AttachTo = nullptr;
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		for (ADCCrewMember* M : Crew->GetCrew())
		{
			if (M && M->CrewName == Speaker) { AttachTo = M->GetRootComponent(); break; }
		}
	}
	UAudioComponent* Comp = AttachTo
		? UGameplayStatics::SpawnSoundAttached(Wave, AttachTo, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false, 1.6f)
		: UGameplayStatics::SpawnSound2D(this, Wave, 1.6f);
	if (Comp)
	{
		FPlaying P;
		P.Component = Comp;
		P.EndTime = FPlatformTime::Seconds() + static_cast<double>(Pcm.Num()) / (2.0 * Channels * Rate) + 0.3;
		Playing.Add(P);
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
