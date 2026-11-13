#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "DCRuins.h"
#include "DCKraken.h"
#include "DCKrakenHead.h"
#include "DCWeatherFX.h"
#include "DCLeviathan.h"
#include "DCPlayer.h"
#include "DCTreadmill.h"
#include "ProceduralMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
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
	Tunnel = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ExitTunnel"));
	Tunnel->SetupAttachment(RootComponent);
	for (UProceduralMeshComponent* Mesh : {Basin.Get(), Falls.Get(), Tunnel.Get()})
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
	}
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
	Exposure->Settings.BloomIntensity = 0.65f;
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
			Phase.Env.FogDensity = 0.00015f;
			Phase.Env.FogHeightFalloff = 0.08f;
			Phase.Env.Mist = 0.f;
			Phase.Env.Rain = 0.f;
			Phase.Env.SunIntensity = 9.f;
			Phase.Env.SunColor = FLinearColor(1.f, 0.93f, 0.8f);
			Phase.Env.SunPitch = -15.f;
			Phase.Env.SunYaw = 210.f;
			Phase.Env.CloudCover = 0.16f;
			Phase.Env.CloudBase = 260000.f;
			Phase.Env.SkyIntensity = 1.5f;
			Phase.Env.Exposure = -0.35f;
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
	if (UMaterialInterface* Spray = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromSpray.M_MaelstromSpray")))
	{
		SprayMID = UMaterialInstanceDynamic::Create(Spray, this);
		Falls->SetMaterial(0, SprayMID);
	}
	BuildBasin();
	BuildTunnel();
	BuildLandmarks();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		// UDS also injects a sea-level density field through a volume material. It
		// cannot follow a 650 m crater; retain ordinary volumetric fog for shafts.
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
	SetStageVisible(false);
	UE_LOG(LogDC, Log, TEXT("Maelstrom stage: %.1f km across, %.0f m deep, breach %.1f ending %.1f"), Radius / 50000.f, Depth / 100.f, BreachStart, EndingStart);
}

float ADCMaelstrom::HeightAt(const FVector2D& Point, bool bWaves) const
{
	const float R = Point.Size();
	const float U = FMath::Clamp(R / Radius, 0.f, 1.f);
	const float Height = -Depth * FMath::Pow(FMath::Max(0.f, 1.f - U * U), 1.25f);
	if (!bWaves) { return Height; }
	const float A = FMath::Atan2(Point.Y, Point.X);
	const float Crest = FMath::Sin(U * 46.f + A * 7.f + FMath::Sin(A * 5.f + U * 14.f) * 1.4f);
	const float Cross = FMath::Sin(U * 91.f - A * 11.f + FMath::Sin(U * 33.f) * 0.8f);
	const float BrokenSwell = FMath::Clamp(0.65f + FMath::PerlinNoise2D(Point / 28000.f), 0.25f, 1.f);
	return Height + (1250.f * Crest * BrokenSwell + 420.f * Cross) * FMath::SmoothStep(0.06f, 0.28f, U);
}

