#include "DCACESpeechCallback.h"
#include "DCVoice.h"

void UDCACESpeechCallback::Sent(bool bSuccess)
{
    ADCVoice* Owner = Voice.Get();
    if (!Owner || Owner->bEnding || Owner->SpeechEpoch != Epoch ||
        Owner->ACERequestSerial != Serial || Owner->ActiveACERelay != this) { return; }
    Owner->ACESent(bSuccess);
}
