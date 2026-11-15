#include "DCTroubles.h"
#include "DCDeckFireEffect.h"
#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "DCCrew.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

ADCTroubles::ADCTroubles()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RiggingLabel=CreateDefaultSubobject<UTextRenderComponent>(TEXT("RiggingRepairLabel"));
	RiggingLabel->SetupAttachment(RootComponent);
	RiggingLabel->SetHorizontalAlignment(EHTA_Center);
	RiggingLabel->SetWorldSize(24.f);
	RiggingLabel->SetTextRenderColor(FColor(255,220,110));
	RiggingLabel->SetVisibility(false);
	RiggingLabel->SetCastShadow(false);
	RiggingArrow=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RiggingRepairArrow"));
	RiggingArrow->SetupAttachment(RootComponent);
	RiggingArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RiggingArrow->SetCanEverAffectNavigation(false);
	RiggingArrow->SetCastShadow(false);
	RiggingArrow->SetVisibility(false);
}

ADCTroubles* ADCTroubles::Get(const UObject* WorldContext)
{
	if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCTroubles> It(World); It; ++It) { return *It; }
	}
	return nullptr;
}

bool ADCTroubles::NearestFire(const FVector& From, FVector& OutLocation) const
{
	float Best = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const FDCFire& F : Fires)
	{
		const float D = FVector::DistSquared(F.Location, From);
		if (D < Best) { Best = D; OutLocation = F.Location; bFound = true; }
	}
	return bFound;
}

void ADCTroubles::StartFire(const FVector& Location)
{
	if (Location.ContainsNaN() || Fires.Num() >= MaxFires) { return; }
	for (const FDCFire& Fire : Fires)
	{
		if (FVector::DistSquared(Fire.Location, Location) < FMath::Square(240.f)) { return; }
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADCDeckFireEffect* Visual = GetWorld()->SpawnActor<ADCDeckFireEffect>(Location, FRotator::ZeroRotator, Params);
	if (!Visual) { return; }
	// The hazard stays in resting-deck coordinates; its effect follows the moving ship.
	if (const ADCRideDirector* Dir = ADCRideDirector::Get(this))
	{
		Visual->SetActorLocation(Dir->DeckToWorld(Location));
		if (Dir->ShipActor)
		{
			Visual->SetActorRotation(Dir->ShipActor->GetActorRotation());
			Visual->AttachToActor(Dir->ShipActor, FAttachmentTransformRules::KeepWorldTransform);
		}
	}
	FDCFire& F = Fires.AddDefaulted_GetRef();
	F.Location = Location;
	F.Seed = FMath::FRandRange(0.f, 100.f);
	F.Visual = Visual;
	Visual->Initialize(F.Seed);
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Report(TEXT("Fire on deck! Swat the flames with a rag!"), true);
	}
}

bool ADCTroubles::ExtinguishNear(const FVector& Location, float Reach)
{
	for (int32 i = 0; i < Fires.Num(); ++i)
	{
		if (FVector::DistSquared(Fires[i].Location, Location) < Reach * Reach)
		{
			if (ADCDeckFireEffect* V = Cast<ADCDeckFireEffect>(Fires[i].Visual.Get())) { V->Extinguish(); }
			Fires.RemoveAt(i);
			if (Director) { Director->Score += 50; }
			return true;
		}
	}
	return false;
}

bool ADCTroubles::FindIgnitionPoint(int32 Preference, FVector& OutLocation) const
{
	if (DeckPoints.IsEmpty()) { return false; }
	for (int32 Offset = 0; Offset < DeckPoints.Num(); ++Offset)
	{
		const FVector Candidate = DeckPoints[(FMath::Abs(Preference) + Offset) % DeckPoints.Num()];
		bool bOccupied = false;
		for (const FDCFire& F : Fires) { bOccupied |= FVector::DistSquared(F.Location, Candidate) < FMath::Square(300.f); }
		if (!bOccupied) { OutLocation = Candidate; return true; }
	}
	OutLocation = DeckPoints[FMath::Abs(Preference) % DeckPoints.Num()];
	return true;
}

int32 ADCTroubles::SwatsRemaining(const FVector& Location, float Reach) const
{
	for (const FDCFire& F : Fires)
	{
		if (FVector::DistSquared(F.Location, Location) < Reach * Reach)
		{
			return FMath::Max(1, FMath::CeilToInt((1.f - F.Suppression) * 3.f - .001f));
		}
	}
	return 0;
}

bool ADCTroubles::SwatNear(const FVector& Location, float Reach)
{
	for (FDCFire& F : Fires)
	{
		if (FVector::DistSquared(F.Location, Location) >= Reach * Reach) { continue; }
		F.Suppression = FMath::Min(1.f, F.Suppression + 1.f / 3.f);
		const FVector At = F.Location;
		if (ADCDeckFireEffect* V = Cast<ADCDeckFireEffect>(F.Visual.Get())) { V->ReactToSwat(); }
		if (F.Suppression >= .999f) { return ExtinguishNear(At, 1.f); }
		if (ADCDeckFireEffect* V = Cast<ADCDeckFireEffect>(F.Visual.Get())) { V->SetStrength(1.f - F.Suppression); }
		return true;
	}
	return false;
}

