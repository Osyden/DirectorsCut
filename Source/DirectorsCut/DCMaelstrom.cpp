#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "DCRuins.h"
#include "DCKraken.h"
#include "DCKrakenHead.h"
#include "DCWeatherFX.h"
#include "DCLeviathan.h"
#include "DCMaelstromAtmosphere.h"
#include "DCPlayer.h"
#include "DCTreadmill.h"
#include "ProceduralMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "KismetProceduralMeshLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"

ADCMaelstrom::ADCMaelstrom()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Arena = CreateDefaultSubobject<USceneComponent>(TEXT("BasinFrame"));
	Arena->SetupAttachment(RootComponent);
	Basin = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Basin"));
	Basin->SetupAttachment(Arena);
	Falls = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RimCascades"));
	Falls->SetupAttachment(Arena);
	RimLand = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ContinuousRim"));
	RimLand->SetupAttachment(Arena);
	Tunnel = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ExitTunnel"));
	Tunnel->SetupAttachment(RootComponent);
	for (UProceduralMeshComponent* Mesh : {Basin.Get(), Falls.Get(), Tunnel.Get(), RimLand.Get()})
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
	}
	// The crater must shade its own lower slopes. Without this depth cue, every
	// surface receives the same grazing sunlight and reads as a shallow pool.
	Basin->SetCastShadow(true);
	RimLand->SetCastShadow(true);
	Exposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("RevealExposure"));
	Exposure->SetupAttachment(RootComponent);
	Exposure->bUnbound = true;
	Exposure->Priority = 100.f;
	Exposure->BlendWeight = 0.f;
	Exposure->Settings.bOverride_AutoExposureMethod = true;
	Exposure->Settings.AutoExposureMethod = AEM_Manual;
	Exposure->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Exposure->Settings.AutoExposureApplyPhysicalCameraExposure = false;
	Exposure->Settings.bOverride_AutoExposureBias = true;
	Exposure->Settings.bOverride_BloomIntensity = true;
	Exposure->Settings.BloomIntensity = 0.35f;
	Exposure->Settings.bOverride_MotionBlurAmount = true;
	Exposure->Settings.MotionBlurAmount = 0.f;
}

ADCMaelstrom* ADCMaelstrom::Get(const UObject* Context)
{
	if (UWorld* World = Context ? Context->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCMaelstrom> It(World); It; ++It) { return *It; }
	}
	return nullptr;
}

void ADCMaelstrom::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);
	if (!Director) { SetActorTickEnabled(false); return; }
	AddTickPrerequisiteActor(Director);
	for (FDCPhaseDef& Phase : Director->Phases)
	{
		if (Phase.Phase == EDCPhase::Slide) { SlideStart = Phase.StartTime; }
		if (Phase.Phase == EDCPhase::Breach) { BreachStart = Phase.StartTime; }
		if (Phase.Phase == EDCPhase::Ending) { EndingStart = Phase.StartTime; }
		// Stage-owned lighting cues use the director's public phase data. The earlier
		// fog, storm front and sky-pack implementation remain untouched.
		if (Phase.Phase == EDCPhase::Breach || Phase.Phase == EDCPhase::Maelstrom)
		{
			Phase.BlendTime = Phase.Phase == EDCPhase::Breach ? 5.f : 3.f;
			Phase.Env.FogDensity = 0.0008f;
			Phase.Env.FogHeightFalloff = 0.0018f;
			Phase.Env.Mist = 0.f;
			Phase.Env.Rain = 0.f;
			Phase.Env.SunIntensity = 9.f;
			Phase.Env.SunColor = FLinearColor(1.f, 0.97f, 0.91f);
			Phase.Env.SunPitch = -38.f;
			Phase.Env.SunYaw = 230.f;
			Phase.Env.CloudCover = 0.16f;
			Phase.Env.CloudBase = 260000.f;
			Phase.Env.SkyIntensity = 2.0f;
			Phase.Env.Exposure = -0.15f;
			Phase.Env.LightningRate = 0.f;
		}
		if (Phase.Phase == EDCPhase::Ending) { Phase.BlendTime = 0.f; }
	}
	if (UMaterialInterface* Water = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromWater.M_MaelstromWater")))
	{
		WaterMID = UMaterialInstanceDynamic::Create(Water, this);
		TunnelMID = UMaterialInstanceDynamic::Create(Water, this);
		Basin->SetMaterial(0, WaterMID);
		Tunnel->SetMaterial(0, TunnelMID);
	}
	if (UMaterialInterface* Cascade = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromCascade.M_MaelstromCascade")))
	{
		CascadeMID = UMaterialInstanceDynamic::Create(Cascade, this);
		Falls->SetMaterial(0, CascadeMID);
	}
	BuildBasin();
	BuildTunnel();
	BuildLandmarks();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		// UDS also injects a sea-level density field through a volume material. It
		// cannot follow the deep crater; retain ordinary volumetric fog for shafts.
		FBoolProperty* Enabled = FindFProperty<FBoolProperty>(It->GetClass(), TEXT("Render Global Volumetric Material"));
		FObjectPropertyBase* Volume = FindFProperty<FObjectPropertyBase>(It->GetClass(), TEXT("Global Volumetric Fog Mesh"));
		if (Enabled && Volume)
		{
			if (UPrimitiveComponent* Mesh = Cast<UPrimitiveComponent>(Volume->GetObjectPropertyValue_InContainer(*It)))
			{
				StageFogVolumes.Add({*It, Mesh, Enabled->GetPropertyValue_InContainer(*It), Mesh->IsVisible(), Mesh->bHiddenInGame != 0});
				AddTickPrerequisiteActor(*It);
			}
		}
		TInlineComponentArray<UExponentialHeightFogComponent*> Fogs(*It);
		for (UExponentialHeightFogComponent* Fog : Fogs)
		{
			if (!Fog->IsVisible() || It->IsHidden()) { continue; }
			StageFogs.Add({Fog, Fog->GetComponentLocation(), Fog->VolumetricFogDistance, Fog->FogHeightFalloff, Fog->FogDensity});
			if (*It != Director) { AddTickPrerequisiteActor(*It); }
		}
	}
	Boss = GetWorld()->SpawnActor<ADCLeviathan>();
	if (Boss)
	{
		// The creature is beaten 20 s before the ending phase begins, which is how long its arms take to
		// withdraw. -DCBossDiesAt=<seconds after the breach> moves it (the ending then follows 20 s later).
		Boss->FinaleAt = EndingStart - BreachStart - 20.f;
		FParse::Value(FCommandLine::Get(), TEXT("DCBossDiesAt="), Boss->FinaleAt);
	}
	Atmosphere = GetWorld()->SpawnActor<ADCMaelstromAtmosphere>();
	SetStageVisible(false);
	UE_LOG(LogDC, Log, TEXT("Maelstrom stage: %.1f km across, %.0f m deep, breach %.1f ending %.1f"), Radius / 50000.f, Depth / 100.f, BreachStart, EndingStart);
}

