#include "DCRuins.h"
#include "DCFogRouteLayout.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace
{
	UStaticMesh* RuinMesh(const TCHAR* Name)
	{
		// Name is "Folder/SM_Name" under /Game/AncientRuins/Meshes.
		const FString Path = FString::Printf(TEXT("/Game/AncientRuins/Meshes/%s.%s"), Name, *FPaths::GetBaseFilename(Name));
		return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	AActor* RuinActor(UWorld* World)
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

	/**
	 * Adds a mesh so that the centre of the bottom of its bounding box sits at Where (cm, in the
	 * structure's own space), whatever the mesh's pivot is. Returns the size of its box after scaling.
	 */
	FVector RuinPiece(AActor* Actor, const TCHAR* Name, const FVector& Where, float Yaw = 0.f, const FVector& Scale = FVector(1.f), float Pitch = 0.f, float Roll = 0.f, FName Tag = NAME_None)
	{
		UStaticMesh* M = RuinMesh(Name);
		if (!M) { return FVector::ZeroVector; }
		const FBox Box = M->GetBoundingBox();
		const FVector Foot = FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z) * Scale;
		const FRotator Rot(Pitch, Yaw, Roll);
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Actor);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(M);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(Actor->GetRootComponent());
		if(!Tag.IsNone()) { C->ComponentTags.Add(Tag); }
		if (FParse::Param(FCommandLine::Get(), TEXT("DCNoRuinShadows"))) { C->SetCastShadow(false); }
		C->SetRelativeTransform(FTransform(Rot, Where - Rot.RotateVector(Foot), Scale));
		// The wash line: foam lapping at the stone where it stands in the sea, and wet stone above it.
		static UMaterialInterface* Waterline = !FParse::Param(FCommandLine::Get(), TEXT("DCWaterline")) ? nullptr : LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Waterline.M_Waterline"));
		if (Waterline) { C->SetOverlayMaterial(Waterline); /* an overlay cannot be drawn on a Nanite mesh */ C->SetForceDisableNanite(true); }
		C->RegisterComponent();
		return Box.GetSize() * Scale;
	}
}

bool DCRuins::Available()
{
	static const bool bAvailable = !FParse::Param(FCommandLine::Get(), TEXT("DCNoRuins")) && RuinMesh(TEXT("Columns/SM_Colonnade_01")) != nullptr;
	return bAvailable;
}

AActor* DCRuins::SpawnColossus(UWorld* World, float Height)
{
	AActor* A = RuinActor(World);
	A->Tags.Add(TEXT("DCRuinArchitecture"));
	// A stack of masonry for a plinth, then the pedestal, then the figure: about 21 m at scale 1.
	RuinPiece(A, TEXT("Stone_blocks/SM_Stone_blocks_04_block_01"), FVector(0.f, 0.f, -300.f), 0.f, FVector(3.2f, 3.f, 3.f));
	RuinPiece(A, TEXT("Arch/SM_Pedestal_01"), FVector(0.f, 0.f, 200.f), 0.f, FVector(1.6f, 0.9f, 1.4f));
	RuinPiece(A, TEXT("Arch/SM_Statue_01"), FVector(0.f, 0.f, 600.f), 0.f, FVector(2.6f));
	A->SetActorScale3D(FVector(Height / 2100.f));
	return A;
}

