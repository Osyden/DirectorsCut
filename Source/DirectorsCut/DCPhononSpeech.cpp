#include "DCPhononSpeech.h"
#include "ACEAudioCurveSourceComponent.h"
#include "ACERuntimeModule.h"
#include "Async/Async.h"
#include "Containers/Queue.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Base64.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogDCPhononSpeech, Log, All);

namespace DCPhononSpeechPrivate
{
    constexpr int32 SampleRate = 24000;
    constexpr int32 MaxSamples = SampleRate * 30;
    constexpr int32 MaxLineCharacters = 128 * 1024;
    constexpr int32 MaxReadCharacters = 2 * 1024 * 1024;
    constexpr int32 SendSamples = SampleRate / 10;
    // The plugin's session map is not internally locked. This protects this
    // component's workers; the owner must also exclude legacy async WAV sends.
    FCriticalSection NativeCalls;

    struct FLease
    {
        explicit FLease(UACEAudioCurveSourceComponent* InConsumer) : Consumer(InConsumer) {}
        TStrongObjectPtr<UACEAudioCurveSourceComponent> Consumer;
    };
    using FLeasePtr = TSharedPtr<FLease, ESPMode::ThreadSafe>;

    FLeasePtr Lease(UACEAudioCurveSourceComponent* Consumer)
    {
        check(IsInGameThread());
        return FLeasePtr(new FLease(Consumer), [](FLease* Value)
        {
            // Strong UObject references are created and released on the GT,
            // including when a worker outlives its owning component's EndPlay.
            if (IsInGameThread()) { delete Value; }
            else { AsyncTask(ENamedThreads::GameThread, [Value] { delete Value; }); }
        });
    }

    struct FRequest
    {
        int32 Id = 0;
        FString Speaker, Text;
        FName Provider;
        FLeasePtr Consumer;
    };

    bool Integer(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& Out)
    {
        double Value = 0;
        const TSharedPtr<FJsonValue> FieldValue = Object->TryGetField(Field);
        if (!FieldValue.IsValid() || FieldValue->Type != EJson::Number ||
            !Object->TryGetNumberField(Field, Value) || !FMath::IsFinite(Value) || Value < 0 ||
            Value > MAX_int32 || Value != FMath::FloorToDouble(Value)) { return false; }
        Out = static_cast<int32>(Value);
        return true;
    }

    bool String(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FString& Out)
    {
        const TSharedPtr<FJsonValue> Value = Object->TryGetField(Field);
        return Value.IsValid() && Value->Type == EJson::String && Object->TryGetStringField(Field, Out);
    }

    struct FWorker : TSharedFromThis<FWorker, ESPMode::ThreadSafe>
    {
        FString Python, Script, Model, Voices;
        FACERuntimeModule* ACE = nullptr;
        TAtomic<bool> Ready{false}, Busy{false}, ShutdownRequested{false};
        TAtomic<int32> CancelId{0};
        TAtomic<uint64> ReuseAfterFrame{0};
        TQueue<FRequest, EQueueMode::Mpsc> Requests;
        TQueue<FLeasePtr, EQueueMode::Mpsc> Stops;
        TFunction<void(int32, bool, double)> Deliver;

        // Everything below is accessed only by the owned worker thread.
        FProcHandle Process;
        void* Output = nullptr;
        void* Input = nullptr;
        void* Errors = nullptr;
        FString PendingLine;
        FRequest Active;
        TArray<int16> PCM;
        int32 PCMOffset = 0, ReceivedSamples = 0, NextSequence = 0, LastRetiredId = 0;
        bool bProducerDone = false, bFailed = false, bSession = false, bCancelSent = false, bStopQueued = false;
        bool bRestart = false, bUnavailable = false;
        double Started = 0, LastPCM = 0, CancelDeadline = 0, Booted = 0;
        int32 Restarts = 0;

