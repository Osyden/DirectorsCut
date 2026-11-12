#include "DCKrakenHead.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const int32 TubeRings = 34;
	const int32 TubeSides = 14;
	// Everything below is in the creature's own space: origin on the crater floor, +Y toward the ship.
	const FVector MoundRadii(11000.f, 11000.f, 9000.f);
	const int32 NeckSection = 1;
	const int32 FirstArmSection = 2;
	// When it roars, in seconds after the maelstrom begins. Kept in step with the roar sounds in DCAudio.
	const float RoarTimes[] = { 27.f, 62.f, 110.f, 148.f };

	/** Skins a spine with a tapering tube. Belly is the side that gets the pale suckered underside. */
	void SkinTube(const TArray<FVector>& Spine, FVector Belly, float BaseRadius, float TipRadius, float UvLength,
		TArray<FVector>& Verts, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& Colors, TArray<FProcMeshTangent>& Tangents, bool bCloseTip)
	{
		const int32 Rings = Spine.Num();
		const int32 Cols = TubeSides + 1;
		Verts.SetNumUninitialized(Rings * Cols);
		Normals.SetNumUninitialized(Rings * Cols);
		UVs.SetNumUninitialized(Rings * Cols);
		Colors.SetNumUninitialized(Rings * Cols);
		Tangents.SetNumUninitialized(Rings * Cols);
		for (int32 r = 0; r < Rings; ++r)
		{
			const float U = static_cast<float>(r) / (Rings - 1);
			const FVector Tangent = (Spine[FMath::Min(r + 1, Rings - 1)] - Spine[FMath::Max(r - 1, 0)]).GetSafeNormal();
			Belly = (Belly - Tangent * FVector::DotProduct(Belly, Tangent)).GetSafeNormal();
			const FVector Bitangent = FVector::CrossProduct(Tangent, Belly);
			float Radius = FMath::Lerp(BaseRadius, TipRadius, FMath::Pow(U, 0.8f));
			if (bCloseTip && r == Rings - 1) { Radius *= 0.2f; }
			for (int32 s = 0; s <= TubeSides; ++s)
			{
				const float A = 2.f * UE_PI * s / TubeSides;
				const float CA = FMath::Cos(A), SA = FMath::Sin(A);
				const FVector Dir = Belly * CA + Bitangent * SA;
				const int32 I = r * Cols + s;
				Verts[I] = Spine[r] + Dir * Radius;
				Normals[I] = Dir;
				UVs[I] = FVector2D(U * UvLength, static_cast<float>(s) / TubeSides);
				Colors[I] = FColor(static_cast<uint8>(FMath::Clamp((CA - 0.25f) / 0.6f, 0.f, 1.f) * 255.f), static_cast<uint8>(FMath::Clamp(0.5f + SA * 0.6f, 0.f, 1.f) * 255.f), static_cast<uint8>(U * 255.f), 255);
				Tangents[I] = FProcMeshTangent(Tangent, false);
			}
		}
	}
}

ADCKrakenHead::ADCKrakenHead()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UStaticMeshComponent* ADCKrakenHead::AddPart(USceneComponent* Parent, const TCHAR* Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale, UMaterialInterface* Material)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Material) { C->SetMaterial(0, Material); }
	C->SetupAttachment(Parent);
	C->SetRelativeTransform(FTransform(Rotation, Location, Scale));
	C->RegisterComponent();
	return C;
}

void ADCKrakenHead::BuildMound()
{
	// The back of the thing, breaking the surface at the bottom of the crater.
	const int32 Rings = 16, Sides = 28;
	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	for (int32 r = 0; r <= Rings; ++r)
	{
		const float V = static_cast<float>(r) / Rings;
		const float Phi = V * UE_PI * 0.55f;
		for (int32 s = 0; s <= Sides; ++s)
		{
			const float A = 2.f * UE_PI * s / Sides;
			const FVector D(FMath::Sin(Phi) * FMath::Cos(A), FMath::Sin(Phi) * FMath::Sin(A), FMath::Cos(Phi));
			const float N = FMath::PerlinNoise3D(D * 2.3f + FVector(11.f)) * 0.18f + FMath::PerlinNoise3D(D * 6.f) * 0.06f;
			Verts.Add(FVector(D.X * MoundRadii.X, D.Y * MoundRadii.Y, D.Z * MoundRadii.Z) * (1.f + N));
			UVs.Add(FVector2D(static_cast<float>(s) / Sides * 16.f, V * 12.f));
			Colors.Add(FColor(0, 128, 0, 255));
		}
	}
	const int32 Cols = Sides + 1;
	for (int32 r = 0; r < Rings; ++r)
	{
		for (int32 s = 0; s < Sides; ++s)
		{
			const int32 A = r * Cols + s, B = A + Cols;
			Tris.Append({ A, A + 1, B });
			Tris.Append({ A + 1, B + 1, B });
		}
	}
	TArray<FVector> Normals;
	Normals.Init(FVector::ZeroVector, Verts.Num());
	for (int32 i = 0; i < Tris.Num(); i += 3)
	{
		const FVector N = FVector::CrossProduct(Verts[Tris[i + 2]] - Verts[Tris[i]], Verts[Tris[i + 1]] - Verts[Tris[i]]);
		Normals[Tris[i]] += N; Normals[Tris[i + 1]] += N; Normals[Tris[i + 2]] += N;
	}
	for (FVector& N : Normals) { N = N.GetSafeNormal(); }
	Mesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
}

