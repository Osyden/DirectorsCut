#pragma once

#include "CoreMinimal.h"
#include "DCTypes.h"

/** Shared response validation for the HTTP and NVIDIA local dialogue backends. */
namespace DCLanguageModelResponse
{
    /** Extract a nonempty assistant message from a non-streaming chat-completion response. */
    bool ExtractChatContent(const FString& ResponseJson, FString& OutContent);

    /**
     * Accept exactly the four string fields crew/station/side/action and their supported values.
     * A valid action:none returns true with bValid=false; malformed or contradictory output
     * returns false. Both failure paths leave no executable partial intent in OutIntent.
     */
    bool ParseIntent(const FString& ModelContent, const FString& RawOrder, FDCIntent& OutIntent);
}
