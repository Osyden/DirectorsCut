#pragma once
#include "CoreMinimal.h"

/** Bounded OpenAI chat SSE decoder. Feed raw bytes on one thread; never split UTF-8 first. */
class FDCDlgSSEStream
{
public:
    static constexpr int32 MaxResponseBytes = 128 * 1024;
    static constexpr int32 MaxTextCharacters = 4096;
    bool Append(const uint8* Data, int32 Length);
    bool Finish();
    bool IsDone() const { return bDone; }
    bool HasFailed() const { return !Error.IsEmpty(); }
    const FString& GetText() const { return Text; }
    const FString& GetError() const { return Error; }
private:
    bool ConsumeLine(const uint8* Data, int32 Length);
    bool DispatchEvent();
    bool Fail(const TCHAR* Reason);
    TArray<uint8> Pending;
    FString EventData, EventType, Text, Error;
    int32 TotalBytes = 0;
    bool bDone = false;
};
