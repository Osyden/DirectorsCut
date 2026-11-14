#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCFogCardFader.generated.h"

/** Keep auxiliary mist sheets from exposing their flat edges from the deck. */
UCLASS()
class UDCFogCardFader : public UActorComponent
{
	GENERATED_BODY()
public:
	UDCFogCardFader();
	void Initialize(class UStaticMeshComponent* InMesh, class UMaterialInstanceDynamic* InMaterial, float InDensity, bool bInGroundMist);
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> Material;
	float Density = 0.f;
	float ShownDensity = 0.f;
	bool bGroundMist = true;
};
