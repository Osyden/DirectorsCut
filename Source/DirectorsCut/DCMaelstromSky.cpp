#include "DCMaelstromAtmosphere.h"
#include "DCRideDirector.h"
#include "DCMaelstrom.h"
#include "DCTypes.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/TextureCube.h"
#include "Interfaces/Interface_PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#endif

namespace
{
constexpr float MaelstromSunBalanceStrength = 0.45f;

struct FMaelstromSunLook
{
	FLinearColor Color;
	float LuxScale = 1.f;
};

FMaelstromSunLook GetMaelstromSunLook()
{
	FMaelstromSunLook Look;
	Look.Color = FParse::Param(FCommandLine::Get(), TEXT("DCMaelSunNeutral"))
		? FLinearColor::White : FLinearColor(1.f, 0.94f, 0.84f);
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCMaelSunGroundBalance"))) { return Look; }
	// Fixed art-direction control for the existing 18-degree sun, based on settled
	// CliffH2Three diagnostics. Partial inverse transmission; no live feedback loop.
	// CliffH2Balanced showed that 0.85 over-cooled atmospheric/sky illumination.
	// Keep a warmer residual and bound the accompanying blue-energy increase.
	const FLinearColor Transmission(0.694727f, 0.511457f, 0.268807f);
	const FLinearColor TemperatureRGB(1.044155f, 0.983268f, 1.035693f); // observed 6500 K
	const FLinearColor Tint(
		FMath::Clamp(FMath::Pow(Transmission.B / Transmission.R, MaelstromSunBalanceStrength), 0.65f, 1.f),
		FMath::Clamp(FMath::Pow(Transmission.B / Transmission.G, MaelstromSunBalanceStrength), 0.65f, 1.f), 1.f);
	const FLinearColor Before = Look.Color * TemperatureRGB * Transmission;
	Look.Color *= Tint; // all channels remain <= 1, avoiding light-colour clipping
	const FLinearColor After = Look.Color * TemperatureRGB * Transmission;
	auto Luminance = [](FLinearColor C) { return 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B; };
	Look.LuxScale = FMath::Clamp(Luminance(Before) / FMath::Max(Luminance(After), 0.001f), 1.f, 1.4f);
	return Look;
}
}

bool ADCMaelstromAtmosphere::CompileSkyMaterial(UMaterialInterface* Material)
{
#if WITH_EDITOR
	if (!Material || !GShaderCompilingManager) { return false; }
	Material->GetMaterial()->ForceRecompileForRendering();
	GShaderCompilingManager->FinishAllCompilation();
	const FMaterialResource* Resource = Material->GetMaterial()->GetMaterialResource(GMaxRHIShaderPlatform);
	if (!Resource) { return false; }
	for (const FString& Error : Resource->GetCompileErrors()) { UE_LOG(LogDC, Error, TEXT("MAEL_SKY compile: %s"), *Error); }
	return Resource->GetCompileErrors().IsEmpty() && Resource->GetGameThreadShaderMap() != nullptr;
#else
	return false;
#endif
}

// The finale owns its composition, while the shared director continues driving
// time, weather and exposure. No changes to the purchased UDS asset are saved.
void ADCMaelstromAtmosphere::ConfigureSkyCues()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("DCOriginalMaelstromSky"))) { return; }
	// Load during startup, before the tunnel reveal. In editor-game reviews a new
	// material can otherwise still be compiling when its first capture is taken.
	SkyCloudTemplate = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/Sky/M_MaelstromCumulus.M_MaelstromCumulus"));
#if WITH_EDITOR
	if (SkyCloudTemplate)
	{
		if (FMaterialResource* Resource = SkyCloudTemplate->GetMaterialResource(GMaxRHIShaderPlatform))
		{
			Resource->FinishCompilation();
			if (!Resource->IsGameThreadShaderMapComplete()) { CompileSkyMaterial(SkyCloudTemplate); }
			UE_LOG(LogDC, Log, TEXT("MAEL_SKY startup shader complete=%d"), Resource->IsGameThreadShaderMapComplete());
		}
	}