        bool SendJSON(const TSharedRef<FJsonObject>& Object)
        {
            FString JSON;
            const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JSON);
            if (!FJsonSerializer::Serialize(Object, Writer)) { return false; }
            JSON += TEXT("\n");
            const FTCHARToUTF8 Bytes(*JSON);
            int32 Written = 0;
            return Input && FPlatformProcess::WritePipe(Input, reinterpret_cast<const uint8*>(Bytes.Get()),
                Bytes.Length(), &Written) && Written == Bytes.Length();
        }

        void CloseProcess()
        {
            Ready.Store(false);
            if (Process.IsValid())
            {
                if (FPlatformProcess::IsProcRunning(Process)) { FPlatformProcess::TerminateProc(Process, true); }
                FPlatformProcess::CloseProc(Process);
                Process.Reset();
            }
            FPlatformProcess::ClosePipe(Output, nullptr); Output = nullptr;
            FPlatformProcess::ClosePipe(nullptr, Input); Input = nullptr;
            FPlatformProcess::ClosePipe(Errors, nullptr); Errors = nullptr;
            PendingLine.Reset();
        }

        bool OpenProcess()
        {
            void* ChildOutput = nullptr;
            void* ChildInput = nullptr;
            void* ChildErrors = nullptr;
            const bool bPipes = FPlatformProcess::CreatePipe(Output, ChildOutput) &&
                FPlatformProcess::CreatePipe(ChildInput, Input, true) &&
                FPlatformProcess::CreatePipe(Errors, ChildErrors);
            if (bPipes)
            {
                // CreateProc launches the executable directly, not through a shell.
                // Python sidecar owns its fixed CPU-thread configuration (four).
                const FString Args = FString::Printf(TEXT("-u \"%s\" --model \"%s\" --voices \"%s\""), *Script, *Model, *Voices);
                Process = FPlatformProcess::CreateProc(*Python, *Args, false, true, true, nullptr, 0,
                    *FPaths::GetPath(Script), ChildOutput, ChildInput, ChildErrors);
            }
            FPlatformProcess::ClosePipe(nullptr, ChildOutput);
            FPlatformProcess::ClosePipe(ChildInput, nullptr);
            FPlatformProcess::ClosePipe(nullptr, ChildErrors);
            Booted = FPlatformTime::Seconds();
            if (!Process.IsValid()) { CloseProcess(); return false; }
            return true;
        }

        void Fail(const TCHAR* Reason, bool bRestartProcess = false)
        {
            if (!bFailed) { UE_LOG(LogDCPhononSpeech, Warning, TEXT("Phonon request %d: %s"), Active.Id, Reason); }
            bFailed = true;
            bRestart |= bRestartProcess;
        }

