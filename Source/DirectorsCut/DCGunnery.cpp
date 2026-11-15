#include "DCGunnery.h"
#include "DCTreadmill.h"
#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "DCCrew.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	int32 TraceHostile(const ADCTreadmill* Treadmill, const FVector& Start, const FVector& End, FVector& OutPoint)
	{
		if (!Treadmill) { return INDEX_NONE; }
		const FVector Delta = End - Start;
		const double LengthSquared = Delta.SizeSquared();
		if (LengthSquared < UE_SMALL_NUMBER) { return INDEX_NONE; }
		double First = 2.0;
		int32 Result = INDEX_NONE;
		for (int32 I = 0; I < Treadmill->NumObjects(); ++I)
		{
			const AActor* Actor = Treadmill->GetHostileActor(I);
			if (!Actor || Actor->IsHidden()) { continue; }
			const FVector Offset = Start - Actor->GetActorLocation();
			const double Radius = Treadmill->GetHostileRadius(I) + 15.f;
			const double B = FVector::DotProduct(Offset, Delta);
			const double C = Offset.SizeSquared() - Radius * Radius;
			const double Discriminant = B * B - LengthSquared * C;
			if (Discriminant < 0.0) { continue; }
			const double Fraction = C <= 0.0 ? 0.0 : (-B - FMath::Sqrt(Discriminant)) / LengthSquared;
			if (Fraction >= 0.0 && Fraction <= 1.0 && Fraction < First)
			{ First = Fraction; Result = I; }
		}
		if (Result != INDEX_NONE) { OutPoint = Start + Delta * First; }
		return Result;
	}

	void IgnoreInactiveWater(UWorld* World, FCollisionQueryParams& Query)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			// Identify the actual inactive water, never infer it from hit height/normal.
			if (It->GetClass()->GetPathName().StartsWith(TEXT("/DynamicRealWater/")))
			{ Query.AddIgnoredActor(*It); }
		}
		if (const ADCRideDirector* D = ADCRideDirector::Get(World))
		{
			if (const UPrimitiveComponent* Sea = Cast<UPrimitiveComponent>(D->GetSea()))
			{ Query.AddIgnoredComponent(Sea); }
		}
	}

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
	Guns.Reset();
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
			// The Brigantine mesh is authored with its origin at the bore opening.
			// Bounds include the carriage behind it and are not a muzzle socket.
			const FVector Muzzle = P->GetComponentLocation();
			Muzzles[SideIdx].Add(Muzzle);
			FGun& Gun = Guns.AddDefaulted_GetRef();
			Gun.Component = Cast<USkeletalMeshComponent>(P);
			Gun.LocalMuzzle = P->GetComponentTransform().InverseTransformPosition(Muzzle);
			Gun.Side = SideIdx == 0 ? EDCSide::Port : EDCSide::Starboard;
			Gun.Station = Loc + FVector(0.f, -FMath::Sign(Loc.Y) * 150.f, 180.f);
		}
	}
	// Stable identity: the named upper-deck gun at each existing player station.
	Guns.Sort([](const FGun& A, const FGun& B) { return A.Component->GetName() < B.Component->GetName(); });
	for (EDCSide Side : { EDCSide::Port, EDCSide::Starboard })
	{
		int32 Best = INDEX_NONE;
		float BestDistance = TNumericLimits<float>::Max();
		const TCHAR* Name = Side == EDCSide::Port ? TEXT("SK_Cannon_2-L9") : TEXT("SK_Cannon_2-R9");
		for (int32 Index = 0; Index < Guns.Num(); ++Index)
		{
			FGun& Gun = Guns[Index];
			if (Gun.Side != Side) { continue; }
			const FVector Location = Gun.Component->Bounds.Origin;
			const float Distance = Gun.Component->GetName() == Name ? -1.f : FVector::DistSquared(Location, FVector(-110.f, Side == EDCSide::Port ? -317.f : 317.f, 542.f));
			if (Distance < BestDistance) { Best = Index; BestDistance = Distance; }
		}
		if (Guns.IsValidIndex(Best)) { Guns[Best].bPlayer = true; }
	}
	UE_LOG(LogDC, Log, TEXT("Gunnery: %d port guns, %d starboard guns"), Muzzles[0].Num(), Muzzles[1].Num());

	// Effects that ship with the vessel asset; optional.
	MuzzleEffectClass = LoadClass<AActor>(nullptr, TEXT("/Game/Brigantine/BP/Cannon/BP_EffectExplosionStartCannon.BP_EffectExplosionStartCannon_C"));
	ImpactEffectClass = LoadClass<AActor>(nullptr, TEXT("/Game/Brigantine/BP/Cannon/BP_EffectExplosionEnd.BP_EffectExplosionEnd_C"));
	WaterImpact = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/ParagonRampage/FX/Particles/Abilities/RipNToss/FX/P_Rampage_Rock_HitWorld_WaterImpact.P_Rampage_Rock_HitWorld_WaterImpact"));
	StoneImpact = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/ParagonRampage/FX/Particles/Abilities/RipNToss/FX/P_Rampage_Rock_HitWorld.P_Rampage_Rock_HitWorld"));
	CreatureImpact = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/ParagonRampage/FX/Particles/Abilities/RipNToss/FX/P_Rampage_Rock_HitCharacter.P_Rampage_Rock_HitCharacter"));
	UE_LOG(LogDC, Log, TEXT("Gunnery effects: muzzle=%s, water=%s, stone=%s, creature=%s"),
		*GetNameSafe(MuzzleEffectClass.Get()), *GetNameSafe(WaterImpact.Get()), *GetNameSafe(StoneImpact.Get()), *GetNameSafe(CreatureImpact.Get()));
}

