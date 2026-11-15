#include "DCConversationPolicy.h"
#include "DCSpeech.h"

namespace DCConversationPolicyPrivate
{
    TArray<FString> Words(const FString& Text)
    {
        FString Normalized;
        for (TCHAR C:Text.ToLower())
        {
            if (C==TEXT('\'') || C==0x2019) { continue; }
            Normalized.AppendChar(FChar::IsAlnum(C) ? C : TEXT(' '));
        }
        TArray<FString> Result; Normalized.ParseIntoArrayWS(Result); return Result;
    }

    bool Starts(const FString& Text,const TCHAR* Prefix)
    {
        return Text==Prefix || Text.StartsWith(FString(Prefix)+TEXT(" "));
    }

    bool Has(const FString& Text,const TCHAR* Word)
    {
        return (TEXT(" ")+Text+TEXT(" ")).Contains(TEXT(" ")+FString(Word)+TEXT(" "));
    }

    int32 AddressIndex(const TArray<FString>& Tokens)
    {
        int32 Index=0;
        if (Tokens.Num() && (Tokens[0]==TEXT("hey") || Tokens[0]==TEXT("hello") || Tokens[0]==TEXT("hi"))) { ++Index; }
        if (!Tokens.IsValidIndex(Index) || DCSpeech::CanonicalCrewName(Tokens[Index]).IsEmpty()) { return INDEX_NONE; }
        // "Ruby is ..." describes Ruby; "Ruby, tell me ..." addresses her.
        if (Tokens.IsValidIndex(Index+1))
        {
            const FString& Next=Tokens[Index+1];
            if (Next==TEXT("is") || Next==TEXT("was") || Next==TEXT("has") || Next==TEXT("looks") || Next==TEXT("ahead")) { return INDEX_NONE; }
        }
        return Index;
    }
}

FString DCConversationPolicy::AddressedCrew(const FString& Text)
{
    using namespace DCConversationPolicyPrivate;
    const TArray<FString> Tokens=Words(Text);
    const int32 Index=AddressIndex(Tokens);
    return Index==INDEX_NONE ? FString() : DCSpeech::CanonicalCrewName(Tokens[Index]);
}

static FString CommandBody(const FString& Text, bool& bMindRequest)
{
    using namespace DCConversationPolicyPrivate;
    TArray<FString> Tokens=Words(Text);
    const int32 Address=AddressIndex(Tokens);
    if (Address!=INDEX_NONE) { Tokens.RemoveAt(0,Address+1); }
    else if (Tokens.Num() && (Tokens[0]==TEXT("hey") || Tokens[0]==TEXT("hello") || Tokens[0]==TEXT("hi"))) { Tokens.RemoveAt(0); }
    FString Body=FString::Join(Tokens,TEXT(" "));
    bMindRequest=false;
    // Remove only leading request wrappers; never search arbitrary prose for verbs.
    const TCHAR* Wrappers[]={TEXT("would you be so kind as to"),TEXT("would you mind"),TEXT("could you please"),
        TEXT("can you please"),TEXT("would you please"),TEXT("will you please"),TEXT("i need you to"),
        TEXT("i want you to"),TEXT("could you"),TEXT("can you"),TEXT("would you"),TEXT("will you"),TEXT("please"),TEXT("kindly"),TEXT("all hands")};
    for (int32 Pass=0;Pass<3;++Pass)
    {
        bool bRemoved=false;
        for (const TCHAR* Prefix:Wrappers)
        {
            if (Starts(Body,Prefix))
            {
                bMindRequest |= FCString::Strcmp(Prefix,TEXT("would you mind"))==0;
                Body=Body.Mid(FCString::Strlen(Prefix)).TrimStart(); bRemoved=true; break;
            }
        }
        if (!bRemoved) { break; }
    }
    return Body;
}

