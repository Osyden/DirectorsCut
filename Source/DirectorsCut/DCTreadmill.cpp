#include "DCTreadmill.h"
#include "DCRideDirector.h"
#include "DCHazards.h"
#include "DCCrew.h"
#include "DCTroubles.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"

ADCTreadmill::ADCTreadmill()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ADCTreadmill* ADCTreadmill::Get(const UObject* WorldContext)
{
	if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCTreadmill> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void ADCTreadmill::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);
	CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

void ADCTreadmill::SpawnObject(bool bHostile, const FVector& Base, const FVector& Scale, UStaticMesh* Mesh, const FLinearColor& Color, float Radius, float Health)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Base, FRotator::ZeroRotator, Params);
	if (!Actor)
	{
		return;
	}
	UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent();
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Actor->SetActorScale3D(Scale);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, Actor);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		Comp->SetMaterial(0, MID);
	}

	FDCWorldObject& Obj = Objects.AddDefaulted_GetRef();
	Obj.Actor = Actor;
	Obj.Base = Base;
	Obj.Radius = Radius;
	Obj.bHostile = bHostile;
	Obj.Health = Health;
}

void ADCTreadmill::Register(AActor* Actor, const FVector& Base, float Radius, bool bHostile, float Health, float Yaw, bool bSolid)
{
	if (!Actor) { return; }
	FDCWorldObject& Obj = Objects.AddDefaulted_GetRef();
	Obj.Actor = Actor;
	Obj.Base = Base;
	Obj.Radius = Radius;
	Obj.bHostile = bHostile;
	Obj.Health = Health;
	Obj.Yaw = Yaw;
	Obj.bSolid = bSolid;
	Actor->SetActorLocation(Base);
}

