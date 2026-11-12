#include "DCHazards.h"
#include "DCTypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimationAsset.h"

namespace
{
	AActor* SpawnEmpty(UWorld* World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Root->SetMobility(EComponentMobility::Movable);
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		return Actor;
	}

	UMaterialInstanceDynamic* Solid(UObject* Outer, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Stone.M_Stone"));
		if (!Base) { Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")); }
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		return MID;
	}

	UStaticMeshComponent* AddShape(AActor* Actor, const TCHAR* Shape, const FVector& Loc, const FRotator& Rot, const FVector& Scale, UMaterialInterface* Mat)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Actor);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMaterial(0, Mat);
		C->SetupAttachment(Actor->GetRootComponent());
		C->SetRelativeTransform(FTransform(Rot, Loc, Scale));
		C->RegisterComponent();
		return C;
	}
}

AActor* DCHazards::SpawnRock(UWorld* World, float Radius, float Height, bool bJagged, const FLinearColor& Color)
{
	AActor* Actor = SpawnEmpty(World);
	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(Actor);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetupAttachment(Actor->GetRootComponent());
	Mesh->RegisterComponent();

	// A column of rings from below the waterline to the top, pushed in and out by 3D noise.
	const int32 Rings = 18, Sides = 20;
	const FVector Seed(FMath::FRandRange(0.f, 500.f), FMath::FRandRange(0.f, 500.f), FMath::FRandRange(0.f, 500.f));
	const FVector2D Lean(FMath::FRandRange(-0.25f, 0.25f), FMath::FRandRange(-0.25f, 0.25f));
	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector2D> UVs;
	for (int32 r = 0; r <= Rings; ++r)
	{
		const float V = static_cast<float>(r) / Rings;
		const float Z = -400.f + V * (Height + 400.f);
		// Profile: broad foot, narrowing; jagged spires pinch to a point, sea stacks keep a blunt top.
		float Profile = bJagged ? FMath::Pow(1.f - V, 0.8f) : FMath::Lerp(1.f, 0.55f, V) * (V > 0.93f ? (1.f - V) / 0.07f : 1.f);
		Profile = FMath::Max(Profile, 0.02f);
		for (int32 s = 0; s <= Sides; ++s)
		{
			const float A = 2.f * UE_PI * s / Sides;
			const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
			const FVector NP = Seed + Dir * 1.3f + FVector(0.f, 0.f, V * (bJagged ? 3.5f : 2.2f));
			const float N = FMath::PerlinNoise3D(NP) * 0.55f + FMath::PerlinNoise3D(NP * 2.7f) * 0.25f;
			const float R = Radius * Profile * (1.f + N);
			Verts.Add(Dir * R + FVector(Lean.X, Lean.Y, 0.f) * Height * V + FVector(0.f, 0.f, Z));
			UVs.Add(FVector2D(static_cast<float>(s) / Sides * 4.f, V * Height / Radius));
		}
	}
	const int32 Cols = Sides + 1;
	for (int32 r = 0; r < Rings; ++r)
	{
		for (int32 s = 0; s < Sides; ++s)
		{
			const int32 A = r * Cols + s, B = A + Cols;
			Tris.Append({ A, B, A + 1 });
			Tris.Append({ A + 1, B, B + 1 });
		}
	}
	// Smooth normals from the faces around each vertex.
	TArray<FVector> Normals;
	Normals.Init(FVector::ZeroVector, Verts.Num());
	for (int32 i = 0; i < Tris.Num(); i += 3)
	{
		const FVector N = FVector::CrossProduct(Verts[Tris[i + 2]] - Verts[Tris[i]], Verts[Tris[i + 1]] - Verts[Tris[i]]);
		Normals[Tris[i]] += N; Normals[Tris[i + 1]] += N; Normals[Tris[i + 2]] += N;
	}
	for (FVector& N : Normals) { N = N.GetSafeNormal(); }

	Mesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	Mesh->SetMaterial(0, Solid(Actor, Color));
	return Actor;
}

