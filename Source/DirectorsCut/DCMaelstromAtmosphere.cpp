#include "DCMaelstromAtmosphere.h"
#include "DCMaelstrom.h"
#include "DCTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

ADCMaelstromAtmosphere::ADCMaelstromAtmosphere()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AtmosphereFrame"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	Spray = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TornCascadeSpray"));
	Spray->SetupAttachment(RootComponent);
	Spray->SetMobility(EComponentMobility::Movable);
	Spray->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Spray->SetCastShadow(false);
	Spray->SetVisibility(false);
}

void ADCMaelstromAtmosphere::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromPlume.M_MaelstromPlume"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		SprayMID = UMaterialInstanceDynamic::Create(Material, this);
		Spray->SetMaterial(0, SprayMID);
	}
	else { UE_LOG(LogDC, Warning, TEXT("Maelstrom plume material missing: run Tools/make_maelstrom_atmosphere.py. Local 3D fog remains available.")); }
	ShaftMask = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromShaftMask.M_MaelstromShaftMask"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		USpotLightComponent* Light = NewObject<USpotLightComponent>(this, *FString::Printf(TEXT("DaylightOpening%d"), Index));
		AddInstanceComponent(Light);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetVisibility(false);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetUseInverseSquaredFalloff(true);
		Light->SetIntensity(0.f);
		Light->SetLightColor(FLinearColor(1.f, 0.95f, 0.84f));
		Light->SetInnerConeAngle(Index == 1 ? 0.45f : (Index == 0 ? 0.65f : 0.85f));
		Light->SetOuterConeAngle(Index == 1 ? 1.7f : (Index == 0 ? 2.3f : 2.8f));
		Light->SetAttenuationRadius(850000.f);
		Light->SetSourceRadius(1200.f);
		Light->SetSoftSourceRadius(2400.f);
		Light->SetCastShadows(true);
		Light->SetCastVolumetricShadow(true);
		Light->SetIndirectLightingIntensity(0.f);
		Light->SetSpecularScale(0.f);
		Light->SetDiffuseScale(ShaftDiffuseScale);
		Light->SetVolumetricScatteringIntensity(ShaftScattering);
		UMaterialInstanceDynamic* MaskMID = nullptr;
		if (ShaftMask)
		{
			MaskMID = UMaterialInstanceDynamic::Create(ShaftMask, this);
			MaskMID->SetScalarParameterValue(TEXT("Seed"), Index * 13.7f);
			Light->SetLightFunctionMaterial(MaskMID);
			Light->SetLightFunctionScale(FVector(9000.f));
			Light->SetLightFunctionFadeDistance(1400000.f);
		}
		Light->RegisterComponent();
		Shafts.Add(Light);
		ShaftMIDs.Add(MaskMID);
	}
}

void ADCMaelstromAtmosphere::AddFogLobe(const FFogLobe& Lobe)
{
	ULocalFogVolumeComponent* Fog = NewObject<ULocalFogVolumeComponent>(this, *FString::Printf(TEXT("CascadeBillow%d"), FogComponents.Num()));
	AddInstanceComponent(Fog);
	Fog->SetupAttachment(RootComponent);
	Fog->SetMobility(EComponentMobility::Movable);
	Fog->SetVisibility(false);
	Fog->RadialFogExtinction = 0.f;
	Fog->HeightFogExtinction = 0.f;
	Fog->HeightFogFalloff = 1.f;
	Fog->HeightFogOffset = 0.f;
	Fog->FogPhaseG = 0.45f;
	Fog->FogAlbedo = FLinearColor(0.74f, 0.82f, 0.87f);
	Fog->FogEmissive = FLinearColor::Black;
	// Local fog volumes support UNIFORM scale. Layer soft spheres instead of
	// requesting an unsupported stretched volume or displaying a sphere surface.
	Fog->SetRelativeScale3D(FVector(Lobe.Radius / ULocalFogVolumeComponent::GetBaseVolumeSize()));
	Fog->SetRelativeLocation(Plumes[Lobe.Plume].Foot + Lobe.Offset);
	Fog->RegisterComponent();
	FogLobes.Add(Lobe);
	FogComponents.Add(Fog);
}

