#include "DCGunnery.h"
#include "DCTreadmill.h"
#include "DCMaelstrom.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Find the first air-to-water crossing, including a slope rising into the shot. */
	bool IntersectMaelstromSurface(const ADCMaelstrom* Maelstrom, const FVector& Start, const FVector& End, FVector& OutPoint)
	{
		float Above = 0.f;
		if (Start.Z <= Maelstrom->SampleSurfaceWorld(Start)) { OutPoint = Start; return true; }
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector::Distance(Start, End) / 250.f));
		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			float Below = static_cast<float>(Step) / Steps;
			const FVector Sample = FMath::Lerp(Start, End, Below);
			if (Sample.Z > Maelstrom->SampleSurfaceWorld(Sample)) { Above = Below; continue; }
			for (int32 Refine = 0; Refine < 9; ++Refine)
			{
				const float Middle = (Above + Below) * 0.5f;
				const FVector Point = FMath::Lerp(Start, End, Middle);
				if (Point.Z > Maelstrom->SampleSurfaceWorld(Point)) { Above = Middle; }
				else { Below = Middle; }
			}
			OutPoint = FMath::Lerp(Start, End, Below);
			return true;
		}
		return false;
	}
}

ADCGunnery::ADCGunnery()
{
	PrimaryActorTick.bCanEverTick = true;

	Balls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Balls"));
	RootComponent = Balls;
	Balls->SetMobility(EComponentMobility::Movable);
	Balls->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) { Balls->SetStaticMesh(Sphere.Object); }
	Trails = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ShotStreaks"));
	Trails->SetupAttachment(Balls);
	Trails->SetMobility(EComponentMobility::Movable);
	Trails->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trails->SetCastShadow(false);
	if (Sphere.Succeeded()) { Trails->SetStaticMesh(Sphere.Object); }
	Spray = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ImpactSpray"));
	Spray->SetupAttachment(Balls);
	Spray->SetMobility(EComponentMobility::Movable);
	Spray->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Spray->SetCastShadow(false);
	if (Sphere.Succeeded()) { Spray->SetStaticMesh(Sphere.Object); }
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
	GunShip = Ship;
	if (UMaterialInterface* Base = Trails->GetMaterial(0))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Trails);
		// A thin grey streak of smoke and blur behind the ball. (It was a thick orange rod, which read as a wooden bolt.)
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.16f, 0.17f));
		Trails->SetMaterial(0, Material);
	}
	if (UMaterialInterface* BallBase = Balls->GetMaterial(0))
	{
		UMaterialInstanceDynamic* Iron = UMaterialInstanceDynamic::Create(BallBase, Balls);
		Iron->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.02f, 0.022f));
		Balls->SetMaterial(0, Iron);
	}
	Muzzles[0].Reset();
	Muzzles[1].Reset();
	if (Ship)
	{
		TArray<UPrimitiveComponent*> Prims;
		Ship->GetComponents(Prims);
		for (UPrimitiveComponent* P : Prims)
		{
			// Broadside guns only: named "...Cannon..." and sitting well off the centreline.
			if (!Cast<USkeletalMeshComponent>(P) || !P->IsVisible() || P->bHiddenInGame
				|| !P->GetName().Contains(TEXT("Cannon"))) { continue; }
			const FVector Loc = P->Bounds.Origin;
			if (FMath::Abs(Loc.Y) < 200.f || P->Bounds.BoxExtent.Y <= P->Bounds.BoxExtent.X) { continue; }
			const int32 SideIdx = Loc.Y < 0.f ? 0 : 1;
			FVector Muzzle = Loc + FVector(0.f, FMath::Sign(Loc.Y) * P->Bounds.BoxExtent.Y, 20.f);
			// The guns below decks sit a metre inside the hull by that reckoning, so their flash and shot started
			// inside the ship. Find the ship's side abreast of the gun and start just outside it.
			FHitResult Side;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(DCMuzzle), true);
			const FVector Out(Muzzle.X, FMath::Sign(Loc.Y) * 1500.f, Muzzle.Z);
			if (Ship->ActorLineTraceSingle(Side, Out, FVector(Muzzle.X, 0.f, Muzzle.Z), ECC_Visibility, Q) && FMath::Abs(Side.ImpactPoint.Y) > FMath::Abs(Muzzle.Y))
			{
				Muzzle.Y = Side.ImpactPoint.Y + FMath::Sign(Loc.Y) * 25.f;
			}
			else if (Muzzle.Z < 470.f) { Muzzle.Y += FMath::Sign(Loc.Y) * 110.f; }   // no hit on the hull: the ports are about a metre deep
			Muzzles[SideIdx].Add(Muzzle);
		}
	}
	UE_LOG(LogDC, Log, TEXT("Gunnery: %d port guns, %d starboard guns"), Muzzles[0].Num(), Muzzles[1].Num());

	// Effects that ship with the vessel asset; optional.
	MuzzleEffectClass = LoadClass<AActor>(nullptr, TEXT("/Game/Brigantine/BP/Cannon/BP_EffectExplosionStartCannon.BP_EffectExplosionStartCannon_C"));
	ImpactEffectClass = LoadClass<AActor>(nullptr, TEXT("/Game/Brigantine/BP/Cannon/BP_EffectExplosionEnd.BP_EffectExplosionEnd_C"));
	UE_LOG(LogDC, Log, TEXT("Gunnery effects: muzzle=%s, impact=%s"),
		*GetNameSafe(MuzzleEffectClass.Get()), *GetNameSafe(ImpactEffectClass.Get()));
}

