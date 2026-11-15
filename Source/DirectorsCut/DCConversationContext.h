#pragma once

#include "CoreMinimal.h"

/** Read-only, game-thread snapshots for free crew conversation. No orders are executed here. */
namespace DCConversationContext
{
    /** Stable character card for the addressed speaker, separate from changing scene facts. */
    FString Persona(const FString& Speaker);
    /** Current crew, captain, ship, weather and visible encounter context. Does not include future events. */
    FString Build(const UObject* WorldContext, const FString& Speaker);
    /** Coarse scene boundary for expiring conversation; callers must also detect rewinds/reloads. */
    FString SceneKey(const UObject* WorldContext);
}
