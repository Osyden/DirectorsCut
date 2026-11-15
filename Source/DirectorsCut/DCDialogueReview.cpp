#include "DCDialogueReview.h"
#include "DCDialoguePrompts.h"
#include "DCLanguageModelResponse.h"
#include "DCVoice.h"
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

namespace
{
    TSharedRef<FJsonObject> DescribeIntent(const FDCIntent& Intent)
    {
        TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
        Object->SetBoolField(TEXT("valid"),Intent.bValid);
        Object->SetStringField(TEXT("crew"),Intent.CrewName);
        Object->SetNumberField(TEXT("action"),int32(Intent.Action));
        Object->SetNumberField(TEXT("station"),int32(Intent.Station));
        Object->SetNumberField(TEXT("side"),int32(Intent.Side));
        return Object;
    }
}

ADCDialogueReviewGameMode::ADCDialogueReviewGameMode()
{
    DefaultPawnClass = nullptr;
    HUDClass = nullptr;
    PrimaryActorTick.bCanEverTick = true;
}

void ADCDialogueReviewGameMode::BeginPlay()
{
    Super::BeginPlay();
    Started = FPlatformTime::Seconds();
    Voice = GetWorld()->SpawnActor<ADCVoice>();
    Backend = Voice ? Voice->DialogueBackend : TEXT("Unknown");
    ReportPath = FPaths::ProjectSavedDir() / TEXT("ACEDialogue") / (Backend + TEXT("-comparison.json"));
    FParse::Value(FCommandLine::Get(),TEXT("DCDialogueReport="),ReportPath);
    if (!Voice) { AddHarnessFailure(TEXT("Could not spawn ADCVoice")); Finish(); return; }

    using A = EDCAction; using S = EDCStationType; using D = EDCSide;
    auto Order = [this](const TCHAR* Label,const TCHAR* Text,const TCHAR* Crew,A Action,S Station,D Side)
    {
        FReviewCase& Case = Cases.AddDefaulted_GetRef();
        Case.Label=Label; Case.User=Text;
        Case.System=DCDialoguePrompts::OrderSystem();
        Case.Expected.CrewName=Crew; Case.Expected.Action=Action; Case.Expected.Station=Station; Case.Expected.Side=Side;
        Case.Expected.bValid=Action!=A::None;
    };
    Order(TEXT("take_wheel"),TEXT("Rex, could you take the wheel?"),TEXT("Rex"),A::Man,S::Helm,D::None);
    Order(TEXT("turn_port"),TEXT("Rex, bring her round to port."),TEXT("Rex"),A::TurnPort,S::Helm,D::Port);
    Order(TEXT("turn_starboard"),TEXT("Rex, turn us to starboard."),TEXT("Rex"),A::TurnStarboard,S::Helm,D::Starboard);
    Order(TEXT("steady"),TEXT("Rex, keep us straight ahead."),TEXT("Rex"),A::Steady,S::Helm,D::None);
    Order(TEXT("fire_at_will"),TEXT("Duncan, fire at will."),TEXT("Duncan"),A::Fire,S::Cannons,D::None);
    Order(TEXT("hold_fire"),TEXT("Duncan, stop shooting. Don't fire."),TEXT("Duncan"),A::HoldFire,S::Cannons,D::None);
    Order(TEXT("man_port_guns"),TEXT("Duncan, take charge of the port battery."),TEXT("Duncan"),A::Man,S::Cannons,D::Port);
    Order(TEXT("man_starboard_guns"),TEXT("Ruby, get on the starboard cannons."),TEXT("Ruby"),A::Man,S::Cannons,D::Starboard);
    Order(TEXT("extinguish"),TEXT("Ruby, douse those flames on deck."),TEXT("Ruby"),A::Extinguish,S::Fire,D::None);
    Order(TEXT("fire_kill"),TEXT("Rex, kill those fires."),TEXT("Rex"),A::Extinguish,S::Fire,D::None);
    Order(TEXT("fire_get"),TEXT("Rex, get the flames."),TEXT("Rex"),A::Extinguish,S::Fire,D::None);
    Order(TEXT("fire_put_split"),TEXT("Duncan, put that fire out."),TEXT("Duncan"),A::Extinguish,S::Fire,D::None);
    Order(TEXT("fire_deal"),TEXT("Ruby, please deal with those blazes on deck."),TEXT("Ruby"),A::Extinguish,S::Fire,D::None);
    Order(TEXT("repair"),TEXT("Ruby, get the damaged sails repaired."),TEXT("Ruby"),A::Repair,S::Rigging,D::None);
    Order(TEXT("report"),TEXT("Rex, how much damage has the hull taken?"),TEXT("Rex"),A::Report,S::None,D::None);
    Order(TEXT("not_an_order"),TEXT("Good morning. Lovely weather today."),TEXT(""),A::None,S::None,D::None);

    // Held-out paraphrases: keep these out of prompt examples and preserve the original cases above.
    Order(TEXT("heldout_cease_port"),TEXT("Duncan, cease fire on the port side."),TEXT("Duncan"),A::HoldFire,S::Cannons,D::Port);
    Order(TEXT("heldout_negated_fire"),TEXT("Ruby, do not shoot the starboard guns."),TEXT("Ruby"),A::HoldFire,S::Cannons,D::Starboard);
    Order(TEXT("heldout_crew_port"),TEXT("Duncan, go and crew the port cannons."),TEXT("Duncan"),A::Man,S::Cannons,D::Port);
    Order(TEXT("heldout_crew_starboard"),TEXT("Ruby, take your place at the starboard battery."),TEXT("Ruby"),A::Man,S::Cannons,D::Starboard);
    Order(TEXT("heldout_discharge_port"),TEXT("Duncan, let the port guns fire now."),TEXT("Duncan"),A::Fire,S::Cannons,D::Port);
    Order(TEXT("heldout_fire_unrestricted"),TEXT("Duncan, keep firing whenever you have a shot."),TEXT("Duncan"),A::Fire,S::Cannons,D::None);
    Order(TEXT("heldout_left"),TEXT("Rex, steer the ship left."),TEXT("Rex"),A::TurnPort,S::Helm,D::Port);
    Order(TEXT("heldout_right"),TEXT("Rex, swing the bow to the right."),TEXT("Rex"),A::TurnStarboard,S::Helm,D::Starboard);
    Order(TEXT("heldout_mend"),TEXT("Ruby, mend the torn sails for me."),TEXT("Ruby"),A::Repair,S::Rigging,D::None);
    Order(TEXT("heldout_put_out"),TEXT("Ruby, put out the blaze beside the mast."),TEXT("Ruby"),A::Extinguish,S::Fire,D::None);
    Order(TEXT("heldout_damage_report"),TEXT("Duncan, give me a damage report."),TEXT("Duncan"),A::Report,S::None,D::None);
    Order(TEXT("heldout_nonorder"),TEXT("I used to sail these waters as a child."),TEXT(""),A::None,S::None,D::None);

    auto Acknowledge = [this](const TCHAR* Crew,const TCHAR* OrderText,const TCHAR* ActionDescription,const TCHAR* Forbidden)
    {
        FReviewCase& Case=Cases.AddDefaulted_GetRef();
        Case.Label=FString(TEXT("ack_"))+Crew; Case.bAcknowledgement=true; Case.ForbiddenDirection=Forbidden;
        Case.System=DCDialoguePrompts::AcknowledgementSystem(Voice->PersonaFor(Crew));
        Case.User=FString::Printf(TEXT("Captain's order: \"%s\"\nWhat you are doing: %s\nSituation: open sea in sunshine; hull at 100 percent"),OrderText,ActionDescription);
    };
    Acknowledge(TEXT("Rex"),TEXT("Rex, hard to starboard"),TEXT("Hard to starboard, aye!"),TEXT("port"));
    Acknowledge(TEXT("Duncan"),TEXT("Duncan, man the port guns"),TEXT("Port guns manned and ready."),TEXT("starboard"));
    Acknowledge(TEXT("Ruby"),TEXT("Ruby, repair the rigging"),TEXT("Aloft and mending, aye."),TEXT(""));
    UE_LOG(LogDC,Display,TEXT("DIALOGUE_REVIEW_WAIT backend=%s cases=%d report=%s"),*Backend,Cases.Num(),*ReportPath);
}

void ADCDialogueReviewGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bCompleted) { return; }
    const double Now=FPlatformTime::Seconds();
    if (!bReady)
    {
        if (Voice && Voice->IsLLMReady())
        {
            bReady=true; ReadyMilliseconds=(Now-Started)*1000.0;
            UE_LOG(LogDC,Display,TEXT("DIALOGUE_REVIEW_READY backend=%s milliseconds=%.0f"),*Backend,ReadyMilliseconds);
        }
        else if (Now-Started>90.0) { AddHarnessFailure(TEXT("Language model did not become ready within 90 seconds")); Finish(); return; }
        else { return; }
    }
    if (bPending)
    {
        // Transport callbacks normally enforce their own deadlines. Stop if one never arrives.
        if (Now-RequestStarted>RequestTimeout+1.0)
        {
            CompleteCase(CaseIndex,false,TEXT(""));
            AddHarnessFailure(TEXT("Backend failed to deliver its bounded request callback"));
            Finish();
        }
        return;
    }
    if (CaseIndex>=Cases.Num()) { Finish(); return; }
    StartCase();
}

void ADCDialogueReviewGameMode::StartCase()
{
    const FReviewCase& Case=Cases[CaseIndex];
    bPending=true; RequestStarted=FPlatformTime::Seconds();
    RequestTimeout=Case.bAcknowledgement ? 1.5f : 4.f;
    const int32 SubmittedIndex=CaseIndex;
    TWeakObjectPtr<ADCDialogueReviewGameMode> Weak(this);
    Voice->RequestDialogue(Case.System,Case.User,Case.bAcknowledgement ? 32 : 80,
        Case.bAcknowledgement ? .6f : 0.f,!Case.bAcknowledgement,RequestTimeout,
        [Weak,SubmittedIndex](bool bOK,const FString& Output)
        {
            ADCDialogueReviewGameMode* Review=Weak.Get();
            if (!Review || Review->bCompleted || !Review->bPending || Review->CaseIndex!=SubmittedIndex) { return; }
            Review->CompleteCase(SubmittedIndex,bOK,Output);
        });
}