        bool HandleLine(const FString& Line)
        {
            if (Line.IsEmpty()) { return true; }
            TSharedPtr<FJsonObject> Object;
            FString Type;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Line), Object) || !Object.IsValid() ||
                !String(Object, TEXT("type"), Type)) { return false; }
            if (Type == TEXT("ready"))
            {
                int32 Rate = 0;
                if (Ready.Load() || Active.Id || !Integer(Object, TEXT("sample_rate"), Rate) || Rate != SampleRate) { return false; }
                Ready.Store(true);
                UE_LOG(LogDCPhononSpeech, Display, TEXT("Phonon local CPU producer ready at %d Hz"), Rate);
                return true;
            }
            int32 Id = 0;
            if (!Integer(Object, TEXT("id"), Id) || Id <= 0) { return false; }
            // Cancelled producers may have already written trailing frames. They
            // never become a new request's audio and cannot affect its counters.
            if (Id <= LastRetiredId) { return true; }
            if (!Active.Id || Id != Active.Id) { return false; }
            if (Type == TEXT("error") || Type == TEXT("cancelled"))
            {
                if (bProducerDone) { return false; }
                FString IgnoredMessage;
                if (Type == TEXT("error") && !String(Object, TEXT("message"), IgnoredMessage)) { return false; }
                Fail(Type == TEXT("error") ? TEXT("producer reported an error") : TEXT("producer cancelled"));
                bProducerDone = true;
                return true;
            }
            if (Type == TEXT("done"))
            {
                int32 Total = 0;
                if (bProducerDone || !Integer(Object, TEXT("samples"), Total)) { return false; }
                // During cancellation PCM is discarded without decoding. The
                // terminal frame still acknowledges producer ownership is over.
                if (!bCancelSent && (Total <= 0 || Total != ReceivedSamples)) { return false; }
                bProducerDone = true;
                return true;
            }
            if (Type != TEXT("pcm") || bProducerDone) { return false; }
            if (bCancelSent) { return true; }
            int32 Sequence = 0, Rate = 0;
            FString Data;
            if (!Integer(Object, TEXT("seq"), Sequence) || Sequence != NextSequence ||
                !Integer(Object, TEXT("rate"), Rate) || Rate != SampleRate ||
                !String(Object, TEXT("data"), Data) || Data.IsEmpty() || Data.Len() > MaxLineCharacters - 128)
            { return false; }
            TArray<uint8> Bytes;
            if (!FBase64::Decode(Data, Bytes) || Bytes.IsEmpty() || (Bytes.Num() & 1) ||
                FBase64::Encode(Bytes) != Data || Bytes.Num() / 2 > MaxSamples - ReceivedSamples)
            { return false; }
            const int32 Count = Bytes.Num() / 2;
            const int32 Offset = PCM.AddUninitialized(Count);
            for (int32 I = 0; I < Count; ++I)
            {
                PCM[Offset + I] = static_cast<int16>(static_cast<uint16>(Bytes[I * 2]) |
                    (static_cast<uint16>(Bytes[I * 2 + 1]) << 8));
            }
            ReceivedSamples += Count;
            ++NextSequence;
            LastPCM = FPlatformTime::Seconds();
            return true;
        }

        bool ReadOutput()
        {
            // Do not log stderr or protocol bodies: they can contain prompts,
            // local paths or arbitrary producer diagnostics. Drain to prevent a
            // full stderr pipe from stalling an otherwise healthy producer.
            if (Errors) { FPlatformProcess::ReadPipe(Errors); }
            const FString Incoming = Output ? FPlatformProcess::ReadPipe(Output) : FString();
            if (Incoming.Len() > MaxReadCharacters || PendingLine.Len() > MaxReadCharacters - Incoming.Len()) { return false; }
            for (TCHAR C : Incoming) { if (C == 0 || static_cast<uint32>(C) > 127) { return false; } }
            PendingLine += Incoming;
            int32 Newline = INDEX_NONE;
            while (PendingLine.FindChar(TEXT('\n'), Newline))
            {
                if (Newline > MaxLineCharacters) { return false; }
                FString Line = PendingLine.Left(Newline);
                PendingLine.RightChopInline(Newline + 1);
                if (Line.EndsWith(TEXT("\r"))) { Line.LeftChopInline(1); }
                if (!HandleLine(Line)) { return false; }
            }
            return PendingLine.Len() <= MaxLineCharacters;
        }

        void Begin(FRequest&& Request)
        {
            Active = MoveTemp(Request);
            PCM.Reset(); PCMOffset = ReceivedSamples = NextSequence = 0;
            bProducerDone = bFailed = bSession = bCancelSent = bStopQueued = false;
            Started = LastPCM = FPlatformTime::Seconds();
            CancelDeadline = 0;
            if (CancelId.Load() == Active.Id || ShutdownRequested.Load())
            {
                bFailed = bProducerDone = true;
                return;
            }
            const auto Object = MakeShared<FJsonObject>();
            Object->SetStringField(TEXT("op"), TEXT("speak"));
            Object->SetNumberField(TEXT("id"), Active.Id);
            Object->SetStringField(TEXT("speaker"), Active.Speaker);
            Object->SetStringField(TEXT("text"), Active.Text);
            if (!Ready.Load() || !SendJSON(Object)) { Fail(TEXT("could not send producer request"), true); }
        }

        void CancelProducer()
        {
            PCM.Reset(); PCMOffset = 0;
            if (!bCancelSent && !bProducerDone)
            {
                bCancelSent = true;
                CancelDeadline = FPlatformTime::Seconds() + 3.0;
                const auto Object = MakeShared<FJsonObject>();
                Object->SetStringField(TEXT("op"), TEXT("cancel"));
                Object->SetNumberField(TEXT("id"), Active.Id);
                if (!SendJSON(Object)) { bRestart = true; }
            }
        }

        void StopOnGameThread(const FLeasePtr& Consumer, bool bReleaseBusy)
        {
            auto Self = AsShared();
            AsyncTask(ENamedThreads::GameThread, [Self, Consumer, bReleaseBusy]
            {
                if (Consumer.IsValid() && IsValid(Consumer->Consumer.Get()))
                {
                    Consumer->Consumer->Stop();
                    Self->ReuseAfterFrame.Store(GFrameCounter + 2);
                }
                if (bReleaseBusy) { Self->Busy.Store(false); }
            });
        }

        bool CloseNative(bool bSuccess)
        {
            if (!bSession) { return bSuccess; }
            FScopeLock NativeGuard(&NativeCalls);
            if (!bSuccess) { ACE->CancelAnimationGeneration(Active.Consumer->Consumer.Get()); }
            bSuccess &= ACE->EndAudioSamples(Active.Consumer->Consumer.Get());
            // Local EndStream blocks until callbacks drain. Also detach after
            // EVERY end, including success, so cancellation between this point
            // and the GT completion cannot reintroduce Stop's lock inversion.
            // Detaching does not stop buffered playback; only GT Stop does that.
            ACE->CancelAnimationGeneration(Active.Consumer->Consumer.Get());
            bSession = false;
            return bSuccess;
        }

        void DrainStops()
        {
            FLeasePtr Consumer;
            while (Stops.Dequeue(Consumer))
            {
                // A successful sender already closed its session. This is the
                // retained playback lease being released by ClearACE/Cancel.
                { FScopeLock NativeGuard(&NativeCalls); ACE->CancelAnimationGeneration(Consumer->Consumer.Get()); }
                StopOnGameThread(Consumer, true);
            }
        }

        void FinishRequest(bool bSuccess)
        {
            // Detach without holding the component's audio lock. Stop() on the
            // GT only happens after this worker has drained native callbacks.
            bSuccess = CloseNative(bSuccess);
            const int32 Id = Active.Id;
            const double Duration = static_cast<double>(ReceivedSamples) / SampleRate;
            FLeasePtr Consumer = MoveTemp(Active.Consumer);
            const bool bAlreadyStopped = bStopQueued;
            LastRetiredId = Id;
            Active = FRequest();
            PCM.Reset(); PCMOffset = 0;
            auto Self = AsShared();
            AsyncTask(ENamedThreads::GameThread, [Self, Consumer, Id, bSuccess, Duration, bAlreadyStopped]
            {
                const bool bCancelled = Self->ShutdownRequested.Load() || Self->CancelId.Load() == Id;
                if (!bAlreadyStopped && (!bSuccess || bCancelled) && Consumer.IsValid() && IsValid(Consumer->Consumer.Get()))
                {
                    // EndAudioSamples finished first: no callback still holds
                    // registry DataCS while Stop acquires the audio lock.
                    Consumer->Consumer->Stop();
                    Self->ReuseAfterFrame.Store(GFrameCounter + 2);
                }
                Self->Busy.Store(false);
                if (!bCancelled && Self->Deliver) { Self->Deliver(Id, bSuccess, Duration); }
            });
        }

        void Run()
        {
            if (!OpenProcess())
            {
                UE_LOG(LogDCPhononSpeech, Error, TEXT("Could not launch local Phonon producer"));
                bUnavailable = true;
            }
            while (!ShutdownRequested.Load())
            {
                DrainStops();
                // Keep the cleanup worker alive even if producer startup/restarts
                // fail: a previous successfully-sent clip can still be playing.
                if (bUnavailable) { FPlatformProcess::SleepNoStats(.01f); continue; }
                FRequest Request;
                if (!Active.Id && Requests.Dequeue(Request)) { Begin(MoveTemp(Request)); }
                const bool bAlive = FPlatformProcess::IsProcRunning(Process);
                if (!bAlive || !ReadOutput())
                {
                    Ready.Store(false);
                    if (Active.Id) { Fail(TEXT("producer exited or violated PCM protocol"), true); }
                    else { bRestart = true; }
                }
                const double Now = FPlatformTime::Seconds();
                if (!Ready.Load() && Now - Booted > 120.0) { bRestart = true; }
                if (Active.Id)
                {
                    if (CancelId.Load() == Active.Id) { bFailed = true; }
                    if (!bProducerDone && (Now - Started > 60.0 || Now - LastPCM > 15.0)) { Fail(TEXT("producer timed out")); }
                    if (bFailed)
                    {
                        CancelProducer();
                        // Release native playback promptly; producer cancellation
                        // acknowledgment may take up to three further seconds.
                        CloseNative(false);
                        if (!bStopQueued) { StopOnGameThread(Active.Consumer, false); bStopQueued = true; }
                    }
                    if (bCancelSent && !bProducerDone && Now >= CancelDeadline) { bRestart = true; }
                    if (bRestart)
                    {
                        CloseProcess();
                        bProducerDone = true;
                        bFailed = true;
                    }
                    if (bFailed && (bProducerDone || bRestart)) { FinishRequest(false); }
                    else if (!bFailed && PCMOffset < PCM.Num())
                    {
                        const int32 Count = FMath::Min(SendSamples, PCM.Num() - PCMOffset);
                        bSession = true; // even a failed first send creates a tracked session
                        bool bSent = false;
                        {
                            FScopeLock NativeGuard(&NativeCalls);
                            bSent = ACE->AnimateFromAudioSamples(Active.Consumer->Consumer.Get(),
                                TArrayView<const int16>(PCM.GetData() + PCMOffset, Count), 1, SampleRate,
                                false, TOptional<FAudio2FaceEmotion>(), nullptr, Active.Provider);
                        }
                        if (!bSent) { Fail(TEXT("native Audio2Face send failed")); }
                        PCMOffset += Count;
                        if (PCMOffset == PCM.Num()) { PCM.Reset(); PCMOffset = 0; }
                    }
                    else if (!bFailed && bProducerDone) { FinishRequest(ReceivedSamples > 0); }
                }
                if (bRestart && !Active.Id)
                {
                    CloseProcess(); bRestart = false;
                    if (++Restarts > 3 || !OpenProcess())
                    {
                        UE_LOG(LogDCPhononSpeech, Error, TEXT("Phonon producer restart limit reached; reinitialize component to retry"));
                        bUnavailable = true;
                    }
                }
                FPlatformProcess::SleepNoStats(.005f);
            }
            Ready.Store(false);
            DrainStops();
            // No GT join: native finalization may need the GT to service pending
            // audio preparation. Retain the consumer lease until it has drained.
            if (Active.Id)
            {
                CloseProcess();
                FinishRequest(false);
            }
            FRequest Abandoned;
            while (Requests.Dequeue(Abandoned))
            {
                Active = MoveTemp(Abandoned);
                bSession = bStopQueued = false; ReceivedSamples = 0;
                FinishRequest(false);
            }
            if (Process.IsValid())
            {
                const auto Object = MakeShared<FJsonObject>(); Object->SetStringField(TEXT("op"), TEXT("shutdown"));
                SendJSON(Object);
            }
            CloseProcess();
        }
    };
}

