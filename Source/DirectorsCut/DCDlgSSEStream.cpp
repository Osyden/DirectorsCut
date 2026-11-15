#include "DCDlgSSEStream.h"
#include "Containers/StringConv.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace DCDlgSSEStreamPrivate
{
    bool ValidUTF8(const uint8* Data, int32 Length)
    {
        for (int32 Index=0; Index<Length;)
        {
            const uint8 Lead=Data[Index++];
            if (Lead<0x80) { if (Lead==0) { return false; } continue; }
            const int32 Count=Lead>=0xC2 && Lead<=0xDF ? 1 : Lead>=0xE0 && Lead<=0xEF ? 2 : Lead>=0xF0 && Lead<=0xF4 ? 3 : -1;
            if (Count<0 || Index+Count>Length) { return false; }
            const uint8 Next=Data[Index];
            if ((Lead==0xE0 && Next<0xA0) || (Lead==0xED && Next>=0xA0) ||
                (Lead==0xF0 && Next<0x90) || (Lead==0xF4 && Next>=0x90)) { return false; }
            for (int32 Byte=0; Byte<Count; ++Byte) { if ((Data[Index++] & 0xC0)!=0x80) { return false; } }
        }
        return true;
    }
}

bool FDCDlgSSEStream::Fail(const TCHAR* Reason)
{
    if (Error.IsEmpty()) { Error=Reason; }
    return false;
}

bool FDCDlgSSEStream::Append(const uint8* Data, int32 Length)
{
    if (HasFailed()) { return false; }
    if (Length<0 || (Length && !Data) || Length>MaxResponseBytes-TotalBytes) { return Fail(TEXT("SSE response exceeds byte limit or has invalid input")); }
    TotalBytes+=Length;
    if (Length) { Pending.Append(Data,Length); }
    int32 Start=0;
    for (int32 Index=0; Index<Pending.Num(); ++Index)
    {
        if (Pending[Index]!=uint8('\n')) { continue; }
        int32 LineLength=Index-Start;
        if (LineLength && Pending[Index-1]==uint8('\r')) { --LineLength; }
        if (!ConsumeLine(Pending.GetData()+Start,LineLength)) { return false; }
        Start=Index+1;
    }
    if (Start) { Pending.RemoveAt(0,Start,EAllowShrinking::No); }
    return true;
}

bool FDCDlgSSEStream::ConsumeLine(const uint8* Data, int32 Length)
{
    if (!Length) { return DispatchEvent(); }
    if (!DCDlgSSEStreamPrivate::ValidUTF8(Data,Length)) { return Fail(TEXT("Invalid UTF-8 in SSE event")); }
    const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Data),Length);
    const FString Line(Converted.Length(),Converted.Get());
    if (Line.StartsWith(TEXT(":"))) { return true; }
    int32 Colon=INDEX_NONE;
    const bool bHasColon=Line.FindChar(TEXT(':'),Colon);
    const FString Field=bHasColon ? Line.Left(Colon) : Line;
    FString Value=bHasColon ? Line.Mid(Colon+1) : FString();
    if (Value.StartsWith(TEXT(" "))) { Value.RightChopInline(1); }
    if (Field==TEXT("data"))
    {
        if (!EventData.IsEmpty()) { EventData+=TEXT("\n"); }
        EventData+=Value;
    }
    else if (Field==TEXT("event")) { EventType=Value; }
    return true;
}

bool FDCDlgSSEStream::DispatchEvent()
{
    FString Data=MoveTemp(EventData), Type=MoveTemp(EventType);
    EventData.Reset(); EventType.Reset();
    if (Type==TEXT("error")) { return Fail(TEXT("SSE error event")); }
    if (Data.IsEmpty()) { return true; }
    if (bDone) { return Fail(TEXT("SSE data after DONE")); }
    if (Data.TrimStartAndEnd()==TEXT("[DONE]")) { bDone=true; return true; }
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),Object) || !Object.IsValid() || Object->HasField(TEXT("error")))
    { return Fail(TEXT("Malformed SSE JSON or model error")); }
    const TArray<TSharedPtr<FJsonValue>>* Choices=nullptr;
    if (!Object->TryGetArrayField(TEXT("choices"),Choices)) { return Fail(TEXT("Missing SSE choices")); }
    // Some servers append token-usage metadata after the last delta.
    if (Choices->IsEmpty() && Object->HasField(TEXT("usage"))) { return true; }
    if (Choices->Num()!=1 || !(*Choices)[0].IsValid() || (*Choices)[0]->Type!=EJson::Object) { return Fail(TEXT("Expected one SSE choice")); }
    const TSharedPtr<FJsonObject> Choice=(*Choices)[0]->AsObject();
    double Index=0;
    if (!Choice->TryGetNumberField(TEXT("index"),Index) || Index!=0) { return Fail(TEXT("Unexpected SSE choice index")); }
    const TSharedPtr<FJsonObject>* Delta=nullptr;
    if (!Choice->TryGetObjectField(TEXT("delta"),Delta) || !Delta->IsValid()) { return Fail(TEXT("Missing SSE delta")); }
    if ((*Delta)->HasField(TEXT("tool_calls")) || (*Delta)->HasField(TEXT("function_call"))) { return Fail(TEXT("Tool calls are not spoken dialogue")); }
    const TSharedPtr<FJsonValue> Content=(*Delta)->TryGetField(TEXT("content"));
    if (Content.IsValid() && Content->Type!=EJson::Null)
    {
        if (Content->Type!=EJson::String) { return Fail(TEXT("SSE content must be text")); }
        const FString Piece=Content->AsString();
        if (Piece.Len()>MaxTextCharacters-Text.Len()) { return Fail(TEXT("SSE text exceeds limit")); }
        Text+=Piece;
    }
    // role-only deltas and reasoning_content are deliberately not spoken.
    return true;
}

bool FDCDlgSSEStream::Finish()
{
    if (HasFailed()) { return false; }
    if (!bDone || Pending.Num() || !EventData.IsEmpty() || !EventType.IsEmpty()) { return Fail(TEXT("Truncated SSE response or missing DONE")); }
    if (Text.TrimStartAndEnd().IsEmpty()) { return Fail(TEXT("SSE response contained no spoken text")); }
    return true;
}
