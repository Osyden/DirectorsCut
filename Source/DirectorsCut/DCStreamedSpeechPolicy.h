#pragma once
#include "CoreMinimal.h"

namespace DCStreamedSpeechPolicy
{
    // One instance per utterance. Call on the owning thread with append-only cumulative
    // text, not token deltas. Returned chunks have already been committed: enqueue
    // only these chunks, never the full final reply again. Reset for the next reply.
    class FChunker
    {
    public:
        TArray<FString> Push(const FString& CumulativeText, bool bFinal = false);
        bool WasRejected() const { return bRejected; }
        FString GetEmittedText() const { return EmittedText; }
        void Reset();

    private:
        FString PreviousRaw;
        FString EmittedText;
        bool bDeferred = false;
        bool bRejected = false;
        bool bFinished = false;
    };
}
