#include "DCPhononSpeech.h"
#include "ACEAudioCurveSourceComponent.h"
#include "ACERuntimeModule.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "Containers/Queue.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Misc/Base64.h"
#include "Misc/CoreDelegates.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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
    TAtomic<int32> DeferredLeaseDeletes{0};
    // All registry/admission access is on the GT. Workers touch only atomics.
    bool bExitAdmissionClosed = false;
    bool bExitDrained = false;
    bool bWorldDrainInProgress = false;

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
            else
            {
                DeferredLeaseDeletes.IncrementExchange();
                AsyncTask(ENamedThreads::GameThread, [Value]
                {
                    delete Value;
                    DeferredLeaseDeletes.DecrementExchange();
                });
            }
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

    struct FWorker : FRunnable, TSharedFromThis<FWorker, ESPMode::ThreadSafe>
    {
        FWorker() : Completed(FPlatformProcess::GetSynchEventFromPool(true)) {}
        ~FWorker() override { FPlatformProcess::ReturnSynchEventToPool(Completed); }
        FEvent* Completed;
        TAtomic<int32> PendingGameThreadWork{0};
        FString Python, Script, Model, Voices;
        FACERuntimeModule* ACE = nullptr;
        TAtomic<bool> Ready{false}, Busy{false}, ShutdownRequested{false};
        TAtomic<int32> CancelId{0};
        TAtomic<int32> ReviewFaultRequested{0}, ReviewFaultApplied{0}, ProducerGeneration{0};
        bool bReviewFaultEnabled = false;
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
                ProducerGeneration.IncrementExchange();
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
            PendingGameThreadWork.IncrementExchange();
            AsyncTask(ENamedThreads::GameThread, [Self, Consumer = FLeasePtr(Consumer), bReleaseBusy]() mutable
            {
                if (Consumer.IsValid() && IsValid(Consumer->Consumer.Get()))
                {
                    Consumer->Consumer->Stop();
                    Self->ReuseAfterFrame.Store(GFrameCounter + 2);
                }
                if (bReleaseBusy) { Self->Busy.Store(false); }
                Consumer.Reset();
                Self->PendingGameThreadWork.DecrementExchange();
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
            PendingGameThreadWork.IncrementExchange();
            AsyncTask(ENamedThreads::GameThread, [Self, Consumer, Id, bSuccess, Duration, bAlreadyStopped]() mutable
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
                Consumer.Reset();
                Self->PendingGameThreadWork.DecrementExchange();
            });
        }

        uint32 Run() override
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
                const int32 FaultId = ReviewFaultRequested.Exchange(0);
                if (FaultId > 0)
                {
                    if (bReviewFaultEnabled && Active.Id == FaultId && Process.IsValid() && FPlatformProcess::IsProcRunning(Process))
                    {
                        // Exercise the ordinary process-death path below, never a synthetic success/failure callback.
                        // This handle was created by this worker; no PID search or unrelated process termination.
                        FPlatformProcess::TerminateProc(Process, false);
                        ReviewFaultApplied.Store(FaultId);
                        UE_LOG(LogDCPhononSpeech, Display, TEXT("PHONON_REVIEW_PRODUCER_FAULT request=%d generation=%d"), FaultId, ProducerGeneration.Load());
                    }
                    else { ReviewFaultApplied.Store(-FaultId); }
                }
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
            Completed->Trigger();
            return 0;
        }
    };

    struct FRegisteredWorker
    {
        TSharedPtr<FWorker, ESPMode::ThreadSafe> State;
        TUniquePtr<FRunnableThread> Thread;
        TFunction<void()> ShutdownOwner;
        TWeakObjectPtr<UWorld> World;
    };
    TArray<FRegisteredWorker> Workers;
    FDelegateHandle EnginePreExitHandle, PreExitHandle, WorldTearDownHandle;

    void DrainWorldTearDown(UWorld* World)
    {
        check(IsInGameThread());
        TArray<int32> Indices;
        TArray<TSharedPtr<FWorker, ESPMode::ThreadSafe>> States;
        for (int32 Index = 0; Index < Workers.Num(); ++Index)
        {
            if (Workers[Index].World.Get() == World)
            {
                Indices.Add(Index);
                States.Add(Workers[Index].State);
            }
        }
        if (Indices.IsEmpty()) { return; }
        if (bWorldDrainInProgress || bExitAdmissionClosed)
        {
            UE_LOG(LogDCPhononSpeech, Fatal, TEXT("PHONON_WORLD_DRAIN_UNSAFE: recursive world/application teardown with registered workers"));
            return;
        }
        // UE5.8 LoadMap broadcasts this before actor EndPlay, CleanupWorld and
        // TrimMemory's world-leak check. Async EndPlay alone releases leases too late.
        bWorldDrainInProgress = true;
        const FString WorldName = GetNameSafe(World);
        const double Started = FPlatformTime::Seconds();
        double NextWarning = Started + 5.0;
        UE_LOG(LogDCPhononSpeech, Display, TEXT("PHONON_WORLD_DRAIN_BEGIN world=%s workers=%d"), *WorldName, States.Num());
        for (int32 Index : Indices)
        {
            FRegisteredWorker& Entry = Workers[Index];
            if (Entry.ShutdownOwner) { Entry.ShutdownOwner(); }
            Entry.State->Ready.Store(false);
            Entry.State->ShutdownRequested.Store(true);
        }
        for (;;)
        {
            bool bOutstanding = DeferredLeaseDeletes.Load() != 0;
            for (const auto& State : States)
            {
                bOutstanding |= !State->Completed->Wait(0) || State->PendingGameThreadWork.Load() != 0;
            }
            if (!bOutstanding) { break; }
            // Never re-enter task processing or wait on GT work from inside a
            // GT task. Normal LoadMap is outside tasks. An unsupported caller
            // must fail explicitly rather than hang or continue into GC with leases.
            if (!FTaskGraphInterface::IsRunning() ||
                FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::GameThread))
            {
                UE_LOG(LogDCPhononSpeech, Fatal, TEXT("PHONON_WORLD_DRAIN_UNSAFE: world=%s requires GT cleanup outside task processing"), *WorldName);
                return;
            }
            // Only shared state snapshots survive the pump. Admission is closed,
            // and recursive teardown is rejected, so registry indices stay stable.
            FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
            const double Now = FPlatformTime::Seconds();
            if (Now >= NextWarning)
            {
                UE_LOG(LogDCPhononSpeech, Warning, TEXT("PHONON_WORLD_DRAIN_WAIT world=%s elapsed=%.1f; native callbacks must drain before world GC"), *WorldName, Now - Started);
                NextWarning = Now + 30.0;
            }
            FPlatformProcess::SleepNoStats(.001f);
        }
        // Frames cannot advance within this barrier. ReuseAfterFrame is irrelevant:
        // these consumers are being destroyed, never reused for another request.
        for (int32 Position = Indices.Num() - 1; Position >= 0; --Position)
        {
            FRegisteredWorker& Entry = Workers[Indices[Position]];
            Entry.Thread->WaitForCompletion();
            Entry.Thread.Reset();
            Entry.State->ACE = nullptr;
            Entry.State->Deliver = nullptr;
            Entry.ShutdownOwner = nullptr;
            Entry.World.Reset();
            Workers.RemoveAt(Indices[Position]);
        }
        bWorldDrainInProgress = false;
        UE_LOG(LogDCPhononSpeech, Display, TEXT("PHONON_WORLD_DRAIN_COMPLETE world=%s workers=%d elapsed=%.3f"),
            *WorldName, States.Num(), FPlatformTime::Seconds() - Started);
    }

    void DrainApplicationExit()
    {
        check(IsInGameThread());
        if (bExitDrained) { return; }
        if (bWorldDrainInProgress)
        {
            UE_LOG(LogDCPhononSpeech, Fatal, TEXT("PHONON_EXIT_DRAIN_UNSAFE: application exit re-entered a world drain"));
            return;
        }
        // No new producer can enter while pumping arbitrary GT tasks below.
        bExitAdmissionClosed = true;
        for (FRegisteredWorker& Entry : Workers)
        {
            // Owner cancellation also releases a successfully sent clip's
            // retained playback lease, before its worker is told to exit.
            if (Entry.ShutdownOwner) { Entry.ShutdownOwner(); }
            Entry.State->Ready.Store(false);
            Entry.State->ShutdownRequested.Store(true);
        }
        const double Started = FPlatformTime::Seconds();
        double NextWarning = Started + 5.0;
        UE_LOG(LogDCPhononSpeech, Display, TEXT("PHONON_EXIT_DRAIN_BEGIN workers=%d"), Workers.Num());
        for (;;)
        {
            bool bOutstanding = DeferredLeaseDeletes.Load() != 0;
            for (const FRegisteredWorker& Entry : Workers)
            {
                bOutstanding |= !Entry.State->Completed->Wait(0) || Entry.State->PendingGameThreadWork.Load() != 0;
            }
            if (!bOutstanding) { break; }
            // OnEnginePreExit runs from the engine's exit sequence, outside the
            // task graph. A raw GT join here would deadlock AudioCompReady.Wait.
            // Pumping lets ACE prepare audio and our Stop/lease cleanup finish
            // while engine, audio, task graph and ACE are all still available.
            if (FTaskGraphInterface::IsRunning() &&
                !FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::GameThread))
            {
                FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
            }
            const double Now = FPlatformTime::Seconds();
            if (Now >= NextWarning)
            {
                UE_LOG(LogDCPhononSpeech, Warning, TEXT("PHONON_EXIT_DRAIN_WAIT elapsed=%.1f; native drain must finish before ACE unload"), Now - Started);
                NextWarning = Now + 30.0;
            }
            // Never timeout into module unload with a live native call. The SDK
            // has no safe hard-abort primitive; a hung driver can delay exit.
            FPlatformProcess::SleepNoStats(.001f);
        }
        for (FRegisteredWorker& Entry : Workers)
        {
            // Completed is signaled after all native work. Join here also covers
            // the final return/FRunnable epilogue before its code can be unloaded.
            Entry.Thread->WaitForCompletion();
            Entry.Thread.Reset();
            Entry.State->ACE = nullptr;
            Entry.State->Deliver = nullptr;
            Entry.ShutdownOwner = nullptr;
        }
        Workers.Empty();
        bExitDrained = true;
        // Core delegates outlive the game module. Do not leave callable pointers
        // into this DLL registered after its workers and cleanup have drained.
        FCoreDelegates::OnEnginePreExit.Remove(EnginePreExitHandle);
        FCoreDelegates::OnPreExit.Remove(PreExitHandle);
        FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle);
        EnginePreExitHandle.Reset();
        PreExitHandle.Reset();
        WorldTearDownHandle.Reset();
        UE_LOG(LogDCPhononSpeech, Display, TEXT("PHONON_EXIT_DRAIN_COMPLETE elapsed=%.3f"), FPlatformTime::Seconds() - Started);
    }

    bool StartWorker(const TSharedRef<FWorker, ESPMode::ThreadSafe>& State, UWorld* World, TFunction<void()> ShutdownOwner)
    {
        check(IsInGameThread());
        if (bExitAdmissionClosed || bWorldDrainInProgress || !IsValid(World) || World->bIsTearingDown) { return false; }
        if (!EnginePreExitHandle.IsValid())
        {
            // UE5.8: UnrealEngine.cpp UEngine::PreExit broadcasts this BEFORE
            // audio/world/core teardown. OnPreExit is a fallback for exit paths
            // without a live engine; LaunchEngineLoop unloads modules afterward.
            EnginePreExitHandle = FCoreDelegates::OnEnginePreExit.AddStatic(&DrainApplicationExit);
            PreExitHandle = FCoreDelegates::OnPreExit.AddStatic(&DrainApplicationExit);
            WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddStatic(&DrainWorldTearDown);
        }
        TUniquePtr<FRunnableThread> Thread(FRunnableThread::Create(&State.Get(), TEXT("DCPhononPCM")));
        if (!Thread) { return false; }
        FRegisteredWorker Entry;
        Entry.State = State;
        Entry.Thread = MoveTemp(Thread);
        Entry.ShutdownOwner = MoveTemp(ShutdownOwner);
        Entry.World = World;
        Workers.Add(MoveTemp(Entry));
        return true;
    }
}

