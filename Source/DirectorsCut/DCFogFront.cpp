#include "DCFogFront.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "HAL/IConsoleManager.h"
#include "SparseVolumeTexture/SparseVolumeTexture.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCFogFront::ADCFogFront()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ADCFogFront::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Base || !Sphere)
	{
		UE_LOG(LogDC, Warning, TEXT("FogFront: M_Cloud missing; no cloud front"));
		return;
	}
	CloudMID = UMaterialInstanceDynamic::Create(Base, this);
	for (int32 s = 0; s < 3; ++s) { CloudShades.Add(UMaterialInstanceDynamic::Create(Base, this)); }

	// A wall of overlapping billows: big and close-packed at the foot, towering and ragged above,
	// bulging toward the ship in the middle like a shelf cloud.
	FRandomStream Rng(4242);
	const float Width = 900000.f;   // 9 km across
	const float Height = 120000.f;  // 1.2 km tall
	const float Depth = 90000.f;
	const int32 Count = 150;
	for (int32 i = 0; i < Count; ++i)
	{
		const float V = FMath::Pow(Rng.FRand(), 1.6f);            // more billows low down
		const float Across = Rng.FRandRange(-0.5f, 0.5f);
		FBillow B;
		B.Size = FMath::Lerp(34000.f, 16000.f, V) * Rng.FRandRange(0.7f, 1.4f);
		// The wall leans forward at the top, and its centre bulges toward the ship.
		const float Bulge = (1.f - FMath::Square(Across * 2.f)) * 40000.f;
		// Keep the wall clear of the mouth where the ship enters.
		if (FMath::Abs(Across * Width) < 34000.f && V * Height < 50000.f) { continue; }
		B.Offset = FVector(Rng.FRandRange(0.f, Depth) - Bulge - V * 30000.f, Across * Width, V * Height + B.Size * 0.15f);
		B.Seed = Rng.FRandRange(0.f, 100.f);

		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Sphere);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetMaterial(0, CloudShades[i % CloudShades.Num()]);
		C->SetupAttachment(RootComponent);
		C->SetRelativeLocation(B.Offset);
		// The engine sphere is 1 m across; billows are flattened a little.
		C->SetRelativeScale3D(FVector(B.Size / 50.f, B.Size / 50.f * 1.2f, B.Size / 50.f * 0.8f));
		C->RegisterComponent();
		B.Component = C;
		Billows.Add(B);
	}

	// The mouth: where the ship goes in, the front is rolled into a tunnel. Rings of billows spiral
	// back into the wall, each a little smaller and turned a little further, around a core of
	// lightning-lit glow. Seen from the sea it is a great curling eye in the storm.
	const FVector MouthCentre(-95000.f, 0.f, 9000.f);
	const int32 RingCount = 6;
	for (int32 r = 0; r < RingCount; ++r)
	{
		const float RingT = static_cast<float>(r) / (RingCount - 1);
		const float Radius = FMath::Lerp(30000.f, 9000.f, RingT);
		const float Back = RingT * 110000.f;
		const int32 PerRing = 20 - r * 2;
		for (int32 i = 0; i < PerRing; ++i)
		{
			const float A = 2.f * UE_PI * i / PerRing + r * 0.5f;
			// Leave the bottom of each ring open: that is the sea the ship sails in on.
			if (FMath::Sin(A) < -0.55f) { continue; }
			FBillow B;
			B.Size = FMath::Lerp(15000.f, 7000.f, RingT) * Rng.FRandRange(0.8f, 1.25f);
			B.Offset = MouthCentre + FVector(Back + Rng.FRandRange(-5000.f, 5000.f), FMath::Cos(A) * Radius, FMath::Sin(A) * Radius + Radius * 0.35f);
			B.Seed = Rng.FRandRange(0.f, 100.f);
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Sphere);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetMaterial(0, CloudShades[(i + r) % CloudShades.Num()]);
			C->SetupAttachment(RootComponent);
			C->SetRelativeLocation(B.Offset);
			C->SetRelativeScale3D(FVector(B.Size / 50.f));
			C->RegisterComponent();
			B.Component = C;
			Billows.Add(B);
		}
	}
	// The glow at the end of the tunnel.
	if (UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud")))
	{
		CoreMID = UMaterialInstanceDynamic::Create(GlowBase, this);
		UStaticMeshComponent* Core = NewObject<UStaticMeshComponent>(this);
		Core->SetMobility(EComponentMobility::Movable);
		Core->SetStaticMesh(Sphere);
		Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Core->SetCastShadow(false);
		Core->SetMaterial(0, CoreMID);
		Core->SetupAttachment(RootComponent);
		Core->SetRelativeLocation(MouthCentre + FVector(125000.f, 0.f, 4000.f));
		Core->SetRelativeScale3D(FVector(520.f));
		Core->RegisterComponent();
	}
}

