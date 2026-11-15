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

protected:
	void DrawCrewPanel();
	void DrawSpeechBubble();
	FString LastBubbleLine;
	double BubbleUntil = 0;
	/** Gameplay seconds only: menu waiting and paused time do not consume the voice hint. */
	double VoiceHintStartedAt = -1;
	bool bVoiceHintDismissed = false;
	void DrawCrewDutyIcon(EDCStationType Duty, float X, float Y, float Size, const FLinearColor& Color, bool bMirror = false);
	UPROPERTY() TArray<TObjectPtr<class UTexture2D>> CrewPortraits;
	bool bCrewPortraitsLoaded = false;
	/** Text drawn from the engine's vector font at the size it is shown, so it stays sharp at any resolution (the stock HUD fonts are small bitmaps stretched up). */
	void CrispText(const FString& Str, FLinearColor Color, float X, float Y, UFont* SizeOf, float Scale);
	void CrispSize(const FString& Str, float& OutWidth, float& OutHeight, UFont* SizeOf, float Scale);
	UPROPERTY() TObjectPtr<UFont> VectorFont;
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	bool IsAwaitingStart() const { return bAwaitingStart; }
	void StartVoyage();

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
	void ShowMainMenu();
	void TickMainMenu(float DeltaSeconds);
	void SetMenuInfoPage(int32 Page);
	int32 MenuInfoPage = 0;
	float MenuWaterReadyAge = -1.f;
	float MenuReveal = 0.f;
	int32 MenuWaterReadyFrames = 0;
	UPROPERTY() TObjectPtr<AActor> MenuWaterManager;
	bool bAwaitingStart = false;
	bool bMenuCaptured = false;
	float MenuAge = 0.f;
	TSharedPtr<class SWidget> MainMenuWidget;
	UPROPERTY() TObjectPtr<class ACameraActor> MenuCamera;
	FVector DeckAt(float X, float Y, float TopZ) const;
	FVector ShipPart(const TCHAR* Name, const FVector& Fallback) const;

	UPROPERTY() TObjectPtr<AActor> Ship;
	/** The ship rides the sea: heave, pitch and roll, eased toward the plane of the water under the hull. */
	float ShipHeave = 0.f;
	FVector2D ShipSlope = FVector2D::ZeroVector;
	float ShipTurn = 0.f;
	/** A blow to the hull: roll, pitch, yaw (degrees) and how fast each is changing; a damped spring brings her back. */
	FVector JoltAngle = FVector::ZeroVector;
	FVector JoltSpeed = FVector::ZeroVector;
	void TickShipMotion(float DeltaSeconds);
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
	float BotWait = 3.f;
	int32 BotStage = 0;
	bool bBotOdessaOnFire = false;
	bool bBotMarlowOnRigging = false;
};
