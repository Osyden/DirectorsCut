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
	HeadSize = Height * 0.6f;
	BaseZ = GetActorLocation().Z;
	if (ADCRideDirector* D = ADCRideDirector::Get(this)) { Slot = D->NumCloudFaces++; }
	// Under the sky pack the face is a statue's head from the ruins, hundreds of metres tall, drawn as cloud.
	ADCRideDirector* Dir = ADCRideDirector::Get(this);
	// Off for now (-DCFaces): the cloud deck lies between the ship and a head this high, so what shows is a pale dome, not a face.
	if (Dir && Dir->bUDS && FParse::Param(FCommandLine::Get(), TEXT("DCFaces")))
	{
		UStaticMesh* Head = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AncientRuins/Meshes/Arch/SM_Statue_01_head_01.SM_Statue_01_head_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		UMaterialInterface* Cloud = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_CloudFace.M_CloudFace"));
		if (Head && Cloud)
		{
			const FBox Box = Head->GetBoundingBox();
			const float S = Height * 1.6f / FMath::Max(1.f, Box.GetSize().Z);
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Head);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetForceDisableNanite(true);   // translucent
			HeadMID = UMaterialInstanceDynamic::Create(Cloud, this);
			C->SetMaterial(0, HeadMID);
			C->SetupAttachment(RootComponent);
			C->SetRelativeScale3D(FVector(S));
			HeadComp = C;
			HeadCentre = Box.GetCenter() * S;
			HeadPitch = PitchDown;
			C->SetBoundsScale(2.f);
			C->RegisterComponent();
			HeadMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.62f, 0.68f, 0.78f, 0.f));
		}
	}
	// The old drawing in the director's own cloud layer (M_StormCloud), and the older sphere build behind a switch.
	if (!bBillows) { return; }
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
	if (!Director) { return; }
	if (Slot >= 0 && Slot < 3)
	{
		// Gone once the city sinks behind the ship.
		if (!bPlaced) { bPlaced = true; BaseZ = GetActorLocation().Z; }
		const FVector L = GetActorLocation();
		Director->CloudFaces[Slot] = FVector4(L.X, L.Y, L.Z, L.Z > BaseZ - 20000.f ? HeadSize : 0.f);
	}
	if (HeadMID)
	{
		// It is there when the ship is within a few kilometres, breathing a little; lightning lights it from inside.
		const FVector L = GetActorLocation();
		const float Dist = FVector2D(L.X, L.Y).Size();
		// It watches the ship: turned to her wherever she is, and bowed to look down at her. (The statue's head looks along +Y in its own space.)
		const FQuat Q = FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(-L.Y, -L.X)), 0.f).Quaternion() * FRotator(-HeadPitch, 0.f, 0.f).Quaternion() * FRotator(0.f, 270.f, 0.f).Quaternion();
		HeadComp->SetWorldLocationAndRotation(L - Q.RotateVector(HeadCentre), Q);
		static float Reach = 420000.f; static bool bReachRead = false;
		if (!bReachRead) { bReachRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCFaceReach="), Reach); }
		const float Want = FMath::Clamp((Reach - Dist) / 120000.f, 0.f, 1.f) * FMath::Clamp(Director->GetEnv().Mist / 0.5f, 0.f, 1.f) * (L.Z > BaseZ - 20000.f ? 1.f : 0.f);
		HeadShown = FMath::FInterpTo(HeadShown, Want, DeltaSeconds, 0.6f);
		HeadFlash = FMath::Max(FMath::Max(0.f, HeadFlash - DeltaSeconds * 1.5f), Director->LightningNow);
		const float Breath = 0.8f + 0.2f * FMath::Sin((Director->RideTime + Seed) * 0.45f);
		HeadMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.62f, 0.68f, 0.78f, HeadShown * Breath * 0.75f));
		HeadMID->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.55f, 0.7f, 1.f) * (HeadFlash * 1.2f));
	}
	if (!Light) { return; }
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