AActor* DCHazards::SpawnRuin(UWorld* World, float Scale, const FLinearColor& Color)
{
	AActor* Actor = SpawnEmpty(World);
	UMaterialInstanceDynamic* Mat = Solid(Actor, Color);
	const float S = Scale;
	if (FMath::RandBool())
	{
		// Broken arch: two piers, one lintel that has slipped.
		AddShape(Actor, TEXT("Cube"), FVector(0.f, -700.f * S, 900.f * S), FRotator(0.f, 0.f, 2.f), FVector(4.f, 4.f, 26.f) * S, Mat);
		AddShape(Actor, TEXT("Cube"), FVector(0.f, 700.f * S, 700.f * S), FRotator(0.f, 0.f, -3.f), FVector(4.f, 4.f, 22.f) * S, Mat);
		AddShape(Actor, TEXT("Cube"), FVector(0.f, -150.f * S, 2100.f * S), FRotator(0.f, 0.f, 9.f), FVector(5.f, 17.f, 4.f) * S, Mat);
	}
	else
	{
		// Colonnade: drowned columns of uneven height, some capped.
		for (int32 i = 0; i < 4; ++i)
		{
			const float H = FMath::FRandRange(10.f, 26.f) * S;
			const FVector Loc((i - 1.5f) * 650.f * S, FMath::FRandRange(-120.f, 120.f) * S, H * 50.f - 300.f);
			AddShape(Actor, TEXT("Cylinder"), Loc, FRotator(FMath::FRandRange(-4.f, 4.f), 0.f, FMath::FRandRange(-4.f, 4.f)), FVector(2.6f * S, 2.6f * S, H), Mat);
			if (FMath::RandBool()) { AddShape(Actor, TEXT("Cube"), Loc + FVector(0.f, 0.f, H * 50.f + 40.f * S), FRotator::ZeroRotator, FVector(3.6f, 3.6f, 0.9f) * S, Mat); }
		}
	}
	return Actor;
}

AActor* DCHazards::SpawnStatue(UWorld* World, float Height, const FLinearColor& Color)
{
	AActor* Actor = SpawnEmpty(World);
	UMaterialInstanceDynamic* Mat = Solid(Actor, Color);
	const float K = Height / 6000.f; // authored at 60 m
	// Robe, torso, shoulders, hooded head, one arm raised toward the ship.
	AddShape(Actor, TEXT("Cone"), FVector(0.f, 0.f, 1700.f) * K, FRotator::ZeroRotator, FVector(17.f, 17.f, 40.f) * K, Mat);
	AddShape(Actor, TEXT("Cylinder"), FVector(0.f, 0.f, 3600.f) * K, FRotator::ZeroRotator, FVector(8.5f, 6.5f, 20.f) * K, Mat);
	AddShape(Actor, TEXT("Sphere"), FVector(0.f, 0.f, 4500.f) * K, FRotator::ZeroRotator, FVector(12.5f, 7.f, 5.f) * K, Mat);
	AddShape(Actor, TEXT("Sphere"), FVector(40.f, 0.f, 5150.f) * K, FRotator::ZeroRotator, FVector(5.5f, 5.f, 6.8f) * K, Mat);
	AddShape(Actor, TEXT("Cone"), FVector(-60.f, 0.f, 5350.f) * K, FRotator(-20.f, 0.f, 0.f), FVector(7.f, 6.4f, 7.f) * K, Mat);
	AddShape(Actor, TEXT("Cylinder"), FVector(650.f, 520.f, 4750.f) * K, FRotator(-62.f, 20.f, 0.f), FVector(2.6f, 2.6f, 17.f) * K, Mat);
	AddShape(Actor, TEXT("Cylinder"), FVector(-150.f, -560.f, 3500.f) * K, FRotator(0.f, 0.f, 6.f), FVector(2.5f, 2.5f, 18.f) * K, Mat);
	// A broken sword or staff in the raised hand.
	AddShape(Actor, TEXT("Cube"), FVector(1400.f, 760.f, 5900.f) * K, FRotator(8.f, 20.f, 0.f), FVector(0.7f, 1.6f, 15.f) * K, Mat);
	return Actor;
}

