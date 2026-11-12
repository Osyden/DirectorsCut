#include "DCCloudFace.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCCloudFace::ADCCloudFace()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	Seed = FMath::FRandRange(0.f, 100.f);
}

void ADCCloudFace::Part(const TCHAR* Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale, UMaterialInterface* Material)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	if (Material) { C->SetMaterial(0, Material); }
	C->SetupAttachment(FaceRoot);
	C->SetRelativeTransform(FTransform(Rotation, Location * K, Scale * K));
	C->RegisterComponent();
}

void ADCCloudFace::Build(float Height, bool bBearded, float PitchDown)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud"));
	if (!Base) { return; }
	Light = UMaterialInstanceDynamic::Create(Base, this);
	Dark = UMaterialInstanceDynamic::Create(Base, this);
	Eye = UMaterialInstanceDynamic::Create(Base, this);
	K = Height / 10000.f;
	// Faces need their modelling kept: only a little of the cloud raggedness.
	Light->SetScalarParameterValue(TEXT("Erode"), 0.2f);
	Dark->SetScalarParameterValue(TEXT("Erode"), 0.f);
	Eye->SetScalarParameterValue(TEXT("Erode"), 0.f);

	// The face looks along +X. It is tipped forward so it gazes down at the sea.
	FaceRoot = NewObject<USceneComponent>(this, TEXT("FaceRoot"));
	FaceRoot->SetMobility(EComponentMobility::Movable);
	FaceRoot->SetupAttachment(RootComponent);
	FaceRoot->SetRelativeRotation(FRotator(-PitchDown, 0.f, 0.f));
	FaceRoot->RegisterComponent();

	const FRotator None = FRotator::ZeroRotator;
	// Mass of the head, then the planes of the face built forward from it.
	Part(TEXT("Sphere"), FVector(-900.f, 0.f, 300.f), None, FVector(72.f, 64.f, 90.f), Light);        // skull
	Part(TEXT("Sphere"), FVector(600.f, 0.f, -2300.f), None, FVector(54.f, 50.f, 42.f), Light);       // jaw
	Part(TEXT("Sphere"), FVector(2500.f, 0.f, 2300.f), None, FVector(26.f, 54.f, 26.f), Light);       // forehead
	Part(TEXT("Sphere"), FVector(2950.f, 0.f, 1350.f), None, FVector(15.f, 58.f, 9.f), Light);        // brow ridge
	Part(TEXT("Cone"), FVector(3500.f, 0.f, 250.f), None, FVector(11.f, 10.f, 27.f), Light);          // nose
	Part(TEXT("Sphere"), FVector(3650.f, 0.f, -950.f), None, FVector(10.f, 13.f, 7.f), Light);        // tip of the nose
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		Part(TEXT("Sphere"), FVector(2400.f, Side * 1850.f, -450.f), None, FVector(24.f, 22.f, 26.f), Light);   // cheekbone
		Part(TEXT("Sphere"), FVector(3080.f, Side * 1350.f, 800.f), None, FVector(8.f, 15.f, 9.f), Dark);       // eye socket
		Part(TEXT("Sphere"), FVector(3260.f, Side * 1350.f, 760.f), None, FVector(3.f, 6.f, 3.2f), Eye);        // the eye in it
		Part(TEXT("Sphere"), FVector(2500.f, Side * 1150.f, -1500.f), None, FVector(9.f, 7.f, 14.f), Dark);     // hollow beside the mouth
	}
	Part(TEXT("Sphere"), FVector(3200.f, 0.f, -1950.f), None, FVector(7.f, 21.f, 3.6f), Dark);        // parted mouth
	Part(TEXT("Sphere"), FVector(3250.f, 0.f, -1620.f), None, FVector(8.f, 19.f, 4.f), Light);        // upper lip
	Part(TEXT("Sphere"), FVector(3200.f, 0.f, -2330.f), None, FVector(9.f, 17.f, 5.f), Light);        // lower lip
	Part(TEXT("Sphere"), FVector(2700.f, 0.f, -3250.f), None, FVector(20.f, 24.f, 19.f), Light);      // chin

	// Hair of cloud streaming back and up; or a long beard and heavier brows for the old king.
	FRandomStream Rng(static_cast<int32>(Seed * 100.f));
	for (int32 i = 0; i < 16; ++i)
	{
		const float A = Rng.FRandRange(-1.9f, 1.9f);
		const FVector Loc(-1600.f + Rng.FRandRange(-2600.f, 900.f), FMath::Sin(A) * 3500.f, 2400.f + FMath::Cos(A) * 3200.f + Rng.FRandRange(-600.f, 900.f));
		const float S = Rng.FRandRange(26.f, 48.f);
		Part(TEXT("Sphere"), Loc, None, FVector(S * 1.3f, S, S * 0.9f), Light);
	}
	if (bBearded)
	{
		for (int32 i = 0; i < 9; ++i)
		{
			const float Y = (i - 4) * 520.f;
			const float Len = 34.f - FMath::Abs(i - 4) * 3.f + Rng.FRandRange(-3.f, 3.f);
			Part(TEXT("Cone"), FVector(2300.f - FMath::Abs(i - 4) * 220.f, Y, -3600.f - Len * 42.f), FRotator(180.f, 0.f, 0.f), FVector(11.f, 9.f, Len), Light);
		}
		Part(TEXT("Sphere"), FVector(3050.f, 0.f, 1500.f), None, FVector(17.f, 62.f, 10.f), Light);     // heavier brow
		Part(TEXT("Sphere"), FVector(3300.f, 0.f, -1450.f), None, FVector(9.f, 27.f, 6.f), Light);      // moustache
	}
	// Neck and shoulders dissolving into the cloud below.
	for (int32 i = 0; i < 7; ++i)
	{
		Part(TEXT("Sphere"), FVector(Rng.FRandRange(-2500.f, 900.f), Rng.FRandRange(-5200.f, 5200.f), -6200.f + Rng.FRandRange(-1800.f, 900.f)), None, FVector(Rng.FRandRange(50.f, 80.f)), Light);
	}
}

