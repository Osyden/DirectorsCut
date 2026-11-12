#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"
#include "DCTypes.h"
#include "DCGame.generated.h"

class UCameraComponent;
class ADCRideDirector;
class ADCTreadmill;

/** Debug HUD: phase, ride time, hull, crew subtitles. */
UCLASS()
class ADCHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};

/**
 * Builds the whole ride at runtime: ship, director, treadmill, stations and crew.
 * The map itself can be empty.
 */
UCLASS(Config = Game)
class ADCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADCGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Blueprint class of the ship. Not edited by us; spawned as-is at the origin. */
	UPROPERTY(Config, EditAnywhere) FSoftClassPath ShipClass;
	/** Used when the map already contains the ship: an actor with this tag is adopted instead of spawning. */
	UPROPERTY(Config, EditAnywhere) FName ShipTag = TEXT("DCShip");
	UPROPERTY(Config, EditAnywhere) FVector ShipSpawnLocation = FVector::ZeroVector;
	UPROPERTY(Config, EditAnywhere) FRotator ShipSpawnRotation = FRotator::ZeroRotator;
	UPROPERTY(Config, EditAnywhere) float SeaLevelZ = 0.f;
	UPROPERTY(Config, EditAnywhere) bool bEnableDLSS = true;
	/** 67 = DLSS Quality, 58 = Balanced, 50 = Performance, 100 = DLAA. */
	UPROPERTY(Config, EditAnywhere) float DLSSScreenPercentage = 67.f;
	UPROPERTY(Config, EditAnywhere) bool bEnableFrameGeneration = false;

protected:
	FVector DeckAt(float X, float Y, float TopZ) const;
	FVector ShipPart(const TCHAR* Name, const FVector& Fallback) const;

	UPROPERTY() TObjectPtr<AActor> Ship;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	UPROPERTY() TObjectPtr<ADCTreadmill> Treadmill;

	// Unattended visual test: -DCAutoShots=5,70,150 seeks to each ride time, saves a screenshot, then quits.
	// Optional -DCShotCam=X,Y,Z,Pitch,Yaw views from a fixed external camera instead of the captain.
	TArray<float> AutoShotTimes;
	int32 AutoShotIndex = 0;
	float AutoShotWait = 0.f;
	bool bAutoShotTaken = false;
	FString TestWavPath;
	float SmoothedFps = 60.f;
	/** -DCOrdersLate=a_b;c_d : orders given one at a time once the language model is up. */
	TArray<FString> LateOrders;
	float LateOrderWait = 0.f;
};
