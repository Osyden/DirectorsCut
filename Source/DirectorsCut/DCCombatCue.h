#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "DCCombatCue.generated.h"

/** Depth-tested, collision-free attack presentation; never creates a target. */
UCLASS()
class UDCCombatCueComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    static UDCCombatCueComponent* Create(AActor* Owner,FName Name);
    void ShowAlert(const FVector& WorldPosition,float WorldRadius,float Charge01,float Age,bool bShowCue);
    void ShowWeakPoint(const FVector& WorldPosition,const FVector& Axis,float WorldRadius,float Charge01,float Age,float HitFlash,bool bShowCue);
private:
    void Present(const FVector& WorldPosition,const FQuat& Rotation,const TArray<FVector>& Vertices,
        const TArray<int32>& Indices,const TArray<FLinearColor>& Colors,float Glow);
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> CueMaterial;
};