bool ADCGunnery::ResolveAim(EDCSide Side, const FVector& Origin, const FVector& Direction, FVector& OutTarget) const
{
	constexpr float MinRange = 650.f;
	constexpr float MaxRange = 20000.f;
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
	IgnoreInactiveWater(GetWorld(), Query);
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
	FVector HostilePoint;
	const ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	const int32 HostileIndex = TraceHostile(Treadmill, Origin, OutTarget, HostilePoint);
	if (HostileIndex != INDEX_NONE)
	{
		OutTarget = HostilePoint;
		// Early encounters reward keeping the sight on the visible creature. Its
		// approach is led here so a good aim does not miss solely during ball flight.
		const AActor* Hostile = Treadmill->GetHostileActor(HostileIndex);
		if (Hostile && Hostile->ActorHasTag(TEXT("DCEarlyThreat")))
		{
			const FVector Point = Hostile->GetActorLocation();
			const FVector Velocity = Hostile->GetVelocity().GetClampedToMaxSize(3500.f);
			float Flight = FMath::Clamp(FVector::Distance(GetMuzzle(GetPlayerGun(Side)), Point) / ShotSpeed, .5f, 3.f);
			OutTarget = Point + Velocity * Flight;
			Flight = FMath::Clamp(FVector::Distance(GetMuzzle(GetPlayerGun(Side)), OutTarget) / ShotSpeed, .5f, 3.f);
			OutTarget = Point + Velocity * Flight;
		}
		Distance = FVector::Distance(Origin, OutTarget);
		bSurface = true;
	}
	return bSurface && Distance >= MinRange && Sign * OutTarget.Y >= 1000.f && CanAimGun(GetPlayerGun(Side), OutTarget);
}

FString ADCGunnery::GetPlayerImpactLabel(EDCSide Side) const
{
	const int32 Index = Side == EDCSide::Port ? 0 : 1;
	if (GetWorld()->GetTimeSeconds() >= ImpactUntil[Index]) { return FString(); }
	return bImpactHit[Index] ? TEXT("HIT") : TEXT("SPLASH");
}

int32 ADCGunnery::GetPlayerGun(EDCSide Side) const
{
	for (int32 I = 0; I < Guns.Num(); ++I) { if (Guns[I].Side == Side && Guns[I].bPlayer) { return I; } }
	return INDEX_NONE;
}

