#pragma once
#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "DCAuthoredTentacle.generated.h"

/** A selected authored Kraken bone branch, posed on the committed attack curve. */
UCLASS()
class UDCAuthoredTentacle : public UPoseableMeshComponent
{
    GENERATED_BODY()
public:
    static UDCAuthoredTentacle* Create(AActor* Owner,class USkeletalMesh* Mesh);
    void Pose(const FVector& Base,const FVector& Bend,const FVector& Joint,const FVector& Tip,float WidthScale);
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
private:
    TArray<FTransform> Reference;
    TArray<int32> Chain;
    TArray<float> ChainU;
    float JointSourceU=.72f;
    TArray<bool> Keep;
    FBox PoseBounds=FBox(ForceInit);
};
