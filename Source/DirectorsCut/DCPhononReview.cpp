#include "DCVoice.h"
#include "DCPhononSpeech.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "HAL/PlatformMisc.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace DCPhononReviewPrivate
{
    enum class EStage { Ready, Prefix, Travel, FailedRequest, Fresh, Done };
    struct FRun
    {
        FString Mode;
        double Started = 0, StageStarted = 0;
        EStage Stage = EStage::Ready;
        TWeakObjectPtr<ADCVoice> OriginalVoice;
        TFunction<bool()> OldWorkerDrained;
        int32 InitialGeneration = 0, FaultId = 0, FailedBatchId = 0, Worlds = 1;
        bool bSawFreshPlayback = false, bSawFailedBatch = false, bOldDrainVerified = false;
    };
    // Created only by the explicit native review flag; survives exactly one normal OpenLevel.
    TUniquePtr<FRun> Run;
    struct FReportRun
    {
        double Started=0, PlaybackStarted=0;
        int32 Case=0, Stage=0, ProtectedCommands=0, NaturalReports=0, CancelledReports=0;
        bool bSawCommand=false, bSawReportRetired=false;
        TSharedPtr<bool> TargetCurrent=MakeShared<bool>(true);
        TSharedPtr<bool> ContinuationCurrent=MakeShared<bool>(true);
    };
    TUniquePtr<FReportRun> ReportRun;
}

