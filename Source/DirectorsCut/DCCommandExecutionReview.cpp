#include "DCCommandExecutionReview.h"
#include "DCCrew.h"
#include "DCVoice.h"
#include "DCTroubles.h"
#include "DCTreadmill.h"
#include "DCRideDirector.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace DCCommandExecutionReviewPrivate
{
    using S=EDCStationType; using D=EDCSide;
    struct FCase { const TCHAR* Text; const TCHAR* Crew; S Station; D Side; int32 Hold; int32 Steer; };
    const FCase Cases[]={
        {TEXT("Rex, man the port cannons"),TEXT("Rex"),S::Cannons,D::Port,-1,0},
        {TEXT("Rex, fire at will"),TEXT("Rex"),S::Cannons,D::Port,0,0},
        {TEXT("Rex, stop shooting"),TEXT("Rex"),S::Cannons,D::Port,1,0},
        {TEXT("Rex, could you not fire?"),TEXT("Rex"),S::Cannons,D::Port,1,0},
        {TEXT("Rex, take the wheel"),TEXT("Rex"),S::Helm,D::None,-1,0},
        {TEXT("Rex, hard to port"),TEXT("Rex"),S::Helm,D::None,-1,-1},
        {TEXT("Rex, hard to starboard"),TEXT("Rex"),S::Helm,D::None,-1,1},
        {TEXT("Rex, steady as she goes"),TEXT("Rex"),S::Helm,D::None,-1,0},
        {TEXT("Ruby, repair the rigging"),TEXT("Ruby"),S::Rigging,D::None,-1,0},
        {TEXT("Duncan, man port guns"),TEXT("Duncan"),S::Cannons,D::Port,-1,0},
        {TEXT("Duncan, hold fire"),TEXT("Duncan"),S::Cannons,D::Port,1,0},
        {TEXT("Duncan, report"),TEXT("Duncan"),S::Cannons,D::Port,-1,0},
        {TEXT("Ruby, man starboard guns"),TEXT("Ruby"),S::Cannons,D::Starboard,-1,0},
        {TEXT("Rex, get the flames."),TEXT("Rex"),S::Fire,D::None,-1,0},
        {TEXT("Rex, get the fire."),TEXT("Rex"),S::Fire,D::None,-1,0},
        {TEXT("Rex, kill those fires."),TEXT("Rex"),S::Fire,D::None,-1,0},
        {TEXT("Rex, kill this fire."),TEXT("Rex"),S::Fire,D::None,-1,0},
        {TEXT("Hey Wrecks, put those fires out"),TEXT("Rex"),S::Fire,D::None,-1,0},
        {TEXT("Rex, could you please deal with the flames on the port side?"),TEXT("Rex"),S::Fire,D::None,-1,0}
    };
}

