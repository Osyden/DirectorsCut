#include "DCFogFront.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCFogFront::ADCFogFront()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ADCFogFront::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Base || !Sphere)
	{
		UE_LOG(LogDC, Warning, TEXT("FogFront: M_Cloud missing; no cloud front"));
		return;
	}
	CloudMID = UMaterialInstanceDynamic::Create(Base, this);
	for (int32 s = 0; s < 3; ++s) { CloudShades.Add(UMaterialInstanceDynamic::Create(Base, this)); }

	// A wall of overlapping billows: big and close-packed at the foot, towering and ragged above,
	// bulging toward the ship in the middle like a shelf cloud.
	FRandomStream Rng(4242);
	const float Width = 900000.f;   // 9 km across
	const float Height = 120000.f;  // 1.2 km tall
	const float Depth = 90000.f;
	const int32 Count = 150;
	for (int32 i = 0; i < Count; ++i)
	{
		const float V = FMath::Pow(Rng.FRand(), 1.6f);            // more billows low down
		const float Across = Rng.FRandRange(-0.5f, 0.5f);
		FBillow B;
		B.Size = FMath::Lerp(34000.f, 16000.f, V) * Rng.FRandRange(0.7f, 1.4f);
		// The wall leans forward at the top, and its centre bulges toward the ship.
		const float Bulge = (1.f - FMath::Square(Across * 2.f)) * 40000.f;
		// Keep the wall clear of the mouth where the ship enters.
		if (FMath::Abs(Across * Width) < 34000.f && V * Height < 50000.f) { continue; }
		B.Offset = FVector(Rng.FRandRange(0.f, Depth) - Bulge - V * 30000.f, Across * Width, V * Height + B.Size * 0.15f);
		B.Seed = Rng.FRandRange(0.f, 100.f);

		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Sphere);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetMaterial(0, CloudShades[i % CloudShades.Num()]);
		C->SetupAttachment(RootComponent);
		C->SetRelativeLocation(B.Offset);
		// The engine sphere is 1 m across; billows are flattened a little.
		C->SetRelativeScale3D(FVector(B.Size / 50.f, B.Size / 50.f * 1.2f, B.Size / 50.f * 0.8f));
		C->RegisterComponent();
		B.Component = C;
		Billows.Add(B);
	}

	// The mouth: where the ship goes in, the front is rolled into a tunnel. Rings of billows spiral
	// back into the wall, each a little smaller and turned a little further, around a core of
	// lightning-lit glow. Seen from the sea it is a great curling eye in the storm.
	const FVector MouthCentre(-95000.f, 0.f, 9000.f);
	const int32 RingCount = 6;
	for (int32 r = 0; r < RingCount; ++r)
	{
		const float RingT = static_cast<float>(r) / (RingCount - 1);
		const float Radius = FMath::Lerp(30000.f, 9000.f, RingT);
		const float Back = RingT * 110000.f;
		const int32 PerRing = 20 - r * 2;
		for (int32 i = 0; i < PerRing; ++i)
		{
			const float A = 2.f * UE_PI * i / PerRing + r * 0.5f;
			// Leave the bottom of each ring open: that is the sea the ship sails in on.
			if (FMath::Sin(A) < -0.55f) { continue; }
			FBillow B;
			B.Size = FMath::Lerp(15000.f, 7000.f, RingT) * Rng.FRandRange(0.8f, 1.25f);
			B.Offset = MouthCentre + FVector(Back + Rng.FRandRange(-5000.f, 5000.f), FMath::Cos(A) * Radius, FMath::Sin(A) * Radius + Radius * 0.35f);
			B.Seed = Rng.FRandRange(0.f, 100.f);
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Sphere);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetMaterial(0, CloudShades[(i + r) % CloudShades.Num()]);
			C->SetupAttachment(RootComponent);
			C->SetRelativeLocation(B.Offset);
			C->SetRelativeScale3D(FVector(B.Size / 50.f));
			C->RegisterComponent();
			B.Component = C;
			Billows.Add(B);
		}
	}
	// The glow at the end of the tunnel.
	if (UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud")))
	{
		CoreMID = UMaterialInstanceDynamic::Create(GlowBase, this);
		UStaticMeshComponent* Core = NewObject<UStaticMeshComponent>(this);
		Core->SetMobility(EComponentMobility::Movable);
		Core->SetStaticMesh(Sphere);
		Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Core->SetCastShadow(false);
		Core->SetMaterial(0, CoreMID);
		Core->SetupAttachment(RootComponent);
		Core->SetRelativeLocation(MouthCentre + FVector(125000.f, 0.f, 4000.f));
		Core->SetRelativeScale3D(FVector(520.f));
		Core->RegisterComponent();
	}
}

