#include "DCACEIntegrationReview.h"
#include "DCACEDialogue.h"
#include "DCCrew.h"
#include "DCRideDirector.h"
#include "DCVoice.h"
#include "ACEAudioCurveSourceComponent.h"
#include "Components/ChildActorComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    struct FCombinedReview
    {
        TArray<TSharedPtr<FJsonValue>> Rows;
        TWeakObjectPtr<ADCVoice> PreviousVoice;
        FString Output;
        double Started=0;
        int32 Worlds=0, Lines=0, Failures=0;
        bool bRestartRequested=false, bSceneCancelled=false, bRestartVerified=false;
        bool bLateSceneCallback=false, bLateRestartCallback=false, bFinished=false;
    };
    TSharedPtr<FCombinedReview> CombinedRun;
    const TCHAR* Speakers[] = {TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")};
    const TCHAR* Orders[] = {TEXT("Rex, hard to starboard"),TEXT("Duncan, man the port guns"),TEXT("Ruby, repair the rigging")};
    const TCHAR* Doing[] = {TEXT("Hard to starboard, aye!"),TEXT("Port guns manned and ready."),TEXT("Aloft and mending, aye.")};
}

bool UDCACEIntegrationReview::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World=Cast<UWorld>(Outer);
    return World && World->WorldType==EWorldType::Game && FParse::Param(FCommandLine::Get(),TEXT("DCACEIntegrationTest")) && Super::ShouldCreateSubsystem(Outer);
}

void UDCACEIntegrationReview::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    WorldStarted=StageStarted=FPlatformTime::Seconds();
    if (!CombinedRun)
    {
        CombinedRun=MakeShared<FCombinedReview>(); CombinedRun->Started=WorldStarted;
        CombinedRun->Output=FPaths::ProjectSavedDir()/TEXT("ACEDialogue/ACE-integration.json");
        FParse::Value(FCommandLine::Get(),TEXT("DCACEIntegrationReport="),CombinedRun->Output);
    }
    WorldCycle=++CombinedRun->Worlds;
    UE_LOG(LogDC,Display,TEXT("ACE_INTEGRATION_WORLD cycle=%d"),WorldCycle);
}

void UDCACEIntegrationReview::Deinitialize()
{
    bEnded=true;
    UE_LOG(LogDC,Display,TEXT("ACE_INTEGRATION_WORLD_END cycle=%d pending_restart=%d"),WorldCycle,CombinedRun.IsValid() && CombinedRun->bRestartRequested);
    Super::Deinitialize();
}

TStatId UDCACEIntegrationReview::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UDCACEIntegrationReview,STATGROUP_Tickables);
}

