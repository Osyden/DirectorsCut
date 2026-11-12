#include "DCKraken.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "DCCrew.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCKraken::ADCKraken()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADCKraken::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);
	SeaZ = Director ? Director->SeaLevelZ : 0.f;

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Kraken.M_Kraken"));
	if (!Base) { Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")); }
	if (Base)
	{
		SkinMID = UMaterialInstanceDynamic::Create(Base, this);
		SkinMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.05f, 0.045f));
	}

	// Every arm shares one triangle list: a tube of Rings x Sides quads.
	for (int32 r = 0; r < Rings - 1; ++r)
	{
		for (int32 s = 0; s < Sides; ++s)
		{
			const int32 A = r * (Sides + 1) + s;
			const int32 B = A + Sides + 1;
			SharedTris.Append({ A, B, A + 1 });
			SharedTris.Append({ A + 1, B, B + 1 });
		}
	}

	Tentacles.SetNum(NumTentacles);
	for (int32 i = 0; i < NumTentacles; ++i)
	{
		PlaceTentacle(Tentacles[i], i);
		UpdateMesh(Tentacles[i], i, true);
		Mesh->SetMaterial(i, SkinMID);
	}
}

void ADCKraken::SetState(FDCTentacle& T, EDCTentacleState NewState, float Duration)
{
	T.State = NewState;
	T.StateTime = 0.f;
	T.StateDuration = Duration;
}

void ADCKraken::PlaceTentacle(FDCTentacle& T, int32 Index)
{
	// Alternate sides, spread along the hull, close enough to reach the deck.
	const float Side = (Index % 2 == 0) ? -1.f : 1.f;
	T.Base = FVector(FMath::FRandRange(-1600.f, 2000.f), Side * FMath::FRandRange(1500.f, 3000.f), SeaZ - 150.f);
	T.Length = FMath::FRandRange(3200.f, 4600.f);
	T.BaseRadius = T.Length * FMath::FRandRange(0.06f, 0.075f);
	T.Seed = FMath::FRandRange(0.f, 100.f);
	T.Rise = 0.f;
	T.Tip = T.Base;
	T.TipVel = FVector::ZeroVector;
	T.TipTarget = T.Base;
	T.bStruck = false;
	SetState(T, EDCTentacleState::Hidden, FMath::FRandRange(1.f, 6.f) + Index * 1.5f);
}