bool DCConversationPolicy::IsFireOrder(const FString& Text)
{
    using namespace DCConversationPolicyPrivate;
    bool bMindRequest=false;
    const FString Body=CommandBody(Text,bMindRequest);
    // Require a direct request whose object is the hazard, not a question, story,
    // negation or a request to fire guns. Plurals matter for Whisper's transcripts.
    const TCHAR* Verbs[]={TEXT("put out"),TEXT("putting out"),TEXT("extinguish"),TEXT("extinguishing"),
        TEXT("douse"),TEXT("dousing"),TEXT("smother"),TEXT("smothering"),TEXT("swat"),TEXT("swatting"),TEXT("kill"),TEXT("killing"),
        TEXT("fight"),TEXT("fighting"),TEXT("tackle"),TEXT("handle"),TEXT("deal with"),TEXT("take care of"),
        TEXT("beat out"),TEXT("stamp out"),TEXT("get rid of"),TEXT("get"),TEXT("put")};
    for (const TCHAR* Verb:Verbs)
    {
        if (!Starts(Body,Verb)) { continue; }
        const FString VerbText(Verb);
        if (VerbText.EndsWith(TEXT("ing")) || VerbText==TEXT("putting out"))
        {
            if (!bMindRequest) { continue; }
        }
        FString Object=Body.Mid(VerbText.Len()).TrimStart();
        TArray<FString> Parts; Object.ParseIntoArrayWS(Parts);
        int32 Index=0;
        while (Parts.IsValidIndex(Index) && (Parts[Index]==TEXT("the") || Parts[Index]==TEXT("that") ||
            Parts[Index]==TEXT("this") || Parts[Index]==TEXT("those") || Parts[Index]==TEXT("these") ||
            Parts[Index]==TEXT("all") || Parts[Index]==TEXT("deck") || Parts[Index]==TEXT("remaining"))) { ++Index; }
        if (!Parts.IsValidIndex(Index)) { continue; }
        const FString& Hazard=Parts[Index];
        if (Hazard!=TEXT("fire") && Hazard!=TEXT("fires") && Hazard!=TEXT("flame") && Hazard!=TEXT("flames") &&
            Hazard!=TEXT("blaze") && Hazard!=TEXT("blazes")) { continue; }
        if (VerbText==TEXT("put") && !Has(Object,TEXT("out"))) { continue; }
        if (Has(Object,TEXT("story")) || Has(Object,TEXT("stories")) || Has(Object,TEXT("report")) ||
            Has(Object,TEXT("information")) || Has(Object,TEXT("at will"))) { continue; }
        return true;
    }
    return false;
}