AActor* DCRuins::SpawnColonnade(UWorld* World, int32 Segments, float Height)
{
	AActor* A = RuinActor(World);
	A->Tags.Add(TEXT("DCRuinArchitecture"));
	static const TCHAR* Kinds[] = { TEXT("Columns/SM_Colonnade_01"), TEXT("Columns/SM_Colonnade_02"), TEXT("Columns/SM_Colonnade_03"), TEXT("Columns/SM_Colonnade_05") };
	const float Step = 1230.f;
	const float Start = -Step * (Segments - 1) * 0.5f;
	for (int32 i = 0; i < Segments; ++i)
	{
		// One span in five has fallen: a stump and a drum lying in the water mark where it stood.
		if (i > 0 && i < Segments - 1 && FMath::RandRange(0, 4) == 0)
		{
			RuinPiece(A, TEXT("Columns/SM_Column_01_dmg_03"), FVector(Start + i * Step - 400.f, 0.f, -100.f));
			RuinPiece(A, TEXT("Columns/SM_Column_01_dmg_07"), FVector(Start + i * Step + 250.f, 300.f, -60.f), FMath::FRandRange(0.f, 360.f));
			continue;
		}
		RuinPiece(A, Kinds[FMath::RandRange(0, 3)], FVector(Start + i * Step, 0.f, -80.f));
	}
	// Broken ends.
	RuinPiece(A, TEXT("Columns/SM_Column_01_dmg_05"), FVector(Start - Step * 0.75f, 0.f, -100.f));
	RuinPiece(A, TEXT("Columns/SM_Column_01_dmg_04"), FVector(-Start + Step * 0.75f, 0.f, -100.f));
	A->SetActorScale3D(FVector(Height / 1540.f));
	return A;
}

namespace
{
	/**
	 * A floor of stone blocks W by D (cm, the structure's own space) and High thick, its top course
	 * laid as many blocks side by side, each near its own proportions. One block stretched to the
	 * whole size, as these bases were first built, drew its face out a hundredfold: from the deck a
	 * temple's base was a smooth streaked sheet (owner, 13 Nov: "deformed band"). One component, many instances.
	 */
	void RuinFloor(AActor* Actor, const FVector& Where, float W, float D, float High, float TileScale = 1.6f)
	{
		UStaticMesh* M = RuinMesh(TEXT("Stone_blocks/SM_Stone_blocks_04_block_01"));
		if (!M) { return; }
		const FBox Box = M->GetBoundingBox();
		const FVector Size = Box.GetSize();
		const float K = TileScale; // Bank blocks are coarser than temple paving, with bounded instance counts.
		const int32 NX = FMath::Max(1, FMath::RoundToInt32(W / (Size.X * K))), NY = FMath::Max(1, FMath::RoundToInt32(D / (Size.Y * K)));
		const FVector S(W / NX / Size.X, D / NY / Size.Y, High / Size.Z);
		// What the hull feels is one unseen block of the whole size (as before); the many blocks are only looked at.
		// (With every block solid, moving them cost 12 ms a frame and the helmsman struck the base 30 times a voyage.)
		{
			const FVector Whole(W / Size.X, D / Size.Y, High / Size.Z);
			UStaticMeshComponent* Solid = NewObject<UStaticMeshComponent>(Actor);
			Solid->SetMobility(EComponentMobility::Movable);
			Solid->SetStaticMesh(M);
			Solid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Solid->SetVisibility(false);
			Solid->SetCastShadow(false);
			Solid->SetupAttachment(Actor->GetRootComponent());
			Solid->SetRelativeTransform(FTransform(FRotator::ZeroRotator, Where - FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z) * Whole, Whole));
			Solid->RegisterComponent();
		}
		UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(Actor);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(M);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(Actor->GetRootComponent());
		C->RegisterComponent();
		FRandomStream Rng(static_cast<int32>(W + D * 3.f + Where.Z));
		for (int32 x = 0; x < NX; ++x)
		{
			for (int32 y = 0; y < NY; ++y)
			{
				// Turned end for end at random and set a hair high or low, so the courses do not read as a printed grid.
				const bool bTurned = Rng.FRand() < 0.5f;
				const FRotator Rot(0.f, bTurned ? 180.f : 0.f, 0.f);
				const FVector Centre(Where.X + (x + 0.5f - NX * 0.5f) * W / NX, Where.Y + (y + 0.5f - NY * 0.5f) * D / NY, Where.Z + Rng.FRandRange(-6.f, 6.f));
				const FVector Foot = FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z) * S;
				C->AddInstance(FTransform(Rot, Centre - Rot.RotateVector(Foot), S));
			}
		}
	}
}