AActor* DCHazards::SpawnGhostShip(UWorld* World, const FLinearColor& Glow)
{
	AActor* Actor = SpawnEmpty(World);
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Ghost.M_Ghost"));
	UMaterialInstanceDynamic* Mat = Base ? UMaterialInstanceDynamic::Create(Base, Actor) : Solid(Actor, Glow);
	Mat->SetVectorParameterValue(TEXT("Glow"), Glow);

	auto AddStatic = [&](const TCHAR* Path)
	{
		if (UStaticMesh* SM = LoadObject<UStaticMesh>(nullptr, Path))
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Actor);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(SM);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			for (int32 i = 0; i < SM->GetStaticMaterials().Num(); ++i) { C->SetMaterial(i, Mat); }
			C->SetupAttachment(Actor->GetRootComponent());
			C->RegisterComponent();
		}
	};
	AddStatic(TEXT("/Game/Brigantine/Mesh/Ship/Hull/SM_Hull-brigantine.SM_Hull-brigantine"));
	AddStatic(TEXT("/Game/Brigantine/Mesh/Ship/Hull/SM_Root-brigantine.SM_Root-brigantine"));

	if (USkeletalMesh* Mast = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Brigantine/Mesh/Ship/Mast/SK_Mast-brigantine.SK_Mast-brigantine")))
	{
		USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(Actor);
		C->SetMobility(EComponentMobility::Movable);
		C->SetSkeletalMeshAsset(Mast);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		for (int32 i = 0; i < Mast->GetMaterials().Num(); ++i) { C->SetMaterial(i, Mat); }
		C->SetupAttachment(Actor->GetRootComponent());
		C->RegisterComponent();
	}
	return Actor;
}

void DCHazards::SetGlow(AActor* Actor, const FLinearColor& Glow)
{
	if (!Actor) { return; }
	TArray<UPrimitiveComponent*> Prims;
	Actor->GetComponents(Prims);
	for (UPrimitiveComponent* P : Prims)
	{
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(P->GetMaterial(0)))
		{
			MID->SetVectorParameterValue(TEXT("Glow"), Glow);
		}
	}
}

