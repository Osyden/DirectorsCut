#include "DCSpeech.h"

FString DCSpeech::CanonicalCrewName(const FString& Name)
{
    const TPair<const TCHAR*,const TCHAR*> Names[] = {
        {TEXT("Rex"),TEXT("Rex")}, {TEXT("Recks"),TEXT("Rex")}, {TEXT("Wrecks"),TEXT("Rex")},
        {TEXT("Duncan"),TEXT("Duncan")}, {TEXT("Dunkin"),TEXT("Duncan")}, {TEXT("Dunken"),TEXT("Duncan")},
        {TEXT("Ruby"),TEXT("Ruby")}, {TEXT("Rubie"),TEXT("Ruby")},
        // Keep existing recordings and developer scripts compatible; not in Whisper's hints.
        {TEXT("Finch"),TEXT("Rex")}, {TEXT("Marlow"),TEXT("Duncan")}, {TEXT("Odessa"),TEXT("Ruby")}
    };
    for (const auto& Entry : Names) { if (Name.Equals(Entry.Key, ESearchCase::IgnoreCase)) { return Entry.Value; } }
    return FString();
}

FString DCSpeech::NormalizeCrewAddress(const FString& Text)
{
    const FString Trimmed = Text.TrimStartAndEnd();
    int32 End = 0;
    while (End < Trimmed.Len() && FChar::IsAlnum(Trimmed[End])) { ++End; }
    const FString Canonical = CanonicalCrewName(Trimmed.Left(End));
    if (Canonical.IsEmpty()) { return Text; }
    int32 Next = End;
    while (Next < Trimmed.Len() && !FChar::IsAlnum(Trimmed[Next])) { ++Next; }
    int32 NextEnd = Next;
    while (NextEnd < Trimmed.Len() && FChar::IsAlnum(Trimmed[NextEnd])) { ++NextEnd; }
    const FString Verb = Trimmed.Mid(Next, NextEnd-Next).ToLower();
    const TCHAR* Commands[] = {TEXT("take"),TEXT("man"),TEXT("get"),TEXT("go"),TEXT("hard"),TEXT("turn"),
        TEXT("steady"),TEXT("hold"),TEXT("fire"),TEXT("shoot"),TEXT("stop"),TEXT("cease"),TEXT("put"),
        TEXT("douse"),TEXT("extinguish"),TEXT("repair"),TEXT("mend"),TEXT("fix"),TEXT("report"),TEXT("status"),
        TEXT("do"),TEXT("don"),TEXT("please"),TEXT("port"),TEXT("starboard"),TEXT("left"),TEXT("right")};
    for (const TCHAR* Command : Commands)
    {
        if (Verb == Command) { return Canonical + Trimmed.Mid(End); }
    }
    return Text; // "wrecks ahead" and ordinary prose are not crew addresses.
}
