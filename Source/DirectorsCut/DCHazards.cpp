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
