#include "DCRideDirector.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "ProceduralMeshComponent.h"
#include "DCTreadmill.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/IConsoleManager.h"

ADCRideDirector::ADCRideDirector()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);

	Flash = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(RootComponent);
	Flash->SetMobility(EComponentMobility::Movable);
	Flash->bAtmosphereSunLight = false;
	Flash->Intensity = 0.f;
	Flash->LightColor = FColor(190, 210, 255);
	Flash->SetRelativeRotation(FRotator(-60.f, 40.f, 0.f));

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(RootComponent);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	Fog->bEnableVolumetricFog = true;

	Sea = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Sea"));
	Sea->SetupAttachment(RootComponent);
	Sea->SetMobility(EComponentMobility::Movable);
	Sea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sea->SetCastShadow(false);

	// Fixed exposure, set per phase. Auto exposure would brighten the dark phases back to mid grey.
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
	PostProcess->Settings.bOverride_AutoExposureMethod = true;
	PostProcess->Settings.AutoExposureMethod = AEM_Manual;
	PostProcess->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PostProcess->Settings.AutoExposureApplyPhysicalCameraExposure = false;
	PostProcess->Settings.bOverride_AutoExposureBias = true;
	PostProcess->Settings.AutoExposureBias = 0.f;

	BuildDefaultPhases();
}

void ADCRideDirector::BuildDefaultPhases()
{
	Phases.Reset();
	auto Add = [this](EDCPhase P, float Start, float Blend) -> FDCEnvState&
	{
		FDCPhaseDef& D = Phases.AddDefaulted_GetRef();
		D.Phase = P; D.StartTime = Start; D.BlendTime = Blend;
		return D.Env;
	};

	{
		FDCEnvState& E = Add(EDCPhase::OpenSea, 0.f, 0.f);
		E.FogColor = FLinearColor(0.75f, 0.55f, 0.4f); E.FogDensity = 0.004f;
		E.SunColor = FLinearColor(1.f, 0.72f, 0.45f); E.SunIntensity = 7.f; E.SunPitch = -8.f;
		E.SeaColor = FLinearColor(0.03f, 0.09f, 0.13f); E.SeaRoughness = 1.2f; E.ShipSpeed = 1000.f; E.WaveHeight = 70.f; E.Foam = 0.5f; E.Exposure = 1.0f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Fog, 60.f, 25.f);
		E.FogColor = FLinearColor(0.12f, 0.17f, 0.2f); E.FogDensity = 0.16f; E.FogHeightFalloff = 0.002f;
		E.SunColor = FLinearColor(0.5f, 0.7f, 0.8f); E.SunIntensity = 0.6f; E.SunPitch = -20.f; E.SkyIntensity = 0.35f;
		E.SeaColor = FLinearColor(0.01f, 0.03f, 0.035f); E.SeaRoughness = 2.5f; E.LightningRate = 3.f; E.ShipSpeed = 1100.f; E.WaveHeight = 110.f; E.Foam = 0.6f; E.Exposure = 0.6f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Slide, 240.f, 6.f);
		E.FogColor = FLinearColor(0.05f, 0.1f, 0.14f); E.FogDensity = 0.05f; E.FogHeightFalloff = 0.002f;
		E.SunIntensity = 0.2f; E.SunPitch = -30.f; E.SkyIntensity = 0.2f;
		E.SeaColor = FLinearColor(0.0f, 0.05f, 0.08f); E.SeaRoughness = 9.f; E.LightningRate = 12.f; E.ShipSpeed = 4000.f; E.WaveHeight = 150.f; E.Foam = 1.4f; E.Exposure = 1.5f; E.Tube = 1.f; E.Drop = 260.f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Hell, 270.f, 8.f);
		E.FogColor = FLinearColor(0.5f, 0.09f, 0.02f); E.FogDensity = 0.07f; E.FogHeightFalloff = 0.03f;
		E.SunColor = FLinearColor(1.f, 0.3f, 0.1f); E.SunIntensity = 2.5f; E.SunPitch = -35.f; E.SkyIntensity = 0.5f;
		E.SeaColor = FLinearColor(0.25f, 0.03f, 0.0f); E.SeaRoughness = 3.5f; E.ShipSpeed = 1600.f; E.WaveHeight = 170.f; E.Foam = 1.2f; E.SeaGlow = FLinearColor(0.7f, 0.1f, 0.006f); E.Exposure = 0.3f; E.Tube = 0.3f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Breach, 420.f, 5.f);
		E.FogColor = FLinearColor(0.1f, 0.1f, 0.12f); E.FogDensity = 0.15f;
		E.SunIntensity = 0.3f; E.SkyIntensity = 0.3f; E.SeaRoughness = 8.f; E.LightningRate = 20.f; E.ShipSpeed = 3500.f; E.WaveHeight = 200.f; E.Foam = 1.5f; E.Exposure = 1.5f; E.Tube = 0.7f; E.Drop = 150.f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Maelstrom, 440.f, 12.f);
		E.FogColor = FLinearColor(0.06f, 0.08f, 0.1f); E.FogDensity = 0.02f; E.FogHeightFalloff = 0.004f;
		E.SunColor = FLinearColor(0.55f, 0.65f, 0.8f); E.SunIntensity = 0.9f; E.SunPitch = -28.f; E.SkyIntensity = 0.6f;
		E.SeaColor = FLinearColor(0.004f, 0.015f, 0.02f); E.SeaRoughness = 6.f; E.Rain = 1.f; E.LightningRate = 14.f; E.ShipSpeed = 2200.f; E.WaveHeight = 300.f; E.Foam = 0.75f; E.Exposure = 0.9f; E.Whirl = 1.f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Ending, 600.f, 15.f);
		E.FogColor = FLinearColor(0.7f, 0.6f, 0.5f); E.FogDensity = 0.006f;
		E.SunColor = FLinearColor(1.f, 0.8f, 0.6f); E.SunIntensity = 6.f; E.SunPitch = -6.f;
		E.SeaRoughness = 1.f; E.ShipSpeed = 600.f; E.WaveHeight = 60.f; E.Foam = 0.4f; E.Exposure = 1.0f;
	}
}