void ADCMaelstromAtmosphere::BuildLayout(ADCMaelstrom* Stage)
{
	FRandomStream Random(71391);
	const FTransform Arena = Stage->GetArenaTransform();
	// Sample potential channels densely, then choose separated strong falls.
	struct FCandidate { float Angle; float Strength; };
	TArray<FCandidate> Candidates;
	for (int32 Index = 0; Index < 96; ++Index)
	{
		const float Angle = UE_TWO_PI * (Index + 0.37f) / 96.f;
		if (FMath::Abs(FMath::UnwindRadians(Angle - UE_PI)) < 0.9f) { continue; }
		const float Overflow = Stage->GetOverflowAmount(Angle);
		if (Overflow > 0.25f) { Candidates.Add({Angle, Overflow * FMath::Sqrt(Stage->GetRimDrop(Angle))}); }
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Strength > B.Strength; });
	for (const FCandidate& Candidate : Candidates)
	{
		bool bNearOther = false;
		for (const FPlume& Other : Plumes)
		{
			if (FMath::Abs(FMath::UnwindRadians(Other.Angle - Candidate.Angle)) < 0.26f) { bNearOther = true; break; }
		}
		if (bNearOther) { continue; }
		FPlume Plume;
		Plume.Angle = Candidate.Angle;
		const float FootR = Stage->GetRimRadius(Plume.Angle) * (Stage->GetRimJoin(Plume.Angle) - 0.02f);
		FVector FootWorld = Arena.TransformPosition(FVector(FMath::Cos(Plume.Angle) * FootR, FMath::Sin(Plume.Angle) * FootR, 0.f));
		FootWorld.Z = Stage->SampleSurfaceWorld(FootWorld);
		Plume.Foot = Arena.InverseTransformPosition(FootWorld);
		Plume.Height = FMath::Clamp(Stage->GetRimDrop(Plume.Angle) * Random.FRandRange(0.60f, 0.82f), 9000.f, 22000.f);
		Plume.Width = Plume.Height * Random.FRandRange(0.90f, 1.25f);
		Plume.Seed = Random.FRand();
		Plume.Density = FMath::Lerp(0.65f, 1.f, Stage->GetOverflowAmount(Plume.Angle));
		Plumes.Add(Plume);
		if (Plumes.Num() >= 14) { break; }
	}
	for (int32 Index = 0; Index < Plumes.Num(); ++Index)
	{
		const FPlume& P = Plumes[Index];
		const FVector Inward(-FMath::Cos(P.Angle), -FMath::Sin(P.Angle), 0.f);
		const FVector Tangent(-FMath::Sin(P.Angle), FMath::Cos(P.Angle), 0.f);
		AddFogLobe({Index, FVector(0.f, 0.f, P.Height * 0.14f), P.Width * 0.90f, P.Density * 0.52f, false});
		AddFogLobe({Index, Inward * P.Height * 0.30f + Tangent * P.Height * (P.Seed - 0.5f) * 0.35f
			+ FVector(0.f, 0.f, P.Height * 0.65f), P.Width * 0.46f, P.Density * 0.48f, true});
		// Connected asymmetric lobes trail DOWN the actual slope. A single sphere
		// parked at each fall foot reads as repeated dots across a kilometre view.
		const int32 WispCount = Index % 2 == 0 ? 2 : 1;
		for (int32 Wisp = 0; Wisp < WispCount; ++Wisp)
		{
			const float WispRadius = P.Height * (Wisp == 0 ? 0.85f : 0.65f);
			const float Side = (P.Seed - 0.5f) * 1.8f;
			const FVector Offset = Inward * P.Height * (0.70f + Wisp * 0.55f)
				+ Tangent * P.Height * Side * (0.30f + Wisp * 0.35f);
			FVector WispWorld = Arena.TransformPosition(P.Foot + Offset);
			WispWorld.Z = Stage->SampleSurfaceWorld(WispWorld) + WispRadius * 0.28f;
			AddFogLobe({Index, Arena.InverseTransformPosition(WispWorld) - P.Foot,
				WispRadius, P.Density * (Wisp == 0 ? 0.42f : 0.23f), true});
		}
		// A few wider low banks supply a middle distance without a uniform ring.
		if (Index % 3 == 0)
		{
			const FVector LowOffset = Inward * P.Height * 1.75f + Tangent * P.Height * (P.Seed - 0.5f);
			FVector LowWorld = Arena.TransformPosition(P.Foot + LowOffset);
			LowWorld.Z = Stage->SampleSurfaceWorld(LowWorld) + 2800.f;
			AddFogLobe({Index, Arena.InverseTransformPosition(LowWorld) - P.Foot,
				FMath::Clamp(P.Height * 1.1f, 15000.f, 25000.f), P.Density * 0.18f, true});
		}
	}
	BuildSpray();
	bBuilt = true;
	UE_LOG(LogDC, Log, TEXT("Maelstrom atmosphere: %d fall plumes, %d local fog lobes, three volumetric daylight openings"), Plumes.Num(), FogComponents.Num());
}