void ADCFogFront::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director || !CloudMID) { return; }

	const float T = Director->RideTime;
	// It comes on steadily, reaches the bow at ArrivalTime, then sweeps over and is gone astern.
	const float Speed = StartDistance / ArrivalTime;
	const float X = (ArrivalTime - T) * Speed;
	// The front itself is in the volumetric cloud layer; tell the director where its face is.
	// It is 15 km off when the voyage starts, a wall on the horizon, and closes at about 250 m a second.
	Director->StormFrontX = 1500000.f * (1.f - T / ArrivalTime);
	Director->StormFrontStrength = FMath::Clamp((ArrivalTime + 28.f - T) / 12.f, 0.f, 1.f);
	Director->StormMouthRadius = 35000.f;
	TickTunnel(T);
	// With the wall of cloud volumes the old skirt of fog domes under the front only lays a pale belt across its foot.
	if (Director->bUDS && (Wall.Num() == 0 || FParse::Param(FCommandLine::Get(), TEXT("DCFrontSkirt"))))
	{
		if (Banks.Num() == 0) { BuildBanks(); }
		// World space: the actor itself only moves when the old billows are shown.
		const float Gone = FMath::Clamp((T - ArrivalTime - 30.f) / 6.f, 0.f, 1.f);
		for (FBank& B : Banks)
		{
			if (ULocalFogVolumeComponent* C = B.Component.Get())
			{
				C->SetWorldLocation(FVector(Director->StormFrontX, 0.f, Director->SeaLevelZ) + B.Offset);
				// The skirt of fog under the front comes with the front itself.
				// The skirt of fog under the front thickens with the front itself; it does not switch on.
				const float In = FMath::SmoothStep(0.3f, 1.f, Director->StormGrowth) * (1.f - Gone);
				C->SetVisibility(In > 0.01f);
				if (!FMath::IsNearlyEqual(In, B.Shown, 0.004f))
				{
					B.Shown = In;
					C->RadialFogExtinction = B.Radial * In;
					C->HeightFogExtinction = B.High * In;
					C->MarkRenderStateDirty();
				}
			}
		}
	}
	const bool bVisible = bBillows && T < ArrivalTime + 35.f;
	RootComponent->SetVisibility(bVisible, true);
	if (!bVisible) { return; }
	SetActorLocation(FVector(X, 0.f, Director->SeaLevelZ - 2000.f));

	// The billows breathe.
	for (FBillow& B : Billows)
	{
		if (UStaticMeshComponent* C = B.Component.Get())
		{
			const float Pulse = 1.f + 0.06f * FMath::Sin(T * 0.4f + B.Seed);
			C->SetRelativeScale3D(FVector(B.Size / 50.f, B.Size / 50.f * 1.2f, B.Size / 50.f * 0.8f) * Pulse);
			C->SetRelativeLocation(B.Offset + FVector(0.f, 0.f, 1500.f * FMath::Sin(T * 0.25f + B.Seed * 2.f)));
		}
	}

	// Lightning inside the wall: the whole front flickers from within, more often as it closes.
	Flash = FMath::Max(0.f, Flash - DeltaSeconds * 5.f);
	NextFlashIn -= DeltaSeconds;
	if (NextFlashIn <= 0.f)
	{
		Flash = FMath::FRandRange(0.5f, 1.f);
		const float Closeness = FMath::Clamp(T / ArrivalTime, 0.f, 1.f);
		NextFlashIn = FMath::FRandRange(1.2f, 5.f) * FMath::Lerp(1.6f, 0.5f, Closeness);
	}
	// Fade the front out as the real fog takes over, so there is no hard edge when it passes.
	const float Opacity = FMath::Clamp(1.f - (T - ArrivalTime + 8.f) / 30.f, 0.f, 1.f) * 0.92f;
	const float Flicker = Flash * (0.6f + 0.4f * FMath::Sin(T * 60.f));
	if (CoreMID)
	{
		// Always a cold light down the tunnel; lightning makes it blaze.
		const FLinearColor CoreColor = FLinearColor(0.35f, 0.45f, 0.75f) * (0.55f + 2.2f * Flicker);
		CoreMID->SetVectorParameterValue(TEXT("Top"), CoreColor);
		CoreMID->SetVectorParameterValue(TEXT("Bottom"), CoreColor);
		CoreMID->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, Opacity));
	}
	// Warm sunset light on the cloud tops while we are still in the open; the foot is storm-dark.
	const FDCEnvState& Env = Director->GetEnv();
	const float Sun = FMath::Clamp(Env.SunIntensity / 7.f, 0.05f, 1.f);
	// Three shades of billow, so the wall has modelling instead of reading as one flat sheet.
	static const float Shades[3] = { 0.45f, 1.f, 1.7f };
	for (int32 i = 0; i < CloudShades.Num(); ++i)
	{
		CloudShades[i]->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.55f * Flicker, 0.7f * Flicker, 1.f * Flicker, Opacity));
		CloudShades[i]->SetVectorParameterValue(TEXT("Top"), (FLinearColor(0.5f, 0.3f, 0.2f) * Sun + FLinearColor(0.035f, 0.05f, 0.065f)) * Shades[i % 3]);
		CloudShades[i]->SetVectorParameterValue(TEXT("Bottom"), FLinearColor(0.008f, 0.012f, 0.02f) * Shades[i % 3]);
	}
}