float ADCRideDirector::GetRideLength() const
{
	return Phases.Num() ? Phases.Last().StartTime + 20.f : 0.f;
}

ADCRideDirector* ADCRideDirector::Get(const UObject* WorldContext)
{
	if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCRideDirector> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void ADCRideDirector::BeginPlay()
{
	Super::BeginPlay();
	BuildSeaMesh();
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Sea.M_Sea"));
	if (!Base) { Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")); }
	if (Base)
	{
		SeaMID = UMaterialInstanceDynamic::Create(Base, this);
		Sea->SetMaterial(0, SeaMID);
	}
	if (Phases.Num())
	{
		Env = Phases[0].Env;
		CurrentPhase = Phases[0].Phase;
	}
	ApplyEnv(0.f);
}

void ADCRideDirector::Seek(float NewTime)
{
	RideTime = FMath::Max(0.f, NewTime);
}

void ADCRideDirector::DamageShip(float Amount)
{
	ShipHealth = FMath::Max(0.f, ShipHealth - Amount);
	UE_LOG(LogDC, Log, TEXT("Ship hit for %.0f, health %.0f"), Amount, ShipHealth);
}

void ADCRideDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Phases.Num())
	{
		return;
	}

	RideTime = FMath::Min(RideTime + DeltaSeconds * TimeScale, GetRideLength());

	int32 Idx = 0;
	for (int32 i = 0; i < Phases.Num(); ++i)
	{
		if (RideTime >= Phases[i].StartTime) { Idx = i; }
	}
	const FDCPhaseDef& Cur = Phases[Idx];
	if (Cur.Phase != CurrentPhase)
	{
		CurrentPhase = Cur.Phase;
		UE_LOG(LogDC, Log, TEXT("Phase -> %s at %.1fs"), DCPhaseName(CurrentPhase), RideTime);
		OnPhaseChanged.Broadcast(CurrentPhase);
	}

	if (Idx > 0 && Cur.BlendTime > 0.f)
	{
		const float T = FMath::Clamp((RideTime - Cur.StartTime) / Cur.BlendTime, 0.f, 1.f);
		Env = FDCEnvState::Lerp(Phases[Idx - 1].Env, Cur.Env, FMath::SmoothStep(0.f, 1.f, T));
	}
	else
	{
		Env = Cur.Env;
	}

	ApplyEnv(DeltaSeconds);
}