bool DCConversationPolicy::IsConversation(const FString& Text)
{
    using namespace DCConversationPolicyPrivate;
    bool bMindRequest=false;
    const FString Body=CommandBody(Text,bMindRequest);
    if (Body.IsEmpty()) { return true; }
    if (IsFireOrder(Text)) { return false; }
    // Explicit legacy status requests and urgent hazard alerts remain actionable.
    if (Body==TEXT("how are we doing") || Body==TEXT("how is the ship") || Body==TEXT("how is the hull") ||
        Body==TEXT("the deck is on fire") || Body==TEXT("the ship is on fire")) { return false; }
    if (Starts(Body,TEXT("how much damage has the hull")) || Starts(Body,TEXT("how much damage has the ship"))) { return false; }
    const TCHAR* Social[]={TEXT("tell me"),TEXT("explain"),TEXT("what"),TEXT("whats"),TEXT("who"),TEXT("whos"),
        TEXT("why"),TEXT("how"),TEXT("where"),TEXT("when"),TEXT("do you"),TEXT("have you"),TEXT("are you"),TEXT("is there")};
    for (const TCHAR* Prefix:Social) { if (Starts(Body,Prefix)) { return true; } }
    const TCHAR* Information[]={TEXT("take me through"),TEXT("take us through"),TEXT("take a look"),TEXT("take another look"),
        TEXT("taking me through"),TEXT("taking us through"),TEXT("taking a look"),
        TEXT("walk me through"),TEXT("walk us through"),TEXT("show me"),TEXT("help me understand"),TEXT("describe"),
        TEXT("get me information"),TEXT("give me information")};
    for (const TCHAR* Prefix:Information) { if (Starts(Body,Prefix)) { return true; } }
    // A gerund describes an activity unless a polite request established imperative intent.
    const TCHAR* Gerunds[]={TEXT("firing"),TEXT("shooting"),TEXT("extinguishing"),TEXT("dousing"),TEXT("repairing"),
        TEXT("mending"),TEXT("fixing"),TEXT("manning"),TEXT("taking"),TEXT("turning"),TEXT("steering"),TEXT("putting")};
    if (!bMindRequest)
    {
        for (const TCHAR* Prefix:Gerunds) { if (Starts(Body,Prefix)) { return true; } }
    }
    if (Has(Body,TEXT("tell")) || Has(Body,TEXT("talk")) || Has(Body,TEXT("talking")) ||
        Has(Body,TEXT("explain")) || Has(Body,TEXT("story"))) { return true; }

    const bool bDirection=Has(Body,TEXT("port")) || Has(Body,TEXT("starboard")) || Has(Body,TEXT("larboard")) || Has(Body,TEXT("left")) || Has(Body,TEXT("right"));
    const bool bGuns=Has(Body,TEXT("guns")) || Has(Body,TEXT("cannons")) || Has(Body,TEXT("gun")) || Has(Body,TEXT("cannon")) || Has(Body,TEXT("battery"));
    const bool bStation=bGuns || Has(Body,TEXT("helm")) || Has(Body,TEXT("wheel")) || Has(Body,TEXT("rigging")) ||
        Has(Body,TEXT("sails")) || Has(Body,TEXT("sail")) || Has(Body,TEXT("ropes")) || Has(Body,TEXT("rope")) || Has(Body,TEXT("rudder")) || Has(Body,TEXT("fire"));
    const TCHAR* Direct[]={TEXT("fire"),TEXT("firing"),TEXT("shoot"),TEXT("shooting"),TEXT("blast"),TEXT("sink"),TEXT("discharge"),TEXT("attack"),TEXT("extinguish"),
        TEXT("extinguishing"),TEXT("douse"),TEXT("dousing"),TEXT("repair"),TEXT("repairing"),TEXT("mend"),TEXT("mending"),
        TEXT("fix"),TEXT("fixing"),TEXT("report"),TEXT("status"),TEXT("steady"),TEXT("midships")};
    for (const TCHAR* Prefix:Direct)
    {
        if (Starts(Body,Prefix))
        {
            // Avoid treating ordinary noun-led remarks as imperatives.
            TArray<FString> Parts; Body.ParseIntoArrayWS(Parts);
            if (Parts.Num()>1 && (Parts[1]==TEXT("is") || Parts[1]==TEXT("was") || Parts[1]==TEXT("can") || Parts[1]==TEXT("looks") ||
                Parts[1]==TEXT("scares") || Parts[1]==TEXT("costs") || Parts[1]==TEXT("seems") || Parts[1]==TEXT("makes") || Parts[1]==TEXT("reminds"))) { return true; }
            return false;
        }
    }
    const bool bFiring=Has(Body,TEXT("fire")) || Has(Body,TEXT("firing")) || Has(Body,TEXT("shoot")) || Has(Body,TEXT("shooting"));
    const TCHAR* CeasePrefixes[]={TEXT("hold your fire"),TEXT("hold fire"),TEXT("stop firing"),TEXT("stop shooting"),
        TEXT("cease fire"),TEXT("cease firing"),TEXT("cease shooting"),TEXT("dont fire"),TEXT("dont shoot"),
        TEXT("do not fire"),TEXT("do not shoot"),TEXT("not fire"),TEXT("not firing"),TEXT("not shoot"),TEXT("not shooting")};
    for (const TCHAR* Prefix:CeasePrefixes) { if (Starts(Body,Prefix)) { return false; } }
    if (Starts(Body,TEXT("let them have it")) || Starts(Body,TEXT("let em have it")) || Starts(Body,TEXT("blow them away"))) { return false; }
    if ((Starts(Body,TEXT("hold")) || Starts(Body,TEXT("keep"))) &&
        (Has(Body,TEXT("course")) || Has(Body,TEXT("heading")) || Has(Body,TEXT("straight")) || bFiring)) { return false; }
    if (Starts(Body,TEXT("put out")) || Starts(Body,TEXT("putting out"))) { return false; }
    if ((Starts(Body,TEXT("man")) || Starts(Body,TEXT("manning")) || Starts(Body,TEXT("take")) ||
        Starts(Body,TEXT("taking")) || Starts(Body,TEXT("get")) || Starts(Body,TEXT("go")) || Starts(Body,TEXT("crew"))) && bStation) { return false; }
    if ((Starts(Body,TEXT("hard")) || Starts(Body,TEXT("turn")) || Starts(Body,TEXT("turning")) ||
        Starts(Body,TEXT("veer")) || Starts(Body,TEXT("steer")) || Starts(Body,TEXT("steering")) ||
        Starts(Body,TEXT("bring")) || Starts(Body,TEXT("swing")) || Starts(Body,TEXT("bear")) || Starts(Body,TEXT("come about")) ||
        Starts(Body,TEXT("dodge")) || Starts(Body,TEXT("evade"))) && bDirection) { return false; }
    if (Body==TEXT("port") || Body==TEXT("starboard") || Body==TEXT("larboard") || Body==TEXT("left") || Body==TEXT("right")) { return false; }
    if (bDirection && bGuns && bFiring && (Starts(Body,TEXT("port")) || Starts(Body,TEXT("starboard")))) { return false; }
    if (Starts(Body,TEXT("give me")) && (Has(Body,TEXT("report")) || Has(Body,TEXT("status")))) { return false; }
    if (Starts(Body,TEXT("let")) && bGuns && bFiring) { return false; }
    return true;
}