UDCPhononSpeech::UDCPhononSpeech()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UDCPhononSpeech::Initialize(const FString& PythonPath, const FString& ScriptPath,
    const FString& ModelDirectory, const FString& VoiceManifestPath)
{
    check(IsInGameThread());
    if (Worker.IsValid() || bStopped) { return false; }
    for (const FString* Path : { &PythonPath, &ScriptPath, &ModelDirectory, &VoiceManifestPath })
    {
        if (Path->IsEmpty() || Path->Contains(TEXT("\"")) || Path->Contains(TEXT("\n")) || Path->Contains(TEXT("\r"))) { return false; }
    }
    if (!FPaths::FileExists(PythonPath) || !FPaths::FileExists(ScriptPath) ||
        !FPaths::FileExists(VoiceManifestPath) || !IFileManager::Get().DirectoryExists(*ModelDirectory)) { return false; }
    Worker = MakeShared<DCPhononSpeechPrivate::FWorker, ESPMode::ThreadSafe>();
    Worker->Python = FPaths::ConvertRelativePathToFull(PythonPath);
    Worker->Script = FPaths::ConvertRelativePathToFull(ScriptPath);
    Worker->Model = FPaths::ConvertRelativePathToFull(ModelDirectory);
    Worker->Voices = FPaths::ConvertRelativePathToFull(VoiceManifestPath);
    Worker->ACE = &FACERuntimeModule::Get();
    TWeakObjectPtr<UDCPhononSpeech> Weak(this);
    Worker->Deliver = [Weak](int32 Id, bool bSuccess, double Duration)
    {
        if (UDCPhononSpeech* Self = Weak.Get()) { Self->Delivered(Id, bSuccess, Duration); }
    };
    const auto State = Worker.ToSharedRef();
    Async(EAsyncExecution::Thread, [State] { State->Run(); });
    return true;
}

