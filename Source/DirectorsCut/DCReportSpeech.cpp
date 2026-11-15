#include "DCVoice.h"
#include "DCTypes.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"

void ADCVoice::ExpireReports()
{
    check(IsInGameThread());
    const double Now=FPlatformTime::Seconds();
    SpeechQueue.RemoveAll([Now](const FQueuedLine& Line) { return Line.ReportGuard && !Line.ReportGuard->CanStart(Now); });
    ReadySpeech.RemoveAll([Now](const FReadySpeech& Line) { return Line.ReportGuard && !Line.ReportGuard->CanStart(Now); });
    for (int32 Index=PendingTTS.Num()-1;Index>=0;--Index)
    {
        auto& Pending=PendingTTS[Index];
        if (!Pending.ReportGuard || Pending.ReportGuard->CanStart(Now)) { continue; }
        if (Pending.Proc.IsValid())
        {
            if (FPlatformProcess::IsProcRunning(Pending.Proc)) { FPlatformProcess::TerminateProc(Pending.Proc,true); }
            FPlatformProcess::CloseProc(Pending.Proc);
        }
        PendingTTS.RemoveAt(Index);
    }
    // Deadline controls admission to playback, not the length of an already
    // relevant sentence. Target validity continues to apply while it is spoken.
    if (ActiveCurves && ActiveReportGuard &&
        (bACEStarted ? !ActiveReportGuard->CanContinue() : !ActiveReportGuard->CanStart(Now)))
    {
        UE_LOG(LogDC,Display,TEXT("REPORT_SPEECH_CANCELLED speaker=%s started=%d elapsed=%.3f reason=%s text=%s"),
            *ActiveSpeaker,bACEStarted,bACEStarted ? Now-ActiveReportGuard->PlaybackStartedAt : 0.,
            bACEStarted ? TEXT("threat_ended") : TEXT("stale_before_start"),*ActiveText);
        ClearACE(); ActiveSpeechBatchId=0;
    }
    for (int32 Index=Playing.Num()-1;Index>=0;--Index)
    {
        auto& Playback=Playing[Index];
        if (!Playback.ReportGuard || Playback.ReportGuard->CanContinue()) { continue; }
        if (UAudioComponent* Component=Playback.Component.Get())
        {
            LiveWaves.Remove(Cast<USoundWaveProcedural>(Component->Sound));
            Component->Stop(); Component->DestroyComponent();
        }
        Playing.RemoveAt(Index);
    }
    if (VisibleReportGuard && !VisibleReportGuard->CanContinue())
    {
        SpeakingSpeaker.Empty(); SpeakingText.Empty(); SpeechVisibleUntil=0;
        SilentSpeechUntil=0; VisibleReportGuard.Reset();
    }
}
