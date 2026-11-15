#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCConversationReview.generated.h"

/** Opt-in end-to-end conversational review in the actual voyage. */
UCLASS()
class UDCConversationReview : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
private:
    UPROPERTY() TObjectPtr<class ADCVoice> Voice;
    UPROPERTY() TObjectPtr<class ADCRideDirector> Director;
    enum class EStage : uint8 { Ready, Drain, Ask, Generate, Speak, Done };
    EStage Stage = EStage::Ready;
    int32 CaseIndex = 0, OrdersBefore = 0;
    double Started = 0, StageStarted = 0, GeneratedMs = 0;
    bool bSawPlayback = false;
    FString Reply, Context, OutputPath;
    TArray<TSharedPtr<class FJsonValue>> Results;
    void Finish(const FString& Error);
};
