#include "DCTroubles.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "DCCrew.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCTroubles::ADCTroubles()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
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
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Visual = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Location), Params);
	USceneComponent* Root = NewObject<USceneComponent>(Visual, TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	Visual->SetRootComponent(Root);
	Root->RegisterComponent();
	Visual->SetActorLocation(Location);

	// Flames: a few glowing tongues that the tick stretches and sways.
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Flame.M_Flame"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	for (int32 i = 0; i < 5; ++i)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Visual);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Cone);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		if (Base) { C->SetMaterial(0, Base); }
		C->SetupAttachment(Root);
		C->SetRelativeLocation(FVector(FMath::FRandRange(-45.f, 45.f), FMath::FRandRange(-45.f, 45.f), 40.f));
		C->RegisterComponent();
	}

	UPointLightComponent* Light = NewObject<UPointLightComponent>(Visual);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Light->SetLightColor(FLinearColor(1.f, 0.42f, 0.1f));
	Light->SetIntensity(6000.f);
	Light->SetAttenuationRadius(1400.f);
	Light->SetCastShadows(true);
	Light->RegisterComponent();

	FDCFire& F = Fires.AddDefaulted_GetRef();
	F.Location = Location;
	F.Seed = FMath::FRandRange(0.f, 100.f);
	F.Visual = Visual;
	F.Light = Light;

	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Report(TEXT("Fire on deck! We need hands on the buckets!"), true);
	}
}

bool ADCTroubles::ExtinguishNear(const FVector& Location, float Reach)
{
	for (int32 i = 0; i < Fires.Num(); ++i)
	{
		if (FVector::DistSquared(Fires[i].Location, Location) < Reach * Reach)
		{
			if (AActor* V = Fires[i].Visual.Get()) { V->Destroy(); }
			Fires.RemoveAt(i);
			if (Director) { Director->Score += 50; }
			return true;
		}
	}
	return false;
}

void ADCTroubles::TearRigging()
{
	if (bRiggingLoose) { return; }
	bRiggingLoose = true;
	RiggingProgress = 0.f;
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Report(TEXT("Mainsail has torn loose! She will not answer the helm!"), true);
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
		if (Director) { Director->Score += 50; }
		return true;
	}
	return false;
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
	if (Phase == EDCPhase::Slide || Phase == EDCPhase::Breach || Phase == EDCPhase::Maelstrom)
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
			if (AActor* V = Fires[i].Visual.Get()) { V->Destroy(); }
			Fires.RemoveAt(i);
		}
	}

	// Fires burn the ship and animate.
	for (FDCFire& F : Fires)
	{
		F.Age += DeltaSeconds;
		Director->DamageShip(FireDamagePerSecond * Dt, true);
		const float Grow = FMath::Clamp(F.Age / 3.f, 0.25f, 1.f);
		if (UPointLightComponent* L = F.Light.Get())
		{
			L->SetIntensity(Grow * (5000.f + 2500.f * FMath::Sin(Clock * 17.f + F.Seed) * FMath::Sin(Clock * 7.3f + F.Seed * 2.f)));
		}
		if (AActor* V = F.Visual.Get())
		{
			TArray<UStaticMeshComponent*> Tongues;
			V->GetComponents(Tongues);
			for (int32 i = 0; i < Tongues.Num(); ++i)
			{
				const float P = Clock * (5.f + i) + F.Seed + i * 1.7f;
				const float H = Grow * (1.1f + 0.5f * FMath::Sin(P)) * (1.f + 0.25f * i);
				Tongues[i]->SetRelativeScale3D(FVector(0.55f + 0.1f * FMath::Sin(P * 1.3f), 0.55f + 0.1f * FMath::Cos(P * 1.1f), H));
				FVector Loc = Tongues[i]->GetRelativeLocation();
				Loc.Z = H * 50.f;
				Tongues[i]->SetRelativeLocation(Loc);
				Tongues[i]->SetRelativeRotation(FRotator(6.f * FMath::Sin(P * 0.9f), 0.f, 6.f * FMath::Cos(P * 0.7f)));
			}
		}
	}

	// Loose rigging makes the ship sluggish.
	if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this))
	{
		Treadmill->SteerAuthority = bRiggingLoose ? 0.35f : 1.f;
	}
}
