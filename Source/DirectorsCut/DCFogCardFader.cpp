#include "DCFogCardFader.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UDCFogCardFader::UDCFogCardFader()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UDCFogCardFader::Initialize(UStaticMeshComponent* InMesh, UMaterialInstanceDynamic* InMaterial, float InDensity, bool bInGroundMist)
{
	Mesh = InMesh;
	Material = InMaterial;
	Density = InDensity;
	bGroundMist = bInGroundMist;
	const bool bLegacy = FParse::Param(FCommandLine::Get(), TEXT("DCLegacyFogCards"));
	SetComponentTickEnabled(!bLegacy);
	ShownDensity = bLegacy ? Density : 0.f;
	if (Material) { Material->SetScalarParameterValue(TEXT("Fog Density"), ShownDensity); }
}

void UDCFogCardFader::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	if (!Mesh || !Material || !GetOwner() || GetOwner()->IsHidden()) { return; }
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC) { return; }
	FVector Eye;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(Eye, ViewRotation);
	const FVector ToEye = (Eye - Mesh->Bounds.Origin).GetSafeNormal();
	const float Facing = FVector::DotProduct(Mesh->GetUpVector(), ToEye);
	// These optional detail sheets cover up to 380 m, but have no thickness. A
	// shallow viewing angle compresses the entire texture into a bright stripe.
	// Ground sheets can also rise with the trough above the captain: never draw
	// their underside as a floating ceiling. Real volume banks provide the depth.
	const float Angle = bGroundMist ? Facing : FMath::Abs(Facing);
	const float Amount = FMath::SmoothStep(.14f, .34f, Angle);
	const float WantedDensity = Density * Amount;
	if (FMath::Abs(WantedDensity - ShownDensity) > .002f || (WantedDensity == 0.f) != (ShownDensity == 0.f))
	{
		ShownDensity = WantedDensity;
		Material->SetScalarParameterValue(TEXT("Fog Density"), ShownDensity);
	}
}
