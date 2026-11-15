#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DCAceFace.generated.h"

class UACEAudioCurveSourceComponent;
class USkeletalMeshComponent;
class UPoseAsset;

/** Copies the body's pose, maps ACE's ARKit curves, then lets the face's own DNA/RigLogic run. */
UCLASS(Transient)
class UDCAceFaceAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) TObjectPtr<UPoseAsset> Mapping;
    virtual void NativeInitializeAnimation() override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};

namespace DCAceFace
{
    UACEAudioCurveSourceComponent* Bind(AActor* Avatar);
}