bool ADCGunnery::ResolveAim(EDCSide Side, const FVector& Origin, const FVector& Direction, FVector& OutTarget) const
{
	constexpr float MinRange = 650.f;
	constexpr float MaxRange = 18000.f;
	const float Sign = Side == EDCSide::Port ? -1.f : 1.f;
	float Distance = MaxRange;
	bool bSurface = false;
	const ADCMaelstrom* Maelstrom = ADCMaelstrom::Get(this);
	if (Maelstrom && Maelstrom->IsActive())
	{
		FVector WaterPoint;
		bSurface = IntersectMaelstromSurface(Maelstrom, Origin, Origin + Direction * MaxRange, WaterPoint);
		if (bSurface) { Distance = FVector::Distance(Origin, WaterPoint); }
	}
	else if (Direction.Z < -UE_SMALL_NUMBER && Origin.Z > SeaLevelZ)
	{
		const float WaterDistance = (SeaLevelZ - Origin.Z) / Direction.Z;
		bSurface = WaterDistance <= MaxRange;
		Distance = FMath::Min(Distance, WaterDistance);
	}

	FCollisionQueryParams Query(SCENE_QUERY_STAT(DCGunAim), false, this);
	if (GunShip.IsValid()) { Query.AddIgnoredActor(GunShip.Get()); }
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * Distance, ECC_Visibility, Query))
	{
		OutTarget = Hit.ImpactPoint;
		Distance = Hit.Distance;
		bSurface = true;
	}
	else
	{
		OutTarget = Origin + Direction * Distance;
	}
	return bSurface && Distance >= MinRange && Sign * OutTarget.Y >= 1000.f;
}

FString ADCGunnery::GetPlayerImpactLabel(EDCSide Side) const
{
	const int32 Index = Side == EDCSide::Port ? 0 : 1;
	if (GetWorld()->GetTimeSeconds() >= ImpactUntil[Index]) { return FString(); }
	return bImpactHit[Index] ? TEXT("HIT") : TEXT("SPLASH");
}

