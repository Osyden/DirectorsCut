#include "DCLanguageModelResponse.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDialogueResponses, "DirectorsCut.Dialogue.ResponseValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDialogueResponses::RunTest(const FString&)
{
    using A = EDCAction; using S = EDCStationType; using D = EDCSide;
    auto Json = [](const TCHAR* Crew,const TCHAR* Station,const TCHAR* Side,const TCHAR* Action) {
        return FString::Printf(TEXT("{\"crew\":\"%s\",\"station\":\"%s\",\"side\":\"%s\",\"action\":\"%s\"}"),Crew,Station,Side,Action);
    };
    struct FCase { const TCHAR* Order; const TCHAR* Crew; const TCHAR* Station; const TCHAR* Side; const TCHAR* Action; A ExpectedAction; S ExpectedStation; D ExpectedSide; const TCHAR* ExpectedCrew; };
    const FCase Valid[] = {
        {TEXT("Rex, get us round to the left"),TEXT("rex"),TEXT("none"),TEXT("none"),TEXT("turn_port"),A::TurnPort,S::Helm,D::Port,TEXT("Rex")},
        {TEXT("Rex, bring her right"),TEXT("Rex"),TEXT("helm"),TEXT("starboard"),TEXT("turn_starboard"),A::TurnStarboard,S::Helm,D::Starboard,TEXT("Rex")},
        {TEXT("Duncan, fire at will"),TEXT("Duncan"),TEXT("cannons"),TEXT("port"),TEXT("fire"),A::Fire,S::Cannons,D::Port,TEXT("Duncan")},
        {TEXT("Duncan, don't fire"),TEXT("Duncan"),TEXT("none"),TEXT("none"),TEXT("hold_fire"),A::HoldFire,S::Cannons,D::None,TEXT("Duncan")},
        {TEXT("Ruby, those flames need water"),TEXT("Rubie"),TEXT("none"),TEXT("none"),TEXT("extinguish"),A::Extinguish,S::Fire,D::None,TEXT("Ruby")},
        {TEXT("Get the ropes fixed"),TEXT(""),TEXT("none"),TEXT("none"),TEXT("repair"),A::Repair,S::Rigging,D::None,TEXT("")},
        {TEXT("Hold our heading"),TEXT("Rex"),TEXT("none"),TEXT("none"),TEXT("steady"),A::Steady,S::Helm,D::None,TEXT("Rex")},
        {TEXT("Ruby, take fire duty"),TEXT("Ruby"),TEXT("fire"),TEXT("none"),TEXT("man"),A::Extinguish,S::Fire,D::None,TEXT("Ruby")},
        {TEXT("Ruby, take the sails"),TEXT("Ruby"),TEXT("rigging"),TEXT("none"),TEXT("man"),A::Repair,S::Rigging,D::None,TEXT("Ruby")},
        {TEXT("Duncan, take the right guns"),TEXT("Duncan"),TEXT("cannons"),TEXT("starboard"),TEXT("man"),A::Man,S::Cannons,D::Starboard,TEXT("Duncan")},
        {TEXT("Rex, is the ship all right?"),TEXT("Rex"),TEXT("none"),TEXT("none"),TEXT("report"),A::Report,S::None,D::None,TEXT("Rex")},
        {TEXT("Lovely weather"),TEXT(""),TEXT("none"),TEXT("none"),TEXT("none"),A::None,S::None,D::None,TEXT("")}
    };
    for (const FCase& Case : Valid)
    {
        FDCIntent Intent;
        TestTrue(Case.Order,DCLanguageModelResponse::ParseIntent(Json(Case.Crew,Case.Station,Case.Side,Case.Action),Case.Order,Intent));
        TestEqual(FString(Case.Order)+TEXT(" action"),int32(Intent.Action),int32(Case.ExpectedAction));
        TestEqual(FString(Case.Order)+TEXT(" station"),int32(Intent.Station),int32(Case.ExpectedStation));
        TestEqual(FString(Case.Order)+TEXT(" side"),int32(Intent.Side),int32(Case.ExpectedSide));
        TestEqual(FString(Case.Order)+TEXT(" crew"),Intent.CrewName,FString(Case.ExpectedCrew));
        TestEqual(FString(Case.Order)+TEXT(" valid"),Intent.bValid,Case.ExpectedAction!=A::None);
        TestEqual(FString(Case.Order)+TEXT(" raw order"),Intent.RawText,FString(Case.Order));
    }
    const FString ValidJson = Json(TEXT("Duncan"),TEXT("cannons"),TEXT("port"),TEXT("hold_fire"));
    FDCIntent Intent;
    TestTrue(TEXT("Complete think block and JSON code fence"),DCLanguageModelResponse::ParseIntent(TEXT("<think>reasoning</think>\n```json\n")+ValidJson+TEXT("\n```"),TEXT("hold fire"),Intent));
    TestEqual(TEXT("Wrapped response keeps cease-fire action"),int32(Intent.Action),int32(A::HoldFire));
    const TArray<FString> Invalid = {
        TEXT(""),TEXT("not json"),TEXT("{}"),TEXT("[]"),TEXT("{\"crew\":\"Rex\",\"station\":\"helm\",\"action\":\"man\"}"),
        TEXT("{\"crew\":null,\"station\":\"helm\",\"side\":\"none\",\"action\":\"man\"}"),
        TEXT("{\"crew\":5,\"station\":\"helm\",\"side\":\"none\",\"action\":\"man\"}"),
        Json(TEXT("Rexx"),TEXT("helm"),TEXT("none"),TEXT("man")),
        Json(TEXT("Rex"),TEXT("wheel"),TEXT("none"),TEXT("man")),
        Json(TEXT("Duncan"),TEXT("cannons"),TEXT("left"),TEXT("fire")),
        Json(TEXT("Ruby"),TEXT("rigging"),TEXT("none"),TEXT("attack")),
        Json(TEXT("Duncan"),TEXT("fire"),TEXT("none"),TEXT("fire")),
        Json(TEXT("Ruby"),TEXT("cannons"),TEXT("none"),TEXT("extinguish")),
        Json(TEXT("Rex"),TEXT("helm"),TEXT("starboard"),TEXT("turn_port")),
        Json(TEXT("Rex"),TEXT("helm"),TEXT("port"),TEXT("turn_starboard")),
        Json(TEXT("Rex"),TEXT("helm"),TEXT("port"),TEXT("steady")),
        Json(TEXT("Ruby"),TEXT("rigging"),TEXT("port"),TEXT("repair")),
        Json(TEXT("Ruby"),TEXT("fire"),TEXT("starboard"),TEXT("man")),
        Json(TEXT("Rex"),TEXT("none"),TEXT("none"),TEXT("man")),
        Json(TEXT("Rex"),TEXT("helm"),TEXT("none"),TEXT("report")),
        Json(TEXT("Duncan"),TEXT("cannons"),TEXT("none"),TEXT("none")),
        ValidJson+TEXT(" extra text"),ValidJson+ValidJson,TEXT("Here is the order: ")+ValidJson,
        TEXT("<think>unfinished ")+ValidJson,TEXT("```json\n")+ValidJson,
        ValidJson.LeftChop(1)+TEXT(",\"extra\":\"run code\"}")
    };
    for (int32 Index=0;Index<Invalid.Num();++Index)
    {
        Intent.bValid=true; Intent.Action=A::Fire; Intent.CrewName=TEXT("Duncan");
        TestFalse(FString::Printf(TEXT("Reject invalid response %d"),Index),DCLanguageModelResponse::ParseIntent(Invalid[Index],TEXT("original"),Intent));
        TestFalse(TEXT("Rejected response cannot execute"),Intent.bValid);
        TestEqual(TEXT("Rejected response clears action"),int32(Intent.Action),int32(A::None));
        TestTrue(TEXT("Rejected response clears crew"),Intent.CrewName.IsEmpty());
        TestEqual(TEXT("Rejected response retains original order"),Intent.RawText,FString(TEXT("original")));
    }

    FString Content;
    TestTrue(TEXT("Extract assistant text"),DCLanguageModelResponse::ExtractChatContent(TEXT("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":\"Aye, Captain.\"}}]}"),Content));
    TestEqual(TEXT("Assistant content preserved"),Content,FString(TEXT("Aye, Captain.")));
    const TCHAR* BadEnvelopes[] = {TEXT("{}"),TEXT("{\"choices\":[]}"),TEXT("{\"choices\":[null]}"),TEXT("{\"choices\":[5]}"),
        TEXT("{\"choices\":[{}]}"),TEXT("{\"choices\":[{\"message\":null}]}"),TEXT("{\"choices\":[{\"message\":{\"content\":null}}]}"),
        TEXT("{\"choices\":[{\"message\":{\"content\":\" \"}}]}"),TEXT("{\"choices\":[{\"message\":{\"content\":5}}]}"),TEXT("broken")};
    for (const TCHAR* Envelope : BadEnvelopes)
    {
        Content=TEXT("stale response");
        TestFalse(TEXT("Reject malformed completion envelope"),DCLanguageModelResponse::ExtractChatContent(Envelope,Content));
        TestTrue(TEXT("No stale content after extraction failure"),Content.IsEmpty());
    }
    return true;
}
#endif