#endif
	if (ADCRideDirector* Director = ADCRideDirector::Get(this))
	{
		for (FDCPhaseDef& Phase : Director->Phases)
		{
			if (Phase.Phase != EDCPhase::Breach && Phase.Phase != EDCPhase::Maelstrom) { continue; }
			Phase.Env.CloudCover = 0.20f;
			Phase.Env.SunPitch = -18.f;
			// Light direction points away from the source: 135 puts the sun at
			// -45 degrees, in the upper-right of the port-facing reveal camera.
			Phase.Env.SunYaw = 135.f;
			Phase.Env.SkyIntensity = 1.3f;
			Phase.Env.SunIntensity = 7.5f;
			Phase.Env.Exposure = -0.15f;
			FParse::Value(FCommandLine::Get(), TEXT("DCMaelSkyFill="), Phase.Env.SkyIntensity);
			FParse::Value(FCommandLine::Get(), TEXT("DCMaelSunEnergy="), Phase.Env.SunIntensity);
			FParse::Value(FCommandLine::Get(), TEXT("DCMaelSkyExposure="), Phase.Env.Exposure);
		}
	}
}

void ADCMaelstromAtmosphere::SetSkyNumber(const TCHAR* Name, double Value)
{
	AActor* Sky = StageSky.Get();
	if (!Sky) { return; }
	FProperty* Property = FindFProperty<FProperty>(Sky->GetClass(), Name);
	if (!Property) { UE_LOG(LogDC, Warning, TEXT("MAEL_SKY missing property %s"), Name); return; }
	if (!SavedSkyProperties.Contains(Name))
	{
		FString Before;
		Property->ExportText_InContainer(0, Before, Sky, Sky, Sky, PPF_None);
		SavedSkyProperties.Add(Name, Before);
	}
	if (FDoubleProperty* P = CastField<FDoubleProperty>(Property)) { P->SetPropertyValue_InContainer(Sky, Value); }
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property)) { FloatProp->SetPropertyValue_InContainer(Sky, Value); }
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Property)) { IntProp->SetPropertyValue_InContainer(Sky, FMath::RoundToInt(Value)); }
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property)) { BoolProp->SetPropertyValue_InContainer(Sky, Value != 0.0); }
}

void ADCMaelstromAtmosphere::SetSkyColor(const TCHAR* Name, FLinearColor Value)
{
	AActor* Sky = StageSky.Get();
	if (!Sky) { return; }
	FStructProperty* Property = FindFProperty<FStructProperty>(Sky->GetClass(), Name);
	if (!Property || Property->Struct != TBaseStructure<FLinearColor>::Get()) { return; }
	if (!SavedSkyProperties.Contains(Name))
	{
		FString Before;
		Property->ExportText_InContainer(0, Before, Sky, Sky, Sky, PPF_None);
		SavedSkyProperties.Add(Name, Before);
	}
	*Property->ContainerPtrToValuePtr<FLinearColor>(Sky) = Value;
}

void ADCMaelstromAtmosphere::RefreshSky()
{
	if (AActor* Sky = StageSky.Get())
	{
		if (GetWorld()->bIsTearingDown) { return; }
		// Runtime material parameters are cached by UDS. Refresh its inspected
		// zero-argument update functions once on entry/exit, never every frame.
		for (const TCHAR* Name : {TEXT("Update Static Variables"), TEXT("Update Active Variables")})
		{
			if (UFunction* Function = Sky->FindFunction(Name); Function && Function->ParmsSize == 0)
			{ Sky->ProcessEvent(Function, nullptr); }
		}
	}
}