void ADCDialogueReviewGameMode::CompleteCase(int32 Index,bool bOK,const FString& Output)
{
    const FReviewCase& Case=Cases[Index];
    const double Elapsed=(FPlatformTime::Seconds()-RequestStarted)*1000.0;
    bool bPass=bOK;
    FString Reason=bOK ? TEXT("") : TEXT("Backend returned failure or timed out; no canned fallback is accepted");
    if (bOK && Elapsed>RequestTimeout*1000.0+100.0)
    {
        bPass=false; Reason=TEXT("Successful response arrived after the production deadline plus 100ms callback tolerance");
    }
    TSharedRef<FJsonObject> Row=MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("label"),Case.Label);
    Row->SetStringField(TEXT("kind"),Case.bAcknowledgement ? TEXT("acknowledgement") : TEXT("order"));
    Row->SetStringField(TEXT("prompt"),Case.User);
    Row->SetStringField(TEXT("raw_output"),Output);
    Row->SetBoolField(TEXT("callback_success"),bOK);
    Row->SetNumberField(TEXT("latency_ms"),Elapsed);
    Row->SetNumberField(TEXT("deadline_ms"),RequestTimeout*1000.0);
    if (Case.bAcknowledgement)
    {
        FString Line=Output;
        const int32 ThinkEnd=Line.Find(TEXT("</think>"));
        if (ThinkEnd!=INDEX_NONE) { Line=Line.Mid(ThinkEnd+8); }
        Line=Line.TrimStartAndEnd().Replace(TEXT("\""),TEXT("")).Replace(TEXT("*"),TEXT(""));
        TArray<FString> Words; Line.ParseIntoArrayWS(Words);
        FString WordText=Line.ToLower();
        for (TCHAR& C:WordText) { if (!FChar::IsAlnum(C)) { C=TEXT(' '); } }
        WordText=TEXT(" ")+WordText+TEXT(" ");
        const bool bOpposite=!Case.ForbiddenDirection.IsEmpty() && WordText.Contains(TEXT(" ")+Case.ForbiddenDirection+TEXT(" "));
        if (Line.IsEmpty() || Line.Len()>120 || Words.Num()>10 || Line.Contains(TEXT("\n")) || Line.Contains(TEXT("{")) || Line.Contains(TEXT("<think>")) || bOpposite)
        {
            bPass=false; if (Reason.IsEmpty()) { Reason=TEXT("Acknowledgement violates spoken-line length/format/direction checks"); }
        }
        Row->SetStringField(TEXT("spoken_line"),Line);
        Row->SetStringField(TEXT("validation_scope"),TEXT("Nonempty single spoken line, <=10 words/120 characters, no opposite explicit direction. Style and full semantic quality require reviewing raw output."));
    }
    else
    {
        FDCIntent Actual;
        const bool bParsed=DCLanguageModelResponse::ParseIntent(Output,Case.User,Actual);
        if (!bParsed || Actual.bValid!=Case.Expected.bValid || Actual.CrewName!=Case.Expected.CrewName ||
            Actual.Action!=Case.Expected.Action || Actual.Station!=Case.Expected.Station || Actual.Side!=Case.Expected.Side)
        {
            bPass=false; if (Reason.IsEmpty()) { Reason=bParsed ? TEXT("Intent differs from expected order") : TEXT("Response failed strict intent validation"); }
        }
        Row->SetBoolField(TEXT("parsed"),bParsed);
        Row->SetObjectField(TEXT("expected"),DescribeIntent(Case.Expected));
        Row->SetObjectField(TEXT("actual"),DescribeIntent(Actual));
    }
    Row->SetBoolField(TEXT("pass"),bPass);
    Row->SetStringField(TEXT("reason"),Reason);
    Results.Add(MakeShared<FJsonValueObject>(Row));
    if (!bPass) { ++Failures; }
    UE_LOG(LogDC,Display,TEXT("DIALOGUE_REVIEW_CASE backend=%s case=%s pass=%d milliseconds=%.0f output=%s"),*Backend,*Case.Label,bPass,Elapsed,*Output.Replace(TEXT("\n"),TEXT(" ")));
    bPending=false; ++CaseIndex;
}