AActor* DCRuins::SpawnTemple(UWorld* World, float Scale)
{
	AActor* A = RuinActor(World);
	A->Tags.Add(TEXT("DCRuinArchitecture"));
	const float Step = 1230.f;
	// Stepped base of big blocks, wider than the building.
	for (int32 s = 0; s < 3; ++s)
	{
		const float W = 4400.f - s * 300.f, D = 3000.f - s * 300.f;
		RuinFloor(A, FVector(0.f, 0.f, -350.f + s * 150.f), W, D, 150.f);
	}
	const float Floor = 100.f;
	for (int32 Row = -1; Row <= 1; Row += 2)
	{
		for (int32 i = -1; i <= 1; ++i)
		{
			RuinPiece(A, i == 0 ? TEXT("Columns/SM_Colonnade_02") : TEXT("Columns/SM_Colonnade_01"), FVector(i * Step, Row * 1000.f, Floor));
		}
		// The pediment across the whole front.
		RuinPiece(A, TEXT("Stone_blocks/SM_Fronton_01"), FVector(0.f, Row * 1000.f, Floor + 1530.f), 0.f, FVector(3.f, 1.f, 3.f));
	}
	// Side colonnades joining front to back, and the walled cella inside.
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		RuinPiece(A, TEXT("Columns/SM_Colonnade_03"), FVector(Side * 1750.f, 0.f, Floor), 90.f, FVector(1.35f, 1.f, 1.f));
		RuinPiece(A, TEXT("Wall_sections/SM_Wall_section_01"), FVector(Side * 550.f, 0.f, Floor), 90.f, FVector(1.3f, 1.f, 1.5f));
	}
	RuinPiece(A, TEXT("Arch/SM_Statue_01"), FVector(0.f, 0.f, Floor), 0.f, FVector(2.2f));
	A->SetActorScale3D(FVector(Scale));
	return A;
}

AActor* DCRuins::SpawnArch(UWorld* World, int32 Variant, float Scale)
{
	AActor* A = RuinActor(World);
	A->Tags.Add(TEXT("DCRuinArchitecture"));
	static const TCHAR* Kinds[] = { TEXT("Arch/SM_Arch_01"), TEXT("Arch/SM_Arch_02"), TEXT("Arch/SM_Arch_03"), TEXT("Arch/SM_Arch_04") };
	RuinPiece(A, Kinds[FMath::Clamp(Variant, 1, 4) - 1], FVector(0.f, 0.f, -150.f));
	A->SetActorScale3D(FVector(Scale));
	return A;
}

AActor* DCRuins::SpawnGate(UWorld* World, float Scale)
{
	AActor* A = RuinActor(World);
	// Two towers 36 m apart, a span of entablature between them, a figure on each tower.
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		RuinPiece(A, TEXT("Arch/SM_Pillar_wall_01"), FVector(Side * 1800.f, 0.f, -200.f), 0.f, FVector(2.f, 1.2f, 1.7f));
		RuinPiece(A, TEXT("Arch/SM_Statue_01"), FVector(Side * 1800.f, 0.f, 2450.f), 0.f, FVector(1.5f));
		RuinPiece(A, TEXT("Arch/SM_Pilaster_01"), FVector(Side * 1150.f, 0.f, -200.f), 0.f, FVector(1.f, 1.f, 1.f));
	}
	RuinPiece(A, TEXT("Columns/SM_Antablement_01"), FVector(0.f, 0.f, 2400.f), 0.f, FVector(2.4f, 2.f, 1.4f));
	RuinPiece(A, TEXT("Stone_blocks/SM_Fronton_01"), FVector(0.f, 0.f, 2880.f), 0.f, FVector(2.3f, 1.1f, 2.f));
	A->SetActorScale3D(FVector(Scale));
	return A;
}

