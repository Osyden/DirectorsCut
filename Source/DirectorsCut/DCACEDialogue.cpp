#include "DCACEDialogue.h"
#include "DCTypes.h"
#include "Misc/Paths.h"

UDCACEDialogue::UDCACEDialogue()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoInitialize = false;
    bSaveChatHistory = false;
    bAutoRunTools = false;
    bDestroyEngineOnEndPlay = false;
    // Character cards, live cast/scene state and bounded conversational history.
    MaxContextSize = 4096;
    MaxHistoryMessages = 4;
    InferenceDefaults.EnableThinkMode = false;
    InferenceDefaults.OutputThinkTokens = false;
}

void UDCACEDialogue::Initialize(const FString& ModelPath)
{
    OnResponseComplete.AddUniqueDynamic(this, &UDCACEDialogue::Received);
    OnLLMError.AddUniqueDynamic(this, &UDCACEDialogue::Failed);
    ModelFileOverride.FilePath = FPaths::ConvertRelativePathToFull(ModelPath);
    if (!FPaths::FileExists(ModelFileOverride.FilePath))
    {
        UE_LOG(LogDC, Warning, TEXT("Dialogue: ACE model missing: %s"), *ModelFileOverride.FilePath);
        bStopped = true;
        return;
    }
    InitializeLLMAsync();
}

void UDCACEDialogue::Submit(const FString& System, const FString& User, float Temperature, float Timeout,
    TFunction<void(bool, const FString&)> Completion)
{
    if (bStopped || !IsLLMReady() || Pending.Num() >= 4)
    {
        Completion(false, FString());
        return;
    }
    FRequest Request;
    Request.System = System;
    Request.User = User;
    Request.Temperature = Temperature;
    Request.Deadline = FPlatformTime::Seconds() + FMath::Max(.1f, Timeout);
    Request.Completion = MoveTemp(Completion);
    Pending.Add(MoveTemp(Request));
    StartNext();
}

void UDCACEDialogue::StartNext()
{
    if (bStopped || Active.IsSet() || IsBusy() || !Pending.Num()) { return; }
    Active.Emplace(MoveTemp(Pending[0]));
    Pending.RemoveAt(0);
    SystemPrompt = Active->System;
    ResetConversation();
    InferenceDefaults.Temperature = Active->Temperature;
    RefreshInferenceOptions();
    SendMessageWithOptions(Active->User, nullptr, SystemPrompt);
}

void UDCACEDialogue::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (bStopped) { return; }
    const double Now = FPlatformTime::Seconds();
    // Remove before invoking callbacks: a fallback can enqueue another acknowledgement.
    TArray<TFunction<void(bool, const FString&)>> Expired;
    for (int32 I = Pending.Num()-1; I >= 0; --I)
    {
        if (Pending[I].Deadline > Now) { continue; }
        Expired.Add(MoveTemp(Pending[I].Completion));
        Pending.RemoveAt(I);
    }
    for (auto& Completion : Expired) { Completion(false, FString()); }
    if (Active.IsSet() && Active->Completion && Active->Deadline <= Now)
    {
        auto Completion = MoveTemp(Active->Completion);
        UE_LOG(LogDC, Warning, TEXT("Dialogue: ACE request timed out; retaining busy context until inference completes"));
        Completion(false, FString());
    }
    StartNext();
}

void UDCACEDialogue::Received(const FString& Text, ULLMChatMessage*)
{
    if (!Active.IsSet()) { return; }
    auto Completion = MoveTemp(Active->Completion);
    Active.Reset();
    if (!bStopped && Completion) { Completion(!Text.IsEmpty(), Text); }
    StartNext();
}

void UDCACEDialogue::Failed(const FString& Error)
{
    UE_LOG(LogDC, Warning, TEXT("Dialogue: ACE error: %s"), *Error);
    auto Completion = Active.IsSet() ? MoveTemp(Active->Completion) : TFunction<void(bool, const FString&)>();
    Active.Reset();
    if (!bStopped && Completion) { Completion(false, FString()); }
}

void UDCACEDialogue::CancelPending()
{
    Pending.Empty();
    // The plugin has no chat-abort API. Keep the context/chat alive and suppress its late result.
    if (Active.IsSet()) { Active->Completion = nullptr; }
}

void UDCACEDialogue::EndPlay(const EEndPlayReason::Type Reason)
{
    bStopped = true;
    CancelPending();
    Super::EndPlay(Reason);
}
