#include "DCMaelstrom.h"
#include "DCCliffGenerator.h"
#include "DCMaelstromRoute.h"
#include "DCRideDirector.h"
#include "DCRuins.h"
#include "DCKraken.h"
#include "DCKrakenHead.h"
#include "DCWeatherFX.h"
#include "DCLeviathan.h"
#include "DCMaelstromAtmosphere.h"
#include "DCPlayer.h"
#include "DCTreadmill.h"
#include "DCTroubles.h"
#include "DCAudio.h"
#include "Kismet/GameplayStatics.h"
#include "DCCrew.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "SparseVolumeTexture/SparseVolumeTexture.h"
#include "HAL/IConsoleManager.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/PointLightComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "KismetProceduralMeshLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"
#include "Sound/SoundBase.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

ADCMaelstrom::ADCMaelstrom()
{
	PrimaryActorTick.bCanEverTick = true;
	// Update the stage camera after pawn movement but BEFORE the camera manager
	// caches its view (between PostPhysics and PostUpdateWork).
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Arena = CreateDefaultSubobject<USceneComponent>(TEXT("BasinFrame"));
	Arena->SetupAttachment(RootComponent);
	Basin = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Basin"));
	Basin->SetupAttachment(Arena);
	Cataract=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("EntryCataract"));
	Cataract->SetupAttachment(Arena);
	Cataract->SetMobility(EComponentMobility::Movable);
	Cataract->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cataract->SetCastShadow(false);
	InletBed=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("EntryRockLandform"));
	InletBed->SetupAttachment(Arena);
	InletBed->SetMobility(EComponentMobility::Movable);
	InletBed->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InletBed->SetCastShadow(true);
	Whitewater=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SpiralWhitewaterFeeds"));
	Whitewater->SetupAttachment(Arena);
	Whitewater->SetMobility(EComponentMobility::Movable);
	Whitewater->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Whitewater->SetCastShadow(false);
	CrestSpray=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CrestSpray"));
	CrestSpray->SetupAttachment(Arena); CrestSpray->SetCollisionEnabled(ECollisionEnabled::NoCollision); CrestSpray->SetCastShadow(false);
	ContactMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ContactSpray"));
	ContactMesh->SetupAttachment(RootComponent);
	ContactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ContactMesh->SetCastShadow(false);
	Falls = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RimCascades"));
	Falls->SetupAttachment(Arena);
	RimLand = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ContinuousRim"));
	RimLand->SetupAttachment(Arena);
	Cliffs=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CraterRockWalls"));
	Cliffs->SetupAttachment(Arena);
	Cliffs->SetMobility(EComponentMobility::Movable);
	Cliffs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cliffs->SetCastShadow(true);
	Tunnel = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ExitTunnel"));
	Tunnel->SetupAttachment(RootComponent);
	for (UProceduralMeshComponent* Mesh : {Basin.Get(), Falls.Get(), Tunnel.Get(), RimLand.Get()})
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
	}
	// The high density water mesh must not fill the virtual shadow marking queue.
	Basin->SetCastShadow(false); // A single half-million-triangle movable shadow caster overflows VSM marking. Rim rocks supply local shadows.
	RimLand->SetCastShadow(false);
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
	ContactSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Sonniss/water_impact.water_impact"),nullptr,LOAD_NoWarn|LOAD_Quiet);
	if (UMaterialInterface* SprayMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_ContactSpray.M_ContactSpray"))) { ContactMesh->SetMaterial(0, SprayMat); }
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
			Phase.Env.FogColor = FLinearColor(.55f,.67f,.77f);
			Phase.Env.FogDensity = 0.0045f;
			Phase.Env.FogHeightFalloff = 0.0018f;
			Phase.Env.Mist = 0.f;
			Phase.Env.Rain = 0.f;
			Phase.Env.SunIntensity = 9.f;
			Phase.Env.SunColor = FLinearColor(1.f, 0.98f, 0.94f);
			Phase.Env.SunPitch = -28.f;
			Phase.Env.SunYaw = 258.f;
			Phase.Env.CloudCover = 0.20f;
			Phase.Env.CloudBase = 260000.f;
			Phase.Env.SkyIntensity = 1.5f;
			Phase.Env.Exposure = 0.15f;
			Phase.Env.LightningRate = 0.f;
		}
		if (Phase.Phase == EDCPhase::Ending) { Phase.BlendTime = 0.f; }
	}
	if (UMaterialInterface* Water = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromWater.M_MaelstromWater")))
	{
		WaterMID = UMaterialInstanceDynamic::Create(Water, this);
		CataractMID = UMaterialInstanceDynamic::Create(Water, this);
		TunnelMID = UMaterialInstanceDynamic::Create(Water, this);
		WaterMID->SetScalarParameterValue(TEXT("WaveHeight"),400.f);
		WaterMID->SetScalarParameterValue(TEXT("CalmCaldera"),1.f);
		WaterMID->SetScalarParameterValue(TEXT("BasinRadius"),Radius*.01f);
		WaterMID->SetScalarParameterValue(TEXT("FlowHandedness"),-1.f);
		Basin->SetMaterial(0, WaterMID);
		CataractMID->CopyInterpParameters(WaterMID);
		CataractMID->SetScalarParameterValue(TEXT("CataractFlow"),1.f);
		CataractMID->SetScalarParameterValue(TEXT("FoamCoverage"),.62f);
		Cataract->SetMaterial(0, CataractMID);
		Tunnel->SetMaterial(0, TunnelMID);
	}
	if (UMaterialInterface* Cascade = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_MaelstromCascade.M_MaelstromCascade")))
	{
		CascadeMID = UMaterialInstanceDynamic::Create(Cascade, this);
		Falls->SetMaterial(0, CascadeMID);
	}
	BuildBasin();
	BuildCataract();
	BuildInletBed();
	BuildTerrain();
	BuildTunnel();
	BuildLandmarks();
	BuildCliffPreview();
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
	const float JoinR=GetRimRadius(Angle)*GetRimJoin(Angle);
	const float JoinHeight=-Depth+Depth*FMath::Pow(JoinR/Radius,1.6f);
	return FMath::Max(1000.f,RimElevation(Angle)-JoinHeight);
}

float ADCMaelstrom::GetRimJoin(float Angle) const
{
	const float Join=FMath::Lerp(.90f,.84f+.11f*GetOverflowAmount(Angle),RimVariation(Angle));
	// The lower bowl must stay below every lip, including outward rim bays.
	// Otherwise a clamped fall height creates a discontinuity at the join.
	return FMath::Min(Join,.91f*Radius/GetRimRadius(Angle));
}

float ADCMaelstrom::RimElevation(float Angle) const
{
	return RimVariation(Angle) * (3500.f * FMath::Sin(Angle * 3.f - 0.5f) + 1600.f * FMath::Sin(Angle * 8.f + 1.2f));
}

float ADCMaelstrom::BaseHeightAt(const FVector2D& Point) const
{
	const float R = Point.Size();
	const float A = FMath::Atan2(Point.Y, Point.X);
	const float LocalRadius = GetRimRadius(A);
	const float U = FMath::Clamp(R / LocalRadius, 0.f, 1.f);
	// One continuous surface: broad entry ramp, steep far overflow, deep inner slope.
	const float Approach = FMath::Pow(FMath::Max(0.f, -FMath::Cos(A)), 8.f);
	const float Join = GetRimJoin(A);
	const float LipHeight = RimElevation(A);
	constexpr float Power = 1.6f;
	const float JoinR=Join*LocalRadius;
	const float JoinHeight=-Depth+Depth*FMath::Pow(JoinR/Radius,Power);
	const float LipDrop=FMath::Max(1000.f,LipHeight-JoinHeight);
	float Height;
	if (U <= Join)
	{
		// A single bowl below the overflow. Interpolating between different
		// angular bowl profiles made large buttresses and warped shiny terrain.
		Height=-Depth+Depth*FMath::Pow(R/Radius,Power);
	}
	else
	{
		const float T = (1.f - U) / (1.f - Join);
		// A rounded approach shoulder hides the crater floor from the helm.
		// Start the entry at a downhill grade; keep the distant overflow lip soft.
		const float M0 = FMath::Lerp(0.08f, 0.65f, Approach);
		const float M1 = (Depth + JoinHeight) * Power * (1.f - Join) / (Join * LipDrop);
		const float Profile = (M0 + M1 - 2.f) * T * T * T + (3.f - 2.f * M0 - M1) * T * T + M0 * T;
		Height = LipHeight - LipDrop * Profile;
	}
	return Height;
}

float ADCMaelstrom::HeightAt(const FVector2D& Point, bool bWaves) const
{
    // One level lake, reached by one finite river. No sculpted spiral banks.
    constexpr float LakeZ=-40000.f;
    const float R=Point.Size();
    const float Q=FMath::SmoothStep(.58f,.985f,R/Radius);
    const float A=UE_PI-.40f*(1.f-Q);
    const float Across=FMath::Abs(FMath::UnwindRadians(FMath::Atan2(Point.Y,Point.X)-A))*R;
    if(R>=Radius*.58f&&Across<=22000.f&&R<=Radius*1.08f)
    { return FMath::Lerp(LakeZ,0.f,Q); }
    return LakeZ;
}

float ADCMaelstrom::RimGroundHeight(float R, float Angle) const
{
    // Low exposed cap. Architecture, rather than sinusoidal hills, owns the skyline.
    const FVector2D D(FMath::Cos(Angle),FMath::Sin(Angle));
    return 1800.f+RimElevation(Angle)*.25f+650.f*FMath::PerlinNoise2D(D*4.f+FVector2D(7,13))
        +1200.f*DCCliffGenerator::Facet(Angle/UE_TWO_PI,1.3f,48)
        +400.f*DCCliffGenerator::Facet(Angle/UE_TWO_PI,2.1f,192);
}
TArray<ADCMaelstrom::FWaterfallContact> ADCMaelstrom::GetWaterfallContacts() const
{
	TArray<FWaterfallContact> Contacts;
	for (const FWaterFeed& Feed : WaterFeeds) { Contacts.Add({Feed.Foot, Feed.Width}); }
	return Contacts;
}

void ADCMaelstrom::BuildBasin()
{
	constexpr int32 Rings = 160;
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
            // Evaluate the duplicate seam column at exactly zero, avoiding
            // sin(2PI) round-off in position, normal and advected water UVs.
            const float A=Side==Sectors ? 0.f : UE_TWO_PI*static_cast<float>(Side)/Sectors;
			const float R = FMath::Lerp(120.f, GetRimRadius(A)*1.10f, static_cast<float>(Ring) / Rings);
			const FVector2D P(FMath::Cos(A) * R, FMath::Sin(A) * R);
			const float Z = -40000.f;
			const float DX = 0.f;
			const float DY = 0.f;
			Vertices.Add(FVector(P.X, P.Y, Z));
			Normals.Add(FVector(-DX, -DY, 1.f).GetSafeNormal());
			Tangents.Add(FProcMeshTangent(FVector(1.f, 0.f, DX).GetSafeNormal(), false));
			UVs.Add(P / 10000.f);
			// Only swell displacement is a crest. Channel shaping must not paint
			// the entire raised bank with an uninterrupted band of foam.
			float Crest=0.f;
			// Ordinary ocean whitecaps come from the live ocean simulation.
			const float UpstreamAngle = A - (12.f / 3.5f) * FMath::Loge(FMath::Clamp(R / GetRimRadius(A), 0.08f, 1.f));
			Colors.Add(FLinearColor(Crest, R / GetRimRadius(A), FMath::Clamp(-Z / Depth, 0.f, 1.f),
				GetOverflowAmount(UpstreamAngle)*RimVariation(A)).ToFColor(false));
			if (Ring < Rings && Side < Sectors)
			{
				const int32 I = Ring * (Sectors + 1) + Side;
				const int32 J = I + Sectors + 1;
				Triangles.Append({I, I + 1, J, I + 1, J + 1, J});
			}
		}
	}
    // Close the former1.2m centre opening with the same continuous water
    // surface. Shared first-ring vertices prevent a crack at the throat.
    const int32 Center=Vertices.Num();
    Vertices.Add(FVector(0.f,0.f,-40000.f));
    Normals.Add(FVector::UpVector); UVs.Add(FVector2D(0.f,0.f));
    Colors.Add(FLinearColor(0.f,0.f,1.f,0.f).ToFColor(false));
    Tangents.Add(FProcMeshTangent(FVector::ForwardVector,false));
    for(int32 Side=0;Side<Sectors;++Side) { Triangles.Append({Center,Side+1,Side}); }
	Basin->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	// Rock walls and discrete cliff falls are built separately from the water.

}