namespace
{
	float RimVariation(float Angle)
	{
		// Leave the playable entry corridor unchanged, including both gun batteries.
		return FMath::SmoothStep(0.35f, 0.85f, FMath::Abs(FMath::UnwindRadians(Angle - UE_PI)));
	}
}

float ADCMaelstrom::GetRimRadius(float Angle) const
{
	const float Bays = 0.065f * FMath::Sin(Angle * 3.f + 0.7f) + 0.032f * FMath::Sin(Angle * 7.f - 1.3f) + 0.014f * FMath::Sin(Angle * 17.f + 2.1f);
	return Radius * (1.f + Bays * RimVariation(Angle));
}

float ADCMaelstrom::GetOverflowAmount(float Angle) const
{
	const float Channels = 0.5f + 0.34f * FMath::Sin(Angle * 7.f + 0.3f) + 0.23f * FMath::Sin(Angle * 13.f - 0.9f);
	return FMath::SmoothStep(0.25f, 0.56f, Channels);
}

float ADCMaelstrom::GetRimDrop(float Angle) const
{
	const float Variation = FMath::Clamp(0.5f + 0.30f * FMath::Sin(Angle * 4.f - 0.4f) + 0.21f * FMath::Sin(Angle * 9.f + 1.7f), 0.f, 1.f);
	return FMath::Lerp(Depth * (240.f / 900.f), Depth * FMath::Lerp(0.10f, 0.39f, Variation), RimVariation(Angle));
}

float ADCMaelstrom::GetRimJoin(float Angle) const
{
	const float Approach = FMath::Pow(FMath::Max(0.f, -FMath::Cos(Angle)), 8.f);
	return FMath::Lerp(0.90f - 0.12f * Approach, 0.84f + 0.11f * GetOverflowAmount(Angle), RimVariation(Angle));
}

float ADCMaelstrom::RimElevation(float Angle) const
{
	return RimVariation(Angle) * (3500.f * FMath::Sin(Angle * 3.f - 0.5f) + 1600.f * FMath::Sin(Angle * 8.f + 1.2f));
}

