#include "DCWeatherFX.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCWeatherFX::ADCWeatherFX()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

UProceduralMeshComponent* ADCWeatherFX::BuildStreaks(const TCHAR* Name, int32 Count)
{
	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this, Name);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetupAttachment(RootComponent);
	Mesh->RegisterComponent();

	// All four corners of a streak share one seed point; the material spreads them apart.
	TArray<FVector> Verts; Verts.Reserve(Count * 4);
	TArray<int32> Tris; Tris.Reserve(Count * 6);
	TArray<FVector> Normals;
	TArray<FVector2D> UVs; UVs.Reserve(Count * 4);
	TArray<FColor> Colors; Colors.Reserve(Count * 4);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Seed(FMath::FRandRange(0.f, 3200.f), FMath::FRandRange(0.f, 3200.f), FMath::FRandRange(0.f, 2600.f));
		const FColor Rand(FMath::RandRange(0, 255), FMath::RandRange(0, 255), FMath::RandRange(0, 255), 255);
		const int32 B = Verts.Num();
		for (int32 c = 0; c < 4; ++c)
		{
			Verts.Add(Seed);
			UVs.Add(FVector2D(c & 1, c >> 1));
			Colors.Add(Rand);
		}
		Tris.Append({ B, B + 2, B + 1, B + 1, B + 2, B + 3 });
	}
	Normals.Init(FVector::UpVector, Verts.Num());
	Mesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
	// The material moves every vertex to the camera, so never let the small seed box be culled.
	Mesh->SetBoundsScale(200.f);
	return Mesh;
}

void ADCWeatherFX::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);
	if (Director)
	{
		Director->OnLightning.AddWeakLambda(this, [this]() { FlashBoost = 1.f; });
	}
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Rain.M_Rain"));
	if (!Base)
	{
		UE_LOG(LogDC, Warning, TEXT("WeatherFX: M_Rain missing; no rain or embers"));
		return;
	}
	Rain = BuildStreaks(TEXT("Rain"), 7000);
	RainMID = UMaterialInstanceDynamic::Create(Base, this);
	Rain->SetMaterial(0, RainMID);

	Embers = BuildStreaks(TEXT("Embers"), 900);
	EmberMID = UMaterialInstanceDynamic::Create(Base, this);
	Embers->SetMaterial(0, EmberMID);
}

void ADCWeatherFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director || !RainMID) { return; }
	const FDCEnvState& Env = Director->GetEnv();
	FlashBoost = FMath::Max(0.f, FlashBoost - DeltaSeconds * 4.f);

	// Rain; inside the water tunnel the same streaks read as spray.
	const float RainAmount = FMath::Clamp(Env.Rain + Env.Tube * 0.6f * (1.f - FMath::Min(Env.SeaGlow.R, 1.f)), 0.f, 1.f);
	Rain->SetVisibility(RainAmount > 0.01f);
	const float Wind = FMath::Clamp(Env.WaveHeight / 300.f, 0.2f, 1.3f);
	RainMID->SetVectorParameterValue(TEXT("Motion"), FLinearColor(2400.f + 900.f * Wind, 110.f + 60.f * Wind, 1.5f, Wind));
	// Unlit, so brightness follows the scene by hand: dim normally, bright in a lightning flash.
	const float Light = 0.12f + 0.15f * Env.SkyIntensity + FlashBoost * 2.5f;
	RainMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.55f * Light, 0.65f * Light, 0.8f * Light, 0.7f * RainAmount));

	// Embers rise out of the burning sea.
	const float EmberAmount = FMath::Clamp(Env.SeaGlow.R, 0.f, 1.f);
	Embers->SetVisibility(EmberAmount > 0.01f);
	EmberMID->SetVectorParameterValue(TEXT("Motion"), FLinearColor(-260.f, 9.f, 5.f, 0.5f));
	EmberMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(30.f, 7.f, 0.6f, 0.9f * EmberAmount));
}