void ADCMaelstrom::BuildCataract()
{
    TArray<FVector> V,N; TArray<FVector2D> UV;
    TArray<int32> I; TArray<FColor> C; TArray<FProcMeshTangent> Tangents;
    constexpr int32 Rows=240,Sides=64;
    for(int32 Row=0;Row<=Rows;++Row)
    {
        const float F=static_cast<float>(Row)/Rows;
        const float R=Radius*FMath::Lerp(1.08f,.52f,F);
        const float Q=FMath::SmoothStep(.58f,.985f,R/Radius);
        const float A=UE_PI-.40f*(1.f-Q);
        const FVector2D D(FMath::Cos(A),FMath::Sin(A));
        for(int32 Side=0;Side<=Sides;++Side)
        {
            const float Across=FMath::Lerp(-22000.f,22000.f,static_cast<float>(Side)/Sides);
            // Use the same polar corridor as the excavated bed and route.
            const float SideAngle=A+Across/R;
            const FVector2D P(FMath::Cos(SideAngle)*R,FMath::Sin(SideAngle)*R);
            const float ActualQ=FMath::SmoothStep(.58f,.985f,static_cast<float>(P.Size())/Radius);
            // Sink the terminal overlap below the lake; both surfaces share
            // the same wave material and coordinates, with no exposed join.
            const float Z=FMath::Lerp(-40000.f,0.f,ActualQ)
                -(1.f-FMath::SmoothStep(.56f,.58f,static_cast<float>(P.Size())/Radius))*150.f;
            V.Add(FVector(P,Z)); UV.Add(P/10000.f);
            const float T=FMath::Clamp((static_cast<float>(P.Size())/Radius-.58f)/.405f,0.f,1.f);
            const float Slope=40000.f*6.f*T*(1.f-T)/(.405f*Radius);
            C.Add(FLinearColor(FMath::Clamp(Slope*.8f,0.f,1.f),P.Size()/Radius,.35f,0.f).ToFColor(false));
            if(Row<Rows&&Side<Sides)
            {
                const int32 K=Row*(Sides+1)+Side,J=K+Sides+1;
                I.Append({K,J,K+1,K+1,J,J+1});
            }
        }
    }
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V,I,UV,N,Tangents);
    Cataract->CreateMeshSection(0,V,I,N,UV,C,Tangents,false);
    UE_LOG(LogDC,Log,TEXT("CALDERA lake_level=-400m inlet_width=440m flat_lake=1 spiral_banks=0"));
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
		const float Along = -200000.f*FMath::Pow(1.f-X/static_cast<float>(LengthSteps),1.5f);
		for (int32 Y = 0; Y <= Around; ++Y)
		{
			const float A = Y * UE_TWO_PI / Around;
			const float Ripple = 650.f*FMath::Sin(A*5.f+Along*.00025f) + 350.f*FMath::Sin(A*11.f-Along*.00055f)
				+220.f*FMath::PerlinNoise2D(FVector2D(Along/2200.f,A*5.f));
			const float Mouth=FMath::SmoothStep(-24000.f,0.f,Along);
			const float R = TubeRadius + Ripple + Mouth*1200.f*(.65f+.35f*FMath::Sin(A*3.f+.7f));
			// Unequal receding lip: the daylight reveal must not expose a flat
			// circular cut through an otherwise cylindrical tube.
			const float Lip=Mouth*(2600.f*FMath::Sin(A*3.f+.4f)+1200.f*FMath::Sin(A*7.f-1.2f));
			Verts.Add(FVector(Along+Lip, FMath::Sin(A) * R, TubeRadius - FMath::Cos(A) * R));
			Normals.Add(FVector(0.f, -FMath::Sin(A), FMath::Cos(A)));
			Tangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
			UVs.Add(FVector2D(Along / 10000.f, A * TubeRadius / 10000.f));
			Colors.Add(FLinearColor(FMath::Clamp(Ripple / 1200.f, 0.f, 1.f), 0.5f, 0.2f, 1.f).ToFColor(false));
			if (X < LengthSteps && Y < Around)
			{
				const int32 I = X * (Around + 1) + Y, J = I + Around + 1;
				Triangles.Append({I, I + 1, J, I + 1, J + 1, J});
			}
		}
	}
	// Clockwise winding points INTO the tube; compute normals from the actual
	// corrugation and flared lip instead of shading it as a perfect cylinder.
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Verts,Triangles,UVs,Normals,Tangents);
	Tunnel->CreateMeshSection(0, Verts, Triangles, Normals, UVs, Colors, Tangents, false);
}