float ADCMaelstrom::HeightAt(const FVector2D& Point, bool bWaves) const
{
	const float R = Point.Size();
	const float A = FMath::Atan2(Point.Y, Point.X);
	const float LocalRadius = GetRimRadius(A);
	const float U = FMath::Clamp(R / LocalRadius, 0.f, 1.f);
	// One continuous surface: broad entry ramp, steep far overflow, deep inner slope.
	const float Approach = FMath::Pow(FMath::Max(0.f, -FMath::Cos(A)), 8.f);
	const float Join = GetRimJoin(A);
	const float LipDrop = GetRimDrop(A);
	const float LipHeight = RimElevation(A);
	constexpr float Power = 1.55f;
	float Height;
	if (U <= Join)
	{
		Height = -Depth + (Depth + LipHeight - LipDrop) * FMath::Pow(U / Join, Power);
	}
	else
	{
		const float T = (1.f - U) / (1.f - Join);
		// A rounded approach shoulder hides the crater floor from the helm.
		// Start the entry at a downhill grade; keep the distant overflow lip soft.
		const float M0 = FMath::Lerp(0.08f, 0.65f, Approach);
		const float M1 = (Depth + LipHeight - LipDrop) * Power * (1.f - Join) / (Join * LipDrop);
		const float Profile = (M0 + M1 - 2.f) * T * T * T + (3.f - 2.f * M0 - M1) * T * T + M0 * T;
		Height = LipHeight - LipDrop * Profile;
	}
	if (!bWaves) { return Height; }
	const float Warp = FMath::PerlinNoise2D(Point / 31000.f);
	const float Phase = R * 0.00058f + A * 4.f + Warp * 4.f + FMath::PerlinNoise2D(Point / 11000.f) * 2.2f + FMath::PerlinNoise2D(Point / 4500.f) * 2.f + FMath::Sin(A * 3.f + U * 9.f);
	const float Crest = FMath::Pow(FMath::Max(0.f, FMath::Sin(Phase)), 3.f) - 0.3f;
	const float Cross = FMath::Sin(Point.X * 0.00019f - Point.Y * 0.00013f + Warp * 3.f);
	const float Patch = FMath::SmoothStep(0.25f, 0.7f, 0.5f + FMath::PerlinNoise2D(Point / 4500.f + FVector2D(17.3f, -8.7f)));
	const float Broken = FMath::Clamp(0.7f + FMath::PerlinNoise2D(Point / 18000.f), 0.15f, 1.2f) * FMath::Lerp(0.15f, 1.f, Patch);
	const float Amplitude = FMath::Lerp(1000.f, 380.f, Approach);
	const float RimFade = 1.f - FMath::SmoothStep(0.89f, 1.02f, U) * 0.85f;
	const float EntryWaves = Amplitude * (Crest * Broken + Cross * 0.3f);
	// Cross seas with unequal wavelengths and directions form long breaking crests,
	// rather than isotropic terrain mounds or one repeating set of parallel ribs.
	const FVector2D Bent = Point + 6500.f * FVector2D(
		FMath::PerlinNoise2D(Point / 22000.f + FVector2D(4.7f, 13.2f)),
		FMath::PerlinNoise2D(Point / 25000.f + FVector2D(-7.9f, 6.1f)));
	struct FWave { FVector2D Direction; float Length; float Height; float Phase; };
	static const FWave Waves[] = {
		{FVector2D(0.958f, 0.287f), 14200.f, 500.f, 1.3f},
		{FVector2D(0.427f, 0.904f), 8700.f, 330.f, 4.7f},
		{FVector2D(0.836f,-0.548f), 5300.f, 180.f, 2.8f},
		{FVector2D(-0.572f,0.820f),21100.f, 700.f, 0.2f},
		{FVector2D(0.752f, 0.659f), 3400.f, 100.f, 5.9f},
		{FVector2D(0.101f,-0.995f), 6400.f, 260.f, 3.1f}
	};
	float FarWaves = 0.f;
	for (const FWave& Wave : Waves)
	{
		const float PhaseAngle = FVector2D::DotProduct(Bent, Wave.Direction) * UE_TWO_PI / Wave.Length + Wave.Phase;
		const float Envelope = FMath::Clamp(0.70f + FMath::PerlinNoise2D(Point / (Wave.Length * 1.8f) + FVector2D(Wave.Phase, -Wave.Phase)), 0.2f, 1.2f);
		FarWaves += Wave.Height * Envelope * (FMath::Sin(PhaseAngle) + 0.22f * FMath::Cos(2.f * PhaseAngle + 0.7f));
	}
	return Height + FMath::Lerp(EntryWaves, FarWaves, RimVariation(A)) * RimFade * FMath::SmoothStep(0.05f, 0.2f, U);
}

float ADCMaelstrom::RimGroundHeight(float R, float Angle) const
{
	const float Ridge = 3000.f + 1800.f * FMath::Sin(Angle * 9.f) + 900.f * FMath::Sin(Angle * 23.f + R * 0.0001f);
	return RimElevation(Angle) + FMath::Lerp(-1800.f, Ridge, FMath::SmoothStep(1.008f, 1.055f, R / GetRimRadius(Angle)));
}