void ADCMaelstromAtmosphere::CaptureSky()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("DCOriginalMaelstromSky"))) { return; }
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName() == TEXT("Ultra_Dynamic_Sky_C")) { StageSky = *It; break; }
	}
	if (!StageSky.IsValid()) { return; }
	LightingDiagnosticSamples = 0;
	NextLightingDiagnosticTime = 0.0;
	// Kilometres: keep the underside safely above the 3.2 km arena. A thick
	// layer and varied floor produce sculpted masses instead of a low flat lid.
	SetSkyNumber(TEXT("Bottom Altitude"), 1.65);
	SetSkyNumber(TEXT("Layer Height Scale"), 1.8);
	SetSkyNumber(TEXT("Volumetric Clouds Scale"), 1.15);
	SetSkyNumber(TEXT("Cloud Formation Texture Scale"), 0.8);
	SetSkyNumber(TEXT("3D Noise Scale"), 0.75);
	SetSkyNumber(TEXT("3D Erosion Intensity"), 1.15);
	SetSkyNumber(TEXT("High Frequency Noise Amount"), 0.28);
	SetSkyNumber(TEXT("High Frequency Noise Distance Scale"), 500000.0);
	SetSkyNumber(TEXT("Extinction Scale"), 14.0);
	SetSkyNumber(TEXT("Base Floor Variation"), 0.35);
	SetSkyNumber(TEXT("Overcast Floor Variation"), 1.1);
	SetSkyNumber(TEXT("2D Overcast Turbulence"), 0.8);
	SetSkyNumber(TEXT("Floor Luminance Scale"), 0.65);
	SetSkyNumber(TEXT("Volumetric Cloud Ambient Light Intensity"), 0.65);
	SetSkyColor(TEXT("Volumetric Cloud Ambient Light (Day)"), FLinearColor(0.32f, 0.40f, 0.52f));
	SetSkyNumber(TEXT("Multiscattering Light Intensity"), 0.65);
	SetSkyNumber(TEXT("Multiscattering Light Occlusion"), 0.65);
	SetSkyNumber(TEXT("View Sample Scale (Day)"), 4.0);
	SetSkyNumber(TEXT("Shadow Sample Scale"), 3.0);
	SetSkyNumber(TEXT("Shadow Tracing Distance"), 4.0);
	SetSkyNumber(TEXT("Macro Variation"), 0.35);
	SetSkyNumber(TEXT("Macro Scale"), 0.8);
	SetSkyNumber(TEXT("Cloud Speed"), 0.12);
	SetSkyNumber(TEXT("Formation Change Speed"), 0.08);
	SetSkyNumber(TEXT("Cloud Wisps Opacity (Clear)"), 0.04);
	SetSkyNumber(TEXT("Cloud Wisps Opacity (Cloudy)"), 0.08);
	// Set UDS's live sun through its saved property; RefreshSky applies/restores it.
	const bool bNeutralSun = FParse::Param(FCommandLine::Get(), TEXT("DCMaelSunNeutral"));
	const FMaelstromSunLook SunLook = GetMaelstromSunLook();
	SunGroundBalanceLuxScale = SunLook.LuxScale;
	SetSkyColor(TEXT("Sun Light Color"), SunLook.Color);
	if (bNeutralSun) { UE_LOG(LogDC, Log, TEXT("MAEL_SKY neutral UDS sun base colour selected")); }
	if (SunLook.LuxScale > 1.f)
	{
		UE_LOG(LogDC, Log, TEXT("MAEL_SKY ground-balance art preset colour=%s luxScale=%.4f strength=%.2f"), *SunLook.Color.ToString(), SunLook.LuxScale, MaelstromSunBalanceStrength);
		if (const ADCRideDirector* Director = ADCRideDirector::Get(this))
		{ SetSkyNumber(TEXT("Sun Light Intensity"), (Director->GetEnv().SunIntensity * 0.75f + 2.f) * SunGroundBalanceLuxScale); }
	}
	SetSkyNumber(TEXT("Mie Scattering Scale Multiplier"), 0.7);
	SetSkyNumber(TEXT("Enable Volumetric Cloud Light Rays"), 0);
	// Numeric test overrides support repeatable look development without a build.
	FString Overrides;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCMaelSky="), Overrides, false))
	{
		TArray<FString> Pairs; Overrides.ParseIntoArray(Pairs, TEXT(";"));
		for (const FString& Pair : Pairs)
		{
			FString Name, Value;
			if (Pair.Split(TEXT("="), &Name, &Value)) { SetSkyNumber(*Name, FCString::Atod(*Value)); }
		}
	}
	RefreshSky();
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCMaelStockClouds")))
	{
		const TCHAR* MaterialPath = FParse::Param(FCommandLine::Get(), TEXT("DCMaelEngineClouds"))
			? TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")
			: TEXT("/Game/DC/Maelstrom/Sky/M_MaelstromCumulus.M_MaelstromCumulus");
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath))
		{
			if (FParse::Param(FCommandLine::Get(), TEXT("DCMaelCompileClouds"))) { CompileSkyMaterial(Material); }
			StageCloud = StageSky->FindComponentByClass<UVolumetricCloudComponent>();
			if (StageCloud.IsValid())
			{
				PreviousCloudMaterial = StageCloud->Material.LoadSynchronous();
				PreviousCloudBottom = StageCloud->LayerBottomAltitude;
				PreviousCloudHeight = StageCloud->LayerHeight;
				PreviousCloudViewSamples = StageCloud->ViewSampleCountScale;
				PreviousCloudShadowSamples = StageCloud->ShadowViewSampleCountScale;
				PreviousCloudBottomOcclusion = StageCloud->SkyLightCloudBottomOcclusion;
				PreviousCloudGroundAlbedo = StageCloud->GroundAlbedo;
				StageCloud->SetGroundAlbedo(FColor(90, 120, 165));
				if (IConsoleVariable* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricRenderTarget.Mode")))
				{
					PreviousCloudRenderMode = Mode->GetInt();
					// Full-resolution tracing avoids bright-edge breakup in this hero vista.
					// The runtime override remains available for lower-end target hardware.
					int32 RenderMode = 3;
					FParse::Value(FCommandLine::Get(), TEXT("DCMaelCloudRenderMode="), RenderMode);
					Mode->SetWithCurrentPriority(FMath::Clamp(RenderMode,0,3));
				}
				if (IConsoleVariable* Distance = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricCloud.DistanceToSampleMaxCount")))
				{
					PreviousCloudSampleDistance = Distance->GetFloat();
					float SampleDistance = 3.f;
					FParse::Value(FCommandLine::Get(), TEXT("DCMaelCloudSampleDistance="), SampleDistance);
					Distance->SetWithCurrentPriority(FMath::Clamp(SampleDistance, 1.f, 20.f));
				}
				SkyCloudMID = UMaterialInstanceDynamic::Create(Material, this);
				// Extend selected bright openings into a restrained radial shaft pass.
				// This supplements native cloud-shadowed fog and restores on exit.
				if (IConsoleVariable* ShaftDistance = IConsoleManager::Get().FindConsoleVariable(TEXT("r.LightShaftFirstPassDistance")))
				{
					PreviousLightShaftDistance = ShaftDistance->GetFloat();
					ShaftDistance->Set(0.2f, ECVF_SetByTemp, TEXT("DCMaelstromSky"));
					UE_LOG(LogDC, Log, TEXT("MAEL_SKY shaft blur active=%.2f previous=%.2f"), ShaftDistance->GetFloat(), PreviousLightShaftDistance);
				}
				float Density = 0.035f, Erosion = 0.28f, Opening = 0.23f;
				FParse::Value(FCommandLine::Get(), TEXT("DCMaelCloudDensity="), Density);
				FParse::Value(FCommandLine::Get(), TEXT("DCMaelCloudErosion="), Erosion);
				FParse::Value(FCommandLine::Get(), TEXT("DCMaelCloudOpening="), Opening);
				SkyCloudMID->SetScalarParameterValue(TEXT("Density"), Density);
				SkyCloudMID->SetScalarParameterValue(TEXT("Erosion"), Erosion);
				SkyCloudMID->SetScalarParameterValue(TEXT("Opening"), Opening);
				float Debug = 0.f;
				FParse::Value(FCommandLine::Get(), TEXT("DCMaelCloudDebug="), Debug);
				SkyCloudMID->SetScalarParameterValue(TEXT("Debug"), Debug);
				bPreviousShafts = bEnableShafts;
				bEnableShafts = FParse::Param(FCommandLine::Get(), TEXT("DCMaelSpotShafts"));
				UpdateSkyVolume();
				UE_LOG(LogDC, Log, TEXT("MAEL_SKY custom=%s cloud=%s registered=%d visible=%d location=%s bottom=%.2f height=%.2f"),
					*GetNameSafe(StageCloud->Material.Get()), *StageCloud->GetName(), StageCloud->IsRegistered(), StageCloud->IsVisible(),
					*StageCloud->GetComponentLocation().ToCompactString(), StageCloud->LayerBottomAltitude, StageCloud->LayerHeight);
			}
		}
	}
	UE_LOG(LogDC, Log, TEXT("MAEL_SKY captured and composed %d properties on %s"), SavedSkyProperties.Num(), *StageSky->GetName());
}