void ADCTroubles::ResetEncounterTroubles()
{
	for (FDCFire& F : Fires) { if (AActor* V = F.Visual.Get()) { V->Destroy(); } }
	Fires.Reset();
	bRiggingLoose = false; RiggingProgress = 0.f;
	UpdateRiggingMarker();
	if (auto* T = ADCTreadmill::Get(this)) { T->SteerAuthority = 1.f; }
}

void ADCTroubles::TearRigging()
{
	if (bRiggingLoose) { return; }
	bRiggingLoose = true;
	RiggingProgress = 0.f;
	UpdateRiggingMarker();
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Report(TEXT("Mainsail loose! Repair the marked lines on deck!"), true);
	}
}

bool ADCTroubles::RepairRigging(float Amount)
{
	if (!bRiggingLoose) { return true; }
	RiggingProgress += Amount;
	if (RiggingProgress >= 1.f)
	{
		bRiggingLoose = false;
		RiggingProgress = 0.f;
		UpdateRiggingMarker();
		if (Director) { Director->Score += 50; }
		return true;
	}
	return false;
}

bool ADCTroubles::GetRiggingRepairLocation(FVector& Location) const
{
	if (const auto* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		for (const FDCStation& Station : Crew->GetStations())
		{
			if (Station.Type==EDCStationType::Rigging) { Location=Station.Location; return true; }
		}
	}
	return false;
}

void ADCTroubles::UpdateRiggingMarker()
{
	FVector Station;
	const bool Visible=bRiggingLoose && GetRiggingRepairLocation(Station);
	if (Visible!=bRiggingMarkerVisible)
	{
		UE_LOG(LogDC,Log,TEXT("RIGGING_REPAIR_MARKER visible=%d station=%s"),Visible,Visible?*Station.ToCompactString():TEXT("none"));
		bRiggingMarkerVisible=Visible;
	}
	RiggingLabel->SetVisibility(Visible); RiggingArrow->SetVisibility(Visible);
	if (!Visible) { return; }
	const auto* Ride=Director ? Director.Get() : ADCRideDirector::Get(this);
	const FTransform Deck=Ride ? Ride->DeckTransform : FTransform::Identity;
	if (!RiggingArrow->GetStaticMesh())
	{
		RiggingArrow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone")));
		if (auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_Bolt.M_Bolt")))
		{
			auto* Glow=UMaterialInstanceDynamic::Create(Base,this);
			Glow->SetVectorParameterValue(TEXT("Color"),FLinearColor(.85f,.55f,.08f));
			RiggingArrow->SetMaterial(0,Glow);
		}
	}
	RiggingArrow->SetWorldLocationAndRotation(Deck.TransformPosition(Station+FVector(0,0,95.f+8.f*FMath::Sin(Clock*3.f))),Deck.GetRotation()*FRotator(180,0,0).Quaternion());
	RiggingArrow->SetWorldScale3D(FVector(.12f,.12f,.20f));
	RiggingLabel->SetWorldLocation(Deck.TransformPosition(Station+FVector(0,0,125.f)));
	float Distance=0.f;
	bool bReadable=false;
	if (auto* PC=GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		const auto* Cam=PC->PlayerCameraManager.Get();
		const FVector Camera=Cam->GetCameraLocation();
		RiggingLabel->SetWorldRotation((Camera-RiggingLabel->GetComponentLocation()).Rotation());
		Distance=FVector::Dist(Camera,Deck.TransformPosition(Station))/100.f;
		// Use the label's actual forward depth, not distance to the repair point:
		// its raised anchor can pass beside the camera even when the station is
		// metres away. Keep one title near16px and hide it inside the near view.
		const float Depth=FVector::DotProduct(RiggingLabel->GetComponentLocation()-Camera,Cam->GetCameraRotation().Vector());
		int32 Width=0,Height=0; PC->GetViewportSize(Width,Height);
		const float TanHalfFov=FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(Cam->GetFOVAngle(),30.f,140.f)*.5f));
		RiggingLabel->SetWorldSize(FMath::Clamp(16.f*2.f*FMath::Max(Depth,0.f)*TanHalfFov/FMath::Max(Width,1),.1f,80.f));
		bReadable=Depth>40.f && Width>0;
	}
	RiggingLabel->SetVisibility(bReadable);
	// Nearby interaction already shows Hold E and progress. Keep the locator
	// compact so it cannot cover a crew member or compete with that prompt.
	RiggingLabel->SetText(FText::FromString(FString::Printf(TEXT("REPAIR RIGGING  •  %.0f m"),Distance)));
}

void ADCTroubles::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director)
	{
		Director = ADCRideDirector::Get(this);
		if (!Director) { return; }
	}
	Clock += DeltaSeconds;
	const EDCPhase Phase = Director->GetPhase();
	const float Dt = DeltaSeconds * Director->TimeScale;

