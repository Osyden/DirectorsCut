#include "DCGunnery.h"
#include "DCTreadmill.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ADCGunnery::ADCGunnery()
{
	PrimaryActorTick.bCanEverTick = true;

	Balls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Balls"));
	RootComponent = Balls;
	Balls->SetMobility(EComponentMobility::Movable);
	Balls->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) { Balls->SetStaticMesh(Sphere.Object); }
}

ADCGunnery* ADCGunnery::Get(const UObject* WorldContext)
{
	if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCGunnery> It(World); It; ++It) { return *It; }
	}
	return nullptr;
}

void ADCGunnery::Init(AActor* Ship, float InSeaLevelZ)
{
	SeaLevelZ = InSeaLevelZ;
	Muzzles[0].Reset();
	Muzzles[1].Reset();
	if (Ship)
	{
		TArray<UPrimitiveComponent*> Prims;
		Ship->GetComponents(Prims);
		for (UPrimitiveComponent* P : Prims)
		{
			// Broadside guns only: named "...Cannon..." and sitting well off the centreline.
			if (!P->GetName().Contains(TEXT("Cannon"))) { continue; }
			const FVector Loc = P->Bounds.Origin;
			if (FMath::Abs(Loc.Y) < 200.f) { continue; }
			const int32 SideIdx = Loc.Y < 0.f ? 0 : 1;
			Muzzles[SideIdx].Add(Loc + FVector(0.f, Loc.Y < 0.f ? -140.f : 140.f, 20.f));
		}
	}
	UE_LOG(LogDC, Log, TEXT("Gunnery: %d port guns, %d starboard guns"), Muzzles[0].Num(), Muzzles[1].Num());

	// Effects that ship with the vessel asset; optional.
	MuzzleEffectClass = LoadClass<AActor>(nullptr, TEXT("/Game/Brigantine/BP/Cannon/BP_EffectExplosionStartCannon.BP_EffectExplosionStartCannon_C"));
	ImpactEffectClass = LoadClass<AActor>(nullptr, TEXT("/Game/Brigantine/BP/Cannon/BP_EffectExplosionEnd.BP_EffectExplosionEnd_C"));
}

bool ADCGunnery::FireBroadside(EDCSide Side, const FVector& Target, int32 MaxGuns)
{
	const int32 SideIdx = Side == EDCSide::Port ? 0 : 1;
	if (ReloadLeft[SideIdx] > 0.f || Muzzles[SideIdx].Num() == 0) { return false; }
	ReloadLeft[SideIdx] = ReloadTime;

	// Use the guns nearest the target along the hull, and spread their fall of shot a little.
	TArray<FVector> Guns = Muzzles[SideIdx];
	Guns.Sort([&Target](const FVector& A, const FVector& B) { return FMath::Abs(A.X - Target.X) < FMath::Abs(B.X - Target.X); });
	const int32 N = FMath::Min(MaxGuns, Guns.Num());
	for (int32 i = 0; i < N; ++i)
	{
		FPendingShot S;
		S.Muzzle = Guns[i];
		S.Target = Target + FVector(FMath::FRandRange(-250.f, 250.f), FMath::FRandRange(-250.f, 250.f), 0.f);
		S.Delay = i * FMath::FRandRange(0.08f, 0.18f);
		Pending.Add(S);
	}
	return true;
}

void ADCGunnery::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (float& R : ReloadLeft) { R = FMath::Max(0.f, R - DeltaSeconds); }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = Pending.Num() - 1; i >= 0; --i)
	{
		FPendingShot& P = Pending[i];
		P.Delay -= DeltaSeconds;
		if (P.Delay > 0.f) { continue; }

		// Solve for the launch velocity that lands on the target after FlightTime under our gravity.
		const FVector Delta = P.Target - P.Muzzle;
		const float FlightTime = FMath::Clamp(Delta.Size() / ShotSpeed, 0.5f, 3.f);
		FShot S;
		S.Pos = P.Muzzle;
		S.Vel = Delta / FlightTime + FVector(0.f, 0.f, 0.5f * Gravity * FlightTime);
		S.Life = FlightTime + 1.f;
		Shots.Add(S);

		if (MuzzleEffectClass)
		{
			if (AActor* Fx = GetWorld()->SpawnActor<AActor>(MuzzleEffectClass, P.Muzzle, FRotator(0.f, P.Muzzle.Y < 0.f ? -90.f : 90.f, 0.f), Params))
			{
				Fx->SetLifeSpan(4.f);
			}
		}
		Pending.RemoveAtSwap(i);
	}

	ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	for (int32 i = Shots.Num() - 1; i >= 0; --i)
	{
		FShot& S = Shots[i];
		S.Vel.Z -= Gravity * DeltaSeconds;
		S.Pos += S.Vel * DeltaSeconds;
		S.Life -= DeltaSeconds;

		const bool bHitTarget = Treadmill && Treadmill->DamageNear(S.Pos, 250.f, 1.f, true) > 0;
		const bool bSplash = S.Pos.Z <= SeaLevelZ && S.Vel.Z < 0.f;
		if (bHitTarget || bSplash || S.Life <= 0.f)
		{
			if (bSplash && Treadmill) { Treadmill->DamageNear(S.Pos, HitRadius, 1.f, false); }
			if (ImpactEffectClass && (bHitTarget || bSplash))
			{
				if (AActor* Fx = GetWorld()->SpawnActor<AActor>(ImpactEffectClass, S.Pos, FRotator::ZeroRotator, Params))
				{
					Fx->SetLifeSpan(4.f);
				}
			}
			Shots.RemoveAtSwap(i);
		}
	}

	// Redraw the cannonballs.
	Balls->ClearInstances();
	for (const FShot& S : Shots)
	{
		Balls->AddInstance(FTransform(FQuat::Identity, S.Pos, FVector(0.35f)), true);
	}
}