void UDCACEIntegrationReview::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bEnded || Stage==EStage::Done || !CombinedRun || CombinedRun->bFinished) { return; }
    const double Now=FPlatformTime::Seconds();
    if (Now-CombinedRun->Started>240.0) { Fail(TEXT("Combined test exceeded 240 seconds")); return; }
    if (CombinedRun->bLateSceneCallback || CombinedRun->bLateRestartCallback) { Fail(TEXT("Cancelled old-scene/world model callback was delivered")); return; }
    if (!Voice) { Voice=ADCVoice::Get(this); }
    if (!Director) { Director=ADCRideDirector::Get(this); }
    // Hold once through TimeScale, never by repeatedly seeking backwards through speech epochs.
    if (Director && !bHeldRide) { Director->TimeScale=0.f; bHeldRide=true; }
    if (Stage==EStage::Ready)
    {
        if (Now-WorldStarted>90.0) { Fail(TEXT("Real voyage, three crew and selected ACE model did not become ready within 90 seconds")); return; }
        if (!Voice || !Director || !Voice->IsLLMReady() || !Voice->IsTTSReady()) { return; }
        if (!Voice->bUseACEDialogue || !Voice->bUseACE || Voice->bSmokeTest || Voice->IsMenuOpen()) { Fail(TEXT("Requires ACE dialogue + ACE lip-sync, active voyage and no DCVoiceSmokeTest")); return; }
        const UDCCrewSubsystem* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>();
        int32 Bound=0;
        if (Crew)
        {
            for (ADCCrewMember* Member:Crew->GetCrew())
            {
                if (!Member) { continue; }
                TInlineComponentArray<UChildActorComponent*> Children(Member);
                for (UChildActorComponent* Child:Children)
                {
                    if (Child->GetChildActor() && Child->GetChildActor()->FindComponentByClass<UACEAudioCurveSourceComponent>()) { ++Bound; break; }
                }
            }
        }
        if (Bound!=3) { return; }
        if (WorldCycle>1)
        {
            ADCVoice* Old=CombinedRun->PreviousVoice.Get();
            CombinedRun->bRestartVerified=CombinedRun->bRestartRequested && Old!=Voice && (!Old || Old->bEnding);
            if (!CombinedRun->bRestartVerified) { Fail(TEXT("Reload did not retire the old voice actor")); return; }
            UE_LOG(LogDC,Display,TEXT("ACE_INTEGRATION_RESTART old_voice_ended=1 new_model_ready=1"));
        }
        Voice->ResetPendingSpeech();
        Stage=EStage::Drain; StageStarted=Now;
        return;
    }
    if (!Voice || !Voice->bUseACE) { Fail(TEXT("Voice lost or Audio2Face fell back/failed")); return; }
    if (Stage==EStage::Drain || Stage==EStage::SceneDrain)
    {
        if (Now-StageStarted>20.0) { Fail(TEXT("Cancelled model inference did not drain within 20 seconds")); return; }
        if (Voice->ACEDialogue && Voice->ACEDialogue->IsBusy()) { return; }
        if (Now-StageStarted<.5) { return; }
        if (Stage==EStage::SceneDrain)
        {
            CombinedRun->bSceneCancelled=bSceneSpeechCleared && Voice->SpeechEpoch>PreviousEpoch && !CombinedRun->bLateSceneCallback;
            if (!CombinedRun->bSceneCancelled) { Fail(TEXT("Scene transition retained old speech or failed to invalidate its epoch")); return; }
            UE_LOG(LogDC,Display,TEXT("ACE_INTEGRATION_SCENE_CANCEL epoch_changed=1 model_drained=1 late_callback=0 speech_empty=1"));
            Stage=EStage::Restart;
        }
        else { Stage=EStage::Generate; }
        StageStarted=Now;
        return;
    }
    if (Stage==EStage::Generate) { Generate(); return; }
    if (Stage==EStage::Generating)
    {
        if (Now-StageStarted>3.0) { Fail(TEXT("Generated acknowledgement callback did not meet its bounded deadline")); }
        return;
    }
    if (Stage==EStage::Speaking)
    {
        if (Now-StageStarted>40.0) { Fail(TEXT("Generated line did not complete Piper and Audio2Face within 40 seconds")); return; }
        const bool bThisLine=Voice->ActiveSpeaker==Speakers[CrewIndex] && Voice->ActiveText==GeneratedLine;
        if (bThisLine && Voice->ActiveCurves && Voice->bACEStarted) { bSawACEPlayback=true; }
        if (bThisLine && bSawACEPlayback && Voice->bACESent && Voice->bACEEnded && !Voice->ActiveCurves)
        {
            const int64 WavBytes=IFileManager::Get().FileSize(*Voice->ActiveWav);
            if (WavBytes<=44) { Fail(TEXT("Audio2Face completed without a valid synthesized WAV")); return; }
            TSharedRef<FJsonObject> Row=MakeShared<FJsonObject>();
            Row->SetNumberField(TEXT("world_cycle"),WorldCycle); Row->SetStringField(TEXT("speaker"),Speakers[CrewIndex]);
            Row->SetStringField(TEXT("generated_text"),GeneratedLine); Row->SetNumberField(TEXT("generation_ms"),GeneratedMilliseconds);
            Row->SetNumberField(TEXT("piper_wav_bytes"),double(WavBytes)); Row->SetBoolField(TEXT("a2f_started"),true);
            Row->SetBoolField(TEXT("a2f_send_completed"),true); Row->SetBoolField(TEXT("a2f_finished"),true);
            Row->SetNumberField(TEXT("speech_pipeline_ms"),(Now-StageStarted)*1000.0);
            CombinedRun->Rows.Add(MakeShared<FJsonValueObject>(Row)); ++CombinedRun->Lines;
            UE_LOG(LogDC,Display,TEXT("ACE_INTEGRATION_LINE cycle=%d speaker=%s generated=1 piper=1 a2f_started=1 a2f_finished=1 text=%s"),WorldCycle,Speakers[CrewIndex],*GeneratedLine);
            ++CrewIndex;
            if (CrewIndex<3) { Stage=EStage::Generate; }
            else if (WorldCycle==1) { Stage=EStage::SceneCancel; }
            else { Succeed(); }
        }
        return;
    }
    if (Stage==EStage::SceneCancel) { CancelAcrossScene(); return; }
    if (Stage==EStage::Restart) { RestartWithPendingRequest(); }
}

