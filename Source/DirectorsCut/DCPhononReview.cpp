#include "DCVoice.h"
#include "DCPhononSpeech.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "HAL/PlatformMisc.h"

bool ADCVoice::TickPhononLifecycleReview()
{
    if (PhononReviewMode.IsEmpty()) { return false; }
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
