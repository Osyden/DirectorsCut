#include "DCMaelstromWaterBridge.h"

#include "Engine/TextureRenderTarget2DArray.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/UnrealType.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	template <typename T>
	T* ReadObject(UObject* Owner, const FName Name)
	{
		if (!IsValid(Owner)) { return nullptr; }
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Owner->GetClass(), Name);
		return Property ? Cast<T>(Property->GetObjectPropertyValue_InContainer(Owner)) : nullptr;
	}

	bool ReadBool(UObject* Owner, const FName Name)
	{
		const FBoolProperty* Property = IsValid(Owner) ? FindFProperty<FBoolProperty>(Owner->GetClass(), Name) : nullptr;
		return Property && Property->GetPropertyValue_InContainer(Owner);
	}

	struct FVectorBinding
	{
		FName Source;
		FName Destination;
	};

	const FVectorBinding VectorBindings[] =
	{
		{TEXT("CascadeInvTileA"), TEXT("DRWInvTileA")},
		{TEXT("CascadeInvTileB"), TEXT("DRWInvTileB")},
		{TEXT("CascadeShortestA"), TEXT("DRWShortestA")},
		{TEXT("CascadeShortestB"), TEXT("DRWShortestB")},
		{TEXT("CascadeSlopeVarA"), TEXT("DRWSlopeVarA")},
		{TEXT("CascadeSlopeVarB"), TEXT("DRWSlopeVarB")},
		{TEXT("WaveRoughness"), TEXT("DRWRoughness")},
		{TEXT("FoamLevelSizes"), TEXT("DRWFoamLevelSizes")},
		{TEXT("WindDirection"), TEXT("DRWWindDirection")}
	};
}

bool FDCMaelstromWaterBridge::Update(UWorld* World, UMaterialInstanceDynamic* Material, bool bActive)
{
	static const bool bNoFFT = FParse::Param(FCommandLine::Get(),TEXT("DCMaelstromNoFFT"));
	if (bNoFFT || !bActive || !IsValid(World) || !IsValid(Material))
	{
		Reset(Material);
		return false;
	}
	if (BoundWorld.Get() != World || LastMaterial.Get() != Material)
	{
		Reset();
		BoundWorld = World;
		LastMaterial = Material;
	}

	// The fallback stays active through startup, missing plugins and invalidated actors.
	if ((!Grid.IsValid() || Grid->GetWorld() != World) && World->GetTimeSeconds() >= NextScanTime)
	{
		Grid.Reset();
		NextScanTime = World->GetTimeSeconds() + 1.0;
		if (UClass* GridClass = FindObject<UClass>(nullptr, TEXT("/Script/DynamicRealWater.DRW_GridManager")))
		{
			for (TActorIterator<AActor> It(World, GridClass); It; ++It)
			{
				if (ReadBool(*It, TEXT("bPhysicalFFT")) && ReadBool(*It, TEXT("bEnableFFT")))
				{
					Grid = *It;
					break;
				}
			}
		}
	}

	AActor* Ocean = Grid.Get();
	if (!IsValid(Ocean) || !Ocean->IsActorTickEnabled()
		|| !ReadBool(Ocean, TEXT("bPhysicalFFT")) || !ReadBool(Ocean, TEXT("bEnableFFT")))
	{
		Grid.Reset();
		Material->SetScalarParameterValue(TEXT("UseLiveFFT"), 0.f);
		bWasBound = false;
		return false;
	}

	UTextureRenderTarget2DArray* Displacement = ReadObject<UTextureRenderTarget2DArray>(Ocean, TEXT("CascadeDisplacementRT"));
	UTextureRenderTarget2DArray* Derivatives = ReadObject<UTextureRenderTarget2DArray>(Ocean, TEXT("CascadeDerivativesRT"));
	UTextureRenderTarget2DArray* Foam = ReadObject<UTextureRenderTarget2DArray>(Ocean, TEXT("FoamLevelsRT"));
	UMaterialParameterCollection* Collection = ReadObject<UMaterialParameterCollection>(Ocean, TEXT("MPCParameters"));
	UMaterialParameterCollectionInstance* Parameters = Collection ? World->GetParameterCollectionInstance(Collection) : nullptr;
	float Count = 0.f;
	float NormalScale = 1.f;
	float WorldCascades = 0.f;
	FLinearColor Values[UE_ARRAY_COUNT(VectorBindings)];
	bool bReady = IsValid(Displacement) && IsValid(Derivatives) && IsValid(Foam) && IsValid(Parameters)
		&& Parameters->GetScalarParameterValue(TEXT("WorldCascades"), WorldCascades)
		&& Parameters->GetScalarParameterValue(TEXT("CascadeCount"), Count)
		&& Parameters->GetScalarParameterValue(TEXT("CascadeNormalScale"), NormalScale)
		&& WorldCascades > 0.5f && Count >= 1.f && Count <= 8.f && FMath::IsFinite(NormalScale);
	for (int32 Index = 0; bReady && Index < UE_ARRAY_COUNT(VectorBindings); ++Index)
	{
		bReady = Parameters->GetVectorParameterValue(VectorBindings[Index].Source, Values[Index]);
	}
	if (!bReady)
	{
		Material->SetScalarParameterValue(TEXT("UseLiveFFT"), 0.f);
		bWasBound = false;
		return false;
	}

	// These UObjects retain a 1x1 placeholder size: DRW rebinds their RHI texture
	// references to the real simulation arrays. Binding the UObjects preserves that update.
	Material->SetTextureParameterValue(TEXT("DRWDisplacementRT"), Displacement);
	Material->SetTextureParameterValue(TEXT("DRWDerivativesRT"), Derivatives);
	Material->SetTextureParameterValue(TEXT("DRWFoamLevelsRT"), Foam);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(VectorBindings); ++Index)
	{
		Material->SetVectorParameterValue(VectorBindings[Index].Destination, Values[Index]);
	}
	Material->SetScalarParameterValue(TEXT("DRWCascadeCount"), Count);
	Material->SetScalarParameterValue(TEXT("DRWNormalScale"), NormalScale);
	Material->SetScalarParameterValue(TEXT("UseLiveFFT"), 1.f);
	if (!bWasBound)
	{
		UE_LOG(LogTemp, Display, TEXT("DC_MAELSTROM live DRW FFT material bound: %s, %.0f cascades; geometry unchanged"), *Ocean->GetName(), Count);
	}
	bWasBound = true;
	return true;
}

void FDCMaelstromWaterBridge::Reset(UMaterialInstanceDynamic* Material)
{
	if (LastMaterial.IsValid()) { LastMaterial->SetScalarParameterValue(TEXT("UseLiveFFT"), 0.f); }
	if (IsValid(Material) && Material != LastMaterial.Get()) { Material->SetScalarParameterValue(TEXT("UseLiveFFT"), 0.f); }
	LastMaterial.Reset();
	Grid.Reset();
	BoundWorld.Reset();
	NextScanTime = 0.0;
	bWasBound = false;
}