bool UDCCommandExecutionReview::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World=Cast<UWorld>(Outer);
    return World && World->WorldType==EWorldType::Game &&
        World->GetOutermost()->GetName()==TEXT("/Game/DC/Maps/L_Ride") &&
        FParse::Param(FCommandLine::Get(),TEXT("DCCommandExecutionReview")) && Super::ShouldCreateSubsystem(Outer);
}
void UDCCommandExecutionReview::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection); Started=FPlatformTime::Seconds();
}
TStatId UDCCommandExecutionReview::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UDCCommandExecutionReview,STATGROUP_Tickables);
}
void UDCCommandExecutionReview::Finish(const FString& Error)
{
    if (bDone) { return; } bDone=true;
    UE_LOG(LogDC,Display,TEXT("COMMAND_EXECUTION_%s passed=%d cannon_walk=%d error=%s"),
        Error.IsEmpty()?TEXT("SUCCESS"):TEXT("FAILED"),Passed,bMoved,*Error);
    FPlatformMisc::RequestExitWithStatus(false,Error.IsEmpty()?0:1);
}
void UDCCommandExecutionReview::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    using namespace DCCommandExecutionReviewPrivate;
    if (bDone) { return; }
    const double Now=FPlatformTime::Seconds();
    if (Now-Started>150.0) { Finish(TEXT("Overall timeout")); return; }
    auto* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>();
    auto* Director=ADCRideDirector::Get(this);
    auto* Troubles=ADCTroubles::Get(this);
    auto* Treadmill=ADCTreadmill::Get(this);
    auto* Voice=ADCVoice::Get(this);
    if (Director) { Director->TimeScale=0.f; }
    if (!Crew || Crew->GetCrew().Num()!=3 || !Director || !Troubles || !Treadmill || !Voice ||
        (Voice->bUseLLM && !Voice->IsLLMReady()) || Now-Started<3.0) { return; }
    auto Member=[Crew](const TCHAR* Name)->ADCCrewMember*
    {
        for (ADCCrewMember* M:Crew->GetCrew()) { if (M && M->CrewName==Name) { return M; } }
        return nullptr;
    };
    ADCCrewMember* Rex=Member(TEXT("Rex"));
    if (!Rex) { Finish(TEXT("Missing Rex")); return; }
    if (bFireWalk)
    {
        bMoved |= FVector::Dist2D(Rex->DeckPos,CannonStart)>100.f;
        if (Troubles->NumFires()==0)
        {
            if (!bMoved || Director->Score-ScoreBefore<99.f) { Finish(TEXT("Fires vanished without crew movement and two scored extinguishes")); return; }
            ++Passed; Finish(FString()); return;
        }
        if (Now-CaseStarted>60.0) { Finish(TEXT("Rex did not walk from cannon and extinguish both fires")); }
        return;
    }
    if (CaseIndex>=UE_ARRAY_COUNT(Cases))
    {
        if (!bVoiceChecked)
        {
            if (Voice->HasIssuedSuccessfulVoiceCommand()) { Finish(TEXT("Typed review commands counted as microphone input")); return; }
            Voice->OnHeard(TEXT("[BLANK_AUDIO]"),true);
            Voice->OnHeard(TEXT("Rex, tell me about the fog"),true);
            if (Voice->HasIssuedSuccessfulVoiceCommand()) { Finish(TEXT("Noise/conversation counted as microphone command")); return; }
            Voice->ResetPendingSpeech();
            const int32 Helm=Crew->FindStation(S::Helm,D::None);
            FDCStation* HelmStation=Crew->GetStation(Helm);
            if (!HelmStation) { Finish(TEXT("Missing helm for voice success check")); return; }
            HelmStation->bPlayerOperated=true;
            Voice->OnHeard(TEXT("Rex, take the wheel"),true);
            HelmStation->bPlayerOperated=false;
            if (Voice->HasIssuedSuccessfulVoiceCommand()) { Finish(TEXT("Rejected occupied-station order counted as success")); return; }
            Voice->OnHeard(TEXT("Rex, take the wheel"),false);
            if (Voice->HasIssuedSuccessfulVoiceCommand()) { Finish(TEXT("WAV/test transcript counted as microphone command")); return; }
            Voice->OnHeard(TEXT("Rex, take the wheel"),true);
            if (!Voice->HasIssuedSuccessfulVoiceCommand()) { Finish(TEXT("Accepted microphone transcript did not count")); return; }
            bVoiceChecked=true;
            UE_LOG(LogDC,Display,TEXT("VOICE_COMMAND_SUCCESS_REVIEW passed=1 typed_ignored=1 chat_ignored=1 rejected_ignored=1 wav_ignored=1 mic_dispatch=1"));
            Voice->ResetPendingSpeech();
        }
        Troubles->ResetEncounterTroubles(); Troubles->FireBurnTime=120.f;
        if (Troubles->DeckPoints.Num()<2) { Finish(TEXT("Missing authored fire points")); return; }
        Troubles->StartFire(Troubles->DeckPoints[0]);
        for (int32 Index=1;Index<Troubles->DeckPoints.Num() && Troubles->NumFires()<2;++Index)
        {
            Troubles->StartFire(Troubles->DeckPoints[Index]);
        }
        if (Troubles->NumFires()!=2) { Finish(TEXT("Could not create two distinct fires")); return; }
        const int32 Gun=Crew->FindCrewGun(D::Port,TEXT("Rex"));
        FDCStation* Station=Crew->GetStation(Gun);
        if (!Station) { Finish(TEXT("Missing Rex cannon")); return; }
        Rex->AssignStation(Gun); Rex->DeckPos=Station->Location+FVector(0,0,90); Rex->State=EDCCrewState::Operating;
        CannonStart=Rex->DeckPos; ScoreBefore=Director->Score; CaseStarted=Now;
        Crew->ExecuteOrder(TEXT("Rex, kill those fires."));
        const FDCStation* Assigned=Crew->GetStation(Rex->StationIndex);
        if (!Assigned || Assigned->Type!=S::Fire || Station->Operator.Get()==Rex) { Finish(TEXT("Final fire order failed to release cannon")); return; }
        bFireWalk=true;
        UE_LOG(LogDC,Display,TEXT("COMMAND_EXECUTION_FIRE_WALK fires=2 from=%s"),*CannonStart.ToCompactString());
        return;
    }
    const FCase& Case=Cases[CaseIndex];
    ADCCrewMember* M=Member(Case.Crew);
    if (!M) { Finish(TEXT("Missing addressed crew member")); return; }
    if (!bPending)
    {
        if (Case.Station==S::Fire)
        {
            const int32 Gun=Crew->FindCrewGun(D::Port,Case.Crew);
            if (!Crew->GetStation(Gun)) { Finish(TEXT("Missing cannon setup station")); return; }
            M->AssignStation(Gun);
        }
        if (Case.Hold>=0) { M->SetHoldFire(Case.Hold==0); }
        if (Case.Steer!=0)
        {
            const int32 Helm=Crew->FindStation(S::Helm,D::None);
            FDCStation* Station=Crew->GetStation(Helm);
            if (!Station) { Finish(TEXT("Missing helm")); return; }
            M->DeckPos=Station->Location+FVector(0,0,90); M->State=EDCCrewState::Operating;
            Treadmill->SetSteer(0.f);
        }
        PreviousStation=M->StationIndex; OrdersBefore=Crew->OrdersGiven; CaseStarted=Now;
        Crew->ExecuteOrder(Case.Text); bPending=true; return;
    }
    if (Now-CaseStarted>8.0) { Finish(FString(TEXT("Order failed: "))+Case.Text); return; }
    const FDCStation* Assigned=Crew->GetStation(M->StationIndex);
    if (!Assigned || Assigned->Type!=Case.Station || (Case.Side!=D::None && Assigned->Side!=Case.Side)) { return; }
    if (Case.Hold>=0 && M->IsHoldingFire()!=(Case.Hold==1)) { return; }
    if (Case.Steer && Treadmill->GetSteer()*Case.Steer<0.9f) { return; }
    if (Crew->OrdersGiven!=OrdersBefore+1) { Finish(FString(TEXT("Order routed into conversation: "))+Case.Text); return; }
    if (Case.Station==S::Fire)
    {
        const FDCStation* Old=Crew->GetStation(PreviousStation);
        if (Old && Old->Operator.Get()==M) { Finish(TEXT("Cannon retained its operator after fire assignment")); return; }
    }
    UE_LOG(LogDC,Display,TEXT("COMMAND_EXECUTION_CASE index=%d crew=%s station=%d text=%s"),CaseIndex,Case.Crew,M->StationIndex,Case.Text);
    ++Passed; ++CaseIndex; bPending=false;
}