bool UDCPhononSpeech::IsReady() const { return !bStopped && Worker.IsValid() && Worker->Ready.Load(); }
bool UDCPhononSpeech::IsBusy() const
{
    check(IsInGameThread());
    return Worker.IsValid() && (Worker->Busy.Load() || GFrameCounter < Worker->ReuseAfterFrame.Load());
}

bool UDCPhononSpeech::Speak(const FString& Speaker, const FString& Text, UACEAudioCurveSourceComponent* Consumer,
    FName Provider, TFunction<void(bool, double)> Sent)
{
    check(IsInGameThread());
    if (!IsReady() || IsBusy() || LastConsumer.Get() != nullptr || !IsValid(Consumer) || !Sent || Text.TrimStartAndEnd().IsEmpty() || Text.Len() > 240 ||
        (Speaker != TEXT("Rex") && Speaker != TEXT("Duncan") && Speaker != TEXT("Ruby")) ||
        Provider.IsNone() || Provider == FName("Default") || NextId == MAX_int32) { return false; }
    ActiveId = ++NextId;
    LastConsumer = Consumer;
    Completion = MoveTemp(Sent);
    Worker->Busy.Store(true);
    DCPhononSpeechPrivate::FRequest Request;
    Request.Id = ActiveId; Request.Speaker = Speaker; Request.Text = Text; Request.Provider = Provider;
    Request.Consumer = DCPhononSpeechPrivate::Lease(Consumer);
    Worker->Requests.Enqueue(MoveTemp(Request));
    return true;
}