UDCPhononSpeech::UDCPhononSpeech()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UDCPhononSpeech::Initialize(const FString& PythonPath, const FString& ScriptPath,
    const FString& ModelDirectory, const FString& VoiceManifestPath)
{
    check(IsInGameThread());
    if (Worker.IsValid() || bStopped || DCPhononSpeechPrivate::bExitAdmissionClosed ||
        DCPhononSpeechPrivate::bWorldDrainInProgress || !GetWorld() || GetWorld()->bIsTearingDown) { return false; }
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
    FString ReviewMode;
    FParse::Value(FCommandLine::Get(), TEXT("DCPhononLifecycleReview="), ReviewMode);
    Worker->bReviewFaultEnabled = ReviewMode == TEXT("Failure");
    Worker->ACE = &FACERuntimeModule::Get();
    TWeakObjectPtr<UDCPhononSpeech> Weak(this);
    Worker->Deliver = [Weak](int32 Id, bool bSuccess, double Duration)
    {
        if (UDCPhononSpeech* Self = Weak.Get()) { Self->Delivered(Id, bSuccess, Duration); }
    };
    const auto State = Worker.ToSharedRef();
    if (!DCPhononSpeechPrivate::StartWorker(State, GetWorld(), [Weak]
    {
        if (UDCPhononSpeech* Self = Weak.Get()) { Self->Shutdown(); }
    }))
    {
        Worker.Reset();
        return false;
    }
    return true;
}

