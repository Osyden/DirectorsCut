#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "DCCrewWorkAnim.generated.h"

/** Evaluated on the existing mannequin pose driver, so assembled pirate bodies
 * inherit the action through their existing leader pose. Facial ACE is separate. */
UCLASS(Transient)
class UDCCrewWorkAnim : public UAnimSingleNodeInstance
{
    GENERATED_BODY()
public:
    float SwatAge = 10.f;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
