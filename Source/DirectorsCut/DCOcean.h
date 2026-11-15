#pragma once
#include "CoreMinimal.h"

/** DRW waves live in the travelled world's coordinates; the ship stage stays at zero. */
namespace DCOcean
{
    FVector WaveSamplePosition(const UObject* Context, const FVector& StagePosition);
}
