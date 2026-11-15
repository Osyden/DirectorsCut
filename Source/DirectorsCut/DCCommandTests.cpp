#include "DCCrew.h"
#include "DCSpeech.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCommandCoverage, "DirectorsCut.Commands.SupportedOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCCommandCoverage::RunTest(const FString&)
{
    struct FCase { const TCHAR* Text; EDCAction Action; EDCStationType Station; EDCSide Side; const TCHAR* Crew; };
    using A = EDCAction; using S = EDCStationType; using D = EDCSide;
    const FCase Cases[] = {
        {TEXT("Rex, man the helm"), A::Man,S::Helm,D::None,TEXT("Rex")},
        {TEXT("take the wheel"), A::Man,S::Helm,D::None,TEXT("")},
        {TEXT("hard to port"), A::TurnPort,S::Helm,D::Port,TEXT("")},
        {TEXT("hard-a-starboard!"), A::TurnStarboard,S::Helm,D::Starboard,TEXT("")},
        {TEXT("Rex turn left"), A::TurnPort,S::Helm,D::Port,TEXT("Rex")},
        {TEXT("veer right"), A::TurnStarboard,S::Helm,D::Starboard,TEXT("")},
        {TEXT("Starboard!"), A::TurnStarboard,S::Helm,D::Starboard,TEXT("")},
        {TEXT("steady as she goes"), A::Steady,S::Helm,D::None,TEXT("")},
        {TEXT("midships"), A::Steady,S::Helm,D::None,TEXT("")},
        {TEXT("hold course"), A::Steady,S::Helm,D::None,TEXT("")},
        {TEXT("Duncan, man the port cannons"), A::Man,S::Cannons,D::Port,TEXT("Duncan")},
        {TEXT("Ruby get on the starboard guns"), A::Man,S::Cannons,D::Starboard,TEXT("Ruby")},
        {TEXT("man port cannons"), A::Man,S::Cannons,D::Port,TEXT("")},
        {TEXT("port battery fire"), A::Fire,S::Cannons,D::Port,TEXT("")},
        {TEXT("Duncan fire starboard"), A::Fire,S::Cannons,D::Starboard,TEXT("Duncan")},
        {TEXT("shoot left"), A::Fire,S::Cannons,D::Port,TEXT("")},
        {TEXT("fire!"), A::Fire,S::Cannons,D::None,TEXT("")},
        {TEXT("fire at will"), A::Fire,S::Cannons,D::None,TEXT("")},
        {TEXT("Duncan, fire at will"), A::Fire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("port battery fire at will"), A::Fire,S::Cannons,D::Port,TEXT("")},
        {TEXT("starboard guns fire at will"), A::Fire,S::Cannons,D::Starboard,TEXT("")},
        {TEXT("Duncan, don't fire at will"), A::HoldFire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("Wrecks, take the helm"), A::Man,S::Helm,D::None,TEXT("Rex")},
        {TEXT("Recks, hard to port"), A::TurnPort,S::Helm,D::Port,TEXT("Rex")},
        {TEXT("Dunkin, fire at will"), A::Fire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("Dunken, hold fire"), A::HoldFire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("Rubie, repair the rigging"), A::Repair,S::Rigging,D::None,TEXT("Ruby")},
        {TEXT("Finch, take the helm"), A::Man,S::Helm,D::None,TEXT("Rex")},
        {TEXT("Marlow, cease fire"), A::HoldFire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("Odessa, report"), A::Report,S::None,D::None,TEXT("Ruby")},
        {TEXT("hold your fire"), A::HoldFire,S::Cannons,D::None,TEXT("")},
        {TEXT("Duncan cease fire"), A::HoldFire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("stop firing starboard"), A::HoldFire,S::Cannons,D::Starboard,TEXT("")},
        {TEXT("Duncan, don't fire"), A::HoldFire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("do not shoot"), A::HoldFire,S::Cannons,D::None,TEXT("")},
        {TEXT("Ruby put out the fire"), A::Extinguish,S::Fire,D::None,TEXT("Ruby")},
        {TEXT("extinguish the flames"), A::Extinguish,S::Fire,D::None,TEXT("")},
        {TEXT("douse that fire"), A::Extinguish,S::Fire,D::None,TEXT("")},
        {TEXT("the deck is on fire"), A::Extinguish,S::Fire,D::None,TEXT("")},
        {TEXT("Ruby repair the rigging"), A::Repair,S::Rigging,D::None,TEXT("Ruby")},
        {TEXT("mend the sails"), A::Repair,S::Rigging,D::None,TEXT("")},
        {TEXT("fix the ropes"), A::Repair,S::Rigging,D::None,TEXT("")},
        {TEXT("report"), A::Report,S::None,D::None,TEXT("")},
        {TEXT("Rex status"), A::Report,S::None,D::None,TEXT("Rex")},
        {TEXT("report on the cannons"), A::Report,S::None,D::None,TEXT("")},
        {TEXT("report on the fire"), A::Report,S::None,D::None,TEXT("")},
        {TEXT("how are we doing"), A::Report,S::None,D::None,TEXT("")},
        {TEXT("hello captain"), A::None,S::None,D::None,TEXT("")},
        {TEXT("fireworks look lovely"), A::None,S::None,D::None,TEXT("")},
        {TEXT("the portrait"), A::None,S::None,D::None,TEXT("")},
        {TEXT("bright sky"), A::None,S::None,D::None,TEXT("")},
    };
    for (const FCase& Case : Cases)
    {
        const FDCIntent Intent = DCIntentParser::Parse(Case.Text, {TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")});
        TestEqual(FString(Case.Text) + TEXT(" action"), int32(Intent.Action), int32(Case.Action));
        TestEqual(FString(Case.Text) + TEXT(" station"), int32(Intent.Station), int32(Case.Station));
        TestEqual(FString(Case.Text) + TEXT(" side"), int32(Intent.Side), int32(Case.Side));
        TestEqual(FString(Case.Text) + TEXT(" crew"), Intent.CrewName, FString(Case.Crew));
        TestEqual(FString(Case.Text) + TEXT(" valid"), Intent.bValid, Case.Action != A::None);
    }
    TestEqual(TEXT("Wrecks as scenery is not a crew address"), DCSpeech::NormalizeCrewAddress(TEXT("wrecks ahead")), FString(TEXT("wrecks ahead")));
    TestEqual(TEXT("No name substitution inside prose"), DCSpeech::NormalizeCrewAddress(TEXT("there are wrecks ahead")), FString(TEXT("there are wrecks ahead")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCSpokenCommandCoverage, "DirectorsCut.Commands.SpokenOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCSpokenCommandCoverage::RunTest(const FString&)
{
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectSavedDir() / TEXT("ACE/SpokenOrders/transcripts.json"))))
    {
        AddError(TEXT("Run Tools/ACE/verify_spoken_orders.py before the spoken-order test."));
        return false;
    }
    TArray<TSharedPtr<FJsonValue>> Cases;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Cases)) { return false; }
    TestEqual(TEXT("Number of speech round trips"), Cases.Num(), 20);
    for (const TSharedPtr<FJsonValue>& Case : Cases)
    {
        const auto Row = Case->AsObject();
        const FString Text = Row->GetStringField(TEXT("text"));
        const FDCIntent Intent = DCIntentParser::Parse(Text, {TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")});
        TestEqual(Text + TEXT(" action"), int32(Intent.Action), int32(Row->GetNumberField(TEXT("action"))));
        TestEqual(Text + TEXT(" station"), int32(Intent.Station), int32(Row->GetNumberField(TEXT("station"))));
        TestEqual(Text + TEXT(" side"), int32(Intent.Side), int32(Row->GetNumberField(TEXT("side"))));
        TestEqual(Text + TEXT(" crew"), Intent.CrewName, Row->GetStringField(TEXT("crew")));
        TestTrue(Text + TEXT(" valid"), Intent.bValid);
    }
    return true;
}
#endif
