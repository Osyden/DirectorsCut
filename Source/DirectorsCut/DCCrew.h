#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCTypes.h"
#include "DCCrewNavigation.h"
#include "DCCrew.generated.h"

class ADCCrewMember;
class ADCTreadmill;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Text -> intent. Deterministic keyword grammar; the LLM path produces the same struct. */
namespace DCIntentParser
{
	FDCIntent Parse(const FString& Text, const TArray<FString>& CrewNames);
}

/** A place on deck where work is done. */
struct FDCStation
{
	EDCStationType Type = EDCStationType::None;
	EDCSide Side = EDCSide::None;
	FVector Location = FVector::ZeroVector;
	TWeakObjectPtr<ADCCrewMember> Operator;
	TWeakObjectPtr<ADCCrewMember> AssignedGunner;
	bool bPlayerOperated = false;
	int32 GunIndex = INDEX_NONE;
	bool bPlayerGun = true;
};

UENUM()
enum class EDCCrewState : uint8 { Idle, Moving, Operating };

/** Placeholder crew member: a capsule with a name tag and a plain state machine. */
UCLASS()
class ADCCrewMember : public AActor
{
	GENERATED_BODY()

public:
	ADCCrewMember();
	virtual void Tick(float DeltaSeconds) override;
	/** Where this hand stands on the ship at rest. All the crew's own reckoning is done there; Tick
	 *  then puts the body where the deck is this frame, so the hands move with the ship as she rides the sea. */
	FVector DeckPos = FVector::ZeroVector;
	FRotator DeckRot = FRotator::ZeroRotator;
	FVector LastPlaced = FVector(FLT_MAX);
	void TickCrew(float DeltaSeconds);

	void Init(const FString& InName, const FLinearColor& Color);
	void AssignStation(int32 StationIndex);
	/** Release a player-claimed post and move clear of its first-person view now. */
	void VacateStation(const FVector& StationLocation);
	void SetHelmOrder(EDCAction Action);
	void SetHoldFire(bool bHold) { bHoldFire = bHold; }
	bool IsHoldingFire() const { return bHoldFire; }

	UPROPERTY(VisibleAnywhere) FString CrewName;
	EDCCrewState State = EDCCrewState::Idle;
	int32 StationIndex = INDEX_NONE;

protected:
	void TickHelm(float DeltaSeconds, ADCTreadmill* Treadmill);
	void TickCannons(float DeltaSeconds, ADCTreadmill* Treadmill, EDCSide Side);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class USkeletalMeshComponent> Figure;
	UPROPERTY() TObjectPtr<class UAnimationAsset> IdleAnim;
	UPROPERTY() TObjectPtr<class UAnimationAsset> WalkAnim;
	bool bWalking = false;
	void SetWalking(bool bWalk);
	bool MoveToward(const FVector& Target, float DeltaSeconds);
	void TickTroubles(float DeltaSeconds, struct FDCStation* Station);
	float WorkTime = 0.f;
	void CancelFireWork();
	float CrewSwatAge = 10.f;
	bool bCrewSwatContact = false;
	FVector CrewSwatTarget = FVector::ZeroVector;

	TArray<FVector> WalkRoute;
	FVector RouteTarget = FVector(FLT_MAX);
	int32 RoutePoint = 0;
	float RouteRetry = 0.f;
	float WalkingSpeed = 0.f;
	bool bDeckGrounded = false;
	float MoveSpeed = 180.f;
	EDCAction HelmOrder = EDCAction::Steady;
	float HelmOrderTimeLeft = 0.f;
	bool bHoldFire = false;
	float ReloadLeft = 0.f;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FDCOnCrewLine, const FString& /*Speaker*/, const FString& /*Text*/);

/** Routes orders to crew and owns the station list. */
UCLASS()
class UDCCrewSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Entry point for every order, whatever the source. Returns the spoken reply. */
	FString ExecuteOrder(const FString& Text);
	void ExecuteIntent(const FDCIntent& Intent);

	void AddStation(EDCStationType Type, EDCSide Side, const FVector& Location);
	ADCCrewMember* SpawnCrew(const FString& Name, const FLinearColor& Color, const FVector& Location);

	int32 FindStation(EDCStationType Type, EDCSide Side) const;
	int32 FindCrewGun(EDCSide Side, const FString& Name) const;
	FDCStation* GetStation(int32 Index) { return Stations.IsValidIndex(Index) ? &Stations[Index] : nullptr; }
	const TArray<FDCStation>& GetStations() const { return Stations; }
	const TArray<TObjectPtr<ADCCrewMember>>& GetCrew() const { return Crew; }
	bool ProjectToDeck(const FVector& Feet, FVector& OnDeck);
	bool FindDeckRoute(const FVector& FromFeet, const FVector& ToFeet, TArray<FVector>& Route);
	bool GroundFeet(FVector& Feet);

	void Say(const FString& Speaker, const FString& Text);
	/** A crew answer to the current order: goes through the voice loop when it is running. */
	void Reply(const FString& Speaker, const FString& Text);
	/** An unprompted warning from the crew ("Rocks dead ahead!"). Rate-limited unless urgent. */
	/** True when admitted; callers may retry a rate-limited warning. Predicate must use weak world/target references. */
	bool Report(const FString& Text, bool bUrgent = false, TFunction<bool()> IsStillRelevant = {});

	FDCOnCrewLine OnCrewLine;
	/** Last few lines, newest last. Read by the HUD for subtitles. */
	TArray<FString> RecentLines;
	int32 OrdersGiven = 0;

protected:
	ADCCrewMember* PickCrew(const FString& Name, int32 ForStation);
	ADCCrewMember* EnsureOperator(int32 StationIdx, const FString& PreferredName);

	TArray<FDCStation> Stations;
	FString CurrentOrderText;
	double LastReportTime = -100.0;
	EDCPhase LastReportPhase = EDCPhase::OpenSea;
	int32 NextReporter = 0;
	UPROPERTY() TArray<TObjectPtr<ADCCrewMember>> Crew;
	FDCCrewNavigation Navigation;
	bool bNavigationAttempted = false;
	double NextNavigationAttempt = 0;
	bool EnsureNavigation();
};
