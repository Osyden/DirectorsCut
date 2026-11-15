#pragma once
#include "CoreMinimal.h"

/** Shared only by one unsolicited report; command replies never carry this guard. */
struct FDCReportSpeechGuard
{
    double StartDeadline=0;
    TFunction<bool()> IsStillRelevant;
    // Optional stable predicate for a sentence already admitted to playback.
    // By default urgent reports retain their original cancellation semantics.
    TFunction<bool()> IsStillRelevantDuringPlayback;
    double PlaybackStartedAt=-1;
    bool IsCurrent() const { return !IsStillRelevant || IsStillRelevant(); }
    bool CanStart(double Now) const { return Now<=StartDeadline && IsCurrent(); }
    bool CanContinue() const { return IsStillRelevantDuringPlayback ? IsStillRelevantDuringPlayback() : IsCurrent(); }
};