void ADCMaelstrom::BuildLandmarks()
{
    if (!DCRuins::Available()) { return; }
    int32 SupportedPieces=0;
    const FProcMeshSection* CliffSection=Cliffs->GetProcMeshSection(0);
    if(!CliffSection) { return; }
    const bool ExportAnchors=FParse::Param(FCommandLine::Get(),TEXT("DCExportCliffAnchors"));
    TArray<TSharedPtr<FJsonValue>> SupportAnchors;
    // Query the rendered triangles, including flattened sanctuary pads. The
    // broad RimGroundHeight function does not describe those actual footings.
    const auto GroundAt=[CliffSection](const FVector2D& P)
    {
        float Turns=FMath::Atan2(P.Y,P.X)/UE_TWO_PI;
        if(Turns<0.f) { Turns+=1.f; }
        const int32 Side=FMath::Min(FMath::FloorToInt(Turns*DCCliffGenerator::MeshSectors),DCCliffGenerator::MeshSectors-1);
        const auto Vertex=[&](int32 S,int32 Row)
        { return CliffSection->ProcVertexBuffer[Row*(DCCliffGenerator::MeshSectors+1)+S].Position; };
        float Ground=-MAX_flt;
        const auto Triangle=[&](const FVector& A,const FVector& B,const FVector& C)
        {
            const FVector2D AB(B-A),AC(C-A),AP=P-FVector2D(A);
            const double Det=AB.X*AC.Y-AB.Y*AC.X;
            if(FMath::Abs(Det)<.001) { return; }
            const double U=(AP.X*AC.Y-AP.Y*AC.X)/Det,VV=(AB.X*AP.Y-AB.Y*AP.X)/Det;
            if(U>=-.000001 && VV>=-.000001 && U+VV<=1.000001)
            { Ground=FMath::Max(Ground,static_cast<float>(A.Z+U*(B.Z-A.Z)+VV*(C.Z-A.Z))); }
        };
        for(int32 Row=static_cast<int32>(DCCliffGenerator::MeshRows*.80f);Row<DCCliffGenerator::MeshRows;++Row)
        {
            const FVector A=Vertex(Side,Row),B=Vertex(Side+1,Row),C=Vertex(Side,Row+1),D=Vertex(Side+1,Row+1);
            Triangle(A,B,C); Triangle(B,D,C);
        }
        return Ground;
    };
    auto Place=[this,&SupportedPieces,&GroundAt,&SupportAnchors,ExportAnchors](AActor* Actor,float AngleDegrees,float Setback,float Yaw)
    {
        if(!Actor) { return; }
        const float A=FMath::DegreesToRadians(AngleDegrees);
        const FVector2D Out(FMath::Cos(A),FMath::Sin(A));
        float R=CliffSurfacePoint(A,.84f).Size2D()+Setback;
        FVector BoundOrigin,BoundExtent;
        Actor->GetActorBounds(false,BoundOrigin,BoundExtent);
        const FRotator Rotation(0,Yaw,0);
        // Set a whole assemblage on the plateau, including the end columns of
        // tangent arcades. A centre-only test could leave their ends over air.
        float Ground=MAX_flt;
        bool bSupported=false;
        for(int32 Iteration=0;Iteration<8;++Iteration)
        {
            float Deficit=0.f;
            Ground=MAX_flt;
            for(int32 X=-1;X<=1;++X) for(int32 Y=-1;Y<=1;++Y)
            {
                const FVector Corner=Rotation.RotateVector(BoundOrigin+FVector(X*BoundExtent.X,Y*BoundExtent.Y,0));
                const FVector2D P=Out*R+FVector2D(Corner);
                const float CA=FMath::Atan2(P.Y,P.X),CR=P.Size();
                Deficit=FMath::Max(Deficit,static_cast<float>(CliffSurfacePoint(CA,.84f).Size2D()+500.f-CR));
                Ground=FMath::Min(Ground,GroundAt(P));
            }
            if(Deficit<1.f) { bSupported=Ground>-MAX_flt; break; }
            R+=Deficit+100.f;
        }
        if(!bSupported)
        {
            UE_LOG(LogTemp,Warning,TEXT("DC_RIM_CITY skipped unsupported group at %.2f degrees"),AngleDegrees);
            Actor->Destroy(); return;
        }
        const float Bottom=BoundOrigin.Z-BoundExtent.Z;
        // Bury the footing slightly; varying ground may cover broken drums,
        // but never leaves a continuous colonnade levitating above the rim.
        const FVector Position(Out*R,Ground-Bottom-100.f);
        Actor->AttachToComponent(Arena,FAttachmentTransformRules::KeepRelativeTransform);
        Actor->SetActorRelativeLocation(Position);
        Actor->SetActorRelativeRotation(Rotation);
        Landmarks.Add(Actor);
        if(ExportAnchors)
        {
            auto Anchor=MakeShared<FJsonObject>();
            Anchor->SetStringField(TEXT("id"),Actor->GetName());
            Anchor->SetNumberField(TEXT("support_height_m"),Ground*.01);
            Anchor->SetNumberField(TEXT("footing_bottom_m"),(Position.Z+Bottom)*.01);
            Anchor->SetNumberField(TEXT("margin_m"),10.);
            Anchor->SetNumberField(TEXT("blend_m"),30.);
            TArray<TSharedPtr<FJsonValue>> Polygon,Samples;
            // Same oriented actor bounds used above to prove placement, not a
            // world AABB enlarged by yaw, nor a guessed temple centre point.
            for(const FVector2D Sign:{FVector2D(-1,-1),FVector2D(1,-1),FVector2D(1,1),FVector2D(-1,1)})
            {
                const FVector P=Position+Rotation.RotateVector(BoundOrigin+FVector(Sign.X*BoundExtent.X,Sign.Y*BoundExtent.Y,0));
                Polygon.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{
                    MakeShared<FJsonValueNumber>(P.X*.01),MakeShared<FJsonValueNumber>(P.Y*.01)}));
            }
            for(int32 X=-1;X<=1;++X) for(int32 Y=-1;Y<=1;++Y)
            {
                const FVector P=Position+Rotation.RotateVector(BoundOrigin+FVector(X*BoundExtent.X,Y*BoundExtent.Y,0));
                auto Sample=MakeShared<FJsonObject>();
                Sample->SetNumberField(TEXT("x_m"),P.X*.01); Sample->SetNumberField(TEXT("y_m"),P.Y*.01);
                const float H=GroundAt(FVector2D(P));
                Sample->SetBoolField(TEXT("covered"),H>-MAX_flt);
                if(H>-MAX_flt) { Sample->SetNumberField(TEXT("ground_m"),H*.01); }
                Samples.Add(MakeShared<FJsonValueObject>(Sample));
            }
            Anchor->SetArrayField(TEXT("footprint_xy_m"),Polygon);
            Anchor->SetArrayField(TEXT("ground_samples"),Samples);
            SupportAnchors.Add(MakeShared<FJsonValueObject>(Anchor));
        }
        ++SupportedPieces;
    };
    // Unequal sanctuary districts, with paired and solitary buildings. The low
    // rim stays visible between them; no evenly repeated temple clock-face.
    static const float Angles[]={-148.f,-130.f,-81.f,-49.f,5.f,28.f,86.f,121.f,152.f};
    for(int32 Index=0;Index<UE_ARRAY_COUNT(Angles);++Index)
    {
        const float Scale=(Index==2 || Index==6) ? 1.30f : .90f+(Index%3)*.12f;
        Place(DCRuins::SpawnComplex(GetWorld(),Scale,419+Index),Angles[Index],12000.f,Angles[Index]+180.f);
    }
    // Foreground skyline accents join those districts into a ruined city:
    // broken colonnades follow the rim, with unequal spans and quieter gaps.
    static const float ColumnAngles[]={-157.f,-136.f,-123.f,-97.f,-73.f,-54.f,-20.f,-8.f,17.f,35.f,62.f,81.f,103.f,130.f,146.f,161.f};
    FRandomStream Layout(61147);
    for(int32 Index=0;Index<UE_ARRAY_COUNT(ColumnAngles);++Index)
    {
        const float A=ColumnAngles[Index];
        Place(DCRuins::SpawnColonnade(GetWorld(),3+Index%4,Layout.FRandRange(2000.f,3300.f)),
            A,Layout.FRandRange(1800.f,4200.f),A+90.f+Layout.FRandRange(-10.f,10.f));
        if(Index%3==1)
        {
            const float AA=A+Layout.FRandRange(2.1f,3.4f);
            Place(DCRuins::SpawnArch(GetWorld(),1+Index%4,Layout.FRandRange(1.05f,1.45f)),AA,5200.f,AA+180.f);
        }
        if(Index%2==0)
        {
            const float BA=A-Layout.FRandRange(2.f,3.6f);
            Place(DCRuins::SpawnBrokenColumns(GetWorld(),Layout.FRandRange(1600.f,2600.f)),BA,2600.f,BA+90.f);
        }
    }
    static const float StatueAngles[]={-152.f,-117.f,-83.f,-64.f,-31.f,-2.f,23.f,55.f,92.f,113.f,139.f,157.f};
    for(int32 Index=0;Index<UE_ARRAY_COUNT(StatueAngles);++Index)
    {
        const float A=StatueAngles[Index];
        const float Height=(Index==2 || Index==9) ? 6800.f : Layout.FRandRange(3000.f,4900.f);
        Place(DCRuins::SpawnColossus(GetWorld(),Height),A,Layout.FRandRange(1400.f,3200.f),A+180.f);
    }
    UE_LOG(LogTemp,Display,TEXT("DC_RIM_CITY %d supported groups: 9 sanctuaries, 16 broken colonnades, 5 arches, 8 debris courts, 12 statues; irregular skyline"),SupportedPieces);
    if(ExportAnchors)
    {
        auto Data=MakeShared<FJsonObject>();
        Data->SetStringField(TEXT("schema"),TEXT("directorscut.cliff_support_anchors.v1"));
        Data->SetStringField(TEXT("coordinate_frame"),TEXT("arena_local_metres"));
        Data->SetStringField(TEXT("source_surface"),TEXT("current rendered legacy polar BC triangles"));
        Data->SetNumberField(TEXT("radius_m"),Radius*.01);
        Data->SetNumberField(TEXT("depth_m"),Depth*.01);
        Data->SetNumberField(TEXT("source_vertices"),CliffSection->ProcVertexBuffer.Num());
        Data->SetNumberField(TEXT("source_triangles"),CliffSection->ProcIndexBuffer.Num()/3);
        FSHA1 Hash;
        for(const FProcMeshVertex& Vertex:CliffSection->ProcVertexBuffer)
        { Hash.Update(reinterpret_cast<const uint8*>(&Vertex.Position),sizeof(Vertex.Position)); }
        Hash.Update(reinterpret_cast<const uint8*>(CliffSection->ProcIndexBuffer.GetData()),CliffSection->ProcIndexBuffer.Num()*sizeof(uint32));
        Hash.Final(); uint8 Bytes[20]; Hash.GetHash(Bytes);
        Data->SetStringField(TEXT("source_geometry_sha1"),BytesToHex(Bytes,20));
        Data->SetStringField(TEXT("hash_encoding"),TEXT("native little-endian FVector positions then uint32 indices"));
        Data->SetArrayField(TEXT("anchors"),SupportAnchors);
        FString Json; FJsonSerializer::Serialize(Data,TJsonWriterFactory<>::Create(&Json));
        const FString Path=FPaths::ProjectSavedDir()/TEXT("CliffSupportAnchors.json");
        const bool Saved=FFileHelper::SaveStringToFile(Json,*Path);
        UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_ANCHORS saved=%d count=%d path=%s"),Saved,SupportAnchors.Num(),*Path);
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
    const FTransform Transform = GetArenaTransform();
    // Solve the world-vertical ray; replacing local Z alone changes XY when pitched.
    float Z = Director ? Director->SeaLevelZ : 230.f;
    const FVector LocalUp = Transform.InverseTransformVectorNoScale(FVector::UpVector);
    for (int32 Iteration=0; Iteration<7; ++Iteration)
    {
        const FVector Local = Transform.InverseTransformPosition(FVector(Position.X,Position.Y,Z));
        const FVector2D P(Local.X,Local.Y);
        const float H = HeightAt(P);
        const float DX = (HeightAt(P+FVector2D(100,0))-HeightAt(P-FVector2D(100,0)))/200.f;
        const float DY = (HeightAt(P+FVector2D(0,100))-HeightAt(P-FVector2D(0,100)))/200.f;
        const float Derivative = LocalUp.Z-DX*LocalUp.X-DY*LocalUp.Y;
        if (FMath::Abs(Derivative)<.15f) { break; }
        const float Error = Local.Z-H;
        Z -= FMath::Clamp(Error/Derivative,-150000.f,150000.f);
        if(FMath::Abs(Error)<.5f) { break; }
    }
    return FMath::Lerp(Director ? Director->SeaLevelZ : 230.f, Z,
        FMath::SmoothStep(1200.f,3500.f,static_cast<float>(FVector2D(Position.X*.5f,Position.Y*2.f).Size())));
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
	Cataract->SetVisibility(bVisible);
	InletBed->SetVisibility(bVisible,true);
	CrestSpray->SetVisibility(bVisible);
	Whitewater->SetVisibility(bVisible);
	Falls->SetVisibility(bVisible);
	RimLand->SetVisibility(bVisible);
	Cliffs->SetVisibility(bVisible);
	for(UInstancedStaticMeshComponent* Rock:CliffRocks) { if(Rock) { Rock->SetVisibility(bVisible); } }
	for(UInstancedStaticMeshComponent* Rock:CliffShadowRocks) { if(Rock) { Rock->SetVisibility(bVisible); } }
	Tunnel->SetVisibility(bVisible);
	if (!bVisible) { if (AActor* A = Throat.Get()) { A->SetActorHiddenInGame(true); } }
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
	auto ShowOcean = [this,bVisible](AActor* Actor)
	{
		if (!IsValid(Actor)) { return; }
		Actor->SetActorHiddenInGame(!bVisible);
		const TWeakObjectPtr<AActor> Key(Actor);
		if (!bVisible)
		{
			if (!OceanCollisionBeforeHide.Contains(Key)) { OceanCollisionBeforeHide.Add(Key, Actor->GetActorEnableCollision()); }
			// An invisible flat ocean must not intercept manual gun aim in the crater.
			Actor->SetActorEnableCollision(false);
		}
		else if (const bool* Saved = OceanCollisionBeforeHide.Find(Key))
		{
			Actor->SetActorEnableCollision(*Saved); OceanCollisionBeforeHide.Remove(Key);
		}
	};
	for (const TWeakObjectPtr<AActor>& Grid : OceanGrids) { ShowOcean(Grid.Get()); }
	ShowOcean(Ocean.Get());
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
			ClearTwisters();
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
		// The local exit fog masks the handoff; do not flash the exposure white.
		Exposure->BlendWeight = 0.f;
		if (ExitTime >= 0.f && Ocean.IsValid())
		{
			Ocean->SetActorHiddenInGame(false);
			if (USceneComponent* OldSea = Director->GetSea()) { OldSea->SetVisibility(false, true); }
		}
	}
	if (bActive) { UpdateTerrainShadows(); }
	UpdateProceduralFalls();
	UpdateContacts(DeltaSeconds);
	static const bool SceneReview=FParse::Param(FCommandLine::Get(),TEXT("DCSceneReview"));
	if (Boss) { Boss->UpdateEncounter(Elapsed, GetArenaTransform(), Radius, Depth, bActive && !SceneReview); }
	// Caldera combat is owned by DCLeviathan/DCBossCombat; keep legacy helpers dormant for merge isolation.
	if (ADCAudio* Sound = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(), ADCAudio::StaticClass())))
	{
		// The wind rises as a spout comes on and while the plume stands.
		float Roar = Boss ? Boss->GetCombatRoar() : 0.f;
		if (bActive)
		{
			for (const FTwister& T : Twisters) { Roar = FMath::Max(Roar, FMath::Clamp(1.f - FMath::Abs(T.Ahead) / 45000.f, 0.f, 1.f)); }
			if (AActor* P = Plume.Get()) { if (!P->IsHidden()) { Roar = FMath::Max(Roar, 0.8f); } }
		}
		Sound->SpoutRoar = Roar;
	} else if (Twisters.Num() || TwistersRaised || Boulders.Num() || BouldersThrown) { ClearTwisters(); }
	if (Atmosphere) { Atmosphere->UpdateAtmosphere(this, Elapsed, bActive); }
	WaterBridge.Update(GetWorld(), WaterMID, bActive);
	if(CataractMID&&WaterMID)
	{
		CataractMID->CopyInterpParameters(WaterMID);
		CataractMID->SetScalarParameterValue(TEXT("CataractFlow"),1.f);
		CataractMID->SetScalarParameterValue(TEXT("FoamCoverage"),.62f);
	}
	UpdateExitTransition();
	UpdateCliffPreview();
	UpdateInletMist();
	PreviousTime = Director->RideTime;
}

