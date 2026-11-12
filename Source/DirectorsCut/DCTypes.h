#pragma once

#include "CoreMinimal.h"
#include "DCTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDC, Log, All);

UENUM(BlueprintType)
enum class EDCPhase : uint8 { OpenSea, Fog, Slide, Hell, Breach, Maelstrom, Ending };

UENUM(BlueprintType)
enum class EDCStationType : uint8 { None, Helm, Cannons, Fire, Rigging };

UENUM(BlueprintType)
enum class EDCSide : uint8 { None, Port, Starboard };

UENUM(BlueprintType)
enum class EDCAction : uint8 { None, Man, Fire, HoldFire, TurnPort, TurnStarboard, Steady, Extinguish, Repair, Report };

/** A parsed order. Every input path (voice, typed, keys, LLM) ends up as one of these. */
USTRUCT(BlueprintType)
struct FDCIntent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) bool bValid = false;
	UPROPERTY(BlueprintReadWrite) FString CrewName;
	UPROPERTY(BlueprintReadWrite) EDCStationType Station = EDCStationType::None;
	UPROPERTY(BlueprintReadWrite) EDCSide Side = EDCSide::None;
	UPROPERTY(BlueprintReadWrite) EDCAction Action = EDCAction::None;
	UPROPERTY(BlueprintReadWrite) FString RawText;
};

/** Everything the ride director blends between phases. */
USTRUCT(BlueprintType)
struct FDCEnvState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FLinearColor FogColor = FLinearColor(0.45f, 0.55f, 0.7f);
	UPROPERTY(EditAnywhere) float FogDensity = 0.01f;
	UPROPERTY(EditAnywhere) float FogHeightFalloff = 0.05f;
	UPROPERTY(EditAnywhere) FLinearColor SunColor = FLinearColor(1.f, 0.9f, 0.8f);
	UPROPERTY(EditAnywhere) float SunIntensity = 6.f;
	UPROPERTY(EditAnywhere) float SunPitch = -25.f;
	UPROPERTY(EditAnywhere) float SkyIntensity = 1.f;
	UPROPERTY(EditAnywhere) FLinearColor SeaColor = FLinearColor(0.02f, 0.08f, 0.12f);
	/** Degrees of roll at the peak of a wave. */
	UPROPERTY(EditAnywhere) float SeaRoughness = 1.5f;
	UPROPERTY(EditAnywhere) float Rain = 0.f;
	/** Flashes per minute. */
	UPROPERTY(EditAnywhere) float LightningRate = 0.f;
	/** World scroll speed in cm/s. */
	UPROPERTY(EditAnywhere) float ShipSpeed = 1200.f;
	/** Manual exposure in stops. Higher is brighter. */
	UPROPERTY(EditAnywhere) float Exposure = 0.f;

	static FDCEnvState Lerp(const FDCEnvState& A, const FDCEnvState& B, float T)
	{
		FDCEnvState R;
		R.FogColor = FMath::Lerp(A.FogColor, B.FogColor, T);
		R.FogDensity = FMath::Lerp(A.FogDensity, B.FogDensity, T);
		R.FogHeightFalloff = FMath::Lerp(A.FogHeightFalloff, B.FogHeightFalloff, T);
		R.SunColor = FMath::Lerp(A.SunColor, B.SunColor, T);
		R.SunIntensity = FMath::Lerp(A.SunIntensity, B.SunIntensity, T);
		R.SunPitch = FMath::Lerp(A.SunPitch, B.SunPitch, T);
		R.SkyIntensity = FMath::Lerp(A.SkyIntensity, B.SkyIntensity, T);
		R.SeaColor = FMath::Lerp(A.SeaColor, B.SeaColor, T);
		R.SeaRoughness = FMath::Lerp(A.SeaRoughness, B.SeaRoughness, T);
		R.Rain = FMath::Lerp(A.Rain, B.Rain, T);
		R.LightningRate = FMath::Lerp(A.LightningRate, B.LightningRate, T);
		R.ShipSpeed = FMath::Lerp(A.ShipSpeed, B.ShipSpeed, T);
		R.Exposure = FMath::Lerp(A.Exposure, B.Exposure, T);
		return R;
	}
};

USTRUCT(BlueprintType)
struct FDCPhaseDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EDCPhase Phase = EDCPhase::OpenSea;
	UPROPERTY(EditAnywhere) float StartTime = 0.f;
	/** Seconds taken to blend in from the previous phase. */
	UPROPERTY(EditAnywhere) float BlendTime = 10.f;
	UPROPERTY(EditAnywhere) FDCEnvState Env;
};

inline const TCHAR* DCPhaseName(EDCPhase P)
{
	switch (P)
	{
	case EDCPhase::OpenSea: return TEXT("Open sea");
	case EDCPhase::Fog: return TEXT("The fog");
	case EDCPhase::Slide: return TEXT("The slide");
	case EDCPhase::Hell: return TEXT("Hell channel");
	case EDCPhase::Breach: return TEXT("The breach");
	case EDCPhase::Maelstrom: return TEXT("The maelstrom");
	default: return TEXT("Ending");
	}
}