AActor* DCHazards::SpawnSpecter(UWorld* World, float Height, const FLinearColor& Glow, bool bWading)
{
	// A see-through giant, walking on (or wading in) the sea. Uses the mannequin that ships with the
	// vessel asset, in the ghost material, so it reads as a figure and nothing more.
	AActor* Actor = SpawnEmpty(World);
	USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Brigantine/Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (!Body) { return Actor; }

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Ghost.M_Ghost"));
	UMaterialInstanceDynamic* Mat = Base ? UMaterialInstanceDynamic::Create(Base, Actor) : nullptr;
	if (Mat) { Mat->SetVectorParameterValue(TEXT("Glow"), Glow); }

	USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(Actor);
	C->SetMobility(EComponentMobility::Movable);
	C->SetSkeletalMeshAsset(Body);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	if (Mat) { for (int32 i = 0; i < Body->GetMaterials().Num(); ++i) { C->SetMaterial(i, Mat); } }
	C->SetupAttachment(Actor->GetRootComponent());
	const float Scale = Height / 180.f;
	// The mannequin faces +Y; turn it to face +X so the caller's yaw means what it says. Waders stand hip-deep.
	C->SetRelativeTransform(FTransform(FRotator(0.f, -90.f, 0.f), FVector(0.f, 0.f, bWading ? -Height * 0.45f : 0.f), FVector(Scale)));
	C->RegisterComponent();
	C->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	if (UAnimationAsset* Walk = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Brigantine/Demo/Characters/Mannequins/Animations/Manny/MM_Walk_InPlace.MM_Walk_InPlace")))
	{
		C->PlayAnimation(Walk, true);
		// Giants move slowly.
		C->SetPlayRate(0.35f);
	}
	return Actor;
}

// ---------------------------------------------------------------- Atlantis

AActor* DCHazards::SpawnSentinel(UWorld* World, float Height, bool bTrident, const FLinearColor& Color)
{
	// A colossal guardian: robed, helmeted, one hand on a spear (or a crowned sea-king with a trident),
	// shield at the other side. Authored at 100 m; faces +X.
	AActor* Actor = SpawnEmpty(World);
	UMaterialInstanceDynamic* Mat = Solid(Actor, Color);
	const float K = Height / 10000.f;
	auto Part = [&](const TCHAR* Shape, const FVector& Loc, const FRotator& Rot, const FVector& Scale) { AddShape(Actor, Shape, Loc * K, Rot, Scale * K, Mat); };

	Part(TEXT("Cone"), FVector(0.f, 0.f, 2500.f), FRotator::ZeroRotator, FVector(30.f, 25.f, 54.f));          // robe
	Part(TEXT("Cube"), FVector(0.f, 0.f, 150.f), FRotator::ZeroRotator, FVector(36.f, 32.f, 4.f));            // plinth
	Part(TEXT("Cylinder"), FVector(0.f, 0.f, 5900.f), FRotator::ZeroRotator, FVector(21.f, 14.5f, 27.f));     // torso
	Part(TEXT("Sphere"), FVector(0.f, 0.f, 7250.f), FRotator::ZeroRotator, FVector(33.f, 16.f, 10.f));         // shoulders
	Part(TEXT("Cylinder"), FVector(0.f, 0.f, 7850.f), FRotator::ZeroRotator, FVector(5.5f, 5.5f, 5.f));       // neck
	Part(TEXT("Sphere"), FVector(60.f, 0.f, 8480.f), FRotator::ZeroRotator, FVector(8.2f, 7.6f, 9.6f));       // head
	// Spear arm: upper arm down from the shoulder, forearm reaching forward to grip the shaft.
	Part(TEXT("Cylinder"), FVector(150.f, 1650.f, 6350.f), FRotator(12.f, 0.f, 0.f), FVector(5.5f, 5.5f, 19.f));
	Part(TEXT("Cylinder"), FVector(800.f, 1650.f, 5450.f), FRotator(62.f, 0.f, 0.f), FVector(3.6f, 3.6f, 15.f));
	Part(TEXT("Cylinder"), FVector(1450.f, 1700.f, 5600.f), FRotator::ZeroRotator, FVector(1.3f, 1.3f, 110.f)); // shaft
	// Shield arm.
	Part(TEXT("Cylinder"), FVector(0.f, -1650.f, 6000.f), FRotator(0.f, 0.f, -5.f), FVector(5.5f, 5.5f, 24.f));
	Part(TEXT("Cylinder"), FVector(500.f, -1500.f, 5000.f), FRotator(0.f, 0.f, 90.f), FVector(22.f, 22.f, 2.5f));

	if (bTrident)
	{
		// Trident head and a crown.
		Part(TEXT("Cube"), FVector(1450.f, 1700.f, 11150.f), FRotator::ZeroRotator, FVector(1.3f, 13.f, 1.5f));
		for (int32 i = -1; i <= 1; ++i) { Part(TEXT("Cone"), FVector(1450.f, 1700.f + i * 600.f, 11750.f), FRotator::ZeroRotator, FVector(2.2f, 2.2f, 11.f)); }
		for (int32 i = 0; i < 7; ++i)
		{
			const float A = 2.f * UE_PI * i / 7.f;
			Part(TEXT("Cone"), FVector(60.f + 330.f * FMath::Cos(A), 330.f * FMath::Sin(A), 9150.f), FRotator::ZeroRotator, FVector(1.6f, 1.6f, 5.f));
		}
		Part(TEXT("Cone"), FVector(420.f, 0.f, 7700.f), FRotator(180.f, 0.f, 0.f), FVector(6.f, 5.f, 12.f));   // beard
	}
	else
	{
		// Spear point, and a crested helmet.
		Part(TEXT("Cone"), FVector(1450.f, 1700.f, 11500.f), FRotator::ZeroRotator, FVector(4.f, 4.f, 10.f));
		Part(TEXT("Cone"), FVector(60.f, 0.f, 9000.f), FRotator::ZeroRotator, FVector(9.4f, 9.f, 6.5f));
		Part(TEXT("Cube"), FVector(-120.f, 0.f, 9250.f), FRotator(-18.f, 0.f, 0.f), FVector(11.f, 0.9f, 5.5f)); // crest
	}
	return Actor;
}

AActor* DCHazards::SpawnAqueduct(UWorld* World, int32 Arches, float Height, const FLinearColor& Color)
{
	// A run of arches on tall piers with a smaller arcade on top, broken off at one end.
	// Runs along local +X from the origin; Height in cm to the top of the lower tier.
	AActor* Actor = SpawnEmpty(World);
	UMaterialInstanceDynamic* Mat = Solid(Actor, Color);
	const float Span = Height * 0.62f;
	const float Pier = Span * 0.2f;
	const int32 Missing = FMath::RandRange(Arches / 2, Arches - 2); // one span has fallen
	for (int32 i = 0; i <= Arches; ++i)
	{
		const float X = i * Span;
		const bool bStump = i == Arches;   // the last pier is a broken stump
		const float H = bStump ? Height * FMath::FRandRange(0.35f, 0.6f) : Height;
		AddShape(Actor, TEXT("Cube"), FVector(X, 0.f, H * 0.5f - 300.f), FRotator(0.f, 0.f, bStump ? 4.f : 0.f), FVector(Pier / 100.f, Pier * 1.5f / 100.f, H / 100.f), Mat);
		if (i == Arches || i == Missing) { continue; }
		// Deck over the span, with angled haunches under it so the opening reads as an arch.
		AddShape(Actor, TEXT("Cube"), FVector(X + Span * 0.5f, 0.f, Height - Height * 0.06f), FRotator::ZeroRotator, FVector(Span / 100.f, Pier * 1.6f / 100.f, Height * 0.12f / 100.f), Mat);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddShape(Actor, TEXT("Cube"), FVector(X + Span * 0.5f + Side * Span * 0.3f, 0.f, Height * 0.82f), FRotator(Side * 38.f, 0.f, 0.f),
				FVector(Span * 0.3f / 100.f, Pier * 1.45f / 100.f, Height * 0.1f / 100.f), Mat);
		}
		// Upper arcade: two small openings per span.
		for (int32 j = 0; j < 3; ++j)
		{
			AddShape(Actor, TEXT("Cube"), FVector(X + j * Span * 0.5f, 0.f, Height * 1.14f), FRotator::ZeroRotator, FVector(Pier * 0.6f / 100.f, Pier * 1.2f / 100.f, Height * 0.28f / 100.f), Mat);
		}
		AddShape(Actor, TEXT("Cube"), FVector(X + Span * 0.5f, 0.f, Height * 1.3f), FRotator::ZeroRotator, FVector(Span / 100.f, Pier * 1.3f / 100.f, Height * 0.05f / 100.f), Mat);
	}
	return Actor;
}

