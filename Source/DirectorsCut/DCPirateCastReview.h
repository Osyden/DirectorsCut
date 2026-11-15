#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCPirateCastReview.generated.h"

/** Deterministic review scene for the four cast bindings; opt in via ?game=. */
UCLASS()
class ADCPirateCastReviewGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADCPirateCastReviewGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
private:
	UPROPERTY() TObjectPtr<class ACameraActor> ReviewCamera;
	float Elapsed = 0.f;
	int32 Stage = 0;
	FString Output;
};
