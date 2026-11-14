#include "DCMaelstromAtmosphere.h"
#include "DCMaelstrom.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

void ADCMaelstromAtmosphere::CaptureSkyPlate()
{
	UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/Sky/M_MaelstromSkyPlate.M_MaelstromSkyPlate"));
	if (!Material || !StageSky.IsValid())
	{
		UE_LOG(LogDC,Error,TEXT("MAEL_PLATE missing sky material; run Tools/make_maelstrom_sky_plate.py"));
		return;
	}
	if (!SkyPlate)
	{
		SkyPlate=NewObject<UStaticMeshComponent>(this,TEXT("MaelstromPhotographicSky"));
		AddInstanceComponent(SkyPlate);
		SkyPlate->SetupAttachment(RootComponent);
		SkyPlate->SetMobility(EComponentMobility::Movable);
		SkyPlate->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		SkyPlate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkyPlate->SetCastShadow(false);
		SkyPlate->SetReceivesDecals(false);
		SkyPlate->bAffectDistanceFieldLighting=false;
		SkyPlate->SetRelativeScale3D(FVector(40000.f)); // 20 km radius; outside the entire arena.
		SkyPlate->RegisterComponent();
	}
	SkyPlateMID=UMaterialInstanceDynamic::Create(Material,this);
	SkyPlate->SetMaterial(0,SkyPlateMID);
	if (UMaterialInterface* Mask=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/Sky/M_MaelstromSkyShadow.M_MaelstromSkyShadow")))
	{ SkyMaskMID=UMaterialInstanceDynamic::Create(Mask,this); }
	TInlineComponentArray<UStaticMeshComponent*> Meshes(StageSky.Get());
	for (UStaticMeshComponent* Mesh:Meshes) { SavedSkyMeshVisibility.Add(Mesh,Mesh->IsVisible()); }
	StageCloud=StageSky->FindComponentByClass<UVolumetricCloudComponent>();
	if (StageCloud.IsValid()) { bPreviousCloudVisible=StageCloud->IsVisible(); }
	bPreviousShafts=bEnableShafts;
	bEnableShafts=false;
	UpdateSkyPlate();
	UE_LOG(LogDC,Log,TEXT("MAEL_PLATE active; saved %d native sky meshes, cloud visible=%d"),SavedSkyMeshVisibility.Num(),bPreviousCloudVisible);
}

void ADCMaelstromAtmosphere::UpdateSkyPlate()
{
	if (!SkyPlateMID) { return; }
	for (const auto& Saved:SavedSkyMeshVisibility)
	{ if (UStaticMeshComponent* Mesh=Saved.Key.Get()) { Mesh->SetVisibility(false); } }
	if (StageCloud.IsValid()) { StageCloud->SetVisibility(false); }
	SkyPlate->SetVisibility(true);
	float Gain=0.18f;
	FParse::Value(FCommandLine::Get(),TEXT("DCMaelPlateGain="),Gain);
	SkyPlateMID->SetScalarParameterValue(TEXT("Gain"),FMath::Max(0.f,Gain));
	float AnchorU=0.56f, AnchorElevation=40.f;
	FParse::Value(FCommandLine::Get(),TEXT("DCMaelPlateAnchorU="),AnchorU);
	FParse::Value(FCommandLine::Get(),TEXT("DCMaelPlateAnchorElevation="),AnchorElevation);
	SkyPlateMID->SetScalarParameterValue(TEXT("AnchorU"),AnchorU);
	SkyPlateMID->SetScalarParameterValue(TEXT("AnchorElevation"),FMath::DegreesToRadians(AnchorElevation));
	if (SkyMaskMID)
	{
		SkyMaskMID->SetScalarParameterValue(TEXT("AnchorU"),AnchorU);
		SkyMaskMID->SetScalarParameterValue(TEXT("AnchorElevation"),FMath::DegreesToRadians(AnchorElevation));
	}
	if (UDirectionalLightComponent* Sun=FindSun())
	{
		const FLinearColor ToSun(-Sun->GetForwardVector());
		SkyPlateMID->SetVectorParameterValue(TEXT("ToSun"),ToSun);
		if (SkyMaskMID)
		{
			SkyMaskMID->SetVectorParameterValue(TEXT("ToSun"),ToSun);
			const ADCMaelstrom* Stage=ADCMaelstrom::Get(this);
			SkyMaskMID->SetScalarParameterValue(TEXT("RimHeight"),Stage ? FMath::Max(0.f,float(Stage->GetArenaTransform().GetLocation().Z)*0.00001f) : 0.f);
		}
	}
}

void ADCMaelstromAtmosphere::RestoreSkyPlate()
{
	if (!SkyPlateMID) { return; }
	if (SkyPlate) { SkyPlate->SetVisibility(false); }
	for (const auto& Saved:SavedSkyMeshVisibility)
	{ if (UStaticMeshComponent* Mesh=Saved.Key.Get()) { Mesh->SetVisibility(Saved.Value); } }
	if (StageCloud.IsValid()) { StageCloud->SetVisibility(bPreviousCloudVisible); }
	bEnableShafts=bPreviousShafts;
	UE_LOG(LogDC,Log,TEXT("MAEL_PLATE restored %d sky meshes and native cloud visibility"),SavedSkyMeshVisibility.Num());
	SavedSkyMeshVisibility.Empty();
	SkyPlateMID=nullptr;
	SkyMaskMID=nullptr;
}