void ADCMaelstromAtmosphere::UpdateSkyVolume()
{
	if (StageSky.IsValid() && SunGroundBalanceLuxScale > 1.f)
	{
		if (const ADCRideDirector* Director = ADCRideDirector::Get(this))
		{
			// Derive from unmodified weather, never from the already corrected light.
			// Match TickUDS without changing Env or its exposure consumers.
			const float Lux = (Director->GetEnv().SunIntensity * 0.75f + 2.f) * SunGroundBalanceLuxScale;
			SetSkyNumber(TEXT("Sun Light Intensity"), Lux);
			if (UDirectionalLightComponent* Sun = FindSun()) { Sun->SetIntensity(Lux); }
		}
	}
	if (SkyCloudMID && StageCloud.IsValid())
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("DCMaelSkyDiagnostics")) && GFrameCounter % 120 == 0)
		{
			UE_LOG(LogDC, Log, TEXT("MAEL_SKY state bottom=%.3f height=%.3f view=%.3f shadow=%.3f ao=%.3f dirty=%d material=%s"),
				StageCloud->LayerBottomAltitude, StageCloud->LayerHeight, StageCloud->ViewSampleCountScale,
				StageCloud->ShadowViewSampleCountScale, StageCloud->SkyLightCloudBottomOcclusion,
				StageCloud->IsRenderStateDirty(), *GetNameSafe(StageCloud->Material.Get()));
		}
		StageCloud->SetMaterial(SkyCloudMID);
		// The ship stays at the origin while the crater rises around it on descent.
		// Keep the sky above the rim rather than letting the far wall enter clouds.
		const ADCMaelstrom* Stage = ADCMaelstrom::Get(this);
		const float RimKm = Stage ? FMath::Max(0.f, static_cast<float>(Stage->GetArenaTransform().GetLocation().Z) * 0.00001f) : 0.f;
		// A fixed conservative tracing envelope avoids rebuilding the cloud proxy
		// every frame. Only density moves, smoothly, inside this envelope.
		StageCloud->SetLayerBottomAltitude(0.55f);
		StageCloud->SetLayerHeight(6.f);
		StageCloud->SetViewSampleCountScale(4.f);
		StageCloud->SetShadowViewSampleCountScale(3.f);
		StageCloud->SetSkyLightCloudBottomOcclusion(0.5f);
		SkyCloudMID->SetScalarParameterValue(TEXT("RimHeight"), RimKm);
		if (const ADCRideDirector* Director = ADCRideDirector::Get(this))
		{ SkyCloudMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime); }
		if (UDirectionalLightComponent* Sun = FindSun())
		{ SkyCloudMID->SetVectorParameterValue(TEXT("ToSun"), FLinearColor(-Sun->GetForwardVector())); }
		if (SavedSun.Component.IsValid() && FParse::Param(FCommandLine::Get(), TEXT("DCMaelLightingDiagnostics"))
			&& LightingDiagnosticSamples < 3 && GetWorld()->GetTimeSeconds() >= NextLightingDiagnosticTime)
		{
			++LightingDiagnosticSamples;
			NextLightingDiagnosticTime = GetWorld()->GetTimeSeconds() + 2.0;
			LogLightingDiagnostics();
		}
	}
}

