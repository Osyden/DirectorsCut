#include "DCRideDirector.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/Texture.h"
#include "Kismet/GameplayStatics.h"
#include "ProceduralMeshComponent.h"
#include "DCTreadmill.h"
#include "DCCrew.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ADCRideDirector::ADCRideDirector()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);

	Sun->bEnableLightShaftBloom = true;
	Sun->BloomThreshold = 0.05f;
	Sun->BloomMaxBrightness = 6.f;
	Sun->bEnableLightShaftOcclusion = true;
	Sun->OcclusionMaskDarkness = 0.25f;
	Sun->bCastCloudShadows = true;
	// Sunlit cloud flanks blew out to plain white at full strength.
	Sun->CloudScatteredLuminanceScale = FLinearColor(0.6f, 0.6f, 0.6f);
	Sun->CloudShadowStrength = 0.9f;
	Sun->CloudShadowOnSurfaceStrength = 0.85f;
	Sun->CloudShadowExtent = 60.f;
	Sun->VolumetricScatteringIntensity = 2.f;

	Flash = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(RootComponent);
	Flash->SetMobility(EComponentMobility::Movable);
	Flash->bAtmosphereSunLight = false;
	Flash->Intensity = 0.f;
	Flash->LightColor = FColor(220, 228, 255);
	Flash->SetRelativeRotation(FRotator(-60.f, 40.f, 0.f));

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(RootComponent);
	// One volume from the sea to 9 km holds both the cloud deck and the storm front (M_StormCloud).
	Clouds->LayerBottomAltitude = 0.f;
	Clouds->LayerHeight = 9.f;
	Clouds->TracingStartMaxDistance = 350.f;
	Clouds->TracingMaxDistance = 60.f;
	Clouds->ViewSampleCountScale = 4.f;
	Clouds->ShadowViewSampleCountScale = 4.f;
	Clouds->ShadowTracingDistance = 25.f;
	Clouds->ShadowTracingDistance = 8.f; // shorter than the default 15 km: finer steps, so the hollows between cloud heads hold their shadow

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
	// Enough bloom for lanterns and sun glitter to flare without washing the frame.
	PostProcess->Settings.bOverride_BloomIntensity = true;
	PostProcess->Settings.BloomIntensity = 0.9f;
	// Fast water close to the ship smeared into flat sheets at the default blur.
	PostProcess->Settings.bOverride_MotionBlurAmount = true;
	PostProcess->Settings.MotionBlurAmount = 0.18f;
	PostProcess->Settings.bOverride_MotionBlurMax = true;
	PostProcess->Settings.MotionBlurMax = 1.5f;

	Bolt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Bolt"));
	Bolt->SetupAttachment(RootComponent);
	Bolt->SetMobility(EComponentMobility::Movable);
	Bolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bolt->SetCastShadow(false);
	Bolt->SetVisibility(false);

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
		E.FogColor = FLinearColor(0.75f, 0.55f, 0.4f); E.FogDensity = 0.0004f; E.CloudCover = 0.05f; E.CloudBase = 520000.f;
		E.SunColor = FLinearColor(1.f, 0.86f, 0.68f); E.SunIntensity = 7.f; E.SunPitch = -30.f; E.SunYaw = 262.f; // low on the port beam: the towers of the storm front ahead are lit on one side and dark on the other
		E.SeaColor = FLinearColor(0.03f, 0.09f, 0.13f); E.SeaRoughness = 1.2f; E.ShipSpeed = 1000.f; E.WaveHeight = 70.f; E.Foam = 0.5f; E.Exposure = 1.0f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Fog, 58.f, 7.f); // the mouth of the storm front closes over the bow at 60 s (DCFogFront); dark a few seconds into the tunnel
		// First the inside of the cloud front: close, dark, nothing to see.
		E.FogColor = FLinearColor(0.1f, 0.14f, 0.18f); E.FogDensity = 0.035f; E.FogHeightFalloff = 0.002f; E.CloudCover = 1.f; E.CloudBase = 50000.f;
		E.SunColor = FLinearColor(0.5f, 0.7f, 0.8f); E.SunIntensity = 0.6f; E.SunPitch = -20.f; E.SkyIntensity = 0.35f;
		E.SeaColor = FLinearColor(0.01f, 0.03f, 0.035f); E.SeaRoughness = 2.5f; E.Rain = 0.7f; E.LightningRate = 8.f; E.ShipSpeed = 1500.f; E.WaveHeight = 130.f; E.Foam = 0.7f; E.Exposure = 0.6f;
	}
	{
		// Then it thins, and the drowned city stands in layered blue-grey haze out to a couple of kilometres:
		// cold moonlight from behind the statues, lightning to pick out their outlines, warm lanterns on deck.
		FDCEnvState& E = Add(EDCPhase::Fog, 78.f, 22.f);
		E.FogColor = FLinearColor(0.2f, 0.26f, 0.33f); E.FogDensity = 0.006f; E.FogHeightFalloff = 0.0012f; E.CloudCover = 0.5f; E.CloudBase = 110000.f; E.Mist = 0.75f;
		E.SunColor = FLinearColor(1.f, 0.82f, 0.62f); E.SunIntensity = 3.f; E.SunPitch = -11.f; E.SunYaw = 168.f; // low sun ahead to port, behind the ruins: silhouettes and long shafts through the mist E.SkyIntensity = 0.55f;
		E.SeaColor = FLinearColor(0.012f, 0.019f, 0.025f); E.SeaRoughness = 3.f; E.Rain = 0.35f; E.LightningRate = 10.f; E.ShipSpeed = 1500.f; E.WaveHeight = 190.f; E.Foam = 1.f; E.Exposure = 0.05f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Slide, 240.f, 6.f);
		E.FogColor = FLinearColor(0.05f, 0.1f, 0.14f); E.FogDensity = 0.05f; E.FogHeightFalloff = 0.002f; E.CloudCover = 1.f; E.CloudBase = 60000.f; E.Mist = 0.4f;
		E.SunIntensity = 0.2f; E.SunPitch = -30.f; E.SkyIntensity = 0.2f;
		E.SeaColor = FLinearColor(0.0f, 0.05f, 0.08f); E.SeaRoughness = 9.f; E.LightningRate = 12.f; E.ShipSpeed = 4000.f; E.WaveHeight = 150.f; E.Foam = 1.4f; E.Exposure = 1.5f; E.Tube = 1.f; E.Drop = 260.f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Hell, 270.f, 8.f);
		E.FogColor = FLinearColor(0.2f, 0.035f, 0.01f); E.FogDensity = 0.06f; E.FogHeightFalloff = 0.003f; E.CloudCover = 0.9f; E.CloudBase = 80000.f;
		E.SunColor = FLinearColor(1.f, 0.3f, 0.1f); E.SunIntensity = 2.5f; E.SunPitch = -35.f; E.SkyIntensity = 0.5f;
		E.SeaColor = FLinearColor(0.02f, 0.006f, 0.003f); E.SeaRoughness = 3.5f; E.ShipSpeed = 1600.f; E.WaveHeight = 170.f; E.Foam = 1.2f; E.SeaGlow = FLinearColor(16.f, 2.2f, 0.14f); E.Exposure = 0.3f; E.Tube = 0.3f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Breach, 420.f, 5.f);
		E.FogColor = FLinearColor(0.3f, 0.38f, 0.5f); E.FogDensity = 0.03f; E.CloudCover = 0.7f; E.CloudBase = 170000.f; // the portal: light at the end of the tunnel
		E.SunIntensity = 0.3f; E.SkyIntensity = 0.3f; E.SeaRoughness = 8.f; E.LightningRate = 20.f; E.ShipSpeed = 3500.f; E.WaveHeight = 200.f; E.Foam = 1.5f; E.Exposure = 1.5f; E.Tube = 0.9f; E.Drop = 120.f; E.TubeMouth = 3000.f; E.SunYaw = 195.f; E.Whirl = 0.5f; E.Crater = 1.f;
	}
	{
		// The reveal. The ship bursts out of the tunnel into daylight: a crater a kilometre across walled
		// by falling water, the creature in the middle of it, sun breaking through towering cloud ahead.
		FDCEnvState& E = Add(EDCPhase::Maelstrom, 440.f, 16.f);
		E.FogColor = FLinearColor(0.35f, 0.5f, 0.68f); E.FogDensity = 0.0013f; E.FogHeightFalloff = 0.003f; E.CloudCover = 0.4f; E.CloudBase = 260000.f;
		E.SunColor = FLinearColor(1.f, 0.93f, 0.8f); E.SunIntensity = 9.f; E.SunPitch = -30.f; E.SunYaw = 195.f; E.SkyIntensity = 1.2f;
		E.SeaColor = FLinearColor(0.004f, 0.05f, 0.065f); E.SeaRoughness = 3.f; E.Rain = 0.3f; E.LightningRate = 4.f; E.ShipSpeed = 2200.f; E.WaveHeight = 240.f; E.Foam = 1.3f; E.Exposure = -0.2f; E.Whirl = 1.f; E.Crater = 1.f; E.TubeMouth = 1200.f;
	}
	{
		FDCEnvState& E = Add(EDCPhase::Ending, 600.f, 15.f);
		E.FogColor = FLinearColor(0.7f, 0.6f, 0.5f); E.FogDensity = 0.0012f; E.CloudCover = 0.16f; E.CloudBase = 220000.f;
		E.SunColor = FLinearColor(1.f, 0.8f, 0.6f); E.SunIntensity = 7.f; E.SunPitch = -8.f; E.SunYaw = 166.f; E.SkyIntensity = 1.f; // a low sun ahead, a little to port of the mast: sailing out into the morning
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
	// The cloud material samples the engine's noise volumes. Nothing on screen "uses" a volume
	// texture in the way the streamer measures, so left alone only their smallest mips stay loaded
	// and every sample returns the same grey. Keep them fully resident.
	for (const TCHAR* NoisePath : { TEXT("/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128.T_VolumeNoiseShape128"), TEXT("/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseErosion32.T_VolumeNoiseErosion32") })
	{
		if (UTexture* Noise = LoadObject<UTexture>(nullptr, NoisePath))
		{
			Noise->SetForceMipLevelsToBeResident(100000.f);
			Noise->WaitForStreaming();
			CloudNoise.Add(Noise);
		}
	}
	// The route is sea -> storm front -> Atlantis in the fog -> portal -> maelstrom. The hell channel is
	// off that route and is skipped unless asked for with -DCHell; later phases close up the gap.
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCHell")))
	{
		for (int32 i = 0; i + 1 < Phases.Num(); ++i)
		{
			if (Phases[i].Phase == EDCPhase::Hell)
			{
				const float Gap = Phases[i + 1].StartTime - Phases[i].StartTime;
				Phases.RemoveAt(i);
				for (int32 j = i; j < Phases.Num(); ++j) { Phases[j].StartTime -= Gap; }
				break;
			}
		}
	}
	if (UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Bolt.M_Bolt")))
	{
		BoltMID = UMaterialInstanceDynamic::Create(GlowBase, this);
	}
	Flash->SetAtmosphereSunLight(false);
	// Trace the cloud volume at half resolution, not the default quarter: at quarter the edges of the
	// front against the sea and the rigging come out as blocks once DLSS has scaled the frame up.
	if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricRenderTarget.Mode"))) { C->Set(1, ECVF_SetByGameOverride); }
	// More shadow samples inside the cloud: at the default cap the shading crawled from frame to frame.
	if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricCloud.Shadow.ViewRaySampleMaxCount"))) { C->Set(256, ECVF_SetByGameOverride); }
	BuildSeaMesh();
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Sea.M_Sea"));
	if (!Base) { Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")); }
	if (Base)
	{
		SeaMID = UMaterialInstanceDynamic::Create(Base, this);
		if (UMaterialInterface* CloudBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_StormCloud.M_StormCloud")))
		{
			CloudMID = UMaterialInstanceDynamic::Create(CloudBase, this);
			Clouds->SetMaterial(CloudMID);
		}
		Sea->SetMaterial(0, SeaMID);
	}
	if (Phases.Num())
	{
		Env = Phases[0].Env;
		CurrentPhase = Phases[0].Phase;
	}
	ApplyEnv(0.f);
	SetupUDS();
	SetupOcean();
}

void ADCRideDirector::Seek(float NewTime)
{
	RideTime = FMath::Max(0.f, NewTime);
}

void ADCRideDirector::DamageShip(float Amount, bool bQuiet)
{
	ShipHealth = FMath::Max(0.f, ShipHealth - Amount);
	if (!bQuiet)
	{
		Trauma = FMath::Min(1.f, Trauma + 0.25f + Amount * 0.06f);
		OnShipHit.Broadcast(Amount);
	}
	if (!bQuiet) { UE_LOG(LogDC, Log, TEXT("Ship hit for %.0f, health %.0f"), Amount, ShipHealth); }
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

	// The last seconds in the maelstrom build to the moment the ship is flung out.
	if (Cur.Phase == EDCPhase::Maelstrom && Phases.IsValidIndex(Idx + 1))
	{
		const float Left = Phases[Idx + 1].StartTime - RideTime;
		if (Left < 10.f) { Trauma = FMath::Max(Trauma, 0.8f * (1.f - Left / 10.f)); }
	}

	// A nudge at the start for a captain who has not spoken yet.
	if (UDCCrewSubsystem* CrewSub = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		if (HintStage == 0 && RideTime > 17.f)
		{
			HintStage = 1;
			if (CrewSub->OrdersGiven <= 1) { CrewSub->Report(TEXT("All hands waiting on your word, Captain. Shall we man the guns?"), true); }
		}
		if (HintStage == 1 && RideTime > 42.f)
		{
			HintStage = 2;
			CrewSub->Report(TEXT("Fog on the horizon, Captain. It is coming on fast."), true);
		}
	}
	if (Cur.Phase != CurrentPhase)
	{
		CurrentPhase = Cur.Phase;
		UE_LOG(LogDC, Log, TEXT("Phase -> %s at %.1fs"), DCPhaseName(CurrentPhase), RideTime);
		OnPhaseChanged.Broadcast(CurrentPhase);
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			switch (CurrentPhase)
			{
			case EDCPhase::Fog: Crew->Report(TEXT("Fog bank rolling in, Captain. Unnatural thick."), true); break;
			case EDCPhase::Slide: Crew->Report(TEXT("The sea is falling away beneath us! Hold on!"), true); break;
			case EDCPhase::Hell: Crew->Report(TEXT("The water is burning! We are in hell itself!"), true); break;
			case EDCPhase::Breach: Crew->Report(TEXT("Light ahead! We are breaking through!"), true); break;
			case EDCPhase::Maelstrom: Crew->Report(TEXT("Whirlpool! And something huge moving in it!"), true); break;
			case EDCPhase::Ending: WhiteOut = 1.f; Crew->Report(TEXT("Clear water, Captain. We made it through."), true); break;
			default: break;
			}
		}
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
	// The ship is drawn down the bowl: the centre comes closer for as long as the maelstrom lasts,
	// so the slope under the ship steepens and the creature grows.
	{
		float Start = -1.f, End = -1.f;
		for (int32 i = 0; i < Phases.Num(); ++i)
		{
			if (Phases[i].Phase == EDCPhase::Maelstrom) { Start = Phases[i].StartTime; End = Phases.IsValidIndex(i + 1) ? Phases[i + 1].StartTime : Start + 160.f; }
		}
		const float Approach = Start >= 0.f ? FMath::SmoothStep(0.f, 1.f, FMath::Clamp((RideTime - Start) / FMath::Max(End - Start, 1.f), 0.f, 1.f)) : 0.f;
		MaelstromElapsed = Start >= 0.f ? FMath::Max(0.f, RideTime - Start) : 0.f;
		// A mountainous swell lifts the ship every half minute. From its crest the deck looks down the
		// slope into the pit; in the trough the walls tower. Not inside the tunnel.
		SwellLift = Env.Whirl * FMath::Clamp(1.f - Env.Tube * 3.f, 0.f, 1.f) * SwellHeight * (0.62f + 0.38f * FMath::Sin(RideTime * 0.21f));
		const float Bearing = FMath::DegreesToRadians(WhirlBearing);
		WhirlCenter = FVector2D(FMath::Cos(Bearing), FMath::Sin(Bearing)) * FMath::Lerp(WhirlStartDistance, WhirlEndDistance, Approach);
	}
	Sun->SetIntensity(Env.SunIntensity);
	Sun->SetLightColor(Env.SunColor);
	Sun->SetRelativeRotation(FRotator(Env.SunPitch, Env.SunYaw, 0.f));
	// God rays and cloud shadows scale with how much sun there is.
	Sun->SetBloomScale(0.22f * FMath::Clamp(Env.SunIntensity / 8.f, 0.f, 1.f));

	SkyLight->SetIntensity(Env.SkyIntensity);
	// Tint ambient light toward the fog so the sea does not mirror a blue sky that is not there.
	{
		const float MaxC = FMath::Max3(Env.FogColor.R, Env.FogColor.G, Env.FogColor.B);
		const FLinearColor Tint = MaxC > KINDA_SMALL_NUMBER ? Env.FogColor / MaxC : FLinearColor::White;
		SkyLight->SetLightColor(FMath::Lerp(FLinearColor::White, Tint, 0.8f));
	}
	if (CloudMID)
	{
		CloudMID->SetVectorParameterValue(TEXT("Sky"), FLinearColor(Env.CloudCover, Env.CloudBase, Env.CloudBase + 380000.f, 0.012f));
		CloudMID->SetVectorParameterValue(TEXT("Portal"), FLinearColor(PortalLocation.X, PortalLocation.Y, PortalRadius, PortalStrength));
		CloudMID->SetVectorParameterValue(TEXT("Mist"), FLinearColor(Env.Mist, 45000.f, 0.3f, 1.f));
		static const TCHAR* FaceNames[3] = { TEXT("Face0"), TEXT("Face1"), TEXT("Face2") };
		for (int32 f = 0; f < 3; ++f) { CloudMID->SetVectorParameterValue(FaceNames[f], FLinearColor(CloudFaces[f].X, CloudFaces[f].Y, CloudFaces[f].Z, CloudFaces[f].W)); }
		CloudMID->SetVectorParameterValue(TEXT("Front"), FLinearColor(StormFrontX, StormFrontStrength, StormMouthRadius, FlashTimeLeft > 0.f ? 1.f : 0.f));
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
		SeaMID->SetScalarParameterValue(TEXT("Roughness"), FMath::Lerp(0.06f, 0.65f, FMath::Clamp(Env.SeaGlow.R, 0.f, 1.f)));
		SeaMID->SetVectorParameterValue(TEXT("Color"), Env.SeaColor);
		// The tunnel is wider in the maelstrom, where it frames the view and must not fill it.
		SeaMID->SetVectorParameterValue(TEXT("Shape"), FLinearColor(Env.Tube, TubeRadius * (1.f + 0.9f * Env.Whirl), Env.Drop, SwellLift));
		SeaMID->SetVectorParameterValue(TEXT("ScatterColor"), FLinearColor(0.01f, 0.085f, 0.105f) * (Env.SunIntensity * 0.03f) * (1.f - FMath::Min(Env.SeaGlow.R, 1.f)));
		SeaMID->SetVectorParameterValue(TEXT("Whirl"), FLinearColor(WhirlCenter.X, WhirlCenter.Y, Env.Whirl * WhirlDepth, WhirlRadius));
		SeaMID->SetVectorParameterValue(TEXT("Crater"), FLinearColor(CraterRim, Env.Crater * CraterCliff, CraterWidth, Env.TubeMouth));
	}
	// Thrown clear of the maelstrom: the screen whites out, then the calm sea fades in.
	WhiteOut = FMath::Max(0.f, WhiteOut - DeltaSeconds / 3.5f);
	// Under the pack an overcast sky already takes most of the sun away, so the dim phases are opened up.
	PostProcess->Settings.AutoExposureBias = Env.Exposure + (bUDS ? 2.2f * (1.f - FMath::Clamp(Env.SunIntensity / 7.f, 0.f, 1.f)) : 0.f) + 7.f * WhiteOut * WhiteOut;
	if (bUDS) { TickUDS(); }
	TickWetness(DeltaSeconds);

	// Sea motion: the world rocks, the ship does not.
	const float T = RideTime;
	// Inside the tunnel the walls are close, so the sea rocks less there: a hard roll swings one wall in against the rail.
	const float A = Env.SeaRoughness * (1.f - 0.6f * Env.Tube);
	const float Roll = A * (FMath::Sin(T * 0.55f) + 0.35f * FMath::Sin(T * 1.3f + 1.f));
	const float Pitch = A * 0.45f * (FMath::Sin(T * 0.4f + 2.f) + 0.3f * FMath::Sin(T * 1.1f));
	const float Heave = A * 18.f * FMath::Sin(T * 0.7f + 0.5f);
	// The plugin sea and the sky pack cannot be rocked, and ruins swaying against a level sea look
	// wrong. While that sea is shown the world stays still and the captain's view leans a little instead.
	const float RockScale = Ocean ? FMath::Clamp(FMath::Max(Env.Tube, Env.Whirl) * 2.f, 0.f, 1.f) : 1.f;
	WorldRock = FTransform(FRotator(Pitch * RockScale, 0.f, Roll * RockScale), FVector(0.f, 0.f, Heave * RockScale));
	DeckSway = FRotator(Pitch * 0.3f, 0.f, Roll * 0.3f) * (1.f - RockScale);
	// On the wall of the maelstrom the sea slopes toward the centre. Tilt the whole sea the other way so
	// the water is level under the ship and the funnel, not the deck, is what leans.
	const float R0 = WhirlCenter.Size();
	const float K = (R0 * R0) / (WhirlRadius * WhirlRadius);
	const float Slope = Env.Whirl * WhirlDepth * 2.f * R0 / (WhirlRadius * WhirlRadius) / FMath::Square(1.f + K);
	// Height falls toward the centre, so the untilted surface normal leans toward it; rotate that normal upright.
	const FVector2D ToCentre = WhirlCenter.GetSafeNormal();
	const FQuat Level = FQuat::FindBetweenNormals(FVector(ToCentre.X * Slope, ToCentre.Y * Slope, 1.f).GetSafeNormal(), FVector::UpVector);
	Sea->SetRelativeTransform(FTransform(WorldRock.GetRotation() * Level, FVector(0.f, 0.f, SeaLevelZ + Heave * RockScale)));
	if (Ocean) { TickOcean(); }
	if (Ocean) { Sea->SetVisibility(Env.Tube > 0.02f || Env.Whirl > 0.02f); Ocean->SetActorHiddenInGame(Env.Tube > 0.5f || Env.Whirl > 0.5f); }

	TickLamps();
	Trauma = FMath::Max(0.f, Trauma - DeltaSeconds * 1.4f);

	// Lightning.
	if (FlashTimeLeft > 0.f)
	{
		FlashTimeLeft -= DeltaSeconds;
		Flash->SetIntensity(FlashTimeLeft > 0.f ? FMath::FRandRange(2.f, 7.f) : 0.f);
		if (FlashTimeLeft <= 0.f) { Bolt->SetVisibility(false); }
		else if (BoltMID) { BoltMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.75f, 0.85f, 1.f) * FMath::FRandRange(40.f, 160.f)); }
	}
	else if (Env.LightningRate > 0.01f && !bUDS)
	{
		NextFlashIn -= DeltaSeconds;
		if (NextFlashIn <= 0.f)
		{
			FlashTimeLeft = FMath::FRandRange(0.08f, 0.35f);
			if (FParse::Param(FCommandLine::Get(), TEXT("DCLightningTest"))) { FlashTimeLeft = 4.f; }
			BuildBolt();
			Bolt->SetVisibility(true);
			Trauma = FMath::Min(1.f, Trauma + 0.12f);
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
	const int32 Rings = 320;
	const int32 Segments = 256;
	const float R0 = 500.f;
	const float Growth = 1.03f;

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

USceneComponent* ADCRideDirector::GetSea() const
{
	return Sea;
}

FVector ADCRideDirector::GetWhirlFloorLocal() const
{
	const float K = WhirlCenter.SizeSquared() / (WhirlRadius * WhirlRadius);
	return FVector(WhirlCenter.X, WhirlCenter.Y, -Env.Whirl * WhirlDepth * (1.f - 1.f / (1.f + K)) - SwellLift);
}

void ADCRideDirector::AddLamp(const FVector& Location)
{
	UPointLightComponent* Lamp = NewObject<UPointLightComponent>(this);
	Lamp->SetMobility(EComponentMobility::Movable);
	Lamp->SetupAttachment(RootComponent);
	Lamp->SetWorldLocation(Location);
	Lamp->SetLightColor(FLinearColor(1.f, 0.62f, 0.28f));
	Lamp->SetAttenuationRadius(2600.f);
	// Each lantern lights the mist round it: a warm halo in the fog.
	Lamp->SetVolumetricScatteringIntensity(0.45f);
	Lamp->SetSourceRadius(12.f);
	Lamp->SetIntensity(0.f);
	// No shadows from the lanterns: four point lights re-rendering the whole rig every frame cost more than they showed.
	Lamp->SetCastShadows(false);
	Lamp->RegisterComponent();
	Lamps.Add(Lamp);
}

void ADCRideDirector::TickLamps()
{
	// Lanterns come up as the daylight goes, and gutter in the wind.
	// Lit whenever the light is low or there is mist to glow in.
	const float Dark = FMath::Max(FMath::Clamp(1.15f - Env.SunIntensity / 3.5f, 0.f, 1.f), FMath::Clamp(Env.Mist * 1.4f, 0.f, 1.f));
	for (int32 i = 0; i < Lamps.Num(); ++i)
	{
		const float Flicker = 0.85f + 0.15f * FMath::Sin(RideTime * 9.f + i * 2.1f) * FMath::Sin(RideTime * 5.3f + i);
		Lamps[i]->SetIntensity(LampIntensity * Dark * Flicker);
	}
	for (UMaterialInstanceDynamic* Glass : LampGlows)
	{
		Glass->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.5f, 0.14f) * (0.2f + 24.f * Dark));
	}
}

void ADCRideDirector::AdoptLampGlass(AActor* Ship)
{
	// The lantern glass on the ship becomes the flame: every glass pane is given a self-lit material
	// that the lamps drive, so the lanterns themselves glow and bloom exactly where they hang.
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Bolt.M_Bolt"));
	if (!Ship || !Base) { return; }
	TInlineComponentArray<UStaticMeshComponent*> Parts(Ship);
	for (UStaticMeshComponent* Part : Parts)
	{
		for (int32 s = 0; s < Part->GetNumMaterials(); ++s)
		{
			const UMaterialInterface* M = Part->GetMaterial(s);
			if (M && M->GetName().Contains(TEXT("Lamp")) && M->GetName().Contains(TEXT("Glass")))
			{
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
				Part->SetMaterial(s, MID);
				LampGlows.Add(MID);
			}
		}
	}
	UE_LOG(LogDC, Log, TEXT("Lanterns: %d glass panes lit"), LampGlows.Num());

	// Wet weather: every ship material built on the pack's master (it has a roughness range) gets one
	// dynamic instance, shared by all the parts that use it, which TickWetness drives.
	TMap<UMaterialInterface*, UMaterialInstanceDynamic*> Shared;
	for (UStaticMeshComponent* Part : Parts)
	{
		for (int32 s = 0; s < Part->GetNumMaterials(); ++s)
		{
			UMaterialInterface* M = Part->GetMaterial(s);
			if (!M || M->IsA<UMaterialInstanceDynamic>()) { continue; }
			UMaterialInstanceDynamic*& MID = Shared.FindOrAdd(M);
			if (!MID)
			{
				float RoughMin = 0.f, RoughMax = 0.f;
				if (!M->GetScalarParameterValue(FName(TEXT("2_Roughness_max")), RoughMax) || !M->GetScalarParameterValue(FName(TEXT("2_Roughness_min")), RoughMin)) { Shared.Remove(M); continue; }
				MID = UMaterialInstanceDynamic::Create(M, this);
				FWetMaterial W;
				W.MID = MID; W.RoughMin = RoughMin; W.RoughMax = RoughMax; W.Bright = 1.f; W.Spec = 0.5f;
				M->GetScalarParameterValue(FName(TEXT("1_Brightness")), W.Bright);
				M->GetScalarParameterValue(FName(TEXT("Specular_Amount")), W.Spec);
				WetMaterials.Add(W);
				WetMIDs.Add(MID);
			}
			Part->SetMaterial(s, MID);
		}
	}
	UE_LOG(LogDC, Log, TEXT("Ship: %d materials made wettable"), WetMaterials.Num());
}

void ADCRideDirector::TickWetness(float DeltaSeconds)
{
	// Rain soaks the ship in a few seconds, and so does the spray in the tunnel and the maelstrom; it dries slowly.
	float Target = FMath::Clamp(FMath::Max3(Env.Rain * 2.5f, Env.Tube, Env.Whirl * 0.8f), 0.f, 1.f);
	FParse::Value(FCommandLine::Get(), TEXT("DCWet="), Target);
	Wetness = FMath::FInterpTo(Wetness, Target, DeltaSeconds, Target > Wetness ? 0.35f : 0.04f);
	if (FMath::Abs(Wetness - WetApplied) < 0.01f) { return; }
	WetApplied = Wetness;
	for (const FWetMaterial& W : WetMaterials)
	{
		if (UMaterialInstanceDynamic* MID = W.MID.Get())
		{
			MID->SetScalarParameterValue(FName(TEXT("2_Roughness_min")), FMath::Lerp(W.RoughMin, FMath::Min(W.RoughMin, 0.07f), Wetness));
			MID->SetScalarParameterValue(FName(TEXT("2_Roughness_max")), FMath::Lerp(W.RoughMax, FMath::Min(W.RoughMax, 0.36f), Wetness));
			MID->SetScalarParameterValue(FName(TEXT("1_Brightness")), W.Bright * FMath::Lerp(1.f, 0.68f, Wetness));
			MID->SetScalarParameterValue(FName(TEXT("Specular_Amount")), FMath::Lerp(W.Spec, FMath::Max(W.Spec, 0.9f), Wetness));
		}
	}
}

void ADCRideDirector::BuildBolt()
{
	// A jagged ribbon from the cloud base to the sea, a few kilometres off, with a couple of forks.
	const float Angle = FMath::FRandRange(0.f, 2.f * UE_PI);
	const float Dist = FMath::FRandRange(60000.f, 260000.f);
	const FVector Foot(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, SeaLevelZ);
	const FVector Side = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f); // across the line of sight
	const float Top = Dist * 0.55f + 30000.f;
	const float Width = Dist * 0.0022f;

	TArray<FVector> Verts;
	TArray<int32> Tris;
	auto Ribbon = [&](FVector From, const FVector& To, int32 Steps, float W, float Jitter)
	{
		FVector Prev = From;
		for (int32 i = 1; i <= Steps; ++i)
		{
			FVector P = FMath::Lerp(From, To, static_cast<float>(i) / Steps);
			if (i < Steps) { P += Side * FMath::FRandRange(-Jitter, Jitter) + FVector(0.f, 0.f, FMath::FRandRange(-Jitter, Jitter) * 0.3f); }
			const int32 B = Verts.Num();
			Verts.Append({ Prev - Side * W, Prev + Side * W, P - Side * W, P + Side * W });
			Tris.Append({ B, B + 2, B + 1, B + 1, B + 2, B + 3 });
			Prev = P;
		}
		return Prev;
	};
	const FVector Head = Foot + FVector(0.f, 0.f, Top) + Side * FMath::FRandRange(-0.2f, 0.2f) * Top;
	Ribbon(Head, Foot, 16, Width, Top * 0.045f);
	for (int32 f = 0; f < 3; ++f)
	{
		// Forks leave the main stroke part way down and die out before the sea.
		const float T = FMath::FRandRange(0.2f, 0.7f);
		const FVector Start = FMath::Lerp(Head, Foot, T);
		const FVector End = Start + Side * FMath::FRandRange(-0.35f, 0.35f) * Top - FVector(0.f, 0.f, Top * FMath::FRandRange(0.15f, 0.35f));
		Ribbon(Start, End, 7, Width * 0.45f, Top * 0.03f);
	}
	TArray<FVector> Normals;
	Normals.Init(FVector::UpVector, Verts.Num());
	Bolt->ClearAllMeshSections();
	Bolt->CreateMeshSection(0, Verts, Tris, Normals, TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	if (BoltMID) { Bolt->SetMaterial(0, BoltMID); }
}

namespace
{
	void SkyNum(UObject* Obj, const TCHAR* Name, double Value)
	{
		if (!Obj) { return; }
		if (FDoubleProperty* P = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name)) { P->SetPropertyValue_InContainer(Obj, Value); }
		else if (FIntProperty* I = FindFProperty<FIntProperty>(Obj->GetClass(), Name)) { I->SetPropertyValue_InContainer(Obj, static_cast<int32>(Value)); }
		else if (FByteProperty* By = FindFProperty<FByteProperty>(Obj->GetClass(), Name)) { By->SetPropertyValue_InContainer(Obj, static_cast<uint8>(Value)); }
		else { UE_LOG(LogDC, Warning, TEXT("UDS: no number '%s' on %s"), Name, *Obj->GetClass()->GetName()); }
	}
	/** Test hook: -DCSky="Name=1.5;Other Name=0" (and -DCWeather=, -DCStorm=) sets pack variables by display name before the actor finishes spawning. */
	void SkyOverrides(UObject* Obj, const TCHAR* Switch)
	{
		FString All;
		if (!Obj || !FParse::Value(FCommandLine::Get(), Switch, All, false)) { return; }
		TArray<FString> Pairs;
		All.ParseIntoArray(Pairs, TEXT(";"));
		for (const FString& Pair : Pairs)
		{
			FString Name, Value;
			if (!Pair.Split(TEXT("="), &Name, &Value)) { continue; }
			if (FBoolProperty* B = FindFProperty<FBoolProperty>(Obj->GetClass(), *Name)) { B->SetPropertyValue_InContainer(Obj, Value.ToBool() || Value == TEXT("1")); }
			else { SkyNum(Obj, *Name, FCString::Atod(*Value)); }
		}
	}
	/** Calls a blueprint function by its display name, filling its first vector parameter (if it has one). */
	void SkyCall(UObject* Obj, const TCHAR* Name, const FVector& V = FVector::ZeroVector)
	{
		if (!Obj) { return; }
		UFunction* Fn = Obj->FindFunction(FName(Name));
		if (!Fn) { UE_LOG(LogDC, Warning, TEXT("UDS: no function '%s' on %s"), Name, *Obj->GetClass()->GetName()); return; }
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Fn->ParmsSize, 1)));
		FMemory::Memzero(Params, Fn->ParmsSize);
		for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It) { It->InitializeValue_InContainer(Params); }
		for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			FStructProperty* SP = CastField<FStructProperty>(*It);
			if (SP && SP->Struct == TBaseStructure<FVector>::Get()) { *SP->ContainerPtrToValuePtr<FVector>(Params) = V; break; }
		}
		Obj->ProcessEvent(Fn, Params);
		for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It) { It->DestroyValue_InContainer(Params); }
	}
	void SkyFlag(UObject* Obj, const TCHAR* Name, bool bValue)
	{
		if (!Obj) { return; }
		if (FBoolProperty* P = FindFProperty<FBoolProperty>(Obj->GetClass(), Name)) { P->SetPropertyValue_InContainer(Obj, bValue); }
		else { UE_LOG(LogDC, Warning, TEXT("UDS: no flag '%s' on %s"), Name, *Obj->GetClass()->GetName()); }
	}
}