FString DCConversationPolicy::CleanReply(const FString& Raw)
{
    FString Line=Raw.TrimStartAndEnd();
    while (Line.StartsWith(TEXT("<think>"),ESearchCase::IgnoreCase))
    {
        const int32 End=Line.Find(TEXT("</think>"),ESearchCase::IgnoreCase);
        if (End==INDEX_NONE) { return FString(); }
        Line=Line.Mid(End+8).TrimStartAndEnd();
    }
    if (Line.StartsWith(TEXT("```")))
    {
        int32 NewLine=INDEX_NONE;
        if (!Line.FindChar(TEXT('\n'),NewLine) || !Line.EndsWith(TEXT("```"))) { return FString(); }
        const FString Tag=Line.Mid(3,NewLine-3).TrimStartAndEnd().ToLower();
        if (!Tag.IsEmpty() && Tag!=TEXT("text") && Tag!=TEXT("plaintext")) { return FString(); }
        Line=Line.Mid(NewLine+1,Line.Len()-NewLine-4).TrimStartAndEnd();
    }
    if (Line.Len()>=2 && Line.StartsWith(TEXT("\"")) && Line.EndsWith(TEXT("\""))) { Line=Line.Mid(1,Line.Len()-2).TrimStartAndEnd(); }
    // Metadata is rejected, not removed in a way that could turn it into dialogue.
    if (Line.Contains(TEXT("<")) || Line.Contains(TEXT(">")) || Line.Contains(TEXT("{")) || Line.Contains(TEXT("}")) ||
        Line.Contains(TEXT("[")) || Line.Contains(TEXT("]")) || Line.Contains(TEXT("```")) || Line.Contains(TEXT("*")) ||
        Line.Contains(TEXT("#")) || Line.Contains(TEXT("(")) || Line.Contains(TEXT(")"))) { return FString(); }
    TArray<FString> Lines; Line.ParseIntoArrayLines(Lines,false);
    for (const FString& Part:Lines)
    {
        const FString Trimmed=Part.TrimStartAndEnd();
        const TCHAR* Headers[]={TEXT("assistant:"),TEXT("user:"),TEXT("system:"),TEXT("developer:"),TEXT("role:"),TEXT("content:"),
            TEXT("analysis:"),TEXT("final:"),TEXT("reasoning:"),TEXT("response:"),TEXT("rex:"),TEXT("duncan:"),TEXT("ruby:")};
        for (const TCHAR* Header:Headers) { if (Trimmed.StartsWith(Header,ESearchCase::IgnoreCase)) { return FString(); } }
    }
    TArray<FString> Tokens; Line.ParseIntoArrayWS(Tokens);
    Line=FString::Join(Tokens,TEXT(" "));
    int32 Sentences=0;
    for (int32 Index=0;Index<Line.Len();++Index)
    {
        if ((Line[Index]==TEXT('.') || Line[Index]==TEXT('!') || Line[Index]==TEXT('?')) &&
            (Index+1==Line.Len() || FChar::IsWhitespace(Line[Index+1])))
        {
            if (++Sentences==2) { Line=Line.Left(Index+1); break; }
        }
    }
    Tokens.Reset(); Line.ParseIntoArrayWS(Tokens);
    FString Result;
    for (int32 Index=0;Index<FMath::Min(40,Tokens.Num());++Index)
    {
        const FString Next=Result.IsEmpty() ? Tokens[Index] : Result+TEXT(" ")+Tokens[Index];
        if (Next.Len()>220) { break; }
        Result=Next;
    }
    if (Result.Len()<Line.Len())
    {
        Result=Result.TrimEnd();
        while (Result.EndsWith(TEXT(",")) || Result.EndsWith(TEXT(";")) || Result.EndsWith(TEXT(":"))) { Result.LeftChopInline(1); }
        if (!Result.IsEmpty() && Result.Len()<220 && !Result.EndsWith(TEXT(".")) && !Result.EndsWith(TEXT("!")) && !Result.EndsWith(TEXT("?"))) { Result+=TEXT("."); }
    }
    for (TCHAR C:Result) { if (FChar::IsAlnum(C)) { return Result; } }
    return FString();
}