void ADCMaelstrom::EmitWaterContact(const FVector& Position, float Strength, bool ProjectToSurface)
{
	if (!bActive || Elapsed < 0.f) { return; }
	const FTransform Frame = GetArenaTransform();
	FVector Surface = Position;
	if (ProjectToSurface) { Surface.Z = SampleSurfaceWorld(Surface) + 35.f; }
	if (ContactSound && Strength>=1.f && Elapsed>NextContactSoundAt)
	{
		const ADCAudio* Mix=Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(),ADCAudio::StaticClass()));
		const float Distance=FVector2D(Position.X,Position.Y).Size();
		const float Volume=(Mix ? Mix->MasterVolume : .8f)*FMath::Clamp(1.f-Distance/60000.f,.08f,1.f)*.8f;
		UGameplayStatics::PlaySoundAtLocation(this,ContactSound,Surface,Volume,FMath::FRandRange(.78f,.95f));
		NextContactSoundAt=Elapsed+.3f;
	}
	const int32 Count = FMath::Clamp(FMath::RoundToInt(Strength * 45.f), 8, 110);
	for (int32 I = 0; I < Count && ContactParticles.Num() < 900; ++I)
	{
		const float Angle = FMath::FRand() * UE_TWO_PI;
		const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector Start = Surface + Radial * FMath::FRandRange(50.f, 350.f) * Strength;
		const FVector Velocity = Radial * FMath::FRandRange(250.f, 750.f) * FMath::Sqrt(Strength)
			+ FVector(0.f, 0.f, FMath::FRandRange(550.f, 1500.f) * FMath::Sqrt(Strength));
		ContactParticles.Add({Frame.InverseTransformPosition(Start), Frame.InverseTransformVectorNoScale(Velocity),
			0.f, FMath::FRandRange(1.0f, 2.4f), FMath::FRandRange(35.f, 130.f) * FMath::Sqrt(Strength)});
	}
}

void ADCMaelstrom::UpdateContacts(float DeltaSeconds)
{
	if (!bActive || Elapsed < 0.f)
	{
		ContactParticles.Reset(); ContactMesh->ClearAllMeshSections(); NextWakeAt = 0.f; return;
	}
	const float DT = FMath::Min(DeltaSeconds * Director->TimeScale, .1f);
	const FTransform Frame = GetArenaTransform();
	// Foam and droplets remain in the basin frame as the ship passes them.
	if (Elapsed > NextWakeAt && Elapsed > 12.f)
	{
		EmitWaterContact(FVector(2600.f, -750.f, 0.f), .14f);
		EmitWaterContact(FVector(2600.f, 750.f, 0.f), .14f);
		NextWakeAt = Elapsed + .16f;
	}
	FVector Eye; FRotator View;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) { PC->GetPlayerViewPoint(Eye, View); }
	else { Eye = FVector(0.f, 0.f, Director->SeaLevelZ + 800.f); }
	const FVector Gravity = Frame.InverseTransformVectorNoScale(FVector(0.f, 0.f, -980.f));
	TArray<FVector> V, N;
	TArray<FVector2D> UV;
	TArray<FColor> C;
	TArray<int32> Tri;
	TArray<FProcMeshTangent> Tangents;
	for (int32 I = ContactParticles.Num()-1; I >= 0; --I)
	{
		FContactParticle& P = ContactParticles[I];
		P.Age += DT;
		if (P.Age >= P.Life) { ContactParticles.RemoveAtSwap(I); continue; }
		P.Velocity += Gravity * DT;
		P.Position += P.Velocity * DT;
		const FVector Center = Frame.TransformPosition(P.Position);
		const FVector Normal = (Eye-Center).GetSafeNormal();
		FVector Right = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
		if (Right.IsNearlyZero()) { Right=FVector::RightVector; }
		const FVector Up = FVector::CrossProduct(Normal, Right);
		const float Size = P.Size * (1.f + P.Age * .7f);
		const float Alpha = FMath::SmoothStep(0.f, .08f, P.Age) * (1.f-FMath::SmoothStep(.2f,1.f,P.Age/P.Life));
		const int32 Start = V.Num();
		for (int32 Corner=0; Corner<4; ++Corner)
		{
			const float X=(Corner==0 || Corner==3) ? -1.f : 1.f;
			const float Y=Corner<2 ? -1.f : 1.f;
			V.Add(ContactMesh->GetComponentTransform().InverseTransformPosition(Center+(Right*X+Up*Y)*Size));
			N.Add(Normal); UV.Add(FVector2D((X+1.f)*.5f,(Y+1.f)*.5f));
			C.Add(FLinearColor(1.f,1.f,1.f,Alpha).ToFColor(false)); Tangents.Add(FProcMeshTangent(Right,false));
		}
		Tri.Append({Start,Start+1,Start+2,Start,Start+2,Start+3});
	}
	ContactMesh->CreateMeshSection(0,V,Tri,N,UV,C,Tangents,false);
}

void ADCMaelstrom::UpdateStage(float DeltaSeconds)
{
	// The tunnel feeds a moving river: turn left before the lip, then descend.
    const ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
    float Input = Treadmill ? Treadmill->GetSteer()*Treadmill->SteerAuthority : 0.f;
    static float ReviewSteer = 2.f;
    static const bool bReviewSteer = FParse::Value(FCommandLine::Get(),TEXT("DCMaelstromSteer="),ReviewSteer);
    if (bReviewSteer) { Input=FMath::Clamp(ReviewSteer,-1.f,1.f); }
    const bool Reset = PreviousTime<0.f || Director->RideTime<PreviousTime || Director->RideTime-PreviousTime>1.f;
    if (Reset) { RouteOffset=0.f; RouteVelocity=0.f; HelmTurn=0.f; ContactParticles.Reset(); NextWakeAt=0.f; NextContactSoundAt=-100.f; }
    const float DT = FMath::Min(DeltaSeconds*Director->TimeScale,.1f);
    const float Riding = FMath::SmoothStep(-4.f,12.f,Elapsed);
    // Smooth hydraulic restoring force at the visible banks. Centre stays free to steer.
    // Integrate in normalized lane coordinates so visible banks and helm
    // containment narrow together without a late hard offset clamp.
    const float Width=DCMaelstromRoute::WidthScale(Elapsed);
    const float PreviousWidth=DCMaelstromRoute::WidthScale(Elapsed-DT);
    RouteOffset/=PreviousWidth; RouteVelocity/=PreviousWidth;
    DCMaelstromRoute::AdvanceSteering(Input,DT,Riding,HelmTurn,RouteOffset,RouteVelocity);
    RouteOffset*=Width; RouteVelocity*=Width;
    const FVector2D P = DCMaelstromRoute::Point(Elapsed,Radius,RouteOffset);
    ShipRadius=P.Size();
    const float Angle=DCMaelstromRoute::Angle(Elapsed);
    const FVector2D Forward=DCMaelstromRoute::Tangent(FMath::Max(-7.9f,Elapsed),Radius);
    const float Bearing=-FMath::RadiansToDegrees(FMath::Atan2(Forward.Y,Forward.X))
        -DCMaelstromRoute::SteeringYaw(FMath::Max(-7.9f,Elapsed),Radius,RouteOffset,RouteVelocity*Riding);
    const FRotator Rotation(0.f,Bearing,0.f);
    const FVector2D Ahead=DCMaelstromRoute::Point(Elapsed+.10f,Radius,RouteOffset);
    const FVector2D Behind=DCMaelstromRoute::Point(Elapsed-.10f,Radius,RouteOffset);
    const float PathSlope=FMath::RadiansToDegrees(FMath::Atan2(HeightAt(Ahead,false)-HeightAt(Behind,false),
        FMath::Max(1.f,static_cast<float>((Ahead-Behind).Size()))));
    // Keep the shared sky and rim horizon level. Full inverse pitch tilts
    // the whole crater against its sky; vertical route travel supplies descent.
    const float Pitch=0.f; // Keep the shared sky/rim horizon level; the route itself supplies the drop.
    const float BankRoll=FMath::Clamp(RouteVelocity*.0014f+RouteOffset*.00018f,-3.5f,3.5f)*Riding;
    const FQuat Over=FRotator(Pitch+.35f*FMath::Sin(Elapsed*.65f)*Riding,0.f,BankRoll).Quaternion();
    const FQuat Pose=Over*Rotation.Quaternion();
    const FVector Ship(0.f,0.f,Director->SeaLevelZ);
    const float Heave=FMath::Sin(Director->RideTime*.85f)*25.f;
    Arena->SetWorldLocationAndRotation(Ship-Pose.RotateVector(FVector(P.X,P.Y,HeightAt(P)))+FVector(0,0,Heave),Pose);
    if(Elapsed>NextRouteLog && FParse::Param(FCommandLine::Get(),TEXT("DCMaelstromRouteLog")))
    {
        NextRouteLog=Elapsed+5.f;
        UE_LOG(LogDC,Log,TEXT("MAEL_ROUTE t=%.1f arc=%.1f radius=%.1fm offset=%.1fm velocity=%.1fm/s grade=%.1f depth=%.1fm travel=%.1fm/s"),Elapsed,FMath::RadiansToDegrees(Angle-UE_PI),ShipRadius*.01f,RouteOffset*.01f,RouteVelocity*.01f,PathSlope,-HeightAt(P,false)*.01f,(Ahead-Behind).Size()*.05f);
    }
	const float ExitX = FMath::Clamp(26000.f - Elapsed * 4000.f, -15000.f, 145000.f);
	const float Entry = FMath::SmoothStep(SlideStart, SlideStart + 7.f, Director->RideTime);
	const float LengthScale = FMath::Lerp(0.5f, 1.f, Entry);
	const float WidthScale = FMath::Lerp(30000.f / 9000.f, 1.f, Entry);
	Tunnel->SetRelativeScale3D(FVector(LengthScale, WidthScale, WidthScale));
	Tunnel->SetRelativeLocation(FVector(ExitX, 0.f, Director->SeaLevelZ));
	Tunnel->SetVisibility(Elapsed < 10.f);
	UpdateThroat(ExitX);
	Basin->SetVisibility(Elapsed >= -7.f);
	Cataract->SetVisibility(Elapsed >= -7.f);
	InletBed->SetVisibility(Elapsed >= -7.f,true);
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
		static float WaterReview=0.f;
		static const bool ReadWaterReview=FParse::Value(FCommandLine::Get(),TEXT("DCWaterReview="),WaterReview);
		WaterMID->SetScalarParameterValue(TEXT("WaterReview"),WaterReview);
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
				Location.Z = FMath::Lerp(Saved.Location.Z, Arena->GetComponentLocation().Z - Depth * .35f, Clear);
				Fog->SetWorldLocation(Location);
				Fog->SetFogHeightFalloff(FMath::Lerp(Saved.Falloff, 0.0018f, Clear));
				// Keep a light participating medium for shafts; local fall plumes
				// supply the dense banks instead of veiling the whole crater.
				static float StageFog=.0045f;
                static const bool HasStageFog=FParse::Value(FCommandLine::Get(),TEXT("DCStageFog="),StageFog);
                Fog->SetFogDensity(FMath::Lerp(Fog->FogDensity, StageFog, Clear));
				// Atmosphere owns volumetric distance and restores its entry value.
				// Do not change it before that actor captures a direct-seek entry.
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

void ADCMaelstrom::UpdateThroat(float ExitX)
{
	// Off unless -DCThroat. Tried 14 Nov: inside the dark water tunnel the cloud hardly shows, at the mouth it
	// whites the picture out, and past the mouth it hangs behind the ship as two lumps with a cut foot. It also
	// costs 20 fps. The exit wants shaping in the water tunnel itself, not a cloud laid over it.
	static const bool bOff = !FParse::Param(FCommandLine::Get(), TEXT("DCThroat"));
	const float Shown = bOff ? 0.f : FMath::SmoothStep(-16.f, -11.f, Elapsed) * (1.f - FMath::SmoothStep(8.f, 13.f, Elapsed));
	if (Shown <= 0.f)
	{
		if (AActor* A = Throat.Get()) { A->SetActorHiddenInGame(true); }
		return;
	}
	if (!Throat.IsValid() && !bThroatTried)
	{
		bThroatTried = true;
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_VdbCloud.M_VdbCloud"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		USparseVolumeTexture* Volume = LoadObject<USparseVolumeTexture>(nullptr, TEXT("/Game/Clouds_Tunnel_Loop/Effects/CT_Clouds_Tunnel_Vdb_01.CT_Clouds_Tunnel_Vdb_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHeterogeneousVolume* A = Base && Volume ? GetWorld()->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(), FTransform::Identity, Params) : nullptr;
		if (UHeterogeneousVolumeComponent* C = A ? A->FindComponentByClass<UHeterogeneousVolumeComponent>() : nullptr)
		{
			C->SetMobility(EComponentMobility::Movable);
			ThroatMID = UMaterialInstanceDynamic::Create(Base, this);
			ThroatMID->SetSparseVolumeTextureParameterValue(TEXT("SparseVolumeTexture"), Volume);
			ThroatMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.4f, 0.48f, 0.62f));
			C->SetMaterial(0, ThroatMID);
			C->StartFrame = 0.f; C->EndFrame = 200.f; C->FrameRate = 24.f; C->Frame = 90.f; C->bPlaying = true; C->bLooping = true;
			C->bIssueBlockingRequests = true;
			Throat = A;
			UE_LOG(LogDC, Log, TEXT("Maelstrom: cloud throat at the tunnel's mouth"));
		}
	}
	AHeterogeneousVolume* A = Throat.Get();
	if (!A || !ThroatMID) { return; }
	// 620 m of it, 180 m through: inside the water tunnel's 90 m radius, its last 40 m standing out of the mouth as a collar.
	static float Scale = 220.f, Sink = 3000.f; static bool bRead = false;   // test hooks: -DCThroatScale= -DCThroatSink=
	if (!bRead) { bRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCThroatScale="), Scale); FParse::Value(FCommandLine::Get(), TEXT("DCThroatSink="), Sink); }
	const float Half = 0.5f * 672.f * 0.42f * Scale;
	A->SetActorHiddenInGame(false);
	A->SetActorTransform(FTransform(FRotator(0.f, -90.f, 0.f), FVector(ExitX + 4000.f - Half, 0.f, Director->SeaLevelZ - Sink), FVector(Scale)));
	ThroatMID->SetScalarParameterValue(TEXT("Density"), Shown);
	ThroatMID->SetVectorParameterValue(TEXT("Floor"), FLinearColor(Sink / (Scale * 0.42f) + 2.f, 6.f, 0.f, 0.f));
}