void ADCMaelstrom::BuildBasin()
{
	constexpr int32 Rings = 240;
	constexpr int32 Sectors = 768;
	TArray<FVector> Vertices, Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Triangles;
	TArray<FProcMeshTangent> Tangents;
	for (int32 Ring = 0; Ring <= Rings; ++Ring)
	{
		const float R = FMath::Lerp(120.f, Radius * 1.24f, static_cast<float>(Ring) / Rings);
		for (int32 Side = 0; Side <= Sectors; ++Side)
		{
			const float A = Side * UE_TWO_PI / Sectors;
			const FVector2D P(FMath::Cos(A) * R, FMath::Sin(A) * R);
			const float Z = HeightAt(P);
			const float DX = (HeightAt(P + FVector2D(60.f, 0.f)) - HeightAt(P - FVector2D(60.f, 0.f))) / 120.f;
			const float DY = (HeightAt(P + FVector2D(0.f, 60.f)) - HeightAt(P - FVector2D(0.f, 60.f))) / 120.f;
			Vertices.Add(FVector(P.X, P.Y, Z));
			Normals.Add(FVector(-DX, -DY, 1.f).GetSafeNormal());
			Tangents.Add(FProcMeshTangent(FVector(1.f, 0.f, DX).GetSafeNormal(), false));
			UVs.Add(P / 10000.f);
			const float Crest = FMath::Clamp((Z - HeightAt(P, false) - 200.f) / 1250.f, 0.f, 1.f);
			Colors.Add(FLinearColor(Crest, R / Radius, FMath::Clamp(-Z / Depth, 0.f, 1.f), 1.f).ToFColor(false));
			if (Ring < Rings && Side < Sectors)
			{
				const int32 I = Ring * (Sectors + 1) + Side;
				const int32 J = I + Sectors + 1;
				Triangles.Append({I, I + 1, J, I + 1, J + 1, J});
			}
		}
	}
	Basin->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	Vertices.Reset(); Normals.Reset(); UVs.Reset(); Colors.Reset(); Triangles.Reset();
	// Discontinuous curtains around the far rim, leaving the tunnel approach open.
	for (int32 Strip = 0; Strip < 48; ++Strip)
	{
		const float Center = Strip * UE_TWO_PI / 48.f;
		if (FMath::Abs(FMath::UnwindRadians(Center - UE_PI)) < 0.42f) { continue; }
		const int32 Start = Vertices.Num();
		for (int32 X = 0; X <= 12; ++X)
		{
			const float A = Center + (static_cast<float>(X) / 12.f - 0.5f) * 0.115f;
			const float Top = 9000.f + 5000.f * FMath::Sin(Center * 3.f);
			for (int32 Y = 0; Y <= 8; ++Y)
			{
				const float T = Y / 8.f;
				const float R = Radius * (1.f - T * 0.055f);
				Vertices.Add(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, FMath::Lerp(Top, -14000.f, T)));
				Normals.Add(FVector(-FMath::Cos(A), -FMath::Sin(A), 0.f));
				UVs.Add(FVector2D(X / 12.f, T));
				Colors.Add(FColor::White);
				if (X < 12 && Y < 8)
				{
					const int32 I = Start + X * 9 + Y;
					Triangles.Append({I, I + 9, I + 1, I + 1, I + 9, I + 10});
				}
			}
		}
	}
	Falls->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
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
	for (int32 I = 0; I < 10; ++I)
	{
		const float Angle = FMath::Lerp(-145.f, 145.f, I / 9.f);
		const float Radians = FMath::DegreesToRadians(Angle);
		const float R = Radius * Random.FRandRange(1.03f, 1.1f);
		const FVector Position(R * FMath::Cos(Radians), R * FMath::Sin(Radians), 7000.f);
		const float ComplexScale = Random.FRandRange(1.8f, 3.4f);
		Place(DCRuins::SpawnCrag(GetWorld(), 22000.f, 21000.f), Position - FVector(0.f, 0.f, 18000.f), Angle);
		// Broad sanctuary platforms need a rocky island, not one narrow peak under
		// their centre. Overlapping footings support the edges from the side guns.
		for (int32 X : {-1, 1})
		{
			for (int32 Y : {-1, 1})
			{
				const FVector Offset = FRotator(0.f, Angle, 0.f).RotateVector(FVector(X * ComplexScale * 2800.f, Y * ComplexScale * 2300.f, -18000.f));
				Place(DCRuins::SpawnCrag(GetWorld(), ComplexScale * 7500.f, 21000.f), Position + Offset, Angle + X * 35.f);
			}
		}
		Place(DCRuins::SpawnComplex(GetWorld(), ComplexScale, 419 + I), Position, Angle + 180.f);
		if (I % 2 == 0)
		{
			Place(DCRuins::SpawnColossus(GetWorld(), 26000.f), Position + FVector(4000.f, 9000.f, 1000.f), Angle + 180.f);
		}
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
	if (bActive && Boss && Boss->ShouldExitEncounter(Elapsed))
	{
		// Finish after the defeated body has sunk instead of holding an empty arena.
		Director->Seek(EndingStart);
	}
	PreviousTime = Director->RideTime;
}

void ADCMaelstrom::UpdateStage(float DeltaSeconds)
{
	const float Descent = FMath::SmoothStep(4.f, EndingStart - BreachStart - 4.f, Elapsed);
	ShipRadius = Radius * FMath::Lerp(0.97f, 0.58f, Descent);
	const ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	const float HelmTarget = Treadmill ? Treadmill->GetSteer() * Treadmill->SteerAuthority : 0.f;
	if (PreviousTime < 0.f || Director->RideTime < PreviousTime || Director->RideTime - PreviousTime > 1.f) { HelmTurn = 0.f; }
	HelmTurn = FMath::FInterpTo(HelmTurn, HelmTarget, DeltaSeconds, 1.5f);
	// Read the shared helm state without changing the treadmill. Turning moves the
	// crater's bearing with inertia; the surface under the fixed hull stays anchored.
	const float Bearing = 19.f + 9.f * Descent - HelmTurn * 4.f;
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
	if (SprayMID) { SprayMID->SetScalarParameterValue(TEXT("Clock"), Director->RideTime); }
	// A sea-level height-fog bank would fill the entire 650 m-deep bowl. Move its
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
				Location.Z = FMath::Lerp(Saved.Location.Z, Arena->GetComponentLocation().Z - Depth + 2000.f, Clear);
				Fog->SetWorldLocation(Location);
				Fog->SetFogHeightFalloff(FMath::Lerp(Saved.Falloff, 0.08f, Clear));
				// UDS maps the phase's weather input to a much denser fog (~0.0062).
				// Clamp the rendered component here so the half-kilometre abyss stays legible.
				Fog->SetFogDensity(FMath::Lerp(Fog->FogDensity, 0.00015f, Clear));
				Fog->SetVolumetricFogDistance(FMath::Lerp(Saved.Distance, 220000.f, Clear));
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
		const FVector DesiredWorld = Captain->GetActorLocation() + FVector(140.f * Offset, 520.f * Offset, 64.f + 180.f * Offset);
		Camera->SetWorldLocation(DesiredWorld);
		Camera->AddAdditiveOffset(FTransform(FRotator(-10.f * Offset, 14.f * Offset, 0.f)), 0.f);
	}
	Camera->SetFieldOfView(90.f + 3.f * FMath::SmoothStep(2.f, 8.f, Elapsed));
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
	RestoreView();
	RestoreAtmosphere();
	if (bOceanHidden) { SetOceanVisible(true); }
	for (AActor* Landmark : Landmarks) { if (IsValid(Landmark)) { Landmark->Destroy(); } }
	if (IsValid(Boss)) { Boss->Destroy(); }
	Super::EndPlay(Reason);
}
