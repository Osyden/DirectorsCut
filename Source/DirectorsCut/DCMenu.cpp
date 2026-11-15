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
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
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
    const FLinearColor Cream(.92f,.91f,.84f), Gold(.85f,.64f,.29f), Muted(.48f,.66f,.68f);
    auto Text=[](const TCHAR* Line,int Size,FLinearColor Color) {
        return SNew(STextBlock).Text(FText::FromString(Line)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size))
            .ColorAndOpacity(Color).AutoWrapText(true);
    };
    static TStrongObjectPtr<UTexture2D> Logo(LoadObject<UTexture2D>(nullptr,TEXT("/Game/DC/UI/T_AtlanteanOdysseyLogo.T_AtlanteanOdysseyLogo")));
    static FSlateBrush LogoBrush;
    if (Logo.IsValid()) { LogoBrush.SetResourceObject(Logo.Get()); LogoBrush.ImageSize=FVector2D(500,250); }
    static const FSlateRoundedBoxBrush Panel(FLinearColor(.008f,.025f,.031f,.94f),8.f);
    static const FButtonStyle Primary=FButtonStyle()
        .SetNormal(FSlateRoundedBoxBrush(FLinearColor(.72f,.49f,.17f),3.f))
        .SetHovered(FSlateRoundedBoxBrush(FLinearColor(.95f,.73f,.34f),3.f))
        .SetPressed(FSlateRoundedBoxBrush(FLinearColor(.56f,.35f,.09f),3.f));
    static const FButtonStyle Secondary=FButtonStyle()
        .SetNormal(FSlateRoundedBoxBrush(FLinearColor(.015f,.07f,.08f,.85f),3.f))
        .SetHovered(FSlateRoundedBoxBrush(FLinearColor(.03f,.16f,.18f,.95f),3.f))
        .SetPressed(FSlateRoundedBoxBrush(FLinearColor(.01f,.04f,.05f,.95f),3.f));
    TWeakObjectPtr<ADCGameMode> Weak(this);
    TSharedPtr<SButton> StartButton;
    TSharedPtr<SBorder> Instructions;
    SAssignNew(Instructions,SBorder).BorderImage(&Panel).Padding(30).Visibility(EVisibility::Collapsed)
    [
        SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[Text(TEXT("THE CAPTAIN'S HANDBOOK"),24,Gold)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,5)[Text(TEXT("EXPLORE THE SHIP"),13,Muted)]
        +SVerticalBox::Slot().AutoHeight()[Text(TEXT("WASD  Move     Mouse  Look\nShift  Sprint     Space  Jump\nE  Take or leave a station"),17,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,5)[Text(TEXT("SAIL & FIGHT"),13,Muted)]
        +SVerticalBox::Slot().AutoHeight()[Text(TEXT("At the wheel: A / D to steer, C for a clear view.\nAt a cannon: mouse to aim, left click to fire.\nSteer around obstacles or shoot them. Break a raised tentacle at its glowing joint. Fire into the Kraken's mouth before it releases fireballs."),17,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,5)[Text(TEXT("COMMAND YOUR CREW"),13,Muted)]
        +SVerticalBox::Slot().AutoHeight()[Text(TEXT("Hold V to speak, or Enter to type an order.\n\"Rex, take the helm\"\n\"Duncan, man the port guns\"\n\"Ruby, put out the fire\"\n\"Ruby, repair the rigging\""),17,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[Text(TEXT("1 / 2 / 3  Port / starboard / steady\n4 / 5  Port / starboard guns    6  Hold fire\nT  Ship view    F5  DLSS    F6  Frame generation\nEsc  Quit    R  Sail again after the ending"),14,Muted)]
    ];
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuShowHelp"))) { Instructions->SetVisibility(EVisibility::Visible); }
    TWeakPtr<SBorder> WeakInstructions=Instructions;
    MainMenuWidget=SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
    [
        SNew(SBox).WidthOverride(1280.f).HeightOverride(720.f)
        [
            SNew(SOverlay)
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(42,24)
            [
                SNew(SBox).WidthOverride(490)
                [
                    SNew(SBorder).BorderImage(&Panel).Padding(FMargin(26,30))
                    [
                        SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                        [SNew(SBox).WidthOverride(438).HeightOverride(220)[SNew(SImage).Image(&LogoBrush)]]
                        +SVerticalBox::Slot().AutoHeight().Padding(12,20,12,6)
                        [Text(TEXT("Captain the ship.\nCommand your crew."),18,Cream)]
                        +SVerticalBox::Slot().AutoHeight().Padding(12,0,12,28)
                        [Text(TEXT("Beyond the storm, a lost world awaits."),16,Muted)]
                        +SVerticalBox::Slot().AutoHeight()
                        [
                            SAssignNew(StartButton,SButton).ButtonStyle(&Primary).HAlign(HAlign_Center).ContentPadding(FMargin(24,15))
                            .OnClicked_Lambda([Weak]() { if(Weak.IsValid()) { Weak->StartVoyage(); } return FReply::Handled(); })
                            [Text(TEXT("BEGIN ODYSSEY"),21,FLinearColor(.018f,.035f,.038f))]
                        ]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)
                        [
                            SNew(SHorizontalBox)
                            +SHorizontalBox::Slot().FillWidth(3.f).Padding(0,0,5,0)
                            [SNew(SButton).ButtonStyle(&Secondary).HAlign(HAlign_Center).ContentPadding(FMargin(12,12))
                                .OnClicked_Lambda([WeakInstructions]() { if(auto Info=WeakInstructions.Pin()) {
                                    Info->SetVisibility(Info->GetVisibility()==EVisibility::Visible?EVisibility::Collapsed:EVisibility::Visible);
                                } return FReply::Handled(); })[Text(TEXT("HOW TO SAIL"),15,Cream)]]
                            +SHorizontalBox::Slot().FillWidth(1.f).Padding(5,0,0,0)
                            [SNew(SButton).ButtonStyle(&Secondary).HAlign(HAlign_Center).ContentPadding(FMargin(12,12))
                                .OnClicked_Lambda([Weak]() { if(Weak.IsValid()) { UKismetSystemLibrary::QuitGame(Weak.Get(),Weak->GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false); } return FReply::Handled(); })
                                [Text(TEXT("QUIT"),15,Muted)]]
                        ]
                        +SVerticalBox::Slot().AutoHeight().Padding(12,22,12,0)
                        [Text(TEXT("WASD  Explore    E  Take a station    Hold V  Speak"),12,Muted)]
                    ]
                ]
            ]
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(24)
            [SNew(SBox).WidthOverride(620)[Instructions.ToSharedRef()]]
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(32,18)
            [SNew(SBox).WidthOverride(260)[Text(TEXT("A voyage into the unknown"),13,Cream)]]
        ]
    ];
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MainMenuWidget.ToSharedRef(),100);
    PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(StartButton).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    UE_LOG(LogDC,Log,TEXT("MAIN_MENU shown; voyage held at open sea"));
}

void ADCGameMode::TickMainMenu(float DeltaSeconds)
{
    const float PreviousMenuAge=MenuAge;
    MenuAge+=FApp::GetDeltaTime();
    if (Director) { Director->TimeScale=0.f; }
    // Hold the route, not the world: ocean simulation, sail animation and the
    // ship's sampled heave/pitch continue behind the title.
    TickShipMotion(DeltaSeconds);
    if(MenuCamera) {
        const FVector Position(-6200.f,-10800.f+90.f*FMath::Sin(MenuAge*.11f),4100.f+45.f*FMath::Sin(MenuAge*.16f));
        const FVector LookAt(2000.f,-1700.f,1800.f);
        MenuCamera->SetActorLocationAndRotation(Position,(LookAt-Position).Rotation());
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuCapture")) && MenuAge>5.f && !bMenuCaptured) {
        bMenuCaptured=true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/MainMenu.png"),true,false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuMotionReview")) && PreviousMenuAge<9.f && MenuAge>=9.f) {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/MainMenuMotion.png"),true,false);
        UE_LOG(LogDC,Log,TEXT("MAIN_MENU animated backdrop; route held at %.2f"),Director?Director->RideTime:-1.f);
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