namespace
{
	// Test hooks: -DCSpawnlingScale= (the mesh is 47 m long at 1), -DCSpawnlingSunk= (share of its height under water), -DCSpawnlingYaw=.
	float SpawnlingScale = 0.6f, SpawnlingSunk = 0.48f, SpawnlingYaw = 0.f;
	struct FSpawnlingHooks
	{
		FSpawnlingHooks()
		{
			FParse::Value(FCommandLine::Get(), TEXT("DCSpawnlingScale="), SpawnlingScale);
			FParse::Value(FCommandLine::Get(), TEXT("DCSpawnlingSunk="), SpawnlingSunk);
			FParse::Value(FCommandLine::Get(), TEXT("DCSpawnlingYaw="), SpawnlingYaw);
		}
	};
}

void ADCMaelstrom::ClearTwisters()
{
	for (FTwister& T : Twisters) { if (AActor* A = T.Actor.Get()) { A->Destroy(); } if (AActor* S = T.Skirt.Get()) { S->Destroy(); } if (AActor* P = T.Plain.Get()) { P->Destroy(); } }
	Twisters.Reset();
	TwisterMIDs.Reset();
	TwistersRaised = 0;
	if (AActor* P = Plume.Get()) { P->Destroy(); }
	if (ASkeletalMeshActor* Body = Carcass.Get()) { Body->Destroy(); }
	Plume.Reset(); PlumeMID = nullptr; bPlumeShaken = false;
	for (FSpawnling& S : Spawnlings) { if (AActor* A = S.Actor.Get()) { A->Destroy(); } if (AActor* M = S.Mark.Get()) { M->Destroy(); } }
	Spawnlings.Reset(); SpawnlingsSent = 0;
	for (FBoulder& B : Boulders) { if (AActor* A = B.Actor.Get()) { A->Destroy(); } }
	Boulders.Reset();
	BouldersThrown = 0;
}