void ADCCloudFace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { Director = ADCRideDirector::Get(this); }
	if (!Director || !Light) { return; }
	if (!bHooked)
	{
		bHooked = true;
		Director->OnLightning.AddWeakLambda(this, [this]() { Flash = FMath::FRandRange(0.6f, 1.f); });
	}
	Flash = FMath::Max(0.f, Flash - DeltaSeconds * 1.6f);

	// Dim and cold by default; a flash floods the pale parts and leaves the hollows dark, which is
	// what makes the features read.
	const float T = Director->RideTime + Seed;
	const float Breath = 0.85f + 0.15f * FMath::Sin(T * 0.5f);
	const FLinearColor Glow = FLinearColor(0.5f, 0.6f, 0.8f) * (Flash * 1.4f);
	Light->SetVectorParameterValue(TEXT("Top"), FLinearColor(0.2f, 0.25f, 0.31f) * Breath);
	Light->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.13f, 0.17f, 0.22f) * Breath);
	Light->SetVectorParameterValue(TEXT("Flash"), FLinearColor(Glow.R, Glow.G, Glow.B, 0.5f));
	Dark->SetVectorParameterValue(TEXT("Top"), FLinearColor(0.015f, 0.02f, 0.03f));
	Dark->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.015f, 0.02f, 0.03f));
	Dark->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, 0.85f));
	const float EyeGlow = 0.25f + Flash * 3.f;
	Eye->SetVectorParameterValue(TEXT("Top"), FLinearColor(0.55f, 0.75f, 1.f) * EyeGlow);
	Eye->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.55f, 0.75f, 1.f) * EyeGlow);
	Eye->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, 0.9f));
}