void ADCMaelstromAtmosphere::LogLightingDiagnostics()
{
	// Read-only CPU diagnostics after stage lighting is applied. No recapture or
	// renderer overrides; repeat after entry so UDS's subsequent updates are visible.
	UDirectionalLightComponent* Sun = FindSun();
	if (!Sun) { return; }
	const FLinearColor Energy = Sun->GetColoredLightBrightness();
	UE_LOG(LogDC, Log, TEXT("MAEL_LIGHT sample=%d sun=%s rgb=%s useTemperature=%d kelvin=%.0f temperatureRGB=%s energyBeforeAtmosphere=%s perPixelTransmittance=%d"),
		LightingDiagnosticSamples, *Sun->GetPathName(), *Sun->GetLightColor().ToString(), Sun->bUseTemperature,
		Sun->Temperature, *Sun->GetColorTemperature().ToString(), *Energy.ToString(), Sun->bPerPixelAtmosphereTransmittance);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<USkyAtmosphereComponent*> Atmospheres(*It);
		for (USkyAtmosphereComponent* Atmosphere : Atmospheres)
		{
			if (!Atmosphere->IsRegistered() || !Atmosphere->IsVisible() || Atmosphere->bHiddenInGame) { continue; }
			// Engine estimate at planet-top ground, not the view's per-pixel result.
			const FLinearColor Transmission = Atmosphere->GetAtmosphereTransmitanceOnGroundAtPlanetTop(Sun);
			UE_LOG(LogDC, Log, TEXT("MAEL_LIGHT atmosphere=%s groundTransmissionEstimate=%s groundEnergyEstimate=%s"),
				*Atmosphere->GetPathName(), *Transmission.ToString(), *(Energy * Transmission).ToString());
		}
		TInlineComponentArray<USkyLightComponent*> Fills(*It);
		for (USkyLightComponent* Fill : Fills)
		{
			if (!Fill->IsRegistered() || !Fill->IsVisible() || Fill->bHiddenInGame || !Fill->bAffectsWorld) { continue; }
			UE_LOG(LogDC, Log, TEXT("MAEL_LIGHT skylight=%s intensity=%.3f rgb=%s realtime=%d sourceType=%d cubemap=%s cloudAO=%d"),
				*Fill->GetPathName(), Fill->Intensity, *Fill->GetLightColor().ToString(), Fill->bRealTimeCapture,
				static_cast<int32>(Fill->SourceType.GetValue()), *GetPathNameSafe(Fill->Cubemap.Get()), Fill->bCloudAmbientOcclusion);
		}
	}
	// Log candidate grading sources and override bits, not a claimed final blend.
	auto LogGrade = [](const FString& Name, const FPostProcessSettings& P, float Weight)
	{
		UE_LOG(LogDC, Log, TEXT("MAEL_LIGHT gradeSource=%s weight=%.3f temperatureType=%d/%d whiteTemp=%d/%.0f whiteTint=%d/%.3f sceneTint=%d/%s LUT=%d/%s LUTIntensity=%.3f"),
			*Name, Weight, P.bOverride_TemperatureType, static_cast<int32>(P.TemperatureType.GetValue()),
			P.bOverride_WhiteTemp, P.WhiteTemp, P.bOverride_WhiteTint, P.WhiteTint,
			P.bOverride_SceneColorTint, *P.SceneColorTint.ToString(), P.bOverride_ColorGradingLUT,
			*GetPathNameSafe(P.ColorGradingLUT.Get()), P.ColorGradingIntensity);
		UE_LOG(LogDC, Log, TEXT("MAEL_LIGHT gradeSource=%s gain=%d/%s midGain=%d/%s shadowGain=%d/%s highlightGain=%d/%s gamma=%d/%s offset=%d/%s"),
			*Name, P.bOverride_ColorGain, *P.ColorGain.ToString(), P.bOverride_ColorGainMidtones, *P.ColorGainMidtones.ToString(),
			P.bOverride_ColorGainShadows, *P.ColorGainShadows.ToString(), P.bOverride_ColorGainHighlights, *P.ColorGainHighlights.ToString(),
			P.bOverride_ColorGamma, *P.ColorGamma.ToString(), P.bOverride_ColorOffset, *P.ColorOffset.ToString());
		for (const FWeightedBlendable& Blend : P.WeightedBlendables.Array)
		{
			if (Blend.Weight > 0.f && Blend.Object)
			{ UE_LOG(LogDC, Log, TEXT("MAEL_LIGHT gradeSource=%s blendable=%s weight=%.3f"), *Name, *Blend.Object->GetPathName(), Blend.Weight); }
		}
	};
	APlayerController* Player = GetWorld()->GetFirstPlayerController();
	APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
	if (Camera)
	{
		const FMinimalViewInfo& View = Camera->GetCameraCacheView();
		LogGrade(TEXT("CameraCache"), View.PostProcessSettings, View.PostProcessBlendWeight);
		const TArray<FPostProcessSettings>* Blends = nullptr;
		const TArray<float>* Weights = nullptr;
		Camera->GetCachedPostProcessBlends(Blends, Weights);
		if (Blends && Weights)
		{
			for (int32 I = 0; I < FMath::Min(Blends->Num(), Weights->Num()); ++I)
			{ LogGrade(FString::Printf(TEXT("CameraBlend%d"), I), (*Blends)[I], (*Weights)[I]); }
		}
	}
	int32 Index = 0;
	for (IInterface_PostProcessVolume& Volume : GetWorld()->GetPostProcessVolumeIterator())
	{
		const FPostProcessVolumeProperties Properties = Volume.GetProperties();
		if (!Properties.bIsEnabled || !Properties.Settings || Properties.BlendWeight <= 0.f) { continue; }
		float Distance = -1.f;
		if (Camera && !Properties.bIsUnbound) { Volume.EncompassesPoint(Camera->GetCameraLocation(), 0.f, &Distance); }
		FString Name = FString::Printf(TEXT("Volume%d"), Index++);
#if DEBUG_POST_PROCESS_VOLUME_ENABLE
		Name = Volume.GetDebugName();
#endif
		UE_LOG(LogDC, Log, TEXT("MAEL_LIGHT volume=%s priority=%.2f unbound=%d cameraDistance=%.1f blendRadius=%.1f"),
			*Name, Properties.Priority, Properties.bIsUnbound, Distance, Properties.BlendRadius);
		LogGrade(Name, *Properties.Settings, Properties.BlendWeight);
	}
}