void ADCMaelstrom::UpdateTwisters(float DeltaSeconds)
{
	// Seeking back starts the schedule again.
	if (PreviousTime >= 0.f && Director->RideTime < PreviousTime - 0.5f) { ClearTwisters(); }
	// The schedule: between the arms' strikes, none in the first half minute, none in the last twenty seconds.
	static const float RaiseAt[] = { 30.f, 50.f, 78.f, 96.f, 118.f, 134.f };
	const float Finale = Boss ? Boss->FinaleAt : 160.f;
	const float Tell = 5.f, Run = 3600.f, Start = 42000.f;   // 5 s standing, then 36 m a second up the slope from 420 m off
	static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("DCNoTwisters"));
	// The volume is played slowly with blocking streaming: without blocking it streams in at a coarse level and at
	// 4K the spout is a smooth grey slab with straight sides; blocking at 30 frames a second or more costs 8 to 15 ms
	// of render thread. At 24 it keeps its detail at 60 to 68 fps in 4K (66 to 74 at 14, but 14 pictures a second may show as a flicker in the boil).
	static float TwisterCut = 0.1f, TwisterStep = 1.f;   // test hooks: -DCTwisterCut= -DCTwisterStep=
	static float TwisterDensity = 0.7f, TwisterRate = 24.f; static bool bDensityRead = false, bTwisterBlocking = true;   // test hooks: -DCTwisterDensity= -DCTwisterRate= -DCTwisterNoBlocking
	if (!bDensityRead) { bDensityRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCTwisterDensity="), TwisterDensity); FParse::Value(FCommandLine::Get(), TEXT("DCTwisterRate="), TwisterRate); FParse::Value(FCommandLine::Get(), TEXT("DCTwisterCut="), TwisterCut); FParse::Value(FCommandLine::Get(), TEXT("DCTwisterStep="), TwisterStep); bTwisterBlocking = !FParse::Param(FCommandLine::Get(), TEXT("DCTwisterNoBlocking")); }
	while (!bOff && TwistersRaised < UE_ARRAY_COUNT(RaiseAt) && Elapsed >= RaiseAt[TwistersRaised])
	{
		const int32 Index = TwistersRaised++;
		if (Elapsed > RaiseAt[Index] + 3.f || RaiseAt[Index] > Finale - 20.f) { continue; }   // skipped over by a seek, or too near the end
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_VdbCloud.M_VdbCloud"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		USparseVolumeTexture* Volume = LoadObject<USparseVolumeTexture>(nullptr, TEXT("/Game/Clouds_Tunnel_Loop/Effects/CT_Clouds_Tunnel_Vdb_01.CT_Clouds_Tunnel_Vdb_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		static const bool bNoPack = FParse::Param(FCommandLine::Get(), TEXT("DCNoCloudPack"));   // test hook: as if the pack were not installed
		if (bNoPack) { Volume = nullptr; }
		FTwister T;
		T.RaisedAt = RaiseAt[Index];
		T.Ahead = Start;
		T.RouteOffsetAtSpawn = RouteOffset;
		// In her path, a little to one side: she must turn away from it, and either way will do.
		T.Abeam = (Index % 2 == 0 ? 1.f : -1.f) * FMath::FRandRange(400.f, 1800.f);
		if (Base && Volume)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AHeterogeneousVolume* A = GetWorld()->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(), FTransform::Identity, Params);
			if (UHeterogeneousVolumeComponent* C = A ? A->FindComponentByClass<UHeterogeneousVolumeComponent>() : nullptr)
			{
				C->SetMobility(EComponentMobility::Movable);
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
				M->SetSparseVolumeTextureParameterValue(TEXT("SparseVolumeTexture"), Volume);
				M->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.93f, 0.96f, 1.f));   // spray, not smoke: thin and white
				M->SetScalarParameterValue(TEXT("Cut"), TwisterCut);   // without it the rising spout shows as a pale box
				M->SetScalarParameterValue(TEXT("Density"), 0.f);
				// Spray scatters light all through itself; the volume's single shadowed bounce leaves it sooty. A little light of its own stands in for that.
				M->SetVectorParameterValue(TEXT("Flare"), FLinearColor(0.13f, 0.15f, 0.17f));
				C->SetMaterial(0, M);
				C->StartFrame = 0.f; C->EndFrame = 200.f; C->StepFactor = TwisterStep; C->ShadowStepFactor = TwisterStep * 2.f; C->FrameRate = TwisterRate; C->Frame = Index * 37.f; C->bPlaying = true; C->bLooping = true;
				C->bIssueBlockingRequests = bTwisterBlocking;
				if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance"))) { if (V->GetFloat() < 600000.f) { V->Set(600000.f, ECVF_SetByCode); } }
				T.Actor = A; T.MID = M;
				TwisterMIDs.Add(M);
			}
		}
		if (!T.Actor.IsValid())
		{
			// No volume: a plain pale column, so the hazard is at least seen.
			UMaterialInterface* Cloud = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud"), nullptr, LOAD_NoWarn | LOAD_Quiet);
			UStaticMesh* Column = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
			FActorSpawnParameters PlainParams;
			PlainParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaticMeshActor* P = Cloud && Column ? GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, PlainParams) : nullptr;
			if (P)
			{
				UStaticMeshComponent* C = P->GetStaticMeshComponent();
				C->SetMobility(EComponentMobility::Movable);
				C->SetStaticMesh(Column);
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				C->SetCastShadow(false);
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Cloud, this);
				M->SetVectorParameterValue(TEXT("Top"), FLinearColor(0.75f, 0.8f, 0.86f));
				M->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.9f, 0.93f, 0.97f));
				M->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, 0.f));
				C->SetMaterial(0, M);
				T.Plain = P; T.PlainMID = M;
				TwisterMIDs.Add(M);
			}
		}
		UStaticMesh* CardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/EasyFog/Meshes/SM_easyFogCard_02.SM_easyFogCard_02"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		UMaterialInterface* CardBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EasyFog/Materials/M_BP_EasyFog.M_BP_EasyFog"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (CardMesh && CardBase)
		{
			FActorSpawnParameters SkirtParams;
			SkirtParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AStaticMeshActor* S = GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, SkirtParams))
			{
				UStaticMeshComponent* C = S->GetStaticMeshComponent();
				C->SetMobility(EComponentMobility::Movable);
				C->SetStaticMesh(CardMesh);
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				C->SetCastShadow(false);
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(CardBase, this);
				if (UTexture* Mask = LoadObject<UTexture>(nullptr, TEXT("/Game/EasyFog/Textures/Fog/T_smoothRadial_mask.T_smoothRadial_mask"), nullptr, LOAD_NoWarn | LOAD_Quiet)) { M->SetTextureParameterValue(TEXT("Opacity Map"), Mask); }
				M->SetScalarParameterValue(TEXT("Fog Density"), 0.f);
				M->SetScalarParameterValue(TEXT("Fading Distance"), 1500.f);
				M->SetScalarParameterValue(TEXT("Camera Fading Distance"), 2500.f);
				M->SetScalarParameterValue(TEXT("Emissive Intensity"), 0.f);
				M->SetVectorParameterValue(TEXT("Base Color Tint"), FLinearColor(0.95f, 0.97f, 1.f));
				C->SetMaterial(0, M);
				T.Skirt = S; T.SkirtMID = M;
				TwisterMIDs.Add(M);
			}
		}
		Twisters.Add(T);
		UE_LOG(LogDC, Log, TEXT("Twister %d raised at %.1f s, %.0f m ahead, %.0f m abeam%s"), Index + 1, Elapsed, T.Ahead / 100.f, T.Abeam / 100.f, T.Actor.IsValid() ? TEXT("") : TEXT(" (no volume: unseen)"));
	}
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	for (int32 i = Twisters.Num() - 1; i >= 0; --i)
	{
		FTwister& T = Twisters[i];
		const float Age = Elapsed - T.RaisedAt;
		if (!T.bCalled && Age > 0.6f)
		{
			T.bCalled = true;
			if (Crew) { Crew->Report(T.Abeam > 0.f ? TEXT("Waterspout dead ahead! Turn to port, Captain!") : TEXT("Waterspout dead ahead! Turn to starboard, Captain!"), true); }
		}
		if (Age > Tell) { T.Ahead -= Run * DeltaSeconds * Director->TimeScale; }
		// The helm carries the ship across the current; seen from her deck the spout slides the other way.
		const float Across = T.Abeam + T.RouteOffsetAtSpawn - RouteOffset;
		const float Rise = FMath::SmoothStep(0.f, Tell, Age);
		const float Gone = 1.f - FMath::SmoothStep(-26000.f, -9000.f, T.Ahead);
		// It comes up the slope from the abyss, on a line a little to starboard of the bow, where the helm's view looks.
		FVector Foot = FRotator(0.f, 20.f, 0.f).RotateVector(FVector(T.Ahead, Across, 0.f));
		Foot.Z = SampleSurfaceWorld(Foot);
		// Tall enough to stand well over the ship however far down the slope its foot is.
		const float Height = FMath::Clamp(Director->SeaLevelZ - Foot.Z + 22000.f, 26000.f, 62000.f);
		if (AHeterogeneousVolume* A = T.Actor.Get())
		{
			// A narrow core leaves a readable gap to steer through; the old cloud
			// skirt obscured hundreds of metres of sea around a much smaller hit area.
			const float Scale = Height / 282.f * FMath::Lerp(0.45f, 1.f, Rise);
			A->SetActorTransform(FTransform(FRotator(0.f, Director->RideTime * 70.f, 90.f), Foot, FVector(Scale * 0.38f, Scale, Scale * 0.38f)));
			if (UHeterogeneousVolumeComponent* C = A->FindComponentByClass<UHeterogeneousVolumeComponent>())
			{
				// Wherever the turned volume's box has landed, stand it with its foot on the water at Foot.
				const FBoxSphereBounds B = C->Bounds;
				A->AddActorWorldOffset(FVector(Foot.X - B.Origin.X, Foot.Y - B.Origin.Y, Foot.Z - 600.f - (B.Origin.Z - B.BoxExtent.Z)));
			}
			if (UMaterialInstanceDynamic* M = T.MID.Get()) { M->SetScalarParameterValue(TEXT("Density"), TwisterDensity * Rise * (1.f - Gone)); }
			static const bool bLog = FParse::Param(FCommandLine::Get(), TEXT("DCTwisterLog"));
			if (bLog && FMath::Fmod(Age, 2.f) < DeltaSeconds)
			{
				const UHeterogeneousVolumeComponent* C = A->FindComponentByClass<UHeterogeneousVolumeComponent>();
				UE_LOG(LogDC, Log, TEXT("Twister: age %.1f foot %s actor %s bounds %s hidden %d res %s"), Age, *Foot.ToCompactString(), *A->GetActorLocation().ToCompactString(), C ? *C->Bounds.ToString() : TEXT("-"), A->IsHidden(), C ? *C->VolumeResolution.ToString() : TEXT("-"));
			}
		}
		if (AActor* P = T.Plain.Get())
		{
			// The engine's cylinder is 1 m through and 1 m high.
			const float Through = Height * 0.22f * FMath::Lerp(0.5f, 1.f, Rise);
			P->SetActorTransform(FTransform(FRotator(0.f, Director->RideTime * 70.f, 0.f), Foot + FVector(0.f, 0.f, Height * 0.5f * Rise - 300.f), FVector(Through / 100.f, Through / 100.f, Height * Rise / 100.f)));
			if (UMaterialInstanceDynamic* M = T.PlainMID.Get()) { M->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, 0.8f * Rise * (1.f - Gone))); }
		}
		if (AActor* S = T.Skirt.Get())
		{
			// A wide dome of white water lying on the slope where the spout stands, turning with it.
			const FVector Dx(600.f, 0.f, SampleSurfaceWorld(Foot + FVector(600.f, 0.f, 0.f)) - Foot.Z), Dy(0.f, 600.f, SampleSurfaceWorld(Foot + FVector(0.f, 600.f, 0.f)) - Foot.Z);
			const FVector Up = FVector::CrossProduct(Dx, Dy).GetSafeNormal();
			const FQuat Lie = FQuat::FindBetweenNormals(FVector::UpVector, Up) * FQuat(FVector::UpVector, FMath::DegreesToRadians(Director->RideTime * 40.f));
			// It boils up first: the patch of white water is there in a second and a half, before the column has risen.
			const float Boil = FMath::SmoothStep(0.f, 1.5f, Age);
			const float SkirtWidth = Height * 0.25f * FMath::Lerp(0.4f, 1.f, Boil);
			S->SetActorTransform(FTransform(Lie, Foot + Up * 100.f, FVector(SkirtWidth / 1160.f, SkirtWidth / 1160.f, SkirtWidth / 1160.f * .55f)));
			if (UMaterialInstanceDynamic* M = T.SkirtMID.Get()) { M->SetScalarParameterValue(TEXT("Fog Density"), .85f * Boil * (1.f - Gone)); }
		}
		// Struck: the spout's core passes over the ship.
		if (!T.bStruck && Age > Tell && FMath::Abs(Foot.X) < 3000.f && FMath::Abs(Foot.Y) < 2600.f)
		{
			T.bStruck = true;
			Director->DamageShip(3.f);
			if (ADCTroubles* Troubles = ADCTroubles::Get(this)) { Troubles->TearRigging(); }
			if (Crew) { Crew->Report(TEXT("The spout has us! Rigging's torn away!"), true); }
			UE_LOG(LogDC, Log, TEXT("Twister struck the ship at %.1f s (%.0f m abeam)"), Elapsed, T.Abeam / 100.f);
		}
		else if (!T.bStruck && T.Ahead < -3000.f)
		{
			T.bStruck = true;   // passed clear
			Director->Score += 100;
			UE_LOG(LogDC, Log, TEXT("Twister passed clear at %.1f s (%.0f m abeam)"), Elapsed, T.Abeam / 100.f);
		}
		if (Gone >= 1.f)
		{
			if (AActor* S = T.Skirt.Get()) { S->Destroy(); }
			if (AActor* P = T.Plain.Get()) { P->Destroy(); }
			if (AActor* A = T.Actor.Get()) { A->Destroy(); }
			Twisters.RemoveAt(i);
		}
	}
}