void ADCKraken::TickTentacle(FDCTentacle& T, int32 Index, float Dt, bool bActive)
{
	T.StateTime += Dt;
	const float Alpha = FMath::Clamp(T.StateTime / FMath::Max(T.StateDuration, 0.01f), 0.f, 1.f);
	const float Side = T.Base.Y < 0.f ? -1.f : 1.f;
	const FVector ToShip(0.f, -Side, 0.f);
	const float Time = Clock + T.Seed;

	// Where the arm idles: high above its base, leaning in over the ship, drifting.
	const FVector SwayTarget = T.Base
		+ FVector(0.f, 0.f, T.Length * 0.78f)
		+ ToShip * T.Length * (0.22f + 0.1f * FMath::Sin(Time * 0.5f))
		+ FVector(FMath::Sin(Time * 0.63f) * T.Length * 0.16f, FMath::Cos(Time * 0.41f) * T.Length * 0.08f, FMath::Sin(Time * 0.9f) * T.Length * 0.05f);

	float Stiffness = 14.f, Damping = 5.f;

	// Guns destroyed the proxy: the arm dies and sinks.
	if (T.bHasProxy && !T.Proxy.IsValid() && T.State != EDCTentacleState::Dying && T.State != EDCTentacleState::Hidden)
	{
		T.bHasProxy = false;
		++ArmsDestroyed;
		SetState(T, EDCTentacleState::Dying, 2.2f);
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(TEXT("Arm down! It is pulling back!"));
		}
	}

	switch (T.State)
	{
	case EDCTentacleState::Hidden:
		T.Rise = 0.f;
		T.TipTarget = T.Base;
		if (bActive && Alpha >= 1.f)
		{
			PlaceTentacle(T, Index);
			SetState(T, EDCTentacleState::Rising, 2.6f);
			if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this))
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Proxy = GetWorld()->SpawnActor<AStaticMeshActor>(T.Base, FRotator::ZeroRotator, Params);
				if (Proxy)
				{
					Proxy->GetRootComponent()->SetMobility(EComponentMobility::Movable);
					Treadmill->AddExternalHostile(Proxy, T.BaseRadius * 2.2f, 14.f);
					T.Proxy = Proxy;
					T.bHasProxy = true;
				}
			}
		}
		break;

	case EDCTentacleState::Rising:
		T.Rise = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.2f);
		T.TipTarget = FMath::Lerp(T.Base + FVector(0.f, 0.f, T.Length * 0.3f), SwayTarget, Alpha);
		Stiffness = 22.f; Damping = 6.f;
		if (Alpha >= 1.f) { SetState(T, EDCTentacleState::Sway, FMath::FRandRange(6.f, 12.f)); }
		break;

	case EDCTentacleState::Sway:
		T.Rise = 1.f;
		T.TipTarget = SwayTarget;
		if (!bActive) { SetState(T, EDCTentacleState::Dying, 2.5f); }
		else if (Alpha >= 1.f)
		{
			T.StrikePoint = FVector(FMath::Clamp(T.Base.X + FMath::FRandRange(-500.f, 500.f), -1300.f, 1500.f), Side * FMath::FRandRange(0.f, 220.f), SeaZ + 420.f);
			T.bStruck = false;
			SetState(T, EDCTentacleState::Windup, 1.0f);
		}
		break;

	case EDCTentacleState::Windup:
		// Rear back: up and away from the ship, coiling.
		T.TipTarget = T.Base + FVector(0.f, 0.f, T.Length * 0.9f) - ToShip * T.Length * 0.28f;
		Stiffness = 30.f; Damping = 8.f;
		if (Alpha >= 1.f) { SetState(T, EDCTentacleState::Strike, 0.32f); }
		break;

	case EDCTentacleState::Strike:
		T.TipTarget = T.StrikePoint;
		Stiffness = 320.f; Damping = 22.f;
		if (!T.bStruck && FVector::DistSquared(T.Tip, T.StrikePoint) < FMath::Square(450.f))
		{
			T.bStruck = true;
			if (Director) { Director->DamageShip(StrikeDamage); }
		}
		if (Alpha >= 1.f) { SetState(T, EDCTentacleState::Hold, 0.7f); }
		break;

	case EDCTentacleState::Hold:
		// Drag along the deck before letting go.
		T.TipTarget = T.StrikePoint + FVector(FMath::Sin(Time * 6.f) * 120.f, Side * 150.f * Alpha, 0.f);
		Stiffness = 120.f; Damping = 16.f;
		if (!T.bStruck) { T.bStruck = true; if (Director) { Director->DamageShip(StrikeDamage); } }
		if (Alpha >= 1.f) { SetState(T, EDCTentacleState::Recoil, 1.4f); }
		break;

	case EDCTentacleState::Recoil:
		T.TipTarget = FMath::Lerp(T.StrikePoint, SwayTarget, FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f));
		Stiffness = 26.f; Damping = 7.f;
		if (Alpha >= 1.f) { SetState(T, EDCTentacleState::Sway, FMath::FRandRange(7.f, 13.f)); }
		break;

	case EDCTentacleState::Dying:
		T.Rise = FMath::Min(T.Rise, 1.f - FMath::InterpEaseIn(0.f, 1.f, Alpha, 2.f));
		// Thrash on the way down.
		T.TipTarget = T.Base + FVector(FMath::Sin(Time * 5.f) * T.Length * 0.25f, FMath::Cos(Time * 4.f) * T.Length * 0.2f, T.Length * 0.5f * T.Rise);
		Stiffness = 40.f; Damping = 5.f;
		if (Alpha >= 1.f)
		{
			if (AActor* Proxy = T.Proxy.Get()) { Proxy->Destroy(); }
			T.bHasProxy = false;
			SetState(T, EDCTentacleState::Hidden, FMath::FRandRange(3.f, 8.f));
		}
		break;
	}

	// Tip spring, then keep the tip within reach of the emerged length.
	T.TipVel += ((T.TipTarget - T.Tip) * Stiffness - T.TipVel * Damping) * Dt;
	T.Tip += T.TipVel * Dt;
	const float Reach = FMath::Max(50.f, T.Length * T.Rise * 0.97f);
	const FVector FromBase = T.Tip - T.Base;
	if (FromBase.SizeSquared() > Reach * Reach)
	{
		T.Tip = T.Base + FromBase.GetSafeNormal() * Reach;
	}

	// The gun target rides two thirds of the way up the arm.
	if (AActor* Proxy = T.Proxy.Get())
	{
		const FVector Mid = FMath::Lerp(T.Base, T.Tip, 0.6f) + FVector(0.f, 0.f, T.Length * 0.12f * T.Rise);
		Proxy->SetActorLocation(T.Rise > 0.3f ? Mid : T.Base - FVector(0.f, 0.f, 5000.f));
	}
}

void ADCKraken::BuildSpine(const FDCTentacle& T, TArray<FVector>& Out) const
{
	Out.SetNumUninitialized(Rings);
	const float Emerged = T.Length * T.Rise;
	const float Time = Clock + T.Seed;
	const float Side = T.Base.Y < 0.f ? -1.f : 1.f;

	// Cubic Bezier: straight up out of the water, then over to the tip, which hooks downward.
	const FVector P0 = T.Base - FVector(0.f, 0.f, 200.f);
	const FVector ToTip = T.Tip - T.Base;
	const FVector P1 = T.Base + FVector(0.f, 0.f, FMath::Max(100.f, Emerged * 0.5f)) + FVector(ToTip.X, ToTip.Y, 0.f) * 0.1f;
	const FVector Hook = (FVector(ToTip.X, ToTip.Y, 0.f).GetSafeNormal() * 0.6f + FVector(0.f, 0.f, 0.8f)) * Emerged * 0.3f;
	const FVector P2 = T.Tip + Hook;
	const FVector P3 = T.Tip;

	const FVector Fwd(1.f, 0.f, 0.f);
	const FVector Lat(0.f, Side, 0.f);
	for (int32 i = 0; i < Rings; ++i)
	{
		const float U = static_cast<float>(i) / (Rings - 1);
		const float V = 1.f - U;
		FVector P = P0 * (V * V * V) + P1 * (3.f * V * V * U) + P2 * (3.f * V * U * U) + P3 * (U * U * U);
		// Travelling waves along the arm, strongest toward the tip.
		const float W = Emerged * 0.045f * U * (1.f - 0.35f * U);
		P += Fwd * (FMath::Sin(U * 7.f - Time * 2.3f) * W) + Lat * (FMath::Cos(U * 5.3f - Time * 1.7f) * W * 0.8f);
		Out[i] = P;
	}
}