void ADCMaelstromAtmosphere::BuildSpray()
{
	TArray<FVector> Vertices, Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<int32> Triangles;
	TArray<FProcMeshTangent> Tangents;
	constexpr int32 Rows = 12, Columns = 8;
	for (const FPlume& P : Plumes)
	{
		const FVector Inward(-FMath::Cos(P.Angle), -FMath::Sin(P.Angle), 0.f);
		for (int32 Sheet = 0; Sheet < 3; ++Sheet)
		{
			const int32 Start = Vertices.Num();
			const float Facing = P.Angle + UE_HALF_PI + Sheet * UE_PI / 3.f + P.Seed * 0.35f;
			const FVector Across(FMath::Cos(Facing), FMath::Sin(Facing), 0.f);
			const FVector Normal = FVector::CrossProduct(FVector::UpVector, Across);
			for (int32 Row = 0; Row <= Rows; ++Row)
			{
				const float V = Row / static_cast<float>(Rows);
				const float Width = P.Width * (0.48f + 0.68f * FMath::Sin(V * UE_PI));
				for (int32 Column = 0; Column <= Columns; ++Column)
				{
					const float U = Column / static_cast<float>(Columns);
					const float Curl = FMath::Cos((U - 0.5f) * UE_PI) * P.Width * 0.13f;
					Vertices.Add(P.Foot + Across * ((U - 0.5f) * Width * 2.f)
						+ Inward * (P.Height * 0.26f * V * V) + Normal * Curl
						+ FVector(0.f, 0.f, V * P.Height - 450.f));
					UVs.Add(FVector2D(U, V));
					Colors.Add(FLinearColor(P.Density, P.Seed, Sheet / 2.f, 0.72f));
					if (Row < Rows && Column < Columns)
					{
						const int32 A = Start + Row * (Columns + 1) + Column, B = A + Columns + 1;
						Triangles.Append({A, A + 1, B, A + 1, B + 1, B});
					}
				}
			}
		}
	}
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
	Spray->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

UDirectionalLightComponent* ADCMaelstromAtmosphere::FindSun() const
{
	UDirectionalLightComponent* Best = nullptr;
	float BestScore = 0.f;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->IsHidden()) { continue; }
		TInlineComponentArray<UDirectionalLightComponent*> Lights(*It);
		for (UDirectionalLightComponent* Light : Lights)
		{
			// UDS unregisters the director's original sun without clearing its
			// visible flag. It must never win selection over the rendered sun.
			if (!Light->IsRegistered() || !Light->bAffectsWorld || !Light->IsVisible() || Light->bHiddenInGame || Light->Intensity <= 0.f) { continue; }
			const float Score = Light->Intensity + (Light->bAtmosphereSunLight && Light->AtmosphereSunLightIndex == 0 ? 1000000.f : 0.f);
			if (Score > BestScore) { BestScore = Score; Best = Light; }
		}
	}
	return Best;
}

void ADCMaelstromAtmosphere::CaptureSun()
{
	if (!bTuneSunCloudShadows) { return; }
	if (UDirectionalLightComponent* Sun = FindSun())
	{
		SavedSun.Component = Sun;
		SavedSun.Scattering = Sun->VolumetricScatteringIntensity;
		SavedSun.CloudStrength = Sun->CloudShadowStrength;
		SavedSun.CloudAtmosphere = Sun->CloudShadowOnAtmosphereStrength;
		SavedSun.CloudSurface = Sun->CloudShadowOnSurfaceStrength;
		SavedSun.CloudExtent = Sun->CloudShadowExtent;
		SavedSun.CloudResolution = Sun->CloudShadowMapResolutionScale;
		SavedSun.bCloudShadows = Sun->bCastCloudShadows;
		SavedSun.bVolumetricShadow = Sun->bCastVolumetricShadow;
	}
}