void ADCKrakenHead::UpdateArm(int32 Index, bool bCreate)
{
	const float Seed = ArmSeeds[Index];
	// Arms stand in a ring around the base, bowing outward and curling back in at the tips.
	const float Angle = 2.f * UE_PI * (Index + 0.5f) / NumArms;
	const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	const FVector Base = Out * MoundRadii.X * 1.35f - FVector(0.f, 0.f, 3000.f);
	const float Length = 24000.f + 8000.f * FMath::Sin(Seed * 7.f);
	const FVector Side = FVector::CrossProduct(Out, FVector::UpVector);
	const float T = Clock * 0.45f + Seed;

	TArray<FVector> Spine; Spine.SetNumUninitialized(TubeRings);
	for (int32 r = 0; r < TubeRings; ++r)
	{
		const float U = static_cast<float>(r) / (TubeRings - 1);
		FVector P = Base + FVector(0.f, 0.f, Length * U * (1.f - 0.25f * U));
		P += Out * Length * (0.32f * FMath::Sin(U * UE_PI * 0.9f) - 0.28f * U * U);
		P += Side * Length * 0.12f * FMath::Sin(U * 4.f - T * 1.3f) * U;
		P += Out * Length * 0.07f * FMath::Sin(U * 5.f - T * 1.7f + 1.f) * U;
		Spine[r] = P;
	}
	TArray<FVector> Verts, Normals; TArray<FVector2D> UVs; TArray<FColor> Colors; TArray<FProcMeshTangent> Tangents;
	SkinTube(Spine, -Out, Length * 0.055f, Length * 0.0035f, Length / 4000.f, Verts, Normals, UVs, Colors, Tangents, true);
	if (bCreate) { Mesh->CreateMeshSection(FirstArmSection + Index, Verts, TubeTris, Normals, UVs, Colors, Tangents, false); }
	else { Mesh->UpdateMeshSection(FirstArmSection + Index, Verts, Normals, UVs, Colors, Tangents); }
}

