#include "DCConversationReview.h"
#include "DCConversationContext.h"
#include "DCConversationPolicy.h"
#include "DCVoice.h"
#include "DCCrew.h"
#include "DCRideDirector.h"
#include "ACEAudioCurveSourceComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace DCConversationReviewPrivate
{
    struct FQuestion { const TCHAR* Speaker; const TCHAR* Text; };
    const FQuestion Questions[] = {
        {TEXT("Rex"), TEXT("Rex, tell me more about yourself.")},
        {TEXT("Rex"), TEXT("Tell me more about that.")},
        {TEXT("Duncan"), TEXT("Duncan, tell me about Ruby and what she looks like.")},
        {TEXT("Ruby"), TEXT("Ruby, what do I look like?")},
        {TEXT("Ruby"), TEXT("Ruby, what are you and the others doing?")},
        {TEXT("Rex"), TEXT("Rex, tell me more about the fog.")},
        {TEXT("Duncan"), TEXT("Duncan, what do you see, and what is the weather like?")},
        {TEXT("Ruby"), TEXT("Ruby, tell me about our ship's condition and how we deal with fires.")}
    };
}

bool UDCConversationReview::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->WorldType==EWorldType::Game &&
        World->GetOutermost()->GetName()==TEXT("/Game/DC/Maps/L_Ride") &&
        FParse::Param(FCommandLine::Get(), TEXT("DCConversationReview")) && Super::ShouldCreateSubsystem(Outer);
}

void UDCConversationReview::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Started=StageStarted=FPlatformTime::Seconds();
    OutputPath=FPaths::ProjectSavedDir()/TEXT("ACEDialogue/conversation-review.json");
    FParse::Value(FCommandLine::Get(), TEXT("DCConversationReport="), OutputPath);
}

TStatId UDCConversationReview::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UDCConversationReview, STATGROUP_Tickables);
}

void UDCConversationReview::ObserveSpeech()
{
    using namespace DCConversationReviewPrivate;
    const double Now=FPlatformTime::Seconds();
    const FString Speaker=Questions[CaseIndex].Speaker;
    if (bCaseStreaming)
    {
        if (!ExpectedBatch)
        {
            // A question can initially wait behind an authored report. Never bind
            // to the previous question's retained finished-batch record.
            if (Voice->StreamingBatch.Id>BatchBefore && Voice->StreamingBatch.Speaker==Speaker)
            { ExpectedBatch=Voice->StreamingBatch.Id; }
            else if (Voice->LastFinishedBatch.Id>BatchBefore && Voice->LastFinishedBatch.Speaker==Speaker)
            { ExpectedBatch=Voice->LastFinishedBatch.Id; }
        }
        if (!ExpectedBatch) { return; }
        if (Voice->StreamingBatch.Id==ExpectedBatch && Voice->StreamingBatch.Queued>0)
        {
            if (!FirstChunkAt) { FirstChunkAt=Now; }
            bChunkBeforeGeneration|=Voice->bConversationPending;
        }
        bool bMatchingTTS=false;
        for (const auto& Pending:Voice->PendingTTS) { bMatchingTTS|=Pending.BatchId==ExpectedBatch; }
        if (bMatchingTTS && !FirstTTSAt) { FirstTTSAt=Now; }
        const bool bMatchingPlayback=Voice->ActiveSpeechBatchId==ExpectedBatch && Voice->ActiveSpeaker==Speaker &&
            Voice->ActiveCurves && Voice->bACEStarted && !Voice->bACEEnded;
        if (bMatchingPlayback)
        {
            bSawPlayback=true;
            if (!FirstSpeechAt) { FirstSpeechAt=Now; }
            bSpeechBeforeGeneration|=Voice->bConversationPending;
            bPlaybackBeforeSendCompleted|=!Voice->bACESent;
            bSawTTSLookahead|=bMatchingTTS;
            for (const auto& Ready:Voice->ReadySpeech) { bSawTTSLookahead|=Ready.BatchId==ExpectedBatch; }
        }
    }
    else
    {
        if (Voice->ActiveSpeaker==Speaker && Voice->ActiveText==Reply && !Reply.IsEmpty() && Voice->ActiveCurves && Voice->bACEStarted)
        {
            bSawPlayback=true;
            if (!FirstSpeechAt) { FirstSpeechAt=Now; }
        }
    }
}