void ADCRideDirector::ApplyEnv(float DeltaSeconds)
{
	Sun->SetIntensity(Env.SunIntensity);
	Sun->SetLightColor(Env.SunColor);
	Sun->SetRelativeRotation(FRotator(Env.SunPitch, 20.f, 0.f));
	SkyLight->SetIntensity(Env.SkyIntensity);
	// Tint ambient light toward the fog so the sea does not mirror a blue sky that is not there.
	{
		const float MaxC = FMath::Max3(Env.FogColor.R, Env.FogColor.G, Env.FogColor.B);
		const FLinearColor Tint = MaxC > KINDA_SMALL_NUMBER ? Env.FogColor / MaxC : FLinearColor::White;
		SkyLight->SetLightColor(FMath::Lerp(FLinearColor::White, Tint, 0.8f));
	}
	Fog->SetFogDensity(Env.FogDensity);
	Fog->SetFogHeightFalloff(Env.FogHeightFalloff);
	Fog->SetFogInscatteringColor(Env.FogColor);
	Fog->SetVolumetricFogAlbedo(Env.FogColor.ToFColor(true));
	if (SeaMID)
	{
		// Water slides aft at ship speed and sideways with the helm, so the sea agrees with the hazards.
		SeaScroll.X += Env.ShipSpeed * TimeScale * DeltaSeconds;
		if (const ADCTreadmill* Treadmill = ADCTreadmill::Get(this)) { SeaScroll.Y = Treadmill->GetLaneOffset(); }
		SeaMID->SetVectorParameterValue(TEXT("Scroll"), FLinearColor(SeaScroll.X, SeaScroll.Y, 0.f, 0.f));
		SeaMID->SetScalarParameterValue(TEXT("WaveAmp"), Env.WaveHeight);
		SeaMID->SetScalarParameterValue(TEXT("Chop"), FMath::Clamp(Env.WaveHeight / 300.f, 0.1f, 1.5f));
		SeaMID->SetScalarParameterValue(TEXT("FoamAmount"), Env.Foam);
		SeaMID->SetVectorParameterValue(TEXT("DeepColor"), Env.SeaColor);
		SeaMID->SetVectorParameterValue(TEXT("GlowColor"), Env.SeaGlow);
		SeaMID->SetVectorParameterValue(TEXT("Color"), Env.SeaColor);
		SeaMID->SetVectorParameterValue(TEXT("Shape"), FLinearColor(Env.Tube, TubeRadius, Env.Drop, 0.f));
		SeaMID->SetVectorParameterValue(TEXT("Whirl"), FLinearColor(WhirlCenter.X, WhirlCenter.Y, Env.Whirl * WhirlDepth, WhirlRadius));
	}
	PostProcess->Settings.AutoExposureBias = Env.Exposure;

	// Sea motion: the world rocks, the ship does not.
	const float T = RideTime;
	const float A = Env.SeaRoughness;
	const float Roll = A * (FMath::Sin(T * 0.55f) + 0.35f * FMath::Sin(T * 1.3f + 1.f));
	const float Pitch = A * 0.45f * (FMath::Sin(T * 0.4f + 2.f) + 0.3f * FMath::Sin(T * 1.1f));
	const float Heave = A * 18.f * FMath::Sin(T * 0.7f + 0.5f);
	WorldRock = FTransform(FRotator(Pitch, 0.f, Roll), FVector(0.f, 0.f, Heave));
	// On the wall of the maelstrom the sea slopes toward the centre. Tilt the whole sea the other way so
	// the water is level under the ship and the funnel, not the deck, is what leans.
	const float R0 = WhirlCenter.Size();
	const float K = (R0 * R0) / (WhirlRadius * WhirlRadius);
	const float Slope = Env.Whirl * WhirlDepth * 2.f * R0 / (WhirlRadius * WhirlRadius) / FMath::Square(1.f + K);
	const FQuat Level = FRotator(0.f, 0.f, FMath::RadiansToDegrees(FMath::Atan(Slope)) * (WhirlCenter.Y < 0.f ? 1.f : -1.f)).Quaternion();
	Sea->SetRelativeTransform(FTransform(WorldRock.GetRotation() * Level, FVector(0.f, 0.f, SeaLevelZ + Heave)));

	// Lightning.
	if (FlashTimeLeft > 0.f)
	{
		FlashTimeLeft -= DeltaSeconds;
		Flash->SetIntensity(FlashTimeLeft > 0.f ? FMath::FRandRange(8.f, 30.f) : 0.f);
	}
	else if (Env.LightningRate > 0.01f)
	{
		NextFlashIn -= DeltaSeconds;
		if (NextFlashIn <= 0.f)
		{
			FlashTimeLeft = FMath::FRandRange(0.08f, 0.35f);
			OnLightning.Broadcast();
			Flash->SetRelativeRotation(FRotator(FMath::FRandRange(-80.f, -30.f), FMath::FRandRange(0.f, 360.f), 0.f));
			NextFlashIn = FMath::FRandRange(0.5f, 1.5f) * 60.f / Env.LightningRate;
		}
	}
}

