#include "DCVoice.h"
#include "DCTypes.h"
#include "Engine/World.h"

int32 ADCVoice::BeginSpeechBatch(const FString& Speaker)
{
    check(StreamingBatch.Id == 0);
    StreamingBatch = FSpeechBatch();
    StreamingBatch.Id = ++NextSpeechBatchId;
    StreamingBatch.Speaker = Speaker;
    return StreamingBatch.Id;
}

void ADCVoice::QueueSpeechChunk(int32 BatchId, const FString& Text)
{
    if (bEnding || BatchId == 0 || StreamingBatch.Id != BatchId || StreamingBatch.bFinalized || Text.IsEmpty()) { return; }
    // The whole utterance is limited to 40 words. Six-word clause boundaries plus
    // the final remainder fit within eight chunks, with one Piper worker and one ready WAV.
    if (StreamingBatch.Queued >= 8) { StreamingBatch.bFailed = true; return; }
    ++StreamingBatch.Queued;
    SpeechQueue.Add({StreamingBatch.Speaker, Text, FPlatformTime::Seconds(), false, BatchId});
    UE_LOG(LogDC, Display, TEXT("SPEECH_STREAM_CHUNK batch=%d chunk=%d speaker=%s text=%s"),
        BatchId, StreamingBatch.Queued, *StreamingBatch.Speaker, *Text);
}

void ADCVoice::FinalizeSpeechBatch(int32 BatchId, const FString& Text, bool bSuccess)
{
    if (BatchId == 0 || StreamingBatch.Id != BatchId) { return; }
    StreamingBatch.bFinalized = true;
    StreamingBatch.Text = Text;
    StreamingBatch.bFailed |= !bSuccess;
    if (!bSuccess)
    {
        // A reply may have reached the model history before a later audio chunk
        // failed. Do not remember the unheard remainder as a completed reply.
        if (auto* History = ConversationHistory.Find(StreamingBatch.Speaker))
        {
            if (!History->IsEmpty() && History->Last().User == StreamingBatch.User &&
                History->Last().Reply == StreamingBatch.Text) { History->Pop(); }
        }
        // Do not replay the full response after a partial response failed. Discard
        // unplayed continuations; an already-playing safe chunk can finish.
        for (int32 I = SpeechQueue.Num()-1; I >= 0; --I)
        {
            if (SpeechQueue[I].BatchId == BatchId) { SpeechQueue.RemoveAt(I); ++StreamingBatch.Completed; }
        }
        for (int32 I = ReadySpeech.Num()-1; I >= 0; --I)
        {
            if (ReadySpeech[I].BatchId == BatchId) { ReadySpeech.RemoveAt(I); ++StreamingBatch.Completed; }
        }
        for (int32 I = PendingTTS.Num()-1; I >= 0; --I)
        {
            if (PendingTTS[I].BatchId != BatchId) { continue; }
            if (FPlatformProcess::IsProcRunning(PendingTTS[I].Proc)) { FPlatformProcess::TerminateProc(PendingTTS[I].Proc, true); }
            FPlatformProcess::CloseProc(PendingTTS[I].Proc);
            PendingTTS.RemoveAt(I); ++StreamingBatch.Completed;
        }
    }
    TryFinishSpeechBatch();
}

void ADCVoice::CompleteSpeechChunk(int32 BatchId, bool bSuccess, bool bFace)
{
    if (BatchId == 0 || StreamingBatch.Id != BatchId) { return; }
    ++StreamingBatch.Completed;
    StreamingBatch.bFailed |= !bSuccess;
    if (bFace) { ++StreamingBatch.Faces; }
    if (!bSuccess)
    {
        FinalizeSpeechBatch(BatchId, StreamingBatch.Text, false);
        return;
    }
    TryFinishSpeechBatch();
}

void ADCVoice::TryFinishSpeechBatch()
{
    if (!StreamingBatch.Id || !StreamingBatch.bFinalized || StreamingBatch.Completed < StreamingBatch.Queued) { return; }
    LastFinishedBatch = StreamingBatch;
    UE_LOG(LogDC, Display, TEXT("SPEECH_STREAM_FINISHED batch=%d speaker=%s chunks=%d faces=%d success=%d text=%s"),
        StreamingBatch.Id, *StreamingBatch.Speaker, StreamingBatch.Completed, StreamingBatch.Faces,
        !StreamingBatch.bFailed && StreamingBatch.Queued > 0, *StreamingBatch.Text);
    StreamingBatch = FSpeechBatch();
}

void ADCVoice::QueueSynthesisFailure(const FString& Speaker, const FString& Text, int32 BatchId, TSharedPtr<FDCReportSpeechGuard> ReportGuard)
{
    if (ReportGuard && !ReportGuard->CanStart(FPlatformTime::Seconds())) { return; }
    if (!bStreamSpeech) { ShowSpeech(Speaker, Text, FMath::Clamp(Text.Len() * .055, 4.0, 9.0), ReportGuard); return; }
    // Queue the subtitle fallback behind the current audible chunk rather than
    // replacing its text while another character's face/audio is still active.
    ReadySpeech.Add({Speaker, Text, FString(), 0, SpeechEpoch, BatchId, ReportGuard});
}

void ADCVoice::StartReadySpeech()
{
    if (bEnding || IsMenuOpen() || GFrameCounter < EarliestSpeechStartFrame || ReadySpeech.IsEmpty() || ActiveCurves || !Playing.IsEmpty() ||
        FPlatformTime::Seconds() < SilentSpeechUntil) { return; }
    FReadySpeech Ready = MoveTemp(ReadySpeech[0]);
    ReadySpeech.RemoveAt(0);
    if (Ready.Epoch != SpeechEpoch || (Ready.ReportGuard && !Ready.ReportGuard->CanStart(FPlatformTime::Seconds()))) { return; }
    ActiveText = Ready.Text;
    ActiveReportGuard = Ready.ReportGuard;
    ActiveSpeechBatchId = Ready.BatchId;
    UE_LOG(LogDC, Display, TEXT("SPEECH_STREAM_AUDIO_READY batch=%d duration=%.3f text=%s"), Ready.BatchId, Ready.Duration, *Ready.Text);
    if (Ready.Path.IsEmpty())
    {
        const double Duration = FMath::Clamp(Ready.Text.Len() * .055, 4.0, 9.0);
        ShowSpeech(Ready.Speaker, Ready.Text, Duration, Ready.ReportGuard);
        SilentSpeechUntil = FPlatformTime::Seconds() + Duration;
        ActiveSpeechBatchId = 0;
        ActiveReportGuard.Reset();
        CompleteSpeechChunk(Ready.BatchId, false, false);
        return;
    }
    if (!TryACE(Ready.Speaker, Ready.Path, Ready.Duration))
    {
        ActiveSpeechBatchId = 0;
        ActiveReportGuard.Reset();
        PlayWav(Ready.Speaker, Ready.Path, Ready.BatchId, Ready.ReportGuard);
    }
}
