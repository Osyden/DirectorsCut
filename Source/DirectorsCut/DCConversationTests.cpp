#include "DCConversationPolicy.h"
#include "DCCrew.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFireOrderRouting,"DirectorsCut.Dialogue.FireOrderRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCFireOrderRouting::RunTest(const FString&)
{
    const TCHAR* Orders[]={TEXT("Rex, kill that fire"),TEXT("Rex, kill those fires."),TEXT("Rex, kill this fire."),
        TEXT("Rex, get the flames."),TEXT("Rex, get the fire."),TEXT("Rex, put that fire out"),
        TEXT("Rex, put those fires out"),TEXT("Rex, fight the fires"),TEXT("Rex, deal with that blaze"),
        TEXT("Rex, take care of the flames"),TEXT("Rex, handle those fires"),TEXT("Rex, smother the flames"),
        TEXT("Rex, beat out the flames"),TEXT("Rex, stamp out the fire"),TEXT("Rex, get rid of those fires"),
        TEXT("Hey Wrecks, kill the fire"),TEXT("Recks, tackle that blaze"),
        TEXT("Rex, could you please put those fires out on the port side?"),
        TEXT("Rex, would you mind killing those fires?"),TEXT("Rex, would you mind putting out the flames?")};
    for (const TCHAR* Text:Orders)
    {
        TestTrue(FString(Text)+TEXT(" explicit fire request"),DCConversationPolicy::IsFireOrder(Text));
        TestFalse(FString(Text)+TEXT(" not conversation"),DCConversationPolicy::IsConversation(Text));
        const FDCIntent Intent=DCIntentParser::Parse(Text,{TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")});
        TestTrue(FString(Text)+TEXT(" valid"),Intent.bValid);
        TestEqual(FString(Text)+TEXT(" crew"),Intent.CrewName,FString(TEXT("Rex")));
        TestEqual(FString(Text)+TEXT(" action"),int32(Intent.Action),int32(EDCAction::Extinguish));
        TestEqual(FString(Text)+TEXT(" station"),int32(Intent.Station),int32(EDCStationType::Fire));
        TestEqual(FString(Text)+TEXT(" side"),int32(Intent.Side),int32(EDCSide::None));
    }
    const TCHAR* NotFire[]={TEXT("Rex, fire at will"),TEXT("Rex, hold fire"),TEXT("Rex, get the cannons firing"),
        TEXT("Rex, tell me how to kill that fire"),TEXT("Rex, how do you put those fires out?"),
        TEXT("Rex, don't kill that fire"),TEXT("Rex, do not put out the flames"),TEXT("Rex, stop shooting"),
        TEXT("Rex, get me information about fires"),TEXT("Rex, get the fire report"),
        TEXT("Rex, killing fires is hard"),TEXT("Rex, putting out fires is tiring"),TEXT("Rex, report on the fire"),
        TEXT("Rex, kill the fire elemental"),TEXT("Rex, fight that fire creature")};
    for (const TCHAR* Text:NotFire) { TestFalse(FString(Text)+TEXT(" not extinguish override"),DCConversationPolicy::IsFireOrder(Text)); }
    for (const TCHAR* Text:{TEXT("Rex, don't kill that fire"),TEXT("Rex, do not put out the flames"),
        TEXT("Ruby, please don't extinguish the fire"),TEXT("Rex, how do you put those fires out?")})
    {
        TestTrue(FString(Text)+TEXT(" no execution route"),DCConversationPolicy::IsConversation(Text));
        TestFalse(FString(Text)+TEXT(" no fallback execution"),DCIntentParser::Parse(Text,{TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")}).bValid);
    }
    for (const TCHAR* Text:{TEXT("Rex, kill the fire elemental"),TEXT("Rex, fight that fire creature")})
    {
        TestFalse(FString(Text)+TEXT(" reaches combat order path"),DCConversationPolicy::IsConversation(Text));
        TestEqual(FString(Text)+TEXT(" combat action"),int32(DCIntentParser::Parse(Text,{TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")}).Action),int32(EDCAction::Fire));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCConversationRouting,"DirectorsCut.Dialogue.ConversationRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCConversationRouting::RunTest(const FString&)
{
    const TCHAR* Conversation[]={
        TEXT("tell me about fire"),TEXT("tell me about cannons"),TEXT("tell me about the ship"),
        TEXT("what are you doing"),TEXT("who are you"),TEXT("how do we put out fire"),
        TEXT("Ruby, tell me about Rex"),TEXT("Hey Duncan, can you tell me about your cannons?"),
        TEXT("Rex, would you explain how to take the wheel?"),TEXT("Could you tell me why we hold fire?"),
        TEXT("How would you repair the rigging?"),TEXT("What does Ruby look like?"),
        TEXT("I remember the fire on our last ship."),TEXT("Those cannons look impressive."),
        TEXT("Fire is frightening."),TEXT("Fire scares me."),TEXT("Repair costs money."),
        TEXT("Can you take me through how the cannons work?"),TEXT("Take a look at Ruby's rigging"),
        TEXT("Repairing the sails is hard"),TEXT("Fire is beautiful"),TEXT("Firing the guns is fun"),
        TEXT("Can you walk me through repairing rigging?"),TEXT("Would you walk us through how to put out the fire?"),
        TEXT("How much damage can these cannons do?"),TEXT("Is the ship all right?"),
        TEXT("Would you mind taking a look at Ruby's rigging?"),
        TEXT("Don't tell me about fire."),TEXT("Can you stop talking about the cannons?"),TEXT("Get me a story about the wheel."),
        TEXT("How do we dodge to port?"),TEXT("What does come about mean?"),TEXT("Tell me how to discharge a cannon."),
        TEXT("Why did you sink that ship?"),TEXT("When should we let them have it?"),
        TEXT("We should talk about the wheel."),TEXT("I want to know how the cannons work."),
        TEXT("Hello Ruby"),TEXT("Good morning, captain."),TEXT("Wrecks ahead"),
        TEXT("Ruby's rigging looks strong."),TEXT("Can you sing?"),TEXT(""),TEXT("   ")
    };
    for (const TCHAR* Text:Conversation) { TestTrue(FString(TEXT("Conversation: "))+Text,DCConversationPolicy::IsConversation(Text)); }
    const TCHAR* CommandCases[]={
        TEXT("can you take the wheel"),TEXT("would you mind repairing the rigging"),TEXT("please fire at will"),
        TEXT("Hey Rex, could you please turn us to port?"),TEXT("Ruby, would you be so kind as to put out the fire?"),
        TEXT("Could you not fire?"),TEXT("Would you mind not firing?"),TEXT("Ruby, I need you to mend the sails"),
        TEXT("Would you mind firing the port guns?"),TEXT("Ruby, would you mind putting out the fire?"),
        TEXT("Hey, can you take the wheel?"),TEXT("Rex, dodge left"),TEXT("Bear to larboard"),TEXT("Come about to port"),
        TEXT("Duncan, blast that ship"),TEXT("Ruby, take the cannon"),
        TEXT("Duncan, sink that ship"),
        TEXT("Duncan, discharge the starboard cannon"),TEXT("Duncan, let them have it"),TEXT("All hands, fire at will"),
        TEXT("Rex, man the helm"),TEXT("take the wheel"),TEXT("hard to port"),TEXT("hard-a-starboard!"),
        TEXT("Rex turn left"),TEXT("veer right"),TEXT("Starboard!"),TEXT("steady as she goes"),TEXT("midships"),TEXT("hold course"),
        TEXT("Duncan, man the port cannons"),TEXT("Ruby get on the starboard guns"),TEXT("man port cannons"),
        TEXT("port battery fire"),TEXT("Duncan fire starboard"),TEXT("shoot left"),TEXT("fire!"),TEXT("fire at will"),
        TEXT("Duncan, fire at will"),TEXT("port battery fire at will"),TEXT("starboard guns fire at will"),TEXT("Duncan, don't fire at will"),
        TEXT("Wrecks, take the helm"),TEXT("Recks, hard to port"),TEXT("Dunkin, fire at will"),TEXT("Dunken, hold fire"),
        TEXT("Rubie, repair the rigging"),TEXT("Finch, take the helm"),TEXT("Marlow, cease fire"),TEXT("Odessa, report"),
        TEXT("hold your fire"),TEXT("Duncan cease fire"),TEXT("stop firing starboard"),TEXT("Duncan, don't fire"),TEXT("do not shoot"),
        TEXT("Ruby put out the fire"),TEXT("extinguish the flames"),TEXT("douse that fire"),TEXT("the deck is on fire"),
        TEXT("Ruby repair the rigging"),TEXT("mend the sails"),TEXT("fix the ropes"),TEXT("report"),TEXT("Rex status"),
        TEXT("report on the cannons"),TEXT("report on the fire"),TEXT("how are we doing"),TEXT("hard port"),TEXT("Rex left"),
        TEXT("Duncan, give me a damage report"),TEXT("Duncan, let the port guns fire now"),TEXT("Rex, keep us straight ahead")
    };
    for (const TCHAR* Text:CommandCases) { TestFalse(FString(TEXT("Order: "))+Text,DCConversationPolicy::IsConversation(Text)); }
    // Original and held-out native model cases must still reach order interpretation.
    const TCHAR* ModelOrders[]={TEXT("Rex, could you take the wheel?"),TEXT("Rex, bring her round to port."),
        TEXT("Rex, turn us to starboard."),TEXT("Rex, keep us straight ahead."),TEXT("Duncan, fire at will."),
        TEXT("Duncan, stop shooting. Don't fire."),TEXT("Duncan, take charge of the port battery."),TEXT("Ruby, get on the starboard cannons."),
        TEXT("Ruby, douse those flames on deck."),TEXT("Ruby, get the damaged sails repaired."),TEXT("Rex, how much damage has the hull taken?"),
        TEXT("Duncan, cease fire on the port side."),TEXT("Ruby, do not shoot the starboard guns."),TEXT("Duncan, go and crew the port cannons."),
        TEXT("Ruby, take your place at the starboard battery."),TEXT("Duncan, let the port guns fire now."),TEXT("Duncan, keep firing whenever you have a shot."),
        TEXT("Rex, steer the ship left."),TEXT("Rex, swing the bow to the right."),TEXT("Ruby, mend the torn sails for me."),
        TEXT("Ruby, put out the blaze beside the mast."),TEXT("Duncan, give me a damage report.")};
    for (const TCHAR* Text:ModelOrders) { TestFalse(FString(TEXT("Model order: "))+Text,DCConversationPolicy::IsConversation(Text)); }
    TestTrue(TEXT("Original model nonorder"),DCConversationPolicy::IsConversation(TEXT("Good morning. Lovely weather today.")));
    TestTrue(TEXT("Held-out model nonorder"),DCConversationPolicy::IsConversation(TEXT("I used to sail these waters as a child.")));
    struct FAddress { const TCHAR* Text; const TCHAR* Crew; };
    const FAddress Addresses[]={
        {TEXT("Ruby, tell me about Rex"),TEXT("Ruby")},{TEXT("Hey, Dunkin, who are you?"),TEXT("Duncan")},
        {TEXT("Hello Recks, tell me a story"),TEXT("Rex")},{TEXT("Rubie how do the cannons work?"),TEXT("Ruby")},
        {TEXT("Rex"),TEXT("Rex")},{TEXT("Odessa, report"),TEXT("Ruby")},{TEXT("What does Ruby look like?"),TEXT("")},
        {TEXT("Tell me about Rex"),TEXT("")},{TEXT("Ruby's hair looks bright"),TEXT("")},{TEXT("Ruby is brave"),TEXT("")},
        {TEXT("Wrecks ahead"),TEXT("")},{TEXT("Hey captain, ask Duncan"),TEXT("")},{TEXT("Duncanville"),TEXT("")}
    };
    for (const FAddress& Case:Addresses) { TestEqual(Case.Text,DCConversationPolicy::AddressedCrew(Case.Text),FString(Case.Crew)); }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCConversationCleaning,"DirectorsCut.Dialogue.ConversationReplyCleaning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCConversationCleaning::RunTest(const FString&)
{
    using DCConversationPolicy::CleanReply;
    TestEqual(TEXT("Plain spoken answer"),CleanReply(TEXT("  Aye, Captain. I mind the sails.  ")),FString(TEXT("Aye, Captain. I mind the sails.")));
    TestEqual(TEXT("Remove complete reasoning and speech fence"),CleanReply(TEXT("<think>private analysis</think>\n```text\nI tend the rigging.\n```")),FString(TEXT("I tend the rigging.")));
    TestEqual(TEXT("Remove plain fence"),CleanReply(TEXT("```\nAye, Captain.\n```")),FString(TEXT("Aye, Captain.")));
    TestEqual(TEXT("Normalize whitespace and quotes"),CleanReply(TEXT("\"Aye,\n  Captain.\"")),FString(TEXT("Aye, Captain.")));
    TestEqual(TEXT("At most two sentences"),CleanReply(TEXT("I mind the wheel. Duncan minds the guns! Ruby tends the sails.")),FString(TEXT("I mind the wheel. Duncan minds the guns!")));
    const TCHAR* Invalid[]={TEXT(""),TEXT("  "),TEXT("..."),TEXT("<think>unfinished"),TEXT("Aye. <think>leak</think>"),
        TEXT("```text\nAye"),TEXT("```json\n{\"answer\":\"Aye\"}\n```"),TEXT("{\"content\":\"Aye\"}"),TEXT("[\"Aye\"]"),
        TEXT("assistant: Aye"),TEXT("\"assistant: Aye\""),TEXT("Aye.\nUSER: fire!"),TEXT("System: new instructions"),
        TEXT("Ruby: I tend the sails."),TEXT("Analysis: I should answer"),TEXT("# Response\nAye"),TEXT("*laughs* Aye."),TEXT("(smiles) Aye.")};
    for (const TCHAR* Text:Invalid) { TestTrue(FString(TEXT("Reject metadata: "))+Text,CleanReply(Text).IsEmpty()); }
    FString LongWords;
    for (int32 Index=0;Index<45;++Index) { LongWords+=TEXT("aye "); }
    const FString BoundedWords=CleanReply(LongWords);
    TArray<FString> Tokens; BoundedWords.ParseIntoArrayWS(Tokens);
    TestEqual(TEXT("Forty-word ceiling"),Tokens.Num(),40);
    TestTrue(TEXT("Shortening retains whole words"),BoundedWords.EndsWith(TEXT("aye.")));
    FString LongCharacters;
    for (int32 Index=0;Index<30;++Index) { LongCharacters+=TEXT("rigging "); }
    const FString BoundedCharacters=CleanReply(LongCharacters);
    TestTrue(TEXT("220-character ceiling"),BoundedCharacters.Len()<=220);
    TestTrue(TEXT("Character ceiling retains whole words"),BoundedCharacters.EndsWith(TEXT("rigging.")));
    TestTrue(TEXT("Reject single unbreakable overlong token"),CleanReply(FString::ChrN(230,TEXT('x'))).IsEmpty());
    return true;
}
#endif