void ADCMaelstrom::BuildBasin()
{
	constexpr int32 Rings = 320;
	constexpr int32 Sectors = 768;
	TArray<FVector> Vertices, Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Triangles;
	TArray<FProcMeshTangent> Tangents;
	for (int32 Ring = 0; Ring <= Rings; ++Ring)
	{
		for (int32 Side = 0; Side <= Sectors; ++Side)
		{
			const float A = Side * UE_TWO_PI / Sectors;
			const float R = FMath::Lerp(120.f, GetRimRadius(A) * 1.24f, static_cast<float>(Ring) / Rings);
			const FVector2D P(FMath::Cos(A) * R, FMath::Sin(A) * R);
			const float Z = HeightAt(P);
			const float DX = (HeightAt(P + FVector2D(60.f, 0.f)) - HeightAt(P - FVector2D(60.f, 0.f))) / 120.f;
			const float DY = (HeightAt(P + FVector2D(0.f, 60.f)) - HeightAt(P - FVector2D(0.f, 60.f))) / 120.f;
			Vertices.Add(FVector(P.X, P.Y, Z));
			Normals.Add(FVector(-DX, -DY, 1.f).GetSafeNormal());
			Tangents.Add(FProcMeshTangent(FVector(1.f, 0.f, DX).GetSafeNormal(), false));
			UVs.Add(P / 10000.f);
			const float Approach = FMath::Pow(FMath::Max(0.f, -FMath::Cos(A)), 8.f);
			const float Crest = FMath::Clamp((Z - HeightAt(P, false)) / FMath::Lerp(1600.f, 250.f, Approach), 0.f, 1.f);
			const float UpstreamAngle = A + (8.f / 3.5f) * FMath::Loge(FMath::Clamp(R / GetRimRadius(A), 0.08f, 1.f));
			Colors.Add(FLinearColor(Crest, R / GetRimRadius(A), FMath::Clamp(-Z / Depth, 0.f, 1.f), GetOverflowAmount(UpstreamAngle)).ToFColor(false));
			if (Ring < Rings && Side < Sectors)
			{
				const int32 I = Ring * (Sectors + 1) + Side;
				const int32 J = I + Sectors + 1;
				Triangles.Append({I, I + 1, J, I + 1, J + 1, J});
			}
		}
	}
	Basin->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	Vertices.Reset(); Normals.Reset(); UVs.Reset(); Colors.Reset(); Triangles.Reset(); Tangents.Reset();
	// The falling face is actual opaque water attached to the overflow, not floating
	// translucent cards. The approach remains an open sloping ride surface.
	constexpr int32 FallSides = 768, FallRows = 32;
	for (int32 Side = 0; Side <= FallSides; ++Side)
	{
		const float A = Side * UE_TWO_PI / FallSides;
		const float LocalRadius = GetRimRadius(A);
		const float Join = GetRimJoin(A);
		const float LipZ = HeightAt(FVector2D(FMath::Cos(A), FMath::Sin(A)) * LocalRadius);
		const float Overflow = GetOverflowAmount(A);
		for (int32 Row = 0; Row <= FallRows; ++Row)
		{
			const float T = Row / static_cast<float>(FallRows);
			const float R = LocalRadius * FMath::Lerp(1.001f, Join - 0.015f - 0.012f * FMath::Sin(A * 21.f), T);
			const FVector2D P(FMath::Cos(A) * R, FMath::Sin(A) * R);
			const float Z = HeightAt(P) + 120.f;
			const float DX = (HeightAt(P + FVector2D(60,0)) - HeightAt(P - FVector2D(60,0))) / 120.f;
			const float DY = (HeightAt(P + FVector2D(0,60)) - HeightAt(P - FVector2D(0,60))) / 120.f;
			Vertices.Add(FVector(P.X, P.Y, Z));
			Normals.Add(FVector(-DX,-DY,1.f).GetSafeNormal());
			Tangents.Add(FProcMeshTangent(FVector(-FMath::Sin(A),FMath::Cos(A),-DX*FMath::Sin(A)+DY*FMath::Cos(A)).GetSafeNormal(), false));
			UVs.Add(FVector2D(A * Radius / 10000.f, FMath::Max(0.f, LipZ - Z) / 10000.f));
			Colors.Add(FLinearColor(Overflow, 1.f, T, Overflow).ToFColor(false));
			if (Side < FallSides && Row < FallRows && Overflow > 0.16f && FMath::Abs(FMath::UnwindRadians(A - UE_PI)) > 0.8f)
			{
				const int32 I = Side * (FallRows + 1) + Row, J = I + FallRows + 1;
				Triangles.Append({I, J, I+1, I+1, J, J+1});
			}
		}
	}
	Falls->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	Vertices.Reset(); Normals.Reset(); UVs.Reset(); Colors.Reset(); Triangles.Reset(); Tangents.Reset();
	constexpr int32 LandRows=16, LandSides=384;
	for (int32 Row=0; Row<=LandRows; ++Row)
	{
		for (int32 Side=0; Side<=LandSides; ++Side)
		{
			const float A=Side*UE_TWO_PI/LandSides;
			const float R=GetRimRadius(A)*FMath::Lerp(1.008f,1.24f,Row/static_cast<float>(LandRows));
			const FVector2D P(FMath::Cos(A)*R,FMath::Sin(A)*R);
			auto Ground=[this](const FVector2D& Q) { return RimGroundHeight(Q.Size(),FMath::Atan2(Q.Y,Q.X)); };
			const float DX=(Ground(P+FVector2D(60,0))-Ground(P-FVector2D(60,0)))/120.f;
			const float DY=(Ground(P+FVector2D(0,60))-Ground(P-FVector2D(0,60)))/120.f;
			Vertices.Add(FVector(P.X,P.Y,Ground(P))); Normals.Add(FVector(-DX,-DY,1).GetSafeNormal());
			UVs.Add(P/3000.f); Colors.Add(FColor::White); Tangents.Add(FProcMeshTangent(FVector(1,0,DX).GetSafeNormal(),false));
			if (Row<LandRows && Side<LandSides && FMath::Abs(FMath::UnwindRadians(A-UE_PI))>0.4f)
			{
				const int32 I=Row*(LandSides+1)+Side,J=I+LandSides+1; Triangles.Append({I,I+1,J,I+1,J+1,J});
			}
		}
	}
	RimLand->CreateMeshSection(0,Vertices,Triangles,Normals,UVs,Colors,Tangents,false);
}

void ADCMaelstrom::BuildTunnel()
{
	TArray<FVector> Verts, Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Triangles;
	constexpr int32 LengthSteps = 100, Around = 128;
	TArray<FProcMeshTangent> Tangents;
	constexpr float TubeRadius = 9000.f;
	for (int32 X = 0; X <= LengthSteps; ++X)
	{
		const float Along = FMath::Lerp(-200000.f, 0.f, X / static_cast<float>(LengthSteps));
		for (int32 Y = 0; Y <= Around; ++Y)
		{
			const float A = Y * UE_TWO_PI / Around;
			const float Ripple = 180.f * FMath::Sin(A * 9.f + Along * 0.0009f) + 80.f * FMath::Sin(A * 17.f - Along * 0.0017f);
			const float R = TubeRadius + Ripple;
			Verts.Add(FVector(Along, FMath::Sin(A) * R, TubeRadius - FMath::Cos(A) * R));
			Normals.Add(FVector(0.f, -FMath::Sin(A), FMath::Cos(A)));
			Tangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
			UVs.Add(FVector2D(Along / 10000.f, A * TubeRadius / 10000.f));
			Colors.Add(FLinearColor(FMath::Clamp(Ripple / 250.f, 0.f, 1.f), 0.5f, 0.2f, 1.f).ToFColor(false));
			if (X < LengthSteps && Y < Around)
			{
				const int32 I = X * (Around + 1) + Y, J = I + Around + 1;
				Triangles.Append({I, J, I + 1, I + 1, J, J + 1});
			}
		}
	}
	Tunnel->CreateMeshSection(0, Verts, Triangles, Normals, UVs, Colors, Tangents, false);
}