AActor* DCRuins::SpawnBrokenColumns(UWorld* World, float Radius)
{
	AActor* A = RuinActor(World);
	static const TCHAR* Stumps[] = { TEXT("Columns/SM_Column_01_dmg_04"), TEXT("Columns/SM_Column_01_dmg_05"), TEXT("Columns/SM_Column_01_dmg_03"), TEXT("Columns/SM_Column_01_01") };
	static const TCHAR* Fallen[] = { TEXT("Columns/SM_Column_01_dmg_07"), TEXT("Columns/SM_Column_01_dmg_08"), TEXT("Stone_blocks/SM_Stone_blocks_04_block_03"), TEXT("Stone_blocks/SM_Stone_blocks_03_block_01") };
	// Built about 9 m across at scale 1.
	const int32 Count = FMath::RandRange(2, 3);
	for (int32 i = 0; i < Count; ++i)
	{
		const float Ang = 2.f * UE_PI * i / Count + FMath::FRandRange(-0.4f, 0.4f);
		const FVector At(FMath::Cos(Ang) * 280.f, FMath::Sin(Ang) * 280.f, -120.f);
		// Keep these vertical so fitting their visible height later cannot widen
		// the already measured collision footprint or lean over the clear lane.
		RuinPiece(A, Stumps[FMath::RandRange(0, 3)], At, FMath::FRandRange(0.f, 360.f), FVector(1.f), 0.f, 0.f, TEXT("DCPillarUpright"));
	}
	for (int32 i = 0; i < 3; ++i)
	{
		RuinPiece(A, Fallen[FMath::RandRange(0, 3)], FVector(FMath::FRandRange(-380.f, 380.f), FMath::FRandRange(-380.f, 380.f), -90.f), FMath::FRandRange(0.f, 360.f), FVector(1.f), FMath::FRandRange(-12.f, 12.f), 0.f);
	}
	A->SetActorScale3D(FVector(Radius / 450.f));
	return A;
}

AActor* DCRuins::SpawnWaterComposition(UWorld* World, int32 Variant)
{
    AActor* A=RuinActor(World);
    // All pieces are bottom-anchored. Registration measures/fits the complete
    // rotated assembly to the existing 15m hazard envelope before water contact.
    const auto Piece=[A](const TCHAR* Name,FVector At,float Width,float Height,float Yaw=0.f,bool Foot=true)
    {
        if(UStaticMesh* Mesh=RuinMesh(Name))
        {
            const FVector Size=Mesh->GetBoundingBox().GetSize();
            const float XY=Width/FMath::Max(FMath::Max(Size.X,Size.Y),1.0);
            RuinPiece(A,Name,At,Yaw,FVector(XY,XY,Height/FMath::Max(Size.Z,1.0)),0.f,0.f,
                Foot?FName(TEXT("DCPillarUpright")):NAME_None);
        }
    };
    switch(Variant%5)
    {
    case 0: // A crescent of unequal broken columns, not the same two posts.
        Piece(TEXT("Columns/SM_Column_01_dmg_04"),FVector(-760,-400,-180),520,2800,15);
        Piece(TEXT("Columns/SM_Column_01_01"),FVector(100,350,-180),650,4100,48);
        Piece(TEXT("Columns/SM_Column_01_dmg_05"),FVector(760,-360,-180),560,3300,-20);
        break;
    case 1: // Low/wide reef with three distinct peaks.
        Piece(TEXT("Rocks/SM_Rock_01"),FVector(-650,-300,-400),1450,3000,25);
        Piece(TEXT("Cliffs/SM_Cliff_01"),FVector(300,340,-500),1550,4300,-28);
        Piece(TEXT("Rocks/SM_Rock_01"),FVector(800,-500,-400),850,2700,115);
        break;
    case 2: // Sunken arch and its broken neighbouring pier.
        Piece(TEXT("Arch/SM_Arch_04"),FVector(-200,250,-300),2200,4400,18);
        Piece(TEXT("Columns/SM_Column_01_dmg_03"),FVector(700,-850,-180),600,2900,-35);
        break;
    case 3: // Split rock ridge with one remnant of the drowned city.
        Piece(TEXT("Cliffs/SM_Cliff_02"),FVector(-650,150,-450),1200,4500,-32);
        Piece(TEXT("Rocks/SM_Rock_01"),FVector(650,-350,-400),1350,3100,85);
        Piece(TEXT("Columns/SM_Column_01_dmg_05"),FVector(550,600,-180),430,3500,10);
        break;
    default: // Diagonal ruined arcade, tall-to-short with a fallen end.
        Piece(TEXT("Columns/SM_Column_01_01"),FVector(-850,-500,-180),500,4300,0);
        Piece(TEXT("Columns/SM_Column_01_dmg_04"),FVector(-260,-150,-180),560,3600,30);
        Piece(TEXT("Columns/SM_Column_01_dmg_05"),FVector(340,250,-180),540,3000,75);
        Piece(TEXT("Columns/SM_Column_01_dmg_03"),FVector(900,650,-180),450,2700,110);
        break;
    }
    Piece(TEXT("Columns/SM_Column_01_dmg_07"),FVector(150,-250,-260),1300,650,55,false);
    Piece(TEXT("Stone_blocks/SM_Stone_blocks_04_block_03"),FVector(-500,480,-280),950,700,-22,false);
    return A;
}