void ADCRideDirector::SetupUDS()
{
	// Ultra Dynamic Sky (Content/UltraDynamicSky, a Fab pack that is not in the repo). When it is
	// installed it takes over the sun, sky light, atmosphere, height fog, clouds and weather, and
	// the ride drives it. Without it, or with -DCNoUDS, the director's own sky is used.
	// -DCNoUDS goes back to the director's own sky (which also has the storm front, portal and cloud faces).
	if (FParse::Param(FCommandLine::Get(), TEXT("DCNoUDS"))) { return; }
	UClass* SkyClass = LoadClass<AActor>(nullptr, TEXT("/Game/UltraDynamicSky/Blueprints/Ultra_Dynamic_Sky.Ultra_Dynamic_Sky_C"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UClass* WeatherClass = LoadClass<AActor>(nullptr, TEXT("/Game/UltraDynamicSky/Blueprints/Ultra_Dynamic_Weather.Ultra_Dynamic_Weather_C"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!SkyClass || !WeatherClass) { return; }

	// The engine renders one sky atmosphere and one cloud layer: the director's must be gone, not
	// just hidden, before the pack's are registered.
	Sun->UnregisterComponent();
	SkyLight->UnregisterComponent();
	Atmosphere->UnregisterComponent();
	Fog->UnregisterComponent();
	Clouds->UnregisterComponent();
	UDSSky = GetWorld()->SpawnActorDeferred<AActor>(SkyClass, FTransform::Identity);
	SkyFlag(UDSSky, TEXT("Manually Position Sun Target"), true);
	SkyFlag(UDSSky, TEXT("Use Volumetric Fog"), true);
	// Far enough for shafts between ruins a few hundred metres off, and strong enough to see.
	SkyNum(UDSSky, TEXT("Volumetric Fog Distance"), 40000.0);
	SkyNum(UDSSky, TEXT("Sun Volumetric Scattering Intensity"), 3.0);
	SkyFlag(UDSSky, TEXT("Enable Sun Light Shaft Bloom"), true);
	// The director keeps its own fixed exposure per phase, so dark scenes stay dark.
	SkyFlag(UDSSky, TEXT("Apply Exposure Settings"), false);
	SkyNum(UDSSky, TEXT("Time of Day"), 1300.0);
	// A low cloud base and a deep layer: the clouds stand up as towers instead of lying as a flat deck.
	SkyNum(UDSSky, TEXT("Bottom Altitude"), 0.2);
	SkyNum(UDSSky, TEXT("Layer Height Scale"), 4.0);
	// Denser and less eaten away: solid heads with crisp edges instead of loose popcorn.
	SkyNum(UDSSky, TEXT("3D Erosion Intensity"), 0.7);
	SkyNum(UDSSky, TEXT("High Frequency Noise Amount"), 0.1);
	SkyNum(UDSSky, TEXT("Extinction Scale"), 20.0);
	SkyOverrides(UDSSky, TEXT("DCSky="));
	UGameplayStatics::FinishSpawningActor(UDSSky, FTransform::Identity);

	UDSWeather = GetWorld()->SpawnActorDeferred<AActor>(WeatherClass, FTransform::Identity);
	for (const TCHAR* Name : { TEXT("Cloud Coverage - Manual Override"), TEXT("Rain - Manual Override"), TEXT("Thunder/Lightning - Manual Override"), TEXT("Wind Intensity - Manual Override"), TEXT("Fog - Manual Override"), TEXT("Material Wetness - Manual Override") })
	{
		SkyFlag(UDSWeather, Name, true);
	}
	// Off: at these resolutions the drops on the lens read as dirt on the picture, not as rain.
	SkyFlag(UDSWeather, TEXT("Enable Screen Droplets"), false);
	SkyOverrides(UDSWeather, TEXT("DCWeather="));
	UGameplayStatics::FinishSpawningActor(UDSWeather, FTransform::Identity);

	// The storm front: the pack's radial storm, a towering mass of cloud with rain, fog and lightning
	// under it, which stands on the horizon ahead and closes on the ship (TickUDS moves it).
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCNoStorm")))
	{
		if (UClass* StormClass = LoadClass<AActor>(nullptr, TEXT("/Game/UltraDynamicSky/Blueprints/Weather_Effects/Radial_Storm.Radial_Storm_C"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			FParse::Value(FCommandLine::Get(), TEXT("DCStormRadius="), StormRadius);
			const FTransform At(FVector(1500000.f + StormRadius, 0.f, SeaLevelZ));
			UDSStorm = GetWorld()->SpawnActorDeferred<AActor>(StormClass, At);
			SkyNum(UDSStorm, TEXT("Outer Radius"), StormRadius);
			SkyNum(UDSStorm, TEXT("Lightning Spawn Rate"), 1.5);
			SkyNum(UDSStorm, TEXT("Cloud Altitude Offset"), -1.0);
			SkyOverrides(UDSStorm, TEXT("DCStorm="));
			UGameplayStatics::FinishSpawningActor(UDSStorm, At);
		}
	}

	// The pack loads its rain and lightning effects the moment they are first wanted, which froze the
	// game for a quarter of a second as the ship entered the front. Load them now, under the title.
	for (const TCHAR* Path : { TEXT("/Game/UltraDynamicSky/Particles/Rain.Rain"), TEXT("/Game/UltraDynamicSky/Particles/Obscured_Lightning.Obscured_Lightning"), TEXT("/Game/UltraDynamicSky/Particles/Lightning_Strike.Lightning_Strike") })
	{
		if (UObject* Loaded = LoadObject<UObject>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet)) { UDSPreloaded.Add(Loaded); }
	}

	bUDS = true;
	UE_LOG(LogDC, Log, TEXT("Sky: Ultra Dynamic Sky in use%s"), UDSStorm ? TEXT(", with its radial storm as the front") : TEXT(""));
}

void ADCRideDirector::TickUDS()
{
	// The director's light points down along its pitch; the pack wants a point to hang the sun on.
	if (FStructProperty* Target = FindFProperty<FStructProperty>(UDSSky->GetClass(), TEXT("Sun Target")))
	{
		const FVector ToSun = -FRotator(Env.SunPitch, Env.SunYaw, 0.f).Vector();
		*Target->ContainerPtrToValuePtr<FVector>(UDSSky.Get()) = ToSun * 1000000.0;
	}
	SkyNum(UDSWeather, TEXT("Cloud Coverage"), FMath::Clamp(2.0 + 8.0 * Env.CloudCover, 0.0, 10.0));
	SkyNum(UDSWeather, TEXT("Fog"), FMath::Clamp(Env.FogDensity * 250.0 + Env.Mist * 3.0, 0.0, 10.0));
	SkyNum(UDSWeather, TEXT("Rain"), FMath::Clamp(Env.Rain * 10.0, 0.0, 10.0));
	SkyNum(UDSWeather, TEXT("Wind Intensity"), FMath::Clamp(Env.WaveHeight / 30.0, 1.0, 10.0));
	// The pack's own lightning (bolts, flashes in the cloud, thunder) replaces the director's.
	SkyNum(UDSWeather, TEXT("Thunder/Lightning"), FMath::Clamp(Env.LightningRate * 0.45, 0.0, 10.0));
	SkyNum(UDSWeather, TEXT("Material Wetness"), FMath::Clamp(Env.Rain * 3.0, 0.0, 1.0));
	SkyNum(UDSSky, TEXT("Sun Light Intensity"), Env.SunIntensity * 0.75 + 2.0);
	SkyNum(UDSSky, TEXT("Sky Light Intensity"), Env.SkyIntensity);
	if (UDSStorm)
	{
		// Its edge is at the front's face. Once it has closed over the ship it draws off astern at
		// more than a kilometre a second, so the drowned city beyond it stands in broken light.
		const float Face = StormFrontX - FMath::Max(0.f, RideTime - 66.f) * 130000.f;
		SkyCall(UDSStorm, TEXT("Set Storm Location"), FVector(Face + StormRadius * 0.85f, 0.f, SeaLevelZ));
	}
}

void ADCRideDirector::SetupOcean()
{
	// Dynamic Real Water (an engine-installed Fab plugin, optional). Trial: -DCDRW puts its stock
	// ocean at sea level and hides the director's own sea outside the tunnel and the maelstrom.
	if (FParse::Param(FCommandLine::Get(), TEXT("DCNoDRW"))) { return; }
	UClass* OceanClass = LoadClass<AActor>(nullptr, TEXT("/DynamicRealWater/Blueprints/BP_DynamicRealWater.BP_DynamicRealWater_C"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!OceanClass) { UE_LOG(LogDC, Warning, TEXT("Ocean: Dynamic Real Water not found")); return; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ocean = GetWorld()->SpawnActorDeferred<AActor>(OceanClass, FTransform(FVector(0.f, 0.f, SeaLevelZ)));
	// The waves run from the bow aft, which reads as the ship making way through them.
	if (FStructProperty* Wind = FindFProperty<FStructProperty>(OceanClass, TEXT("WindDirection"))) { *Wind->ContainerPtrToValuePtr<FVector2D>(Ocean.Get()) = FVector2D(-1.0, 0.2); }
	UGameplayStatics::FinishSpawningActor(Ocean, FTransform(FVector(0.f, 0.f, SeaLevelZ)));
	UE_LOG(LogDC, Log, TEXT("Ocean: Dynamic Real Water in use (%s)"), Ocean ? *Ocean->GetClass()->GetName() : TEXT("spawn failed"));
}

void ADCRideDirector::TickOcean()
{
	// The plugin sea follows the ride's weather: a fresh breeze on the open sea, a gale in the storm.
	// Its wave state is an asset; a copy of the stock one is given the new wind and blended in.
	const float Wind = FMath::Clamp(9.f + Env.WaveHeight * 0.1f, 5.f, 60.f);
	if (FMath::Abs(Wind - OceanWind) < 2.f) { return; }
	UClass* Library = FindObject<UClass>(nullptr, TEXT("/Script/DynamicRealWater.DRW_BlueprintLibrary"));
	UObject* Stock = LoadObject<UObject>(nullptr, TEXT("/DynamicRealWater/Blueprints/WaterStates/Waves/DA_DefaultWaves.DA_DefaultWaves"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UFunction* Fn = Library ? Library->FindFunctionByName(TEXT("SetOceanWaveData")) : nullptr;
	if (!Fn || !Stock) { OceanWind = Wind; return; }
	const bool bFirst = OceanWind < 0.f;
	OceanWind = Wind;
	OceanWaves = DuplicateObject<UObject>(Stock, this);
	if (FFloatProperty* P = FindFProperty<FFloatProperty>(OceanWaves->GetClass(), TEXT("WindSpeed"))) { P->SetPropertyValue_InContainer(OceanWaves.Get(), Wind); }
	if (FFloatProperty* P = FindFProperty<FFloatProperty>(OceanWaves->GetClass(), TEXT("Choppiness"))) { P->SetPropertyValue_InContainer(OceanWaves.Get(), FMath::GetMappedRangeValueClamped(FVector2f(15.f, 35.f), FVector2f(1.f, 1.5f), Wind)); }
	// The waves run from the bow aft, which reads as the ship making way through them.
	if (FStructProperty* P = FindFProperty<FStructProperty>(OceanWaves->GetClass(), TEXT("WindDirection"))) { *P->ContainerPtrToValuePtr<FVector2D>(OceanWaves.Get()) = FVector2D(-1.0, 0.2); }
	// The sea streams aft past the hull. The plugin's waves are fixed to the world and the ship stands
	// still, so the wave shader is patched (Tools/patch_drw_current.py) to shift the whole field; it
	// does so when the swell is off and its direction is exactly this. Without the patch this does nothing.
	{
		float Sign = 1.f;   // +1 streams aft (measured with Tools/flow.ps1), -1 forward, 0 off
		FParse::Value(FCommandLine::Get(), TEXT("DCCurrent="), Sign);
		if (FFloatProperty* P = FindFProperty<FFloatProperty>(OceanWaves->GetClass(), TEXT("SwellHeight"))) { P->SetPropertyValue_InContainer(OceanWaves.Get(), 0.f); }
		if (FStructProperty* P = FindFProperty<FStructProperty>(OceanWaves->GetClass(), TEXT("SwellDirection"))) { *P->ContainerPtrToValuePtr<FVector2D>(OceanWaves.Get()) = Sign == 0.f ? FVector2D(1.0, 1.0) : FVector2D(Sign, 0.0123).GetSafeNormal(); }
	}

	// Calls one of the plugin's "set ocean data" functions with an asset and a blend time.
	auto Apply = [this, Library, bFirst](UFunction* Call, UObject* Asset)
	{
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Call->ParmsSize, 1)));
		FMemory::Memzero(Params, Call->ParmsSize);
		for (TFieldIterator<FProperty> It(Call); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->InitializeValue_InContainer(Params);
			if (FObjectProperty* O = CastField<FObjectProperty>(*It))
			{
				O->SetObjectPropertyValue_InContainer(Params, It->GetName() == TEXT("WorldContextObject") ? static_cast<UObject*>(this) : Asset);
			}
			else if (FFloatProperty* F = CastField<FFloatProperty>(*It)) { F->SetPropertyValue_InContainer(Params, bFirst ? 0.01f : 10.f); }
		}
		Library->GetDefaultObject()->ProcessEvent(Call, Params);
	};
	Apply(Fn, OceanWaves);

	// The look goes with the wind: whitecaps and a greyer, rougher surface as it rises.
	UObject* StockLook = LoadObject<UObject>(nullptr, TEXT("/DynamicRealWater/Blueprints/WaterStates/Visuals/DA_DefaultVisual.DA_DefaultVisual"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UFunction* LookFn = Library->FindFunctionByName(TEXT("SetOceanVisualData"));
	if (StockLook && LookFn && !FParse::Param(FCommandLine::Get(), TEXT("DCNoSeaLook")))
	{
		const float Storm = FMath::GetMappedRangeValueClamped(FVector2f(16.f, 30.f), FVector2f(0.f, 1.f), Wind);
		float Foam = FMath::Lerp(0.12f, 0.45f, Storm);
		FParse::Value(FCommandLine::Get(), TEXT("DCSeaFoam="), Foam);
		OceanLook = DuplicateObject<UObject>(StockLook, this);
		if (FFloatProperty* P = FindFProperty<FFloatProperty>(OceanLook->GetClass(), TEXT("FoamStrength"))) { P->SetPropertyValue_InContainer(OceanLook.Get(), Foam); }
		Apply(LookFn, OceanLook);
	}
	UE_LOG(LogDC, Log, TEXT("Ocean: wind %.0f m/s at %.0f s"), Wind, RideTime);
}