void ADCMaelstromAtmosphere::RestoreSky()
{
	if (StageCloud.IsValid() && SkyCloudMID)
	{
		StageCloud->SetMaterial(PreviousCloudMaterial);
		StageCloud->SetLayerBottomAltitude(PreviousCloudBottom);
		StageCloud->SetLayerHeight(PreviousCloudHeight);
		StageCloud->SetViewSampleCountScale(PreviousCloudViewSamples);
		StageCloud->SetShadowViewSampleCountScale(PreviousCloudShadowSamples);
		StageCloud->SetSkyLightCloudBottomOcclusion(PreviousCloudBottomOcclusion);
		StageCloud->SetGroundAlbedo(PreviousCloudGroundAlbedo);
		bEnableShafts = bPreviousShafts;
	}
	SkyCloudMID = nullptr;
	PreviousCloudMaterial = nullptr;
	StageCloud.Reset();
	if (AActor* Sky = StageSky.Get())
	{
		for (const auto& Saved : SavedSkyProperties)
		{
			if (FProperty* P = FindFProperty<FProperty>(Sky->GetClass(), Saved.Key))
			{ P->ImportText_Direct(*Saved.Value, P->ContainerPtrToValuePtr<void>(Sky), Sky, PPF_None); }
		}
		if (SunGroundBalanceLuxScale > 1.f)
		{
			// A seek may already have applied another phase's weather. Release to that
			// current intensity rather than freezing the stage-entry value.
			if (const ADCRideDirector* Director = ADCRideDirector::Get(this))
			{ SetSkyNumber(TEXT("Sun Light Intensity"), Director->GetEnv().SunIntensity * 0.75f + 2.f); }
		}
		RefreshSky();
		if (SunGroundBalanceLuxScale > 1.f && GetWorld() && !GetWorld()->bIsTearingDown)
		{
			if (const ADCRideDirector* Director = ADCRideDirector::Get(this))
			{
				const float Lux = Director->GetEnv().SunIntensity * 0.75f + 2.f;
				if (UDirectionalLightComponent* Sun = FindSun()) { Sun->SetIntensity(Lux); }
				UE_LOG(LogDC, Log, TEXT("MAEL_SKY ground-balance released to current weather lux=%.4f"), Lux);
			}
		}
		UE_LOG(LogDC, Log, TEXT("MAEL_SKY restored %d properties"), SavedSkyProperties.Num());
	}
	SunGroundBalanceLuxScale = 1.f;
	SavedSkyProperties.Empty();
	StageSky.Reset();
	if (PreviousLightShaftDistance >= 0.f)
	{
		if (IConsoleVariable* ShaftDistance = IConsoleManager::Get().FindConsoleVariable(TEXT("r.LightShaftFirstPassDistance")))
		{
			ShaftDistance->Unset(ECVF_SetByTemp, TEXT("DCMaelstromSky"));
			UE_LOG(LogDC, Log, TEXT("MAEL_SKY shaft blur restored=%.2f expected=%.2f"), ShaftDistance->GetFloat(), PreviousLightShaftDistance);
		}
		PreviousLightShaftDistance = -1.f;
	}
	if (PreviousCloudSampleDistance >= 0.f)
	{
		if (IConsoleVariable* Distance = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricCloud.DistanceToSampleMaxCount")))
		{ Distance->SetWithCurrentPriority(PreviousCloudSampleDistance); }
		PreviousCloudSampleDistance = -1.f;
	}
	if (PreviousCloudRenderMode >= 0)
	{
		if (IConsoleVariable* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricRenderTarget.Mode")))
		{ Mode->SetWithCurrentPriority(PreviousCloudRenderMode); }
		PreviousCloudRenderMode = -1;
	}
}
