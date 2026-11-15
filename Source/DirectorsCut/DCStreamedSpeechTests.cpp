#include "DCStreamedSpeechPolicy.h"
#include "DCConversationPolicy.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStreamedSpeechChunks, "DirectorsCut.Dialogue.StreamedSpeechChunks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStreamedSpeechChunks::RunTest(const FString&)
{
    using DCStreamedSpeechPolicy::FChunker;
    FChunker Chunker;
    TestEqual(TEXT("Partial word withheld"), Chunker.Push(TEXT("Mind the rigg")).Num(), 0);
    TestEqual(TEXT("Terminal punctuation needs confirmation"), Chunker.Push(TEXT("Mind the rigging.")).Num(), 0);
    TArray<FString> Chunks = Chunker.Push(TEXT("Mind the rigging. "));
    TestEqual(TEXT("Confirmed sentence emitted"), FString::Join(Chunks, TEXT(" ")), FString(TEXT("Mind the rigging.")));
    TestEqual(TEXT("Duplicate snapshot emits nothing"), Chunker.Push(TEXT("Mind the rigging. ")).Num(), 0);
    TestEqual(TEXT("Partial second sentence withheld"), Chunker.Push(TEXT("Mind the rigging. Keep a")).Num(), 0);
    Chunks = Chunker.Push(TEXT("Mind the rigging. Keep a weather eye."), true);
    TestEqual(TEXT("Only final unsaid suffix emitted"), FString::Join(Chunks, TEXT(" ")), FString(TEXT("Keep a weather eye.")));
    TestEqual(TEXT("Repeated completion is idempotent"), Chunker.Push(TEXT("Mind the rigging. Keep a weather eye."), true).Num(), 0);
    TestFalse(TEXT("Valid stream accepted"), Chunker.WasRejected());

    Chunker.Reset();
    TestEqual(TEXT("Short comma clause withheld"), Chunker.Push(TEXT("Aye, Captain, ")).Num(), 0);
    Chunks = Chunker.Push(TEXT("Aye, Captain, I have the wheel, "));
    TestEqual(TEXT("Substantial comma clause emitted"), FString::Join(Chunks, TEXT(" ")), FString(TEXT("Aye, Captain, I have the wheel,")));
    Chunks = Chunker.Push(TEXT("Aye, Captain, I have the wheel, and she answers well."), true);
    TestEqual(TEXT("Comma clause not replayed"), FString::Join(Chunks, TEXT(" ")), FString(TEXT("and she answers well.")));

    Chunker.Reset();
    Chunks = Chunker.Push(TEXT("I have sailed these waters for years; "));
    TestEqual(TEXT("Substantial semicolon clause emitted"), Chunks.Num(), 1);
    Chunker.Reset();
    Chunks = Chunker.Push(TEXT("I have sailed these waters for years, and know the shoals. "));
    TestEqual(TEXT("Available sentence preferred to its clause"), Chunks.Num(), 1);
    TestEqual(TEXT("Sentence includes clause"), FString::Join(Chunks, TEXT(" ")),
        FString(TEXT("I have sailed these waters for years, and know the shoals.")));

    Chunker.Reset();
    TestEqual(TEXT("Decimal period held"), Chunker.Push(TEXT("We have 3.")).Num(), 0);
    TestEqual(TEXT("Decimal continuation held"), Chunker.Push(TEXT("We have 3.5 ")).Num(), 0);
    TestEqual(TEXT("Decimal sentence complete"), Chunker.Push(TEXT("We have 3.5 fathoms. ")).Num(), 1);

    Chunker.Reset();
    Chunks = Chunker.Push(TEXT("  Aye,\tCaptain.\n\n Keep the\tship steady."), true);
    TestEqual(TEXT("Whitespace normalized"), FString::Join(Chunks, TEXT(" ")),
        FString(TEXT("Aye, Captain. Keep the ship steady.")));

    // Every TCHAR split, including whitespace and punctuation boundaries, must
    // produce exactly the cleaner's answer with neither repeated nor lost text.
    const FString Samples[] = {
        TEXT("Aye, Captain. I have the wheel."),
        TEXT("I have sailed these waters for years, and know the shoals."),
        TEXT("I mind the sails; Duncan minds the guns."),
        FString(UTF8_TO_TCHAR("Aye, Capitaine. Le café est prêt.")),
        TEXT("  Aye,\tCaptain.\nKeep the ship steady.")
    };
    for (const FString& Sample : Samples)
    {
        Chunker.Reset();
        TArray<FString> Spoken;
        for (int32 Length = 0; Length <= Sample.Len(); ++Length)
        {
            Spoken.Append(Chunker.Push(Sample.Left(Length)));
        }
        Spoken.Append(Chunker.Push(Sample, true));
        const FString Expected = DCConversationPolicy::CleanReply(Sample);
        TestEqual(TEXT("Character-stream reconstruction"), FString::Join(Spoken, TEXT(" ")), Expected);
        TestEqual(TEXT("Committed text equals reconstruction"), Chunker.GetEmittedText(), Expected);
        TestFalse(TEXT("Character-stream accepted"), Chunker.WasRejected());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStreamedSpeechValidation, "DirectorsCut.Dialogue.StreamedSpeechValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStreamedSpeechValidation::RunTest(const FString&)
{
    using DCStreamedSpeechPolicy::FChunker;
    FChunker Chunker;
    const FString Wrapped[] = {
        TEXT("<think>private reasoning. Do not speak.</think>Aye, Captain."),
        TEXT("```text\nAye, Captain.\n```"),
        TEXT("\"Aye, Captain.\"")
    };
    for (const FString& Raw : Wrapped)
    {
        Chunker.Reset();
        for (int32 Length = 0; Length <= Raw.Len(); ++Length)
        {
            TestEqual(TEXT("Wrapped text wholly deferred"), Chunker.Push(Raw.Left(Length)).Num(), 0);
        }
        TestEqual(TEXT("Validated wrapper only emits clean speech"), FString::Join(Chunker.Push(Raw, true), TEXT(" ")),
            FString(TEXT("Aye, Captain.")));
        TestFalse(TEXT("Valid wrapper accepted"), Chunker.WasRejected());
    }
    const FString Invalid[] = {
        TEXT("{\"content\":\"Aye.\"}"), TEXT("[\"Aye.\"]"), TEXT("assistant: Aye, Captain."),
        TEXT("Ruby: Aye, Captain."), TEXT("Analysis: We should answer."), TEXT("<think>unfinished"),
        TEXT("```json\n{\"content\":\"Aye\"}\n```"), TEXT("\"assistant: Aye.\""),
        TEXT("Aye. assistant: Answer again."), TEXT("Aye.\nUSER: Speak again."),
        TEXT("Aye. <think>private</think>"), TEXT("# Response\nAye."), TEXT("(smiles) Aye.")
    };
    for (const FString& Raw : Invalid)
    {
        Chunker.Reset();
        TestEqual(TEXT("Invalid full snapshot emits nothing"), Chunker.Push(Raw).Num(), 0);
        TestEqual(TEXT("Invalid final emits nothing"), Chunker.Push(Raw, true).Num(), 0);
        TestTrue(TEXT("Invalid metadata rejected"), Chunker.WasRejected());
    }
    Chunker.Reset();
    Chunker.Push(TEXT("Aye, Captain. "));
    TestEqual(TEXT("Late metadata stopped"), Chunker.Push(TEXT("Aye, Captain. <think>private"), true).Num(), 0);
    TestTrue(TEXT("Late metadata marks rejection"), Chunker.WasRejected());
    TestEqual(TEXT("Already spoken prefix retained"), Chunker.GetEmittedText(), FString(TEXT("Aye, Captain.")));

    Chunker.Reset();
    Chunker.Push(TEXT("Aye, Captain. "));
    TestEqual(TEXT("Rewritten body rejected"), Chunker.Push(TEXT("No, Captain. ")).Num(), 0);
    TestTrue(TEXT("Nonmonotonic failure sticky"), Chunker.WasRejected());
    TestEqual(TEXT("Cannot resurrect rejected stream"), Chunker.Push(TEXT("Aye, Captain. "), true).Num(), 0);
    Chunker.Reset();
    Chunker.Push(TEXT("Aye, Captain"));
    Chunker.Push(TEXT("Aye"));
    TestTrue(TEXT("Shortened unfinished body rejected"), Chunker.WasRejected());
    Chunker.Reset();
    TestFalse(TEXT("Reset clears rejection"), Chunker.WasRejected());
    TestTrue(TEXT("Reset clears emitted prefix"), Chunker.GetEmittedText().IsEmpty());

    // A trailing high surrogate cannot be spoken at completion. Appending its
    // low surrogate before the real sentence boundary is valid.
    FString Unicode = TEXT("The flag ");
    Unicode.AppendChar(static_cast<TCHAR>(0xD83D));
    TestEqual(TEXT("Split surrogate withheld"), Chunker.Push(Unicode).Num(), 0);
    Unicode.AppendChar(static_cast<TCHAR>(0xDEA9));
    Unicode += TEXT(" flies. ");
    TestEqual(TEXT("Complete surrogate pair emitted"), Chunker.Push(Unicode).Num(), 1);
    Chunker.Reset();
    FString Broken = TEXT("The flag "); Broken.AppendChar(static_cast<TCHAR>(0xD83D));
    TestEqual(TEXT("Broken final Unicode withheld"), Chunker.Push(Broken, true).Num(), 0);
    TestTrue(TEXT("Broken final Unicode rejected"), Chunker.WasRejected());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStreamedSpeechLimits, "DirectorsCut.Dialogue.StreamedSpeechLimits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStreamedSpeechLimits::RunTest(const FString&)
{
    using DCStreamedSpeechPolicy::FChunker;
    FChunker Chunker;
    FString Long;
    for (int32 Index = 0; Index < 50; ++Index) { Long += TEXT("aye "); }
    TestEqual(TEXT("Cleaner truncation period is not a source boundary"), Chunker.Push(Long).Num(), 0);
    TArray<FString> Chunks = Chunker.Push(Long, true);
    TestEqual(TEXT("Final truncation matches existing policy"), FString::Join(Chunks, TEXT(" ")), DCConversationPolicy::CleanReply(Long));
    TArray<FString> Words; Chunker.GetEmittedText().ParseIntoArrayWS(Words);
    TestEqual(TEXT("Whole utterance forty-word cap"), Words.Num(), 40);
    TestTrue(TEXT("Whole utterance character cap"), Chunker.GetEmittedText().Len() <= 220);

    Chunker.Reset();
    FString Raw = TEXT("Mind the rigging. ");
    Chunks = Chunker.Push(Raw);
    for (int32 Index = 0; Index < 40; ++Index) { Raw += TEXT("rigging "); }
    TestEqual(TEXT("Long incomplete suffix never falsely completed"), Chunker.Push(Raw).Num(), 0);
    Chunks.Append(Chunker.Push(Raw, true));
    TestEqual(TEXT("Final cap applies across previous chunks"), FString::Join(Chunks, TEXT(" ")), DCConversationPolicy::CleanReply(Raw));
    TestTrue(TEXT("Combined character cap"), Chunker.GetEmittedText().Len() <= 220);

    Chunker.Reset();
    Raw = TEXT("I mind the wheel. Duncan minds the guns! Ruby tends the sails. ");
    Chunks = Chunker.Push(Raw);
    Chunks.Append(Chunker.Push(Raw, true));
    TestEqual(TEXT("Only two source sentences committed"), Chunks.Num(), 2);
    TestEqual(TEXT("Third sentence discarded without replay"), FString::Join(Chunks, TEXT(" ")),
        FString(TEXT("I mind the wheel. Duncan minds the guns!")));
    TestFalse(TEXT("Normal cap is not rejection"), Chunker.WasRejected());
    return true;
}
#endif