AActor* DCRuins::SpawnCrag(UWorld* World, float Radius, float Height)
{
	AActor* A = RuinActor(World);
	static const TCHAR* Kinds[] = { TEXT("Cliffs/SM_Cliff_01"), TEXT("Cliffs/SM_Cliff_02"), TEXT("Rocks/SM_Rock_01") };
	const TCHAR* Kind = Kinds[FMath::RandRange(0, 2)];
	if (UStaticMesh* M = RuinMesh(Kind))
	{
		// Scaled so its footprint matches the collision radius and it stands Height tall, with a fifth of it under water.
		const FVector Size = M->GetBoundingBox().GetSize();
		const float Wide = Radius * 2.f / FMath::Max(Size.X, Size.Y);
		RuinPiece(A, Kind, FVector(0.f, 0.f, -Height * 0.2f), 0.f, FVector(Wide, Wide, Height * 1.2f / Size.Z));
	}
	return A;
}

AActor* DCRuins::SpawnComplex(UWorld* World, float Scale, int32 Seed)
{
	// A sanctuary: a stepped platform out of the water carrying a temple at the back, a forecourt
	// walled by colonnades, a monumental arch at the front and statues along the approach. About
	// 110 m wide and 150 m deep at scale 1, facing along its own +X.
	AActor* A = RuinActor(World);
	A->Tags.Add(TEXT("DCRuinArchitecture"));
	FRandomStream Rng(Seed);
	for (int32 s = 0; s < 3; ++s)
	{
		const float W = 15000.f - s * 900.f, D = 11000.f - s * 900.f;
		RuinFloor(A, FVector(0.f, 0.f, -500.f + s * 220.f), W, D, 220.f);
	}
	const float Floor = 160.f;
	// Temple at the back: six spans wide, columns front and back, pediment over each.
	for (int32 Row = 0; Row < 2; ++Row)
	{
		const float X = -5200.f - Row * 2200.f;
		for (int32 i = -2; i <= 2; ++i)
		{
			RuinPiece(A, (i + Row) % 2 ? TEXT("Columns/SM_Colonnade_02") : TEXT("Columns/SM_Colonnade_01"), FVector(X, i * 1230.f, Floor), 90.f);
		}
		RuinPiece(A, TEXT("Stone_blocks/SM_Fronton_01"), FVector(X, 0.f, Floor + 1530.f), 90.f, FVector(4.9f, 1.f, 4.2f));
	}
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		RuinPiece(A, TEXT("Wall_sections/SM_Wall_section_01"), FVector(-6300.f, Side * 2600.f, Floor), 0.f, FVector(1.9f, 1.f, 1.6f));
		// Forecourt colonnades running forward from the temple, with gaps where spans have fallen.
		for (int32 i = 0; i < 6; ++i)
		{
			const FVector At(-3600.f + i * 1230.f, Side * 4300.f, Floor);
			if (Rng.FRand() < 0.22f)
			{
				RuinPiece(A, TEXT("Columns/SM_Column_01_dmg_04"), At + FVector(-350.f, 0.f, 0.f));
				RuinPiece(A, TEXT("Columns/SM_Column_01_dmg_07"), At + FVector(300.f, -Side * 500.f, 0.f), Rng.FRandRange(0.f, 360.f));
			}
			else
			{
				RuinPiece(A, i % 2 ? TEXT("Columns/SM_Colonnade_03") : TEXT("Columns/SM_Colonnade_05"), At);
			}
		}
		// Statues along the approach, some broken.
		for (int32 i = 0; i < 3; ++i)
		{
			static const TCHAR* Figures[] = { TEXT("Arch/SM_Statue_01"), TEXT("Arch/SM_Statue_01_dmg_03"), TEXT("Arch/SM_Statue_01"), TEXT("Arch/SM_Statue_01_dmg_02") };
			const FVector At(-1800.f + i * 2300.f, Side * 2100.f, Floor);
			RuinPiece(A, TEXT("Arch/SM_Pedestal_01"), At, 90.f);
			RuinPiece(A, Figures[Rng.RandRange(0, 3)], At + FVector(0.f, 0.f, 300.f), Side > 0 ? -90.f : 90.f, FVector(1.5f));
		}
		RuinPiece(A, TEXT("Arch/SM_Pillar_wall_01"), FVector(5600.f, Side * 3300.f, Floor), 90.f, FVector(1.f, 1.f, 1.2f));
	}
	// The monumental arch at the front of the court, and the cult statue inside the temple.
	static const TCHAR* Arches[] = { TEXT("Arch/SM_Arch_01"), TEXT("Arch/SM_Arch_02"), TEXT("Arch/SM_Arch_03"), TEXT("Arch/SM_Arch_04") };
	RuinPiece(A, Arches[Rng.RandRange(0, 3)], FVector(5200.f, 0.f, Floor), 90.f, FVector(1.5f));
	RuinPiece(A, TEXT("Arch/SM_Statue_01"), FVector(-6200.f, 0.f, Floor), 90.f, FVector(2.6f));
	// Fallen masonry in the water around the foot of the platform.
	for (int32 i = 0; i < 8; ++i)
	{
		const float Ang = Rng.FRandRange(0.f, 2.f * UE_PI);
		static const TCHAR* Debris[] = { TEXT("Columns/SM_Column_01_dmg_07"), TEXT("Columns/SM_Column_01_dmg_08"), TEXT("Stone_blocks/SM_Stone_blocks_04_block_03"), TEXT("Columns/SM_Column_01_dmg_03") };
		RuinPiece(A, Debris[Rng.RandRange(0, 3)], FVector(FMath::Cos(Ang) * 8200.f, FMath::Sin(Ang) * 6300.f, -450.f), Rng.FRandRange(0.f, 360.f), FVector(Rng.FRandRange(1.f, 1.8f)), Rng.FRandRange(-10.f, 10.f));
	}
	A->SetActorScale3D(FVector(Scale));
	return A;
}

AActor* DCRuins::SpawnBank(UWorld* World, float Length, float Width, float Bottom, float Top, float Along, float Side)
{
	AActor* A = RuinActor(World);
	A->Tags.Add(TEXT("DCCityBank"));
    // The old safety fix made both shorelines ruler-straight. Keep its same
    // supported walking bench, but recess the spare inboard edge and extend
    // the outer terrace asymmetrically. Each visible slab has its matching proxy.
    constexpr int32 Slices=8;
    const float Step=Length/Slices;
    for(int32 I=0;I<Slices;++I)
    {
        const float X=-Length*.5f+(I+.5f)*Step;
        const float Inset=DCFogRouteLayout::ShoreInset(Along+X,Side);
        const float Outer=DCFogRouteLayout::ShoreOuterExtension(Along+X,Side);
        RuinFloor(A,FVector(X,Side*(Inset+Outer)*.5f,Bottom),Step,Width-Inset+Outer,Top-Bottom,8.f);
    }
	return A;
}