void ADCMaelstrom::UpdateSpawnlings(float DeltaSeconds)
{
	// Two in the later part of the fight, one on each beam.
	static const FSpawnlingHooks Hooks;
	static const float SendAt[] = { 92.f, 108.f };   // 48 s from sight to hull: the first is on her at 140 s, the second at 156
	const float Finale = Boss ? Boss->FinaleAt : 160.f;
	static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("DCNoSpawnlings"));
	static float Extra = -1.f; static bool bRead = false;   // test hook: -DCSpawnlingAt=<seconds after the breach> sends one more, to port
	if (!bRead) { bRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCSpawnlingAt="), Extra); }
	bool bSend = false;
	while (!bOff && SpawnlingsSent < UE_ARRAY_COUNT(SendAt) && Elapsed >= SendAt[SpawnlingsSent])
	{
		const float At = SendAt[SpawnlingsSent++];
		if (Elapsed <= At + 2.f && At < Finale - 50.f) { bSend = true; }
	}
	if (Extra >= 0.f && Elapsed >= Extra && Elapsed < Extra + 1.f) { Extra = -1.f; bSend = true; }
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	if (bSend && Treadmill)
	{
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimation.ClawKaiju_allAnimation"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		UAnimSequence* Crawl = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimationClawKaiju_Crawling.ClawKaiju_allAnimationClawKaiju_Crawling"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASkeletalMeshActor* A = Mesh ? GetWorld()->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params) : nullptr;
		if (A)
		{
			FSpawnling S;
			S.Side = SpawnlingsSent == 2 ? 1.f : -1.f;   // the first to port, the second to starboard
			S.Abeam = 30000.f;
			USkeletalMeshComponent* C = A->GetSkeletalMeshComponent();
			C->SetMobility(EComponentMobility::Movable);
			C->SetSkeletalMesh(Mesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (Crawl) { C->PlayAnimation(Crawl, true); C->SetPlayRate(.45f); }
			// Prefer the installed UDIM skin with its authored normal maps; keep a fallback for machines without it.
			UMaterialInterface* Hide = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/LeviathanSkin/M_LeviathanSkin.M_LeviathanSkin"), nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!Hide) { Hide=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_Kraken.M_Kraken"),nullptr,LOAD_NoWarn|LOAD_Quiet); }
			if (Hide)
			{
				UMaterialInstanceDynamic* HideMID = UMaterialInstanceDynamic::Create(Hide, A);
				HideMID->SetVectorParameterValue(TEXT("Skin"), FLinearColor(0.035f, 0.048f, 0.045f));
				HideMID->SetVectorParameterValue(TEXT("Belly"), FLinearColor(0.22f, 0.11f, 0.055f));
				for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot) { C->SetMaterial(Slot, HideMID); }
			}
			A->SetActorScale3D(FVector(SpawnlingScale));
			S.Actor = A;
			S.SafeDistance=FMath::Max(3500.f,(Mesh->GetBounds().BoxExtent.Size()+Mesh->GetBounds().Origin.Size())*SpawnlingScale+1800.f);
			S.NextSprayAt=Elapsed;
			// Somewhere to start, so the treadmill's first reading of it is on the right side.
			A->SetActorLocation(FVector(1500.f, S.Side * (S.Abeam + 600.f), Director->SeaLevelZ));
			// The guns' target is an unseen actor that rides with it. When the treadmill destroys that, the
			// creature itself is still here to roll over and go down instead of vanishing.
			if (AActor* Mark = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(A->GetActorLocation()), Params))
			{
				USceneComponent* Root = NewObject<USceneComponent>(Mark);
				Root->SetMobility(EComponentMobility::Movable);
				Mark->SetRootComponent(Root);
				Root->RegisterComponent();
				Mark->SetActorLocation(A->GetActorLocation());
				Treadmill->AddExternalHostile(Mark, 1500.f, 3.f, true);
				S.Mark = Mark;
			}
			Spawnlings.Add(S);
			UE_LOG(LogDC, Log, TEXT("Spawnling sent at %.1f s on the %s beam"), Elapsed, S.Side < 0.f ? TEXT("port") : TEXT("starboard"));
		}
	}
	for (int32 i = Spawnlings.Num() - 1; i >= 0; --i)
	{
		FSpawnling& S = Spawnlings[i];
		AActor* A = S.Actor.Get();
		if (!A) { Spawnlings.RemoveAt(i); continue; }
		if (S.SunkAt < 0.f && !S.Mark.IsValid())
		{
			// The guns have sunk it (the treadmill destroys a hostile whose health is gone).
			S.SunkAt = Elapsed;
			if (Crew) { Crew->Report(TEXT("It is going under! Good shooting!"), true); }
			UE_LOG(LogDC, Log, TEXT("Spawnling sunk at %.1f s, %.0f m off"), Elapsed, S.Abeam / 100.f);
			if (ASkeletalMeshActor* Body = Cast<ASkeletalMeshActor>(A))
			{
				if (UAnimSequence* Dying = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimationClawKaiju_Defeated.ClawKaiju_allAnimationClawKaiju_Defeated"), nullptr, LOAD_NoWarn | LOAD_Quiet))
				{
					Body->GetSkeletalMeshComponent()->PlayAnimation(Dying, false);
				}
			}
		}
		if (S.SunkAt >= 0.f)
		{
			// It stops, rolls and goes down over four seconds.
			const float Down = (Elapsed - S.SunkAt) / 4.f;
			FVector Where(1500.f, S.Side * (S.Abeam + 600.f), 0.f);
			Where.Z = SampleSurfaceWorld(Where) - 37.f * 100.f * SpawnlingScale * (SpawnlingSunk + Down * 1.1f);
			A->SetActorLocationAndRotation(Where, FRotator(0.f, (S.Side < 0.f ? 90.f : -90.f) + SpawnlingYaw, (S.bAttacked ? -8.f : 50.f) * FMath::SmoothStep(0.f, 1.f, Down)));
			if (Down >= 1.f) { A->Destroy(); Spawnlings.RemoveAt(i); }
			continue;
		}
		if (!S.bWarned)
		{
			S.bWarned = true;
			if (Crew) { Crew->Report(S.Side < 0.f ? TEXT("Something is swimming at us on the port beam! Port battery, sink it before it boards!") : TEXT("Something is swimming at us on the starboard beam! Starboard battery, sink it before it boards!"), true); }
		}
		// Fast across the open water, then slowly up to the hull over the last 200 m, which is where the batteries can
		// reach it. (At 13 m a second all the way the guns, which do not lead a target, put every ball behind it.)
		S.Abeam = FMath::Max(S.SafeDistance-600.f,S.Abeam-(S.Abeam>20000.f ? 1300.f : 500.f)*DeltaSeconds*Director->TimeScale);
		FVector Where(1500.f, S.Side * (S.Abeam + 600.f), 0.f);
		// Half under: its back and claws break the surface.
		Where.Z = SampleSurfaceWorld(Where) - 37.f * 100.f * SpawnlingScale * SpawnlingSunk;
		// It comes on facing the ship.
		A->SetActorLocationAndRotation(Where, FRotator(0.f, (S.Side < 0.f ? 90.f : -90.f) + SpawnlingYaw, 0.f));
		if (AActor* Mark = S.Mark.Get()) { Mark->SetActorLocation(Where + FVector(0.f, 0.f, 37.f * 50.f * SpawnlingScale)); }
		if (Elapsed>S.NextSprayAt) { EmitWaterContact(Where, .35f); S.NextSprayAt=Elapsed+.5f; }
		if (S.Abeam + 600.f <= S.SafeDistance || Elapsed > Finale)
		{
			if (S.Abeam + 600.f <= S.SafeDistance)
			{
				Director->DamageShip(5.f);
				ADCTroubles* Troubles = ADCTroubles::Get(this);
				if (Troubles) { Troubles->TearRigging(); }
				// A water strike damages the rigging; only burning stones ignite the deck.
				EmitWaterContact(Where,2.f);
				if (Crew) { Crew->Report(TEXT("Brace! Its wake is breaking over the rail!"), true); }
				UE_LOG(LogDC, Log, TEXT("Spawnling wake attack outside hull at %.1f s"), Elapsed);
			}
			if (AActor* Mark = S.Mark.Get()) { Mark->Destroy(); }
			S.bAttacked=true; S.SunkAt=Elapsed; // Withdraw outside the collision envelope instead of walking through the hull.
		}
	}
}

void ADCMaelstrom::UpdatePlume()
{
	const float Since = Elapsed - (Boss ? Boss->FinaleAt : 160.f);
	static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("DCNoPlume"));
	if (bOff || Since < 0.f || Since > 19.f)
	{
		if (AActor* P = Plume.Get()) { P->SetActorHiddenInGame(true); }
		if (ASkeletalMeshActor* Body = Carcass.Get()) { Body->Destroy(); }
		if (Since < 0.f) { bPlumeShaken = false; }
		return;
	}
	if (!bPlumeShaken)
	{
		bPlumeShaken = true;
		Director->Trauma = FMath::Min(1.f, Director->Trauma + 0.7f);   // the deck jumps as it goes
		if (ADCAudio* Sound = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(), ADCAudio::StaticClass()))) { Sound->Boom(1.2f); }
	}
	// The one sight of the creature: in its last seconds it heaves half out of the water at the heart of the crater,
	// more than a kilometre off and inside the plume, a shape some four hundred metres high, and goes down.
	static const bool bNoCarcass = FParse::Param(FCommandLine::Get(), TEXT("DCNoCarcass"));
	static float CarcassScale = 11.f; static bool bCarcassRead = false;   // test hook: -DCCarcassScale= (the mesh is 37 m high at 1)
	if (!bCarcassRead) { bCarcassRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCCarcassScale="), CarcassScale); }
	if (!bNoCarcass && !Carcass.IsValid() && Since < 1.f)
	{
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimation.ClawKaiju_allAnimation"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		FActorSpawnParameters BodyParams;
		BodyParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASkeletalMeshActor* Body = Mesh ? GetWorld()->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, BodyParams) : nullptr;
		if (Body)
		{
			USkeletalMeshComponent* C = Body->GetSkeletalMeshComponent();
			C->SetMobility(EComponentMobility::Movable);
			C->SetSkeletalMesh(Mesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			UMaterialInterface* Hide = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/LeviathanSkin/M_LeviathanSkin.M_LeviathanSkin"), nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!Hide) { Hide=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_Kraken.M_Kraken"),nullptr,LOAD_NoWarn|LOAD_Quiet); }
			if (Hide)
			{
				UMaterialInstanceDynamic* HideMID = UMaterialInstanceDynamic::Create(Hide, Body);
				HideMID->SetVectorParameterValue(TEXT("Skin"), FLinearColor(0.035f, 0.048f, 0.045f));
				HideMID->SetVectorParameterValue(TEXT("Belly"), FLinearColor(0.22f, 0.11f, 0.055f));
				for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot) { C->SetMaterial(Slot, HideMID); }
			}
			if (UAnimSequence* Dying = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimationClawKaiju_Defeated.ClawKaiju_allAnimationClawKaiju_Defeated"), nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				C->PlayAnimation(Dying, false);
				C->SetPlayRate(0.5f);   // sixteen seconds: a thing that size moves slowly
			}
			Body->SetActorScale3D(FVector(CarcassScale));
			Carcass = Body;
		}
	}
	if (ASkeletalMeshActor* Body = Carcass.Get())
	{
		FVector At = Arena->GetComponentLocation();
		At.Z = SampleSurfaceWorld(At);
		const float High = 3700.f * CarcassScale;
		// Up to two thirds out of the water in four seconds, held, then under by nineteen.
		const float Out = FMath::SmoothStep(0.f, 4.f, Since) * (1.f - FMath::SmoothStep(9.f, 19.f, Since));
		At.Z += High * (0.40f * Out - 1.f);
		// Facing the ship.
		const FVector ToShip = FVector(0.f, 0.f, Director->SeaLevelZ) - At;
		Body->SetActorLocationAndRotation(At, FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(ToShip.Y, ToShip.X)), 0.f));
	}
	if (!Plume.IsValid())
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_VdbCloud.M_VdbCloud"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		USparseVolumeTexture* Volume = LoadObject<USparseVolumeTexture>(nullptr, TEXT("/Game/Clouds_Tunnel_Loop/Effects/CT_Clouds_Tunnel_Vdb_01.CT_Clouds_Tunnel_Vdb_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHeterogeneousVolume* A = Base && Volume ? GetWorld()->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(), FTransform::Identity, Params) : nullptr;
		UHeterogeneousVolumeComponent* C = A ? A->FindComponentByClass<UHeterogeneousVolumeComponent>() : nullptr;
		if (!C) { return; }
		C->SetMobility(EComponentMobility::Movable);
		PlumeMID = UMaterialInstanceDynamic::Create(Base, this);
		PlumeMID->SetSparseVolumeTextureParameterValue(TEXT("SparseVolumeTexture"), Volume);
		PlumeMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.93f, 0.96f, 1.f));
		PlumeMID->SetScalarParameterValue(TEXT("Cut"), 0.1f);
		PlumeMID->SetVectorParameterValue(TEXT("Flare"), FLinearColor(0.13f, 0.15f, 0.17f));
		C->SetMaterial(0, PlumeMID);
		C->StartFrame = 0.f; C->EndFrame = 200.f; C->FrameRate = 14.f; C->Frame = 150.f; C->bPlaying = true; C->bLooping = true;
		C->bIssueBlockingRequests = true;
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance"))) { if (V->GetFloat() < 600000.f) { V->Set(600000.f, ECVF_SetByCode); } }
		Plume = A;
		UE_LOG(LogDC, Log, TEXT("Maelstrom: the creature's end, a plume from the heart of the crater"));
	}
	AHeterogeneousVolume* A = Plume.Get();
	UHeterogeneousVolumeComponent* C = A ? A->FindComponentByClass<UHeterogeneousVolumeComponent>() : nullptr;
	if (!C || !PlumeMID) { return; }
	// It goes up in three seconds to a kilometre, stands, and falls back as the arms let go.
	const float Up = FMath::SmoothStep(0.f, 3.f, Since), Down = FMath::SmoothStep(9.f, 19.f, Since);
	FVector Foot = Arena->GetComponentLocation();
	Foot.Z = SampleSurfaceWorld(Foot);
	const float Height = 100000.f * FMath::Lerp(0.15f, 1.f, Up) * FMath::Lerp(1.f, 0.6f, Down);
	const float Scale = Height / 282.f;
	A->SetActorHiddenInGame(false);
	A->SetActorTransform(FTransform(FRotator(0.f, Director->RideTime * 25.f, 90.f), Foot, FVector(Scale * 1.6f, Scale, Scale * 1.6f)));
	const FBoxSphereBounds B = C->Bounds;
	A->AddActorWorldOffset(FVector(Foot.X - B.Origin.X, Foot.Y - B.Origin.Y, Foot.Z - 2000.f - (B.Origin.Z - B.BoxExtent.Z)));
	PlumeMID->SetScalarParameterValue(TEXT("Density"), 0.5f * Up * (1.f - Down));
}

