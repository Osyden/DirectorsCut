#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
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
	UPROPERTY(EditAnywhere) float BellVolume = 1.f;

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

	FLayer Wind, Sea, Rain, Rumble, Drone, Crackle;
	TArray<TArray<uint8>> Impacts;
	/** Stone grinding along the planking: a groan that sticks and slips, with grit over it. */
	TArray<TArray<uint8>> Scrapes;
	void OnHullStrike(const FVector& Where, float Strength);
	TArray<uint8> KrakenRoar;
	float MaelstromTime = 0.f;
	int32 RoarsPlayed = 0;
	void OnShipHit(float Amount);
	void TickBell(float DeltaSeconds);

	// Recorded sounds, when present in Content/Sonniss.
	struct FRealBed { TWeakObjectPtr<UAudioComponent> Component; float Volume = 0.f; };
	class USoundBase* FindReal(FName Name);
	bool SetReal(FName Name, float Target, float DeltaSeconds);
	bool PlayReal(FName Name, float Volume, float Pitch = 1.f);
	UPROPERTY() TMap<FName, TObjectPtr<class USoundBase>> RealSounds;
	TMap<FName, FRealBed> RealBeds;
	TWeakObjectPtr<class ADCKraken> Kraken;
	int32 LastArmsDestroyed = 0;
	EDCPhase LastPhase = EDCPhase::OpenSea;
	UPROPERTY() TObjectPtr<class USoundBase> BellSound;
	UPROPERTY() TObjectPtr<class UAnimationAsset> BellAnim;
	TWeakObjectPtr<class USkeletalMeshComponent> BellMesh;
	float NextBellIn = 6.f;
	float BellDuck = 0.f;
	int32 BellStrikes = 0;
	void DumpWav(const TCHAR* Name, const TArray<uint8>& Pcm) const;
	TArray<TArray<uint8>> Thunder;
	TArray<FOneShot> OneShots;
	TArray<double> PendingThunder;
	UPROPERTY() TArray<TObjectPtr<USoundWaveProcedural>> Waves;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
};