bool ADCGunnery::FireBroadside(EDCSide Side, const FVector& Target, int32 MaxGuns, bool bPlayerVolley)
{
	const int32 SideIdx = Side == EDCSide::Port ? 0 : 1;
	if (Side == EDCSide::None || MaxGuns <= 0 || ReloadLeft[SideIdx] > 0.f || Muzzles[SideIdx].Num() == 0) { return false; }
	ReloadLeft[SideIdx] = ReloadTime;
	UE_LOG(LogDC, Verbose, TEXT("Broadside %s (%s) at %s"), SideIdx == 0 ? TEXT("port") : TEXT("starboard"),
		bPlayerVolley ? TEXT("player") : TEXT("crew"), *Target.ToCompactString());

	// Use the guns nearest the target along the hull, and spread their fall of shot a little.
	TArray<FVector> Guns = Muzzles[SideIdx];
	Guns.Sort([&Target, bPlayerVolley](const FVector& A, const FVector& B)
	{
		// Let the captain see the deck battery fire before the lower-deck guns join in.
		if (bPlayerVolley && (A.Z > 470.f) != (B.Z > 470.f)) { return A.Z > B.Z; }
		return FMath::Abs(A.X - Target.X) < FMath::Abs(B.X - Target.X);
	});
	const int32 N = FMath::Min(MaxGuns, Guns.Num());
	for (int32 i = 0; i < N; ++i)
	{
		FPendingShot S;
		S.Muzzle = Guns[i];
		const float Spread = bPlayerVolley ? 120.f : 250.f;
		S.Target = i == 0 ? Target : Target + FVector(FMath::FRandRange(-Spread, Spread), FMath::FRandRange(-Spread, Spread), 0.f);
		S.Delay = i * 0.12f;
		S.PlayerSide = bPlayerVolley ? Side : EDCSide::None;
		Pending.Add(S);
	}
	return true;
}