void ADCKrakenHead::UpdateNeck(bool bCreate)
{
	// An S-curve: up out of the water, leaning back, then arching over toward the ship. It sways,
	// and rears a little when the jaws open.
	const float T = Clock * 0.32f;
	const float Rear = JawOpen * 0.08f;
	NeckSpine.SetNumUninitialized(TubeRings);
	for (int32 r = 0; r < TubeRings; ++r)
	{
		const float U = static_cast<float>(r) / (TubeRings - 1);
		FVector P(0.f, 0.f, -4000.f + NeckLength * (U * (1.f - 0.12f * U) + Rear * U * U));
		P.Y += NeckLength * (-0.16f * FMath::Sin(U * UE_PI) + 0.5f * U * U * U);
		P.X += NeckLength * 0.07f * FMath::Sin(U * 2.6f - T) * U;
		P.Y += NeckLength * 0.035f * FMath::Sin(U * 3.4f - T * 1.3f + 1.f) * U;
		NeckSpine[r] = P;
	}
	TArray<FVector> Verts, Normals; TArray<FVector2D> UVs; TArray<FColor> Colors; TArray<FProcMeshTangent> Tangents;
	// The pale underside faces the ship.
	SkinTube(NeckSpine, FVector(0.f, 1.f, 0.f), NeckLength * 0.17f, NeckLength * 0.1f, NeckLength / 4000.f, Verts, Normals, UVs, Colors, Tangents, false);
	if (bCreate) { Mesh->CreateMeshSection(NeckSection, Verts, TubeTris, Normals, UVs, Colors, Tangents, false); }
	else { Mesh->UpdateMeshSection(NeckSection, Verts, Normals, UVs, Colors, Tangents); }

	// Head: at the top of the neck, looking along it and down at the ship.
	const FVector Top = NeckSpine.Last();
	const FVector Along = (NeckSpine.Last() - NeckSpine[TubeRings - 3]).GetSafeNormal();
	const FVector Look = (Along * 0.55f + FVector(0.f, 0.8f, -0.32f)).GetSafeNormal();
	if (HeadRoot)
	{
		HeadRoot->SetRelativeLocationAndRotation(Top, FRotationMatrix::MakeFromXZ(Look, FVector::UpVector).Rotator());
	}
	// Spines down the back of the neck (the side away from the ship).
	for (int32 i = 0; i < Spines.Num(); ++i)
	{
		const int32 R = FMath::Clamp(3 + i * 2, 1, TubeRings - 2);
		const float U = static_cast<float>(R) / (TubeRings - 1);
		const FVector Tangent = (NeckSpine[R + 1] - NeckSpine[R - 1]).GetSafeNormal();
		const FVector Back = (FVector(0.f, -1.f, 0.f) - Tangent * FVector::DotProduct(FVector(0.f, -1.f, 0.f), Tangent)).GetSafeNormal();
		const float Radius = FMath::Lerp(NeckLength * 0.17f, NeckLength * 0.1f, FMath::Pow(U, 0.8f));
		// Cones point along their own +Z; lean them back toward the base.
		const FVector Up = (Back * 0.85f - Tangent * 0.5f).GetSafeNormal();
		Spines[i]->SetRelativeLocationAndRotation(NeckSpine[R] + Back * Radius * 0.85f + Up * 900.f, FRotationMatrix::MakeFromZX(Up, Tangent).Rotator());
	}
}