void ADCTreadmill::SpawnForPhase(EDCPhase Phase, float DeltaSeconds)
{
	float ObstacleEvery = 0.f, HostileEvery = 0.f, SceneryEvery = 0.f;
	switch (Phase)
	{
	case EDCPhase::Fog: ObstacleEvery = 7.f; HostileEvery = 14.f; SceneryEvery = 9.f; break;
	case EDCPhase::Hell: ObstacleEvery = 3.5f; HostileEvery = 9.f; SceneryEvery = 5.f; break;
	default: break; // the kraken owns the maelstrom
	}
	const bool bHell = Phase == EDCPhase::Hell;
	const float SeaZ = Director ? Director->SeaLevelZ : 0.f;
	const FLinearColor Stone = bHell ? FLinearColor(0.05f, 0.025f, 0.02f) : FLinearColor(0.07f, 0.085f, 0.08f);
	const FLinearColor Ember(9.f, 1.6f, 0.15f);
	UWorld* World = GetWorld();

	// Obstacles: in the lane, must be steered around.
	if (ObstacleEvery > 0.f && (ObstacleTimer -= DeltaSeconds) <= 0.f)
	{
		ObstacleTimer = ObstacleEvery * FMath::FRandRange(0.7f, 1.3f);
		// Sea space: add the lane offset so they land near the ship's present course.
		const FVector Base(SpawnDistance, LaneOffset + FMath::FRandRange(-2500.f, 2500.f), SeaZ);
		if (bHell || FMath::FRand() < 0.6f)
		{
			const float Radius = FMath::FRandRange(350.f, 750.f);
			AActor* Rock = DCHazards::SpawnRock(World, Radius, FMath::FRandRange(1500.f, 4200.f), bHell, Stone);
			if (bHell) { DCHazards::SetGlow(Rock, Ember); }
			Register(Rock, Base, Radius, false, 1.f, FMath::FRandRange(0.f, 360.f), true);
		}
		else
		{
			const float Scale = FMath::FRandRange(0.7f, 1.15f);
			Register(DCHazards::SpawnRuin(World, Scale, Stone), Base, 1000.f * Scale, false, 1.f, FMath::FRandRange(-25.f, 25.f), true);
		}
	}

	// Scenery: off to the sides, never hit, there to loom.
	if (SceneryEvery > 0.f && (SceneryTimer -= DeltaSeconds) <= 0.f)
	{
		SceneryTimer = SceneryEvery * FMath::FRandRange(0.7f, 1.4f);
		const float Side = FMath::RandBool() ? 1.f : -1.f;
		const FVector Base(SpawnDistance, LaneOffset + Side * FMath::FRandRange(4500.f, 9000.f), SeaZ);
		if (!bHell && FMath::FRand() < 0.45f)
		{
			// Statues face the passing ship.
			Register(DCHazards::SpawnStatue(World, FMath::FRandRange(5000.f, 9500.f), Stone), Base - FVector(0.f, 0.f, 600.f), 1500.f, false, 1.f, Side > 0.f ? -110.f : 110.f, false);
		}
		else
		{
			const float Radius = FMath::FRandRange(900.f, 2200.f);
			AActor* Rock = DCHazards::SpawnRock(World, Radius, FMath::FRandRange(3000.f, 9000.f), bHell, Stone);
			if (bHell) { DCHazards::SetGlow(Rock, Ember); }
			Register(Rock, Base, Radius, false, 1.f, FMath::FRandRange(0.f, 360.f), false);
		}
	}

	// Hostiles: spectral ships that come alongside to be shot at.
	if (HostileEvery > 0.f && (HostileTimer -= DeltaSeconds) <= 0.f && NumHostiles() < 2)
	{
		HostileTimer = HostileEvery * FMath::FRandRange(0.7f, 1.3f);
		const float Side = FMath::RandBool() ? 1.f : -1.f;
		const FVector Base(SpawnDistance, Side * FMath::FRandRange(5000.f, 8500.f), SeaZ);
		AActor* Ghost = DCHazards::SpawnGhostShip(World, bHell ? FLinearColor(1.f, 0.25f, 0.03f) : FLinearColor(0.15f, 0.9f, 0.75f));
		if (UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(bHell ? (Side > 0.f ? TEXT("Burning ship to starboard!") : TEXT("Burning ship to port!")) : (Side > 0.f ? TEXT("Ghost ship to starboard!") : TEXT("Ghost ship to port!")));
		}
		Register(Ghost, Base, 1600.f, true, 6.f, FMath::FRandRange(-8.f, 8.f), false);
	}
}
void ADCTreadmill::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director)
	{
		Director = ADCRideDirector::Get(this);
		if (!Director) { return; }
	}

	const FDCEnvState& Env = Director->GetEnv();
	const float Speed = Env.ShipSpeed * Director->TimeScale;

	LaneOffset = FMath::Clamp(LaneOffset + Steer * LaneSpeed * SteerAuthority * DeltaSeconds, -LaneLimit, LaneLimit);

	SpawnForPhase(Director->GetPhase(), DeltaSeconds * Director->TimeScale);
	TickIncoming(DeltaSeconds);

	const FTransform Rock = Director->GetWorldRock();
	for (int32 i = Objects.Num() - 1; i >= 0; --i)
	{
		FDCWorldObject& Obj = Objects[i];
		AActor* Actor = Obj.Actor.Get();
		if (!Actor)
		{
			Objects.RemoveAtSwap(i);
			continue;
		}

		if (Obj.bExternal)
		{
			// Owned and moved elsewhere; just keep our sea-space copy of its position current.
			Obj.Base = Actor->GetActorLocation() + FVector(0.f, LaneOffset, 0.f);
			continue;
		}

		// Hostiles pace the ship once alongside; obstacles sweep past.
		if (Obj.bHostile)
		{
			const float TargetX = 0.f;
			Obj.Base.X = FMath::FInterpConstantTo(Obj.Base.X, TargetX, DeltaSeconds, Obj.Base.X > 4000.f ? Speed : Speed * 0.08f);
			if (Obj.Base.X <= 1.f)
			{
				Obj.Base.X -= Speed * 0.3f * DeltaSeconds;
			}
			// Alongside, an untouched hostile keeps shooting. Sinking it is the only way to stop that.
			if (!Obj.bExternal && FMath::Abs(Obj.Base.X) < 5000.f)
			{
				Obj.AttackTimer -= DeltaSeconds;
				if (Obj.AttackTimer <= 0.f)
				{
					Obj.AttackTimer = FMath::FRandRange(10.f, 14.f);
					LaunchAt(Actor->GetActorLocation() + FVector(0.f, 0.f, 700.f));
				}
			}
		}
		else
		{
			Obj.Base.X -= Speed * DeltaSeconds;
		}

		// Ships that pace us hold station on the ship itself; everything else is fixed in the sea and slides with the helm.
		const bool bPacing = Obj.bHostile && !Obj.bExternal;
		const FVector Local(Obj.Base.X, bPacing ? Obj.Base.Y : Obj.Base.Y - LaneOffset, Obj.Base.Z);
		Actor->SetActorLocationAndRotation(Rock.TransformPosition(Local), Rock.GetRotation() * FRotator(0.f, Obj.Yaw, 0.f).Quaternion());

		// Lookout: call an obstacle once, when it is close enough to matter and near our course.
		if (!Obj.bHostile && Obj.bSolid && !Obj.bCalled && Local.X < 13000.f && FMath::Abs(Local.Y) < ShipHalfWidth + Obj.Radius + 150.f)
		{
			Obj.bCalled = true;
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(FMath::Abs(Local.Y) < 250.f ? TEXT("Rocks dead ahead!") : (Local.Y > 0.f ? TEXT("Rocks ahead, off the starboard bow!") : TEXT("Rocks ahead, off the port bow!")));
			}
		}

		if (!Obj.bHostile && Obj.bSolid && FMath::Abs(Local.X) < ShipHalfLength && FMath::Abs(Local.Y) < ShipHalfWidth + Obj.Radius)
		{
			Director->DamageShip(3.f);
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->Report(TEXT("We struck the rocks! Hull is breached!"), true); }
			Actor->Destroy();
			Objects.RemoveAtSwap(i);
			continue;
		}

		if (Local.X < -DespawnDistance)
		{
			Actor->Destroy();
			Objects.RemoveAtSwap(i);
		}
	}
}