void ADCDialogueReviewGameMode::AddHarnessFailure(const FString& Reason)
{
    ++Failures;
    TSharedRef<FJsonObject> Row=MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("kind"),TEXT("harness")); Row->SetBoolField(TEXT("pass"),false); Row->SetStringField(TEXT("reason"),Reason);
    Results.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogDC,Error,TEXT("DIALOGUE_REVIEW_FAILURE backend=%s reason=%s"),*Backend,*Reason);
}

void ADCDialogueReviewGameMode::Finish()
{
    if (bCompleted) { return; }
    bCompleted=true; bPending=false;
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("backend"),Backend);
    Report->SetStringField(TEXT("selected_model"),Voice ? Voice->LLMModel : TEXT("unknown"));
    Report->SetBoolField(TEXT("selected_model_ready"),Voice && Voice->IsLLMReady());
    Report->SetNumberField(TEXT("readiness_ms"),ReadyMilliseconds);
    Report->SetNumberField(TEXT("elapsed_ms"),(FPlatformTime::Seconds()-Started)*1000.0);
    Report->SetNumberField(TEXT("planned_cases"),Cases.Num());
    Report->SetNumberField(TEXT("completed_cases"),CaseIndex);
    Report->SetNumberField(TEXT("failures"),Failures);
    Report->SetBoolField(TEXT("success"),Failures==0 && CaseIndex==Cases.Num());
    Report->SetArrayField(TEXT("results"),Results);
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath),true);
    if (!FFileHelper::SaveStringToFile(Json,*ReportPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        ++Failures; UE_LOG(LogDC,Error,TEXT("DIALOGUE_REVIEW_FAILURE could not write report: %s"),*ReportPath);
    }
    if (Failures==0 && CaseIndex==Cases.Num()) { UE_LOG(LogDC,Display,TEXT("DIALOGUE_REVIEW_SUCCESS backend=%s total=%d"),*Backend,CaseIndex); }
    else { UE_LOG(LogDC,Error,TEXT("DIALOGUE_REVIEW_FAILED backend=%s completed=%d total=%d failures=%d"),*Backend,CaseIndex,Cases.Num(),Failures); }
    FPlatformMisc::RequestExitWithStatus(false,Failures ? 1 : 0);
}

void ADCDialogueReviewGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    bCompleted=true; bPending=false;
    // ADCVoice's normal world shutdown owns and stops only the sidecars it launched.
    Super::EndPlay(Reason);
}
