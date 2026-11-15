#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCObstacleWaterContact.generated.h"

/** Submerged stone foundations, wave-following wash and small contact splashes. */
UCLASS()
class UDCObstacleWaterContact : public UActorComponent
{
    GENERATED_BODY()
public:
    UDCObstacleWaterContact();
    static void Attach(AActor* Pillars);
    virtual void TickComponent(float DeltaSeconds,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
private:
    void Build();
    float SurfaceAt(const FVector& Position,int32 Channel) const;
    struct FFoot
    {
        FVector LocalCenter=FVector::ZeroVector;
        float Radius=100.f, Height=0.f, PreviousWater=0.f, NextSplash=0.f;
        bool bSampled=false;
    };
    TArray<FFoot> Feet;
    UPROPERTY() TObjectPtr<class ADCRideDirector> Director;
    UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Foam;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> FoamMaterial;
    UPROPERTY() TObjectPtr<class UParticleSystem> Splash;
    UPROPERTY() TObjectPtr<UClass> WaterLibrary;
    UPROPERTY() TObjectPtr<UFunction> SurfaceFunction;
    TArray<TWeakObjectPtr<class UParticleSystemComponent>> Pulses;
    int32 SampleChannel=0;
    float LogTimer=0.f;
};
