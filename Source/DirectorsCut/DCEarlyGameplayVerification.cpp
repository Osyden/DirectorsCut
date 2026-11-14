#include "DCEarlyEncounters.h"
#include "DCPlayer.h"
#include "DCRideDirector.h"
#include "DCTroubles.h"
#include "DCDeckFireEffect.h"
#include "DCGunnery.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#if !UE_BUILD_SHIPPING
void ADCEarlyEncounters::TickEncounterVerification()
{
	// The captain's actual gun, reload and ballistic collision path must stop an
	// approaching attacker. Other threats are deliberately left to complete attacks.
	if (VerifyStage == 0 && !Threats.IsEmpty() && Threats[0].Target.IsValid())
	{
		FThreat& T = Threats[0];
		const int32 Gun = Guns->GetPlayerGun(EDCSide::Port);
		if (Guns->CanAimGun(Gun, T.Target->GetActorLocation()))
		{
			VerifyEnemy = T.Body; VerifyStage = 1;
		}
	}
	if (VerifyStage == 1 && VerifyEnemy.IsValid())
	{
		for (FThreat& T : Threats)
		{
			if (T.Body != VerifyEnemy || !T.Target.IsValid()) { continue; }
			const int32 Gun = Guns->GetPlayerGun(EDCSide::Port);
			const FVector Point = T.Target->GetActorLocation();
			const FVector View = Director->DeckToWorld(Guns->GetGunStation(Gun) + FVector(0.f, 80.f, 250.f));
			FVector Aim;
			if (Guns->ResolveAim(EDCSide::Port, View, (Point - View).GetSafeNormal(), Aim) && Guns->FireGun(Gun, Aim, true)) { ++VerifyShots; }
		}
	}
	if (VerifyStage < 2 && Director->RideTime >= 146.f)
	{
		auto Check = [this](bool bPassed, const TCHAR* What)
		{
			bVerifyFailed |= !bPassed;
			UE_LOG(LogDC, Display, TEXT("ENCOUNTER VERIFY %s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), What);
		};
		Check(bVerifyInterrupted, TEXT("captain cannon kills a moving creature before its attack releases"));
		Check(VerifyShots >= 3 && VerifyShots <= 4, TEXT("three-health target needs three hits with at most one missed shot"));
		Check(VerifyFireballImpacts > 0, TEXT("ignored creatures deliver visible projectiles to the deck"));
		Check(VerifyBeamImpacts > 0, TEXT("ignored statue warms up and ignites a deck point"));
		Check(Troubles->NumFires() <= Troubles->MaxFires, TEXT("overlapping attacks respect the deck fire cap"));
		UE_LOG(LogDC, Display, TEXT("ENCOUNTER VERIFY METRICS shots=%d fireballs=%d beams=%d"), VerifyShots, VerifyFireballImpacts, VerifyBeamImpacts);
		VerifyStage = 2;
		Director->Seek(241.f);
	}
	else if (VerifyStage == 2)
	{
		const bool bClean = Threats.IsEmpty() && Projectiles.IsEmpty();
		bVerifyFailed |= !bClean;
		UE_LOG(LogDC, Display, TEXT("ENCOUNTER VERIFY %s: phase exit removes encounter actors and projectiles"), bClean ? TEXT("PASS") : TEXT("FAIL"));
		UE_LOG(LogDC, Display, TEXT("ENCOUNTER VERIFICATION %s"), bVerifyFailed ? TEXT("FAILED") : TEXT("PASSED"));
		VerifyStage = 3;
		FPlatformMisc::RequestExitWithStatus(false, bVerifyFailed ? 1 : 0);
	}
}

void ADCEarlyEncounters::TickVerification(float DeltaSeconds)
{
	VerifyClock += DeltaSeconds;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ADCPlayerCharacter* Captain = PC ? Cast<ADCPlayerCharacter>(PC->GetPawn()) : nullptr;
	if (!Captain || Troubles->DeckPoints.IsEmpty()) { return; }
	auto Check = [this](bool bPassed, const TCHAR* What)
	{
		bVerifyFailed |= !bPassed;
		UE_LOG(LogDC, Display, TEXT("EARLY VERIFY %s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), What);
	};
	if (VerifyStage == 0 && GetWorld()->GetTimeSeconds() > 2.f)
	{
		VerifyFire = Troubles->DeckPoints[1];
		Troubles->StartFire(VerifyFire);
		Troubles->StartFire(VerifyFire + FVector(10, 0, 0));
		VerifyScore = Director->Score;
		Check(Troubles->NumFires() == 1, TEXT("duplicate impacts do not stack fires at one deck point"));
		Check(Troubles->SwatsRemaining(VerifyFire) == 3, TEXT("fresh fire needs three swats"));
		Check(!Troubles->SwatNear(VerifyFire + FVector(2000, 0, 0), 100.f), TEXT("out of reach swat does no work"));
		const FVector Stand = Director->DeckToWorld(VerifyFire + FVector(-155, 0, Captain->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 8.f));
		Captain->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
		Captain->GetCharacterMovement()->StopMovementImmediately();
		PC->SetControlRotation((Director->DeckToWorld(VerifyFire) - (Stand + FVector(0, 0, 64))).Rotation());
		VerifyStage = 1; VerifyClock = 0.f;
	}
	else if ((VerifyStage == 1 || VerifyStage == 3 || VerifyStage == 5) && VerifyClock > .5f)
	{
		Check(Captain->TrySwatFire(), TEXT("player begins a real rag swing within reach"));
		++VerifyStage; VerifyClock = 0.f;
	}
	else if ((VerifyStage == 2 || VerifyStage == 4) && VerifyClock > .4f)
	{
		Check(Troubles->NumFires() == 1 && Troubles->SwatsRemaining(VerifyFire) == (VerifyStage == 2 ? 2 : 1), TEXT("rag contacts retain partial suppression"));
		Check(Director->Score == VerifyScore, TEXT("partial swat awards no completion score"));
		++VerifyStage; VerifyClock = 0.f;
	}
	else if (VerifyStage == 6 && VerifyClock > .4f)
	{
		Check(Troubles->NumFires() == 0, TEXT("third animated rag contact extinguishes fire"));
		Check(Director->Score == VerifyScore + 50, TEXT("extinguishing awards exactly one score increment"));
		Check(!Troubles->SwatNear(VerifyFire) && !Troubles->ExtinguishNear(VerifyFire), TEXT("completed fire cannot be scored again"));
		VerifyStage = 7; VerifyClock = 0.f;
	}
	else if (VerifyStage == 7 && VerifyClock > 3.8f)
	{
		int32 Effects = 0;
		for (TActorIterator<ADCDeckFireEffect> It(GetWorld()); It; ++It) { ++Effects; }
		Check(Effects == 0, TEXT("extinguished fire effect cleans up after smoke fades"));
		Troubles->StartFire(VerifyFire);
		Check(Troubles->ExtinguishNear(VerifyFire) && Troubles->NumFires() == 0, TEXT("crew bucket extinguish still completes a fire"));
		UE_LOG(LogDC, Display, TEXT("EARLY GAMEPLAY VERIFICATION %s"), bVerifyFailed ? TEXT("FAILED") : TEXT("PASSED"));
		VerifyStage = 8;
		FPlatformMisc::RequestExitWithStatus(false, bVerifyFailed ? 1 : 0);
	}
}
#endif
