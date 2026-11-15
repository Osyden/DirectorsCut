#include "DCVoice.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCReportSpeechFreshnessTest,"DirectorsCut.Dialogue.ReportSpeechFreshness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCReportSpeechFreshnessTest::RunTest(const FString&)
{
    bool bTargetCurrent=true;
    auto Guard=MakeShared<FDCReportSpeechGuard>();
    Guard->StartDeadline=10.0; Guard->IsStillRelevant=[&bTargetCurrent] { return bTargetCurrent; };
    TestTrue(TEXT("Valid report can begin at deadline"),Guard->CanStart(10.0));
    TestFalse(TEXT("Slow synthesis cannot reset report deadline"),Guard->CanStart(10.1));
    TestTrue(TEXT("Already speaking valid target is not cut off by start deadline"),Guard->IsCurrent());
    bTargetCurrent=false;
    TestFalse(TEXT("Target leaving cancels report before deadline"),Guard->CanStart(2.0));
    TestFalse(TEXT("Target leaving cancels already-started report"),Guard->IsCurrent());

    UWorld::InitializationValues Values;
    Values.InitializeScenes(false).AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values,true);
    if (!TestNotNull(TEXT("World"),World)) { return false; }
    ADCVoice* Voice=World->SpawnActor<ADCVoice>();
    if (!TestNotNull(TEXT("Voice"),Voice)) { World->DestroyWorld(false); return false; }
    Voice->bTTSReady=true; Voice->bStreamSpeech=true;
    bTargetCurrent=true;
    Voice->Speak(TEXT("Rex"),TEXT("Threat to port!"),true,[&bTargetCurrent] { return bTargetCurrent; });
    Voice->Speak(TEXT("Duncan"),TEXT("Holding fire, Captain."));
    if (!TestEqual(TEXT("Report and command queued"),Voice->SpeechQueue.Num(),2)) { World->DestroyWorld(false); return false; }
    const auto Shared=Voice->SpeechQueue[0].ReportGuard;
    TestTrue(TEXT("Authored report carries its origin guard"),Shared.IsValid());
    TestFalse(TEXT("Command does not carry report cancellation"),Voice->SpeechQueue[1].ReportGuard.IsValid());
    Voice->ReadySpeech.Add({TEXT("Rex"),TEXT("Threat to port!"),TEXT("unused.wav"),1.0,Voice->SpeechEpoch,0,Shared});
    Voice->ReadySpeech.Add({TEXT("Duncan"),TEXT("Holding fire, Captain."),TEXT("unused.wav"),1.0,Voice->SpeechEpoch,0});
    auto& Pending=Voice->PendingTTS.AddDefaulted_GetRef(); Pending.ReportGuard=Shared;
    auto& PendingCommand=Voice->PendingTTS.AddDefaulted_GetRef(); PendingCommand.Text=TEXT("Command");
    bTargetCurrent=false;
    Voice->ExpireReports();
    TestEqual(TEXT("Stale queued report removed; command retained"),Voice->SpeechQueue.Num(),1);
    TestEqual(TEXT("Stale synthesized report removed; command retained"),Voice->ReadySpeech.Num(),1);
    TestEqual(TEXT("Stale pending report removed; command retained"),Voice->PendingTTS.Num(),1);
    TestEqual(TEXT("Surviving queued line is command"),Voice->SpeechQueue[0].Speaker,FString(TEXT("Duncan")));
    Voice->ActiveReportGuard=Shared; Voice->bACEStarted=false;
    Voice->ACEStarted();
    TestFalse(TEXT("Invalid target rejected at actual ACE playback callback"),Voice->bACEStarted);
    bTargetCurrent=true; Shared->StartDeadline=FPlatformTime::Seconds()-1.0;
    Voice->ACEStarted();
    TestFalse(TEXT("Expired report rejected at actual ACE playback callback"),Voice->bACEStarted);
    Voice->ActiveReportGuard.Reset(); Voice->ActiveSpeaker=TEXT("Duncan"); Voice->ActiveText=TEXT("Holding fire, Captain.");
    Voice->ACEStarted();
    TestTrue(TEXT("Command playback remains admitted"),Voice->bACEStarted);
    TestEqual(TEXT("Command subtitle remains intact"),Voice->SpeakingText,FString(TEXT("Holding fire, Captain.")));
    Voice->PendingTTS.Empty(); Voice->ReadySpeech.Empty(); Voice->SpeechQueue.Empty();
    World->DestroyWorld(false);
    return true;
}
#endif
