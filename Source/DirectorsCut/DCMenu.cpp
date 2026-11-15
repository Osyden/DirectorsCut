#include "DCGame.h"
#include "DCRideDirector.h"
#include "DCCrew.h"
#include "DCVoice.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SNullWidget.h"
#include "Styling/CoreStyle.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"

void ADCGameMode::ShowMainMenu()
{
    if (!Director || !GetWorld()->GetGameViewport()) { bAwaitingStart=false; return; }
    Director->Seek(0.f);
    Director->TimeScale=0.f;
    APlayerController* PC=GetWorld()->GetFirstPlayerController();
    if (!PC) { return; }
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    PC->bShowMouseCursor=true;
    const FVector Position(-6200.f,-10800.f,4100.f);
    // Look slightly off the ship's centre: keep the complete vessel beside the menu.
    const FVector LookAt(2000.f,-1700.f,1800.f);
    MenuCamera=GetWorld()->SpawnActor<ACameraActor>(Position,(LookAt-Position).Rotation());
    MenuCamera->GetCameraComponent()->SetFieldOfView(58.f);
    MenuCamera->GetCameraComponent()->bConstrainAspectRatio=false;
    PC->SetViewTarget(MenuCamera);
    const FLinearColor Cream(.96f,.92f,.82f), Gold(.96f,.74f,.36f), Muted(.65f,.75f,.79f);
    auto Text=[&](const TCHAR* Line,int Size,FLinearColor Color) {
        return SNew(STextBlock).Text(FText::FromString(Line)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(Color).AutoWrapText(true);
    };
    TSharedPtr<SButton> StartButton;
    TWeakObjectPtr<ADCGameMode> Weak(this);
    MainMenuWidget=SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
    [
        SNew(SBox).WidthOverride(1280.f).HeightOverride(720.f)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(.48f)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(.012f,.025f,.035f,.94f)).Padding(FMargin(38,30))
                [
                    SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
                    [
                    SNew(SBox).WidthOverride(538.f)
                    [
                    SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("DIRECTOR'S CUT"),42,Gold)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,18)[Text(TEXT("Captain the ship. Command your crew. Survive the storm."),21,Cream)]
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("WASD  Move     Mouse  Look     Shift  Sprint     Space  Jump"),17,Cream)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,9)[Text(TEXT("E  Take / leave a station\nAt the wheel: A / D to steer, C for a clear view\nAt a cannon: mouse to aim, left click to fire"),17,Cream)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,9)[Text(TEXT("Hold V to speak  •  Enter to type an order\n1 / 2 / 3  Port / starboard / steady\n4 / 5  Port / starboard guns   •   6  Hold fire"),17,Cream)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,9)[Text(TEXT("\"Rex, take the helm\"\n\"Duncan, man the port guns\"\n\"Ruby, put out the fire\"\n\"Ruby, repair the rigging\""),18,Gold)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,9)[Text(TEXT("Steer clear of obstacles or shoot them. Interrupt a raised tentacle at its glowing joint; fire into the Kraken's mouth before it releases fireballs."),17,Cream)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,6)[Text(TEXT("T  Ship view    F5  DLSS    F6  Frame generation\nEsc  Quit    R  Sail again after the ending"),15,Muted)]
                    +SVerticalBox::Slot().FillHeight(1.f)
                    +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,0)
                    [
                        SAssignNew(StartButton,SButton).HAlign(HAlign_Center).ContentPadding(FMargin(24,12))
                        .OnClicked_Lambda([Weak]() { if(Weak.IsValid()) { Weak->StartVoyage(); } return FReply::Handled(); })
                        [Text(TEXT("START GAME"),24,Gold)]
                    ]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)
                    [
                        SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(12,6))
                        .OnClicked_Lambda([Weak]() { if(Weak.IsValid()) { UKismetSystemLibrary::QuitGame(Weak.Get(),Weak->GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false); } return FReply::Handled(); })
                        [Text(TEXT("QUIT"),16,Cream)]
                    ]
                    ]
                    ]
                ]
            ]
            +SHorizontalBox::Slot().FillWidth(.52f)[SNullWidget::NullWidget]
        ]
    ];
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MainMenuWidget.ToSharedRef(),100);
    PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(StartButton).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    UE_LOG(LogDC,Log,TEXT("MAIN_MENU shown; voyage held at open sea"));
}

void ADCGameMode::TickMainMenu(float DeltaSeconds)
{
    MenuAge+=FApp::GetDeltaTime();
    if (Director) { Director->TimeScale=0.f; }
    if (!bMenuPaused) {
        TickShipMotion(DeltaSeconds);
        if(MenuAge>.7f) { UGameplayStatics::SetGamePaused(this,true); bMenuPaused=true; }
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuCapture")) && MenuAge>5.f && !bMenuCaptured) {
        bMenuCaptured=true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/MainMenu.png"),true,false);
    }
    float AutoStart=0;
    if(FParse::Value(FCommandLine::Get(),TEXT("DCMenuAutoStart="),AutoStart) && AutoStart>0 && MenuAge>=AutoStart) { StartVoyage(); }
}

void ADCGameMode::StartVoyage()
{
    if(!bAwaitingStart) { return; }
    bAwaitingStart=false;
    UGameplayStatics::SetGamePaused(this,false);
    if(MainMenuWidget.IsValid() && GetWorld()->GetGameViewport()) { GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MainMenuWidget.ToSharedRef()); }
    MainMenuWidget.Reset();
    if(ADCVoice* Voice=ADCVoice::Get(this)) { Voice->ResetPendingSpeech(); }
    if(UDCCrewSubsystem* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->RecentLines.Reset(); }
    if(Director) { Director->TimeScale=1.f; Director->Seek(0.f); }
    if(APlayerController* PC=GetWorld()->GetFirstPlayerController()) {
        PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false);
        PC->bShowMouseCursor=false; PC->SetInputMode(FInputModeGameOnly());
        PC->FlushPressedKeys();
        if(PC->GetPawn()) { PC->SetViewTargetWithBlend(PC->GetPawn(),.65f); }
    }
    UE_LOG(LogDC,Log,TEXT("MAIN_MENU start; fresh voyage at %.2f"),Director?Director->RideTime:-1.f);
}

void ADCGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if(MainMenuWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) { GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MainMenuWidget.ToSharedRef()); }
    MainMenuWidget.Reset();
    Super::EndPlay(EndPlayReason);
}