void ADCMaelstromAtmosphere::UpdateSun()
{
	if (!bTuneSunCloudShadows) { RestoreSun(); return; }
	if (!SavedSun.Component.IsValid()) { CaptureSun(); }
	if (UDirectionalLightComponent* Sun = SavedSun.Component.Get())
	{
		Sun->SetVolumetricScatteringIntensity(FMath::Max(SavedSun.Scattering, 1.15f));
		Sun->SetCastVolumetricShadow(true);
		const float Extent = FMath::Max(5.f, SavedSun.CloudExtent);
		const float Resolution = FMath::Max(2.f, SavedSun.CloudResolution);
		if (!Sun->bCastCloudShadows || Sun->CloudShadowStrength != 0.85f || Sun->CloudShadowOnAtmosphereStrength != 1.f
			|| Sun->CloudShadowOnSurfaceStrength != 0.7f || Sun->CloudShadowExtent != Extent || Sun->CloudShadowMapResolutionScale != Resolution)
		{
			Sun->bCastCloudShadows = true;
			Sun->CloudShadowStrength = 0.85f;
			Sun->CloudShadowOnAtmosphereStrength = 1.f;
			Sun->CloudShadowOnSurfaceStrength = 0.7f;
			Sun->CloudShadowExtent = Extent;
			Sun->CloudShadowMapResolutionScale = Resolution;
			Sun->MarkRenderStateDirty();
		}
	}
}

void ADCMaelstromAtmosphere::RestoreSun()
{
	if (UDirectionalLightComponent* Sun = SavedSun.Component.Get())
	{
		Sun->SetVolumetricScatteringIntensity(SavedSun.Scattering);
		Sun->SetCastVolumetricShadow(SavedSun.bVolumetricShadow);
		Sun->bCastCloudShadows = SavedSun.bCloudShadows;
		Sun->CloudShadowStrength = SavedSun.CloudStrength;
		Sun->CloudShadowOnAtmosphereStrength = SavedSun.CloudAtmosphere;
		Sun->CloudShadowOnSurfaceStrength = SavedSun.CloudSurface;
		Sun->CloudShadowExtent = SavedSun.CloudExtent;
		Sun->CloudShadowMapResolutionScale = SavedSun.CloudResolution;
		Sun->MarkRenderStateDirty();
	}
	SavedSun.Component.Reset();
}

void ADCMaelstromAtmosphere::CaptureVolumetrics()
{
	// The stage owns height, density and falloff. Enable the existing renderer and
	// its scattering medium, restoring both UDS values on exit or backward seek.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->IsHidden()) { continue; }
		TInlineComponentArray<UExponentialHeightFogComponent*> Fogs(*It);
		for (UExponentialHeightFogComponent* Fog : Fogs)
		{
			if (!Fog->IsRegistered() || !Fog->IsVisible() || Fog->bHiddenInGame) { continue; }
			SavedVolumetrics.Add({Fog, Fog->bEnableVolumetricFog, Fog->VolumetricFogExtinctionScale});
			Fog->SetVolumetricFog(true);
			Fog->SetVolumetricFogExtinctionScale(FMath::Max(0.f, VolumetricExtinctionScale));
		}
	}
	if (SavedVolumetrics.IsEmpty()) { UE_LOG(LogDC, Warning, TEXT("Maelstrom daylight shafts require an existing visible height fog component; none found")); }
}

void ADCMaelstromAtmosphere::RestoreVolumetrics()
{
	for (const FVolumetricState& Saved : SavedVolumetrics)
	{
		if (UExponentialHeightFogComponent* Fog = Saved.Component.Get())
		{
			Fog->SetVolumetricFogExtinctionScale(Saved.ExtinctionScale);
			Fog->SetVolumetricFog(Saved.bEnabled);
		}
	}
	SavedVolumetrics.Reset();
}

