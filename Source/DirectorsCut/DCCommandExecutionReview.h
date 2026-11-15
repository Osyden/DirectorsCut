#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCCommandExecutionReview.generated.h"

/** Opt-in real-world command execution and cannon-to-fire regression. */
UCLASS()
class UDCCommandExecutionReview : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
private:
    double Started=0, CaseStarted=0;
    int32 CaseIndex=0, OrdersBefore=0, PreviousStation=INDEX_NONE, Passed=0;
    float ScoreBefore=0;
    bool bPending=false, bFireWalk=false, bDone=false, bMoved=false, bVoiceChecked=false;
    FVector CannonStart=FVector::ZeroVector;
    void Finish(const FString& Error);
};