#if !UE_BUILD_SHIPPING
	// Root-owned native review: real routed crew action and real repair marker.
	if (FParse::Param(FCommandLine::Get(),TEXT("DCCrewWorkReview")) && Director->RideTime>=12.f && DeckPoints.Num() && !Tags.Contains(TEXT("CrewWorkReviewStarted")))
	{
		Tags.Add(TEXT("CrewWorkReviewStarted"));
		StartFire(DeckPoints[FMath::Min(2,DeckPoints.Num()-1)]);
		TearRigging();
		if (auto* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->ExecuteOrder(TEXT("Ruby, put out the fire")); }
		UE_LOG(LogDC,Log,TEXT("CREW_WORK_REVIEW started"));
	}
	// Repeatable visual/lifecycle review; ordinary play never enters this path.
	float ReviewAt = -1.f, ReviewOut = -1.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCFireReviewAt="), ReviewAt) && DeckPoints.Num())
	{
		if (Director->RideTime >= ReviewAt && !Tags.Contains(TEXT("FireReviewStarted")))
		{
			Tags.Add(TEXT("FireReviewStarted"));
			StartFire(DeckPoints[FMath::Min(2, DeckPoints.Num()-1)]);
			UE_LOG(LogDC, Log, TEXT("Fire review: started at %.2f, active=%d, deck=%s"), Director->RideTime, NumFires(),
				*DeckPoints[FMath::Min(2, DeckPoints.Num()-1)].ToCompactString());
		}
		if (FParse::Value(FCommandLine::Get(), TEXT("DCFireReviewOut="), ReviewOut) && Director->RideTime >= ReviewOut
			&& !Tags.Contains(TEXT("FireReviewOut")))
		{
			Tags.Add(TEXT("FireReviewOut"));
			const float Before = Director->Score;
			const bool bOut = ExtinguishNear(DeckPoints[FMath::Min(2, DeckPoints.Num()-1)]);
			UE_LOG(LogDC, Log, TEXT("Fire review: extinguish=%d, active=%d, score delta=%.0f"), bOut, NumFires(), Director->Score-Before);
		}
		const float GoneAt = ReviewOut >= 0.f ? ReviewOut+4.f : ReviewAt+FireBurnTime+4.1f;
		if (Director->RideTime >= GoneAt && !Tags.Contains(TEXT("FireReviewClean")))
		{
			Tags.Add(TEXT("FireReviewClean"));
			int32 Effects = 0;
			for (TActorIterator<ADCDeckFireEffect> It(GetWorld()); It; ++It) { ++Effects; }
			UE_LOG(LogDC, Log, TEXT("Fire review: after cleanup active=%d, effects=%d"), NumFires(), Effects);
		}
	}
#endif

	// Clear water: whatever was still wrong is behind us.
	if (Phase == EDCPhase::Ending)
	{
		for (FDCFire& F : Fires) { if (ADCDeckFireEffect* V = Cast<ADCDeckFireEffect>(F.Visual.Get())) { V->Extinguish(); } }
		Fires.Reset();
		bRiggingLoose = false;
	}

	// New fires in the hell channel.
	if (Phase == EDCPhase::Hell && DeckPoints.Num() && Fires.Num() < MaxFires)
	{
		FireTimer -= Dt;
		if (FireTimer <= 0.f)
		{
			FireTimer = FMath::FRandRange(14.f, 22.f);
			StartFire(DeckPoints[FMath::RandRange(0, DeckPoints.Num() - 1)]);
		}
	}

	// Rigging tears in the slide, the breach and the maelstrom.
	if ((Phase == EDCPhase::Slide || Phase == EDCPhase::Breach || Phase == EDCPhase::Maelstrom)
        && !(ADCMaelstrom::Get(this) && ADCMaelstrom::Get(this)->IsActive()))
	{
		RiggingTimer -= Dt;
		if (RiggingTimer <= 0.f)
		{
			RiggingTimer = FMath::FRandRange(28.f, 40.f);
			TearRigging();
		}
	}

	// A neglected fire burns out in the end.
	for (int32 i = Fires.Num() - 1; i >= 0; --i)
	{
		if (Fires[i].Age > FireBurnTime)
		{
			if (ADCDeckFireEffect* V = Cast<ADCDeckFireEffect>(Fires[i].Visual.Get())) { V->Extinguish(); }
			Fires.RemoveAt(i);
		}
	}

	// Gameplay damage and burn duration remain independent of the new effect's fade-out.
	for (FDCFire& F : Fires)
	{
		F.Age += Dt;
		Director->DamageShip(FireDamagePerSecond * Dt * (1.f - F.Suppression), true);
		if (ADCDeckFireEffect* V = Cast<ADCDeckFireEffect>(F.Visual.Get()))
		{
			V->SetStrength(FMath::Clamp(F.Age / 3.f, 0.25f, 1.f) * (1.f - F.Suppression));
		}
	}

	// Loose rigging makes the ship sluggish.
	if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this))
	{
		Treadmill->SteerAuthority = bRiggingLoose ? 0.35f : 1.f;
	}
	UpdateRiggingMarker();
}
