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

	Spray = BuildStreaks(TEXT("Spray"), 2200);
	SprayMID = UMaterialInstanceDynamic::Create(Base, this);
	Spray->SetMaterial(0, SprayMID);

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

	TickBowSpray(DeltaSeconds);

	// Splashes on the deck, once the ship is there to be rained on.
	if (!bSplashesBuilt && Director->RideTime > 1.f) { BuildSplashes(); }
	if (Splashes && SplashMID)
	{
		float Amount = FMath::Clamp(Env.Rain * 1.6f, 0.f, 1.f);
		FParse::Value(FCommandLine::Get(), TEXT("DCSplash="), Amount);
		Splashes->SetVisibility(Amount > 0.02f);
		// Lit by whatever lights the deck: daylight, or the lanterns when it is dark.
		const float Day = FMath::Clamp(Env.SunIntensity / 6.f, 0.f, 1.f);
		const FLinearColor Lit = FMath::Lerp(FLinearColor(0.5f, 0.36f, 0.2f), FLinearColor(0.9f, 0.95f, 1.f), Day) * FMath::Lerp(0.3f, 0.8f, Day) * (1.f + 3.f * FlashBoost);
		SplashMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(Lit.R, Lit.G, Lit.B, Amount));
	}

	// Rain; inside the water tunnel the same streaks read as spray.
	const float RainAmount = FMath::Clamp(Env.Rain + Env.Tube * 0.6f * (1.f - FMath::Min(Env.SeaGlow.R, 1.f)), 0.f, 1.f);
	Rain->SetVisibility(RainAmount > 0.01f && !Director->bUDS);   // the sky pack brings its own rain
	const float Wind = FMath::Clamp(Env.WaveHeight / 300.f, 0.2f, 1.3f);
	RainMID->SetVectorParameterValue(TEXT("Motion"), FLinearColor(2400.f + 900.f * Wind, 110.f + 60.f * Wind, 1.5f, Wind));
	// Unlit, so brightness follows the scene by hand: dim normally, bright in a lightning flash.
	const float Light = 0.12f + 0.15f * Env.SkyIntensity + FlashBoost * 2.5f;
	RainMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.55f * Light, 0.65f * Light, 0.8f * Light, 0.7f * RainAmount));

	// Spray: white flecks torn off the wave tops and blown down the deck, wherever the water is wild.
	const float SprayAmount = FMath::Clamp(Env.Whirl * 0.9f + Env.Tube * 0.6f * (1.f - FMath::Min(Env.SeaGlow.R, 1.f)), 0.f, 1.f);
	Spray->SetVisibility(SprayAmount > 0.01f);
	SprayMID->SetVectorParameterValue(TEXT("Motion"), FLinearColor(140.f, 22.f, 15.f, 3.4f));
	const float SprayLight = 0.25f + 0.11f * Env.SunIntensity + FlashBoost * 1.5f;
	SprayMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.9f * SprayLight, 0.95f * SprayLight, SprayLight, 0.5f * SprayAmount));

	// Embers rise out of the burning sea.
	const float EmberAmount = FMath::Clamp(Env.SeaGlow.R, 0.f, 1.f);
	Embers->SetVisibility(EmberAmount > 0.01f);
	EmberMID->SetVectorParameterValue(TEXT("Motion"), FLinearColor(-260.f, 9.f, 5.f, 0.5f));
	EmberMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(30.f, 7.f, 0.6f, 0.9f * EmberAmount));
}

