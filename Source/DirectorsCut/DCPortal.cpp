#include "DCPortal.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	struct FPortalSurface
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;

		void Quad(int32 A, int32 B, int32 C, int32 D)
		{
			// Unreal's procedural-mesh normal calculation uses clockwise winding.
			Triangles.Append({ A, B, C, B, D, C });
		}

		void Apply(UProceduralMeshComponent* Mesh)
		{
			TArray<FVector> Normals;
			TArray<FProcMeshTangent> Tangents;
			UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
			Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
		}
	};
}

ADCPortal::ADCPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	Mouth = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterMouth"));
	Crests = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SpiralCrests"));
	Spray = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CrestSpray"));
	for (UProceduralMeshComponent* Mesh : { Mouth.Get(), Crests.Get(), Spray.Get() })
	{
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
	}
}

void ADCPortal::BeginPlay()
{
	Super::BeginPlay();
	UMaterialInterface* Water = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromWater.M_MaelstromWater"));
	if (!Water) { Water = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Wave.M_Wave")); }
	if (Water)
	{
		WaterMID = UMaterialInstanceDynamic::Create(Water, this);
		WaterMID->SetScalarParameterValue(TEXT("WaveHeight"), 70.f);
		WaterMID->SetScalarParameterValue(TEXT("FlowStrength"), 1.4f);
		WaterMID->SetScalarParameterValue(TEXT("TunnelFlow"), 1.f);
		Mouth->SetMaterial(0, WaterMID);
		Crests->SetMaterial(0, WaterMID);
	}
	UMaterialInterface* Mist = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromSpray.M_MaelstromSpray"));
	if (Mist)
	{
		SprayMID = UMaterialInstanceDynamic::Create(Mist, this);
		Spray->SetMaterial(0, SprayMID);
	}
	Radius = FMath::Max(Radius, 6000.f);
	BuildMouth();
	BuildCrests();
	if (SprayMID) { BuildSpray(); }
}

void ADCPortal::BuildMouth()
{
	FPortalSurface Surface;
	constexpr int32 Along = 48;
	constexpr int32 Around = 144;
	for (int32 X = 0; X <= Along; ++X)
	{
		const float T = static_cast<float>(X) / Along;
		const float R = Radius * FMath::Lerp(1.f, 0.38f, FMath::SmoothStep(0.f, 1.f, T));
		const float Height = R * 0.72f;
		for (int32 Ring = 0; Ring <= Around; ++Ring)
		{
			const float A = UE_TWO_PI * Ring / Around;
			const float Twist = A + T * 4.8f;
			const float Crest = FMath::Pow(FMath::Max(0.f, FMath::Sin(Twist * 5.f + T * 3.f)), 6.f);
			const float Uneven = 0.035f * FMath::Sin(A * 3.f + T * 8.f) + 0.018f * FMath::Sin(A * 11.f - T * 19.f);
			const float Radial = R * (1.f + Uneven - Crest * 0.075f);
			Surface.Vertices.Add(FVector(T * Radius * 3.4f, FMath::Cos(A) * Radial, Height + FMath::Sin(A) * Radial));
			Surface.UVs.Add(FVector2D(T * Radius * 3.4f / 10000.f, A * R / 10000.f));
			Surface.Colors.Add(FLinearColor(0.15f + Crest * 0.55f, T, 0.65f, 1.f));
		}
	}
	for (int32 X = 0; X < Along; ++X)
	{
		for (int32 Ring = 0; Ring < Around; ++Ring)
		{
			const int32 A = X * (Around + 1) + Ring;
			Surface.Quad(A, A + 1, A + Around + 1, A + Around + 2);
		}
	}
	Surface.Apply(Mouth);
}

void ADCPortal::BuildCrests()
{
	FPortalSurface Surface;
	constexpr int32 Arms = 6;
	constexpr int32 Along = 64;
	constexpr int32 Across = 12;
	for (int32 Arm = 0; Arm < Arms; ++Arm)
	{
		const int32 Base = Surface.Vertices.Num();
		const float Phase = Arm * UE_TWO_PI / Arms;
		for (int32 X = 0; X <= Along; ++X)
		{
			const float T = static_cast<float>(X) / Along;
			const float Angle = Phase + T * 2.8f;
			const float R = Radius * FMath::Lerp(1.65f, 0.84f, T);
			const float Width = Radius * 0.16f * FMath::Sin(UE_PI * T);
			for (int32 Y = 0; Y <= Across; ++Y)
			{
				const float V = static_cast<float>(Y) / Across;
				const float Curl = V * UE_PI * 1.25f;
				const float Radial = R + Width * FMath::Cos(Curl);
				const float Forward = Radius * (-0.10f + T * 0.28f) - Width * FMath::Sin(Curl);
				Surface.Vertices.Add(FVector(Forward, FMath::Cos(Angle) * Radial,
					Radius * 0.72f + FMath::Sin(Angle) * Radial));
				Surface.UVs.Add(FVector2D(T * Radius * 4.f / 10000.f, V * Radius * 0.32f / 10000.f));
				Surface.Colors.Add(FLinearColor(FMath::Pow(V, 4.f) * 0.9f, T, 0.5f, 1.f));
			}
		}
		for (int32 X = 0; X < Along; ++X)
		{
			for (int32 Y = 0; Y < Across; ++Y)
			{
				const int32 A = Base + X * (Across + 1) + Y;
				Surface.Quad(A, A + 1, A + Across + 1, A + Across + 2);
			}
		}
	}
	Surface.Apply(Crests);
}

void ADCPortal::BuildSpray()
{
	FPortalSurface Surface;
	constexpr int32 Sheets = 9;
	constexpr int32 Along = 36;
	constexpr int32 Across = 5;
	for (int32 Sheet = 0; Sheet < Sheets; ++Sheet)
	{
		const int32 Base = Surface.Vertices.Num();
		const float Phase = Sheet * UE_TWO_PI / Sheets;
		for (int32 X = 0; X <= Along; ++X)
		{
			const float T = static_cast<float>(X) / Along;
			const float Angle = Phase + T * 1.25f;
			const float R = Radius * (1.12f - T * 0.13f);
			for (int32 Y = 0; Y <= Across; ++Y)
			{
				const float V = static_cast<float>(Y) / Across;
				const float Radial = R + V * Radius * 0.15f;
				Surface.Vertices.Add(FVector(-Radius * (0.08f + V * 0.30f),
					FMath::Cos(Angle) * Radial, Radius * 0.72f + FMath::Sin(Angle) * Radial));
				Surface.UVs.Add(FVector2D(T, V));
				const float Density = FMath::Sin(UE_PI * T) * (1.f - V);
				Surface.Colors.Add(FLinearColor(Density, 0.f, V, 0.55f));
			}
		}
		for (int32 X = 0; X < Along; ++X)
		{
			for (int32 Y = 0; Y < Across; ++Y)
			{
				const int32 A = Base + X * (Across + 1) + Y;
				Surface.Quad(A, A + 1, A + Across + 1, A + Across + 2);
			}
		}
	}
	Surface.Apply(Spray);
}

void ADCPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { Director = ADCRideDirector::Get(this); }
	if (!Director) { return; }
	// The treadmill sinks the entrance in sea space, then applies world rocking.
	// Keep the opaque shell until its tallest crest (~2.4 radii) is underwater;
	// fading it after only one radius made the remaining tower disappear in view.
	const FVector SeaPosition = Director->GetWorldRock().InverseTransformPosition(GetActorLocation());
	const float SinkDistance = FMath::Max(0.f, Director->SeaLevelZ - SeaPosition.Z);
	const float Strength = FMath::Clamp(1.f - SinkDistance / (Radius * 2.5f), 0.f, 1.f);
	Director->PortalLocation = GetActorLocation();
	Director->PortalRadius = Radius;
	Director->PortalStrength = Strength;
	const FVector Centre = GetActorTransform().TransformPosition(FVector(0.f, 0.f, Radius * 0.72f));
	if (WaterMID)
	{
		WaterMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime);
		WaterMID->SetVectorParameterValue(TEXT("BowlCenter"), FLinearColor(Centre.X, Centre.Y, Centre.Z));
		WaterMID->SetVectorParameterValue(TEXT("Deep"), Director->GetEnv().SeaColor);
		WaterMID->SetVectorParameterValue(TEXT("Scatter"), FLinearColor(0.02f, 0.12f, 0.16f, 0.2f));
	}
	if (SprayMID)
	{
		SprayMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime);
		SprayMID->SetScalarParameterValue(TEXT("Reveal"), Strength);
	}
	// The owning treadmill sinks the entrance as the ship enters the slide. Never leave a tall
	// portal sheet protruding above the water after that transition has finished.
	const bool bVisible = Strength > 0.01f && Director->GetPhase() != EDCPhase::Maelstrom && Director->GetPhase() != EDCPhase::Ending;
	Mouth->SetVisibility(bVisible);
	Crests->SetVisibility(bVisible);
	Spray->SetVisibility(bVisible);
}
