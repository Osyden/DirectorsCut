#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCTypes.h"
#include "DCDialogueReview.generated.h"

class ADCVoice;
class FJsonValue;

/** Opt-in, bounded comparison of the actual selected dialogue backend; executes no game orders. */
UCLASS()
class ADCDialogueReviewGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ADCDialogueReviewGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    struct FReviewCase
    {
        FString Label, User, System;
        bool bAcknowledgement = false;
        FDCIntent Expected;
        FString ForbiddenDirection;
    };
    void StartCase();
    void CompleteCase(int32 Index, bool bOK, const FString& Output);
    void AddHarnessFailure(const FString& Reason);
    void Finish();

    UPROPERTY() TObjectPtr<ADCVoice> Voice;
    TArray<FReviewCase> Cases;
    TArray<TSharedPtr<FJsonValue>> Results;
    FString Backend, ReportPath;
    double Started = 0, ReadyMilliseconds = 0, RequestStarted = 0;
    float RequestTimeout = 0;
    int32 CaseIndex = 0, Failures = 0;
    bool bReady = false, bPending = false, bCompleted = false;
};