void ADCWeatherFX::TickBowSpray(float DeltaSeconds)
{
	static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("DCNoBowSpray"));
	if (bOff || !Director->HasPluginSea() || DeltaSeconds <= 0.f) { return; }
	const FDCEnvState& Env = Director->GetEnv();
	const float SeaZ = Director->SeaLevelZ;
	// How fast the stem is going down, from where the deck's frame puts it.
	const float BowHeight = Director->DeckToWorld(FVector(2500.f, 0.f, SeaZ)).Z - SeaZ;
	const float Falling = (LastBowHeight - BowHeight) / DeltaSeconds;
	LastBowHeight = BowHeight;
	BowSprayCooldown -= DeltaSeconds;
	const bool bSeaway = Env.Tube < 0.3f && Env.Whirl < 0.3f && Env.WaveHeight > 60.f;
	static float Threshold = 70.f; static bool bRead = false;   // test hook: -DCBowSpray= (cm/s of plunge that throws spray; lower = more often)
	if (!bRead) { bRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCBowSpray="), Threshold); }
	if (bSeaway && BowSprayCooldown <= 0.f && Falling > Threshold && BowHeight < 10.f)
	{
		for (FBowSpray& S : BowSprays)
		{
			if (S.Age >= 0.f) { continue; }
			if (!S.Mesh)
			{
				UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_FogVolume.M_FogVolume"));
				UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
				if (!Base || !Sphere) { return; }
				S.Mesh = NewObject<UStaticMeshComponent>(this);
				S.Mesh->SetMobility(EComponentMobility::Movable);
				S.Mesh->SetStaticMesh(Sphere);
				S.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				S.Mesh->SetCastShadow(false);
				S.Mesh->SetUsingAbsoluteLocation(true); S.Mesh->SetUsingAbsoluteRotation(true); S.Mesh->SetUsingAbsoluteScale(true);
				S.Mesh->SetupAttachment(RootComponent);
				S.Mesh->RegisterComponent();
				S.MID = UMaterialInstanceDynamic::Create(Base, this);
				S.Mesh->SetMaterial(0, S.MID);
				BowSprayRefs.Add(S.Mesh); BowSprayRefs.Add(S.MID);
				S.MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.95f, 0.97f, 1.f));
				S.MID->SetScalarParameterValue(TEXT("Size"), 700.f);
				S.MID->SetScalarParameterValue(TEXT("Erode"), 0.75f);
				S.MID->SetVectorParameterValue(TEXT("Drift"), FLinearColor(-500.f, 0.f, 250.f, 0.f));
				S.MID->SetVectorParameterValue(TEXT("Floor"), FLinearColor(SeaZ - 100.f, 0.f, 0.f, 150.f));
			}
			S.Age = 0.f;
			S.Strength = FMath::Clamp(Falling / 180.f, 0.45f, 1.3f) * FMath::Clamp(Env.WaveHeight / 150.f, 0.5f, 1.3f);
			S.Side = FMath::RandBool() ? 1.f : -1.f;
			S.MID->SetScalarParameterValue(TEXT("Seed"), FMath::FRandRange(0.f, 90.f));
			BowSprayCooldown = FMath::FRandRange(2.2f, 4.5f);
			if (FParse::Param(FCommandLine::Get(), TEXT("DCSprayLog"))) { UE_LOG(LogDC, Log, TEXT("Bow spray: stem falling %.0f cm/s, strength %.2f, at ride time %.2f"), Falling, S.Strength, Director->RideTime); }
			break;
		}
	}
	for (FBowSpray& S : BowSprays)
	{
		if (S.Age < 0.f || !S.Mesh) { continue; }
		S.Age += DeltaSeconds;
		const float Life = 2.6f;
		const float A = S.Age / Life;
		if (A >= 1.f) { S.Age = -1.f; S.Mesh->SetVisibility(false); continue; }
		// Thrown up fast at the stem, opening out as it is carried aft over the forecastle, thinning as it goes.
		const float Up = 1.f - FMath::Square(1.f - FMath::Min(A * 2.2f, 1.f));
		const FVector At = Director->DeckToWorld(FVector(2550.f - A * 1500.f, S.Side * (120.f + A * 380.f), SeaZ + 200.f + Up * 650.f - A * A * 250.f));
		const FVector Radii = FVector(380.f + A * 900.f, 420.f + A * 700.f, 420.f + Up * 500.f) * (0.8f + 0.3f * S.Strength);
		S.Mesh->SetWorldLocation(At);
		S.Mesh->SetWorldScale3D(Radii / 50.f);
		S.Mesh->SetVisibility(true);
		S.MID->SetVectorParameterValue(TEXT("Radii"), FLinearColor(Radii.X, Radii.Y, Radii.Z, 0.f));
		const float Thick = FMath::Min(A * 9.f, 1.f) * FMath::Square(1.f - A);
		S.MID->SetScalarParameterValue(TEXT("Density"), 0.3f * S.Strength * Thick);
		S.MID->SetVectorParameterValue(TEXT("Glow"), (Director->FogGlow + FLinearColor(0.5f, 0.55f, 0.62f) * (0.12f + 0.25f * FMath::Clamp(Env.SunIntensity / 6.f, 0.f, 1.f))) * 1.6f);
	}
}

