#include "DCTreadmill.h"
#include "Components/LocalFogVolumeComponent.h"
#include "DCFogBank.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "DCRideDirector.h"
#include "DCHazards.h"
#include "DCCrew.h"
#include "DCTroubles.h"
#include "DCCloudFace.h"
#include "DCPortal.h"
#include "DCRuins.h"
#include "DCAudio.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
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
	Obj.Base = Base + FVector(SpawnAhead, 0.f, 0.f);
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
	Actor->SetActorLocation(Obj.Base);
}

void ADCTreadmill::SpawnForPhase(EDCPhase Phase, float DeltaSeconds)
{
	float ObstacleEvery = 0.f, HostileEvery = 0.f, SceneryEvery = 0.f;
	switch (Phase)
	{
	case EDCPhase::Fog: ObstacleEvery = 8.f; HostileEvery = bGhostShips ? 16.f : 0.f; SceneryEvery = 15.f; break;
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
			AActor* Rock = (!bHell && DCRuins::Available()) ? DCRuins::SpawnCrag(World, Radius, FMath::FRandRange(1500.f, 3200.f)) : DCHazards::SpawnRock(World, Radius, FMath::FRandRange(1500.f, 4200.f), bHell, Stone);
			if (bHell) { DCHazards::SetGlow(Rock, Ember); }
			Register(Rock, Base, Radius, false, 1.f, FMath::FRandRange(0.f, 360.f), true);
		}
		else
		{
			const float Scale = FMath::FRandRange(0.7f, 1.15f);
			Register(DCRuins::Available() ? DCRuins::SpawnBrokenColumns(World, 1000.f * Scale) : DCHazards::SpawnRuin(World, Scale, Stone), Base, 1000.f * Scale, false, 1.f, FMath::FRandRange(-25.f, 25.f), true);
		}
	}

	// Scenery: off to the sides, never hit, there to loom.
	if (SceneryEvery > 0.f && (SceneryTimer -= DeltaSeconds) <= 0.f)
	{
		SceneryTimer = SceneryEvery * FMath::FRandRange(0.7f, 1.4f);
		const float Side = FMath::RandBool() ? 1.f : -1.f;
		const FVector Base(SpawnDistance, LaneOffset + Side * FMath::FRandRange(4500.f, 9000.f), SeaZ);
		if (!bHell && bSpecters && FMath::FRand() < 0.3f)
		{
			// Specters: pale giants walking on the water, turned toward the ship as it passes.
			Register(DCHazards::SpawnSpecter(World, FMath::FRandRange(2200.f, 5000.f), FLinearColor(0.7f, 1.5f, 1.7f), false), Base, 800.f, false, 1.f, Side > 0.f ? -90.f : 90.f, false);
		}
		else if (!bHell && FMath::FRand() < 0.55f)
		{
			// Statues face the passing ship.
			Register(DCRuins::Available() ? DCRuins::SpawnColossus(World, FMath::FRandRange(6000.f, 9000.f)) : DCHazards::SpawnSentinel(World, FMath::FRandRange(5000.f, 8000.f), FMath::RandBool(), Stone), Base - FVector(0.f, 0.f, 300.f), 1500.f, false, 1.f, Side > 0.f ? 205.f : 155.f, false);
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
		if (bHell && FMath::RandBool())
		{
			// A demon wading in the burning sea, pacing the ship and hurling fire.
			AActor* Demon = DCHazards::SpawnSpecter(World, FMath::FRandRange(2800.f, 3800.f), FLinearColor(1.8f, 0.32f, 0.03f), true);
			if (UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(Side > 0.f ? TEXT("Demon in the fire to starboard!") : TEXT("Demon in the fire to port!"));
			}
			Register(Demon, FVector(SpawnDistance, Side * FMath::FRandRange(3800.f, 5500.f), SeaZ), 900.f, true, 6.f, Side > 0.f ? -90.f : 90.f, false);
		}
		else
		{
			AActor* Ghost = DCHazards::SpawnGhostShip(World, bHell ? FLinearColor(1.f, 0.25f, 0.03f) : FLinearColor(0.15f, 0.9f, 0.75f));
			if (UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(bHell ? (Side > 0.f ? TEXT("Burning ship to starboard!") : TEXT("Burning ship to port!")) : (Side > 0.f ? TEXT("Ghost ship to starboard!") : TEXT("Ghost ship to port!")));
			}
			Register(Ghost, Base, 1600.f, true, 6.f, FMath::FRandRange(-8.f, 8.f), false);
		}
	}
}
void ADCTreadmill::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (VdbTrial)
	{
		static float NextVdbLog = 3.f; static float VdbClock = 0.f;
		VdbClock += DeltaSeconds;
		if (VdbClock > NextVdbLog)
		{
			NextVdbLog += 6.f;
			UE_LOG(LogDC, Log, TEXT("Vdb tunnel: frame %.1f resolution %s bounds %s visible %d material %s"), VdbTrial->Frame, *VdbTrial->VolumeResolution.ToString(), *VdbTrial->Bounds.ToString(), VdbTrial->IsVisible(), *GetNameSafe(VdbTrial->GetMaterial(0)));
		}
	}
	if (!Director)
	{
		Director = ADCRideDirector::Get(this);
		if (!Director) { return; }
	}

	const FDCEnvState& Env = Director->GetEnv();
	const float Speed = Env.ShipSpeed * Director->TimeScale;

	// Test hook: -DCSteer=1 holds the helm hard over (starboard) for the whole run.
	static float ForcedSteer = 0.f;
	static const bool bForced = FParse::Value(FCommandLine::Get(), TEXT("DCSteer="), ForcedSteer);
	if (bForced) { Steer = ForcedSteer; bSteerForced = true; }
	// The helm takes a moment to bite and the ship carries her swing after it is eased: she turns, she is not slid sideways.
	// (In the trough she is given half as much helm again: it is a wide channel and there is a slope to climb.)
	LaneVelocity = FMath::FInterpTo(LaneVelocity, Steer * LaneSpeed * SteerAuthority * (Director->TroughRise > 1.f ? 1.5f : 1.f), DeltaSeconds, 1.1f);
	// In the trough of the drowned city the sea itself brings her back: she slides down the slope she
	// has climbed, harder the higher she is, so full helm holds her near the edge and no further, and
	// with the wheel let go she swings back to the middle. Elsewhere the lane simply ends.
	// (The slope eases off toward the shoulder, so past the middle of it a steady pull is added that grows with the distance out: she cannot be held on the shelf.)
	const float Out = FMath::Abs(LaneOffset) / FMath::Max(Director->TroughHalfWidth, 1.f);
	const float Pull = Director->SeaRiseSlope(LaneOffset) * 4400.f + (Director->TroughRise > 1.f ? FMath::Sign(LaneOffset) * FMath::Max(0.f, Out - 0.45f) * 2600.f : 0.f);
	LaneVelocity -= Pull * DeltaSeconds;
	const float Edge = Director->TroughRise > 1.f ? Director->TroughHalfWidth : LaneLimit;
	LaneOffset = FMath::Clamp(LaneOffset + (LaneVelocity + CurrentDrift) * DeltaSeconds, -Edge, Edge);
	if (FMath::Abs(LaneOffset) >= Edge && LaneOffset * LaneVelocity > 0.f) { LaneVelocity = 0.f; }
	// Test hook: -DCLaneLog writes where she is across the channel twice a second.
	{
		static const bool bLaneLog = FParse::Param(FCommandLine::Get(), TEXT("DCLaneLog"));
		static float LaneLogTimer = 0.f;
		LaneLogTimer -= DeltaSeconds;
		if (bLaneLog && LaneLogTimer <= 0.f)
		{
			LaneLogTimer = 0.5f;
			UE_LOG(LogDC, Log, TEXT("Lane %.0f cm, moving %.0f cm/s, helm %.2f, sea rise here %.0f cm, slope %.3f, at %.1f s"), LaneOffset, LaneVelocity, Steer, Director->SeaRise(LaneOffset), Director->SeaRiseSlope(LaneOffset), Director->RideTime);
		}
	}
	// The water's material lays the trough round the middle of the channel, which is LaneOffset to port of the ship.
	if (!SeaCollection) { SeaCollection = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/DC/Materials/MPC_Sea.MPC_Sea")); }
	if (SeaCollection)
	{
		if (UMaterialParameterCollectionInstance* Sea = GetWorld()->GetParameterCollectionInstance(SeaCollection))
		{
			// The fourth number is how much the wave crests break white: nothing on the open sea, full in the gale over the city.
			static float Caps = -1.f; static bool bCapsRead = false;   // test hook: -DCWhitecaps=
			if (!bCapsRead) { bCapsRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCWhitecaps="), Caps); }
			const float Whitecaps = Caps >= 0.f ? Caps : FMath::SmoothStep(100.f, 185.f, Env.WaveHeight);
			Sea->SetVectorParameterValue(TEXT("Trough"), FLinearColor(-LaneOffset, Director->TroughRise, Director->TroughHalfWidth, Whitecaps));
			// Her wake, drawn by the sea's own material astern of her: stronger the faster she goes. -DCNoWakeFoam switches it off.
			static const bool bNoWake = FParse::Param(FCommandLine::Get(), TEXT("DCNoWakeFoam"));
			Sea->SetVectorParameterValue(TEXT("Wake"), FLinearColor(bNoWake ? 0.f : FMath::Clamp(Env.ShipSpeed / 1300.f, 0.5f, 1.3f), 1900.f, 330.f, 0.2f));
		}
	}
	TickStrikes(DeltaSeconds);
	TickDeckWisps(DeltaSeconds);
	TickGiants(DeltaSeconds);

	// Ruins on the open sea before the fog are off (owner, 13 Nov): the city is first seen in the murk. -DCOutliers brings them back.
	if (!bOutliersBuilt)
	{
		bOutliersBuilt = true;
		if (FParse::Param(FCommandLine::Get(), TEXT("DCOutliers"))) { SpawnOutliers(); }
	}
	// The fog begins at 58 s; until then the ship runs at the open-sea speed. (The test harness seeks
	// in the first frames, so wait a moment before trusting the ride time.)
	const float FogStart = 58.f;
	if (!bAtlantisBuilt && (Director->GetPhase() == EDCPhase::Fog || Director->RideTime > 2.5f))
	{
		bAtlantisBuilt = true;
		const bool bEarly = Director->GetPhase() == EDCPhase::OpenSea && Director->RideTime < FogStart;
		SpawnAhead = bEarly ? Env.ShipSpeed * (FogStart - Director->RideTime) : 0.f;
		const int32 Before = Objects.Num();
		SpawnAtlantis();
		SpawnAhead = 0.f;
		if (bEarly)
		{
			for (int32 i = Before; i < Objects.Num(); ++i)
			{
				if (AActor* A = Objects[i].Actor.Get()) { A->SetActorHiddenInGame(true); AtlantisHidden.Add(A); }
			}
		}
	}
	// Not until the ship is deep in the murk of the front: shown as the fog began, the city popped up close by in thin haze.
	if (AtlantisHidden.Num() && Director->GetPhase() != EDCPhase::OpenSea && Director->RideTime > FogStart + 5.5f)
	{
		// A couple a frame: showing them all at once still cost a quarter of a second. They are inside the front and out of sight meanwhile.
		for (int32 n = 0; n < 2 && AtlantisHidden.Num(); ++n)
		{
			if (AActor* A = AtlantisHidden.Pop().Get()) { A->SetActorHiddenInGame(false); }
		}
	}
	SpawnForPhase(Director->GetPhase(), DeltaSeconds * Director->TimeScale);
	TickIncoming(DeltaSeconds);
	for (int32 w = Wrecks.Num() - 1; w >= 0; --w)
	{
		FDCWreck& Wreck = Wrecks[w];
		Wreck.Age += DeltaSeconds;
		AActor* Hulk = Wreck.Actor.Get();
		if (!Hulk || Wreck.Age > 4.f)
		{
			if (Hulk) { Hulk->Destroy(); }
			Wrecks.RemoveAtSwap(w);
			continue;
		}
		// Settle, heel over and slide under, falling astern.
		Hulk->AddActorWorldOffset(FVector(-Speed * 0.5f * DeltaSeconds, 0.f, -260.f * Wreck.Age * DeltaSeconds));
		Hulk->AddActorLocalRotation(FRotator(-9.f * DeltaSeconds, 0.f, 14.f * DeltaSeconds));
	}

	const EDCPhase Now = Director->GetPhase();
	const bool bLeavingAtlantis = Now == EDCPhase::Slide || Now == EDCPhase::Breach || Now == EDCPhase::Maelstrom || Now == EDCPhase::Ending;
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


		// Once the sea rolls up, nothing from Atlantis follows the ship in. Ghost ships founder at
		// once (alongside in the tunnel they filled half the view from the helm); the rest goes under
		// as the walls close, so the breach opens on an empty bowl.
		if (bLeavingAtlantis)
		{
			if (Obj.bHostile)
			{
				Wrecks.Add({ Actor, 0.f });
				Objects.RemoveAtSwap(i);
				continue;
			}
			if (Env.Tube > 0.5f || Director->GetPhase() != EDCPhase::Slide)
			{
				Obj.Base.Z -= 30000.f * DeltaSeconds * Director->TimeScale;
				if (Obj.Base.Z < -160000.f)
				{
					Actor->Destroy();
					Objects.RemoveAtSwap(i);
					continue;
				}
			}
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
		// Radius 0 marks a destination (the portal): it stays dead ahead whatever the helm does.
		FVector Local(Obj.Base.X, (bPacing || Obj.Radius < 1.f) ? Obj.Base.Y : Obj.Base.Y - LaneOffset, Obj.Base.Z);
		// Whatever stands in the sea stands on the sea as it lies there: higher up the sides of the trough than the ship in it.
		// A wide ruin stands across the slope: it is set at the height of the sea at its edge nearest the
		// channel, so its far side is drowned deeper. (Set at the height under its middle, its near edge
		// hung in the air over lower water: a great flat slab seen from the deck. Owner, 13 Nov.)
		if (Director->TroughRise > 0.f)
		{
			const float Mid = Local.Y + LaneOffset;
			// (Tried setting a wide ruin at the height of the sea at its edge nearest the channel, to stop its
			// near edge hanging over lower water: its base then lay in the ship's path and she struck 31 times in a voyage. Back to the height under its middle.)
			Local.Z += Director->SeaRise(Mid) - Director->SeaRise(LaneOffset);
		}
		Actor->SetActorLocationAndRotation(Rock.TransformPosition(Local), Rock.GetRotation() * FRotator(0.f, Obj.Yaw, 0.f).Quaternion());
		// Stone alongside the cloud tunnel showed through its wall as a dark outline. It is kept out of sight until the
		// tunnel's far end has gone by it, and so comes out of the cloud. (Radius over 100: stone, not the fog's own banks and cards.)
		if (Obj.Radius > 100.f)
		{
			const bool bHide = Local.X < Director->CloudTunnelFarX;
			if (bHide != Obj.bTunnelHid) { Obj.bTunnelHid = bHide; Actor->SetActorHiddenInGame(bHide); }
		}
		// (Rocks in the lane used to be a circle the ship ran through: three damage and the rock vanished. They are
		// stone like the rest now: the hull's feelers find them and she is shouldered off. Owner, 14 Nov.)
		if (!Obj.bHostile && Obj.Radius > 100.f)
		{
			// Scenery is solid stone. Its meshes answer the hull's feelers only while it is near the ship
			// (moving hundreds of colliding pieces every frame for the whole city would cost too much).
			if (Obj.Reach <= 0.f)
			{
				FVector Origin, Extent;
				Actor->GetActorBounds(false, Origin, Extent);
				Obj.Reach = FVector2D(Extent.X, Extent.Y).Size() + FVector2D(Origin - Actor->GetActorLocation()).Size();
			}
			// (The faces in the cloud are registered as scenery too; they are kilometres across and not stone.)
			const bool bNear = Obj.Reach < 90000.f && !Actor->IsA<ADCFogBank>() && FVector2D(Local.X, Local.Y).Size() < Obj.Reach + 20000.f;
			if (bNear != Obj.bFeelable)
			{
				Obj.bFeelable = bNear;
				UE_LOG(LogDC, Log, TEXT("Strikes: %s %s at %.0f, %.0f (reach %.0f), lane %.0f"), *Actor->GetName(), bNear ? TEXT("near") : TEXT("left astern"), Local.X, Local.Y, Obj.Reach, LaneOffset);
				TInlineComponentArray<UStaticMeshComponent*> Pieces(Actor);
				for (UStaticMeshComponent* C : Pieces)
				{
					if (C->IsA<UInstancedStaticMeshComponent>()) { continue; }   // floors of many blocks: looked at, not felt (an unseen block stands in for them)
					C->SetCollisionObjectType(ECC_Destructible);
					C->SetCollisionResponseToAllChannels(ECR_Ignore);
					C->SetCollisionEnabled(bNear ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
				}
			}
		}

		// Lookout: call an obstacle once, when it is close enough to matter and near our course.
		if (!Obj.bHostile && Obj.bSolid && !Obj.bCalled && Local.X < 13000.f && FMath::Abs(Local.Y) < ShipHalfWidth + Obj.Radius + 150.f)
		{
			Obj.bCalled = true;
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(FMath::Abs(Local.Y) < 250.f ? TEXT("Rocks dead ahead!") : (Local.Y > 0.f ? TEXT("Rocks ahead, off the starboard bow!") : TEXT("Rocks ahead, off the port bow!")));
			}
		}


		// Scenery is big and seen from far off, so it is kept until it is well astern.
		if (Local.X < -(DespawnDistance + ((Obj.bSolid || Obj.bHostile) ? 0.f : 90000.f)))
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
		float Dist = FVector2D(Obj.Base.X, Y).Size();
		if (Dist >= MaxRange) { continue; }
		if (Obj.bUrgent) { Dist -= MaxRange; }   // in range and urgent: before anything nearer
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
		if (AActor* Actor = Obj.Actor.Get())
		{
			// Kraken arms are retired by their owner. A ship goes down by the stern instead of vanishing.
			if (Obj.bExternal) { Actor->Destroy(); }
			else
			{
				Wrecks.Add({ Actor, 0.f });
				if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->Report(TEXT("She is going down! Good shooting!")); }
			}
		}
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

void ADCTreadmill::AddExternalHostile(AActor* Actor, float Radius, float Health, bool bUrgent)
{
	FDCWorldObject& Obj = Objects.AddDefaulted_GetRef();
	Obj.Actor = Actor;
	Obj.Base = Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
	Obj.Radius = Radius;
	Obj.bHostile = true;
	Obj.bExternal = true;
	Obj.bUrgent = bUrgent;
	Obj.Health = Health;
}

float ADCTreadmill::FindClearLane(float Lookahead) const
{
	float Best = LaneOffset;
	float BestCost = TNumericLimits<float>::Max();
	// In the trough she cannot hold a lane much beyond three quarters of the way to its edge.
	const float Reach = (Director && Director->TroughRise > 1.f) ? LaneLimit * 0.7f : LaneLimit;
	for (float Candidate = -LaneLimit; Candidate <= LaneLimit; Candidate += 250.f)
	{
		if (FMath::Abs(Candidate) > Reach) { continue; }
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
		// Stone of the drowned city on this lane (TickStrikes looks for it).
		const int32 Lane = FMath::RoundToInt32((Candidate + LaneLimit) / 250.f);
		if (StoneAhead.IsValidIndex(Lane) && StoneAhead[Lane] >= 0.f) { Cost += 10.f * (1.f + 8000.f / (StoneAhead[Lane] + 2500.f)); }
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

namespace
{
	UAnimSequence* GiantAnim(const TCHAR* Name)
	{
		return LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

void ADCTreadmill::TickGiants(float DeltaSeconds)
{
	if (!Director) { return; }
	static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("DCNoGiants"));
	// Test hooks: -DCGiantScale= (he is about 3 m tall at 1), -DCGiantYaw= (which way the mesh faces), -DCGiantSunk= (cm of him under water),
	// -DCGiantAt=<ride time> sends an extra one to starboard.
	static float Scale = 16.f, MeshYaw = -90.f, Sunk = 550.f, Extra = -1.f; static bool bRead = false;
	if (!bRead)
	{
		bRead = true;
		FParse::Value(FCommandLine::Get(), TEXT("DCGiantScale="), Scale);
		FParse::Value(FCommandLine::Get(), TEXT("DCGiantYaw="), MeshYaw);
		FParse::Value(FCommandLine::Get(), TEXT("DCGiantSunk="), Sunk);
		FParse::Value(FCommandLine::Get(), TEXT("DCGiantAt="), Extra);
	}
	const FDCEnvState& Env = Director->GetEnv();
	const float Speed = Env.ShipSpeed * Director->TimeScale;
	const float T = Director->RideTime;
	UWorld* World = GetWorld();
	UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>();
	const bool bCity = Director->GetPhase() == EDCPhase::Fog && Env.Mist > 0.55f;

	// When they come: every twenty-odd seconds through the city, port and starboard by turns.
	static const float SendAt[] = { 96.f, 118.f, 141.f, 164.f, 187.f, 208.f };
	bool bSend = false;
	float Side = 1.f;
	if (T < SendAt[0] - 5.f) { GiantsSent = 0; }   // the voyage was begun again
	while (!bOff && GiantsSent < UE_ARRAY_COUNT(SendAt) && T >= SendAt[GiantsSent])
	{
		Side = GiantsSent % 2 == 0 ? -1.f : 1.f;
		if (T <= SendAt[GiantsSent] + 2.f && bCity) { bSend = true; }
		++GiantsSent;
	}
	if (Extra >= 0.f && T >= Extra && T < Extra + 1.f) { Extra = -1.f; bSend = true; Side = 1.f; }
	if (bSend)
	{
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Skins/Tier2/Elemental/Meshes/Rampage_Elemental.Rampage_Elemental"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASkeletalMeshActor* A = Mesh ? World->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params) : nullptr;
		if (A)
		{
			USkeletalMeshComponent* C = A->GetSkeletalMeshComponent();
			C->SetMobility(EComponentMobility::Movable);
			C->SetSkeletalMesh(Mesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			A->SetActorScale3D(FVector(Scale));
			if (UAnimSequence* Jog = GiantAnim(TEXT("Jog_Biped_Fwd"))) { C->PlayAnimation(Jog, true); }
			FGiant G;
			G.Actor = A;
			G.Side = Side;
			G.X = 46000.f;                 // 460 m ahead: out of sight in the mist
			G.Y = Side * 15500.f;          // out beyond the bank; he wades in to it
			if (AActor* Mark = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params))
			{
				USceneComponent* Root = NewObject<USceneComponent>(Mark);
				Root->SetMobility(EComponentMobility::Movable);
				Mark->SetRootComponent(Root);
				Root->RegisterComponent();
				AddExternalHostile(Mark, 2200.f, 4.f, true);
				G.Mark = Mark;
			}
			Giants.Add(G);
			UE_LOG(LogDC, Log, TEXT("Giant sent at %.1f s to the %s bank"), T, Side < 0.f ? TEXT("port") : TEXT("starboard"));
		}
		else { UE_LOG(LogDC, Log, TEXT("Giant: the stone Rampage mesh was not found (pack not installed); none sent")); }
	}

	const FTransform Rock = Director->GetWorldRock();
	for (int32 i = Giants.Num() - 1; i >= 0; --i)
	{
		FGiant& G = Giants[i];
		ASkeletalMeshActor* A = G.Actor.Get();
		if (!A) { if (AActor* M = G.Mark.Get()) { M->Destroy(); } Giants.RemoveAt(i); continue; }
		USkeletalMeshComponent* C = A->GetSkeletalMeshComponent();
		G.X -= Speed * DeltaSeconds;
		G.StateTime += DeltaSeconds;
		// The guns have killed him: he goes down where he stands.
		if (G.State != 4 && !G.Mark.IsValid())
		{
			G.State = 4; G.StateTime = 0.f;
			if (UAnimSequence* Death = GiantAnim(TEXT("Death_A"))) { C->PlayAnimation(Death, false); }
			Director->Score += 200;
			if (Crew) { Crew->Report(TEXT("The giant is down! Good shooting!"), true); }
			UE_LOG(LogDC, Log, TEXT("Giant killed at %.1f s, %.0f m ahead"), T, G.X / 100.f);
		}
		const float Bank = 8200.f;   // he stands 82 m from the middle of the channel
		float Down = Sunk;
		switch (G.State)
		{
		case 0:
			G.Y = G.Side * FMath::Max(Bank, FMath::Abs(G.Y) - 420.f * DeltaSeconds);
			if (!G.bCalled && G.X < 30000.f)
			{
				G.bCalled = true;
				if (Crew) { Crew->Report(G.Side < 0.f ? TEXT("Giant wading in on the port bank! Port battery, bring it down!") : TEXT("Giant wading in on the starboard bank! Starboard battery, bring it down!"), true); }
			}
			if (FMath::Abs(G.Y) <= Bank + 1.f && G.X < 15000.f && G.X > -6000.f)
			{
				G.State = 1; G.StateTime = 0.f;
				if (UAnimSequence* Rip = GiantAnim(TEXT("Ability_RipNToss_Rip"))) { C->PlayAnimation(Rip, false); }
			}
			break;
		case 1:
			// The tell: he tears a rock out of the sea bed.
			if (G.StateTime > 1.6f)
			{
				G.State = 2; G.StateTime = 0.f; G.bThrown = false;
				if (UAnimSequence* Toss = GiantAnim(TEXT("Ability_RipNToss_Toss"))) { C->PlayAnimation(Toss, false); }
			}
			break;
		case 2:
			if (!G.bThrown && G.StateTime > 0.45f)
			{
				G.bThrown = true;
				FFireball F;
				F.From = A->GetActorLocation() + FVector(0.f, 0.f, 330.f * Scale);
				// At the deck, somewhere along her length.
				F.To = FVector(FMath::FRandRange(-1300.f, 1400.f), FMath::FRandRange(-200.f, 200.f), Director->SeaLevelZ + 620.f);
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (AStaticMeshActor* B = World->SpawnActor<AStaticMeshActor>(F.From, FRotator::ZeroRotator, Params))
				{
					UStaticMeshComponent* M = B->GetStaticMeshComponent();
					M->SetMobility(EComponentMobility::Movable);
					UStaticMesh* Held = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ParagonRampage/FX/Meshes/Debris/SM_Rock_To_Hold.SM_Rock_To_Hold"), nullptr, LOAD_NoWarn | LOAD_Quiet);
					M->SetStaticMesh(Held ? Held : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
					M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					M->SetCastShadow(false);
					if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Glow.M_Glow")))
					{
						UMaterialInstanceDynamic* Hot = UMaterialInstanceDynamic::Create(Glow, B);
						Hot->SetVectorParameterValue(TEXT("Color"), FLinearColor(11.f, 2.6f, 0.3f));
						for (int32 Slot = 0; Slot < M->GetNumMaterials(); ++Slot) { M->SetMaterial(Slot, Hot); }
					}
					const float Through = FMath::Max(M->GetStaticMesh() ? M->GetStaticMesh()->GetBoundingBox().GetSize().GetMax() : 100.f, 1.f);
					B->SetActorScale3D(FVector(260.f / Through));
					UPointLightComponent* Light = NewObject<UPointLightComponent>(B);
					Light->SetMobility(EComponentMobility::Movable);
					Light->SetupAttachment(M);
					Light->SetIntensity(120000.f);
					Light->SetAttenuationRadius(4500.f);
					Light->SetLightColor(FLinearColor(1.f, 0.45f, 0.12f));
					Light->SetCastShadows(false);
					Light->RegisterComponent();
					F.Actor = B;
				}
				Fireballs.Add(F);
				if (Crew) { Crew->Report(TEXT("Fire incoming!"), true); }
			}
			if (G.StateTime > 1.5f)
			{
				G.State = 3; G.StateTime = 0.f;
				if (UAnimSequence* Idle = GiantAnim(TEXT("Idle"))) { C->PlayAnimation(Idle, true); }
			}
			break;
		case 3:
			// He throws again every six seconds while the ship is in his reach.
			if (G.StateTime > 4.f && G.X > -6000.f)
			{
				G.State = 1; G.StateTime = 0.f;
				if (UAnimSequence* Rip = GiantAnim(TEXT("Ability_RipNToss_Rip"))) { C->PlayAnimation(Rip, false); }
			}
			break;
		default:
			Down = Sunk + FMath::Max(0.f, G.StateTime - 2.f) * 500.f;
			break;
		}
		FVector Local(G.X, G.Y - LaneOffset, Director->SeaLevelZ + Director->SeaRise(G.Y) - Director->SeaRise(LaneOffset) - Down);
		// He faces the ship.
		const float Facing = FMath::RadiansToDegrees(FMath::Atan2(-Local.Y, -Local.X));
		A->SetActorLocationAndRotation(Rock.TransformPosition(Local), FRotator(0.f, Facing + MeshYaw, 0.f));
		if (AActor* M = G.Mark.Get()) { M->SetActorLocation(A->GetActorLocation() + FVector(0.f, 0.f, 220.f * Scale)); }
		if (G.X < -16000.f || (G.State == 4 && G.StateTime > 6.f) || Director->GetPhase() != EDCPhase::Fog)
		{
			if (AActor* M = G.Mark.Get()) { M->Destroy(); }
			A->Destroy();
			Giants.RemoveAt(i);
		}
	}

	// What they throw: three seconds in a high arc, then fire where it lands.
	for (int32 i = Fireballs.Num() - 1; i >= 0; --i)
	{
		FFireball& F = Fireballs[i];
		F.Age += DeltaSeconds;
		const float U = FMath::Clamp(F.Age / 3.f, 0.f, 1.f);
		const FVector Where = FMath::Lerp(F.From, F.To, U) + FVector(0.f, 0.f, 4.f * 4500.f * U * (1.f - U));
		if (AActor* B = F.Actor.Get()) { B->SetActorLocation(Where); B->SetActorRotation(FRotator(F.Age * 260.f, F.Age * 170.f, 0.f)); }
		if (U < 1.f) { continue; }
		Director->DamageShip(2.f);
		ADCTroubles* Troubles = ADCTroubles::Get(this);
		if (Troubles && Troubles->DeckPoints.Num() && Troubles->NumFires() < Troubles->MaxFires)
		{
			FVector Best = Troubles->DeckPoints[0];
			for (const FVector& P : Troubles->DeckPoints) { if (FVector::DistSquared2D(P, F.To) < FVector::DistSquared2D(Best, F.To)) { Best = P; } }
			Troubles->StartFire(Best);
		}
		if (ADCAudio* Sound = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(World, ADCAudio::StaticClass()))) { Sound->Boom(0.7f); }
		UE_LOG(LogDC, Log, TEXT("Giant fire struck the deck at %.1f s"), T);
		if (AActor* B = F.Actor.Get()) { B->Destroy(); }
		Fireballs.RemoveAt(i);
	}
}

void ADCTreadmill::TickDeckWisps(float DeltaSeconds)
{
	if (DeckWisps.Num() == 0 || !Director) { return; }
	// Only in the fog proper: they come with the mist and go with it.
	const float Mist = FMath::SmoothStep(0.45f, 0.7f, Director->GetEnv().Mist);
	const float T = Director->RideTime;
	for (FDeckWisp& W : DeckWisps)
	{
		AActor* A = W.Actor.Get();
		if (!A) { continue; }
		A->SetActorHiddenInGame(Mist <= 0.f);
		if (Mist <= 0.f) { continue; }
		// From 45 m ahead of the bow to 45 m astern, then round again.
		const float Span = 9000.f;
		const float U = FMath::Frac(W.Phase + T * W.Speed / Span);
		const float X = FMath::Lerp(4500.f, -4500.f, U);
		const float Y = W.Side + 160.f * FMath::Sin(T * 0.5f + W.Phase * 20.f);
		const float Z = Director->SeaLevelZ + W.Height + 90.f * FMath::Sin(T * 0.7f + W.Phase * 11.f);
		// Upright, broadside to the wind, leaning with it.
		A->SetActorLocationAndRotation(FVector(X, Y, Z), FRotator(72.f, 0.f, 12.f * FMath::Sin(T * 0.3f + W.Phase * 5.f)));
		const float Ends = FMath::SmoothStep(0.f, 0.15f, U) * (1.f - FMath::SmoothStep(0.85f, 1.f, U));
		if (UMaterialInstanceDynamic* M = W.MID.Get()) { M->SetScalarParameterValue(TEXT("Fog Density"), W.Density * Mist * Ends); }
	}
}

void ADCTreadmill::TickStrikes(float DeltaSeconds)
{
	StrikeCooldown -= DeltaSeconds;
	StrikeCallCooldown -= DeltaSeconds;
	SinceContact += DeltaSeconds;
	if (SinceContact > 1.5f) { StrikeAway = 0.f; }
	if (!Director || FParse::Param(FCommandLine::Get(), TEXT("DCNoStrikes"))) { return; }
	bool bAny = false;
	for (const FDCWorldObject& Obj : Objects) { if (Obj.bFeelable) { bAny = true; break; } }
	if (!bAny) { StoneAhead.Reset(); CurrentDrift = FMath::FInterpTo(CurrentDrift, 0.f, DeltaSeconds, 1.5f); return; }

	// The helmsman's look ahead: for each lane he might take, three lines run forward from the bow
	// (keel and both sides) and the nearest stone on any of them is remembered.
	StoneLookTimer -= DeltaSeconds;
	if (StoneLookTimer <= 0.f)
	{
		StoneLookTimer = 0.2f;
		const int32 Lanes = FMath::RoundToInt32(2.f * LaneLimit / 250.f) + 1;
		StoneAhead.Init(-1.f, Lanes);
		FCollisionQueryParams Look(TEXT("DCStoneLook"), true);
		const FCollisionObjectQueryParams LookFor(ECC_Destructible);
		const float LookZ = Director->SeaLevelZ + 140.f;
		for (int32 i = 0; i < Lanes; ++i)
		{
			const float Y = -LaneLimit + i * 250.f - LaneOffset;
			for (float Off : { 0.f, -ShipHalfWidth - 150.f, ShipHalfWidth + 150.f })
			{
				FHitResult Hit;
				if (GetWorld()->LineTraceSingleByObjectType(Hit, FVector(-ShipHalfLength, Y + Off, LookZ), FVector(16000.f, Y + Off, LookZ), LookFor, Look))
				{
					const float Ahead = FMath::Max(0.f, Hit.ImpactPoint.X);
					if (StoneAhead[i] < 0.f || Ahead < StoneAhead[i]) { StoneAhead[i] = Ahead; }
				}
			}
		}
	}

	// The current parts round big stone. Three lines run 90 m forward from the bow: along the keel and along each side.
	// Stone on the keel's line means she would take it on the stem: the current sets her toward whichever side is
	// clear (or clearer), harder the nearer it is, until the keel's line is free. Stone on a side's line alone is
	// left: she grazes it. -DCNoCurrent switches this off.
	{
		static const bool bNoCurrent = FParse::Param(FCommandLine::Get(), TEXT("DCNoCurrent"));
		FCollisionQueryParams Ahead(TEXT("DCCurrent"), true);
		const FCollisionObjectQueryParams StoneAheadOf(ECC_Destructible);
		const float LineZ = Director->SeaLevelZ + 140.f, Far = 15000.f, BowX = 1200.f;   // 150 m: ten seconds of warning at her speed in the city
		float Hit[3] = { -1.f, -1.f, -1.f };
		float FaceY = 0.f;
		const float Offs[3] = { 0.f, -(ShipHalfWidth + 120.f), ShipHalfWidth + 120.f };
		for (int32 k = 0; k < 3; ++k)
		{
			FHitResult H;
			if (GetWorld()->LineTraceSingleByObjectType(H, FVector(BowX, Offs[k], LineZ), FVector(BowX + Far, Offs[k], LineZ), StoneAheadOf, Ahead))
			{
				Hit[k] = FMath::Max(0.f, H.ImpactPoint.X - BowX);
				if (k == 0) { FaceY = H.ImpactNormal.Y; }
			}
		}
		float Want = 0.f;
		if (!bNoCurrent && Hit[0] >= 0.f)
		{
			float Dir;
			if (Hit[1] < 0.f && Hit[2] >= 0.f) { Dir = -1.f; }
			else if (Hit[2] < 0.f && Hit[1] >= 0.f) { Dir = 1.f; }
			else if (Hit[1] >= 0.f && Hit[2] >= 0.f) { Dir = Hit[1] > Hit[2] ? -1.f : 1.f; }
			else { Dir = FMath::Abs(FaceY) > 0.05f ? FMath::Sign(FaceY) : (CurrentDrift != 0.f ? FMath::Sign(CurrentDrift) : (LaneOffset >= 0.f ? -1.f : 1.f)); }
			// Keep to the side already chosen while the same stone is ahead.
			if (FMath::Abs(CurrentDrift) > 40.f) { Dir = FMath::Sign(CurrentDrift); }
			const float Urge = 1.f - Hit[0] / Far;
			Want = Dir * FMath::Lerp(260.f, 1200.f, Urge);
		}
		CurrentDrift = FMath::FInterpTo(CurrentDrift, Want, DeltaSeconds, Want != 0.f ? 5.f : 1.5f);
		static const bool bCurrentLog = FParse::Param(FCommandLine::Get(), TEXT("DCCurrentLog"));
		static float CurrentLogTimer = 0.f;
		CurrentLogTimer -= DeltaSeconds;
		if (bCurrentLog && CurrentLogTimer <= 0.f && (Hit[0] >= 0.f || Hit[1] >= 0.f || Hit[2] >= 0.f))
		{
			CurrentLogTimer = 0.5f;
			UE_LOG(LogDC, Log, TEXT("Current: keel %.0f port %.0f starboard %.0f cm ahead, want %.0f, drift %.0f, lane %.0f moving %.0f, helm %.2f, at %.1f s"), Hit[0], Hit[1], Hit[2], Want, CurrentDrift, LaneOffset, LaneVelocity, Steer, Director->RideTime);
		}
	}

	// Feelers at the waterline, in the ship's resting place (she lies at the origin, bow to +X): out
	// from the centreline to the planking on both sides, and forward from the foremast round the bow.
	const float Z = Director->SeaLevelZ + 140.f;
	struct FFeeler { FVector From, To; };
	TArray<FFeeler, TInlineAllocator<32>> Feelers;
	for (int32 i = 0; i < 9; ++i)
	{
		const float X = -1900.f + i * 480.f;
		// Half the beam at this station: full amidships, fining toward the bow, a little toward the stern.
		const float U = X / 2400.f;
		const float HalfBeam = ShipHalfWidth * (U > 0.25f ? FMath::Sqrt(FMath::Max(0.f, 1.f - FMath::Square((U - 0.25f) / 0.8f))) : FMath::Lerp(1.f, 0.72f, FMath::Clamp((0.25f - U) / 1.05f, 0.f, 1.f)));
		for (float S : { -1.f, 1.f }) { Feelers.Add({ FVector(X, 0.f, Z), FVector(X, S * (HalfBeam + 40.f), Z) }); }
	}
	for (float Deg : { -50.f, -25.f, 0.f, 25.f, 50.f })
	{
		const float R = FMath::DegreesToRadians(Deg);
		Feelers.Add({ FVector(1500.f, 0.f, Z), FVector(1500.f + FMath::Cos(R) * 1050.f, FMath::Sin(R) * 420.f, Z) });
	}

	FCollisionQueryParams Params(TEXT("DCStrike"), true);
	const FCollisionObjectQueryParams Stone(ECC_Destructible);
	float Deepest = 0.f, PushY = 0.f;
	FVector Where = FVector::ZeroVector, Normal = FVector::ZeroVector;
	for (const FFeeler& F : Feelers)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByObjectType(Hit, F.From, F.To, Stone, Params)) { continue; }
		// A feeler that begins inside the stone (she has driven her bow into it) counts for its whole length.
		const float In = Hit.bStartPenetrating ? (F.To - F.From).Size() : (F.To - Hit.ImpactPoint).Size();
		if (In > Deepest) { Deepest = In; Where = Hit.ImpactPoint; Normal = Hit.ImpactNormal; }
		// Which way to shoulder her: away from the side the stone is on, or for stone dead ahead, the way its face turns her.
		const float Side = FMath::Abs(Hit.ImpactPoint.Y) > 60.f ? FMath::Sign(Hit.ImpactPoint.Y) : (FMath::Abs(Hit.ImpactNormal.Y) > 0.1f ? -FMath::Sign(Hit.ImpactNormal.Y) : (LaneOffset >= 0.f ? -1.f : 1.f));
		PushY -= Side * In;
	}
	if (Deepest <= 0.f) { return; }

	// She cannot sail through it: shoulder her off the stone (the sea is moved, not the ship) and kill her swing toward it.
	if (StrikeAway == 0.f) { StrikeAway = PushY >= 0.f ? 1.f : -1.f; }
	const float Away = StrikeAway;
	const bool bFirst = SinceContact > 1.5f;
	SinceContact = 0.f;
	const float Closing = FMath::Max(0.f, -LaneVelocity * Away) + Director->GetEnv().ShipSpeed * Director->TimeScale * FMath::Max(0.f, -Normal.X);
	LaneOffset = FMath::Clamp(LaneOffset + Away * FMath::Min(Deepest + 20.f, 4000.f * DeltaSeconds + 30.f), -LaneLimit - 3000.f, LaneLimit + 3000.f);
	if (LaneVelocity * Away < 0.f) { LaneVelocity = -LaneVelocity * 0.3f + Away * 160.f; }
	else { LaneVelocity = FMath::Max(FMath::Abs(LaneVelocity), 160.f) * Away; }

	if (StrikeCooldown <= 0.f)
	{
		// The first blow is the heavy one; while she grinds along the stone, lighter ones follow.
		const float Strength = FMath::Clamp(Closing / 900.f, 0.25f, 1.5f) * (bFirst ? 1.f : 0.4f);
		StrikeCooldown = bFirst ? 0.5f : 0.8f;
		Director->StrikeHull(Where, Strength, !bFirst);
		UE_LOG(LogDC, Log, TEXT("Hull struck stone at %s (in %.0f cm, closing %.0f cm/s, strength %.2f) at ride time %.1f"), *Where.ToCompactString(), Deepest, Closing, Strength, Director->RideTime);
		if (StrikeCallCooldown <= 0.f)
		{
			StrikeCallCooldown = 7.f;
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(FMath::Abs(Where.Y) < 150.f ? TEXT("We have struck, Captain! Stone under the bow!") : (Where.Y > 0.f ? TEXT("She is grinding on the stone to starboard!") : TEXT("She is grinding on the stone to port!")), true);
			}
		}
	}
}

void ADCTreadmill::SpawnOutliers()
{
	// The first sign of the drowned city, out on the open sea in full sun before the storm closes in:
	// a broken arch, a lone statue, a run of columns, passed close on either hand in the first minute.
	if (!DCRuins::Available() || FParse::Param(FCommandLine::Get(), TEXT("DCNoOutliers"))) { return; }
	UWorld* World = GetWorld();
	const float SeaZ = Director ? Director->SeaLevelZ : 0.f;
	Register(DCRuins::SpawnArch(World, 3, 2.6f), FVector(21000.f, -10500.f, SeaZ - 250.f), 2000.f, false, 1.f, 70.f, false);
	Register(DCRuins::SpawnColossus(World, 11000.f), FVector(33000.f, 9000.f, SeaZ - 300.f), 2000.f, false, 1.f, 215.f, false);
	Register(DCRuins::SpawnBrokenColumns(World, 1500.f), FVector(27000.f, 5200.f, SeaZ), 2000.f, false, 1.f, 30.f, false);
	Register(DCRuins::SpawnColonnade(World, 6, 7000.f), FVector(47000.f, -13000.f, SeaZ - 200.f), 2000.f, false, 1.f, 25.f, false);
	Register(DCRuins::SpawnBrokenColumns(World, 1800.f), FVector(52000.f, 8000.f, SeaZ), 2000.f, false, 1.f, -15.f, false);
	Register(DCRuins::SpawnCrag(World, 1600.f, 5200.f), FVector(40000.f, -22000.f, SeaZ), 2000.f, false, 1.f, 40.f, false);
	Register(DCRuins::SpawnArch(World, 1, 2.2f), FVector(63000.f, -9500.f, SeaZ - 250.f), 2000.f, false, 1.f, 100.f, false);
	Register(DCRuins::SpawnColonnade(World, 5, 8000.f), FVector(72000.f, 12500.f, SeaZ - 200.f), 2000.f, false, 1.f, -20.f, false);
}

void ADCTreadmill::SpawnAtlantis()
{
	// The drowned city, laid out once along the two or three kilometres the ship covers in the fog.
	// Positions are in sea space (cm ahead of the ship now, cm to starboard), so each landmark comes
	// up out of the haze in its turn: sentinels flanking the course, aqueducts crossing behind them,
	// the high temple far ahead to starboard, and vast figures standing in the cloud beyond.
	UWorld* World = GetWorld();
	const float SeaZ = Director ? Director->SeaLevelZ : 0.f;
	const FLinearColor Stone(0.085f, 0.1f, 0.11f);
	const bool bRuins = DCRuins::Available();

	struct FSentinel { float X, Y, Height; bool bTrident; };
	const FSentinel Sentinels[] = {
		{ 70000.f, -12000.f, 11000.f, false }, { 76000.f, 13000.f, 11500.f, false },
		{ 150000.f, 17000.f, 15000.f, true }, { 160000.f, -16000.f, 13000.f, false },
		{ 240000.f, -14000.f, 14000.f, true }, { 252000.f, 15000.f, 16000.f, false } };
	for (const FSentinel& S : Sentinels)
	{
		// Turned to watch the ship come.
		Register(bRuins ? DCRuins::SpawnColossus(World, S.Height * 1.5f) : DCHazards::SpawnSentinel(World, S.Height, S.bTrident, Stone), FVector(S.X, S.Y, SeaZ - 300.f), 2000.f, false, 1.f, S.Y > 0.f ? 205.f : 155.f, false);
	}

	Register(bRuins ? DCRuins::SpawnColonnade(World, 9, 9000.f) : DCHazards::SpawnAqueduct(World, 8, 5500.f, Stone), FVector(95000.f, 21000.f, SeaZ - 200.f), 2000.f, false, 1.f, 18.f, false);
	Register(bRuins ? DCRuins::SpawnColonnade(World, 12, 11000.f) : DCHazards::SpawnAqueduct(World, 10, 7000.f, Stone), FVector(185000.f, -27000.f, SeaZ - 200.f), 2000.f, false, 1.f, -22.f, false);
	Register(bRuins ? DCRuins::SpawnColonnade(World, 8, 10000.f) : DCHazards::SpawnAqueduct(World, 7, 6000.f, Stone), FVector(275000.f, 24000.f, SeaZ - 200.f), 2000.f, false, 1.f, 62.f, false);

	Register(bRuins ? DCRuins::SpawnTemple(World, 9.f) : DCHazards::SpawnTemple(World, 1.f, Stone), FVector(345000.f, 36000.f, SeaZ), 2000.f, false, 1.f, bRuins ? 76.f : -14.f, false);
	if (bRuins)
	{
		// The city proper: gates and arches the course threads between, temples on both hands, all at
		// several depths so that there is always something large coming up out of the mist.
		struct FPiece { int32 Kind; float X, Y, Scale, Yaw; };   // Kind 1..4 arch variants, 5 gate, 6 temple
		const FPiece City[] = {
			{ 5, 60000.f, 0.f, 5.f, 90.f },
			{ 2, 88000.f, -24000.f, 6.f, 60.f }, { 6, 112000.f, 30000.f, 5.f, 100.f }, { 4, 132000.f, -30000.f, 6.f, 75.f },
			{ 1, 172000.f, 26000.f, 5.f, 110.f }, { 6, 205000.f, -40000.f, 7.f, 70.f }, { 3, 222000.f, 22000.f, 6.f, 95.f },
			{ 5, 262000.f, 0.f, 9.f, 90.f }, { 4, 300000.f, -32000.f, 8.f, 80.f }, { 1, 318000.f, 30000.f, 8.f, 100.f } };
		// Sanctuaries: whole complexes on platforms, set back on either hand and turned to face the course.
		struct FSite { float X, Y, Scale; int32 Seed; };
		const FSite Sites[] = { { 92000.f, -30000.f, 3.6f, 11 }, { 140000.f, 34000.f, 4.2f, 23 }, { 200000.f, -40000.f, 5.f, 37 }, { 262000.f, 46000.f, 6.f, 41 }, { 330000.f, -60000.f, 8.f, 53 } };
		for (const FSite& S : Sites)
		{
			Register(DCRuins::SpawnComplex(World, S.Scale, S.Seed), FVector(S.X, S.Y, SeaZ), 2000.f, false, 1.f, S.Y > 0.f ? -90.f : 90.f, false);
		}
		for (const FPiece& P : City)
		{
			AActor* Built = P.Kind == 5 ? DCRuins::SpawnGate(World, P.Scale) : P.Kind == 6 ? DCRuins::SpawnTemple(World, P.Scale) : DCRuins::SpawnArch(World, P.Kind, P.Scale);
			// A gate stands across the course (radius 0: it is sailed through, not pushed aside).
			Register(Built, FVector(P.X, P.Y, SeaZ), P.Kind == 5 ? 0.f : 2000.f, false, 1.f, P.Yaw, false);
		}
	}

	// Fog laid by hand among the ruins (DCFogBank: real volumes of fog, lit and shadowed like the air).
	// Three kinds. Walls to either hand of the course, low and thick, which leave a way through and
	// drown the feet of the ruins so that their heads stand out of it. Curtains, tall, in front of each
	// great figure, which part when the ship is close: that is how a colossus comes out of nowhere.
	// And veils lying across the course that the ship sails through.
	{
		FRandomStream Rng(613);
		int32 Seed = 0;
		auto Lay = [&](const FDCFogBankDesc& D, float X, float Y, float Z)
		{
			if (ADCFogBank* Bank = ADCFogBank::Spawn(World, D, ++Seed)) { Register(Bank, FVector(X, Y, Z), 1.5f, false, 1.f, 0.f, false); }
		};
		// Inside the storm front (the first 300 m after the fog begins): a tunnel of cloud. Rings of heavy
		// billows to either hand, on the shoulders and overhead, with the middle left open, so the ship
		// runs down a rolling throat of cloud with the light at the far end, not through an even grey.
		// Off by default (-DCTunnel): tried 13 Nov, it reads as patches of fog, not a throat of cloud. The fog grid cannot hold the shape.
		if (FParse::Param(FCommandLine::Get(), TEXT("DCTunnel")))
		{
			for (float X = 1500.f; X < 31000.f; X += 4200.f)
			{
				const float Turn = Rng.FRandRange(-0.25f, 0.25f);
				for (int32 k = 0; k < 5; ++k)
				{
					// Round the throat from the port waterline, over the mastheads, to the starboard waterline.
					const float A = UE_PI * (k / 4.f) + Turn;
					const float Across = FMath::Cos(A), Up = FMath::Sin(A);
					FDCFogBankDesc D;
					D.Radii = FVector(Rng.FRandRange(3800.f, 5200.f), Rng.FRandRange(3200.f, 4400.f), Rng.FRandRange(2600.f, 3600.f));
					D.Density = Rng.FRandRange(0.05f, 0.08f);
					D.Size = Rng.FRandRange(3500.f, 5500.f);
					D.Erode = Rng.FRandRange(0.45f, 0.65f);
					D.Drift = FVector(Rng.FRandRange(-500.f, -250.f), -Across * 120.f, Rng.FRandRange(-40.f, 60.f));
					D.bHero = true;   // sharp, self-shadowed cloud: the fog grid could not hold this shape
					Lay(D, X + Rng.FRandRange(-1200.f, 1200.f), -Across * Rng.FRandRange(6200.f, 7600.f), SeaZ + 800.f + FMath::Max(0.f, Up) * Rng.FRandRange(5200.f, 6400.f));
				}
			}
		}
		// Walls: a pair every 130 m or so, staggered, standing off the course by a ship's length or two.
		for (float X = 30000.f; X < 340000.f; X += Rng.FRandRange(11000.f, 15000.f))
		{
			for (float Side : { -1.f, 1.f })
			{
				if (Rng.FRand() < 0.18f) { continue; }   // a gap: a look out to the side
				FDCFogBankDesc D;
				D.Radii = FVector(Rng.FRandRange(10000.f, 16000.f), Rng.FRandRange(8000.f, 13000.f), Rng.FRandRange(2400.f, 4600.f));
				D.Density = Rng.FRandRange(0.04f, 0.065f);
				D.Size = Rng.FRandRange(5000.f, 8000.f);
				D.Erode = Rng.FRandRange(0.4f, 0.65f);
				D.Drift = FVector(Rng.FRandRange(-350.f, -150.f), Side * Rng.FRandRange(-60.f, 160.f), 0.f);
				// The walls along the course are drawn sharp and self-shadowed (heterogeneous volumes).
				static const bool bHeroBanks = FParse::Param(FCommandLine::Get(), TEXT("DCHeroBanks"));
				D.bHero = bHeroBanks;
				Lay(D, X + Rng.FRandRange(-3000.f, 3000.f), Side * (D.Radii.Y + Rng.FRandRange(3500.f, 7000.f)), SeaZ + D.Radii.Z * 0.35f);
			}
		}
		// Behind the walls, further out on either hand: wide low fields of thinner fog, so that the city
		// beyond the channel stands in it to the knees wherever the eye goes.
		for (float X = 20000.f; X < 350000.f; X += Rng.FRandRange(22000.f, 30000.f))
		{
			for (float Side : { -1.f, 1.f })
			{
				FDCFogBankDesc D;
				D.Radii = FVector(Rng.FRandRange(18000.f, 26000.f), Rng.FRandRange(14000.f, 22000.f), Rng.FRandRange(2000.f, 3400.f));
				D.Density = Rng.FRandRange(0.014f, 0.024f);
				D.Size = Rng.FRandRange(8000.f, 12000.f);
				D.Erode = Rng.FRandRange(0.45f, 0.6f);
				D.Drift = FVector(Rng.FRandRange(-250.f, -100.f), Side * Rng.FRandRange(-40.f, 120.f), 0.f);
				Lay(D, X + Rng.FRandRange(-6000.f, 6000.f), Side * Rng.FRandRange(30000.f, 46000.f), SeaZ + D.Radii.Z * 0.3f);
			}
		}
		// Curtains before the great figures and the temples.
		struct FHidden { float X, Y, Height; };
		const FHidden Hidden[] = {
			{ 70000.f, -12000.f, 16000.f }, { 76000.f, 13000.f, 17000.f }, { 150000.f, 17000.f, 22000.f }, { 160000.f, -16000.f, 19000.f },
			{ 240000.f, -14000.f, 21000.f }, { 252000.f, 15000.f, 24000.f }, { 112000.f, 30000.f, 14000.f }, { 205000.f, -40000.f, 18000.f },
			{ 92000.f, -30000.f, 12000.f }, { 140000.f, 34000.f, 14000.f }, { 200000.f, -40000.f, 16000.f }, { 262000.f, 46000.f, 18000.f } };
		for (const FHidden& H : Hidden)
		{
			FDCFogBankDesc D;
			D.Radii = FVector(9000.f, 8000.f, H.Height * 0.65f);
			D.Density = 0.06f;
			D.Size = 7000.f;
			D.Erode = 0.5f;
			D.Drift = FVector(-200.f, 0.f, 60.f);
			D.RevealWithin = FMath::Abs(H.Y) * 0.62f + 15000.f;
			D.bHero = FParse::Param(FCommandLine::Get(), TEXT("DCHeroBanks"));   // a tower of sharp cloud that thins away as the ship comes up
			// Between the ship's course and the figure, and a little before it.
			Lay(D, H.X - 7000.f, H.Y * 0.62f, SeaZ + H.Height * 0.45f);
		}
		// Veils across the course.
		for (float X = 52000.f; X < 330000.f; X += Rng.FRandRange(30000.f, 45000.f))
		{
			FDCFogBankDesc D;
			D.Radii = FVector(Rng.FRandRange(4000.f, 7000.f), Rng.FRandRange(9000.f, 14000.f), Rng.FRandRange(2000.f, 3200.f));
			D.Density = Rng.FRandRange(0.01f, 0.018f);
			D.Size = 4500.f;
			D.Erode = 0.75f;
			Lay(D, X, Rng.FRandRange(-2500.f, 2500.f), SeaZ + D.Radii.Z * 0.5f);
		}
		// Wisps across the deck: a thin fast-running layer round the ship herself, drawn out along the wind.
		FDCFogBankDesc W;
		W.bOnDeck = true;
		W.Radii = FVector(6000.f, 3000.f, 900.f);
		W.Density = 0.07f;
		W.Size = 1400.f;
		W.Erode = 0.3f;
		W.Wisp = 0.5f;
		W.Drift = FVector(-900.f, 260.f, 0.f);
		W.Stretch = FVector(3.f, 1.f, 0.6f);
		if (ADCFogBank* Wisps = ADCFogBank::Spawn(World, W, 77)) { Wisps->SetActorLocation(FVector(500.f, 0.f, SeaZ + 800.f)); }
		// Sea smoke: one wide, thin, low layer that goes with the ship, so the water out to the ruins lies under
		// pale mist everywhere and not only where a bank happens to stand. (The reference's fog is everywhere.)
		float Smoke = 0.016f;   // -DCSeaSmoke=<density>, 0 = off
		FParse::Value(FCommandLine::Get(), TEXT("DCSeaSmoke="), Smoke);
		if (Smoke > 0.f)
		{
			FDCFogBankDesc S;
			S.bOnDeck = true;
			S.Radii = FVector(60000.f, 50000.f, 520.f);
			S.Density = Smoke;
			S.Size = 9000.f;
			S.Erode = 0.55f;
			S.Drift = FVector(-500.f, 120.f, 0.f);
			if (ADCFogBank* Layer = ADCFogBank::Spawn(World, S, 78)) { Layer->SetActorLocation(FVector(8000.f, 0.f, SeaZ + 180.f)); }
		}

		// Easy Fog cards (bought, not in the repository; absent, nothing is laid). Lit, flowing sheets of mist:
		// wide ones lying on the sea so the water and the feet of the ruins stand in layered fog, and slanted
		// rags of scud hanging over the channel's shoulders. They fade out near the eye and where they meet stone.
		UStaticMesh* CardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/EasyFog/Meshes/SM_easyFogCard_02.SM_easyFogCard_02"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		UMaterialInterface* CardBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EasyFog/Materials/M_BP_EasyFog.M_BP_EasyFog"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (CardMesh && CardBase && !FParse::Param(FCommandLine::Get(), TEXT("DCNoFogCards")))
		{
			float CardDensity = 1.f; FParse::Value(FCommandLine::Get(), TEXT("DCFogCards="), CardDensity);
			TArray<UTexture*> Masks, Normals;
			for (int32 i = 1; i <= 8; ++i)
			{
				Masks.Add(LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/EasyFog/Textures/Fog/T_mountainFog_0%d_mask.T_mountainFog_0%d_mask"), i, i), nullptr, LOAD_NoWarn | LOAD_Quiet));
				Normals.Add(LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/EasyFog/Textures/Fog/T_mountainFog_0%d_N.T_mountainFog_0%d_N"), i, i), nullptr, LOAD_NoWarn | LOAD_Quiet));
			}
			FActorSpawnParameters CardParams;
			CardParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			int32 Cards = 0;
			auto LayCard = [&](const FVector& At, float Across, float Pitch, float Yaw, float Density)
			{
				AActor* Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, CardParams);
				if (!Holder) { return; }
				USceneComponent* Root = NewObject<USceneComponent>(Holder);
				Root->SetMobility(EComponentMobility::Movable);
				Holder->SetRootComponent(Root);
				Root->RegisterComponent();
				UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Holder);
				C->SetMobility(EComponentMobility::Movable);
				C->SetStaticMesh(CardMesh);
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				C->SetCastShadow(false);
				C->SetupAttachment(Root);
				C->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
				C->SetRelativeScale3D(FVector(Across / 1160.f, Across / 1160.f * Rng.FRandRange(0.6f, 1.f), Across / 1160.f));
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(CardBase, Holder);
				const int32 Shape = Rng.RandRange(0, 7);
				if (Masks[Shape]) { MID->SetTextureParameterValue(TEXT("Opacity Map"), Masks[Shape]); }
				if (Normals[Shape]) { MID->SetTextureParameterValue(TEXT("FogNormals"), Normals[Shape]); }
				MID->SetScalarParameterValue(TEXT("Fog Density"), Density * CardDensity);
				MID->SetScalarParameterValue(TEXT("Fading Distance"), FMath::Min(Across * 0.25f, 3000.f));
				MID->SetScalarParameterValue(TEXT("Camera Fading Distance"), 2500.f);
				MID->SetScalarParameterValue(TEXT("Normal Intensity"), 1.f);
				MID->SetScalarParameterValue(TEXT("Emissive Intensity"), 0.f);   // lit by the scene only
				MID->SetVectorParameterValue(TEXT("Base Color Tint"), FLinearColor(0.62f, 0.68f, 0.78f));
				C->SetMaterial(0, MID);
				C->RegisterComponent();
				Register(Holder, At, 1.5f, false, 1.f, Yaw, false);
				++Cards;
			};
			for (float X = 26000.f; X < 345000.f; X += Rng.FRandRange(5000.f, 9000.f))
			{
				// A sheet on the water, anywhere from the course to well out among the ruins.
				LayCard(FVector(X, Rng.FRandRange(-14000.f, 14000.f), SeaZ + Rng.FRandRange(180.f, 420.f)), Rng.FRandRange(14000.f, 38000.f), 0.f, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.5f, 0.9f));
				// Every other step, a rag of scud leaning over one shoulder of the channel.
				if (Rng.FRand() < 0.5f)
				{
					const float Side = Rng.FRand() < 0.5f ? -1.f : 1.f;
					LayCard(FVector(X + 2500.f, Side * Rng.FRandRange(7000.f, 18000.f), SeaZ + Rng.FRandRange(2500.f, 7000.f)), Rng.FRandRange(9000.f, 22000.f), Rng.FRandRange(50.f, 78.f), Rng.FRandRange(-25.f, 25.f), Rng.FRandRange(0.35f, 0.7f));
				}
			}
			UE_LOG(LogDC, Log, TEXT("Fog cards: %d laid"), Cards);
			// And across the deck: six upright rags that come in over the bow and run aft past the masts, over and over.
			if (!FParse::Param(FCommandLine::Get(), TEXT("DCNoDeckWisps")))
			{
				for (int32 i = 0; i < 6; ++i)
				{
					AActor* Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, CardParams);
					if (!Holder) { continue; }
					UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Holder);
					C->SetMobility(EComponentMobility::Movable);
					Holder->SetRootComponent(C);
					C->SetStaticMesh(CardMesh);
					C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					C->SetCastShadow(false);
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(CardBase, Holder);
					const int32 Shape = Rng.RandRange(0, 7);
					if (Masks[Shape]) { MID->SetTextureParameterValue(TEXT("Opacity Map"), Masks[Shape]); }
					if (Normals[Shape]) { MID->SetTextureParameterValue(TEXT("FogNormals"), Normals[Shape]); }
					MID->SetScalarParameterValue(TEXT("Fog Density"), 0.f);
					MID->SetScalarParameterValue(TEXT("Fading Distance"), 250.f);
					MID->SetScalarParameterValue(TEXT("Camera Fading Distance"), 500.f);
					MID->SetScalarParameterValue(TEXT("Normal Intensity"), 1.f);
					MID->SetScalarParameterValue(TEXT("Emissive Intensity"), 0.f);
					MID->SetVectorParameterValue(TEXT("Base Color Tint"), FLinearColor(0.7f, 0.76f, 0.86f));
					C->SetMaterial(0, MID);
					C->RegisterComponent();
					const float Across = Rng.FRandRange(900.f, 2200.f);
					Holder->SetActorScale3D(FVector(Across / 1160.f * 2.2f, Across / 1160.f, Across / 1160.f));
					FDeckWisp Wisp;
					Wisp.Actor = Holder; Wisp.MID = MID;
					Wisp.Phase = Rng.FRand(); Wisp.Speed = Rng.FRandRange(700.f, 1300.f); Wisp.Side = Rng.FRandRange(-420.f, 420.f);
					Wisp.Height = Rng.FRandRange(620.f, 1500.f); Wisp.Density = Rng.FRandRange(0.18f, 0.35f) * CardDensity;
					DeckWisps.Add(Wisp);
					DeckWispMIDs.Add(MID);
				}
			}
		}
	}

	// Trial (-DCVdbTunnel=N): one of the bought looping cloud tunnels (Clouds Tunnel Vdb Effects Pack Loop,
	// 1 to 10), standing still by the ship. -DCVdbScale= enlarges it (the pack's own size is about 50 m long),
	// -DCVdbAt=x,y,z places its foot, -DCVdbYaw= turns it, -DCVdbRate= is frames a second, -DCVdbDensity= thickens it.
	int32 VdbIndex = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCVdbTunnel="), VdbIndex) && VdbIndex > 0)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Clouds_Tunnel_Loop/Material/MI_SparseVolumeMaterial_Inst_%d.MI_SparseVolumeMaterial_Inst_%d"), VdbIndex, VdbIndex));
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHeterogeneousVolume* Holder = World->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(), FTransform::Identity, Params);
		if (Base && Holder)
		{
			float Scale = 20.f, Yaw = -90.f, Rate = 24.f, VDensity = 1.f; FVector At(0.f, 0.f, SeaZ);
			FParse::Value(FCommandLine::Get(), TEXT("DCVdbScale="), Scale);
			FParse::Value(FCommandLine::Get(), TEXT("DCVdbYaw="), Yaw);
			FParse::Value(FCommandLine::Get(), TEXT("DCVdbRate="), Rate);
			FParse::Value(FCommandLine::Get(), TEXT("DCVdbDensity="), VDensity);
			FString AtText;
			if (FParse::Value(FCommandLine::Get(), TEXT("DCVdbAt="), AtText, false))
			{
				TArray<FString> Parts; AtText.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() == 3) { At = FVector(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), SeaZ + FCString::Atof(*Parts[2])); }
			}
			UHeterogeneousVolumeComponent* H = Holder->FindComponentByClass<UHeterogeneousVolumeComponent>();
			H->SetMobility(EComponentMobility::Movable);
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Holder);
			MID->SetScalarParameterValue(TEXT("Density Scale"), VDensity);
			// As bought it is brown cloud lit teal from within. Storm cloud: grey-blue, with a faint cold light in its folds.
			if (!FParse::Param(FCommandLine::Get(), TEXT("DCVdbRaw")))
			{
				float Temp = 0.12f, Albedo = 0.9f; FParse::Value(FCommandLine::Get(), TEXT("DCVdbTemp="), Temp); FParse::Value(FCommandLine::Get(), TEXT("DCVdbAlbedo="), Albedo);
				MID->SetVectorParameterValue(TEXT("Albedo Color"), FLinearColor(0.78f, 0.83f, 0.92f));
				MID->SetScalarParameterValue(TEXT("Albedo Scale"), Albedo);
				MID->SetVectorParameterValue(TEXT("Temperature Color"), FLinearColor(0.45f, 0.6f, 1.f));
				MID->SetVectorParameterValue(TEXT("Blackbody Tint"), FLinearColor(0.45f, 0.6f, 1.f));
				MID->SetScalarParameterValue(TEXT("Temperature Scale"), Temp);
			}
			H->SetMaterial(0, MID);
			H->StartFrame = 0.f; H->EndFrame = 200.f; H->FrameRate = Rate; H->bPlaying = true; H->bLooping = true;
			H->bIssueBlockingRequests = true;
			Holder->SetActorTransform(FTransform(FRotator(0.f, Yaw, 0.f), At, FVector(Scale)));
			UE_LOG(LogDC, Log, TEXT("Vdb tunnel %d: scale %.1f at %s yaw %.0f, material %s"), VdbIndex, Scale, *At.ToString(), Yaw, *GetNameSafe(Base));
			VdbTrial = H;
		}
		else { UE_LOG(LogDC, Warning, TEXT("Vdb tunnel %d: material not found"), VdbIndex); }
	}

	// Trial (-DCHeteroTest): one bank drawn by the engine's heterogeneous-volume renderer instead of the
	// volumetric fog grid. It is ray-marched at its own resolution with its own shadows, so it can be
	// sharp, and seen from any distance; the fog grid is coarse and ends 700 m out.
	if (FParse::Param(FCommandLine::Get(), TEXT("DCHeteroTest")))
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, FParse::Param(FCommandLine::Get(), TEXT("DCHeteroPlain")) ? TEXT("/Game/DC/Materials/M_HeteroTest.M_HeteroTest") : TEXT("/Game/DC/Materials/M_HeteroCloud.M_HeteroCloud"));
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (Base && Holder)
		{
			UHeterogeneousVolumeComponent* H = NewObject<UHeterogeneousVolumeComponent>(Holder);
			H->SetMobility(EComponentMobility::Movable);
			Holder->SetRootComponent(H);
			float Res = 128.f; FParse::Value(FCommandLine::Get(), TEXT("DCHeteroRes="), Res);
			H->VolumeResolution = FIntVector(Res, Res, Res);
			H->bPivotAtCentroid = true;
			const FVector Radii(12000.f, 9000.f, 6000.f);
			H->SetWorldScale3D(Radii * 2.f / Res);
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Holder);
			MID->SetVectorParameterValue(TEXT("Radii"), FLinearColor(Radii.X, Radii.Y, Radii.Z, 0.f));
			float HDensity = 0.05f, HErode = 0.7f; FParse::Value(FCommandLine::Get(), TEXT("DCHeteroDensity="), HDensity); FParse::Value(FCommandLine::Get(), TEXT("DCHeteroErode="), HErode);
			MID->SetScalarParameterValue(TEXT("Density"), HDensity);
			MID->SetScalarParameterValue(TEXT("Size"), 6000.f);
			MID->SetScalarParameterValue(TEXT("Erode"), HErode);
			MID->SetVectorParameterValue(TEXT("Floor"), FLinearColor(SeaZ - 150.f, 0.f, 0.f, 500.f));
			MID->SetVectorParameterValue(TEXT("Glow"), FLinearColor(0.2f, 0.25f, 0.32f));
			H->SetMaterial(0, MID);
			H->RegisterComponent();
			float At = 90000.f; FParse::Value(FCommandLine::Get(), TEXT("DCHeteroAt="), At);
			// -DCHeteroNear: a small one standing still beside the ship instead (20 m cube, 30 m off the starboard bow).
			if (FParse::Param(FCommandLine::Get(), TEXT("DCHeteroNear")))
			{
				H->SetWorldScale3D(FVector(2000.f / Res));
				Holder->SetActorLocation(FVector(3000.f, 3000.f, SeaZ + 1500.f));
				float NearSize = 2000.f; FParse::Value(FCommandLine::Get(), TEXT("DCHeteroSize="), NearSize);
				H->SetWorldScale3D(FVector(NearSize / Res));
				Holder->SetActorLocation(FVector(3000.f + NearSize, 3000.f + NearSize * 0.5f, SeaZ + NearSize * 0.45f));
				const float Unit = NearSize / Res;   // cm of world to one unit of the volume
				MID->SetVectorParameterValue(TEXT("Centre"), FLinearColor(0.f, 0.f, 0.f, 0.f));
				MID->SetVectorParameterValue(TEXT("Radii"), FLinearColor(Res * 0.5f, Res * 0.5f, Res * 0.5f, 0.f));
				MID->SetScalarParameterValue(TEXT("Size"), NearSize * 0.6f / Unit);
				MID->SetVectorParameterValue(TEXT("Drift"), FLinearColor(-200.f / Unit, 60.f / Unit, 0.f, 0.f));
				UE_LOG(LogDC, Log, TEXT("Hetero test: near volume, bounds %s"), *H->Bounds.ToString());
			}
			else
			Register(Holder, FVector(At, -14000.f, SeaZ + 3000.f), 1.5f, false, 1.f, 0.f, false);
			UE_LOG(LogDC, Log, TEXT("Hetero test: volume registered %.0f cm ahead; bounds %s; registered %d visible %d material %s"), At, *H->Bounds.ToString(), H->IsRegistered(), H->IsVisible(), *GetNameSafe(H->GetMaterial(0)));
		}
	}

	// The portal out of the city: on the ship's course, at the distance covered by the time the slide begins.
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ADCPortal* Portal = World->SpawnActor<ADCPortal>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
		{
			// Radius 0 marks it as something the ship is meant to sail into, so it is not pushed aside.
			Register(Portal, FVector(PortalDistance, 0.f, SeaZ), 0.f, false, 1.f, 0.f, false);
		}
	}

	// Faces in the clouds, high above the city, turned toward the ship and gazing down at it.
	struct FFace { float X, Y, Z, Height, Yaw; bool bBearded; };
	const FFace Faces[] = {
		{ 120000.f, -30000.f, 9500.f, 9000.f, 160.f, false }, { 190000.f, 34000.f, 11000.f, 10000.f, 200.f, true },
		{ 255000.f, -36000.f, 12000.f, 11000.f, 165.f, false }, { 320000.f, 30000.f, 14000.f, 13000.f, 190.f, true } };
	for (const FFace& F : Faces)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ADCCloudFace* Face = World->SpawnActor<ADCCloudFace>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
		{
			Face->Build(F.Height, F.bBearded, 14.f);
			Register(Face, FVector(F.X, F.Y, SeaZ + F.Z), 2000.f, false, 1.f, F.Yaw, false);
		}
	}

	// Presences in the cloud: figures a kilometre tall, seen only as pale shapes far off.
	if (!bSpecters) { return; }
	const FLinearColor Pale(0.22f, 0.3f, 0.38f);
	Register(DCHazards::SpawnSpecter(World, 95000.f, Pale, false), FVector(310000.f, -120000.f, SeaZ - 30000.f), 2000.f, false, 1.f, 150.f, false);
	Register(DCHazards::SpawnSpecter(World, 110000.f, Pale, false), FVector(430000.f, 95000.f, SeaZ - 35000.f), 2000.f, false, 1.f, 215.f, false);
	Register(DCHazards::SpawnSpecter(World, 80000.f, Pale, false), FVector(230000.f, 140000.f, SeaZ - 25000.f), 2000.f, false, 1.f, 240.f, false);
}
