#include "DCCrew.h"
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
        {TEXT("Finch, man the helm"), A::Man,S::Helm,D::None,TEXT("Finch")},
        {TEXT("take the wheel"), A::Man,S::Helm,D::None,TEXT("")},
        {TEXT("hard to port"), A::TurnPort,S::Helm,D::Port,TEXT("")},
        {TEXT("hard-a-starboard!"), A::TurnStarboard,S::Helm,D::Starboard,TEXT("")},
        {TEXT("Finch turn left"), A::TurnPort,S::Helm,D::Port,TEXT("Finch")},
        {TEXT("veer right"), A::TurnStarboard,S::Helm,D::Starboard,TEXT("")},
        {TEXT("Starboard!"), A::TurnStarboard,S::Helm,D::Starboard,TEXT("")},
        {TEXT("steady as she goes"), A::Steady,S::Helm,D::None,TEXT("")},
        {TEXT("midships"), A::Steady,S::Helm,D::None,TEXT("")},
        {TEXT("hold course"), A::Steady,S::Helm,D::None,TEXT("")},
        {TEXT("Marlow, man the port cannons"), A::Man,S::Cannons,D::Port,TEXT("Marlow")},
        {TEXT("Odessa get on the starboard guns"), A::Man,S::Cannons,D::Starboard,TEXT("Odessa")},
        {TEXT("man port cannons"), A::Man,S::Cannons,D::Port,TEXT("")},
        {TEXT("port battery fire"), A::Fire,S::Cannons,D::Port,TEXT("")},
        {TEXT("Marlow fire starboard"), A::Fire,S::Cannons,D::Starboard,TEXT("Marlow")},
        {TEXT("shoot left"), A::Fire,S::Cannons,D::Port,TEXT("")},
        {TEXT("fire!"), A::Fire,S::Cannons,D::None,TEXT("")},
        {TEXT("hold your fire"), A::HoldFire,S::Cannons,D::None,TEXT("")},
        {TEXT("Marlow cease fire"), A::HoldFire,S::Cannons,D::None,TEXT("Marlow")},
        {TEXT("stop firing starboard"), A::HoldFire,S::Cannons,D::Starboard,TEXT("")},
        {TEXT("Marlow, don't fire"), A::HoldFire,S::Cannons,D::None,TEXT("Marlow")},
        {TEXT("do not shoot"), A::HoldFire,S::Cannons,D::None,TEXT("")},
        {TEXT("Odessa put out the fire"), A::Extinguish,S::Fire,D::None,TEXT("Odessa")},
        {TEXT("extinguish the flames"), A::Extinguish,S::Fire,D::None,TEXT("")},
        {TEXT("douse that fire"), A::Extinguish,S::Fire,D::None,TEXT("")},
        {TEXT("the deck is on fire"), A::Extinguish,S::Fire,D::None,TEXT("")},
        {TEXT("Odessa repair the rigging"), A::Repair,S::Rigging,D::None,TEXT("Odessa")},
        {TEXT("mend the sails"), A::Repair,S::Rigging,D::None,TEXT("")},
        {TEXT("fix the ropes"), A::Repair,S::Rigging,D::None,TEXT("")},
        {TEXT("report"), A::Report,S::None,D::None,TEXT("")},
        {TEXT("Finch status"), A::Report,S::None,D::None,TEXT("Finch")},
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
        const FDCIntent Intent = DCIntentParser::Parse(Case.Text, {TEXT("Finch"),TEXT("Marlow"),TEXT("Odessa")});
        TestEqual(FString(Case.Text) + TEXT(" action"), int32(Intent.Action), int32(Case.Action));
        TestEqual(FString(Case.Text) + TEXT(" station"), int32(Intent.Station), int32(Case.Station));
        TestEqual(FString(Case.Text) + TEXT(" side"), int32(Intent.Side), int32(Case.Side));
        TestEqual(FString(Case.Text) + TEXT(" crew"), Intent.CrewName, FString(Case.Crew));
        TestEqual(FString(Case.Text) + TEXT(" valid"), Intent.bValid, Case.Action != A::None);
    }
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
    TestEqual(TEXT("Number of speech round trips"), Cases.Num(), 13);
    for (const TSharedPtr<FJsonValue>& Case : Cases)
    {
        const auto Row = Case->AsObject();
        const FString Text = Row->GetStringField(TEXT("text"));
        const FDCIntent Intent = DCIntentParser::Parse(Text, {TEXT("Finch"),TEXT("Marlow"),TEXT("Odessa")});
        TestEqual(Text + TEXT(" action"), int32(Intent.Action), int32(Row->GetNumberField(TEXT("action"))));
        TestEqual(Text + TEXT(" station"), int32(Intent.Station), int32(Row->GetNumberField(TEXT("station"))));
        TestEqual(Text + TEXT(" side"), int32(Intent.Side), int32(Row->GetNumberField(TEXT("side"))));
        TestEqual(Text + TEXT(" crew"), Intent.CrewName, Row->GetStringField(TEXT("crew")));
        TestTrue(Text + TEXT(" valid"), Intent.bValid);
    }
    return true;
}
#endif
