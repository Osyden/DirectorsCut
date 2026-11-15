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
	LaneOffset = LaneVelocity = CurrentDrift = 0.f;
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
	UpdateLanePaths();
	const float Chosen = ChooseClearLane(17000.f);
	Check(Chosen <= -750.f && ForecastLaneImpact(Chosen) < 0.f,
		TEXT("planner chooses a reachable port route around both stones"));
	UE_LOG(LogDC, Display, TEXT("NAVIGATION VERIFY METRICS chosen=%.0f pathMs=%.3f"), Chosen, LanePathMilliseconds);

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
