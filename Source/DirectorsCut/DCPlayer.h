#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DCTypes.h"
#include "DCPlayer.generated.h"

class UCameraComponent;
class ACameraActor;

/**
 * First-person captain. Walks the deck, takes a station with E, and reads keys directly so the
 * project needs no input assets.
 *   Helm:    A/D steer.
 *   Cannons: mouse traverses the battery camera; click fires at the center sight.
 */
UCLASS()
class ADCPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADCPlayerCharacter();
	virtual void Tick(float DeltaSeconds) override;

	/** One line of help for the HUD, e.g. "E  take the helm". */
	const FString& GetPrompt() const { return Prompt; }
	int32 GetNearbyStation() const { return NearbyStation; }
	EDCSide GetGunSide() const { return ActiveSide; }
	bool HasGunSolution() const { return bGunSolution; }
	bool IsTypingOrder() const { return bTyping; }
	bool IsAtStation() const { return ActiveStation != INDEX_NONE; }
	/** Looking at the ship from outside (T). */
	bool IsShipView() const { return bShipView; }
	/** For unattended tests and debug commands. */
	void DebugTakeStation(EDCStationType Type, EDCSide Side);

	UPROPERTY(EditAnywhere) float LookSensitivity = 0.12f;
	UPROPERTY(EditAnywhere) float InteractRange = 300.f;

protected:
	void TickOnFoot(class APlayerController* PC);
	void TickHelm(APlayerController* PC, float DeltaSeconds);
	void RestoreWheel();
	void TickCannons(APlayerController* PC);
	void Look(APlayerController* PC);
	void TickMeta(APlayerController* PC);
	void TickCameraFeel(float DeltaSeconds);
	void TickGraphicsKeys(APlayerController* PC);
	int32 DLSSModeIndex = 0;
	bool bFrameGen = false;
	void EnterStation(APlayerController* PC, int32 Index);
	void LeaveStation(APlayerController* PC);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<ACameraActor> GunCamera;
	/** The ship seen from outside (T): a camera that stands off her quarter and can be swung round her with the mouse.
	 *  The captain keeps doing whatever he was doing; at the helm it is a good view to steer by. */
	UPROPERTY() TObjectPtr<ACameraActor> ShipCamera;
	bool bShipView = false;
	float OrbitYaw = 0.f;
	float OrbitPitch = 14.f;
	float OrbitDistance = 5200.f;
	void TickShipView(APlayerController* PC, float DeltaSeconds);
	void SetShipView(APlayerController* PC, bool bOn);
	int32 NearbyStation = INDEX_NONE;
	bool bGunSolution = false;
	bool bClearHelmView = false;
	TWeakObjectPtr<USceneComponent> HelmWheel;
	FQuat WheelRestRotation = FQuat::Identity;
	FVector WheelAxisInParent = FVector::ForwardVector;
	float WheelAngle = 0.f;

	int32 ActiveStation = INDEX_NONE;
	EDCStationType ActiveType = EDCStationType::None;
	EDCSide ActiveSide = EDCSide::None;
	FString Prompt;

	// Typed orders: Enter opens a text box, Enter again sends it to the crew.
	void OpenOrderBox(APlayerController* PC);
	void CloseOrderBox();
	TSharedPtr<class SWidget> OrderRoot;
	TSharedPtr<class SEditableTextBox> OrderBox;
	bool bTyping = false;
	float HoldTime = 0.f;

public:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