bool ADCVoice::TickPhononLifecycleReview()
{
    if (PhononReviewMode.IsEmpty()) { return false; }
    if (PhononReviewMode == TEXT("Reports"))
    {
        using namespace DCPhononReviewPrivate;
        const double Now=FPlatformTime::Seconds();
        if (!ReportRun) { ReportRun=MakeUnique<FReportRun>(); ReportRun->Started=Now; }
        auto& Review=*ReportRun;
        if (Review.Stage==3) { return true; }
        auto Fail=[&](const TCHAR* Reason)
        {
            UE_LOG(LogDC,Error,TEXT("REPORT_LIFECYCLE_FAILED case=%d stage=%d reason=%s"),Review.Case,Review.Stage,Reason);
            Review.Stage=3; FPlatformMisc::RequestExitWithStatus(false,1);
        };
        if (auto* Director=ADCRideDirector::Get(this)) { Director->TimeScale=0.f; }
        if (!bUsePhonon || !bUseACE || bSmokeTest || Now-Review.Started>90.0) { Fail(TEXT("Invalid configuration or 90-second timeout")); return true; }
        const FString Warning=TEXT("Threat to port, Captain! Keep your eyes on that creature while I watch its approach beside the ruined temple.");
        const FString Command=TEXT("Holding fire, Captain. Waiting for your order.");
        if (Review.Stage==0 && bTTSReady && bLLMReady && Now-Review.Started>5.0)
        {
            ResetPendingSpeech(); *Review.TargetCurrent=true; *Review.ContinuationCurrent=true;
            Review.bSawCommand=false; Review.bSawReportRetired=false; Review.PlaybackStarted=0;
            const auto Target=Review.TargetCurrent;
            const auto Continuation=Review.ContinuationCurrent;
            TFunction<bool()> CanContinue;
            if (Review.Case>=3) { CanContinue=[Continuation] { return *Continuation; }; }
            Speak(TEXT("Ruby"),Warning,true,[Target] { return *Target; },MoveTemp(CanContinue));
            Speak(TEXT("Duncan"),Command);
            Review.Stage=1;
        }
        else if (Review.Stage==1 && bActivePhonon && ActiveCurves && ActiveSpeaker==TEXT("Ruby"))
        {
            if (Review.Case<2 && bACEStarted) { Fail(TEXT("Report began before pre-playback rejection")); return true; }
            if (Review.Case>=2 && !bACEStarted) { return true; }
            if (!ActiveReportGuard) { Fail(TEXT("Missing active report guard")); return true; }
            Review.PlaybackStarted=ActiveReportGuard->PlaybackStartedAt;
            if (Review.Case==0 || Review.Case==4) { ActiveReportGuard->StartDeadline=Now-0.01; }
            else if (Review.Case==5) { *Review.ContinuationCurrent=false; }
            else { *Review.TargetCurrent=false; }
            UE_LOG(LogDC,Display,TEXT("REPORT_LIFECYCLE_INVALIDATE case=%d started=%d native_pending=%d command_queued=%d admission=%d continuation=%d expect_natural=%d"),
                Review.Case,bACEStarted,!bACESent,SpeechQueue.Num(),ActiveReportGuard->CanStart(Now),
                ActiveReportGuard->CanContinue(),Review.Case==3 || Review.Case==4);
            Review.Stage=2;
        }
        else if (Review.Stage==2)
        {
            const bool ExpectNatural=Review.Case==3 || Review.Case==4;
            if (ActiveSpeaker==TEXT("Ruby") && ActiveText==Warning && !ActiveCurves && !Review.bSawReportRetired)
            {
                const bool FinishedNaturally=bACEStarted && bACESent && bACESendSucceeded && bACEEnded;
                if (FinishedNaturally!=ExpectNatural) { Fail(TEXT("Report did not retire with the expected natural/cancelled lifecycle")); return true; }
                if (ExpectNatural && (Review.PlaybackStarted<=0 || Now-Review.PlaybackStarted+.35<ActiveDuration))
                { Fail(TEXT("Report ended before its full generated audio duration")); return true; }
                Review.bSawReportRetired=true;
                if (ExpectNatural) { ++Review.NaturalReports; } else { ++Review.CancelledReports; }
                UE_LOG(LogDC,Display,TEXT("REPORT_LIFECYCLE_REPORT_RETIRED case=%d natural=%d elapsed=%.3f duration=%.3f"),
                    Review.Case,FinishedNaturally,Review.PlaybackStarted>0 ? Now-Review.PlaybackStarted : 0.,ActiveDuration);
            }
            if (ActiveSpeaker==TEXT("Duncan") && ActiveText==Command && ActiveCurves && bACEStarted)
            {
                if (!Review.bSawReportRetired) { Fail(TEXT("Command started without observing the report lifecycle")); return true; }
                Review.bSawCommand=true;
            }
            if (Review.bSawCommand && ActiveSpeaker==TEXT("Duncan") && ActiveText==Command &&
                bACESent && bACESendSucceeded && bACEEnded && !ActiveCurves && PhononSpeech && !PhononSpeech->IsBusy())
            {
                ++Review.ProtectedCommands; ++Review.Case;
                if (Review.Case==6)
                {
                    if (Review.NaturalReports!=2 || Review.CancelledReports!=4 || Review.ProtectedCommands!=6)
                    { Fail(TEXT("Incomplete lifecycle accounting")); return true; }
                    UE_LOG(LogDC,Display,TEXT("REPORT_LIFECYCLE_SUCCESS expiry_before_playback=1 target_before_playback=1 target_during_playback=1 soft_admission_during_playback=1 expiry_during_playback=1 continuation_invalid_during_playback=1 natural_reports=%d cancelled_reports=%d commands_preserved=%d elapsed=%.2f"),
                        Review.NaturalReports,Review.CancelledReports,Review.ProtectedCommands,Now-Review.Started);
                    Review.Stage=3; FPlatformMisc::RequestExitWithStatus(false,0);
                }
                else { Review.Stage=0; }
            }
        }
        return true;
    }
    if (PhononReviewMode == TEXT("Reload") || PhononReviewMode == TEXT("Failure"))
    {
        using namespace DCPhononReviewPrivate;
        const double Now = FPlatformTime::Seconds();
        if (!Run)
        {
            Run = MakeUnique<FRun>();
            Run->Mode = PhononReviewMode;
            Run->Started = Run->StageStarted = Now;
            Run->OriginalVoice = this;
        }
        FRun& Review = *Run;
        if (Review.Stage == EStage::Done) { return true; }
        auto Fail = [&](const TCHAR* Reason)
        {
            UE_LOG(LogDC, Error, TEXT("PHONON_LIFECYCLE_FAILED mode=%s stage=%d reason=%s"), *Review.Mode, int32(Review.Stage), Reason);
            Review.Stage = EStage::Done;
            FPlatformMisc::RequestExitWithStatus(false, 1);
        };
        const FString Prefix = TEXT("Hold her steady, Captain. That fog hides more than rocks, and I need every hand watching the channel while I keep us on course.");
        const FString Critical = TEXT("Fire on deck! Captain, keep us clear while I smother it.");
        if (!bUsePhonon || !bUseACE || bSmokeTest || Review.Mode != PhononReviewMode ||
            !GetWorld() || GetWorld()->GetOutermost()->GetName() != TEXT("/Game/DC/Maps/L_Ride") || Now - Review.Started > 120)
        { Fail(TEXT("Invalid review configuration or 120-second overall deadline")); return true; }
        if (auto* Director = ADCRideDirector::Get(this)) { Director->TimeScale = 0.f; }
        if (Review.Stage == EStage::Ready && bTTSReady && bLLMReady && Now - Review.Started > 5)
        {
            ResetPendingSpeech();
            if (!PhononSpeech) { Fail(TEXT("Missing Phonon component")); return true; }
            int32 IgnoredFault = 0;
            PhononSpeech->GetProducerReviewState(Review.InitialGeneration, IgnoredFault);
            if (Review.Mode == TEXT("Failure"))
            {
                // Use a real batch so failure cannot silently count as completed facial speech.
                Review.FailedBatchId = BeginSpeechBatch(TEXT("Rex"));
                QueueSpeechChunk(Review.FailedBatchId, Prefix);
                FinalizeSpeechBatch(Review.FailedBatchId, Prefix, true);
            }
            else { Speak(TEXT("Rex"), Prefix); }
            Review.Stage = EStage::Prefix; Review.StageStarted = Now;
        }
        else if (Review.Stage == EStage::Prefix && bActivePhonon && ActiveSpeaker == TEXT("Rex") &&
                 ActiveText == Prefix && ActiveCurves && bACEStarted && !bACESent)
        {
            UE_LOG(LogDC, Display, TEXT("PHONON_LIFECYCLE_INTERRUPTING mode=%s active_audio=1 native_send_pending=1 epoch=%d"), *Review.Mode, SpeechEpoch);
            if (Review.Mode == TEXT("Reload"))
            {
                Review.OldWorkerDrained = PhononSpeech->CaptureReloadDrainProbeForReview();
                if (!Review.OldWorkerDrained) { Fail(TEXT("Could not capture old worker drain receipt")); return true; }
                Review.Stage = EStage::Travel; Review.StageStarted = Now;
                // Normal world teardown must cancel the prefix; do not pre-clear speech for this test.
                UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
            }
            else
            {
                Review.FaultId = PhononSpeech->RequestProducerFailureForReview();
                if (Review.FaultId <= 0) { Fail(TEXT("Owned producer fault was not admitted")); return true; }
                Review.Stage = EStage::FailedRequest; Review.StageStarted = Now;
            }
        }
        else if (Review.Stage == EStage::Travel)
        {
            if (Now - Review.StageStarted > 45) { Fail(TEXT("Reload/drain exceeded 45 seconds")); return true; }
            if (Review.OriginalVoice.Get() == this) { return true; }
            ADCVoice* Old = Review.OriginalVoice.Get();
            if (Old && !Old->bEnding) { Fail(TEXT("Previous voice did not receive EndPlay")); return true; }
            if (!Review.OldWorkerDrained() || !bTTSReady || !bLLMReady || !PhononSpeech || PhononSpeech->IsBusy()) { return true; }
            Review.bOldDrainVerified = true;
            Review.Worlds = 2;
            Review.OldWorkerDrained = nullptr;
            UE_LOG(LogDC, Display, TEXT("PHONON_RELOAD_DRAIN_VERIFIED old_voice_ended=1 old_worker_drained=1 worlds=2"));
            ResetPendingSpeech();
            Speak(TEXT("Ruby"), Critical, true);
            Review.Stage = EStage::Fresh; Review.StageStarted = Now;
        }
        else if (Review.Stage == EStage::FailedRequest)
        {
            if (!PhononSpeech || Now - Review.StageStarted > 30) { Fail(TEXT("Producer failure/restart exceeded 30 seconds")); return true; }
            int32 Generation = 0, Applied = 0;
            PhononSpeech->GetProducerReviewState(Generation, Applied);
            if (Applied < 0) { Fail(TEXT("Prefix finished before worker could apply producer fault")); return true; }
            if (LastFinishedBatch.Id == Review.FailedBatchId)
            {
                if (!LastFinishedBatch.bFinalized || !LastFinishedBatch.bFailed || LastFinishedBatch.Queued != 1 ||
                    LastFinishedBatch.Completed != 1 || LastFinishedBatch.Faces != 0 || LastFinishedBatch.Text != Prefix ||
                    LastFinishedBatch.Speaker != TEXT("Rex"))
                { Fail(TEXT("Interrupted batch was not retired as exactly one failed chunk")); return true; }
                Review.bSawFailedBatch = true;
            }
            if (Applied != Review.FaultId || !Review.bSawFailedBatch || !bACESent || bACESendSucceeded ||
                ActiveCurves || PhononSpeech->IsBusy() || !PhononSpeech->IsReady() || Generation <= Review.InitialGeneration) { return true; }
            if (!SpeechQueue.IsEmpty() || !ReadySpeech.IsEmpty() || !PendingTTS.IsEmpty())
            { Fail(TEXT("Interrupted prefix or continuation remains queued")); return true; }
            UE_LOG(LogDC, Display, TEXT("PHONON_FAILURE_RECOVERY_READY request=%d generation_before=%d generation_after=%d failed_batch=%d"),
                Review.FaultId, Review.InitialGeneration, Generation, Review.FailedBatchId);
            // No ResetPendingSpeech: prove the ordinary failure callback and recovery path.
            Speak(TEXT("Ruby"), Critical, true);
            Review.Stage = EStage::Fresh; Review.StageStarted = Now;
        }
        else if (Review.Stage == EStage::Fresh)
        {
            if (Now - Review.StageStarted > 45) { Fail(TEXT("Fresh critical speech exceeded 45 seconds")); return true; }
            if (ActiveCurves && (ActiveSpeaker != TEXT("Ruby") || ActiveText != Critical))
            { Fail(TEXT("Stale/unexpected speech after recovery")); return true; }
            if (ActiveSpeaker == TEXT("Ruby") && ActiveText == Critical && ActiveCurves && bACEStarted)
            { Review.bSawFreshPlayback = true; }
            if (Review.bSawFreshPlayback && ActiveSpeaker == TEXT("Ruby") && ActiveText == Critical &&
                bACESent && bACESendSucceeded && bACEEnded && !ActiveCurves && PhononSpeech && !PhononSpeech->IsBusy())
            {
                UE_LOG(LogDC, Display, TEXT("PHONON_LIFECYCLE_SUCCESS mode=%s fresh_critical_audio=1 face_lifecycle=1 old_worker_drained=%d failed_batch_verified=%d worlds=%d"),
                    *Review.Mode, Review.bOldDrainVerified, Review.bSawFailedBatch, Review.Worlds);
                Review.Stage = EStage::Done; FPlatformMisc::RequestExitWithStatus(false, 0);
            }
        }
        return true;
    }
    if (PhononReviewStage == 3) { return true; }
    const double Now = FPlatformTime::Seconds();
    if (!PhononReviewStarted) { PhononReviewStarted = Now; }
    if (auto* Director = ADCRideDirector::Get(this)) { Director->TimeScale = 0.f; }
    if (!bUsePhonon || !bUseACE ||
        (PhononReviewMode != TEXT("Cancel") && PhononReviewMode != TEXT("Exit")) || Now - PhononReviewStarted > 120)
    {
        UE_LOG(LogDC, Error, TEXT("PHONON_LIFECYCLE_FAILED mode=%s stage=%d"), *PhononReviewMode, PhononReviewStage);
        PhononReviewStage = 3; FPlatformMisc::RequestExitWithStatus(false, 1);
        return true;
    }
    const FString Critical = TEXT("Fire on deck! Captain, keep us clear while I smother it.");
    if (PhononReviewStage == 0 && bTTSReady && bLLMReady && Now - PhononReviewStarted > 5)
    {
        ResetPendingSpeech();
        Speak(TEXT("Rex"), TEXT("Hold her steady, Captain. That fog hides more than rocks, and I need every hand watching the channel while I keep us on course."));
        PhononReviewStage = 1;
    }
    else if (PhononReviewStage == 1 && bActivePhonon && ActiveCurves && bACEStarted && !bACESent)
    {
        UE_LOG(LogDC, Display, TEXT("PHONON_LIFECYCLE_INTERRUPTING mode=%s active_audio=1 native_send_pending=1 epoch=%d"), *PhononReviewMode, SpeechEpoch);
        if (PhononReviewMode == TEXT("Exit"))
        {
            // Exit0 alone is not sufficient: require the pre-exit drain marker
            // and the actual process exit, with no worker/model left behind.
            PhononReviewStage = 3; FPlatformMisc::RequestExitWithStatus(false, 0);
        }
        else
        {
            ResetPendingSpeech();
            Speak(TEXT("Ruby"), Critical, true);
            PhononReviewStage = 2;
        }
    }
    else if (PhononReviewStage == 2)
    {
        if (ActiveSpeaker == TEXT("Ruby") && ActiveText == Critical && ActiveCurves && bACEStarted)
        { bPhononReviewSawFreshPlayback = true; }
        if (bPhononReviewSawFreshPlayback && ActiveSpeaker == TEXT("Ruby") && ActiveText == Critical &&
            bACESent && bACESendSucceeded && bACEEnded && !ActiveCurves && PhononSpeech && !PhononSpeech->IsBusy())
        {
            UE_LOG(LogDC, Display, TEXT("PHONON_LIFECYCLE_SUCCESS mode=Cancel fresh_critical_audio=1 face_lifecycle=1 epoch=%d"), SpeechEpoch);
            PhononReviewStage = 3; FPlatformMisc::RequestExitWithStatus(false, 0);
        }
    }
    return true;
}