bool UDCPhononSpeech::IsReady() const { return !bStopped && Worker.IsValid() && Worker->Ready.Load(); }
bool UDCPhononSpeech::IsBusy() const
{
    check(IsInGameThread());
    return Worker.IsValid() && (Worker->Busy.Load() || GFrameCounter < Worker->ReuseAfterFrame.Load());
}

int32 UDCPhononSpeech::RequestProducerFailureForReview()
{
    check(IsInGameThread());
    if (bStopped || !Worker.IsValid() || !Worker->bReviewFaultEnabled || !ActiveId || !Worker->Busy.Load() ||
        Worker->ReviewFaultRequested.Load() || Worker->ReviewFaultApplied.Load()) { return 0; }
    Worker->ReviewFaultRequested.Store(ActiveId);
    return ActiveId;
}

void UDCPhononSpeech::GetProducerReviewState(int32& Generation, int32& AppliedFaultId) const
{
    check(IsInGameThread());
    Generation = Worker.IsValid() ? Worker->ProducerGeneration.Load() : 0;
    AppliedFaultId = Worker.IsValid() ? Worker->ReviewFaultApplied.Load() : 0;
}

TFunction<bool()> UDCPhononSpeech::CaptureReloadDrainProbeForReview() const
{
    check(IsInGameThread());
    FString Mode;
    FParse::Value(FCommandLine::Get(), TEXT("DCPhononLifecycleReview="), Mode);
    if (Mode != TEXT("Reload") || !Worker.IsValid()) { return nullptr; }
    const auto State = Worker;
    return [State]
    {
        check(IsInGameThread());
        return State->ShutdownRequested.Load() && State->Completed->Wait(0) &&
            State->PendingGameThreadWork.Load() == 0 && !State->Busy.Load() &&
            GFrameCounter >= State->ReuseAfterFrame.Load() && DCPhononSpeechPrivate::DeferredLeaseDeletes.Load() == 0;
    };
}

bool UDCPhononSpeech::Speak(const FString& Speaker, const FString& Text, UACEAudioCurveSourceComponent* Consumer,
    FName Provider, TFunction<void(bool, double)> Sent)
{
    check(IsInGameThread());
    if (DCPhononSpeechPrivate::bExitAdmissionClosed || DCPhononSpeechPrivate::bWorldDrainInProgress ||
        !IsReady() || IsBusy() || LastConsumer.Get() != nullptr || !IsValid(Consumer) || !Sent || Text.TrimStartAndEnd().IsEmpty() || Text.Len() > 240 ||
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