void ADCFogFront::BuildTunnel()
{
	// Clouds Tunnel Vdb Effects Pack Loop (bought, not in the repository): absent, the front simply has no mouth.
	int32 Index = 1;
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnel="), Index);
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_VdbCloud.M_VdbCloud"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	const FString VolumeName = FString::Printf(TEXT("CT_Clouds_Tunnel_Vdb_0%d"), Index);
	USparseVolumeTexture* Volume = Index > 0 ? LoadObject<USparseVolumeTexture>(nullptr, *FString::Printf(TEXT("/Game/Clouds_Tunnel_Loop/Effects/%s.%s"), *VolumeName, *VolumeName), nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
	if (!Volume) { Base = nullptr; }
	if (!Base) { UE_LOG(LogDC, Log, TEXT("FogFront: no cloud tunnel (pack not installed or switched off)")); return; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Tunnel = GetWorld()->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(), FTransform::Identity, Params);
	UHeterogeneousVolumeComponent* H = Tunnel ? Tunnel->FindComponentByClass<UHeterogeneousVolumeComponent>() : nullptr;
	if (!H) { return; }
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelScale="), TunnelScale);
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelSink="), TunnelSink);
	float Rate = 24.f, Temp = 0.f;   // no light of its own: only lightning lights it from within
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelDensity="), TunnelDensity);
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelRate="), Rate);
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelTemp="), Temp);
	H->SetMobility(EComponentMobility::Movable);
	TunnelMID = UMaterialInstanceDynamic::Create(Base, this);
	TunnelMID->SetSparseVolumeTextureParameterValue(TEXT("SparseVolumeTexture"), Volume);
	// The cloud stands on the sea: nothing below a floor just above the water, thickening over the first metres.
	float Floor = TunnelSink / (TunnelScale * 0.42f) + 2.f, Soft = 8.f;
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelFloor="), Floor);
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelSoft="), Soft);
	// Storm cloud is dark and cold: a low blue-grey albedo, so the low sun cannot turn it to sand.
	FLinearColor StormTint(0.34f, 0.42f, 0.58f);
	{
		FString TintText;
		if (FParse::Value(FCommandLine::Get(), TEXT("DCFrontTint="), TintText, false))
		{
			TArray<FString> Parts; TintText.ParseIntoArray(Parts, TEXT(","));
			if (Parts.Num() == 3) { StormTint = FLinearColor(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2])); }
		}
	}
	TunnelMID->SetVectorParameterValue(TEXT("Tint"), StormTint);
	float FrontCut = 0.f;   // test hook: -DCFrontCut= takes the thin haze off the front's volumes
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontCut="), FrontCut);
	TunnelMID->SetScalarParameterValue(TEXT("Cut"), FrontCut);
	TunnelMID->SetVectorParameterValue(TEXT("Floor"), FLinearColor(Floor, Soft, 0.f, 0.f));
	TunnelMID->SetVectorParameterValue(TEXT("Glow"), FLinearColor(0.45f, 0.6f, 1.f) * Temp);
	H->SetMaterial(0, TunnelMID);
	H->StartFrame = 0.f; H->EndFrame = 200.f; H->FrameRate = Rate; H->bPlaying = true; H->bLooping = true;
	H->bIssueBlockingRequests = true;   // without it the far wall streams in at a coarse level and goes soft (tried 14 Nov)
	// The engine stops marching these volumes a few hundred metres from the eye; the tunnel has to be seen coming.
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance"))) { V->Set(1000000.f, ECVF_SetByCode); }   // 10 km: the wall is seen from about 20 s
	UE_LOG(LogDC, Log, TEXT("FogFront: cloud tunnel %d, scale %.0f"), Index, TunnelScale);
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontTunnelLight="), TunnelLight);
	for (int32 i = 0; i < 2 && TunnelLight > 0.f; ++i)
	{
		if (APointLight* L = GetWorld()->SpawnActor<APointLight>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
		{
			UPointLightComponent* P = Cast<UPointLightComponent>(L->GetLightComponent());
			P->SetMobility(EComponentMobility::Movable);
			P->bUseInverseSquaredFalloff = false;
			P->SetLightFalloffExponent(2.f);
			P->SetAttenuationRadius(i == 0 ? 52000.f : 30000.f);
			P->SetLightColor(i == 0 ? FLinearColor(0.8f, 0.88f, 1.f) : FLinearColor(0.55f, 0.65f, 0.85f));
			P->SetCastShadows(false);
			P->SetIntensity(0.f);
			TunnelLights.Add(L);
		}
	}

	// The wall. Each piece is the tunnel lying across the course, nearly five times the size: a roll of cloud
	// 4 km long, 1.1 km through and 900 m high whose lower quarter is under the sea. One either side of
	// the mouth, and one over it whose foot is cut away above the tunnel's crown.
	if (FParse::Param(FCommandLine::Get(), TEXT("DCNoFrontWall"))) { return; }
	float WallScale = 1400.f, WallDensity = 1.f, WallGap = 11000.f;
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontWallScale="), WallScale);
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontWallDensity="), WallDensity);
	FParse::Value(FCommandLine::Get(), TEXT("DCFrontWallGap="), WallGap);
	const float Unit = WallScale * 0.42f;             // cm of world to one voxel
	const float HalfLong = 0.5f * 672.f * Unit, HalfThrough = 0.5f * 192.f * Unit, WallSink = 40.f * Unit;
	// Unequal sizes, a few degrees of turn and one set back, so the top of the wall is not one level line.
	struct FPlace { float Side, Lift, Floor, Frame, Size, Yaw, Back, Soft; };
	const FPlace Places[] = {
		{ -1.f, 0.f, 42.f, 0.f, 1.f, 7.f, 0.f, 1.5f },
		{ 1.f, 0.f, 42.f, 70.f, 1.25f, -5.f, 9000.f, 1.5f },
		{ 0.f, 0.f, 40.f + 17000.f / Unit, 135.f, 1.1f, 0.f, 4000.f, 30.f },
	};
	for (const FPlace& P : Places)
	{
		AHeterogeneousVolume* A = GetWorld()->SpawnActor<AHeterogeneousVolume>(AHeterogeneousVolume::StaticClass(), FTransform::Identity, Params);
		UHeterogeneousVolumeComponent* C = A ? A->FindComponentByClass<UHeterogeneousVolumeComponent>() : nullptr;
		if (!C) { continue; }
		C->SetMobility(EComponentMobility::Movable);
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
		M->SetSparseVolumeTextureParameterValue(TEXT("SparseVolumeTexture"), Volume);
		M->SetVectorParameterValue(TEXT("Floor"), FLinearColor(P.Floor, P.Soft, 0.f, 0.f));
		M->SetVectorParameterValue(TEXT("Tint"), StormTint);
		M->SetScalarParameterValue(TEXT("Cut"), FrontCut);
		M->SetVectorParameterValue(TEXT("Glow"), FLinearColor(0.45f, 0.6f, 1.f) * Temp);
		C->SetMaterial(0, M);
		C->StartFrame = 0.f; C->EndFrame = 200.f; C->FrameRate = Rate * 0.75f; C->Frame = P.Frame; C->bPlaying = true; C->bLooping = true;
		C->bIssueBlockingRequests = true;
		FWallPiece W;
		W.Actor = A; W.MID = M; W.Scale = WallScale * P.Size; W.Density = WallDensity; W.Yaw = P.Yaw;
		W.Offset = FVector(HalfThrough * P.Size - 6000.f + P.Back, P.Side * (WallGap + HalfLong * P.Size), P.Lift - WallSink * P.Size);
		Wall.Add(W);
		WallMIDs.Add(M);
		Director->bCloudWall = true;
	}
}