void ADCKrakenHead::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);

	UMaterialInterface* Skin = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Kraken.M_Kraken"));
	UMaterialInterface* Stone = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Stone.M_Stone"));
	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Glow.M_Glow"));
	if (Stone)
	{
		// Horn, bone and hide for the parts built from simple shapes.
		HideMID = UMaterialInstanceDynamic::Create(Stone, this);
		HideMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.035f, 0.075f, 0.07f));
		MawMID = UMaterialInstanceDynamic::Create(Stone, this);
		MawMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.03f, 0.03f));
	}
	if (Glow) { EyeMID = UMaterialInstanceDynamic::Create(Glow, this); }

	BuildMound();
	Mesh->SetMaterial(0, Skin);

	for (int32 r = 0; r < TubeRings - 1; ++r)
	{
		for (int32 s = 0; s < TubeSides; ++s)
		{
			const int32 A = r * (TubeSides + 1) + s, B = A + TubeSides + 1;
			TubeTris.Append({ A, B, A + 1 });
			TubeTris.Append({ A + 1, B, B + 1 });
		}
	}

	// Head, built around a root whose +X is "forward". Sizes in cm: the skull is about 70 m long.
	HeadRoot = NewObject<USceneComponent>(this, TEXT("HeadRoot"));
	HeadRoot->SetMobility(EComponentMobility::Movable);
	HeadRoot->SetupAttachment(Mesh);
	HeadRoot->RegisterComponent();
	const float K = HeadScale;
	AddPart(HeadRoot, TEXT("Sphere"), FVector(1200.f, 0.f, 300.f) * K, FRotator::ZeroRotator, FVector(52.f, 40.f, 34.f) * K, HideMID);            // skull
	AddPart(HeadRoot, TEXT("Cone"), FVector(5600.f, 0.f, 500.f) * K, FRotator(-90.f, 0.f, 0.f), FVector(30.f, 17.f, 62.f) * K, HideMID);          // upper jaw / snout
	AddPart(HeadRoot, TEXT("Sphere"), FVector(3000.f, 0.f, 1500.f) * K, FRotator(-12.f, 0.f, 0.f), FVector(34.f, 22.f, 12.f) * K, HideMID);       // brow ridge
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		// Eyes under the brow, horns sweeping back, a row of teeth along the upper jaw.
		AddPart(HeadRoot, TEXT("Sphere"), FVector(3300.f, Side * 1500.f, 950.f) * K, FRotator::ZeroRotator, FVector(8.f, 5.f, 6.f) * K, EyeMID);
		AddPart(HeadRoot, TEXT("Cone"), FVector(-1200.f, Side * 1300.f, 2300.f) * K, FRotator(55.f, Side * 18.f, 0.f), FVector(9.f, 9.f, 50.f) * K, HideMID);
		AddPart(HeadRoot, TEXT("Cone"), FVector(-600.f, Side * 2100.f, 1300.f) * K, FRotator(70.f, Side * 40.f, 0.f), FVector(7.f, 7.f, 34.f) * K, HideMID);
		for (int32 t = 0; t < 6; ++t)
		{
			AddPart(HeadRoot, TEXT("Cone"), FVector(3200.f + t * 900.f, Side * (1250.f - t * 150.f), -350.f) * K, FRotator(180.f, 0.f, 0.f), FVector(3.2f, 3.2f, 9.f - t * 0.6f) * K, nullptr);
		}
	}
	// Lower jaw on a hinge at the back of the skull, so it can drop open. The maw is dark red inside.
	JawRoot = NewObject<USceneComponent>(this, TEXT("JawRoot"));
	JawRoot->SetMobility(EComponentMobility::Movable);
	JawRoot->SetupAttachment(HeadRoot);
	JawRoot->SetRelativeLocation(FVector(600.f, 0.f, -700.f) * K);
	JawRoot->RegisterComponent();
	AddPart(JawRoot, TEXT("Cone"), FVector(4300.f, 0.f, -300.f) * K, FRotator(-90.f, 0.f, 0.f), FVector(24.f, 11.f, 56.f) * K, HideMID);
	AddPart(JawRoot, TEXT("Sphere"), FVector(2800.f, 0.f, 150.f) * K, FRotator::ZeroRotator, FVector(46.f, 20.f, 5.f) * K, MawMID);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 t = 0; t < 5; ++t)
		{
			AddPart(JawRoot, TEXT("Cone"), FVector(3000.f + t * 850.f, Side * (950.f - t * 120.f), 350.f) * K, FRotator::ZeroRotator, FVector(2.8f, 2.8f, 7.5f - t * 0.5f) * K, nullptr);
		}
	}
	// Spines for the neck; placed every frame in UpdateNeck.
	for (int32 i = 0; i < 13; ++i)
	{
		const float S = (16.f - i * 0.5f) * K;
		Spines.Add(AddPart(Mesh, TEXT("Cone"), FVector::ZeroVector, FRotator::ZeroRotator, FVector(S * 0.55f, S * 0.3f, S * 2.2f), HideMID));
	}

	// Mist: where the falls land at the foot of the crater wall, and boiling around the creature itself.
	if (UMaterialInterface* CloudBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud")))
	{
		MistMID = UMaterialInstanceDynamic::Create(CloudBase, this);
		MistRoot = NewObject<USceneComponent>(this, TEXT("MistRoot"));
		MistRoot->SetMobility(EComponentMobility::Movable);
		MistRoot->SetupAttachment(Mesh);
		MistRoot->RegisterComponent();
		FRandomStream MistRng(99);
		const float RimRadius = (Director ? Director->CraterRim - Director->CraterWidth * 1.6f : 100000.f) / BodyScale;
		for (int32 i = 0; i < 84; ++i)
		{
			const float A = 2.f * UE_PI * i / 84.f + MistRng.FRandRange(-0.03f, 0.03f);
			const float Size = MistRng.FRandRange(11000.f, 20000.f);
			UStaticMeshComponent* Puff = AddPart(MistRoot, TEXT("Sphere"), FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (RimRadius + MistRng.FRandRange(-5000.f, 3000.f)) + FVector(0.f, 0.f, MistRng.FRandRange(-2000.f, 5000.f)),
				FRotator::ZeroRotator, FVector(Size / 50.f, Size / 50.f, Size / 85.f), MistMID);
			Puff->SetCastShadow(false);
		}
		for (int32 i = 0; i < 14; ++i)
		{
			const float A = 2.f * UE_PI * i / 14.f;
			const float Size = MistRng.FRandRange(7000.f, 12000.f);
			UStaticMeshComponent* Puff = AddPart(Mesh, TEXT("Sphere"), FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * MoundRadii.X * MistRng.FRandRange(1.1f, 1.7f) + FVector(0.f, 0.f, MistRng.FRandRange(-1500.f, 2500.f)),
				FRotator::ZeroRotator, FVector(Size / 50.f, Size / 50.f, Size / 90.f), MistMID);
			Puff->SetCastShadow(false);
		}
	}

	UpdateNeck(true);
	Mesh->SetMaterial(NeckSection, Skin);
	for (int32 i = 0; i < NumArms; ++i)
	{
		ArmSeeds.Add(FMath::FRandRange(0.f, 50.f));
		UpdateArm(i, true);
		Mesh->SetMaterial(FirstArmSection + i, Skin);
	}

	if (Director) { Director->OnLightning.AddWeakLambda(this, [this]() { EyeFlare = 1.f; }); }
	Mesh->SetVisibility(false, true);
}