EDCSide ADCGunnery::GetGunSide(int32 Gun) const { return Guns.IsValidIndex(Gun) ? Guns[Gun].Side : EDCSide::None; }
FVector ADCGunnery::GetGunStation(int32 Gun) const { return Guns.IsValidIndex(Gun) ? Guns[Gun].Station : FVector::ZeroVector; }
FVector ADCGunnery::GetMuzzle(int32 Gun) const
{
	return Guns.IsValidIndex(Gun) && Guns[Gun].Component.IsValid()
		? Guns[Gun].Component->GetComponentTransform().TransformPosition(Guns[Gun].LocalMuzzle) : FVector::ZeroVector;
}
bool ADCGunnery::IsGunReady(int32 Gun) const { return Guns.IsValidIndex(Gun) && Guns[Gun].Component.IsValid() && Guns[Gun].Reload <= 0.f; }
float ADCGunnery::GetReloadSeconds(EDCSide Side) const
{
	const int32 Gun = GetPlayerGun(Side);
	return Guns.IsValidIndex(Gun) ? Guns[Gun].Reload : 0.f;
}
int32 ADCGunnery::NumGuns(EDCSide Side) const
{
	int32 Count = 0;
	for (const FGun& Gun : Guns) { if (Gun.Side == Side) { ++Count; } }
	return Count;
}
bool ADCGunnery::CanAimGun(int32 Gun, const FVector& Target) const
{
	if (!Guns.IsValidIndex(Gun) || !Guns[Gun].Component.IsValid() || Target.ContainsNaN()) { return false; }
	const FVector Delta = Target - GetMuzzle(Gun);
	const FVector Local = GunShip.IsValid() ? GunShip->GetActorTransform().InverseTransformVectorNoScale(Delta) : Delta;
	const float Sign = Guns[Gun].Side == EDCSide::Port ? -1.f : 1.f;
	return Delta.SizeSquared() <= FMath::Square(21000.f) && Sign * Local.Y > 100.f
		&& FMath::Abs(Local.X) <= Sign * Local.Y * 1.9f;
}
bool ADCGunnery::HasClearShot(int32 Gun, const AActor* Target) const
{
	if (!IsValid(Target) || Target->IsHidden() || !CanAimGun(Gun, Target->GetActorLocation())) { return false; }
	FCollisionQueryParams Query(SCENE_QUERY_STAT(DCCrewGunSight), false, this);
	if (GunShip.IsValid()) { Query.AddIgnoredActor(GunShip.Get()); }
	IgnoreInactiveWater(GetWorld(), Query);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, GetMuzzle(Gun), Target->GetActorLocation(), ECC_Visibility, Query)
		|| Hit.GetActor() == Target;
}

bool ADCGunnery::FindCrewTarget(int32 Gun, FVector& OutTarget, bool bLeadTarget) const
{
	const ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	if (!Treadmill || !Guns.IsValidIndex(Gun)) { return false; }
	float Best = TNumericLimits<float>::Max();
	bool bFound = false;
	for (int32 I = 0; I < Treadmill->NumObjects(); ++I)
	{
		AActor* Target = Treadmill->GetHostileActor(I);
		if (!HasClearShot(Gun, Target)) { continue; }
		const FVector Point = Target->GetActorLocation();
		const float Priority = FVector::Distance(GetMuzzle(Gun), Point) - (Treadmill->IsUrgentHostile(I) ? 40000.f : 0.f);
		// Moving bank creatures must be led; aiming at their previous position made
		// otherwise valid broadside opportunities miss while they approached.
		const FVector Velocity = Target->GetVelocity().GetClampedToMaxSize(3500.f);
		const float Flight = FVector::Distance(GetMuzzle(Gun), Point) / FMath::Max(ShotSpeed, 1.f);
		const FVector LedPoint = bLeadTarget ? Point + Velocity * FMath::Clamp(Flight, 0.f, 3.f) : Point;
		if (Priority < Best && CanAimGun(Gun, LedPoint)) { Best = Priority; OutTarget = LedPoint; bFound = true; }
	}
	return bFound;
}
bool ADCGunnery::FireGun(int32 Gun, const FVector& Target, bool bPlayer)
{
	if (!IsGunReady(Gun) || !CanAimGun(Gun, Target) || bPlayer != Guns[Gun].bPlayer) { return false; }
	Guns[Gun].Reload = ReloadTime;
	const FVector Muzzle = GetMuzzle(Gun);
	Pending.Add({ Gun, Target, bPlayer ? Guns[Gun].Side : EDCSide::None });
	UE_LOG(LogDC, Log, TEXT("Cannon fired gun=%s player=%d muzzle=%s target=%s"),
		*Guns[Gun].Component->GetName(), bPlayer, *Muzzle.ToCompactString(), *Target.ToCompactString());
	return true;
}
bool ADCGunnery::FireBroadside(EDCSide Side, const FVector& Target, int32 MaxGuns, bool bPlayerVolley)
{
	// Compatibility for unattended callers: one trigger always means one cannonball.
	if (MaxGuns <= 0) { return false; }
	if (bPlayerVolley) { return FireGun(GetPlayerGun(Side), Target, true); }
	for (int32 I = 0; I < Guns.Num(); ++I)
	{
		if (Guns[I].Side == Side && !Guns[I].bPlayer && FireGun(I, Target)) { return true; }
	}
	return false;
}

