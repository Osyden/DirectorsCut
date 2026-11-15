#include "DCGame.h"
#include "DCCrew.h"
#include "DCVoice.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "RenderUtils.h"

void ADCHUD::DrawSpeechBubble()
{
    if(!Canvas || !GEngine) { return; }
    UDCCrewSubsystem* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>();
    ADCVoice* Voice=ADCVoice::Get(this);
    const double Now=FPlatformTime::Seconds();
    FString Speaker,Text;
    const bool HasVoiceLine=Voice && !Voice->GetSpeakingText().IsEmpty() && Voice->GetSpeechRemainingSeconds()>0;
    if(HasVoiceLine) {
        Speaker=Voice->GetSpeakingSpeaker(); Text=Voice->GetSpeakingText();
        LastBubbleLine=Speaker+TEXT(": ")+Text;
        BubbleUntil=Now+Voice->GetSpeechRemainingSeconds();
    } else if(Crew && (!Voice || !Voice->IsTTSReady()) && !Crew->RecentLines.IsEmpty()) {
        const FString& Latest=Crew->RecentLines.Last();
        if(Latest!=LastBubbleLine) { LastBubbleLine=Latest; BubbleUntil=Now+FMath::Clamp(Latest.Len()*.055,4.,9.); }
        if(Now<BubbleUntil) { LastBubbleLine.Split(TEXT(": "),&Speaker,&Text); }
    } else { return; }
    if(Text.IsEmpty()) { return; }
    const TCHAR* Names[]={TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")};
    int Index=INDEX_NONE;
    for(int I=0;I<3;++I) { if(Speaker==Names[I]) { Index=I; break; } }
    if(Index==INDEX_NONE) { return; }
    const float S=FMath::Clamp(FMath::Min(Canvas->SizeX/1280.f,Canvas->SizeY/720.f),.65f,2.f);
    const float Width=FMath::Min(670.f*S,Canvas->SizeX-48.f*S);
    const float X=24.f*S,TextX=X+100.f*S,TextWidth=Width-120.f*S;
    UFont* Font=GEngine->GetMediumFont();
    TArray<FString> Words,Lines;
    Text.ParseIntoArrayWS(Words);
    FString Line;
    for(const FString& Word:Words) {
        const FString Candidate=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;
        float W,H; CrispSize(Candidate,W,H,Font,1.3f*S);
        if(W>TextWidth && !Line.IsEmpty()) { Lines.Add(Line); Line=Word; }
        else { Line=Candidate; }
    }
    if(!Line.IsEmpty()) { Lines.Add(Line); }
    const float Height=FMath::Max(100.f,48.f+Lines.Num()*25.f)*S;
    const float Y=Canvas->SizeY-65.f*S-Height;
    const FLinearColor Gold(.96f,.77f,.4f),Plate(.015f,.025f,.034f,.94f);
    DrawRect(FLinearColor(0,0,0,.3f),X+3*S,Y+4*S,Width,Height);
    DrawRect(Plate,X,Y,Width,Height);
    DrawRect(Gold,X,Y,3*S,Height);
    // A real circular crop, rather than a square portrait behind a ring.
    const FVector2D Center(X+49*S,Y+Height*.5f);
    const float Radius=36*S;
    if(CrewPortraits.IsValidIndex(Index) && CrewPortraits[Index] && CrewPortraits[Index]->GetResource()) {
        TArray<FCanvasUVTri> Tris;
        for(int I=0;I<48;++I) {
            const float A=I*2*PI/48,B=(I+1)*2*PI/48;
            const FVector2D U(FMath::Cos(A),FMath::Sin(A)),V(FMath::Cos(B),FMath::Sin(B));
            FCanvasUVTri T;
            T.V0_Pos=Center; T.V1_Pos=Center+U*Radius; T.V2_Pos=Center+V*Radius;
            T.V0_UV=FVector2D(.5,.4); T.V1_UV=T.V0_UV+U*.4; T.V2_UV=T.V0_UV+V*.4;
            T.V0_Color=T.V1_Color=T.V2_Color=FLinearColor::White;
            Tris.Add(T);
            DrawLine(Center.X+U.X*(Radius+2*S),Center.Y+U.Y*(Radius+2*S),Center.X+V.X*(Radius+2*S),Center.Y+V.Y*(Radius+2*S),Gold,2*S);
        }
        FCanvasTriangleItem Portrait(Tris,CrewPortraits[Index]->GetResource());
        Portrait.BlendMode=SE_BLEND_Translucent;
        Canvas->DrawItem(Portrait);
    }
    CrispText(Speaker,Gold,TextX,Y+13*S,Font,1.15f*S);
    for(int I=0;I<Lines.Num();++I) { CrispText(Lines[I],FLinearColor(.98f,.96f,.9f),TextX,Y+(40+I*25)*S,Font,1.3f*S); }
    // Small speech tail, clear of the interaction/reload area.
    FCanvasTriangleItem Tail(FVector2D(X+70*S,Y+Height),FVector2D(X+90*S,Y+Height),FVector2D(X+70*S,Y+Height+10*S),GWhiteTexture);
    Tail.SetColor(Plate); Tail.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(Tail);
}
