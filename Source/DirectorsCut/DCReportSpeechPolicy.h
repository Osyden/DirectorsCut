#pragma once
#include "CoreMinimal.h"

/** Shared only by one unsolicited report; command replies never carry this guard. */
struct FDCReportSpeechGuard
{
    double StartDeadline=0;
    TFunction<bool()> IsStillRelevant;
    bool IsCurrent() const { return !IsStillRelevant || IsStillRelevant(); }
    bool CanStart(double Now) const { return Now<=StartDeadline && IsCurrent(); }
};
