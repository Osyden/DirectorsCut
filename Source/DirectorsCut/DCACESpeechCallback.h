#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DCACESpeechCallback.generated.h"

/** A moved SDK completion delegate can outlive cancellation. Validate its owner and generation. */
UCLASS()
class UDCACESpeechCallback : public UObject
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<class ADCVoice> Voice;
    int32 Epoch = 0, Serial = 0;
    UFUNCTION() void Sent(bool bSuccess);
};