void UDCACEIntegrationReview::Generate()
{
    Stage=EStage::Generating; StageStarted=FPlatformTime::Seconds(); bSawACEPlayback=false;
    const FString System=FString::Printf(TEXT("You are %s, crew on a pirate brigantine. The captain has given an order and you are already carrying it out. Say one short line aloud to the captain confirming it, in character. At most 10 words. Spoken words only: no quotes, no actions, no narration."),*Voice->PersonaFor(Speakers[CrewIndex]));
    const FString User=FString::Printf(TEXT("Captain's order: \"%s\"\nWhat you are doing: %s\nSituation: open sea in sunshine; hull at 100 percent"),Orders[CrewIndex],Doing[CrewIndex]);
    TWeakObjectPtr<UDCACEIntegrationReview> Weak(this);
    Voice->RequestDialogue(System,User,32,.6f,false,1.5f,[Weak](bool bOK,const FString& Raw)
    {
        UDCACEIntegrationReview* Self=Weak.Get();
        if (!Self || Self->bEnded || Self->Stage!=EStage::Generating) { return; }
        Self->GeneratedMilliseconds=(FPlatformTime::Seconds()-Self->StageStarted)*1000.0;
        FString Line=Raw; const int32 ThinkEnd=Line.Find(TEXT("</think>"));
        if (ThinkEnd!=INDEX_NONE) { Line=Line.Mid(ThinkEnd+8); }
        Line=Line.TrimStartAndEnd().Replace(TEXT("\""),TEXT("")).Replace(TEXT("*"),TEXT(""));
        if (!bOK || Self->GeneratedMilliseconds>1600.0 || Line.IsEmpty() || Line.Len()>120 || Line.Contains(TEXT("\n")) || Line.Contains(TEXT("<think>")))
        { Self->Fail(TEXT("Acknowledgement generation failed; canned speech cannot pass this test")); return; }
        Self->GeneratedLine=Line;
        Self->Stage=EStage::Speaking; Self->StageStarted=FPlatformTime::Seconds();
        Self->Voice->FinishLine(Speakers[Self->CrewIndex],Line);
    });
}

void UDCACEIntegrationReview::CancelAcrossScene()
{
    PreviousEpoch=Voice->SpeechEpoch;
    const TSharedPtr<FCombinedReview> Run=CombinedRun;
    Voice->RequestDialogue(TEXT("Reply with one brief pirate acknowledgement."),TEXT("Captain, take the helm."),32,.6f,false,4.f,
        [Run](bool,const FString&) { Run->bLateSceneCallback=true; });
    if (!Voice->ACEDialogue || !Voice->ACEDialogue->IsBusy()) { Fail(TEXT("Scene cancellation probe did not start real model inference")); return; }
    Director->Seek(60.f);
    Voice->SynchronizeScene();
    // Capture the reset immediately; newly authored fog reports are allowed to start later.
    bSceneSpeechCleared=Voice->SpeechQueue.IsEmpty() && Voice->PendingTTS.IsEmpty() && !Voice->ActiveCurves && !Voice->IsSpeaking();
    Stage=EStage::SceneDrain; StageStarted=FPlatformTime::Seconds();
}

