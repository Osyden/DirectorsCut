#pragma once
#include "CoreMinimal.h"

namespace DCConversationPolicy
{
    // Route before keyword parsing; a mention of a station is not itself an order.
    bool IsConversation(const FString& Text);
    // Explicit hazard-removal requests share one interpretation across chat and command parsing.
    bool IsFireOrder(const FString& Text);
    FString AddressedCrew(const FString& Text);
    // Empty means unusable model output. Never speak model metadata or reasoning.
    FString CleanReply(const FString& Raw);
}