void UDCConversationReview::Tick(float DeltaTime)
{
    using namespace DCConversationReviewPrivate;
    Super::Tick(DeltaTime);
    if (Stage==EStage::Done) { return; }
    const double Now=FPlatformTime::Seconds();
    if (Now-Started>240) { Finish(TEXT("Conversation review exceeded 240 seconds")); return; }
    if (!Voice) { Voice=ADCVoice::Get(this); }
    if (!Director) { Director=ADCRideDirector::Get(this); }
    if (Director) { Director->TimeScale=0; }
    if (Stage==EStage::Ready)
    {
        if (Now-Started>90) { Finish(TEXT("Model/cast did not become ready")); return; }
        if (!Voice || !Director || !Voice->IsLLMReady() || !Voice->IsTTSReady() || Voice->IsMenuOpen()) { return; }
        if (DCConversationContext::Build(this,TEXT("Rex")).Contains(TEXT("not confirmed"))) { return; }
        if (!Voice->bUseACE) { Finish(TEXT("Review requires live Audio2Face, not audio-only fallback")); return; }
        Voice->ResetPendingSpeech();
        Stage=EStage::Drain; StageStarted=Now;
        return;
    }
    if (!Voice || !Voice->bUseACE) { Finish(TEXT("Voice lost or Audio2Face disabled")); return; }
    if (Stage==EStage::Drain)
    {
        if (Now-StageStarted<1.0) { return; }
        Stage=EStage::Ask;
    }
    if (CaseIndex>=static_cast<int32>(UE_ARRAY_COUNT(Questions))) { Finish(FString()); return; }
    const FQuestion& Question=Questions[CaseIndex];
    UDCCrewSubsystem* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>();
    if (!Crew) { Finish(TEXT("Missing crew subsystem")); return; }
    if (Stage==EStage::Ask)
    {
        if (CaseIndex==5 && Director->GetPhase()!=EDCPhase::Fog)
        {
            Director->Seek(94.f); Director->Tick(0.f); Voice->SynchronizeScene();
            if (!Voice->ConversationHistory.IsEmpty()) { Finish(TEXT("Old scene conversational memory was retained")); return; }
            Voice->ResetPendingSpeech();
            Stage=EStage::Drain; StageStarted=Now;
            return;
        }
        OrdersBefore=Crew->OrdersGiven;
        Context=DCConversationContext::Build(this,Question.Speaker);
        Reply.Empty(); bSawPlayback=false;
        BatchBefore=Voice->NextSpeechBatchId; ExpectedBatch=0; CaseEpoch=Voice->SpeechEpoch;
        bCaseStreaming=Voice->bStreamSpeech;
        bChunkBeforeGeneration=bSpeechBeforeGeneration=bSawTTSLookahead=bPlaybackBeforeSendCompleted=false;
        FirstChunkAt=FirstTTSAt=FirstSpeechAt=0; QuestionStarted=Now;
        Stage=EStage::Generate; StageStarted=Now;
        Crew->ExecuteOrder(Question.Text); // Exact shared typed/microphone entry point.
        if (Crew->OrdersGiven!=OrdersBefore) { Finish(TEXT("Conversation entered command path; missing ExecuteOrder seam")); return; }
        ObserveSpeech();
        return;
    }
    if (Crew->OrdersGiven!=OrdersBefore) { Finish(TEXT("Conversation changed the order count")); return; }
    if (Voice->SpeechEpoch!=CaseEpoch) { Finish(TEXT("Review utterance was cancelled or changed scene before completion")); return; }
    ObserveSpeech();
    if (bCaseStreaming && ExpectedBatch)
    {
        if (ExpectedBatch!=BatchBefore+1) { Finish(TEXT("Unexpected competing speech batch during review")); return; }
        if ((Voice->StreamingBatch.Id==ExpectedBatch && Voice->StreamingBatch.bFailed) ||
            (Voice->LastFinishedBatch.Id==ExpectedBatch && Voice->LastFinishedBatch.bFailed))
        { Finish(TEXT("Streamed speech batch failed; fallback cannot pass")); return; }
    }
    if (Stage==EStage::Generate)
    {
        const auto* History=Voice->ConversationHistory.Find(Question.Speaker);
        if (History && !History->IsEmpty() && History->Last().User==Question.Text)
        {
            Reply=History->Last().Reply;
            GeneratedMs=(Now-StageStarted)*1000;
            if (Reply.IsEmpty() || Reply!=DCConversationPolicy::CleanReply(Reply)) { Finish(TEXT("Invalid generated spoken response")); return; }
            ObserveSpeech();
            Stage=EStage::Speak; StageStarted=Now;
        }
        else if (Now-StageStarted>15) { Finish(TEXT("No genuine model response; fallback lines cannot pass")); }
        return;
    }
    if (Stage==EStage::Speak)
    {
        if (Now-StageStarted>40) { Finish(TEXT("Generated conversation did not finish speech/lip-sync")); return; }
        bool bCompleted=false;
        if (bCaseStreaming)
        {
            if (ExpectedBatch && Voice->LastFinishedBatch.Id==ExpectedBatch)
            {
                const auto& Batch=Voice->LastFinishedBatch;
                if (Batch.Speaker!=Question.Speaker || Batch.Text!=Reply || !Batch.bFinalized || Batch.bFailed ||
                    Batch.Queued<=0 || Batch.Completed!=Batch.Queued || Batch.Faces!=Batch.Queued)
                { Finish(TEXT("Completed batch identity/text/chunk or face counts do not match the generated utterance")); return; }
                bCompleted=true;
            }
        }
        else
        {
            const bool bMatching=Voice->ActiveSpeaker==Question.Speaker && Voice->ActiveText==Reply;
            bCompleted=bMatching && bSawPlayback && Voice->bACESent && Voice->bACEEnded && !Voice->ActiveCurves;
        }
        if (bCompleted)
        {
            TSharedRef<FJsonObject> Row=MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("speaker"),Question.Speaker); Row->SetStringField(TEXT("question"),Question.Text);
            Row->SetStringField(TEXT("answer"),Reply); Row->SetStringField(TEXT("live_context"),Context);
            Row->SetNumberField(TEXT("generation_ms"),GeneratedMs); Row->SetBoolField(TEXT("orders_unchanged"),true);
            Row->SetBoolField(TEXT("a2f_started_and_finished"),true);
            Row->SetBoolField(TEXT("streaming_enabled"),bCaseStreaming);
            Row->SetNumberField(TEXT("speech_batch_id"),ExpectedBatch);
            if (bCaseStreaming)
            {
                Row->SetNumberField(TEXT("chunks_queued"),Voice->LastFinishedBatch.Queued);
                Row->SetNumberField(TEXT("chunks_completed"),Voice->LastFinishedBatch.Completed);
                Row->SetNumberField(TEXT("chunks_with_faces"),Voice->LastFinishedBatch.Faces);
                Row->SetBoolField(TEXT("batch_finalized"),Voice->LastFinishedBatch.bFinalized);
                Row->SetBoolField(TEXT("batch_failed"),Voice->LastFinishedBatch.bFailed);
            }
            auto ObservedTime=[&](const TCHAR* Key,double Time)
            {
                if (Time>0) { Row->SetNumberField(Key,(Time-QuestionStarted)*1000.0); }
                else { Row->SetField(Key,MakeShared<FJsonValueNull>()); }
            };
            ObservedTime(TEXT("first_chunk_observed_ms"),FirstChunkAt);
            ObservedTime(TEXT("first_tts_observed_ms"),FirstTTSAt);
            ObservedTime(TEXT("time_to_speech_observed_ms"),FirstSpeechAt);
            Row->SetBoolField(TEXT("playback_observed"),bSawPlayback);
            Row->SetBoolField(TEXT("chunk_before_generation_complete_observed"),bChunkBeforeGeneration);
            Row->SetBoolField(TEXT("speech_before_generation_complete_observed"),bSpeechBeforeGeneration);
            Row->SetBoolField(TEXT("tts_lookahead_during_playback_observed"),bSawTTSLookahead);
            Row->SetBoolField(TEXT("a2f_playback_before_send_completed_observed"),bPlaybackBeforeSendCompleted);
            Row->SetStringField(TEXT("timing_scope"),TEXT("First state observed on a review tick, measured from question submission; missing observations are null. Overlap flags are observations, not required quality checks."));
            Results.Add(MakeShared<FJsonValueObject>(Row));
            UE_LOG(LogDC,Display,TEXT("CONVERSATION_REVIEW_CASE index=%d speaker=%s orders_unchanged=1 a2f=1 answer=%s"),CaseIndex,Question.Speaker,*Reply);
            ++CaseIndex; Stage=EStage::Ask;
        }
    }
}

void UDCConversationReview::Finish(const FString& Error)
{
    if (Stage==EStage::Done) { return; }
    Stage=EStage::Done;
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("success"),Error.IsEmpty()); Report->SetStringField(TEXT("error"),Error);
    Report->SetStringField(TEXT("backend"),Voice ? Voice->DialogueBackend : TEXT("unknown"));
    Report->SetStringField(TEXT("scope"),TEXT("Real conversation entry, no order execution, generated speech/A2F and scene-memory reset. Streaming validates matching finalized batch text and every chunk's face completion; stable mode validates the whole line. Positive sample logs and answer/context semantic review remain required."));
    Report->SetArrayField(TEXT("results"),Results);
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath),true);
    const bool bSaved=FFileHelper::SaveStringToFile(Json,*OutputPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const bool bSuccess=Error.IsEmpty() && bSaved;
    UE_LOG(LogDC,Display,TEXT("CONVERSATION_REVIEW_%s cases=%d error=%s report=%s"),bSuccess?TEXT("SUCCESS"):TEXT("FAILED"),CaseIndex,*Error,*OutputPath);
    FPlatformMisc::RequestExitWithStatus(false,bSuccess?0:1);
}