bool ADCTreadmill::FindObstacleAhead(float MaxDistance, float& OutX, float& OutY) const
{
	float BestX = MaxDistance;
	bool bFound = false;
	for (const FDCWorldObject& Obj : Objects)
	{
		if (Obj.bHostile || !Obj.bSolid) { continue; }
		const float X = Obj.Base.X;
		const float Y = Obj.Base.Y - LaneOffset;
		if (X > ShipHalfLength && X < BestX && FMath::Abs(Y) < ShipHalfWidth + Obj.Radius + 600.f)
		{
			BestX = X; OutX = X; OutY = Y; bFound = true;
		}
	}
	return bFound;
}

int32 ADCTreadmill::FindHostile(EDCSide Side, float MaxRange) const
{
	int32 Best = INDEX_NONE;
	float BestDist = MaxRange;
	for (int32 i = 0; i < Objects.Num(); ++i)
	{
		const FDCWorldObject& Obj = Objects[i];
		if (!Obj.bHostile) { continue; }
		const float Y = Obj.bExternal ? Obj.Base.Y - LaneOffset : Obj.Base.Y;
		if ((Side == EDCSide::Port && Y > 0.f) || (Side == EDCSide::Starboard && Y < 0.f)) { continue; }
		const float Dist = FVector2D(Obj.Base.X, Y).Size();
		if (Dist < BestDist)
		{
			BestDist = Dist; Best = i;
		}
	}
	return Best;
}

FVector ADCTreadmill::GetObjectLocation(int32 Index) const
{
	if (Objects.IsValidIndex(Index) && Objects[Index].Actor.IsValid())
	{
		return Objects[Index].Actor->GetActorLocation();
	}
	return FVector::ZeroVector;
}

bool ADCTreadmill::DamageObject(int32 Index, float Damage)
{
	if (!Objects.IsValidIndex(Index)) { return false; }
	FDCWorldObject& Obj = Objects[Index];
	Obj.Health -= Damage;
	if (Obj.Health <= 0.f)
	{
		if (AActor* Actor = Obj.Actor.Get()) { Actor->Destroy(); }
		Objects.RemoveAtSwap(Index);
		if (Director) { Director->Score += 100; }
		return true;
	}
	return false;
}

int32 ADCTreadmill::NumHostiles() const
{
	int32 N = 0;
	for (const FDCWorldObject& Obj : Objects) { if (Obj.bHostile) { ++N; } }
	return N;
}

int32 ADCTreadmill::DamageNear(const FVector& WorldPos, float Radius, float Damage, bool bDirect)
{
	int32 Hits = 0;
	for (int32 i = Objects.Num() - 1; i >= 0; --i)
	{
		const FDCWorldObject& Obj = Objects[i];
		const AActor* Actor = Obj.Actor.Get();
		if (!Obj.bHostile || !Actor) { continue; }
		const float Reach = Radius + Obj.Radius * (bDirect ? 0.6f : 1.f);
		if (FVector::DistSquared(Actor->GetActorLocation(), WorldPos) < Reach * Reach)
		{
			DamageObject(i, Damage);
			++Hits;
			if (bDirect) { break; }
		}
	}
	return Hits;
}