void ADCFogFront::TickTunnel(float T)
{
	if (!Tunnel && TunnelShown < 0.f) { TunnelShown = 0.f; BuildTunnel(); }
	if (!Tunnel) { return; }
	// The volume is 672 voxels of 0.42 cm long, its foot at the actor.
	const float Half = 0.5f * 672.f * 0.42f * TunnelScale;
	// It rides on the face of the front until its mouth reaches the bow, then draws aft over the ship
	// slowly enough that she is inside it until the city opens (phase Fog, 58 to 76 s).
	// Its mouth stands in the face of the front, so the opening is seen in the wall as the wall comes on.
	const float Through = (2.f * Half + 6000.f) / 16.f;
	const float Reach = ArrivalTime * (1.f - 6000.f / 1500000.f);   // when the face is at the bow
	const float Mouth = FMath::Max(Director->StormFrontX, 6000.f - (T - Reach) * Through);
	Tunnel->SetActorTransform(FTransform(FRotator(0.f, -90.f, 0.f), FVector(Mouth + Half, 0.f, Director->SeaLevelZ - TunnelSink), FVector(TunnelScale)));
	TunnelMID->SetVectorParameterValue(TEXT("Flare"), Director->FogFlare * 0.02f);
	// Daylight down the tunnel from its far end, and a dimmer light under its crown a third of the way in. Both come up
	// as the mouth nears and flare with the lightning.
	{
		const float Near = (1.f - FMath::SmoothStep(8000.f, 60000.f, Mouth)) * FMath::Clamp((ArrivalTime + 18.f - T) / 4.f, 0.f, 1.f);
		const float Struck = 1.f + 2.5f * Director->LightningNow;
		for (int32 i = 0; i < TunnelLights.Num(); ++i)
		{
			if (APointLight* L = TunnelLights[i])
			{
				L->SetActorLocation(FVector(Mouth + (i == 0 ? 1.9f : 0.7f) * Half, 0.f, Director->SeaLevelZ + (i == 0 ? 5000.f : 9000.f)));
				L->GetLightComponent()->SetIntensity((i == 0 ? 26.f : 9.f) * TunnelLight * Near * Struck);
			}
		}
	}
	// It thickens with the storm and thins away once it has passed.
	const float Shown = FMath::SmoothStep(0.25f, 0.9f, Director->StormGrowth) * FMath::Clamp((ArrivalTime + 20.f - T) / 4.f, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(Shown, TunnelShown, 0.004f))
	{
		TunnelShown = Shown;
		TunnelMID->SetScalarParameterValue(TEXT("Density"), Shown * TunnelDensity);
		Tunnel->SetActorHiddenInGame(Shown <= 0.01f);
		for (FWallPiece& W : Wall)
		{
			if (W.Actor.IsValid()) { W.Actor->SetActorHiddenInGame(Shown <= 0.01f); }
			if (W.MID.IsValid()) { W.MID->SetScalarParameterValue(TEXT("Density"), Shown * W.Density); }
		}
	}
	for (FWallPiece& W : Wall)
	{
		if (!W.Actor.IsValid()) { continue; }
		W.Actor->SetActorTransform(FTransform(FRotator(0.f, W.Yaw, 0.f), FVector(Mouth, 0.f, Director->SeaLevelZ) + W.Offset, FVector(W.Scale)));
		// Less than in the tunnel: at full strength the whole wall turned pale blue at once, from end to end.
		if (W.MID.IsValid()) { W.MID->SetVectorParameterValue(TEXT("Flare"), Director->FogFlare * 0.007f); }
	}
}