void ADCMaelstrom::BuildLandmarks()
{
	if (!DCRuins::Available()) { return; }
	FRandomStream Random(419);
	auto Place = [this](AActor* Actor, const FVector& Position, float Yaw)
	{
		if (!Actor) { return; }
		Actor->AttachToComponent(Arena, FAttachmentTransformRules::KeepRelativeTransform);
		Actor->SetActorRelativeLocation(Position);
		Actor->SetActorRelativeRotation(FRotator(0.f, Yaw, 0.f));
		Landmarks.Add(Actor);
	};
	// One connected coast, with modest-size architecture as distance markers.
	for (int32 I = 0; I < 9; ++I)
	{
		const float Angle = FMath::Lerp(-145.f, 145.f, I / 8.f) + Random.FRandRange(-6.f,6.f);
		const float A = FMath::DegreesToRadians(Angle);
		const float R = GetRimRadius(A) * Random.FRandRange(1.045f, 1.075f);
		const FVector Position(R*FMath::Cos(A),R*FMath::Sin(A),RimGroundHeight(R,A));
		const float Scale=Random.FRandRange(0.65f,1.4f);
		AActor* Crag=DCRuins::SpawnCrag(GetWorld(),12000.f,7000.f);
		Place(Crag,Position-FVector(0,0,6500.f),Angle);
		if (Crag && I == 0)
		{
			if (UStaticMeshComponent* Rock=Crag->FindComponentByClass<UStaticMeshComponent>()) { RimLand->SetMaterial(0,Rock->GetMaterial(0)); }
		}
		Place(DCRuins::SpawnComplex(GetWorld(),Scale,419+I),Position+FVector(0,0,300.f),Angle+180.f);
		if (I%3==0) { Place(DCRuins::SpawnColossus(GetWorld(),11000.f),Position+FVector(3000,6000,500),Angle+180.f); }
	}
	// Rock buttresses interrupt the overflow instead of making one white circular
	// wall. Their feet descend into the bowl; their crests join the uneven coast.
	auto BroadenCrag = [](AActor* Crag)
	{
		if (!Crag) { return; }
		TInlineComponentArray<UStaticMeshComponent*> Pieces(Crag);
		for (UStaticMeshComponent* Piece : Pieces)
		{
			if (!Piece->GetStaticMesh()) { continue; }
			const FBox Box = Piece->GetStaticMesh()->GetBoundingBox();
			const FVector Size = Box.GetSize();
			const FVector OldScale = Piece->GetRelativeScale3D();
			FVector Scale = OldScale;
			const float Width = FMath::Max(Size.X * Scale.X, Size.Y * Scale.Y);
			Scale.X = FMath::Max(Scale.X, Width * 0.65f / FMath::Max(Size.X, 1.f));
			Scale.Y = FMath::Max(Scale.Y, Width * 0.65f / FMath::Max(Size.Y, 1.f));
			const FVector Foot(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z);
			Piece->SetRelativeLocation(Piece->GetRelativeLocation() + Piece->GetRelativeRotation().RotateVector(Foot * (OldScale - Scale)));
			Piece->SetRelativeScale3D(Scale);
		}
	};
	float LastButtress = -10.f;
	for (int32 I = 0; I < 100; ++I)
	{
		const float A = -2.30f + I * 4.60f / 99.f;
		if (GetOverflowAmount(A) > 0.24f || A - LastButtress < 0.22f) { continue; }
		LastButtress = A;
		const float Drop = GetRimDrop(A);
		const float R = GetRimRadius(A) * 0.975f;
		const float Height = Drop + Random.FRandRange(5000.f, 10500.f);
		const FVector At(FMath::Cos(A)*R,FMath::Sin(A)*R,RimElevation(A)-Drop-1500.f);
		AActor* Buttress = DCRuins::SpawnCrag(GetWorld(),Random.FRandRange(12000.f,19000.f),Height);
		BroadenCrag(Buttress);
		Place(Buttress,At,FMath::RadiansToDegrees(A)+Random.FRandRange(-30.f,30.f));
		const float SpurR = R - Random.FRandRange(7000.f,13000.f);
		const FVector2D SpurPoint(FMath::Cos(A+0.035f)*SpurR,FMath::Sin(A+0.035f)*SpurR);
		const FVector Spur(SpurPoint,HeightAt(SpurPoint)-2200.f);
		Place(DCRuins::SpawnCrag(GetWorld(),Random.FRandRange(4500.f,7500.f),Random.FRandRange(6000.f,13000.f)),Spur,FMath::RadiansToDegrees(A)+40.f);
	}

}

FTransform ADCMaelstrom::GetArenaTransform() const { return Arena->GetComponentTransform(); }

float ADCMaelstrom::SampleSurfaceWorld(const FVector& Position) const
{
	if (Elapsed < -7.f)
	{
		const FTransform TubeTransform = Tunnel->GetComponentTransform();
		const FVector TubePoint = TubeTransform.InverseTransformPosition(Position);
		if (TubePoint.X < -200000.f || TubePoint.X > 0.f) { return Director->SeaLevelZ; }
		const float Side = FMath::Clamp(static_cast<float>(TubePoint.Y), -8999.f, 8999.f);
		return TubeTransform.TransformPosition(FVector(TubePoint.X, Side, 9000.f - FMath::Sqrt(81000000.f - Side * Side))).Z;
	}
	const FVector Local = GetArenaTransform().InverseTransformPosition(Position);
	const float BasinHeight = GetArenaTransform().TransformPosition(FVector(Local.X, Local.Y, HeightAt(FVector2D(Local.X, Local.Y)))).Z;
	// Match the water material's hull shelf. Small shader ripples are deliberately excluded.
	return FMath::Lerp(Director ? Director->SeaLevelZ : 230.f, BasinHeight,
		FMath::SmoothStep(1200.f, 3500.f, static_cast<float>(FVector2D(Position.X * 0.5, Position.Y * 2.0).Size())));
}

