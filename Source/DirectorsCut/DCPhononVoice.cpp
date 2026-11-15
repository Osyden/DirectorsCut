#include "DCVoice.h"
#include "DCPhononSpeech.h"
#include "DCCrew.h"
#include "DCTypes.h"
#include "ACEAudioCurveSourceComponent.h"
#include "Components/ChildActorComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

void ADCVoice::InitializePhonon()
{
    bTTSReady = false;
    if (!bUseACE)
    {
        UE_LOG(LogDC, Warning, TEXT("PHONON_UNAVAILABLE requires the configured local Audio2Face provider"));
        return;
    }
    const FString Base = SidecarDir() / TEXT("bin/phonon");
    FString Python = Base / TEXT("python/python.exe");
    FString Script = Base / TEXT("serve.py");
    FString Model = Base / TEXT("model");
    FString Voices = Base / TEXT("voices.json");
    FParse::Value(FCommandLine::Get(), TEXT("DCPhononPython="), Python);
    FParse::Value(FCommandLine::Get(), TEXT("DCPhononScript="), Script);
    FParse::Value(FCommandLine::Get(), TEXT("DCPhononModel="), Model);
    FParse::Value(FCommandLine::Get(), TEXT("DCPhononVoices="), Voices);
    PhononSpeech = NewObject<UDCPhononSpeech>(this);
    PhononSpeech->RegisterComponent();
    const bool bLaunched = PhononSpeech->Initialize(Python, Script, Model, Voices);
    UE_LOG(LogDC, Display, TEXT("TTS_BACKEND selected=Phonon31M launched=%d device=CPU threads=4"), bLaunched);
}

bool ADCVoice::TryPhonon(const FString& Speaker, const FString& Text, int32 BatchId)
{
    if (!bUseACE || bEnding || !PhononSpeech || !PhononSpeech->IsReady() || PhononSpeech->IsBusy()) { return false; }
    AActor* Avatar = nullptr;
    if (auto* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
    {
        for (ADCCrewMember* Member : Crew->GetCrew())
        {
            if (!Member || Member->CrewName != Speaker) { continue; }
            TInlineComponentArray<UChildActorComponent*> Children(Member);
            for (UChildActorComponent* Child : Children)
            {
                if (Child->GetName() == TEXT("PirateAppearance")) { Avatar = Child->GetChildActor(); break; }
            }
        }
    }
    if (!Avatar) { return false; }
    ActiveCurves = Avatar->FindComponentByClass<UACEAudioCurveSourceComponent>();
    if (!ActiveCurves) { return false; }
    ActiveSpeaker = Speaker; ActiveText = Text; ActiveWav.Empty();
    ActiveSpeechBatchId = BatchId;
    ActiveDuration = FMath::Clamp(Text.Len() * .055, 1.0, 15.0);
    bACEStarted = bACESent = bACESendSucceeded = bACEEnded = false;
    ACEDeadline = FPlatformTime::Seconds() + 60;
    ActiveCurves->Volume = 1.6f;
    ActiveCurves->Priority = 100.f;
    ActiveCurves->bOverrideAttenuation = false;
    ActiveCurves->AttenuationSettings = nullptr;
    ActiveCurves->BufferLengthInSeconds = StreamPrebufferSeconds;
    ActiveCurves->OnAnimationStarted.AddDynamic(this, &ADCVoice::ACEStarted);
    ActiveCurves->OnAnimationEnded.AddDynamic(this, &ADCVoice::ACEEnded);
    const int32 Epoch = SpeechEpoch, Serial = ++ACERequestSerial;
    TWeakObjectPtr<ADCVoice> Weak(this);
    bActivePhonon = true;
    const bool bAccepted = PhononSpeech->Speak(Speaker, Text, ActiveCurves, ACEProvider,
        [Weak, Epoch, Serial](bool bSuccess, double Duration)
        {
            ADCVoice* Voice = Weak.Get();
            if (!Voice || Voice->bEnding || Voice->SpeechEpoch != Epoch || Voice->ACERequestSerial != Serial || !Voice->bActivePhonon) { return; }
            if (bSuccess)
            {
                Voice->ActiveDuration = Duration;
                Voice->ACEDeadline = FPlatformTime::Seconds() + Duration + 10;
            }
            if (bSuccess) { Voice->ACESent(true); }
            else
            {
                // A producer/transport failure is not proof that the GPU provider
                // is broken. Retire this batch without replay; allow a fresh request.
                Voice->bACESent = Voice->bACEEnded = true;
                Voice->bACESendSucceeded = false;
                UE_LOG(LogDC, Warning, TEXT("PHONON_SPEECH_FAILED speaker=%s"), *Voice->ActiveSpeaker);
            }
        });
    if (!bAccepted)
    {
        ClearACE(); ActiveSpeechBatchId = 0;
        return false;
    }
    UE_LOG(LogDC, Display, TEXT("PHONON_SPEECH_STARTED batch=%d speaker=%s text=%s"), BatchId, *Speaker, *Text);
    return true;
}
