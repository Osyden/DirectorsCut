#include "DCTreadmill.h"
#include "DCRideDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

#if !UE_BUILD_SHIPPING
void ADCTreadmill::VerifyNavigation()
{
	// Engine collision regression: both destination lanes are clear, but a pillar
	// lies across only one turning route. This exercises real sweeps, not a mock.
	bool bFailed = false;
	auto Check = [&bFailed](bool bPassed, const TCHAR* What)
	{
		bFailed |= !bPassed;
		UE_LOG(LogDC, Display, TEXT("NAVIGATION VERIFY %s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), What);
	};
	LaneOffset = LaneVelocity = LaneAcceleration = CurrentDrift = 0.f;
	const float Speed = Director->GetEnv().ShipSpeed * Director->TimeScale;
	const float Z = Director->SeaLevelZ + 140.f;
	auto Pillar = [this, Z](float X, float Y)
	{
		AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>();
		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetCollisionObjectType(ECC_Destructible);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Actor->SetActorScale3D(FVector(4.f, 5.f, 12.f));
		Actor->SetActorLocation(FVector(X, Y, Z));
		return Actor;
	};
	AStaticMeshActor* CrossingStone = Pillar(Speed * 2.5f, 1400.f);
	FCollisionQueryParams Query(TEXT("DCNavigationVerify"), true);
	FHitResult Hit;
	const bool bDestinationBlocked = GetWorld()->LineTraceSingleByObjectType(Hit,
		FVector(-ShipHalfLength, 3000.f, Z), FVector(16000.f, 3000.f, Z), FCollisionObjectQueryParams(ECC_Destructible), Query);
	Check(!bDestinationBlocked, TEXT("starboard destination is clear along its full straight lane"));
	Check(ForecastLaneImpact(3000.f) >= 0.f, TEXT("hull sweep detects intervening stone on the starboard crossing"));
	Check(ForecastLaneImpact(-3000.f) < 0.f, TEXT("opposite crossing remains usable"));
	Check(ForecastLaneImpact(0.f) < 0.f, TEXT("holding the current clear lane remains usable"));
	CrossingStone->SetActorLocation(FVector(Speed * 2.5f, -1400.f, Z));
	Check(ForecastLaneImpact(-3000.f) >= 0.f && ForecastLaneImpact(3000.f) < 0.f,
		TEXT("mirrored obstacle reverses which crossing is blocked"));
	CrossingStone->SetActorLocation(FVector(Speed * 2.5f, 1400.f, Z));
	AStaticMeshActor* AheadStone = Pillar(Speed * 4.5f, 0.f);
	// The original center-distance fixture leaves under two seconds before the
	// bow contacts this stone. With carried steering that is genuinely too late:
	// keep it as an impossible-escape regression, not an expected safe route.
	bool bLateEscape = false;
	for (float Candidate = -LaneLimit; Candidate <= LaneLimit; Candidate += 250.f)
	{
		bLateEscape |= ForecastLaneImpact(Candidate) < 0.f;
	}
	Check(!bLateEscape, TEXT("late stone correctly has no physically reachable escape under inertia"));
	// Measure warning from the padded bow to the stone's near face, rather
	// than from the ship center. The crossing stone and every sweep stay real.
	const float AheadX = ShipHalfLength + 150.f + 200.f + Speed * 3.25f;
	AheadStone->SetActorLocation(FVector(AheadX, 0.f, Z));
	UpdateLanePaths();
	const float Chosen = ChooseClearLane(17000.f);
	Check(Chosen <= -750.f && ForecastLaneImpact(Chosen) < 0.f,
		TEXT("planner chooses a reachable port route around both stones"));
	UE_LOG(LogDC, Display, TEXT("NAVIGATION VERIFY METRICS chosen=%.0f pathMs=%.3f"), Chosen, LanePathMilliseconds);

	// Nearby ruin clusters have conservative circular steering envelopes. They
	// can overlap a destination even though the moving hull passes them before
	// turning into that lane. Confirm that several such penalties cannot outweigh
	// a real impending collision when a swept route through the gap is clear.
	const float SavedLimit = LaneLimit;
	LaneLimit = 2000.f;
	const int32 SavedObjects = Objects.Num();
	TArray<AStaticMeshActor*> RearStones;
	for (int32 I = 0; I < 3; ++I)
	{
		const FVector Position(-1800.f + I * 200.f, -1700.f - I * 650.f, Z);
		AStaticMeshActor* Stone = Pillar(Position.X, Position.Y);
		RearStones.Add(Stone);
		FDCWorldObject& Scenery = Objects.AddDefaulted_GetRef();
		Scenery.Actor = Stone; Scenery.Base = Position; Scenery.Radius = 1000.f;
		Scenery.bFeelable = true;
	}
	UpdateLanePaths();
	Check(ForecastLaneImpact(-1500.f) < 0.f && ForecastLaneImpact(0.f) >= 0.f,
		TEXT("rear scenery envelopes leave a swept port escape from the blocked course"));
	const float EnvelopeChoice = ChooseClearLane(17000.f);
	Check(ForecastLaneImpact(EnvelopeChoice) < 0.f,
		TEXT("clear swept route takes priority over accumulated scenery-envelope costs"));
	UE_LOG(LogDC, Display, TEXT("NAVIGATION VERIFY ENVELOPES chosen=%.0f hit=%.2f cost=%.2f clearCost=%.2f"),
		EnvelopeChoice, ForecastLaneImpact(EnvelopeChoice), ScoreLane(EnvelopeChoice, 17000.f), ScoreLane(-1500.f, 17000.f));
	AvoidanceLane = 0.f; bHasAvoidance = true; AvoidanceHold = 2.4f;
	AvoidanceCandidateTime = 0.f; StoneLookTimer = 0.f;
	Check(ForecastLaneImpact(0.f) >= 3.f, TEXT("reachable escape is detected with at least three seconds of bow-contact warning"));
	TickStrikes(.2f);
	Check(AvoidanceLane < 0.f && ForecastLaneImpact(AvoidanceLane) < 0.f,
		TEXT("proven safe escape immediately cancels a held blocked course while steering time remains"));
	for (int32 Probe = 0; Probe < 5; ++Probe) { TickStrikes(.2f); }
	Check(FindClearLane(17000.f) < 0.f && ForecastLaneImpact(FindClearLane(17000.f)) < 0.f,
		TEXT("committed crew course accepts the safer route despite its higher envelope cost"));
	for (AStaticMeshActor* Stone : RearStones) { Stone->Destroy(); }
	Objects.SetNum(SavedObjects);
	LaneLimit = SavedLimit;

	// A destination's straight ray can see a pillar already astern of the
	// turning hull. That must not override a held, swept-clear course.
	CrossingStone->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AheadStone->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaneOffset = LaneVelocity = LaneAcceleration = CurrentDrift = 0.f;
	AStaticMeshActor* AsternStone = Pillar(-1900.f, 1500.f);
	FDCWorldObject& Astern = Objects.AddDefaulted_GetRef();
	Astern.Actor = AsternStone; Astern.Base = FVector(-1900.f, 1500.f, Z);
	Astern.Radius = 1000.f; Astern.bFeelable = true;
	AvoidanceLane = 1500.f; bHasAvoidance = true; AvoidanceHold = 2.4f;
	AvoidanceCandidateTime = 0.f; StoneLookTimer = 0.f;
	const float HeldPath = ForecastLaneImpact(AvoidanceLane);
	const bool bRearRayHit = GetWorld()->LineTraceSingleByObjectType(Hit,
		FVector(-ShipHalfLength, 1500.f, Z), FVector(16000.f, 1500.f, Z), FCollisionObjectQueryParams(ECC_Destructible), Query);
	Check(bRearRayHit && Hit.ImpactPoint.X < 0.f && HeldPath < 0.f,
		TEXT("astern pillar obstructs the destination ray but leaves the turning hull clear"));
	TickStrikes(.2f);
	UE_LOG(LogDC, Display, TEXT("NAVIGATION VERIFY ASTERN path=%.2f rayX=%.0f held=%.0f hold=%.2f"),
		HeldPath, Hit.ImpactPoint.X, AvoidanceLane, AvoidanceHold);
	Check(FMath::IsNearlyEqual(AvoidanceLane, 1500.f) && AvoidanceHold > 0.f,
		TEXT("astern destination-ray hit cannot interrupt a clear committed turn"));
	// A real imminent collision must still cancel that hold without waiting.
	AsternStone->SetActorLocation(FVector(ShipHalfLength + 1200.f, 0.f, Z));
	Astern.Base = AsternStone->GetActorLocation();
	LaneOffset = LaneVelocity = LaneAcceleration = CurrentDrift = 0.f;
	AvoidanceLane = 0.f; AvoidanceHold = 2.4f; AvoidanceCandidateTime = 0.f; StoneLookTimer = 0.f;
	const float Imminent = ForecastLaneImpact(0.f);
	Check(Imminent >= 0.f && Imminent < 1.8f, TEXT("forward pillar creates an imminent swept collision"));
	TickStrikes(.2f);
	Check(FMath::Abs(AvoidanceLane) > 250.f,
		TEXT("imminent swept collision interrupts the course hold immediately"));
	AsternStone->SetActorLocation(FVector(4600.f, 0.f, Z));
	Astern.Base = AsternStone->GetActorLocation();
	LaneOffset = LaneVelocity = LaneAcceleration = CurrentDrift = 0.f;
	AvoidanceLane = 0.f; AvoidanceHold = 2.4f; AvoidanceCandidateTime = 0.f; StoneLookTimer = 0.f;
	Check(ForecastLaneImpact(0.f) >= 1.8f,
		TEXT("farther pillar gives more than 1.8 seconds of swept warning"));
	TickStrikes(.2f);
	Check(FMath::Abs(AvoidanceLane) > 250.f,
		TEXT("blocked course retains the earlier straight-probe emergency lead time"));
	AsternStone->Destroy(); Objects.SetNum(SavedObjects);
	CrossingStone->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// A crew destination across the obstacle previously overrode the physical
	// contact side and shouldered the hull further into the stone.
	CrossingStone->SetActorLocation(FVector(1460.f, 400.f, Z));
	FDCWorldObject& Contact = Objects.AddDefaulted_GetRef();
	Contact.Actor = CrossingStone; Contact.bFeelable = true;
	LaneVelocity = 500.f; CurrentDrift = 200.f; AvoidanceLane = 3000.f;
	bHasAvoidance = true; AvoidanceHold = 2.4f; StoneLookTimer = 1.f;
	StoneAhead.Reset(); LanePathImpact.Reset();
	TickStrikes(.016f);
	Check(LaneOffset < 0.f && LaneVelocity <= 0.f,
		TEXT("starboard contact separates to port despite an opposing crew destination"));
	CrossingStone->Destroy(); AheadStone->Destroy();
	UE_LOG(LogDC, Display, TEXT("NAVIGATION VERIFICATION %s"), bFailed ? TEXT("FAILED") : TEXT("PASSED"));
	FPlatformMisc::RequestExitWithStatus(false, bFailed ? 1 : 0);
}
#endif