void ADCMaelstrom::DisableLegacyEncounter()
{
	if (bLegacyDisabled) { return; }
	for (TActorIterator<ADCKraken> It(GetWorld()); It; ++It) { It->SetActorHiddenInGame(true); It->SetActorTickEnabled(false); }
	for (TActorIterator<ADCKrakenHead> It(GetWorld()); It; ++It) { It->SetActorHiddenInGame(true); It->SetActorTickEnabled(false); }
	for (TActorIterator<ADCWeatherFX> It(GetWorld()); It; ++It) { LegacyWeather = *It; }
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("BP_DynamicRealWater"))) { Ocean = *It; }
	}
	bLegacyDisabled = true;
}

void ADCMaelstrom::SetStageVisible(bool bVisible)
{
	Basin->SetVisibility(bVisible);
	Falls->SetVisibility(bVisible);
	RimLand->SetVisibility(bVisible);
	Tunnel->SetVisibility(bVisible);
	for (AActor* Landmark : Landmarks) { if (Landmark) { Landmark->SetActorHiddenInGame(!bVisible); } }
}

void ADCMaelstrom::SetOceanVisible(bool bVisible)
{
	// Query the actual grid class, including any asynchronously created grids.
	// On the current plugin BP_DynamicRealWater itself derives from this class.
	if (GetWorld()->GetTimeSeconds() >= NextOceanScan)
	{
		NextOceanScan = GetWorld()->GetTimeSeconds() + 1.f;
		if (UClass* GridClass = FindObject<UClass>(nullptr, TEXT("/Script/DynamicRealWater.DRW_GridManager")))
		{
			for (TActorIterator<AActor> It(GetWorld(), GridClass); It; ++It)
			{
				if (!OceanGrids.Contains(*It))
				{
					OceanGrids.Add(*It);
					UE_LOG(LogDC, Log, TEXT("Maelstrom ocean grid: %s (%s)"), *It->GetName(), *It->GetClass()->GetPathName());
				}
			}
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("DCMaelstromDebug")) && OceanGrids.IsEmpty())
		{
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				if (It->GetClass()->GetPathName().Contains(TEXT("DRW")) || It->GetClass()->GetPathName().Contains(TEXT("DynamicRealWater")))
				{ UE_LOG(LogDC, Log, TEXT("Maelstrom ocean candidate: %s class %s hidden %d"), *It->GetName(), *It->GetClass()->GetPathName(), It->IsHidden()); }
			}
		}
	}
	for (const TWeakObjectPtr<AActor>& Grid : OceanGrids)
	{
		if (Grid.IsValid()) { Grid->SetActorHiddenInGame(!bVisible); }
	}
	if (Ocean.IsValid()) { Ocean->SetActorHiddenInGame(!bVisible); }
	bOceanHidden = !bVisible;
}

void ADCMaelstrom::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { return; }
	// Isolated review hook: enter once, then run the normal timeline without repeated seeks.
	if (PreviousTime < 0.f)
	{
		float ReviewStart = 0.f;
		if (FParse::Value(FCommandLine::Get(), TEXT("DCMaelstromStart="), ReviewStart)) { Director->Seek(ReviewStart); }
	}
	DisableLegacyEncounter();
	Elapsed = Director->RideTime - BreachStart;
	const bool bNowActive = Director->RideTime >= SlideStart && Director->RideTime < EndingStart;
	if (bNowActive != bActive)
	{
		bActive = bNowActive;
		if (bActive) { bAutomaticHelmView = true; }
		SetStageVisible(bActive);
		if (!bActive)
		{
			RestoreView();
			RestoreAtmosphere();
			if (!Ocean.IsValid())
			{
				if (USceneComponent* OldSea = Director->GetSea()) { OldSea->SetVisibility(true, true); }
			}
		}
		UE_LOG(LogDC, Log, TEXT("Maelstrom stage %s at %.2f"), bActive ? TEXT("entered") : TEXT("exited"), Director->RideTime);
	}
	if (bActive) { UpdateStage(DeltaSeconds); UpdateView(DeltaSeconds); }
	else
	{
		if (bOceanHidden) { SetOceanVisible(true); }
		const float ExitTime = Director->RideTime - EndingStart;
		Exposure->BlendWeight = ExitTime >= 0.f && ExitTime < 2.f ? 1.f : 0.f;
		Exposure->Settings.AutoExposureBias = 1.f + 5.f * FMath::Square(FMath::Clamp(1.f - ExitTime / 1.7f, 0.f, 1.f));
		if (ExitTime >= 0.f && Ocean.IsValid())
		{
			Ocean->SetActorHiddenInGame(false);
			if (USceneComponent* OldSea = Director->GetSea()) { OldSea->SetVisibility(false, true); }
		}
	}
	if (Boss) { Boss->UpdateEncounter(Elapsed, GetArenaTransform(), Radius, Depth, bActive); }
	if (Atmosphere) { Atmosphere->UpdateAtmosphere(this, Elapsed, bActive); }
	WaterBridge.Update(GetWorld(), WaterMID, bActive);
	if (bActive && Boss && Boss->ShouldExitEncounter(Elapsed))
	{
		// Let the defeated arms withdraw before the cut back to open sea.
		Director->Seek(EndingStart);
	}
	PreviousTime = Director->RideTime;
}

