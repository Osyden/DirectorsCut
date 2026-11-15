#include "DCEarlyEncounters.h"
#include "DCEarlyEncounterPolicy.h"
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
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING
void ADCEarlyEncounters::TickEncounterVerification()
{
	const bool StatueReview=FParse::Param(FCommandLine::Get(),TEXT("DCStatueVerify"));
	// The captain's actual gun, reload and ballistic collision path must stop an
	// approaching attacker. Other threats are deliberately left to complete attacks.
	if (VerifyStage == 0)
	{
		for(FThreat& T:Threats)
		{
			if(T.bStatue!=StatueReview || !T.Target.IsValid()) { continue; }
			const int32 Gun=Guns->GetPlayerGun(T.Side<0?EDCSide::Port:EDCSide::Starboard);
			if(Guns->HasClearShot(Gun,T.Target.Get())) { VerifyEnemy=T.Body; VerifyStage=1; break; }
		}
	}
	if (VerifyStage == 1 && VerifyEnemy.IsValid())
	{
		for (FThreat& T : Threats)
		{
			if (T.Body != VerifyEnemy || !T.Target.IsValid()) { continue; }
			const EDCSide Side=T.Side<0?EDCSide::Port:EDCSide::Starboard;
			const int32 Gun = Guns->GetPlayerGun(Side);
			const FVector Point = T.Target->GetActorLocation();
			const FVector View = Director->DeckToWorld(Guns->GetGunStation(Gun) + FVector(0.f, 80.f, 250.f));
			FVector Aim;
			if (Guns->ResolveAim(Side, View, (Point - View).GetSafeNormal(), Aim) && Guns->FireGun(Gun, Aim, true)) { ++VerifyShots; }
		}
	}
	// A particular statue may correctly lose its firing window behind masonry.
	// Exercise all three opportunities rather than requiring the first to fire
	// through whatever scenery the natural route presents.
	if (VerifyStage < 2 && Director->RideTime >= 236.f)
	{
		auto Check = [this](bool bPassed, const TCHAR* What)
		{
			bVerifyFailed |= !bPassed;
			UE_LOG(LogDC, Display, TEXT("ENCOUNTER VERIFY %s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), What);
		};
		Check(bVerifyInterrupted, StatueReview?TEXT("captain cannon silences a statue before its beam releases"):TEXT("captain cannon kills a moving creature before its attack releases"));
		const int32 Hits=StatueReview?int32(DCEarlyEncounterPolicy::StatueHealth):VerifyCreatureHits;
		if (!StatueReview) { Check(Hits >= 1 && Hits <= 3, TEXT("regular creature dies on the first, second or third valid cannon hit")); }
		Check(VerifyShots >= Hits && VerifyShots <= Hits+1, StatueReview?TEXT("four-health statue needs four hits with at most one missed shot"):TEXT("one/two/three-hit creature uses at most one missed shot"));
		Check(VerifyFireballImpacts > 0, TEXT("ignored creatures deliver visible projectiles to the deck"));
		Check(VerifyBeamImpacts > 0, TEXT("ignored statue warms up and damages the deck once"));
        Check(VerifyBeamLines>0 && VerifyBeamLineContacts>=VerifyBeamLines*2, TEXT("statue beam sweeps two separated deck contacts"));
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
	auto FaceFire = [this, Captain, PC]()
	{
		const FVector Stand = Director->DeckToWorld(VerifyFire + FVector(-155, 0, Captain->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 8.f));
		Captain->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
		Captain->GetCharacterMovement()->StopMovementImmediately();
		PC->SetControlRotation((Director->DeckToWorld(VerifyFire) - (Stand + FVector(0, 0, 64))).Rotation());
	};
	auto DelayedContact = [Captain]()
	{
		// Wind up just short of contact, then delay one player frame by 400 ms.
		// Both contact and the visual end fall inside that frame. Use the public
		// player tick without changing world time or global frame-rate settings.
		Captain->Tick(.225f);
		Captain->Tick(.4f);
	};
	if (VerifyStage == 0 && GetWorld()->GetTimeSeconds() > 2.f)
	{
		// Observe contacts after the player's tick. A first-use loading hitch can
		// cross both timers in one frame; elapsed time alone does not order actors.
		AddTickPrerequisiteActor(Captain);
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
		Check(Captain->GetFireFeedback().StartsWith(TEXT("FIRE WEAKENED")), TEXT("confirmed partial contact displays feedback"));
		++VerifyStage; VerifyClock = 0.f;
	}
	else if (VerifyStage == 6 && VerifyClock > .4f)
	{
		Check(Troubles->NumFires() == 0, TEXT("third animated rag contact extinguishes fire"));
		Check(Director->Score == VerifyScore + 50, TEXT("extinguishing awards exactly one score increment"));
		Check(!Troubles->SwatNear(VerifyFire) && !Troubles->ExtinguishNear(VerifyFire), TEXT("completed fire cannot be scored again"));
		Check(Captain->GetFireFeedback() == TEXT("FIRE OUT   +50"), TEXT("confirmed final contact displays completion feedback"));
		VerifyStage = 7; VerifyClock = 0.f;
	}
	else if (VerifyStage == 7 && VerifyClock > 3.8f)
	{
		int32 Effects = 0;
		for (TActorIterator<ADCDeckFireEffect> It(GetWorld()); It; ++It) { ++Effects; }
		Check(Effects == 0, TEXT("extinguished fire effect cleans up after smoke fades"));
		Troubles->StartFire(VerifyFire);
		Check(Troubles->ExtinguishNear(VerifyFire) && Troubles->NumFires() == 0, TEXT("crew bucket extinguish still completes a fire"));
		Troubles->StartFire(VerifyFire);
		VerifyScore = Director->Score;
		FaceFire();
		VerifyStage = 8; VerifyClock = 0.f;
	}
	else if (VerifyStage == 8 && VerifyClock > .2f)
	{
		Check(Captain->TrySwatFire(), TEXT("look-away case starts a real swing"));
		VerifyStage = 9; VerifyClock = 0.f;
	}
	else if (VerifyStage == 9 && VerifyClock > .06f)
	{
		PC->SetControlRotation(PC->GetControlRotation() + FRotator(0, 180, 0));
		VerifyStage = 10; VerifyClock = 0.f;
	}
	else if (VerifyStage == 10 && VerifyClock > .7f)
	{
		Check(Troubles->SwatsRemaining(VerifyFire) == 3 && Captain->GetFireFeedback().IsEmpty(), TEXT("turning away before contact does no work and gives no hit feedback"));
		FaceFire(); VerifyStage = 11; VerifyClock = 0.f;
	}
	else if (VerifyStage == 11 && VerifyClock > .2f)
	{
		Check(Captain->TrySwatFire(), TEXT("station cancellation case starts a real swing"));
		// Enter and leave in the same frame: cancellation must occur at the
		// transition, not depend on spending a tick inside the station.
		Captain->DebugTakeStation(EDCStationType::Helm, EDCSide::None);
		Check(Captain->IsAtStation(), TEXT("cancellation case enters the actual helm"));
		Captain->DebugTakeStation(EDCStationType::None, EDCSide::None);
		FaceFire(); VerifyStage = 12; VerifyClock = 0.f;
	}
	else if (VerifyStage == 12 && VerifyClock > .7f)
	{
		Check(!Captain->IsAtStation() && Troubles->SwatsRemaining(VerifyFire) == 3 && Captain->GetFireFeedback().IsEmpty(), TEXT("interrupted swing cannot resume after leaving a station"));
		Check(Captain->TrySwatFire(), TEXT("retreat case starts a real swing"));
		Captain->SetActorLocation(Director->DeckToWorld(VerifyFire + FVector(-800, 0, 150)), false, nullptr, ETeleportType::TeleportPhysics);
		VerifyStage = 13; VerifyClock = 0.f;
	}
	else if (VerifyStage == 13 && VerifyClock > .7f)
	{
		Check(Troubles->SwatsRemaining(VerifyFire) == 3 && Captain->GetFireFeedback().IsEmpty(), TEXT("leaving reach before contact does no work and gives no hit feedback"));
		Check(Director->Score == VerifyScore, TEXT("all interrupted and missed swings award no score"));
		FaceFire(); VerifyStage = 14; VerifyClock = 0.f;
	}
	else if (VerifyStage == 14 && VerifyClock > .2f)
	{
		Check(Captain->TrySwatFire(), TEXT("crew completion case starts a real swing"));
		Check(Troubles->ExtinguishNear(VerifyFire), TEXT("crew can complete the fire before rag contact"));
		VerifyStage = 15; VerifyClock = 0.f;
	}
	else if (VerifyStage == 15 && VerifyClock > .7f)
	{
		Check(Troubles->NumFires() == 0 && Captain->GetFireFeedback().IsEmpty() && Director->Score == VerifyScore + 50, TEXT("crew completion gives no false player hit or duplicate score"));
		Troubles->StartFire(VerifyFire);
		VerifyScore = Director->Score;
		FaceFire(); VerifyStage = 16; VerifyClock = 0.f;
	}
	else if (VerifyStage == 16 && VerifyClock > .2f)
	{
		Check(Captain->TrySwatFire(), TEXT("delayed-frame case begins a real swing"));
		DelayedContact();
		Check(Troubles->SwatsRemaining(VerifyFire) == 2 && Director->Score == VerifyScore
			&& Captain->GetFireFeedback().StartsWith(TEXT("FIRE WEAKENED")), TEXT("delayed frame resolves one contact even after the visual swing ends"));
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Auto/shot_hitch_contact.png"), true, false);
		VerifyStage = 17; VerifyClock = 0.f;
	}
	else if (VerifyStage == 17 && VerifyClock > .2f)
	{
		Check(Troubles->SwatsRemaining(VerifyFire) == 2 && Director->Score == VerifyScore, TEXT("following frames cannot repeat the delayed contact"));
		Check(Captain->TrySwatFire(), TEXT("second delayed-frame swing begins"));
		DelayedContact();
		Check(Troubles->SwatsRemaining(VerifyFire) == 1 && Director->Score == VerifyScore, TEXT("second delayed contact preserves partial suppression"));
		VerifyStage = 18; VerifyClock = 0.f;
	}
	else if (VerifyStage == 18 && VerifyClock > .2f)
	{
		Check(Captain->TrySwatFire(), TEXT("third delayed-frame swing begins"));
		DelayedContact();
		Check(Troubles->NumFires() == 0 && Director->Score == VerifyScore + 50
			&& Captain->GetFireFeedback() == TEXT("FIRE OUT   +50"), TEXT("third delayed contact extinguishes and scores exactly once"));
		// Give the cancellation case its own baseline even when the prior check fails.
		Troubles->ExtinguishNear(VerifyFire);
		Troubles->StartFire(VerifyFire);
		VerifyScore = Director->Score;
		FaceFire(); VerifyStage = 19; VerifyClock = 0.f;
	}
	else if (VerifyStage == 19 && VerifyClock > .2f)
	{
		Check(Captain->TrySwatFire(), TEXT("delayed cancellation case begins a real swing"));
		Captain->DebugTakeStation(EDCStationType::Helm, EDCSide::None);
		Captain->DebugTakeStation(EDCStationType::None, EDCSide::None);
		FaceFire();
		DelayedContact();
		Check(Troubles->SwatsRemaining(VerifyFire) == 3 && Captain->GetFireFeedback().IsEmpty()
			&& Director->Score == VerifyScore, TEXT("a delayed frame cannot revive a cancelled swing"));
		Troubles->ExtinguishNear(VerifyFire);
		UE_LOG(LogDC, Display, TEXT("EARLY GAMEPLAY VERIFICATION %s"), bVerifyFailed ? TEXT("FAILED") : TEXT("PASSED"));
		VerifyStage = 20;
		FPlatformMisc::RequestExitWithStatus(false, bVerifyFailed ? 1 : 0);
	}
}
#endif