void ADCFogFront::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director || !CloudMID) { return; }

	const float T = Director->RideTime;
	// It comes on steadily, reaches the bow at ArrivalTime, then sweeps over and is gone astern.
	const float Speed = StartDistance / ArrivalTime;
	const float X = (ArrivalTime - T) * Speed;
	const bool bVisible = T < ArrivalTime + 35.f;
	RootComponent->SetVisibility(bVisible, true);
	if (!bVisible) { return; }
	SetActorLocation(FVector(X, 0.f, Director->SeaLevelZ - 2000.f));

	// The billows breathe.
	for (FBillow& B : Billows)
	{
		if (UStaticMeshComponent* C = B.Component.Get())
		{
			const float Pulse = 1.f + 0.06f * FMath::Sin(T * 0.4f + B.Seed);
			C->SetRelativeScale3D(FVector(B.Size / 50.f, B.Size / 50.f * 1.2f, B.Size / 50.f * 0.8f) * Pulse);
			C->SetRelativeLocation(B.Offset + FVector(0.f, 0.f, 1500.f * FMath::Sin(T * 0.25f + B.Seed * 2.f)));
		}
	}

	// Lightning inside the wall: the whole front flickers from within, more often as it closes.
	Flash = FMath::Max(0.f, Flash - DeltaSeconds * 5.f);
	NextFlashIn -= DeltaSeconds;
	if (NextFlashIn <= 0.f)
	{
		Flash = FMath::FRandRange(0.5f, 1.f);
		const float Closeness = FMath::Clamp(T / ArrivalTime, 0.f, 1.f);
		NextFlashIn = FMath::FRandRange(1.2f, 5.f) * FMath::Lerp(1.6f, 0.5f, Closeness);
	}
	// Fade the front out as the real fog takes over, so there is no hard edge when it passes.
	const float Opacity = FMath::Clamp(1.f - (T - ArrivalTime + 8.f) / 30.f, 0.f, 1.f) * 0.92f;
	const float Flicker = Flash * (0.6f + 0.4f * FMath::Sin(T * 60.f));
	if (CoreMID)
	{
		// Always a cold light down the tunnel; lightning makes it blaze.
		const FLinearColor CoreColor = FLinearColor(0.35f, 0.45f, 0.75f) * (0.55f + 2.2f * Flicker);
		CoreMID->SetVectorParameterValue(TEXT("Top"), CoreColor);
		CoreMID->SetVectorParameterValue(TEXT("Bottom"), CoreColor);
		CoreMID->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, Opacity));
	}
	// Warm sunset light on the cloud tops while we are still in the open; the foot is storm-dark.
	const FDCEnvState& Env = Director->GetEnv();
	const float Sun = FMath::Clamp(Env.SunIntensity / 7.f, 0.05f, 1.f);
	// Three shades of billow, so the wall has modelling instead of reading as one flat sheet.
	static const float Shades[3] = { 0.45f, 1.f, 1.7f };
	for (int32 i = 0; i < CloudShades.Num(); ++i)
	{
		CloudShades[i]->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.55f * Flicker, 0.7f * Flicker, 1.f * Flicker, Opacity));
		CloudShades[i]->SetVectorParameterValue(TEXT("Top"), (FLinearColor(0.5f, 0.3f, 0.2f) * Sun + FLinearColor(0.035f, 0.05f, 0.065f)) * Shades[i % 3]);
		CloudShades[i]->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.008f, 0.012f, 0.02f) * Shades[i % 3]);
	}
}