void ADCWeatherFX::BuildSplashes()
{
	bSplashesBuilt = true;
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Splash.M_Splash"));
	if (!Base) { UE_LOG(LogDC, Warning, TEXT("WeatherFX: M_Splash missing; no rain splashes")); return; }

	// Find the decks: drop probes over the hull and keep the ones that land on something level.
	TMap<FString, int32> SplashSurfaces;
	TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals; TArray<FVector2D> UVs; TArray<FColor> Colors;
	FCollisionQueryParams Params(TEXT("DCSplash"), true);
	FRandomStream Rng(31);
	const float Half = 4.5f;
	for (int32 i = 0; i < 16000 && Verts.Num() < 7000 * 12; ++i)
	{
		const float X = Rng.FRandRange(-2000.f, 2650.f);
		const float Y = Rng.FRandRange(-560.f, 560.f);
		// Ropes, spars and rails are in the way: keep going down through them to the first level surface.
		FHitResult Hit;
		bool bFound = false;
		float From = 980.f;
		for (int32 Step = 0; Step < 30 && !bFound; ++Step)
		{
			if (!GetWorld()->LineTraceSingleByObjectType(Hit, FVector(X, Y, From), FVector(X, Y, 200.f), FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), Params)) { break; }
			// SM_Root is the ship blueprint's unseen base plate, not a deck.
			// Only the hull mesh itself counts: the blueprint also carries unseen plates and collision shapes (one stands above the stern).
			const UPrimitiveComponent* On = Hit.GetComponent();
			bFound = !Hit.bStartPenetrating && Hit.ImpactNormal.Z > 0.9f && On && On->GetName() == TEXT("HullShip") && On->IsVisible() && !On->bHiddenInGame;
			if (On && !Hit.bStartPenetrating && Hit.ImpactNormal.Z > 0.9f) { SplashSurfaces.FindOrAdd(On->GetName())++; }
			From = (Hit.bStartPenetrating ? From : Hit.ImpactPoint.Z) - (Hit.bStartPenetrating ? 30.f : 4.f);
		}
		// Open decks only: rain does not reach the gun deck below, which the probes get to through the gratings.
		if (!bFound || Hit.ImpactPoint.Z < Director->SeaLevelZ + 240.f) { continue; }
		const FVector C = Hit.ImpactPoint + FVector(0.f, 0.f, 1.2f);
		// Alpha 0: the quad lying on the deck (the ring). Alpha 255: two crossed upright cards (the crown of thrown water).
		FColor Rand(Rng.RandRange(0, 255), Rng.RandRange(0, 255), Rng.RandRange(0, 255), 0);
		int32 B = Verts.Num();
		for (int32 c = 0; c < 4; ++c)
		{
			Verts.Add(C + FVector((c & 1) ? Half : -Half, (c >> 1) ? Half : -Half, 0.f));
			UVs.Add(FVector2D(c & 1, c >> 1));
			Colors.Add(Rand);
		}
		Tris.Append({ B, B + 2, B + 1, B + 1, B + 2, B + 3 });
		Rand.A = 255;
		const float Turn = Rng.FRandRange(0.f, UE_PI);
		const float Wide = Rng.FRandRange(3.f, 4.5f), High = Rng.FRandRange(4.f, 7.f);
		for (int32 Card = 0; Card < 2; ++Card)
		{
			const FVector Along(FMath::Cos(Turn + Card * UE_HALF_PI), FMath::Sin(Turn + Card * UE_HALF_PI), 0.f);
			B = Verts.Num();
			for (int32 c = 0; c < 4; ++c)
			{
				Verts.Add(C + Along * ((c & 1) ? Wide : -Wide) + FVector(0.f, 0.f, (c >> 1) ? High : 0.f));
				UVs.Add(FVector2D(c & 1, c >> 1));
				Colors.Add(Rand);
			}
			Tris.Append({ B, B + 2, B + 1, B + 1, B + 2, B + 3 });
		}
	}
	for (const TPair<FString, int32>& S : SplashSurfaces) { UE_LOG(LogDC, Log, TEXT("WeatherFX: level surface under the rain: %s (%d probes)"), *S.Key, S.Value); }
	if (Verts.Num() == 0) { UE_LOG(LogDC, Warning, TEXT("WeatherFX: no deck found for rain splashes")); return; }
	Normals.Init(FVector::UpVector, Verts.Num());
	Splashes = NewObject<UProceduralMeshComponent>(this, TEXT("Splashes"));
	Splashes->SetMobility(EComponentMobility::Movable);
	Splashes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Splashes->SetCastShadow(false);
	// The water plugin photographs the scene from above for its own effects; bright rings on the deck came back stamped across the sea.
	Splashes->SetHiddenInSceneCapture(true);
	Splashes->SetupAttachment(RootComponent);
	Splashes->RegisterComponent();
	// The points were found on the ship as she lies this frame; from here on they ride with her.
	if (Director->ShipActor && Director->ShipActor->GetRootComponent()) { Splashes->AttachToComponent(Director->ShipActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform); }
	Splashes->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
	SplashMID = UMaterialInstanceDynamic::Create(Base, this);
	Splashes->SetMaterial(0, SplashMID);
	UE_LOG(LogDC, Log, TEXT("WeatherFX: %d rain splash points on the ship"), Verts.Num() / 12);
	{ FBox Box(Verts); int32 Low = 0; for (const FVector& V : Verts) { if (V.Z < Director->SeaLevelZ + 150.f) { ++Low; } } UE_LOG(LogDC, Log, TEXT("WeatherFX: splash bounds %s to %s, %d corners within 1.5 m of the sea"), *Box.Min.ToString(), *Box.Max.ToString(), Low); }
}