void UDCPhononSpeech::Cancel()
{
    check(IsInGameThread());
    if (Worker.IsValid() && ActiveId) { Worker->CancelId.Store(ActiveId); }
    else if (Worker.IsValid() && IsValid(LastConsumer.Get()))
    {
        Worker->Busy.Store(true);
        Worker->Stops.Enqueue(DCPhononSpeechPrivate::Lease(LastConsumer.Get()));
    }
    LastConsumer = nullptr;
    Completion = nullptr;
    ActiveId = 0;
}

void UDCPhononSpeech::Delivered(int32 Id, bool bSuccess, double Duration)
{
    check(IsInGameThread());
    if (bStopped || Id != ActiveId) { return; }
    auto Callback = MoveTemp(Completion);
    ActiveId = 0;
    if (!bSuccess) { LastConsumer = nullptr; }
    if (Callback) { Callback(bSuccess, Duration); }
}

void UDCPhononSpeech::Shutdown()
{
    if (bStopped) { return; }
    bStopped = true;
    Cancel();
    if (Worker.IsValid()) { Worker->ShutdownRequested.Store(true); }
}

void UDCPhononSpeech::EndPlay(const EEndPlayReason::Type Reason)
{
    Shutdown();
    Super::EndPlay(Reason);
}

void UDCPhononSpeech::BeginDestroy()
{
    Shutdown();
    Super::BeginDestroy();
}