void ADCMaelstrom::UpdateStage(float DeltaSeconds)
{
	const float Descent = FMath::SmoothStep(18.f, EndingStart - BreachStart - 15.f, Elapsed);
	ShipRadius = Radius * FMath::Lerp(0.985f, 0.78f, Descent);
	const ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	const float HelmTarget = Treadmill ? Treadmill->GetSteer() * Treadmill->SteerAuthority : 0.f;
	if (PreviousTime < 0.f || Director->RideTime < PreviousTime || Director->RideTime - PreviousTime > 1.f) { HelmTurn = 0.f; }
	HelmTurn = FMath::FInterpTo(HelmTurn, HelmTarget, DeltaSeconds, 1.5f);
	// Read the shared helm state without changing the treadmill. Turning moves the
	// crater's bearing with inertia; the surface under the fixed hull stays anchored.
	const float Bearing = 40.f + 9.f * Descent - HelmTurn * 4.f;
	const FRotator Rotation(0.f, Bearing, 0.f);
	const FVector Center = Rotation.RotateVector(FVector(ShipRadius, 0.f, 0.f));
	const float Heave = FMath::Sin(Director->RideTime * 0.85f) * 35.f;
	Arena->SetWorldLocationAndRotation(Center + FVector(0.f, 0.f, Director->SeaLevelZ - HeightAt(FVector2D(-ShipRadius, 0.f)) + Heave), Rotation);
	const float ExitX = FMath::Clamp(26000.f - Elapsed * 4000.f, -15000.f, 145000.f);
	const float Entry = FMath::SmoothStep(SlideStart, SlideStart + 7.f, Director->RideTime);
	const float LengthScale = FMath::Lerp(0.5f, 1.f, Entry);
	const float WidthScale = FMath::Lerp(30000.f / 9000.f, 1.f, Entry);
	Tunnel->SetRelativeScale3D(FVector(LengthScale, WidthScale, WidthScale));
	Tunnel->SetRelativeLocation(FVector(ExitX, 0.f, Director->SeaLevelZ));
	Tunnel->SetVisibility(Elapsed < 10.f);
	Basin->SetVisibility(Elapsed >= -7.f);
	Falls->SetVisibility(Elapsed >= -7.f);
	RimLand->SetVisibility(Elapsed >= -7.f);
	for (AActor* Landmark : Landmarks) { if (Landmark) { Landmark->SetActorHiddenInGame(Elapsed < -7.f); } }
	const bool bInsideTunnel = ExitX - 200000.f * LengthScale <= -3000.f;
	if (USceneComponent* OldSea = Director->GetSea()) { OldSea->SetVisibility(!bInsideTunnel && !Ocean.IsValid(), true); }
	SetOceanVisible(!bInsideTunnel);
	if (LegacyWeather.IsValid()) { LegacyWeather->SetActorHiddenInGame(true); }
	static bool bDebugDumped = false;
	if (!bDebugDumped && Elapsed > 12.f && FParse::Param(FCommandLine::Get(), TEXT("DCMaelstromDebug")))
	{
		bDebugDumped = true;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			TInlineComponentArray<UPrimitiveComponent*> Primitives(*It);
			for (UPrimitiveComponent* Primitive : Primitives)
			{
				UMaterialInterface* Material = Primitive->GetMaterial(0);
				const FString Name = Material ? Material->GetPathName() : FString();
				if (Name.Contains(TEXT("Water")) || Name.Contains(TEXT("Sea")) || It->IsA<ADCWeatherFX>())
				{
					UE_LOG(LogDC, Log, TEXT("MAEL_VIS %s.%s actorHidden %d visible %d componentHidden %d at %s material %s"),
						*It->GetName(), *Primitive->GetName(), It->IsHidden(), Primitive->IsVisible(), Primitive->bHiddenInGame,
						*Primitive->Bounds.Origin.ToCompactString(), *Name);
				}
			}
		}
	}
	if (WaterMID)
	{
		WaterMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime);
		WaterMID->SetScalarParameterValue(TEXT("HullFlatten"), 1.f);
		WaterMID->SetScalarParameterValue(TEXT("OverflowFoam"), 1.f);
		WaterMID->SetScalarParameterValue(TEXT("SeaLevel"), Director->SeaLevelZ);
		WaterMID->SetVectorParameterValue(TEXT("BowlCenter"), FLinearColor(Arena->GetComponentLocation()));
	}
	if (TunnelMID)
	{
		TunnelMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime * 2.f);
		TunnelMID->SetScalarParameterValue(TEXT("FlowStrength"), 2.f);
		TunnelMID->SetScalarParameterValue(TEXT("TunnelFlow"), 1.f);
		TunnelMID->SetVectorParameterValue(TEXT("BowlCenter"), FLinearColor(0.f, 0.f, Director->SeaLevelZ + 9000.f));
	}
	if (CascadeMID) { CascadeMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime); }
	// A sea-level height-fog bank would fill the deep bowl. Move its
	// base down for this stage and restore it on exit, using the existing renderer.
	if (Elapsed >= -1.f)
	{
		bAtmosphereAdjusted = true;
		for (const FStageFogVolume& Volume : StageFogVolumes)
		{
			if (AActor* FogActor = Volume.Owner.Get())
			{
				if (FBoolProperty* Enabled = FindFProperty<FBoolProperty>(FogActor->GetClass(), TEXT("Render Global Volumetric Material")))
				{ Enabled->SetPropertyValue_InContainer(FogActor, false); }
			}
			if (UPrimitiveComponent* Mesh = Volume.Mesh.Get()) { Mesh->SetVisibility(false); Mesh->SetHiddenInGame(true); }
		}
		const float Clear = FMath::SmoothStep(-1.f, 5.f, Elapsed);
		static bool bReportedFog = false;
		for (const FStageFog& Saved : StageFogs)
		{
			if (UExponentialHeightFogComponent* Fog = Saved.Component.Get())
			{
				FVector Location = Saved.Location;
				Location.Z = FMath::Lerp(Saved.Location.Z, Arena->GetComponentLocation().Z - Depth + 10000.f, Clear);
				Fog->SetWorldLocation(Location);
				Fog->SetFogHeightFalloff(FMath::Lerp(Saved.Falloff, 0.0018f, Clear));
				// UDS maps the phase's weather input to a much denser fog (~0.0062).
				// Clamp the rendered component here so the abyss stays legible.
				Fog->SetFogDensity(FMath::Lerp(Fog->FogDensity, 0.0008f, Clear));
				Fog->SetVolumetricFogDistance(FMath::Lerp(Saved.Distance, 400000.f, Clear));
				if (!bReportedFog && Clear >= 1.f)
				{
					UE_LOG(LogDC, Log, TEXT("Maelstrom fog %s: density %.6f, second %.6f, falloff %.3f, baseZ %.0f"),
						*Fog->GetPathName(), Fog->FogDensity, Fog->SecondFogData.FogDensity, Fog->FogHeightFalloff, Location.Z);
				}
			}
		}
		if (Clear >= 1.f) { bReportedFog = true; }
	}
	else { RestoreAtmosphere(); }
	const float Glare = FMath::Exp(-FMath::Square((Elapsed - 6.5f) / 1.1f)) * 2.8f;
	Exposure->BlendWeight = 1.f;
	const FDCEnvState& Env = Director->GetEnv();
	Exposure->Settings.AutoExposureBias = Env.Exposure + (Director->bUDS ? 2.2f * (1.f - FMath::Clamp(Env.SunIntensity / 7.f, 0.f, 1.f)) : 0.f) + Glare;
}