void ADCMaelstromAtmosphere::UpdateShafts(ADCMaelstrom* Stage, float Elapsed, float Reveal)
{
	UDirectionalLightComponent* Sun = SavedSun.Component.IsValid() ? SavedSun.Component.Get() : FindSun();
	FVector Down = Sun ? Sun->GetForwardVector() : FRotator(-22.f, 230.f, 0.f).Vector();
	if (Down.Z > 0.f) { Down = -Down; }
	if (Down.Z > -0.08f) { Down.Z = -0.08f; Down.Normalize(); }
	const FVector2D Anchors[] = {FVector2D(-0.28f, -0.13f), FVector2D(0.04f, 0.38f), FVector2D(0.48f, -0.23f)};
	for (int32 Index = 0; Index < Shafts.Num(); ++Index)
	{
		USpotLightComponent* Light = Shafts[Index];
		Light->SetVisibility(bEnableShafts && Reveal > 0.01f);
		if (!bEnableShafts) { continue; }
		FVector Target = Stage->GetArenaTransform().TransformPosition(FVector(Anchors[Index].X * Stage->Radius, Anchors[Index].Y * Stage->Radius, 0.f));
		Target.Z = Stage->SampleSurfaceWorld(Target) + 12000.f;
		if (Index > 0 && !Plumes.IsEmpty())
		{
			// Illuminate spray actually present at two far-side channels. A cone
			// aimed at empty air can be invisible even at a sensible light intensity.
			const float WantedAngle = Index == 1 ? -0.50f : 0.60f;
			const FPlume* Nearest = &Plumes[0];
			for (const FPlume& Plume : Plumes)
			{
				if (FMath::Abs(FMath::UnwindRadians(Plume.Angle - WantedAngle)) < FMath::Abs(FMath::UnwindRadians(Nearest->Angle - WantedAngle))) { Nearest = &Plume; }
			}
			Target = Stage->GetArenaTransform().TransformPosition(Nearest->Foot + FVector(0.f, 0.f, 15000.f));
		}
		// Put the virtual opening well outside the bowl so rays are almost parallel
		// through it, never a bright cone apex sitting above the water.
		const float Distance = 600000.f + Index * 35000.f;
		Light->SetWorldLocationAndRotation(Target - Down * Distance, Down.Rotation());
		// Inverse-square attenuation and the finite-radius window both act here.
		// Keep a minimum for low-unit lighting rigs, but scale to UDS's *actual*
		// directional-light lux so the ray is not lost against a daylight exposure.
		const float RadiusWindow = FMath::Square(1.f - FMath::Pow(Distance / 850000.f, 4.f));
		const float SunMatchedCandela = (Sun ? Sun->Intensity : 0.f) * ShaftSunFraction
			* FMath::Square(Distance / 100.f) / FMath::Max(0.05f, RadiusWindow);
		Light->SetIntensity(FMath::Max(ShaftCandela, SunMatchedCandela) * Reveal * (Index == 1 ? 0.75f : 1.f));
		Light->SetDiffuseScale(FMath::Clamp(ShaftDiffuseScale, 0.f, 0.1f));
		Light->SetVolumetricScatteringIntensity(FMath::Max(0.f, ShaftScattering));
		if (ShaftMIDs[Index]) { ShaftMIDs[Index]->SetScalarParameterValue(TEXT("Clock"), Elapsed); }
	}
	if (!bReportedRenderer && Reveal >= 1.f)
	{
		bReportedRenderer = true;
		auto CVar = [](const TCHAR* Name) { const IConsoleVariable* Value = IConsoleManager::Get().FindConsoleVariable(Name); return Value ? Value->GetInt() : -1; };
		UE_LOG(LogDC, Log, TEXT("MAEL_ATMOS sun=%s lux=%.3f direction=%s LFV=%d support=%d voxelize=%d fog=%d gridXY=%d gridZ=%d LFVcap=%d"),
			*GetPathNameSafe(Sun), Sun ? Sun->Intensity : 0.f, *Down.ToCompactString(),
			CVar(TEXT("r.LocalFogVolume")), CVar(TEXT("r.SupportLocalFogVolumes")), CVar(TEXT("r.LocalFogVolume.RenderIntoVolumetricFog")),
			CVar(TEXT("r.VolumetricFog")), CVar(TEXT("r.VolumetricFog.GridPixelSize")), CVar(TEXT("r.VolumetricFog.GridSizeZ")), CVar(TEXT("r.LocalFogVolume.TileMaxInstanceCount")));
		for (const FVolumetricState& Saved : SavedVolumetrics)
		{
			if (const UExponentialHeightFogComponent* Fog = Saved.Component.Get())
			{
				UE_LOG(LogDC, Log, TEXT("MAEL_ATMOS fog=%s enabled=%d density=%.6f falloff=%.6f distance=%.0f start=%.0f extinction=%.3f"),
					*Fog->GetPathName(), Fog->bEnableVolumetricFog, Fog->FogDensity, Fog->FogHeightFalloff,
					Fog->VolumetricFogDistance, Fog->VolumetricFogStartDistance, Fog->VolumetricFogExtinctionScale);
			}
		}
		for (int32 Index = 0; Index < Shafts.Num(); ++Index)
		{
			UE_LOG(LogDC, Log, TEXT("MAEL_ATMOS shaft%d candela=%.0f scattering=%.2f diffuse=%.4f at=%s"), Index,
				Shafts[Index]->Intensity, Shafts[Index]->VolumetricScatteringIntensity, Shafts[Index]->DiffuseScale, *Shafts[Index]->GetComponentLocation().ToCompactString());
		}
		if (FogComponents.Num())
		{
			UE_LOG(LogDC, Log, TEXT("MAEL_ATMOS firstPlume radial=%.3f height=%.5f radius=%.0f world=%s spray=%d"),
				FogComponents[0]->RadialFogExtinction, FogComponents[0]->HeightFogExtinction, FogLobes[0].Radius, *FogComponents[0]->GetComponentLocation().ToCompactString(), SprayMID != nullptr);
		}
	}
}

