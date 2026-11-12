#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCAudio.generated.h"

class UAudioComponent;
class USoundWaveProcedural;
class ADCRideDirector;

/**
 * Weather and sea sound, synthesised at startup from filtered noise so the project needs no audio files:
 * wind, sea wash, rain, a low rumble for the hell channel, and thunder that follows each lightning flash.
 * Layer volumes follow the ride director's environment every frame.
 */
UCLASS()
class ADCAudio : public AActor
{
	GENERATED_BODY()

public:
	ADCAudio();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere) float MasterVolume = 0.8f;

protected:
	struct FLayer
	{
		TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> Pcm;
		TWeakObjectPtr<UAudioComponent> Component;
		float Volume = 0.f;
	};
	struct FOneShot
	{
		TWeakObjectPtr<UAudioComponent> Component;
		double EndTime = 0.0;
	};

	void StartLayer(FLayer& Layer, TArray<float>&& Samples);
	void PlayOneShot(const TArray<uint8>& Pcm, float Volume);
	void OnLightning();
	void SetLayer(FLayer& Layer, float Target, float DeltaSeconds);

	static const int32 Rate = 24000;

	FLayer Wind, Sea, Rain, Rumble;
	TArray<TArray<uint8>> Thunder;
	TArray<FOneShot> OneShots;
	TArray<double> PendingThunder;
	UPROPERTY() TArray<TObjectPtr<USoundWaveProcedural>> Waves;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
};