static FAutoConsoleCommandWithWorldAndArgs GDCSeekCmd(
	TEXT("dc.Ride.Seek"), TEXT("Jump the ride to a time in seconds."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (ADCRideDirector* D = ADCRideDirector::Get(World))
		{
			D->Seek(Args.Num() ? FCString::Atof(*Args[0]) : 0.f);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDCSpeedCmd(
	TEXT("dc.Ride.Speed"), TEXT("Set the ride time scale (1 = normal)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (ADCRideDirector* D = ADCRideDirector::Get(World))
		{
			D->TimeScale = Args.Num() ? FCString::Atof(*Args[0]) : 1.f;
		}
	}));

void ADCRideDirector::BuildSeaMesh()
{
	// Polar grid centred on the ship: fine near the hull where waves are seen close up,
	// coarse toward a horizon tens of kilometres out. The ship never moves, so neither does this.
	const int32 Rings = 112;
	const int32 Segments = 192;
	const float R0 = 500.f;
	const float Growth = 1.09f;

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	Verts.Reserve(Rings * Segments + 1);
	Verts.Add(FVector::ZeroVector);
	for (int32 r = 1; r <= Rings; ++r)
	{
		const float Radius = R0 * (FMath::Pow(Growth, static_cast<float>(r)) - 1.f);
		for (int32 s = 0; s < Segments; ++s)
		{
			const float A = 2.f * UE_PI * s / Segments;
			Verts.Add(FVector(Radius * FMath::Cos(A), Radius * FMath::Sin(A), 0.f));
		}
	}
	for (int32 s = 0; s < Segments; ++s)
	{
		Tris.Append({ 0, 1 + (s + 1) % Segments, 1 + s });
	}
	for (int32 r = 1; r < Rings; ++r)
	{
		const int32 A0 = 1 + (r - 1) * Segments;
		const int32 B0 = 1 + r * Segments;
		for (int32 s = 0; s < Segments; ++s)
		{
			const int32 S1 = (s + 1) % Segments;
			Tris.Append({ A0 + s, A0 + S1, B0 + s });
			Tris.Append({ A0 + S1, B0 + S1, B0 + s });
		}
	}
	Normals.Init(FVector::UpVector, Verts.Num());
	UVs.Init(FVector2D::ZeroVector, Verts.Num());
	Sea->CreateMeshSection(0, Verts, Tris, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	// Waves move vertices in the material, so give culling generous bounds.
	Sea->SetBoundsScale(1.5f);
}