void ADCMaelstromAtmosphere::SetEffectsVisible(bool bVisible)
{
	Spray->SetVisibility(bVisible && SprayMID != nullptr);
	for (ULocalFogVolumeComponent* Fog : FogComponents) { Fog->SetVisibility(bVisible); }
	for (USpotLightComponent* Light : Shafts) { Light->SetVisibility(bVisible && bEnableShafts); }
}

void ADCMaelstromAtmosphere::UpdateAtmosphere(ADCMaelstrom* Stage, float Elapsed, bool bActive)
{
	const bool bShow = bActive && IsValid(Stage) && Elapsed >= -1.f;
	if (!bShow)
	{
		if (bEffectsActive) { SetEffectsVisible(false); RestoreSun(); RestoreVolumetrics(); bEffectsActive = false; }
		return;
	}
	SetActorTransform(Stage->GetArenaTransform());
	if (!bBuilt) { BuildLayout(Stage); }
	if (!bEffectsActive) { CaptureSun(); CaptureVolumetrics(); SetEffectsVisible(true); bEffectsActive = true; }
	// Weather blueprints may reapply these every frame. Reassert only while this
	// stage is active, without modifying their fog density or height distribution.
	for (const FVolumetricState& Saved : SavedVolumetrics)
	{
		if (UExponentialHeightFogComponent* Fog = Saved.Component.Get())
		{
			Fog->SetVolumetricFog(true);
			Fog->SetVolumetricFogExtinctionScale(FMath::Max(0.f, VolumetricExtinctionScale));
		}
	}
	const float Reveal = FMath::SmoothStep(-1.f, 8.f, Elapsed);
	// Quantize only density edits: each local-fog setter recreates its render proxy.
	// Positions remain continuous, while startup density needs at most 32 changes.
	const float FogReveal = FMath::RoundToFloat(Reveal * 32.f) / 32.f;
	for (int32 Index = 0; Index < FogComponents.Num(); ++Index)
	{
		const FFogLobe& Lobe = FogLobes[Index];
		const FPlume& P = Plumes[Lobe.Plume];
		FVector Position = P.Foot + Lobe.Offset;
		if (Lobe.bDrift)
		{
			const FVector Tangent(-FMath::Sin(P.Angle), FMath::Cos(P.Angle), 0.f);
			Position += Tangent * (FMath::Sin(Elapsed * 0.045f + P.Seed * 8.f) * Lobe.Radius * 0.22f);
			Position.Z += FMath::Sin(Elapsed * 0.067f + P.Seed * 4.f) * Lobe.Radius * 0.06f;
		}
		FogComponents[Index]->SetRelativeLocation(Position);
		FogComponents[Index]->SetRadialFogExtinction(FMath::Max(0.f, MistDensity) * Lobe.Density * FogReveal);
		// UE 5.8 combines radial and height coverage multiplicatively: either
		// density being zero erases the whole local volume. The voxelized path uses
		// extinction per metre, so normalize height density to our 50..180 m radii
		// instead of making these enormous billows opaque white spheres.
		FogComponents[Index]->SetHeightFogExtinction(0.015f * (8000.f / FMath::Max(1000.f, Lobe.Radius))
			* (FMath::Max(0.f, MistDensity) / 0.85f) * Lobe.Density * FogReveal);
	}
	if (SprayMID)
	{
		SprayMID->SetScalarParameterValue(TEXT("Clock"), Elapsed);
		SprayMID->SetScalarParameterValue(TEXT("Reveal"), Reveal);
		SprayMID->SetScalarParameterValue(TEXT("Density"), SprayDensity);
	}
	UpdateSun();
	UpdateShafts(Stage, Elapsed, Reveal);
}

void ADCMaelstromAtmosphere::EndPlay(const EEndPlayReason::Type Reason)
{
	RestoreSun();
	RestoreVolumetrics();
	Super::EndPlay(Reason);
}
