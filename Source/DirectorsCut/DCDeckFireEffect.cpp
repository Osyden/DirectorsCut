#include "DCDeckFireEffect.h"
#include "DCRideDirector.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace
{
	struct FFireCard
	{
		FVector Center, Right, Up;
		FVector2D Size;
		FColor Color;
	};

	void DrawCards(UProceduralMeshComponent* Mesh, TArray<FFireCard>& Cards, const FVector& Eye)
	{
		// Sort individual translucent puffs as well as the three component layers.
		Cards.Sort([&Eye](const FFireCard& A, const FFireCard& B)
		{
			return FVector::DistSquared(A.Center, Eye) > FVector::DistSquared(B.Center, Eye);
		});
		TArray<FVector> Vertices, Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<int32> Indices;
		for (const FFireCard& C : Cards)
		{
			const int32 Base = Vertices.Num();
			for (int32 Corner = 0; Corner < 4; ++Corner)
			{
				const FVector2D UV(Corner & 1, Corner >> 1);
				Vertices.Add(C.Center + C.Right * ((UV.X-.5)*C.Size.X) + C.Up * ((UV.Y-.5)*C.Size.Y));
				Normals.Add(FVector::CrossProduct(C.Right, C.Up));
				UVs.Add(UV);
				Colors.Add(C.Color);
			}
			Indices.Append({Base, Base+1, Base+2, Base+2, Base+1, Base+3});
		}
		if (Mesh->GetNumSections() == 0)
		{
			Mesh->CreateMeshSection(0, Vertices, Indices, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
		}
		else
		{
			Mesh->UpdateMeshSection(0, Vertices, Normals, UVs, Colors, TArray<FProcMeshTangent>());
		}
	}
}

ADCDeckFireEffect::ADCDeckFireEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Flames = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Flames"));
	Smoke = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Smoke"));
	Embers = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Embers"));
	for (UProceduralMeshComponent* Mesh : {Flames.Get(), Smoke.Get(), Embers.Get()})
	{
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->bReceivesDecals = false;
		Mesh->SetCanEverAffectNavigation(false);
	}
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(FVector(0,0,65));
	Light->SetLightColor(FLinearColor(1.f,.24f,.035f));
	Light->SetIntensity(1800.f);
	Light->SetAttenuationRadius(650.f);
	Light->SetSourceRadius(30.f);
	Light->SetSoftSourceRadius(45.f);
	Light->SetCastShadows(true);
	// Swatting brings the crew's hair close to the flame. Ordinary geometry
	// shadows alone do not provide strand self-shadowing for this local light.
	Light->bCastDeepShadow = true;
	// A nearby fire should warm the deck, not bleach a whole bank of city fog.
	Light->SetVolumetricScatteringIntensity(.08f);
}

void ADCDeckFireEffect::Initialize(float Seed)
{
	SeedValue = Seed;
	auto Bind = [this](UProceduralMeshComponent* Mesh, const TCHAR* Path)
	{
		UMaterialInstanceDynamic* MID = nullptr;
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, Path))
		{
			MID = UMaterialInstanceDynamic::Create(Base, this);
			Mesh->SetMaterial(0, MID);
		}
		else { UE_LOG(LogDC, Error, TEXT("Deck fire material missing: %s"), Path); }
		return MID;
	};
	FlameMaterial = Bind(Flames, TEXT("/Game/DC/Materials/M_DeckFire.M_DeckFire"));
	SmokeMaterial = Bind(Smoke, TEXT("/Game/DC/Materials/M_DeckSmoke.M_DeckSmoke"));
	EmberMaterial = Bind(Embers, TEXT("/Game/DC/Materials/M_DeckEmber.M_DeckEmber"));
}

void ADCDeckFireEffect::ReactToSwat()
{
	bSwatPending = true;
}

void ADCDeckFireEffect::Extinguish()
{
	if (StopTime >= 0.f) { return; }
	StopTime = Clock;
	SetLifeSpan(3.5f);
}

void ADCDeckFireEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	// A contact arrives before this visual tick. Start its pulse now so a loading
	// hitch is not incorrectly counted as time elapsed since the new contact.
	if (bSwatPending) { SwatTime = Clock; bSwatPending = false; }
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager || !FlameMaterial || !SmokeMaterial || !EmberMaterial) { return; }
	const FVector Eye = GetActorTransform().InverseTransformPosition(PC->PlayerCameraManager->GetCameraLocation());
	const FVector ToEye = Eye.GetSafeNormal();
	FVector Right = FVector::CrossProduct(FVector::UpVector, ToEye).GetSafeNormal();
	if (Right.IsNearlyZero()) { Right = FVector::RightVector; }
	const FVector CameraUp = FVector::CrossProduct(ToEye, Right).GetSafeNormal();
	const float SinceStop = StopTime >= 0.f ? Clock-StopTime : 0.f;
	const float SinceSwat = FMath::Max(0.f, Clock - SwatTime);
	const float Smother = 1.f - FMath::SmoothStep(0.f, .36f, SinceSwat);
	const float Burning = Strength * (1.f-FMath::SmoothStep(0.f,.35f,SinceStop)) * (1.f - .55f * Smother);
	const float Smolder = StopTime >= 0.f ? 1.f-FMath::SmoothStep(.3f,3.3f,SinceStop) : Strength;
	const float Flicker = .82f + .12f*FMath::Sin(Clock*11.7f+SeedValue) + .06f*FMath::Sin(Clock*23.1f+SeedValue*2.f);
	Light->SetIntensity(1800.f*Burning*Flicker);
	Light->SetVisibility(Burning > .001f);
	FlameMaterial->SetScalarParameterValue(TEXT("Clock"), Clock+SeedValue);
	SmokeMaterial->SetScalarParameterValue(TEXT("Clock"), Clock);
	FlameMaterial->SetScalarParameterValue(TEXT("Energy"), .9f+.15f*Flicker);
	float Wind = .7f;
	if (const ADCRideDirector* Dir = ADCRideDirector::Get(this))
	{
		Wind = FMath::Clamp(Dir->GetEnv().WaveHeight/250.f, .35f, 1.2f);
	}
	const FVector Drift(-38.f*Wind, 12.f*Wind, 0);
	FRandomStream Rng(173 + FMath::RoundToInt(SeedValue*100.f));
	TArray<FFireCard> Cards;
	for (int32 I=0; I<7; ++I)
	{
		const float Phase = Rng.FRandRange(0,6.28f);
		const float Pulse = 1.f+.18f*FMath::Sin(Clock*(3.1f+I*.37f)+Phase);
		const float Height = Rng.FRandRange(100,195)*Pulse*FMath::Sqrt(FMath::Max(Burning,.001f));
		const FVector Base(Rng.FRandRange(-42,42),Rng.FRandRange(-40,40),3);
		const FVector Lean = (FVector::UpVector + Drift*.0025f + Right*(.07f*FMath::Sin(Clock*2.5f+Phase))).GetSafeNormal();
		FColor Color(Rng.RandRange(0,255),Rng.RandRange(0,255),0,FMath::RoundToInt(Burning*255));
		Cards.Add({Base+Lean*(Height*.5f),Right,Lean,FVector2D(Rng.FRandRange(65,105),Height),Color});
	}
	DrawCards(Flames,Cards,Eye);
	Flames->SetVisibility(Burning > .001f);
	Cards.Reset();
	for (int32 I=0; I<12; ++I)
	{
		const float Life = Rng.FRandRange(2.6f,4.4f);
		const float Offset = Rng.FRand();
		const float Age = FMath::Frac(Clock/Life+Offset);
		const float Alive = FMath::SmoothStep(0.f,.12f,Age)*(1.f-FMath::SmoothStep(.45f,1.f,Age))*Smolder
			* FMath::SmoothStep(0.f,.8f,Clock);
		const float Height = 90.f+Age*450.f;
		const float Width = FMath::Lerp(70.f,250.f,Age);
		FVector Center(Rng.FRandRange(-40,40),Rng.FRandRange(-35,35),Height);
		Center += Drift*(Age*Life) + Right*(FMath::Sin(Age*6.f+Offset*9.f)*Age*35.f);
		const float Angle = Rng.FRandRange(-.5f,.5f)+Age*.25f;
		const FVector R = Right*FMath::Cos(Angle)+CameraUp*FMath::Sin(Angle);
		const FVector U = CameraUp*FMath::Cos(Angle)-Right*FMath::Sin(Angle);
		FColor Color(Rng.RandRange(0,255),Rng.RandRange(0,255),Rng.RandRange(0,255),FMath::RoundToInt(Alive*255));
		Cards.Add({Center,R,U,FVector2D(Width,Width*1.2f),Color});
	}
	// A successful cloth contact briefly flattens the flame and pushes a small
	// gust of smoke out at its base. Keep the cards present with zero opacity
	// between hits so UpdateMeshSection retains a fixed vertex/index layout.
	FRandomStream GustRng(928 + FMath::RoundToInt(SeedValue * 100.f));
	const float GustAge = FMath::Clamp(SinceSwat / .65f, 0.f, 1.f);
	const float GustAlpha = FMath::SmoothStep(0.f, .05f, SinceSwat)
		* (1.f - FMath::SmoothStep(.12f, .65f, SinceSwat)) * .25f;
	for (int32 I = 0; I < 4; ++I)
	{
		const float Angle = GustRng.FRandRange(0.f, 2.f * PI);
		FVector Center(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Center *= 20.f + GustAge * 65.f;
		Center += Drift * (GustAge * .65f);
		Center.Z = 20.f + GustAge * GustRng.FRandRange(65.f, 110.f);
		const float Width = 45.f + GustAge * 85.f;
		const FColor Color(GustRng.RandRange(0,255), GustRng.RandRange(0,255), GustRng.RandRange(0,255), FMath::RoundToInt(GustAlpha * 255.f));
		Cards.Add({Center, Right, CameraUp, FVector2D(Width, Width * .75f), Color});
	}
	DrawCards(Smoke,Cards,Eye);
	Cards.Reset();
	for (int32 I=0; I<22; ++I)
	{
		const float Life = Rng.FRandRange(1.f,2.3f);
		const float Offset = Rng.FRand();
		const float Age = FMath::Frac(Clock/Life+Offset);
		const float Fade = FMath::SmoothStep(0.f,.06f,Age)*(1.f-FMath::SmoothStep(.35f,1.f,Age))*Burning;
		FVector Center(Rng.FRandRange(-40,40),Rng.FRandRange(-40,40),35+Age*Rng.FRandRange(200,440));
		Center += Drift*(Age*Life)*1.6f + Right*(FMath::Sin(Age*10.f+Offset*12.f)*Age*22.f);
		Cards.Add({Center,Right,CameraUp,FVector2D(Rng.FRandRange(1.1f,2.4f),Rng.FRandRange(3.f,7.f)),FColor(255,255,255,FMath::RoundToInt(Fade*255))});
	}
	DrawCards(Embers,Cards,Eye);
	Embers->SetVisibility(Burning > .001f);
}
