#include "DCGame.h"
#include "DCCrew.h"
#include "DCTroubles.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"

void ADCHUD::DrawCrewDutyIcon(EDCStationType Duty, float X, float Y, float Size, const FLinearColor& Color, bool bMirror)
{
    auto Line = [&](float AX, float AY, float BX, float BY) {
        if (bMirror) { AX = 1.f - AX; BX = 1.f - BX; }
        DrawLine(X+AX*Size,Y+AY*Size,X+BX*Size,Y+BY*Size,Color,FMath::Max(1.3f,Size*.075f));
    };
    auto Circle = [&](float CX,float CY,float Radius) {
        for(int32 I=0;I<20;++I) {
            const float A=I*2*PI/20, B=(I+1)*2*PI/20;
            Line(CX+FMath::Cos(A)*Radius,CY+FMath::Sin(A)*Radius,CX+FMath::Cos(B)*Radius,CY+FMath::Sin(B)*Radius);
        }
    };
    switch(Duty) {
    case EDCStationType::Helm:
        Circle(.5f,.5f,.28f); Circle(.5f,.5f,.06f);
        for(int32 I=0;I<8;++I) { const float A=I*PI/4; Line(.5f+FMath::Cos(A)*.1f,.5f+FMath::Sin(A)*.1f,.5f+FMath::Cos(A)*.47f,.5f+FMath::Sin(A)*.47f); }
        break;
    case EDCStationType::Cannons:
        Line(.18f,.43f,.86f,.14f); Line(.18f,.63f,.94f,.35f); Line(.86f,.14f,.94f,.35f); Line(.18f,.43f,.18f,.63f);
        Line(.12f,.69f,.79f,.69f); Line(.49f,.53f,.64f,.69f); Circle(.27f,.81f,.12f); Circle(.68f,.81f,.12f);
        break;
    case EDCStationType::Rigging:
        Line(.45f,.05f,.45f,.93f); Line(.08f,.93f,.91f,.93f);
        Line(.5f,.13f,.91f,.72f); Line(.91f,.72f,.5f,.72f);
        Line(.38f,.32f,.08f,.76f); Line(.08f,.76f,.38f,.76f);
        break;
    case EDCStationType::Fire:
        Line(.5f,.02f,.58f,.34f); Line(.58f,.34f,.78f,.23f); Line(.78f,.23f,.91f,.61f);
        Line(.91f,.61f,.81f,.87f); Line(.81f,.87f,.5f,.99f); Line(.5f,.99f,.18f,.87f);
        Line(.18f,.87f,.08f,.59f); Line(.08f,.59f,.3f,.3f); Line(.3f,.3f,.29f,.53f); Line(.29f,.53f,.5f,.02f);
        Line(.5f,.5f,.35f,.76f); Line(.35f,.76f,.5f,.89f); Line(.5f,.89f,.65f,.76f); Line(.65f,.76f,.5f,.5f);
        break;
    default: Circle(.5f,.5f,.34f); Line(.3f,.5f,.7f,.5f); break;
    }
}