AActor* DCHazards::SpawnTemple(UWorld* World, float Scale, const FLinearColor& Color)
{
	// The high temple: stepped terraces rising out of the sea, a colonnade on each step, waterfalls
	// pouring down the faces, towers at the corners, and a domed, pillared hall on top.
	// Authored about 700 m wide and 380 m tall; front faces -X (toward a ship approaching along +X).
	AActor* Actor = SpawnEmpty(World);
	UMaterialInstanceDynamic* Mat = Solid(Actor, Color);
	UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Bolt.M_Bolt"));
	UMaterialInstanceDynamic* Water = GlowBase ? UMaterialInstanceDynamic::Create(GlowBase, Actor) : Mat;
	// Falling water catches what light there is: pale, so it reads through the haze.
	Water->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.25f, 0.3f));
	const float S = Scale;
	auto Part = [&](const TCHAR* Shape, const FVector& Loc, const FRotator& Rot, const FVector& Sc, UMaterialInterface* M) { AddShape(Actor, Shape, Loc * S, Rot, Sc * S, M); };

	const int32 Tiers = 5;
	float Z = -1000.f;
	for (int32 t = 0; t < Tiers; ++t)
	{
		const float W = FMath::Lerp(70000.f, 26000.f, static_cast<float>(t) / (Tiers - 1));
		const float D = W * 0.7f;
		const float H = t == 0 ? 9000.f : 6000.f;
		Part(TEXT("Cube"), FVector(0.f, 0.f, Z + H * 0.5f), FRotator::ZeroRotator, FVector(D / 100.f, W / 100.f, H / 100.f), Mat);
		// Colonnade along the front edge of this step.
		const int32 Cols = 9 + (Tiers - t) * 2;
		for (int32 c = 0; c < Cols; ++c)
		{
			const float Y = (static_cast<float>(c) / (Cols - 1) - 0.5f) * W * 0.9f;
			Part(TEXT("Cylinder"), FVector(-D * 0.5f + 900.f, Y, Z + H + 1500.f), FRotator::ZeroRotator, FVector(5.f, 5.f, 30.f), Mat);
		}
		Part(TEXT("Cube"), FVector(-D * 0.5f + 900.f, 0.f, Z + H + 3150.f), FRotator::ZeroRotator, FVector(9.f, W * 0.93f / 100.f, 3.f), Mat);
		// Waterfalls down the front face.
		for (int32 f = 0; f < 3; ++f)
		{
			const float Y = (f - 1) * W * 0.28f + FMath::FRandRange(-0.05f, 0.05f) * W;
			Part(TEXT("Cube"), FVector(-D * 0.5f - 120.f, Y, Z + H * 0.5f), FRotator::ZeroRotator, FVector(1.f, W * 0.035f / 100.f, H / 100.f), Water);
		}
		Z += H;
	}
	// The hall on top: a block, pillars across the front, a pediment and a dome.
	Part(TEXT("Cube"), FVector(1500.f, 0.f, Z + 3500.f), FRotator::ZeroRotator, FVector(110.f, 190.f, 70.f), Mat);
	for (int32 c = 0; c < 8; ++c)
	{
		Part(TEXT("Cylinder"), FVector(-4800.f, (c - 3.5f) * 2500.f, Z + 3000.f), FRotator::ZeroRotator, FVector(7.f, 7.f, 60.f), Mat);
	}
	Part(TEXT("Cone"), FVector(-4300.f, 0.f, Z + 7400.f), FRotator::ZeroRotator, FVector(30.f, 215.f, 30.f), Mat);       // pediment
	Part(TEXT("Sphere"), FVector(2500.f, 0.f, Z + 7600.f), FRotator::ZeroRotator, FVector(90.f, 90.f, 70.f), Mat);       // dome
	Part(TEXT("Cone"), FVector(2500.f, 0.f, Z + 11800.f), FRotator::ZeroRotator, FVector(8.f, 8.f, 30.f), Mat);          // finial
	// Corner towers.
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		Part(TEXT("Cylinder"), FVector(-14000.f, Side * 39000.f, 9000.f), FRotator::ZeroRotator, FVector(40.f, 40.f, 200.f), Mat);
		Part(TEXT("Cone"), FVector(-14000.f, Side * 39000.f, 21000.f), FRotator::ZeroRotator, FVector(46.f, 46.f, 50.f), Mat);
	}
	return Actor;
}