void ADCTreadmill::AddExternalHostile(AActor* Actor, float Radius, float Health)
{
	FDCWorldObject& Obj = Objects.AddDefaulted_GetRef();
	Obj.Actor = Actor;
	Obj.Base = Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
	Obj.Radius = Radius;
	Obj.bHostile = true;
	Obj.bExternal = true;
	Obj.Health = Health;
}

float ADCTreadmill::FindClearLane(float Lookahead) const
{
	float Best = LaneOffset;
	float BestCost = TNumericLimits<float>::Max();
	for (float Candidate = -LaneLimit; Candidate <= LaneLimit; Candidate += 250.f)
	{
		// Prefer staying put, and drift back toward the middle when nothing is in the way.
		float Cost = FMath::Abs(Candidate - LaneOffset) * 0.0005f + FMath::Abs(Candidate) * 0.00005f;
		for (const FDCWorldObject& Obj : Objects)
		{
			if (Obj.bHostile || !Obj.bSolid) { continue; }
			if (Obj.Base.X < -ShipHalfLength || Obj.Base.X > Lookahead) { continue; }
			const float Gap = FMath::Abs(Obj.Base.Y - Candidate) - (ShipHalfWidth + Obj.Radius);
			if (Gap < 500.f)
			{
				// Nearer obstacles count for more; overlapping the hull counts for much more than a tight squeeze.
				Cost += (Gap < 0.f ? 10.f : 2.f) * (1.f + 8000.f / (Obj.Base.X + 2500.f));
			}
		}
		if (Cost < BestCost) { BestCost = Cost; Best = Candidate; }
	}
	return Best;
}

void ADCTreadmill::LaunchAt(const FVector& From)
{
	// A spectral shot lobbed from a hostile ship onto our deck.
	const bool bHell = Director && Director->GetPhase() == EDCPhase::Hell;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Shot = GetWorld()->SpawnActor<AStaticMeshActor>(From, FRotator::ZeroRotator, Params);
	if (!Shot) { return; }
	UStaticMeshComponent* Comp = Shot->GetStaticMeshComponent();
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	Shot->SetActorScale3D(FVector(1.3f));
	if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Bolt.M_Bolt")))
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Glow, Shot);
		MID->SetVectorParameterValue(TEXT("Color"), bHell ? FLinearColor(40.f, 8.f, 0.6f) : FLinearColor(3.f, 22.f, 18.f));
		Comp->SetMaterial(0, MID);
	}

	FDCIncoming& In = Incoming.AddDefaulted_GetRef();
	In.Actor = Shot;
	In.From = From;
	In.To = FVector(FMath::FRandRange(-900.f, 900.f), FMath::FRandRange(-220.f, 220.f), (Director ? Director->SeaLevelZ : 0.f) + 420.f);
	In.Duration = FMath::Clamp(FVector::Dist(In.From, In.To) / 4500.f, 1.2f, 3.f);

	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Report(From.Y > 0.f ? TEXT("They are firing on us from starboard!") : TEXT("They are firing on us from port!"));
	}
}

void ADCTreadmill::TickIncoming(float DeltaSeconds)
{
	for (int32 i = Incoming.Num() - 1; i >= 0; --i)
	{
		FDCIncoming& In = Incoming[i];
		In.T += DeltaSeconds / In.Duration;
		AActor* Shot = In.Actor.Get();
		if (In.T >= 1.f || !Shot)
		{
			if (Shot) { Shot->Destroy(); }
			if (Director)
			{
				Director->DamageShip(1.f);
				// In the hell channel their shot is fire, and sometimes it catches.
				if (Director->GetPhase() == EDCPhase::Hell && FMath::FRand() < 0.3f)
				{
					ADCTroubles* Troubles = ADCTroubles::Get(this);
					if (Troubles && Troubles->DeckPoints.Num() && Troubles->NumFires() < Troubles->MaxFires)
					{
						Troubles->StartFire(Troubles->DeckPoints[FMath::RandRange(0, Troubles->DeckPoints.Num() - 1)]);
					}
				}
			}
			Incoming.RemoveAtSwap(i);
			continue;
		}
		FVector P = FMath::Lerp(In.From, In.To, In.T);
		P.Z += FMath::Sin(In.T * UE_PI) * 1400.f;
		Shot->SetActorLocation(P);
	}
}