void ADCHUD::DrawCrewPanel()
{
    UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
    if (!Crew || !Canvas || !GEngine) { return; }
    const TCHAR* Names[] = {TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby")};
    if (!bCrewPortraitsLoaded) {
        bCrewPortraitsLoaded=true;
        for(const TCHAR* Name:Names) {
            const FString Path=FString::Printf(TEXT("/Game/DC/UI/T_Crew_%s.T_Crew_%s"),Name,Name);
            UTexture2D* Portrait=LoadObject<UTexture2D>(nullptr,*Path);
            CrewPortraits.Add(Portrait);
            if(!Portrait) { UE_LOG(LogDC,Warning,TEXT("Crew HUD portrait missing: %s"),*Path); }
        }
    }
    const float S=FMath::Clamp(FMath::Min(Canvas->SizeX/1280.f,Canvas->SizeY/720.f),.78f,2.f);
    const float Width=218*S, Height=76*S, Left=Canvas->SizeX-Width-28*S;
    const FLinearColor Plate(.015f,.026f,.033f,.93f), Border(.31f,.34f,.32f,.8f);
    const FLinearColor Cream(.96f,.91f,.78f), Muted(.65f,.73f,.75f);
    UFont* Font=GEngine->GetMediumFont();
    for(int32 Index=0;Index<3;++Index) {
        ADCCrewMember* Member=nullptr;
        for(ADCCrewMember* Candidate:Crew->GetCrew()) { if(Candidate && Candidate->CrewName==Names[Index]) { Member=Candidate; break; } }
        if(!Member) { continue; }
        FDCStation* Station=Crew->GetStation(Member->StationIndex);
        const EDCStationType Duty=Station ? Station->Type : EDCStationType::None;
        FLinearColor Accent(.65f,.72f,.73f);
        FString Label=TEXT("Available");
        switch(Duty) {
        case EDCStationType::Helm: Label=TEXT("Wheel"); Accent=FLinearColor(.53f,.81f,.87f); break;
        case EDCStationType::Cannons: Label=Station->Side==EDCSide::Port ? TEXT("Port cannon") : TEXT("Starboard cannon"); Accent=FLinearColor(.93f,.76f,.43f); break;
        case EDCStationType::Fire: Label=TEXT("Fire duty"); Accent=FLinearColor(1.f,.48f,.24f); break;
        case EDCStationType::Rigging: Label=TEXT("Rigging"); Accent=FLinearColor(.54f,.84f,.69f); break;
        default: break;
        }
        FString Status=Member->State==EDCCrewState::Moving ? TEXT("MOVING") : Member->State==EDCCrewState::Idle ? TEXT("READY") : TEXT("ON DUTY");
        if(Member->State==EDCCrewState::Operating && Duty==EDCStationType::Cannons && Member->IsHoldingFire()) { Status=TEXT("HOLDING FIRE"); }
        if(Member->State==EDCCrewState::Operating && Duty==EDCStationType::Fire) {
            if(ADCTroubles* Troubles=ADCTroubles::Get(this)) { Status=Troubles->NumFires()>0 ? TEXT("EXTINGUISHING") : TEXT("STANDING BY"); }
        }
        if(Member->State==EDCCrewState::Operating && Duty==EDCStationType::Rigging) {
            if(ADCTroubles* Troubles=ADCTroubles::Get(this)) { Status=Troubles->IsRiggingLoose() ? TEXT("REPAIRING") : TEXT("STANDING BY"); }
        }
        const float Top=(96+Index*84)*S;
        DrawRect(FLinearColor(0,0,0,.3f),Left+3*S,Top+4*S,Width,Height);
        DrawRect(Plate,Left,Top,Width,Height);
        DrawRect(Border,Left,Top,Width,S); DrawRect(Border,Left,Top+Height-S,Width,S);
        DrawRect(Accent,Left,Top,2*S,Height);
        const float PX=Left+7*S,PY=Top+7*S,PortraitSize=62*S;
        if(CrewPortraits.IsValidIndex(Index) && CrewPortraits[Index]) {
            DrawTexture(CrewPortraits[Index],PX,PY,PortraitSize,PortraitSize,0,0,1,1,FLinearColor::White,BLEND_Opaque);
        } else {
            DrawRect(FLinearColor(.08f,.12f,.14f),PX,PY,PortraitSize,PortraitSize);
            CrispText(FString(Names[Index]).Left(1),Cream,PX+22*S,PY+16*S,Font,1.6f*S);
        }
        CrispText(Names[Index],Cream,Left+80*S,Top+6*S,Font,1.35f*S);
        DrawCrewDutyIcon(Duty,Left+80*S,Top+32*S,19*S,Accent,Duty==EDCStationType::Cannons && Station->Side==EDCSide::Port);
        CrispText(Label,Accent,Left+105*S,Top+33*S,Font,.9f*S);
        CrispText(Status,Muted,Left+80*S,Top+56*S,Font,.68f*S);
    }
}