void ADCKraken::UpdateMesh(const FDCTentacle& T, int32 Index, bool bCreate)
{
	TArray<FVector> Spine;
	BuildSpine(T, Spine);

	const int32 Cols = Sides + 1;
	TArray<FVector> Verts; Verts.SetNumUninitialized(Rings * Cols);
	TArray<FVector> Normals; Normals.SetNumUninitialized(Rings * Cols);
	TArray<FVector2D> UVs; UVs.SetNumUninitialized(Rings * Cols);
	TArray<FColor> Colors; Colors.SetNumUninitialized(Rings * Cols);
	TArray<FProcMeshTangent> Tangents; Tangents.SetNumUninitialized(Rings * Cols);

	// Frame carried up the arm by parallel transport so the tube never twists or flips.
	const float SideSign = T.Base.Y < 0.f ? -1.f : 1.f;
	FVector Belly(0.f, -SideSign, 0.f); // the underside faces the ship
	const float Time = Clock + T.Seed;

	for (int32 r = 0; r < Rings; ++r)
	{
		const float U = static_cast<float>(r) / (Rings - 1);
		const FVector Tangent = (Spine[FMath::Min(r + 1, Rings - 1)] - Spine[FMath::Max(r - 1, 0)]).GetSafeNormal();
		Belly = (Belly - Tangent * FVector::DotProduct(Belly, Tangent)).GetSafeNormal();
		if (Belly.IsNearlyZero()) { Belly = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal(); }
		const FVector Bitangent = FVector::CrossProduct(Tangent, Belly);

		// Taper, with a slow pulse travelling up the arm; collapsed while under water.
		float Radius = FMath::Lerp(T.BaseRadius, T.BaseRadius * 0.07f, FMath::Pow(U, 0.8f));
		Radius *= 1.f + 0.06f * FMath::Sin(U * 9.f - Time * 3.f);
		Radius *= FMath::Clamp(T.Rise * 4.f, 0.02f, 1.f);
		if (r == Rings - 1) { Radius *= 0.15f; }

		for (int32 s = 0; s <= Sides; ++s)
		{
			const float A = 2.f * UE_PI * s / Sides;
			const float CA = FMath::Cos(A), SA = FMath::Sin(A);
			// Slightly flattened on the belly side, like a real arm.
			const FVector Dir = Belly * CA + Bitangent * SA;
			const float Flat = CA > 0.f ? 0.82f : 1.f;
			const int32 I = r * Cols + s;
			Verts[I] = Spine[r] + Dir * Radius * FMath::Lerp(1.f, Flat, FMath::Abs(CA));
			Normals[I] = Dir;
			UVs[I] = FVector2D(U * T.Length / 400.f, static_cast<float>(s) / Sides);
			// R = how much this vertex is belly; G = position across the belly, 0..1 (0.5 is the centre line).
			const float BellyAmount = FMath::Clamp((CA - 0.25f) / 0.6f, 0.f, 1.f);
			Colors[I] = FColor(static_cast<uint8>(BellyAmount * 255.f), static_cast<uint8>(FMath::Clamp(0.5f + SA * 0.6f, 0.f, 1.f) * 255.f), static_cast<uint8>(U * 255.f), 255);
			Tangents[I] = FProcMeshTangent(Tangent, false);
		}
	}

	if (bCreate)
	{
		Mesh->CreateMeshSection(Index, Verts, SharedTris, Normals, UVs, Colors, Tangents, false);
	}
	else
	{
		Mesh->UpdateMeshSection(Index, Verts, Normals, UVs, Colors, Tangents);
	}
}

void ADCKraken::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director)
	{
		Director = ADCRideDirector::Get(this);
		if (!Director) { return; }
	}
	Clock += DeltaSeconds;
	const bool bActive = Director->GetPhase() == EDCPhase::Maelstrom;

	bool bAnyVisible = false;
	for (int32 i = 0; i < Tentacles.Num(); ++i)
	{
		FDCTentacle& T = Tentacles[i];
		if (!bActive && T.State == EDCTentacleState::Hidden)
		{
			T.StateTime = 0.f;
			continue;
		}
		TickTentacle(T, i, DeltaSeconds, bActive);
		UpdateMesh(T, i, false);
		bAnyVisible = true;
	}
	Mesh->SetVisibility(bAnyVisible);
}
