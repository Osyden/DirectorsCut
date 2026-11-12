#include "DCTreadmill.h"
#include "DCRideDirector.h"
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

void ADCTreadmill::SpawnForPhase(EDCPhase Phase, float DeltaSeconds)
{
	float ObstacleEvery = 0.f, HostileEvery = 0.f;
	switch (Phase)
	{
	case EDCPhase::Fog: ObstacleEvery = 7.f; HostileEvery = 14.f; break;
	case EDCPhase::Hell: ObstacleEvery = 3.5f; HostileEvery = 9.f; break;
	case EDCPhase::Maelstrom: ObstacleEvery = 0.f; HostileEvery = 6.f; break;
	default: break;
	}

	if (ObstacleEvery > 0.f)
	{
		ObstacleTimer -= DeltaSeconds;
		if (ObstacleTimer <= 0.f)
		{
			ObstacleTimer = ObstacleEvery * FMath::FRandRange(0.7f, 1.3f);
			// Placed in sea space, so add the current lane offset to aim near the ship's present course.
			const float Y = LaneOffset + FMath::FRandRange(-2500.f, 2500.f);
			const bool bHell = Phase == EDCPhase::Hell;
			const float H = FMath::FRandRange(8.f, 30.f);
			const float W = FMath::FRandRange(4.f, 9.f);
			SpawnObject(false, FVector(SpawnDistance, Y, H * 50.f - 200.f), FVector(W, W, H), bHell ? ConeMesh.Get() : CubeMesh.Get(),
				bHell ? FLinearColor(0.35f, 0.05f, 0.01f) : FLinearColor(0.08f, 0.1f, 0.1f), W * 50.f, 1.f);
		}
	}

	if (HostileEvery > 0.f)
	{
		HostileTimer -= DeltaSeconds;
		if (HostileTimer <= 0.f)
		{
			HostileTimer = HostileEvery * FMath::FRandRange(0.7f, 1.3f);
			const float Side = FMath::RandBool() ? 1.f : -1.f;
			const float Y = LaneOffset + Side * FMath::FRandRange(5000.f, 9000.f);
			if (Phase == EDCPhase::Maelstrom)
			{
				// Stand-in tentacle: rises close alongside.
				SpawnObject(true, FVector(FMath::FRandRange(6000.f, 14000.f), LaneOffset + Side * FMath::FRandRange(2500.f, 4500.f), 1500.f),
					FVector(5.f, 5.f, 40.f), CylinderMesh.Get(), FLinearColor(0.1f, 0.2f, 0.12f), 400.f, 3.f);
			}
			else
			{
				// Stand-in ghost ship / demon.
				const bool bHell = Phase == EDCPhase::Hell;
				SpawnObject(true, FVector(SpawnDistance, Y, 400.f), FVector(30.f, 8.f, 10.f), CubeMesh.Get(),
					bHell ? FLinearColor(0.9f, 0.25f, 0.02f) : FLinearColor(0.3f, 0.9f, 0.8f), 1500.f, 2.f);
			}
		}
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

	LaneOffset = FMath::Clamp(LaneOffset + Steer * LaneSpeed * DeltaSeconds, -LaneLimit, LaneLimit);

	SpawnForPhase(Director->GetPhase(), DeltaSeconds * Director->TimeScale);

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

		// Hostiles pace the ship once alongside; obstacles sweep past.
		if (Obj.bHostile)
		{
			const float TargetX = 0.f;
			Obj.Base.X = FMath::FInterpConstantTo(Obj.Base.X, TargetX, DeltaSeconds, Obj.Base.X > 4000.f ? Speed : Speed * 0.08f);
			if (Obj.Base.X <= 1.f)
			{
				Obj.Base.X -= Speed * 0.3f * DeltaSeconds;
			}
		}
		else
		{
			Obj.Base.X -= Speed * DeltaSeconds;
		}

		const FVector Local(Obj.Base.X, Obj.Base.Y - LaneOffset, Obj.Base.Z);
		Actor->SetActorLocationAndRotation(Rock.TransformPosition(Local), Rock.GetRotation());

		if (!Obj.bHostile && FMath::Abs(Local.X) < ShipHalfLength && FMath::Abs(Local.Y) < ShipHalfWidth + Obj.Radius)
		{
			Director->DamageShip(10.f);
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
		if (Obj.bHostile) { continue; }
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
		const float Y = Obj.Base.Y - LaneOffset;
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
