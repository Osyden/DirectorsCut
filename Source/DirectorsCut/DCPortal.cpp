#include "DCPortal.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCPortal::ADCPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ADCPortal::BeginPlay()
{
	Super::BeginPlay();
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Base || !Sphere) { return; }
	for (int32 s = 0; s < 3; ++s)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
		MID->SetScalarParameterValue(TEXT("Erode"), 0.8f);
		Shades.Add(MID);
	}
	CoreMID = UMaterialInstanceDynamic::Create(Base, this);
	CoreMID->SetScalarParameterValue(TEXT("Erode"), 0.f);

	auto Billow = [&](USceneComponent* Parent, const FVector& Loc, float Size, UMaterialInterface* Mat)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Sphere);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetMaterial(0, Mat);
		C->SetupAttachment(Parent);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(FVector(Size / 50.f));
		C->RegisterComponent();
	};

	// The tunnel runs away along +X. The centre of the mouth sits a little above the sea, so the
	// lowest billows are in the water and the opening is where the ship can sail into it.
	FRandomStream Rng(777);
	const int32 RingCount = 8;
	for (int32 r = 0; r < RingCount; ++r)
	{
		const float RingT = static_cast<float>(r) / (RingCount - 1);
		USceneComponent* Ring = NewObject<USceneComponent>(this);
		Ring->SetMobility(EComponentMobility::Movable);
		Ring->SetupAttachment(RootComponent);
		Ring->SetRelativeLocation(FVector(RingT * Radius * 5.f, 0.f, Radius * 0.55f));
		Ring->RegisterComponent();
		Rings.Add(Ring);

		const float RingRadius = Radius * FMath::Lerp(1.3f, 0.75f, RingT);
		const int32 Count = 22 - r;
		for (int32 i = 0; i < Count; ++i)
		{
			const float A = 2.f * UE_PI * i / Count;
			// Arms trail off each ring, which is what makes it read as curling.
			const float Trail = Rng.FRandRange(0.f, 1.f);
			const float Rad = RingRadius * (1.f + 0.35f * Trail);
			const float Size = Radius * FMath::Lerp(0.42f, 0.26f, RingT) * Rng.FRandRange(0.75f, 1.25f) * (1.f - 0.4f * Trail);
			Billow(Ring, FVector(Rng.FRandRange(-0.12f, 0.12f) * Radius + Trail * Radius * 0.5f, FMath::Cos(A + Trail * 0.5f) * Rad, FMath::Sin(A + Trail * 0.5f) * Rad), Size, Shades[(i + r) % 3]);
		}
	}
	// The light at the far end.
	Billow(RootComponent, FVector(Radius * 4.5f, 0.f, Radius * 0.55f), Radius * 1.7f, CoreMID);
}

void ADCPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { Director = ADCRideDirector::Get(this); }
	if (!Director || !CoreMID) { return; }
	if (!bHooked)
	{
		bHooked = true;
		Director->OnLightning.AddWeakLambda(this, [this]() { Flash = 1.f; });
	}
	Clock += DeltaSeconds;
	Flash = FMath::Max(0.f, Flash - DeltaSeconds * 2.f);

	// Each ring turns about the tunnel's axis, alternate rings at different rates.
	for (int32 r = 0; r < Rings.Num(); ++r)
	{
		Rings[r]->SetRelativeRotation(FRotator(0.f, 0.f, Clock * (14.f + r * 3.f) * (r % 2 ? 1.f : 0.7f) + r * 30.f));
	}

	const float Pulse = 0.85f + 0.15f * FMath::Sin(Clock * 2.3f);
	const FLinearColor CoreColor = FLinearColor(0.75f, 0.9f, 1.25f) * (4.5f * Pulse + 4.f * Flash);
	CoreMID->SetVectorParameterValue(TEXT("Top"), CoreColor);
	CoreMID->SetVectorParameterValue(TEXT("Bottom"), CoreColor);
	CoreMID->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, 0.95f));
	static const float Tone[3] = { 0.5f, 1.f, 1.6f };
	for (int32 s = 0; s < Shades.Num(); ++s)
	{
		// Lit from within: the inner faces of the billows catch the core's light.
		Shades[s]->SetVectorParameterValue(TEXT("Top"), FLinearColor(0.2f, 0.26f, 0.36f) * Tone[s] * (1.f + Flash));
		Shades[s]->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.1f, 0.14f, 0.2f) * Tone[s] * (1.f + Flash));
		Shades[s]->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.25f * Pulse + Flash * 0.6f, 0.32f * Pulse + Flash * 0.7f, 0.5f * Pulse + Flash, 0.9f));
	}
}
