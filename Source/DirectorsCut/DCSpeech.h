#pragma once
#include "CoreMinimal.h"

namespace DCSpeech
{
    // Address corrections only: never fuzzy-replace words throughout an order.
    FString NormalizeCrewAddress(const FString& Text);
    FString CanonicalCrewName(const FString& Name);
}
