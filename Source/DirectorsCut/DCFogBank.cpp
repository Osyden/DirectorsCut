#include "DCFogBank.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/HeterogeneousVolumeComponent.h"

ADCFogBank::ADCFogBank()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	SetRootComponent(Mesh);
}

ADCFogBank* ADCFogBank::Spawn(UWorld* World, const FDCFogBankDesc& InDesc, int32 Seed)
{
	static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("DCNoBanks"));
	UMaterialInterface* Base = bOff ? nullptr : LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_FogVolume.M_FogVolume"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!World || !Base || !Sphere) { return nullptr; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADCFogBank* Bank = World->SpawnActor<ADCFogBank>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Bank) { return nullptr; }
	Bank->Desc = InDesc;
	Bank->Director = ADCRideDirector::Get(World);
	Bank->Mesh->SetStaticMesh(Sphere);
	Bank->Mesh->SetWorldScale3D(InDesc.Radii / 50.f);
	Bank->MID = UMaterialInstanceDynamic::Create(Base, Bank);
	Bank->Mesh->SetMaterial(0, Bank->MID);
	Bank->MID->SetVectorParameterValue(TEXT("Tint"), InDesc.Tint);
	Bank->MID->SetVectorParameterValue(TEXT("Radii"), FLinearColor(InDesc.Radii.X, InDesc.Radii.Y, InDesc.Radii.Z, 0.f));
	Bank->MID->SetScalarParameterValue(TEXT("Size"), InDesc.Size);
	Bank->MID->SetScalarParameterValue(TEXT("Erode"), InDesc.Erode);
	Bank->MID->SetVectorParameterValue(TEXT("Drift"), FLinearColor(InDesc.Drift.X, InDesc.Drift.Y, InDesc.Drift.Z, 0.f));
	Bank->MID->SetVectorParameterValue(TEXT("Stretch"), FLinearColor(InDesc.Stretch.X, InDesc.Stretch.Y, InDesc.Stretch.Z, 0.f));
	Bank->MID->SetScalarParameterValue(TEXT("Seed"), static_cast<float>(Seed % 97));
	Bank->MID->SetScalarParameterValue(TEXT("Density"), 0.f);
	Bank->MID->SetScalarParameterValue(TEXT("Wisp"), InDesc.Wisp);
	const float SeaZ = Bank->Director ? Bank->Director->SeaLevelZ : 0.f;
	// No fog below the sea, and it gathers over the first few metres above it.
	Bank->MID->SetVectorParameterValue(TEXT("Floor"), FLinearColor(SeaZ - 150.f, 0.f, 0.f, 500.f));
	Bank->Mesh->SetVisibility(false);
	UMaterialInterface* HeroBase = InDesc.bHero ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_HeteroCloud.M_HeteroCloud")) : nullptr;
	if (HeroBase)
	{
		static float Res = 96.f; static bool bResRead = false;
		if (!bResRead)
		{
			bResRead = true;
			FParse::Value(FCommandLine::Get(), TEXT("DCHeroRes="), Res);
			// The engine stops marching these volumes 300 m from the eye unless told otherwise.
			if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance"))) { V->Set(120000.f, ECVF_SetByCode); }
			if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxShadowTraceDistance"))) { V->Set(60000.f, ECVF_SetByCode); }
		}
		UHeterogeneousVolumeComponent* H = NewObject<UHeterogeneousVolumeComponent>(Bank);
		H->SetMobility(EComponentMobility::Movable);
		H->VolumeResolution = FIntVector(Res, Res, Res);
		H->bPivotAtCentroid = true;
		H->SetupAttachment(Bank->Mesh);
		H->SetUsingAbsoluteScale(true);
		const FVector Unit = InDesc.Radii * 2.f / Res;   // cm of world to one unit of the volume, per axis
		H->SetWorldScale3D(Unit);
		Bank->HeroMID = UMaterialInstanceDynamic::Create(HeroBase, Bank);
		Bank->HeroMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.3f, 0.34f, 0.42f));
		Bank->HeroMID->SetVectorParameterValue(TEXT("Radii"), FLinearColor(Res * 0.5f, Res * 0.5f, Res * 0.5f, 0.f));
		Bank->HeroMID->SetVectorParameterValue(TEXT("Unit"), FLinearColor(Unit.X, Unit.Y, Unit.Z, 0.f));
		Bank->HeroMID->SetScalarParameterValue(TEXT("Size"), InDesc.Size * 4.f);
		Bank->HeroMID->SetScalarParameterValue(TEXT("Erode"), InDesc.Erode);
		Bank->HeroMID->SetVectorParameterValue(TEXT("Drift"), FLinearColor(InDesc.Drift.X, InDesc.Drift.Y, InDesc.Drift.Z, 0.f));
		Bank->HeroMID->SetScalarParameterValue(TEXT("Seed"), static_cast<float>(Seed % 97));
		Bank->HeroMID->SetScalarParameterValue(TEXT("Density"), 0.f);
		H->SetMaterial(0, Bank->HeroMID);
		H->RegisterComponent();
		H->SetVisibility(false);
		Bank->Hero = H;
		Bank->HeroUnit = (Unit.X + Unit.Y + Unit.Z) / 3.f;
	}
	return Bank;
}

void ADCFogBank::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!MID || !Director) { return; }
	static float Scale = 1.f; static bool bRead = false;   // test hook: -DCBankDensity=
	if (!bRead) { bRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCBankDensity="), Scale); }

	const FVector At = GetActorLocation();
	// The ship lies at the origin. Banks belong to misty weather; they go when the mist goes.
	const float Weather = FMath::Clamp(Director->GetEnv().Mist / 0.5f, 0.f, 1.f);
	const bool bRevealed = Desc.RevealWithin > 0.f && FVector2D(At.X, At.Y).Size() < Desc.RevealWithin;
	// A revealing curtain parts in a few seconds; everything else comes and goes slowly.
	Amount = FMath::FInterpConstantTo(Amount, bRevealed ? 0.f : 1.f, DeltaSeconds, bRevealed ? 0.22f : 0.5f);
	const float Now = Amount * Weather * Scale;
	if (Now > 0.f && HeroMID) { static float HG = 0.25f; static bool bHG = false; if (!bHG) { bHG = true; FParse::Value(FCommandLine::Get(), TEXT("DCHeroGlow="), HG); } HeroMID->SetVectorParameterValue(TEXT("Glow"), (Director->FogGlow + Director->FogFlare * 0.25f) * HG); }
	else if (Now > 0.f) { MID->SetVectorParameterValue(TEXT("Glow"), Director->FogGlow + Director->FogFlare * (Desc.bOnDeck ? 0.12f : 1.f)); }
	if (FMath::Abs(Now - Shown) > 0.002f || (Now == 0.f) != (Shown == 0.f))
	{
		Shown = Now;
		if (Hero)
		{
			// Its extinction is per unit of the volume, not per cm.
			static float HeroDensity = 1.f; static bool bHD = false; if (!bHD) { bHD = true; FParse::Value(FCommandLine::Get(), TEXT("DCHeroDensity="), HeroDensity); }
			HeroMID->SetScalarParameterValue(TEXT("Density"), Desc.Density * Now * HeroUnit * HeroDensity);
			Hero->SetVisibility(Now > 0.f);
		}
		else
		{
			MID->SetScalarParameterValue(TEXT("Density"), Desc.Density * Now);
			Mesh->SetVisibility(Now > 0.f);
		}
	}
}