void ADCKrakenHead::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { return; }
	if (!bAttached)
	{
		// Ride on the sea so the creature tilts and rocks with the crater it stands in.
		if (USceneComponent* Sea = Director->GetSea())
		{
			AttachToComponent(Sea, FAttachmentTransformRules::KeepRelativeTransform);
			bAttached = true;
		}
	}

	Clock += DeltaSeconds;
	const bool bMaelstrom = Director->GetPhase() == EDCPhase::Maelstrom;
	MaelstromTime = bMaelstrom ? Director->MaelstromElapsed : 0.f;
	const float Target = (bMaelstrom && MaelstromTime > RiseDelay) ? 1.f : 0.f;
	// Taken from the ride clock, so jumping to a time in the ride shows the creature as it would be then.
	Raised = bMaelstrom ? FMath::Clamp((MaelstromTime - RiseDelay) / RiseTime, 0.f, 1.f) : FMath::FInterpConstantTo(Raised, Target, DeltaSeconds * Director->TimeScale, 1.f / RiseTime);

	const bool bVisible = Raised > 0.001f;
	Mesh->SetVisibility(bVisible, true);
	if (!bVisible) { return; }

	// Jaws: slightly parted, thrown wide for each roar.
	float Roaring = 0.f;
	for (float RoarAt : RoarTimes)
	{
		const float Since = MaelstromTime - RoarAt;
		if (Since > -0.4f && Since < 4.5f) { Roaring = FMath::Clamp(FMath::Min((Since + 0.4f) / 0.6f, (4.5f - Since) / 1.2f), 0.f, 1.f); }
	}
	JawOpen = FMath::FInterpTo(JawOpen, Roaring, DeltaSeconds, 4.f);
	if (JawRoot) { JawRoot->SetRelativeRotation(FRotator(-(9.f + 30.f * JawOpen + 2.f * FMath::Sin(Clock * 1.3f)), 0.f, 0.f)); }

	// Rise out of the crater floor, with a slow heave once up. Its face (local +Y) turns to the ship.
	const FVector Floor = Director->GetWhirlFloorLocal();
	const float Lift = BodyScale * (FMath::InterpEaseInOut(-NeckLength * 1.15f, 0.f, Raised, 2.f) + 500.f * FMath::Sin(Clock * 0.35f));
	SetActorRelativeLocation(Floor + FVector(0.f, 0.f, Lift));
	const float FaceYaw = FMath::RadiansToDegrees(FMath::Atan2(-Director->WhirlCenter.Y, -Director->WhirlCenter.X)) - 90.f;
	SetActorRelativeRotation(FRotator(0.f, FaceYaw, 0.f));
	SetActorRelativeScale3D(FVector(BodyScale));

	// Keep the ring of mist at the foot of the wall while the creature itself rises through the floor.
	if (MistRoot && MistMID)
	{
		const float RimRatio = Director->CraterRim / Director->WhirlRadius;
		const float BowlAtRim = Director->GetEnv().Whirl * Director->WhirlDepth * (1.f - 1.f / (1.f + RimRatio * RimRatio));
		MistRoot->SetRelativeLocation(FVector(0.f, 0.f, (BowlAtRim - Lift) / BodyScale));
		const float SunLit = FMath::Clamp(Director->GetEnv().SunIntensity / 9.f, 0.1f, 1.f);
		MistMID->SetVectorParameterValue(TEXT("Top"), FLinearColor(0.8f, 0.88f, 0.95f) * SunLit);
		MistMID->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.5f, 0.62f, 0.72f) * SunLit);
		MistMID->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, 0.6f * Director->GetEnv().Crater));
	}
	UpdateNeck(false);
	for (int32 i = 0; i < NumArms; ++i) { UpdateArm(i, false); }

	// Eyes smoulder, and blaze for a moment in each lightning flash and each roar.
	EyeFlare = FMath::Max(0.f, EyeFlare - DeltaSeconds * 1.5f);
	if (EyeMID)
	{
		const float Pulse = 5.f + 1.5f * FMath::Sin(Clock * 1.7f) + EyeFlare * 10.f + JawOpen * 8.f;
		EyeMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.6f, 0.1f) * Pulse);
	}
}
