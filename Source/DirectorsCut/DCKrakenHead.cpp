#include "DCKrakenHead.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const int32 ArmRings = 28;
	const int32 ArmSides = 10;
	// Everything below is in the head's own space: origin on the funnel floor, +Y toward the ship.
	const FVector MantleRadii(15500.f, 13000.f, 21000.f);
}

ADCKrakenHead::ADCKrakenHead()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADCKrakenHead::BuildMantle()
{
	// A tall lumpy dome, pinched toward the crown, with brow ridges over the eyes.
	const int32 Rings = 28, Sides = 36;
	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	for (int32 r = 0; r <= Rings; ++r)
	{
		const float V = static_cast<float>(r) / Rings;
		const float Phi = V * UE_PI * 0.62f; // from crown down past the equator
		for (int32 s = 0; s <= Sides; ++s)
		{
			const float A = 2.f * UE_PI * s / Sides;
			FVector D(FMath::Sin(Phi) * FMath::Cos(A), FMath::Sin(Phi) * FMath::Sin(A), FMath::Cos(Phi));
			const float N = FMath::PerlinNoise3D(D * 2.3f + FVector(11.f)) * 0.12f + FMath::PerlinNoise3D(D * 6.f) * 0.04f;
			// Brow: a swelling on the side that faces the ship, at eye height.
			const float Brow = FMath::Exp(-FMath::Square((Phi - 1.25f) * 3.f)) * FMath::Max(0.f, FMath::Sin(A)) * 0.12f;
			Verts.Add(FVector(D.X * MantleRadii.X, D.Y * MantleRadii.Y, D.Z * MantleRadii.Z) * (1.f + N + Brow));
			UVs.Add(FVector2D(static_cast<float>(s) / Sides * 20.f, V * 20.f));
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
	// Arms stand in a ring around the mantle, the nearest ones leaning toward the ship.
	const float Angle = 2.f * UE_PI * (Index + 0.5f) / NumArms;
	const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	const FVector Base = Out * FVector(MantleRadii.X, MantleRadii.Y, 0.f).Size() * 1.25f - FVector(0.f, 0.f, 3000.f);
	const float Length = 30000.f + 9000.f * FMath::Sin(Seed * 7.f);
	const float BaseRadius = Length * 0.055f;
	const FVector Side = FVector::CrossProduct(Out, FVector::UpVector);
	const float T = Clock * 0.45f + Seed;

	const int32 Cols = ArmSides + 1;
	TArray<FVector> Spine; Spine.SetNumUninitialized(ArmRings);
	for (int32 r = 0; r < ArmRings; ++r)
	{
		const float U = static_cast<float>(r) / (ArmRings - 1);
		// Up, bowing outward, then curling back in at the tip; slow travelling sway on top.
		FVector P = Base + FVector(0.f, 0.f, Length * U * (1.f - 0.25f * U));
		P += Out * Length * (0.32f * FMath::Sin(U * UE_PI * 0.9f) - 0.28f * U * U);
		P += Side * Length * 0.12f * FMath::Sin(U * 4.f - T * 1.3f) * U;
		P += Out * Length * 0.07f * FMath::Sin(U * 5.f - T * 1.7f + 1.f) * U;
		Spine[r] = P;
	}

	TArray<FVector> Verts; Verts.SetNumUninitialized(ArmRings * Cols);
	TArray<FVector> Normals; Normals.SetNumUninitialized(ArmRings * Cols);
	TArray<FVector2D> UVs; UVs.SetNumUninitialized(ArmRings * Cols);
	TArray<FColor> Colors; Colors.SetNumUninitialized(ArmRings * Cols);
	TArray<FProcMeshTangent> Tangents; Tangents.SetNumUninitialized(ArmRings * Cols);
	FVector Belly = -Out;
	for (int32 r = 0; r < ArmRings; ++r)
	{
		const float U = static_cast<float>(r) / (ArmRings - 1);
		const FVector Tangent = (Spine[FMath::Min(r + 1, ArmRings - 1)] - Spine[FMath::Max(r - 1, 0)]).GetSafeNormal();
		Belly = (Belly - Tangent * FVector::DotProduct(Belly, Tangent)).GetSafeNormal();
		const FVector Bitangent = FVector::CrossProduct(Tangent, Belly);
		const float Radius = FMath::Lerp(BaseRadius, BaseRadius * 0.06f, FMath::Pow(U, 0.8f)) * (r == ArmRings - 1 ? 0.2f : 1.f);
		for (int32 s = 0; s <= ArmSides; ++s)
		{
			const float A = 2.f * UE_PI * s / ArmSides;
			const float CA = FMath::Cos(A), SA = FMath::Sin(A);
			const FVector Dir = Belly * CA + Bitangent * SA;
			const int32 I = r * Cols + s;
			Verts[I] = Spine[r] + Dir * Radius;
			Normals[I] = Dir;
			UVs[I] = FVector2D(U * Length / 4000.f, static_cast<float>(s) / ArmSides);
			Colors[I] = FColor(static_cast<uint8>(FMath::Clamp((CA - 0.25f) / 0.6f, 0.f, 1.f) * 255.f), static_cast<uint8>(FMath::Clamp(0.5f + SA * 0.6f, 0.f, 1.f) * 255.f), static_cast<uint8>(U * 255.f), 255);
			Tangents[I] = FProcMeshTangent(Tangent, false);
		}
	}
	if (bCreate) { Mesh->CreateMeshSection(1 + Index, Verts, ArmTris, Normals, UVs, Colors, Tangents, false); }
	else { Mesh->UpdateMeshSection(1 + Index, Verts, Normals, UVs, Colors, Tangents); }
}

void ADCKrakenHead::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);

	UMaterialInterface* Skin = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Kraken.M_Kraken"));
	BuildMantle();
	Mesh->SetMaterial(0, Skin);

	for (int32 r = 0; r < ArmRings - 1; ++r)
	{
		for (int32 s = 0; s < ArmSides; ++s)
		{
			const int32 A = r * (ArmSides + 1) + s, B = A + ArmSides + 1;
			ArmTris.Append({ A, B, A + 1 });
			ArmTris.Append({ A + 1, B, B + 1 });
		}
	}
	for (int32 i = 0; i < NumArms; ++i)
	{
		ArmSeeds.Add(FMath::FRandRange(0.f, 50.f));
		UpdateArm(i, true);
		Mesh->SetMaterial(1 + i, Skin);
	}

	// Eyes: on the side facing the ship (+Y), under the brow.
	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Glow.M_Glow"));
	if (Glow) { EyeMID = UMaterialInstanceDynamic::Create(Glow, this); }
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	for (int32 i = 0; i < 2; ++i)
	{
		UStaticMeshComponent* Eye = NewObject<UStaticMeshComponent>(this);
		Eye->SetMobility(EComponentMobility::Movable);
		Eye->SetStaticMesh(Sphere);
		Eye->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Eye->SetCastShadow(false);
		if (EyeMID) { Eye->SetMaterial(0, EyeMID); }
		Eye->SetupAttachment(Mesh);
		Eye->SetRelativeLocation(FVector(i == 0 ? -5200.f : 5200.f, MantleRadii.Y * 0.93f, 4200.f));
		Eye->SetRelativeScale3D(FVector(26.f, 12.f, 17.f));
		Eye->RegisterComponent();
		Eyes.Add(Eye);
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
		// Ride on the sea so the head tilts and rocks with the funnel it stands in.
		if (USceneComponent* Sea = Director->GetSea())
		{
			AttachToComponent(Sea, FAttachmentTransformRules::KeepRelativeTransform);
			bAttached = true;
		}
	}

	Clock += DeltaSeconds;
	const bool bMaelstrom = Director->GetPhase() == EDCPhase::Maelstrom;
	MaelstromTime = bMaelstrom ? MaelstromTime + DeltaSeconds * Director->TimeScale : 0.f;
	const float Target = (bMaelstrom && MaelstromTime > RiseDelay) ? 1.f : 0.f;
	Raised = FMath::FInterpConstantTo(Raised, Target, DeltaSeconds * Director->TimeScale, 1.f / RiseTime);

	const bool bVisible = Raised > 0.001f;
	Mesh->SetVisibility(bVisible, true);
	if (!bVisible) { return; }

	// Rise out of the funnel floor, with a slow heave once up.
	const FVector Floor = Director->GetWhirlFloorLocal();
	const float Lift = FMath::InterpEaseInOut(-static_cast<float>(MantleRadii.Z) * 1.05f - 30000.f, 20000.f, Raised, 2.f) + 600.f * FMath::Sin(Clock * 0.6f);
	SetActorRelativeLocation(Floor + FVector(0.f, 0.f, Lift));
	SetActorRelativeRotation(FRotator(3.f * FMath::Sin(Clock * 0.23f), 0.f, 2.f * FMath::Sin(Clock * 0.31f)));

	for (int32 i = 0; i < NumArms; ++i) { UpdateArm(i, false); }

	// Eyes smoulder, and blaze for a moment in each lightning flash.
	EyeFlare = FMath::Max(0.f, EyeFlare - DeltaSeconds * 1.5f);
	if (EyeMID)
	{
		const float Pulse = 3.f + 1.2f * FMath::Sin(Clock * 1.7f) + EyeFlare * 12.f;
		EyeMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.55f, 0.08f) * Pulse);
	}
}
