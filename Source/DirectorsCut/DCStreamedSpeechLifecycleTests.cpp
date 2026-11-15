#include "DCVoice.h"
#include "DCACESpeechCallback.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCStreamedSpeechLifecycleTest, "DirectorsCut.Dialogue.StreamedSpeechLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCStreamedSpeechLifecycleTest::RunTest(const FString&)
{
    // No world initialization, BeginPlay, ticking, audio devices or child
    // processes. Exercise the actual actor's bookkeeping directly.
    UWorld::InitializationValues Values;
    Values.InitializeScenes(false).AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Values, true);
    if (!TestNotNull(TEXT("Bookkeeping world created"), World)) { return false; }
    ADCVoice* Voice = World->SpawnActor<ADCVoice>();
    if (!TestNotNull(TEXT("Voice actor created without BeginPlay"), Voice))
    {
        World->DestroyWorld(false);
        return false;
    }
    TestFalse(TEXT("Actor never began play"), Voice->HasActorBegunPlay());
    Voice->bStreamSpeech = true;

    // Generation can finish before either audio chunk. Retiring the first
    // chunk must not release the batch or make the next conversation eligible.
    int32 Batch = Voice->BeginSpeechBatch(TEXT("Rex"));
    Voice->QueueSpeechChunk(Batch, TEXT("Aye, Captain."));
    Voice->QueueSpeechChunk(Batch, TEXT("I have the wheel."));
    Voice->SpeechQueue.Empty(); // simulate handing both chunks to playback
    Voice->FinalizeSpeechBatch(Batch, TEXT("Aye, Captain. I have the wheel."), true);
    TestEqual(TEXT("Finalization waits for playback"), Voice->StreamingBatch.Id, Batch);
    Voice->CompleteSpeechChunk(Batch, true, true);
    TestEqual(TEXT("First completion retains batch"), Voice->StreamingBatch.Completed, 1);
    Voice->CompleteSpeechChunk(Batch, true, true);
    TestEqual(TEXT("Last completion releases batch"), Voice->StreamingBatch.Id, 0);
    TestEqual(TEXT("Finished counts both chunks"), Voice->LastFinishedBatch.Completed, 2);
    TestEqual(TEXT("Finished counts both animated faces"), Voice->LastFinishedBatch.Faces, 2);
    TestFalse(TEXT("Success remains successful"), Voice->LastFinishedBatch.bFailed);
    Voice->CompleteSpeechChunk(Batch, false, false);
    TestFalse(TEXT("Late duplicate cannot fail finished batch"), Voice->LastFinishedBatch.bFailed);

    // Conversely, very short speech can finish while the model is still
    // generating. Keep the open batch until the final model callback arrives.
    Batch = Voice->BeginSpeechBatch(TEXT("Ruby"));
    Voice->QueueSpeechChunk(Batch, TEXT("Aye.")); Voice->SpeechQueue.Empty();
    Voice->CompleteSpeechChunk(Batch, true, true);
    TestEqual(TEXT("Playback before finalization retains open batch"), Voice->StreamingBatch.Id, Batch);
    Voice->FinalizeSpeechBatch(Batch, TEXT("Aye."), true);
    TestEqual(TEXT("Model completion releases already-played batch"), Voice->StreamingBatch.Id, 0);

    // One active, one ready, two queued. Failed generation discards only the
    // three unplayed continuations, while a safe active chunk can finish.
    Batch = Voice->BeginSpeechBatch(TEXT("Duncan"));
    for (int32 I = 0; I < 4; ++I) { Voice->QueueSpeechChunk(Batch, TEXT("Mind the guns.")); }
    Voice->SpeechQueue.RemoveAt(0, 2);
    Voice->ReadySpeech.Add({TEXT("Duncan"), TEXT("Ready continuation."), TEXT("unused.wav"), 1.0, Voice->SpeechEpoch, Batch});
    Voice->SpeechQueue.Add({TEXT("Ruby"), TEXT("The rigging is sound."), 0.0, false, 0});
    Voice->FinalizeSpeechBatch(Batch, FString(), false);
    TestEqual(TEXT("Failure accounts for discarded queued and ready chunks"), Voice->StreamingBatch.Completed, 3);
    TestEqual(TEXT("Failure retains active chunk until completion"), Voice->StreamingBatch.Id, Batch);
    TestTrue(TEXT("Failure clears ready continuation"), Voice->ReadySpeech.IsEmpty());
    TestEqual(TEXT("Unrelated speech survives failure"), Voice->SpeechQueue.Num(), 1);
    TestEqual(TEXT("Unrelated line has no failed batch id"), Voice->SpeechQueue[0].BatchId, 0);
    Voice->QueueSpeechChunk(Batch, TEXT("Must not append after finalization."));
    TestEqual(TEXT("Failed batch rejects late chunks"), Voice->StreamingBatch.Queued, 4);
    Voice->FinalizeSpeechBatch(Batch, FString(), false);
    TestEqual(TEXT("Repeated finalization does not recount discarded chunks"), Voice->StreamingBatch.Completed, 3);
    Voice->CompleteSpeechChunk(Batch, true, true);
    TestEqual(TEXT("Failure eventually drains"), Voice->StreamingBatch.Id, 0);
    TestEqual(TEXT("Failure completes all four exactly once"), Voice->LastFinishedBatch.Completed, 4);
    TestTrue(TEXT("Successful active tail cannot erase failure"), Voice->LastFinishedBatch.bFailed);
    Voice->ResetPendingSpeech();

    // History may already contain the response when a later spoken chunk fails.
    const FString Question = TEXT("Who minds the wheel?");
    const FString Reply = TEXT("I mind the wheel. Duncan minds the guns.");
    Batch = Voice->BeginSpeechBatch(TEXT("Rex"));
    Voice->StreamingBatch.User = Question;
    Voice->QueueSpeechChunk(Batch, TEXT("I mind the wheel."));
    Voice->QueueSpeechChunk(Batch, TEXT("Duncan minds the guns."));
    Voice->SpeechQueue.Empty();
    Voice->ConversationHistory.FindOrAdd(TEXT("Rex")).Add({TEXT("Earlier question"), TEXT("Earlier answer.")});
    Voice->ConversationHistory.FindOrAdd(TEXT("Rex")).Add({Question, Reply});
    Voice->ConversationHistory.FindOrAdd(TEXT("Ruby")).Add({Question, Reply});
    Voice->FinalizeSpeechBatch(Batch, Reply, true);
    Voice->CompleteSpeechChunk(Batch, true, true);
    Voice->CompleteSpeechChunk(Batch, false, false);
    TestEqual(TEXT("Failed reply removed from its speaker history"), Voice->ConversationHistory.FindChecked(TEXT("Rex")).Num(), 1);
    TestEqual(TEXT("Older conversation retained"), Voice->ConversationHistory.FindChecked(TEXT("Rex"))[0].Reply, FString(TEXT("Earlier answer.")));
    TestEqual(TEXT("Other speaker history untouched"), Voice->ConversationHistory.FindChecked(TEXT("Ruby")).Num(), 1);

    Batch = Voice->BeginSpeechBatch(TEXT("Rex"));
    Voice->StreamingBatch.User = Question;
    Voice->QueueSpeechChunk(Batch, TEXT("Aye.")); Voice->SpeechQueue.Empty();
    Voice->ConversationHistory.FindOrAdd(TEXT("Rex")).Add({Question, TEXT("Different answer.")});
    Voice->FinalizeSpeechBatch(Batch, Reply, true);
    Voice->CompleteSpeechChunk(Batch, false, false);
    TestEqual(TEXT("Same question with different answer not removed"), Voice->ConversationHistory.FindChecked(TEXT("Rex")).Last().Reply,
        FString(TEXT("Different answer.")));

    // Reset invalidates batch ids and callback generations without touching any
    // process or audio instance in this test.
    Batch = Voice->BeginSpeechBatch(TEXT("Ruby"));
    Voice->QueueSpeechChunk(Batch, TEXT("Mind the rigging."));
    UDCACESpeechCallback* OldRelay = NewObject<UDCACESpeechCallback>(Voice);
    OldRelay->Voice = Voice; OldRelay->Epoch = Voice->SpeechEpoch; OldRelay->Serial = ++Voice->ACERequestSerial;
    Voice->ActiveACERelay = OldRelay;
    const int32 OldEpoch = Voice->SpeechEpoch;
    Voice->ResetPendingSpeech();
    TestTrue(TEXT("Reset advances speech epoch"), Voice->SpeechEpoch > OldEpoch);
    TestTrue(TEXT("Reset empties queued and ready speech"), Voice->SpeechQueue.IsEmpty() && Voice->ReadySpeech.IsEmpty());
    TestTrue(TEXT("Reset empties history"), Voice->ConversationHistory.IsEmpty());
    TestEqual(TEXT("Reset clears active batch"), Voice->StreamingBatch.Id, 0);
    const int32 NextBatch = Voice->BeginSpeechBatch(TEXT("Rex"));
    Voice->QueueSpeechChunk(NextBatch, TEXT("A fresh answer."));
    Voice->CompleteSpeechChunk(Batch, false, false);
    Voice->FinalizeSpeechBatch(Batch, FString(), false);
    TestEqual(TEXT("Stale batch completion cannot retire fresh batch"), Voice->StreamingBatch.Id, NextBatch);
    TestEqual(TEXT("Stale completion does not alter fresh count"), Voice->StreamingBatch.Completed, 0);
    Voice->bUseACE = true; Voice->bACESent = false;
    OldRelay->Sent(false);
    TestTrue(TEXT("Stale moved callback cannot disable ACE"), Voice->bUseACE);
    TestFalse(TEXT("Stale moved callback cannot mark current send complete"), Voice->bACESent);

    // Consecutive chunks share an epoch. Request identity/serial must also
    // reject a previous chunk's moved delegate, without rejecting the live one.
    UDCACESpeechCallback* PriorChunkRelay = NewObject<UDCACESpeechCallback>(Voice);
    PriorChunkRelay->Voice = Voice;
    PriorChunkRelay->Epoch = Voice->SpeechEpoch;
    PriorChunkRelay->Serial = ++Voice->ACERequestSerial;
    Voice->ActiveACERelay = PriorChunkRelay;
    UDCACESpeechCallback* CurrentRelay = NewObject<UDCACESpeechCallback>(Voice);
    CurrentRelay->Voice = Voice;
    CurrentRelay->Epoch = Voice->SpeechEpoch;
    CurrentRelay->Serial = ++Voice->ACERequestSerial;
    Voice->ActiveACERelay = CurrentRelay;
    TestEqual(TEXT("Consecutive request callbacks share scene epoch"), PriorChunkRelay->Epoch, CurrentRelay->Epoch);
    PriorChunkRelay->Sent(false);
    TestTrue(TEXT("Same-epoch stale relay cannot disable ACE"), Voice->bUseACE);
    TestFalse(TEXT("Same-epoch stale relay cannot complete live send"), Voice->bACESent);
    CurrentRelay->Sent(true);
    TestTrue(TEXT("Live relay completes current send"), Voice->bACESent);
    TestTrue(TEXT("Live successful send preserves ACE"), Voice->bUseACE);
    Voice->ResetPendingSpeech();

    Batch = Voice->BeginSpeechBatch(TEXT("Rex"));
    Voice->FinalizeSpeechBatch(Batch, FString(), false);
    TestEqual(TEXT("Empty failed batch cannot deadlock queue"), Voice->StreamingBatch.Id, 0);
    TestTrue(TEXT("No processes created by lifecycle tests"), Voice->PendingTTS.IsEmpty());
    TestTrue(TEXT("No audio components created by lifecycle tests"), Voice->Playing.IsEmpty());
    TestFalse(TEXT("Voice remains without BeginPlay"), Voice->HasActorBegunPlay());
    Voice->ResetPendingSpeech();
    World->DestroyWorld(false);
    return true;
}
#endif
