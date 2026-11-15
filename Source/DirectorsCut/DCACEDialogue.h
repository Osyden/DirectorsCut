#pragma once

#include "CoreMinimal.h"
#include "ACELLMComponent.h"
#include "DCACEDialogue.generated.h"

/** One serialized NVIDIA dialogue queue. Timeouts discard replies without freeing live CUDA work. */
UCLASS()
class UDCACEDialogue : public UACELLMComponent
{
    GENERATED_BODY()
public:
    UDCACEDialogue();
    void Initialize(const FString& ModelPath);
    void Submit(const FString& System, const FString& User, float Temperature, float Timeout,
        TFunction<void(bool, const FString&)> Completion, TFunction<void(const FString&)> Progress = nullptr);
    void CancelPending();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    struct FRequest
    {
        FString System, User;
        float Temperature = 0;
        double Deadline = 0;
        TFunction<void(bool, const FString&)> Completion;
        TFunction<void(const FString&)> Progress;
        FString LastProgress;
    };
    TArray<FRequest> Pending;
    TOptional<FRequest> Active;
    bool bStopped = false;
    void StartNext();
    void ExpireActive();
    UFUNCTION() void Streamed(const FString& Token, const FString& PartialText);
    UFUNCTION() void Received(const FString& Text, ULLMChatMessage* Message);
    UFUNCTION() void Failed(const FString& Error);
};
