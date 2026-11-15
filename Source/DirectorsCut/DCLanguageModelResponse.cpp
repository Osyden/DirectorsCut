#include "DCLanguageModelResponse.h"
#include "DCSpeech.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool StringField(const FJsonObject& Object, const TCHAR* Name, FString& Out)
    {
        const TSharedPtr<FJsonValue> Value = Object.TryGetField(Name);
        return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out);
    }

    bool UnwrapObject(const FString& Content, FString& OutJson)
    {
        OutJson = Content.TrimStartAndEnd();
        if (OutJson.StartsWith(TEXT("<think>")))
        {
            const int32 End = OutJson.Find(TEXT("</think>"));
            if (End == INDEX_NONE) { return false; }
            OutJson = OutJson.Mid(End + 8).TrimStartAndEnd();
        }
        if (OutJson.StartsWith(TEXT("```")))
        {
            int32 LineEnd;
            if (!OutJson.FindChar(TEXT('\n'), LineEnd) || !OutJson.EndsWith(TEXT("```"))) { return false; }
            const FString Fence = OutJson.Left(LineEnd).TrimEnd();
            if (Fence != TEXT("```") && !Fence.Equals(TEXT("```json"), ESearchCase::IgnoreCase)) { return false; }
            OutJson = OutJson.Mid(LineEnd + 1, OutJson.Len() - LineEnd - 4).TrimStartAndEnd();
        }
        if (!OutJson.StartsWith(TEXT("{")) || !OutJson.EndsWith(TEXT("}"))) { return false; }

        // Require one entire object. Do not accept a valid prefix followed by another order.
        int32 Depth = 0;
        bool bString = false, bEscape = false;
        for (int32 Index = 0; Index < OutJson.Len(); ++Index)
        {
            const TCHAR C = OutJson[Index];
            if (bString)
            {
                if (bEscape) { bEscape = false; }
                else if (C == TEXT('\\')) { bEscape = true; }
                else if (C == TEXT('"')) { bString = false; }
                continue;
            }
            if (C == TEXT('"')) { bString = true; }
            else if (C == TEXT('{')) { ++Depth; }
            else if (C == TEXT('}') && (--Depth < 0 || (Depth == 0 && Index != OutJson.Len() - 1))) { return false; }
        }
        return Depth == 0 && !bString;
    }

    template<typename T, int32 Count>
    bool ParseEnum(const FString& Value, const TPair<const TCHAR*, T> (&Values)[Count], T& Out)
    {
        for (const auto& Pair : Values)
        {
            if (Value.Equals(Pair.Key, ESearchCase::IgnoreCase)) { Out = Pair.Value; return true; }
        }
        return false;
    }
}

bool DCLanguageModelResponse::ExtractChatContent(const FString& ResponseJson, FString& OutContent)
{
    OutContent.Reset();
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ResponseJson), Root) || !Root.IsValid()) { return false; }
    const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
    if (!Root->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->IsEmpty() ||
        !(*Choices)[0].IsValid() || (*Choices)[0]->Type != EJson::Object) { return false; }
    const TSharedPtr<FJsonObject> Choice = (*Choices)[0]->AsObject();
    const TSharedPtr<FJsonObject>* Message = nullptr;
    if (!Choice.IsValid() || !Choice->TryGetObjectField(TEXT("message"), Message) || !Message || !Message->IsValid() ||
        !StringField(*Message->Get(), TEXT("content"), OutContent) || OutContent.TrimStartAndEnd().IsEmpty())
    {
        OutContent.Reset();
        return false;
    }
    return true;
}

bool DCLanguageModelResponse::ParseIntent(const FString& ModelContent, const FString& RawOrder, FDCIntent& OutIntent)
{
    OutIntent = FDCIntent();
    OutIntent.RawText = RawOrder;
    FString Json;
    if (!UnwrapObject(ModelContent, Json)) { return false; }
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid() || Root->Values.Num() != 4) { return false; }
    FString Crew, Station, Side, Action;
    if (!StringField(*Root,TEXT("crew"),Crew) || !StringField(*Root,TEXT("station"),Station) ||
        !StringField(*Root,TEXT("side"),Side) || !StringField(*Root,TEXT("action"),Action)) { return false; }
    Crew.TrimStartAndEndInline(); Station.TrimStartAndEndInline(); Side.TrimStartAndEndInline(); Action.TrimStartAndEndInline();
    FDCIntent Intent;
    Intent.RawText = RawOrder;
    Intent.CrewName = DCSpeech::CanonicalCrewName(Crew);
    if (!Crew.IsEmpty() && Intent.CrewName.IsEmpty()) { return false; }

    using S = EDCStationType; using D = EDCSide; using A = EDCAction;
    const TPair<const TCHAR*, S> Stations[] = {{TEXT("none"),S::None},{TEXT("helm"),S::Helm},{TEXT("cannons"),S::Cannons},{TEXT("fire"),S::Fire},{TEXT("rigging"),S::Rigging}};
    const TPair<const TCHAR*, D> Sides[] = {{TEXT("none"),D::None},{TEXT("port"),D::Port},{TEXT("starboard"),D::Starboard}};
    const TPair<const TCHAR*, A> Actions[] = {{TEXT("none"),A::None},{TEXT("man"),A::Man},{TEXT("fire"),A::Fire},{TEXT("hold_fire"),A::HoldFire},
        {TEXT("turn_port"),A::TurnPort},{TEXT("turn_starboard"),A::TurnStarboard},{TEXT("steady"),A::Steady},
        {TEXT("extinguish"),A::Extinguish},{TEXT("repair"),A::Repair},{TEXT("report"),A::Report}};
    if (!ParseEnum(Station,Stations,Intent.Station) || !ParseEnum(Side,Sides,Intent.Side) || !ParseEnum(Action,Actions,Intent.Action)) { return false; }

    S Required = S::None;
    switch (Intent.Action)
    {
    case A::TurnPort: case A::TurnStarboard: case A::Steady: Required = S::Helm; break;
    case A::Fire: case A::HoldFire: Required = S::Cannons; break;
    case A::Extinguish: Required = S::Fire; break;
    case A::Repair: Required = S::Rigging; break;
    case A::Man:
        if (Intent.Station == S::None) { return false; }
        if (Intent.Station == S::Fire) { Intent.Action = A::Extinguish; }
        if (Intent.Station == S::Rigging) { Intent.Action = A::Repair; }
        Required = Intent.Station;
        break;
    case A::Report: case A::None:
        if (Intent.Station != S::None || Intent.Side != D::None) { return false; }
        break;
    }
    if (Required != S::None)
    {
        if (Intent.Station != S::None && Intent.Station != Required) { return false; }
        Intent.Station = Required;
    }
    if (Intent.Action == A::TurnPort || Intent.Action == A::TurnStarboard)
    {
        const D RequiredSide = Intent.Action == A::TurnPort ? D::Port : D::Starboard;
        if (Intent.Side != D::None && Intent.Side != RequiredSide) { return false; }
        Intent.Side = RequiredSide;
    }
    else if (Intent.Station != S::Cannons && Intent.Side != D::None) { return false; }

    Intent.bValid = Intent.Action != A::None;
    OutIntent = MoveTemp(Intent);
    return true;
}
