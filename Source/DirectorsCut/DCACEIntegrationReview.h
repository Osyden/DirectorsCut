#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCACEIntegrationReview.generated.h"

class ADCVoice;
class ADCRideDirector;

/** Opt-in real-voyage test: generated dialogue, Piper, GPU A2F, scene cancellation and reload. */
UCLASS()
class UDCACEIntegrationReview : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
private:
    enum class EStage : uint8 { Ready, Drain, Generate, Generating, Speaking, SceneCancel, SceneDrain, Restart, Restarting, Done };
    void Generate();
    void CancelAcrossScene();
    void RestartWithPendingRequest();
    void Fail(const FString& Reason);
    void SaveReport(bool bFinal);
    void Succeed();
    UPROPERTY() TObjectPtr<ADCVoice> Voice;
    UPROPERTY() TObjectPtr<ADCRideDirector> Director;
    EStage Stage=EStage::Ready;
    double WorldStarted=0, StageStarted=0, GeneratedMilliseconds=0;
    int32 WorldCycle=0, CrewIndex=0, PreviousEpoch=0;
    FString GeneratedLine;
    bool bHeldRide=false, bSawACEPlayback=false, bSceneSpeechCleared=false, bEnded=false;
};
