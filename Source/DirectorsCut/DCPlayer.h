#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DCTypes.h"
#include "DCPlayer.generated.h"

class UCameraComponent;
class ACameraActor;
class AStaticMeshActor;

/**
 * First-person captain. Walks the deck, takes a station with E, and reads keys directly so the
 * project needs no input assets.
 *   Helm:    A/D steer.
 *   Cannons: fixed side camera, mouse aims a marker on the sea, click fires the battery in an arc.
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
	bool IsAtStation() const { return ActiveStation != INDEX_NONE; }
	/** For unattended tests and debug commands. */
	void DebugTakeStation(EDCStationType Type, EDCSide Side);

	UPROPERTY(EditAnywhere) float LookSensitivity = 0.12f;
	UPROPERTY(EditAnywhere) float InteractRange = 300.f;

protected:
	void TickOnFoot(class APlayerController* PC);
	void TickHelm(APlayerController* PC);
	void TickCannons(APlayerController* PC);
	void Look(APlayerController* PC);
	void TickMeta(APlayerController* PC);
	void EnterStation(APlayerController* PC, int32 Index);
	void LeaveStation(APlayerController* PC);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<ACameraActor> GunCamera;
	UPROPERTY() TObjectPtr<AStaticMeshActor> AimMarker;

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