void ADCGunnery::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("DCCannonVerify"))) { TickVerification(); }
#endif

	for (FGun& Gun : Guns) { Gun.Reload = FMath::Max(0.f, Gun.Reload - DeltaSeconds); }
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
		if (!Guns.IsValidIndex(P.Gun) || !Guns[P.Gun].Component.IsValid()) { Pending.RemoveAtSwap(i); continue; }
		const FVector Muzzle = GetMuzzle(P.Gun);

		// Solve for the launch velocity that lands on the target after FlightTime under our gravity.
		const FVector Delta = P.Target - Muzzle;
		const float FlightTime = FMath::Clamp(Delta.Size() / ShotSpeed, 0.5f, 3.f);
		FShot S;
		S.Pos = Muzzle;
		S.Vel = Delta / FlightTime + FVector(0.f, 0.f, 0.5f * Gravity * FlightTime);
		S.Life = FlightTime + 0.8f;
		S.PlayerSide = P.PlayerSide;
		Shots.Add(S);

		if (MuzzleEffectClass)
		{
			if (AActor* Fx = GetWorld()->SpawnActor<AActor>(MuzzleEffectClass, Muzzle, S.Vel.Rotation(), Params))
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
		bool bCreatureHit = false;
		FVector ImpactNormal = FVector::UpVector;
		bool bSplash = false;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(DCGunShot), false, this);
		if (GunShip.IsValid()) { Query.AddIgnoredActor(GunShip.Get()); }
		IgnoreInactiveWater(GetWorld(), Query);
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
			bool bGeometry = GetWorld()->LineTraceSingleByChannel(Hit, Previous, S.Pos, ECC_Visibility, Query);
			if (bGeometry) { S.Pos = Hit.ImpactPoint; ImpactNormal = Hit.ImpactNormal; }
			bHitTarget = bGeometry;
			FVector HostilePoint;
			int32 Hostile = TraceHostile(Treadmill, Previous, S.Pos, HostilePoint);
			if (Hostile == INDEX_NONE && bGeometry && Treadmill)
			{
				// A creature's authored collider may extend beyond its fallback target sphere.
				for (int32 Target = 0; Target < Treadmill->NumObjects(); ++Target)
				{
					if (Treadmill->GetHostileActor(Target) == Hit.GetActor())
					{ Hostile = Target; HostilePoint = S.Pos; break; }
				}
			}
			if (Hostile != INDEX_NONE)
			{
				S.Pos = HostilePoint;
				ImpactNormal = -S.Vel.GetSafeNormal();
				Treadmill->DamageObject(Hostile, 1.f);
				bHitTarget = true;
				bCreatureHit = true;
				bSplash = false;
			}
			const float SurfaceZ = Maelstrom ? Maelstrom->SampleSurfaceWorld(S.Pos) : SeaLevelZ;
			if (bGeometry && S.Pos.Z > SurfaceZ + 10.f) { bSplash = false; }
		}
		S.Life -= DeltaSeconds;
		if (bHitTarget || bSplash || S.Life <= 0.f)
		{
			// Water misses do not deal area damage to creatures or rocks.
			if (S.PlayerSide != EDCSide::None && (bHitTarget || bSplash))
			{
				const int32 Index = S.PlayerSide == EDCSide::Port ? 0 : 1;
				const float Now = GetWorld()->GetTimeSeconds();
				bImpactHit[Index] = bHitTarget || (bImpactHit[Index] && Now < ImpactUntil[Index]);
				ImpactUntil[Index] = Now + 1.15f;
			}
			static const bool bGunLog = FParse::Param(FCommandLine::Get(), TEXT("DCGunLog"));
			if (bGunLog) { UE_LOG(LogDC, Log, TEXT("Shot ended at %s: hit %d splash %d life %.2f"), *S.Pos.ToCompactString(), bHitTarget, bSplash, S.Life); }
			UParticleSystem* Effect = bHitTarget ? (bCreatureHit ? CreatureImpact.Get() : StoneImpact.Get()) : (bSplash ? WaterImpact.Get() : nullptr);
			if (Effect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Effect, S.Pos,
					bHitTarget ? FRotationMatrix::MakeFromZ(ImpactNormal).Rotator() : FRotator::ZeroRotator,
					FVector(bHitTarget ? 0.55f : 0.7f));
			}
			else if (bSplash && !bHitTarget) { Splashes.Add({S.Pos, 0.f}); }
			else if (ImpactEffectClass && bHitTarget)
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