void ADCMaelstrom::UpdateBoulders(float DeltaSeconds)
{
	// In the later part of the fight, between the spouts (which rise at 78, 96, 118 and 134 s).
	static const float ThrowAt[] = { 86.f, 104.f, 110.f, 126.f, 141.f };
	const float Finale = Boss ? Boss->FinaleAt : 160.f;
	const float Flight = 5.f, Apex = 16000.f;
	static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("DCNoBoulders"));
	static float First = -1.f; static bool bRead = false;   // test hook: -DCBoulderAt=<seconds after the breach> throws an extra one then
	if (!bRead) { bRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCBoulderAt="), First); }
	bool bThrow = false;
	while (!bOff && BouldersThrown < UE_ARRAY_COUNT(ThrowAt) && Elapsed >= ThrowAt[BouldersThrown])
	{
		const float At = ThrowAt[BouldersThrown++];
		if (Elapsed <= At + 2.f && At < Finale - 12.f) { bThrow = true; }
	}
	if (First >= 0.f && Elapsed >= First && Elapsed < First + 1.f) { First = -1.f; bThrow = true; }
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	if (bThrow)
	{
		FBoulder B;
		B.ThrownAt = Elapsed;
		// Out of the abyss, off the starboard bow where the crater's heart is.
		B.From = FRotator(0.f, 40.f + FMath::FRandRange(-12.f, 12.f), 0.f).RotateVector(FVector(52000.f, 0.f, 0.f));
		B.From.Z = SampleSurfaceWorld(B.From);
		B.Fall = FVector(FMath::FRandRange(-900.f, 900.f), FMath::FRandRange(-250.f, 250.f), Director->SeaLevelZ + 620.f);
		B.RouteOffsetAtSpawn=RouteOffset; B.TargetAcross=B.Fall.Y;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(B.From, FRotator::ZeroRotator, Params))
		{
			UStaticMeshComponent* C = A->GetStaticMeshComponent();
			C->SetMobility(EComponentMobility::Movable);
			// A real rock from the ruins, eight metres through, wrapped in flame; a plain glowing ball where the ruins pack is absent.
			UStaticMesh* Rock = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AncientRuins/Meshes/Rocks/SM_Rock_01.SM_Rock_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
			UStaticMesh* Ball = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
			UStaticMesh* FlameCone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
			UMaterialInterface* FlameBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Flame.M_Flame"), nullptr, LOAD_NoWarn | LOAD_Quiet);
			UMaterialInstanceDynamic* GlowMID = nullptr;
			if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Glow.M_Glow")))
			{
				GlowMID = UMaterialInstanceDynamic::Create(Glow, A);
				GlowMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(9.f, 2.2f, 0.25f));
			}
			C->SetStaticMesh(Rock ? Rock : Ball);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			if (!Rock && GlowMID) { C->SetMaterial(0, GlowMID); }
			const float Through = Rock ? FMath::Max(Rock->GetBoundingBox().GetSize().GetMax(), 1.f) : 100.f;
			A->SetActorScale3D(FVector(800.f / Through));
			// A tail of fire behind it, so the eye finds it and reads which way it is going.
			for (int32 k = 0; k < 9; ++k)
			{
				UStaticMeshComponent* T = NewObject<UStaticMeshComponent>(A);
				T->SetMobility(EComponentMobility::Movable);
				// Tongues of flame streaming back from it (the deck fires' own material), or glowing balls without it.
				// (Tried 14 Nov with the deck fires' flame cones: in daylight they are a flat pale yellow and hide the stone. Balls of glow again.)
				const bool bFlame = false && FlameCone && FlameBase;
				T->SetStaticMesh(bFlame ? FlameCone : Ball);
				T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				T->SetCastShadow(false);
				if (bFlame) { T->SetMaterial(0, FlameBase); } else if (GlowMID) { T->SetMaterial(0, GlowMID); }
				T->SetUsingAbsoluteLocation(true); T->SetUsingAbsoluteRotation(true); T->SetUsingAbsoluteScale(true);
				T->SetupAttachment(C);
				const float Fat = 9.f * (1.f - k / 11.f);
				T->SetWorldScale3D(bFlame ? FVector(Fat, Fat, Fat * 2.2f) : FVector(6.5f * (1.f - k / 10.f)));
				T->RegisterComponent();
				B.Tail.Add(T);
			}
			UPointLightComponent* Light = NewObject<UPointLightComponent>(A);
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetupAttachment(C);
			Light->SetIntensity(400000.f);
			Light->SetAttenuationRadius(6000.f);
			Light->SetLightColor(FLinearColor(1.f, 0.45f, 0.12f));
			Light->SetCastShadows(false);
			Light->RegisterComponent();
			B.Actor = A;
		}
		Boulders.Add(B);
		if (Crew) { Crew->Report(TEXT("Fire from the deep! It is coming down on us! Turn her, Captain!"), true); }
		UE_LOG(LogDC, Log, TEXT("Boulder thrown at %.1f s from %s"), Elapsed, *B.From.ToCompactString());
	}
	for (int32 i = Boulders.Num() - 1; i >= 0; --i)
	{
		FBoulder& B = Boulders[i];
		// The helm carries the ship out from under it.
		B.Fall.Y = B.TargetAcross + B.RouteOffsetAtSpawn - RouteOffset;
		const float U = (Elapsed - B.ThrownAt) / Flight;
		const FVector Where = FMath::Lerp(B.From, B.Fall, FMath::Clamp(U, 0.f, 1.f)) + FVector(0.f, 0.f, 4.f * Apex * U * (1.f - U));
		if (AActor* A = B.Actor.Get()) { A->SetActorLocation(Where); A->SetActorRotation(FRotator(Elapsed * 200.f, Elapsed * 130.f, 0.f)); }
		for (int32 k = 0; k < B.Tail.Num(); ++k)
		{
			if (UStaticMeshComponent* T = B.Tail[k].Get())
			{
				const float V = FMath::Clamp(U - (k + 1.2f) * 0.012f, 0.f, 1.f);
				const FVector At = FMath::Lerp(B.From, B.Fall, V) + FVector(0.f, 0.f, 4.f * Apex * V * (1.f - V));
				T->SetWorldLocation(At);
				// The cone's point trails: its axis lies back along the way the stone has come.
				const FVector Back = (At - Where).GetSafeNormal();
				if (!Back.IsNearlyZero()) { T->SetWorldRotation(FRotationMatrix::MakeFromZ(Back).Rotator()); }
			}
		}
		if (U < 1.f) { continue; }
		const bool bHit = FMath::Abs(B.Fall.Y) < 1100.f && FMath::Abs(B.Fall.X) < 2600.f;
		if (bHit)
		{
			Director->DamageShip(3.f);
			ADCTroubles* Troubles = ADCTroubles::Get(this);
			if (Troubles && Troubles->DeckPoints.Num() && Troubles->NumFires() < Troubles->MaxFires)
			{
				// The fire starts at the place on deck nearest to where it struck.
				FVector Best = Troubles->DeckPoints[0];
				for (const FVector& P : Troubles->DeckPoints) { if (FVector::DistSquared2D(P, B.Fall) < FVector::DistSquared2D(Best, B.Fall)) { Best = P; } }
				Troubles->StartFire(Best);
			}
			if (Crew) { Crew->Report(TEXT("We are struck! Fire on deck!"), true); }
		}
		else { Director->Score += 100; EmitWaterContact(B.Fall, 2.5f); }
		if (UClass* Burst = LoadClass<AActor>(nullptr, TEXT("/Game/Brigantine/BP/Cannon/BP_EffectExplosionEnd.BP_EffectExplosionEnd_C"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			FVector At = B.Fall;
			if (!bHit) { At.Z = SampleSurfaceWorld(At); }
			if (AActor* Fx = GetWorld()->SpawnActor<AActor>(Burst, At, FRotator::ZeroRotator, Params)) { Fx->SetActorScale3D(FVector(bHit ? 2.f : 4.f)); Fx->SetLifeSpan(4.f); }
		}
		if (ADCAudio* Sound = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(), ADCAudio::StaticClass()))) { Sound->Boom(bHit ? 1.f : 0.6f); }
		UE_LOG(LogDC, Log, TEXT("Boulder %s at %.1f s (%.0f m abeam)"), bHit ? TEXT("struck the ship") : TEXT("fell clear"), Elapsed, B.Fall.Y / 100.f);
		if (AActor* A = B.Actor.Get()) { A->Destroy(); }
		Boulders.RemoveAt(i);
	}
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
		// Release the reveal framing before combat so committed obstacles are visible ahead.
		const float Look = FMath::SmoothStep(4.f, 10.f, Elapsed) * (1.f-FMath::SmoothStep(32.f,40.f,Elapsed));
		const FVector DesiredWorld = Captain->GetActorLocation() + FVector(-20.f * Offset, -180.f * Offset, 64.f + 216.f * Offset);
		Camera->SetWorldLocation(DesiredWorld);
		// Compose towards the crater, accounting for the ship's downhill pitch.
		// A fixed -16 degree eye rotation stared into nearby water on the drop.
		const FVector2D Ahead=DCMaelstromRoute::Point(FMath::Min(180.f,Elapsed+16.f),Radius);
        // Follow the bank with the lower basin visible beyond its inner edge.
        const FVector RouteFocus(Ahead,HeightAt(Ahead,false));
        const FVector Focus=GetArenaTransform().TransformPosition(RouteFocus*.82f+FVector(0.f,0.f,-Depth*.75f)*.18f);
		const FRotator IntoCrater=(Focus-DesiredWorld).Rotation();
		Camera->AddAdditiveOffset(FTransform(FRotator(FMath::Clamp(IntoCrater.Pitch,-12.f,3.f)*Look,
			FMath::Clamp(IntoCrater.Yaw,-65.f,0.f)*Look,0.f)),0.f);
	}
	// Over the lip: the view opens out as she starts down and closes again as she settles into the spiral.
	const float Over = FMath::SmoothStep(11.f, 16.f, Elapsed) * (1.f - FMath::SmoothStep(20.f, 32.f, Elapsed));
	Camera->SetFieldOfView(90.f + 9.f * Over);
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
			// Volumetric distance is restored by ADCMaelstromAtmosphere.
		}
	}
	bAtmosphereAdjusted = false;
}

void ADCMaelstrom::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearInletMist();
	ResetExitTransition();
	ClearCliffPreview();
	ClearProceduralFalls();
	WaterBridge.Reset(WaterMID);
	RestoreView();
	RestoreAtmosphere();
	if (bOceanHidden) { SetOceanVisible(true); }
	for (AActor* Landmark : Landmarks) { if (IsValid(Landmark)) { Landmark->Destroy(); } }
	if (IsValid(Boss)) { Boss->Destroy(); }
	if (IsValid(Atmosphere)) { Atmosphere->Destroy(); }
	Super::EndPlay(Reason);
}