void ADCGunnery::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (float& R : ReloadLeft) { R = FMath::Max(0.f, R - DeltaSeconds); }
	// Test hook: -DCGunTestAt=<seconds> fires a captain's starboard volley abeam at that time after start.
	{
		static float TestAt = -1.f; static bool bRead = false;
		if (!bRead) { bRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCGunTestAt="), TestAt); }
		if (TestAt >= 0.f && GetWorld()->GetTimeSeconds() >= TestAt)
		{
			TestAt = -1.f;
			for (const FVector& M : Muzzles[1]) { UE_LOG(LogDC, Log, TEXT("Gun test: starboard muzzle at %s"), *M.ToCompactString()); }
			FireBroadside(EDCSide::Starboard, FVector(0.f, 15000.f, SeaLevelZ), 6, true);
		}
	}


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
		S.Life = FlightTime + 0.8f;
		S.PlayerSide = P.PlayerSide;
		Shots.Add(S);

		if (APointLight* Flash = GetWorld()->SpawnActor<APointLight>(P.Muzzle, FRotator::ZeroRotator, Params))
		{
			UPointLightComponent* Light = Cast<UPointLightComponent>(Flash->GetLightComponent());
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetCastShadows(false);
			Light->SetLightColor(FLinearColor(1.f, 0.54f, 0.19f));
			Light->SetIntensity(45000.f);
			Light->SetAttenuationRadius(700.f);
			Flash->SetLifeSpan(0.075f);
		}
		if (MuzzleEffectClass)
		{
			if (AActor* Fx = GetWorld()->SpawnActor<AActor>(MuzzleEffectClass, P.Muzzle, S.Vel.Rotation(), Params))
			{
				Fx->SetLifeSpan(4.f);
			}
		}
		Pending.RemoveAtSwap(i);
	}

	ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	const ADCMaelstrom* Maelstrom = ADCMaelstrom::Get(this);
	if (Maelstrom && !Maelstrom->IsActive()) { Maelstrom = nullptr; }
	for (int32 i = Shots.Num() - 1; i >= 0; --i)
	{
		FShot& S = Shots[i];
		bool bHitTarget = false;
		bool bSplash = false;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(DCGunShot), false, this);
		if (GunShip.IsValid()) { Query.AddIgnoredActor(GunShip.Get()); }
		// Limit step length so fast shots cannot skip small targets at low frame rates.
		float Remaining = FMath::Min(DeltaSeconds, S.Life);
		while (Remaining > UE_SMALL_NUMBER && !bHitTarget && !bSplash)
		{
			const float Step = FMath::Min(Remaining, 0.025f);
			const FVector Previous = S.Pos;
			S.Pos += S.Vel * Step - FVector(0.f, 0.f, 0.5f * Gravity * Step * Step);
			S.Vel.Z -= Gravity * Step;
			Remaining -= Step;

			if (Maelstrom)
			{
				FVector WaterPoint;
				bSplash = IntersectMaelstromSurface(Maelstrom, Previous, S.Pos, WaterPoint);
				if (bSplash) { S.Pos = WaterPoint; }
			}
			else
			{
				bSplash = S.Pos.Z <= SeaLevelZ && S.Vel.Z < 0.f;
				if (bSplash && Previous.Z > SeaLevelZ)
				{
					S.Pos = FMath::Lerp(Previous, S.Pos, (Previous.Z - SeaLevelZ) / (Previous.Z - S.Pos.Z));
				}
			}
			FHitResult Hit;
			const bool bGeometry = GetWorld()->LineTraceSingleByChannel(Hit, Previous, S.Pos, ECC_Visibility, Query);
			if (bGeometry) { S.Pos = Hit.ImpactPoint; }
			bHitTarget = (Treadmill && Treadmill->DamageNear(S.Pos, 250.f, 1.f, true) > 0) || bGeometry;
			const float SurfaceZ = Maelstrom ? Maelstrom->SampleSurfaceWorld(S.Pos) : SeaLevelZ;
			if (bGeometry && S.Pos.Z > SurfaceZ + 10.f) { bSplash = false; }
		}
		S.Life -= DeltaSeconds;
		if (bHitTarget || bSplash || S.Life <= 0.f)
		{
			if (bSplash && !bHitTarget && Treadmill)
			{
				bHitTarget = Treadmill->DamageNear(S.Pos, HitRadius, 1.f, false) > 0;
			}
			if (S.PlayerSide != EDCSide::None && (bHitTarget || bSplash))
			{
				const int32 Index = S.PlayerSide == EDCSide::Port ? 0 : 1;
				const float Now = GetWorld()->GetTimeSeconds();
				bImpactHit[Index] = bHitTarget || (bImpactHit[Index] && Now < ImpactUntil[Index]);
				ImpactUntil[Index] = Now + 1.15f;
			}
			if (bSplash) { Splashes.Add({S.Pos, 0.f}); }
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
	Trails->ClearInstances();
	for (const FShot& S : Shots)
	{
		const FVector Direction = S.Vel.GetSafeNormal();
		Trails->AddInstance(FTransform(Direction.Rotation(), S.Pos - Direction * 70.f, FVector(1.3f, 0.05f, 0.05f)), true);
		Balls->AddInstance(FTransform(FQuat::Identity, S.Pos, FVector(0.3f)), true);
	}

	// A brief fan of water above the smoke, large enough to read from the deck.
	Spray->ClearInstances();
	for (int32 Index = Splashes.Num() - 1; Index >= 0; --Index)
	{
		FSplash& Splash = Splashes[Index];
		Splash.Age += DeltaSeconds;
		const float T = Splash.Age;
		const float Fade = FMath::Clamp(1.f - T / 0.8f, 0.f, 1.f);
		if (Fade <= 0.f) { Splashes.RemoveAtSwap(Index); continue; }

		for (int32 Jet = 0; Jet < 7; ++Jet)
		{
			const float Angle = Jet * UE_TWO_PI / 7.f;
			const float Up = Jet == 0 ? 900.f : 650.f + 32.f * Jet;
			const FVector Initial(Jet == 0 ? 0.f : 230.f * FMath::Cos(Angle),
				Jet == 0 ? 0.f : 230.f * FMath::Sin(Angle), Up);
			const float Z = Up * T - 1150.f * T * T;
			if (Z <= 0.f) { continue; }
			const FVector Position = Splash.Position + FVector(Initial.X * T, Initial.Y * T, Z);
			const FVector Velocity = Initial - FVector(0.f, 0.f, 2300.f * T);
			Spray->AddInstance(FTransform(Velocity.Rotation(), Position, FVector(1.35f, 0.23f, 0.23f) * Fade), true);
		}
	}
}