void UDCACEIntegrationReview::RestartWithPendingRequest()
{
    Voice->ResetPendingSpeech();
    const TSharedPtr<FCombinedReview> Run=CombinedRun;
    Voice->RequestDialogue(TEXT("Reply with one brief pirate acknowledgement."),TEXT("Captain, watch the rigging."),32,.6f,false,4.f,
        [Run](bool,const FString&) { Run->bLateRestartCallback=true; });
    if (!Voice->ACEDialogue || !Voice->ACEDialogue->IsBusy()) { Fail(TEXT("Restart probe did not start real model inference")); return; }
    CombinedRun->PreviousVoice=Voice; CombinedRun->bRestartRequested=true;
    Stage=EStage::Restarting;
    SaveReport(false);
    UE_LOG(LogDC,Display,TEXT("ACE_INTEGRATION_RESTART_REQUEST pending_model=1"));
    UGameplayStatics::ClearSoundMixModifiers(this);
    UGameplayStatics::OpenLevel(this,FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void UDCACEIntegrationReview::SaveReport(bool bFinal)
{
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("final"),bFinal);
    Report->SetBoolField(TEXT("success"),bFinal && CombinedRun->Failures==0 && CombinedRun->Lines==6 && CombinedRun->bSceneCancelled && CombinedRun->bRestartVerified);
    Report->SetNumberField(TEXT("completed_generated_lines"),CombinedRun->Lines); Report->SetNumberField(TEXT("world_cycles"),CombinedRun->Worlds);
    Report->SetNumberField(TEXT("failures"),CombinedRun->Failures); Report->SetBoolField(TEXT("scene_cancel_verified"),CombinedRun->bSceneCancelled);
    Report->SetBoolField(TEXT("restart_verified"),CombinedRun->bRestartVerified);
    Report->SetBoolField(TEXT("late_scene_callback"),CombinedRun->bLateSceneCallback); Report->SetBoolField(TEXT("late_restart_callback"),CombinedRun->bLateRestartCallback);
    Report->SetNumberField(TEXT("elapsed_ms"),(FPlatformTime::Seconds()-CombinedRun->Started)*1000.0);
    Report->SetArrayField(TEXT("results"),CombinedRun->Rows);
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CombinedRun->Output),true);
    if (!FFileHelper::SaveStringToFile(Json,*CombinedRun->Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) { ++CombinedRun->Failures; UE_LOG(LogDC,Error,TEXT("ACE_INTEGRATION_REPORT_WRITE_FAILED")); }
}

void UDCACEIntegrationReview::Fail(const FString& Reason)
{
    if (Stage==EStage::Done || !CombinedRun || CombinedRun->bFinished) { return; }
    ++CombinedRun->Failures; CombinedRun->bFinished=true; Stage=EStage::Done;
    TSharedRef<FJsonObject> Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("failure"),Reason); CombinedRun->Rows.Add(MakeShared<FJsonValueObject>(Row));
    SaveReport(true);
    UE_LOG(LogDC,Error,TEXT("ACE_INTEGRATION_FAILED reason=%s"),*Reason);
    FPlatformMisc::RequestExitWithStatus(false,1);
}

void UDCACEIntegrationReview::Succeed()
{
    if (CombinedRun->Lines!=6 || !CombinedRun->bSceneCancelled || !CombinedRun->bRestartVerified) { Fail(TEXT("Combined coverage incomplete")); return; }
    Stage=EStage::Done; CombinedRun->bFinished=true; SaveReport(true);
    if (!CombinedRun->Failures) { UE_LOG(LogDC,Display,TEXT("ACE_INTEGRATION_SUCCESS generated_lines=6 crews=3 world_cycles=2 scene_cancel=1 restart=1")); }
    FPlatformMisc::RequestExitWithStatus(false,CombinedRun->Failures ? 1 : 0);
}