void ADCFogFront::BuildBanks()
{
	// Local fog volumes: analytic, so a couple of dozen of them kilometres across cost next to nothing.
	// Two ranks of overlapping domes, the nearer lower and ragged, the further tall, 24 km from end to end.
	float Density = 1.2f, Tall = 1.f;
	FParse::Value(FCommandLine::Get(), TEXT("DCWallDensity="), Density);
	FParse::Value(FCommandLine::Get(), TEXT("DCWallHeight="), Tall);
	// The front's banks and the city's drifts can all be in view at once.
	if (IConsoleVariable* Max = IConsoleManager::Get().FindConsoleVariable(TEXT("r.LocalFogVolume.TileMaxInstanceCount"))) { Max->Set(64); }
	FRandomStream Rng(977);
	for (int32 Rank = 0; Rank < 2; ++Rank)
	{
		const int32 Count = Rank == 0 ? 23 : 15;
		for (int32 i = 0; i < Count; ++i)
		{
			const float Across = (static_cast<float>(i) / (Count - 1) - 0.5f) * 4200000.f + Rng.FRandRange(-40000.f, 40000.f);
			// A volume is always a sphere; its fog thins with height, so each is a wide low bank whose
			// top fades out a hundred metres or so up (the nearer rank) or a few hundred (the further).
			const float Radius = (Rank == 0 ? 260000.f : 380000.f) * Rng.FRandRange(0.8f, 1.3f);
			const float Bow = FMath::Square(Across / 1200000.f) * 250000.f;
			FBank B;
			B.Offset = FVector(Radius * 0.8f + (Rank == 0 ? 0.f : 120000.f) + Bow + Rng.FRandRange(0.f, 40000.f), Across, 0.f);
			ULocalFogVolumeComponent* C = NewObject<ULocalFogVolumeComponent>(this);
			C->SetMobility(EComponentMobility::Movable);
			// Not attached: the root is hidden with the old billows, and that would hide these too.
			// The unit volume is a sphere of 5 m radius. Optical depth scales with the radius in
			// metres, so the densities are small numbers.
			C->SetWorldScale3D(FVector(Radius / 500.f));
			C->RadialFogExtinction = 0.02f * Density;
			C->HeightFogExtinction = 0.028f * Density;
			C->HeightFogFalloff = (Rank == 0 ? 1500.f : 700.f) * Rng.FRandRange(0.7f, 1.4f) / Tall;
			C->FogPhaseG = 0.5f;
			C->FogAlbedo = Rank == 0 ? FLinearColor(0.62f, 0.7f, 0.82f) : FLinearColor(0.3f, 0.36f, 0.46f);
			B.Radial = C->RadialFogExtinction; B.High = C->HeightFogExtinction;
			C->RegisterComponent();
			B.Component = C;
			Banks.Add(B);
		}
	}
}
