#include "DCMaelstromAtmosphere.h"
#include "DCRideDirector.h"
#include "DCMaelstrom.h"
#include "DCTypes.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#endif

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
			Phase.Env.SkyIntensity = 1.7f;
			Phase.Env.SunIntensity = 6.5f;
			Phase.Env.Exposure = 0.05f;
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
	SetSkyNumber(TEXT("View Sample Scale (Day)"), 3.0);
	SetSkyNumber(TEXT("Shadow Sample Scale"), 1.0);
	SetSkyNumber(TEXT("Shadow Tracing Distance"), 4.0);
	SetSkyNumber(TEXT("Macro Variation"), 0.35);
	SetSkyNumber(TEXT("Macro Scale"), 0.8);
	SetSkyNumber(TEXT("Cloud Speed"), 0.12);
	SetSkyNumber(TEXT("Formation Change Speed"), 0.08);
	SetSkyNumber(TEXT("Cloud Wisps Opacity (Clear)"), 0.04);
	SetSkyNumber(TEXT("Cloud Wisps Opacity (Cloudy)"), 0.08);
	SetSkyColor(TEXT("Sun Light Color"), FLinearColor::White);
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
				SkyCloudMID = UMaterialInstanceDynamic::Create(Material, this);
				float Density = 0.025f, Erosion = 0.18f, Opening = 0.23f;
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
	if (SkyCloudMID && StageCloud.IsValid())
	{
		StageCloud->SetMaterial(SkyCloudMID);
		// The ship stays at the origin while the crater rises around it on descent.
		// Keep the sky above the rim rather than letting the far wall enter clouds.
		const ADCMaelstrom* Stage = ADCMaelstrom::Get(this);
		const float RimKm = Stage ? FMath::Max(0.f, static_cast<float>(Stage->GetArenaTransform().GetLocation().Z) * 0.00001f) : 0.f;
		// A fixed conservative tracing envelope avoids rebuilding the cloud proxy
		// every frame. Only density moves, smoothly, inside this envelope.
		StageCloud->SetLayerBottomAltitude(0.55f);
		StageCloud->SetLayerHeight(6.f);
		StageCloud->SetViewSampleCountScale(3.f);
		StageCloud->SetShadowViewSampleCountScale(2.f);
		StageCloud->SetSkyLightCloudBottomOcclusion(0.5f);
		SkyCloudMID->SetScalarParameterValue(TEXT("RimHeight"), RimKm);
		if (const ADCRideDirector* Director = ADCRideDirector::Get(this))
		{ SkyCloudMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime); }
		if (UDirectionalLightComponent* Sun = FindSun())
		{ SkyCloudMID->SetVectorParameterValue(TEXT("ToSun"), FLinearColor(-Sun->GetForwardVector())); }
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
		RefreshSky();
		UE_LOG(LogDC, Log, TEXT("MAEL_SKY restored %d properties"), SavedSkyProperties.Num());
	}
	SavedSkyProperties.Empty();
	StageSky.Reset();
	if (PreviousCloudRenderMode >= 0)
	{
		if (IConsoleVariable* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricRenderTarget.Mode")))
		{ Mode->SetWithCurrentPriority(PreviousCloudRenderMode); }
		PreviousCloudRenderMode = -1;
	}
}