#if !UE_BUILD_SHIPPING
void ADCGunnery::TickVerification()
{
	// Opt-in integration scenario in the actual ship/map, never used in normal play.
	const float Time = GetWorld()->GetTimeSeconds();
	if (VerificationStage == 0 && Time > 1.f)
	{
		VerificationStage = 1;
		UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
		Crew->ExecuteOrder(TEXT("Duncan, man the starboard guns"));
		Crew->ExecuteOrder(TEXT("Ruby, man the starboard guns"));
		const int32 Marlow = Crew->FindCrewGun(EDCSide::Starboard, TEXT("Duncan"));
		const int32 Odessa = Crew->FindCrewGun(EDCSide::Starboard, TEXT("Ruby"));
		auto Check = [this](bool bPassed, const TCHAR* What)
		{
			UE_LOG(LogDC, Display, TEXT("CANNON VERIFY %s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), What);
			bVerificationFailed |= !bPassed;
		};
		Check(Marlow != INDEX_NONE && Odessa != INDEX_NONE && Marlow != Odessa, TEXT("two crew have distinct guns on one side"));
		Crew->ExecuteOrder(TEXT("Duncan, repair the rigging"));
		Crew->ExecuteOrder(TEXT("Duncan, man the starboard guns"));
		Check(Crew->FindCrewGun(EDCSide::Starboard, TEXT("Duncan")) == Marlow, TEXT("gun reservation survives repair duty"));
		const FDCStation* M = Crew->GetStation(Marlow);
		const FDCStation* O = Crew->GetStation(Odessa);
		Check(M && O && M->GunIndex != GetPlayerGun(EDCSide::Starboard) && O->GunIndex != GetPlayerGun(EDCSide::Starboard), TEXT("crew never take the player gun"));
		const int32 Before = Pending.Num();
		Check(FireGun(GetPlayerGun(EDCSide::Port), FVector(0.f, -8000.f, SeaLevelZ), true), TEXT("player port gun fires"));
		Check(FireGun(GetPlayerGun(EDCSide::Starboard), FVector(0.f, 8000.f, SeaLevelZ), true), TEXT("player starboard gun fires independently"));
		Check(Pending.Num() == Before + 2, TEXT("one ball queued per trigger"));
		Check(!FireGun(GetPlayerGun(EDCSide::Starboard), FVector(0.f, 8000.f, SeaLevelZ), true), TEXT("same gun cannot bypass reload"));
		Check(M && O && IsGunReady(M->GunIndex) && IsGunReady(O->GunIndex), TEXT("player reload does not lock crew guns"));
		Check(M && !FireGun(M->GunIndex, FVector(0.f, 8000.f, SeaLevelZ), true), TEXT("player cannot fire an NPC gun"));
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Target = GetWorld()->SpawnActor<AActor>(FVector(0.f, 5000.f, SeaLevelZ + 1200.f), FRotator::ZeroRotator, Params);
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Target);
		Target->SetRootComponent(Mesh);
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		Mesh->RegisterComponent();
		Target->SetActorLocation(FVector(0.f, 5000.f, SeaLevelZ + 1200.f));
		Target->SetActorScale3D(FVector(4.f));
		VerificationTarget = Target;
		ADCTreadmill::Get(this)->AddExternalHostile(Target, 200.f, 1.f, true);
	}
	else if (VerificationStage == 1 && Time > 12.f)
	{
		VerificationStage = 2;
		bVerificationFailed |= VerificationTarget.IsValid();
		UE_LOG(LogDC, Display, TEXT("CANNON VERIFY %s: NPCs walk to their guns and damage a real collision target"), VerificationTarget.IsValid() ? TEXT("FAIL") : TEXT("PASS"));
		UE_LOG(LogDC, Display, TEXT("CANNON VERIFICATION %s"), bVerificationFailed ? TEXT("FAILED") : TEXT("PASSED"));
		FPlatformMisc::RequestExitWithStatus(false, bVerificationFailed ? 1 : 0);
	}
}
#endif
