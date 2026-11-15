#include "DCStreamedSpeechPolicy.h"
#include "DCConversationPolicy.h"

namespace DCStreamedSpeechPolicyPrivate
{
    FString Normalize(const FString& Text)
    {
        TArray<FString> Words;
        Text.ParseIntoArrayWS(Words);
        return FString::Join(Words, TEXT(" "));
    }

    bool IsSentenceEnd(TCHAR C)
    {
        return C == TEXT('.') || C == TEXT('!') || C == TEXT('?');
    }

    bool HasRoleLabel(const FString& Text)
    {
        const TCHAR* Labels[] = { TEXT("assistant:"), TEXT("user:"), TEXT("system:"), TEXT("developer:"),
            TEXT("role:"), TEXT("content:"), TEXT("analysis:"), TEXT("final:"), TEXT("reasoning:"),
            TEXT("response:"), TEXT("rex:"), TEXT("duncan:"), TEXT("ruby:") };
        // Also catch labels embedded after a sentence, which the line-oriented
        // final cleaner deliberately does not attempt to parse.
        const FString Lower = Text.ToLower();
        for (const TCHAR* Label : Labels)
        {
            int32 From = 0;
            for (;;)
            {
                const int32 Index = Lower.Find(Label, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
                if (Index == INDEX_NONE) { break; }
                if (Index == 0 || !FChar::IsAlnum(Lower[Index - 1])) { return true; }
                From = Index + 1;
            }
        }
        return false;
    }

    bool HasCompleteUnicode(const FString& Text)
    {
        // FString offsets are TCHAR offsets, never UTF-8 byte offsets. On Windows
        // do not commit a broken UTF-16 surrogate pair from a split token update.
        for (int32 Index = 0; Index < Text.Len(); ++Index)
        {
            const uint32 C = static_cast<uint32>(Text[Index]);
            if (C >= 0xD800 && C <= 0xDBFF)
            {
                if (++Index >= Text.Len()) { return false; }
                const uint32 Next = static_cast<uint32>(Text[Index]);
                if (Next < 0xDC00 || Next > 0xDFFF) { return false; }
            }
            else if (C >= 0xDC00 && C <= 0xDFFF) { return false; }
        }
        return true;
    }

    bool MustDefer(const FString& Text)
    {
        if (Text.IsEmpty()) { return false; }
        const TCHAR C = Text[0];
        return C == TEXT('<') || C == TEXT('`') || C == TEXT('"') || C == TEXT('\'') ||
            C == 0x201C || C == 0x2018 || C == TEXT('{') || C == TEXT('[') ||
            C == TEXT('#') || C == TEXT('*') || C == TEXT('(');
    }

    int32 WordCount(const FString& Text)
    {
        TArray<FString> Words;
        Text.ParseIntoArrayWS(Words);
        return Words.Num();
    }
}

void DCStreamedSpeechPolicy::FChunker::Reset()
{
    PreviousRaw.Reset();
    EmittedText.Reset();
    bDeferred = bRejected = bFinished = false;
}

TArray<FString> DCStreamedSpeechPolicy::FChunker::Push(const FString& CumulativeText, bool bFinal)
{
    using namespace DCStreamedSpeechPolicyPrivate;
    TArray<FString> Chunks;
    if (bRejected || bFinished) { return Chunks; }
    if (!CumulativeText.StartsWith(PreviousRaw, ESearchCase::CaseSensitive))
    {
        bRejected = true;
        return Chunks;
    }
    PreviousRaw = CumulativeText;
    const FString Trimmed = CumulativeText.TrimStartAndEnd();
    if (EmittedText.IsEmpty() && MustDefer(Trimmed)) { bDeferred = true; }

    if (bFinal)
    {
        bFinished = true;
        const FString Clean = DCConversationPolicy::CleanReply(CumulativeText);
        if (Clean.IsEmpty() || HasRoleLabel(Clean) || (!bDeferred && HasRoleLabel(CumulativeText)) || !HasCompleteUnicode(Clean) ||
            !Clean.StartsWith(EmittedText, ESearchCase::CaseSensitive) ||
            (!EmittedText.IsEmpty() && Clean.Len() > EmittedText.Len() && !FChar::IsWhitespace(Clean[EmittedText.Len()])))
        {
            bRejected = true;
            return Chunks;
        }
        const FString Tail = Clean.Mid(EmittedText.Len()).TrimStartAndEnd();
        if (!Tail.IsEmpty()) { Chunks.Add(Tail); }
        EmittedText = Clean;
        return Chunks;
    }
    if (bDeferred || Trimmed.IsEmpty()) { return Chunks; }
    if (HasRoleLabel(CumulativeText) || DCConversationPolicy::CleanReply(CumulativeText).IsEmpty())
    {
        bRejected = true;
        return Chunks;
    }

    // Preserve a trailing space here: punctuation at the current last character
    // could still become a decimal, abbreviation, ellipsis, or joined token.
    FString Normalized = Normalize(CumulativeText);
    if (!CumulativeText.IsEmpty() && FChar::IsWhitespace(CumulativeText[CumulativeText.Len() - 1]))
    {
        Normalized += TEXT(" ");
    }
    if (!Normalized.StartsWith(EmittedText, ESearchCase::CaseSensitive))
    {
        bRejected = true;
        return Chunks;
    }

    for (;;)
    {
        int32 Start = EmittedText.Len();
        while (Start < Normalized.Len() && FChar::IsWhitespace(Normalized[Start])) { ++Start; }
        int32 Sentence = INDEX_NONE;
        int32 Clause = INDEX_NONE;
        for (int32 Index = Start; Index + 1 < Normalized.Len(); ++Index)
        {
            if (!FChar::IsWhitespace(Normalized[Index + 1])) { continue; }
            if (IsSentenceEnd(Normalized[Index])) { Sentence = Index; break; }
            if (Clause == INDEX_NONE && (Normalized[Index] == TEXT(',') || Normalized[Index] == TEXT(';')) &&
                WordCount(Normalized.Mid(Start, Index - Start + 1)) >= 6)
            {
                Clause = Index;
            }
        }
        const int32 End = Sentence != INDEX_NONE ? Sentence : Clause;
        if (End == INDEX_NONE) { break; }
        const FString Prefix = Normalized.Left(End + 1);
        // Inspect real source boundaries, then apply the existing whole-utterance
        // limits. Never search the cleaner's synthetic truncation period for a
        // streaming sentence boundary, or independently cap individual chunks.
        if (DCConversationPolicy::CleanReply(Prefix) != Prefix) { break; }
        if (!HasCompleteUnicode(Prefix)) { bRejected = true; break; }
        const FString Chunk = Normalized.Mid(Start, End - Start + 1);
        if (Chunk.IsEmpty()) { break; }
        Chunks.Add(Chunk);
        EmittedText = Prefix;
    }
    return Chunks;
}