void ADCMaelstrom::UpdateView(float DeltaSeconds)
{
	if (CaptainCamera.IsValid()) { CaptainCamera->ClearAdditiveOffset(); }
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ADCPlayerCharacter* Captain = PC ? Cast<ADCPlayerCharacter>(PC->GetPawn()) : nullptr;
	if (!Captain || PC->GetViewTarget() != Captain || Captain->GetGunSide() != EDCSide::None) { return; }
	UCameraComponent* Camera = Captain->FindComponentByClass<UCameraComponent>();
	if (!Camera) { return; }
	if (!bHadView)
	{
		CaptainCamera = Camera;
		OriginalEye = Camera->GetRelativeLocation();
		bHadView = true;
	}
	// Offset at the wheel leaves the central abyss clear while preserving player mouse-look.
	if (Captain->IsAtStation() && PC->WasInputKeyJustPressed(EKeys::C)) { bAutomaticHelmView = false; }
	if (Captain->IsAtStation() && bAutomaticHelmView)
	{
		const float Offset = FMath::SmoothStep(-2.f, 5.f, Elapsed);
		// Look through the opening before turning toward the abyss. Turning early
		// filled the reveal with the tunnel's near sidewall instead of its exit.
		const float Look = FMath::SmoothStep(4.f, 10.f, Elapsed);
		const FVector DesiredWorld = Captain->GetActorLocation() + FVector(-20.f * Offset, 180.f * Offset, 64.f + 216.f * Offset);
		Camera->SetWorldLocation(DesiredWorld);
		Camera->AddAdditiveOffset(FTransform(FRotator(-12.f * Look, 38.f * Look, 0.f)), 0.f);
	}
	Camera->SetFieldOfView(90.f);
}

void ADCMaelstrom::RestoreView()
{
	if (LegacyWeather.IsValid()) { LegacyWeather->SetActorHiddenInGame(false); }
	if (UCameraComponent* Camera = CaptainCamera.Get()) { Camera->SetRelativeLocation(OriginalEye); Camera->ClearAdditiveOffset(); }
	CaptainCamera.Reset();
	bHadView = false;
}

void ADCMaelstrom::RestoreAtmosphere()
{
	if (!bAtmosphereAdjusted) { return; }
	for (const FStageFogVolume& Volume : StageFogVolumes)
	{
		if (AActor* FogActor = Volume.Owner.Get())
		{
			if (FBoolProperty* Enabled = FindFProperty<FBoolProperty>(FogActor->GetClass(), TEXT("Render Global Volumetric Material")))
			{ Enabled->SetPropertyValue_InContainer(FogActor, Volume.bEnabled); }
		}
		if (UPrimitiveComponent* Mesh = Volume.Mesh.Get()) { Mesh->SetVisibility(Volume.bVisible); Mesh->SetHiddenInGame(Volume.bHidden); }
	}
	for (const FStageFog& Saved : StageFogs)
	{
		if (UExponentialHeightFogComponent* Fog = Saved.Component.Get())
		{
			Fog->SetWorldLocation(Saved.Location);
			Fog->SetFogHeightFalloff(Saved.Falloff);
			Fog->SetFogDensity(Saved.Density);
			Fog->SetVolumetricFogDistance(Saved.Distance);
		}
	}
	bAtmosphereAdjusted = false;
}

void ADCMaelstrom::EndPlay(const EEndPlayReason::Type Reason)
{
	WaterBridge.Reset(WaterMID);
	RestoreView();
	RestoreAtmosphere();
	if (bOceanHidden) { SetOceanVisible(true); }
	for (AActor* Landmark : Landmarks) { if (IsValid(Landmark)) { Landmark->Destroy(); } }
	if (IsValid(Boss)) { Boss->Destroy(); }
	if (IsValid(Atmosphere)) { Atmosphere->Destroy(); }
	Super::EndPlay(Reason);
}
