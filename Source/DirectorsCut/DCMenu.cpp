#include "DCGame.h"
#include "DCRideDirector.h"
#include "DCCrew.h"
#include "DCVoice.h"
#include "DCAudio.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
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
#include "EngineUtils.h"

void ADCGameMode::SetMenuInfoPage(int32 Page)
{
    MenuInfoPage=MenuInfoPage==Page?0:Page;
    if(MainMenuWidget.IsValid()) { MainMenuWidget->Invalidate(EInvalidateWidgetReason::Layout); }
    UE_LOG(LogDC,Log,TEXT("MAIN_MENU panel=%d ride=%.2f"),MenuInfoPage,Director?Director->RideTime:-1.f);
}

void ADCGameMode::ShowMainMenu()
{
    if(!Director || !GetWorld()->GetGameViewport()) { bAwaitingStart=false; return; }
    Director->Seek(0.f); Director->TimeScale=0.f; MenuInfoPage=0;
    MenuWaterReadyAge=-1.f; MenuWaterReadyFrames=0; MenuReveal=0.f; MenuWaterManager=nullptr;
    APlayerController* PC=GetWorld()->GetFirstPlayerController();
    if(!PC) { return; }
    PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true); PC->bShowMouseCursor=true;
    const FVector Position(-6200.f,-10800.f,4100.f), LookAt(2000.f,-1700.f,1800.f);
    MenuCamera=GetWorld()->SpawnActor<ACameraActor>(Position,(LookAt-Position).Rotation());
    MenuCamera->GetCameraComponent()->SetFieldOfView(58.f);
    MenuCamera->GetCameraComponent()->bConstrainAspectRatio=false;
    PC->SetViewTarget(MenuCamera);
    const FLinearColor Cream(.93f,.87f,.70f), Gold(.72f,.49f,.20f), Muted(.46f,.64f,.65f);
    auto Body=[](const TCHAR* Line,int Size,FLinearColor Color) {
        return SNew(STextBlock).Text(FText::FromString(Line)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size))
            .ColorAndOpacity(Color).AutoWrapText(true);
    };
    auto Title=[](const TCHAR* Line,int Size,FLinearColor Color) {
        return SNew(STextBlock).Text(FText::FromString(Line))
            .Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("DC/UI/Fonts/Cinzel.ttf"),Size))
            .ColorAndOpacity(Color).Justification(ETextJustify::Center);
    };
    static TStrongObjectPtr<UTexture2D> Logo(LoadObject<UTexture2D>(nullptr,TEXT("/Game/DC/UI/T_AtlanteanOdysseyLogoV2.T_AtlanteanOdysseyLogoV2")));
    static TStrongObjectPtr<UTexture2D> Backdrop(LoadObject<UTexture2D>(nullptr,TEXT("/Game/DC/UI/T_AtlanteanMenuPanel.T_AtlanteanMenuPanel")));
    static FSlateBrush LogoBrush,BackdropBrush;
    if(Logo.IsValid()) { LogoBrush.SetResourceObject(Logo.Get()); LogoBrush.ImageSize=FVector2D(420,210); }
    if(Backdrop.IsValid()) { BackdropBrush.SetResourceObject(Backdrop.Get()); BackdropBrush.ImageSize=FVector2D(500,720); }
    static const FSlateRoundedBoxBrush InfoPanel(FLinearColor(.004f,.017f,.022f,.97f),2.f,FLinearColor(.44f,.30f,.13f),1.f);
    static const FSlateRoundedBoxBrush Divider(FLinearColor(.49f,.32f,.13f),0.f);
    static const FSlateRoundedBoxBrush Warmup(FLinearColor(.004f,.017f,.022f),0.f);
    static const FButtonStyle Primary=FButtonStyle()
        .SetNormal(FSlateRoundedBoxBrush(FLinearColor(.10f,.065f,.027f,.95f),2.f,FLinearColor(.78f,.53f,.20f),2.f))
        .SetHovered(FSlateRoundedBoxBrush(FLinearColor(.24f,.14f,.045f,.98f),2.f,FLinearColor(1.f,.77f,.38f),2.f))
        .SetPressed(FSlateRoundedBoxBrush(FLinearColor(.05f,.035f,.015f),2.f,FLinearColor(.85f,.62f,.28f),2.f))
        .SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.015f,.025f,.027f),2.f,FLinearColor(.24f,.19f,.10f),1.f));
    static const FButtonStyle Secondary=FButtonStyle()
        .SetNormal(FSlateRoundedBoxBrush(FLinearColor(.005f,.025f,.031f,.9f),2.f,FLinearColor(.45f,.29f,.10f),1.f))
        .SetHovered(FSlateRoundedBoxBrush(FLinearColor(.022f,.075f,.08f,.96f),2.f,FLinearColor(.88f,.65f,.28f),1.5f))
        .SetPressed(FSlateRoundedBoxBrush(FLinearColor(.012f,.04f,.045f),2.f,FLinearColor(.85f,.62f,.28f),1.5f));
    TWeakObjectPtr<ADCGameMode> Weak(this);
    TSharedPtr<SButton> StartButton;
    auto Help=SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[Title(TEXT("HOW TO PLAY"),27,Gold)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,22)[Body(TEXT("Captain the Ship. Direct the Crew. Survive the Storm"),18,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)[Body(TEXT("TAKE COMMAND"),12,Gold)]
        +SVerticalBox::Slot().AutoHeight()[Body(TEXT("WASD  Move  -  Mouse  Look\nE  -  Take / Leave Station\nAt the wheel: A/D to steer the ship\nAt a cannon: mouse to aim, left click to fire\nT  -  Toggle camera to ship view"),17,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,22,0,6)[Body(TEXT("DIRECT YOUR CREW"),12,Gold)]
        +SVerticalBox::Slot().AutoHeight()[Body(TEXT("Hold V to speak and give an order\n\"Rex, take the helm!\"\n\"Duncan, man the port guns!\"\n\"Ruby, put out the fires!\""),17,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,22,0,6)[Body(TEXT("SURVIVE THE VOYAGE"),12,Gold)]
        +SVerticalBox::Slot().AutoHeight()[Body(TEXT("Steer clear of obstacles.\nShoot enemy creatures with the cannons.\nPut out fires and repair rigging."),17,Cream)];
    auto Credits=SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[Title(TEXT("CREDITS"),24,Gold)]
        +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,4,0,14)[Body(TEXT("2026 Epic Game MegaJam"),12,Muted)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,14)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(.32f).Padding(0,0,18,0)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[Body(TEXT("DIRECTOR"),11,Gold)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,3,0,0)[Body(TEXT("Eva Jobse"),16,Cream)]]
            +SHorizontalBox::Slot().FillWidth(.68f)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[Body(TEXT("AI"),11,Gold)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,3,0,0)[Body(TEXT("GPT-6 Astra, Claude Fable 5.2,\nQwen 3.8 27B, Google Gemma E4B,\nWhisperCPP, Phonon, Suno"),16,Cream)]]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,5)[Body(TEXT("ART & ASSET PACKS"),11,Gold)]
        +SVerticalBox::Slot().AutoHeight()[Body(TEXT("Brigantine Ship  -  MainStore\nDynamic Real Water  -  Natural Level Game Studio\nUltra Dynamic Sky & Weather  -  Everett Gunther\nModular Ancient Ruins  -  archafizov\nKraken  -  PROTOFACTOR INC\nClaw Kaiju Monster  -  HuyKhoi2407\nMHC: Pirates  -  Quang Phan\nEasy Fog  -  William Faucher\nClouds Tunnel Vdb Effects Pack Loop  -  one.studio2\nNordic coastal cliff scans  -  Quixel Megascans\nParagon: Rampage, MetaHuman & engine art  -  Epic Games"),13,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,4)[Body(TEXT("THANKS TO"),11,Gold)]
        +SVerticalBox::Slot().AutoHeight()[Body(TEXT("Epic Games, Unreal Engine, NVIDIA, LlamaCPP,\nGeorgi Gerganov, Gradium, Laurent Mazare,\nKyutai, NVIDIA ACE / Audio2Face, DLSS"),16,Cream)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,4)[Body(TEXT("AUDIO & TYPE"),11,Gold)]
        +SVerticalBox::Slot().AutoHeight()[Body(TEXT("Sonniss: Just Sound Effects, 344 Audio, InMotionAudio, Jake Fielding, Epic Stock Media, SoundBits.\nCinzel: The Cinzel Project Authors (SIL Open Font License)."),11,Muted)];
    MainMenuWidget=SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
    [SNew(SBox).WidthOverride(1280).HeightOverride(720)
        [SNew(SOverlay)
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Fill)
            [SNew(SBox).WidthOverride(780)
                .Visibility_Lambda([Weak](){return Weak.IsValid() && Weak->MenuReveal<1.f?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
                [SNew(SBorder).BorderImage(&Warmup).HAlign(HAlign_Center).VAlign(VAlign_Center)
                    .BorderBackgroundColor_Lambda([Weak](){return FLinearColor(1.f,1.f,1.f,Weak.IsValid()?1.f-Weak->MenuReveal:0.f);})
                    .ColorAndOpacity_Lambda([Weak](){return FLinearColor(1.f,1.f,1.f,Weak.IsValid()?1.f-Weak->MenuReveal:0.f);})
                    [Title(TEXT("Preparing the sea..."),18,Cream)]]]
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
            [SNew(SBox).WidthOverride(500)[SNew(SImage).Image(&BackdropBrush)]]
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(25,26,0,0)
            [SNew(SBox).WidthOverride(425)
                [SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(214)[SNew(SImage).Image(&LogoBrush)]]
                    +SVerticalBox::Slot().AutoHeight().Padding(30,22,30,14)[SNew(SBox).HeightOverride(1)[SNew(SImage).Image(&Divider)]]
                    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                    [SNew(STextBlock).Text(FText::FromString(TEXT("Captain a ship into myth and storm.")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Italic",15)).ColorAndOpacity(Cream)]
                    +SVerticalBox::Slot().AutoHeight().Padding(48,15,48,22)[SNew(SBox).HeightOverride(1)[SNew(SImage).Image(&Divider)]]
                    +SVerticalBox::Slot().AutoHeight().Padding(24,0,24,12)
                    [SNew(SBox).HeightOverride(58)[SAssignNew(StartButton,SButton).ButtonStyle(&Primary).HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .IsEnabled_Lambda([Weak](){return Weak.IsValid() && Weak->MenuReveal>=1.f;})
                        .OnClicked_Lambda([Weak](){if(Weak.IsValid()){Weak->StartVoyage();}return FReply::Handled();})
                        [Title(TEXT("START GAME"),25,Cream)]]]
                    +SVerticalBox::Slot().AutoHeight().Padding(45,0,45,11)
                    [SNew(SBox).HeightOverride(47)[SNew(SButton).ButtonStyle(&Secondary).HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .OnClicked_Lambda([Weak](){if(Weak.IsValid()){Weak->SetMenuInfoPage(1);}return FReply::Handled();})
                        [Title(TEXT("HOW TO PLAY"),21,Cream)]]]
                    +SVerticalBox::Slot().AutoHeight().Padding(45,0,45,11)
                    [SNew(SBox).HeightOverride(47)[SNew(SButton).ButtonStyle(&Secondary).HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .OnClicked_Lambda([Weak](){if(Weak.IsValid()){Weak->SetMenuInfoPage(2);}return FReply::Handled();})
                        [Title(TEXT("CREDITS"),21,Cream)]]]
                    +SVerticalBox::Slot().AutoHeight().Padding(45,0,45,0)
                    [SNew(SBox).HeightOverride(47)[SNew(SButton).ButtonStyle(&Secondary).HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .OnClicked_Lambda([Weak](){if(Weak.IsValid()){UKismetSystemLibrary::QuitGame(Weak.Get(),Weak->GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false);}return FReply::Handled();})
                        [Title(TEXT("QUIT"),21,Cream)]]]
                ]
            ]
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0,30,30,30)
            [SNew(SBox).WidthOverride(690).HeightOverride(660)
                .Visibility_Lambda([Weak](){return Weak.IsValid() && Weak->MenuInfoPage!=0?EVisibility::Visible:EVisibility::Collapsed;})
                [SNew(SBorder).BorderImage(&InfoPanel).Padding(FMargin(32,25))
                    [SNew(SOverlay)
                        +SOverlay::Slot()[SNew(SScrollBox).Visibility_Lambda([Weak](){return Weak.IsValid() && Weak->MenuInfoPage==1?EVisibility::Visible:EVisibility::Collapsed;})
                            +SScrollBox::Slot()[Help]]
                        +SOverlay::Slot()[SNew(SScrollBox).Visibility_Lambda([Weak](){return Weak.IsValid() && Weak->MenuInfoPage==2?EVisibility::Visible:EVisibility::Collapsed;})
                            +SScrollBox::Slot()[Credits]]
                    ]
                ]
            ]
        ]
    ];
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuShowHelp"))) { SetMenuInfoPage(1); }
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuShowCredits"))) { SetMenuInfoPage(2); }
    MainMenuWidget->ForceVolatile(true);
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MainMenuWidget.ToSharedRef(),100);
    PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(StartButton).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    UE_LOG(LogDC,Log,TEXT("MAIN_MENU shown; voyage held at open sea; buttons=4"));
}

void ADCGameMode::TickMainMenu(float DeltaSeconds)
{
    const float PreviousMenuAge=MenuAge;
    MenuAge+=FApp::GetDeltaTime();
    // Let the actual ocean initialize and render beneath the menu before its
    // reveal. Hiding the ocean actor would merely defer its first-use PSOs.
    if(MenuReveal<1.f) {
        if(!MenuWaterManager) {
            UClass* GridClass=FindObject<UClass>(nullptr,TEXT("/Script/DynamicRealWater.DRW_GridManager"));
            for(TActorIterator<AActor> It(GetWorld(),GridClass?GridClass:AActor::StaticClass());GridClass && It;++It) { MenuWaterManager=*It; break; }
        }
        bool Ready=false;
        if(MenuWaterManager) {
            if(UFunction* Query=MenuWaterManager->FindFunction(TEXT("IsDRWReady"))) { MenuWaterManager->ProcessEvent(Query,&Ready); }
        }
        if(Ready) {
            if(MenuWaterReadyAge<0.f) { MenuWaterReadyAge=MenuAge; UE_LOG(LogDC,Log,TEXT("MAIN_MENU ocean ready at %.2f; settling visible scene"),MenuAge); }
            ++MenuWaterReadyFrames;
            if(MenuWaterReadyFrames>=60) { MenuReveal=FMath::Clamp((MenuAge-MenuWaterReadyAge-2.f)/.8f,0.f,1.f); }
            if(MenuReveal>=1.f) { UE_LOG(LogDC,Log,TEXT("MAIN_MENU ocean revealed at %.2f frames=%d"),MenuAge,MenuWaterReadyFrames); }
        }
    }
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
        UE_LOG(LogDC,Log,TEXT("MAIN_MENU motion at %.2f ride=%.2f heave=%.3f slope=%.4f,%.4f"),MenuAge,Director?Director->RideTime:-1.f,ShipHeave,ShipSlope.X,ShipSlope.Y);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/MainMenu.png"),true,false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuMotionReview")) && PreviousMenuAge<9.f && MenuAge>=9.f) {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/MainMenuMotion.png"),true,false);
        UE_LOG(LogDC,Log,TEXT("MAIN_MENU animated backdrop; route held at %.2f"),Director?Director->RideTime:-1.f);
        UE_LOG(LogDC,Log,TEXT("MAIN_MENU motion at %.2f ride=%.2f heave=%.3f slope=%.4f,%.4f"),MenuAge,Director?Director->RideTime:-1.f,ShipHeave,ShipSlope.X,ShipSlope.Y);
    }
    float AutoStart=0;
    if(FParse::Param(FCommandLine::Get(),TEXT("DCMenuReviewPanels"))) {
        if(PreviousMenuAge<12.f && MenuAge>=12.f) { SetMenuInfoPage(1); }
        if(PreviousMenuAge<13.f && MenuAge>=13.f) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/MainMenuHelp.png"),true,false); }
        if(PreviousMenuAge<18.f && MenuAge>=18.f) { SetMenuInfoPage(2); }
        if(PreviousMenuAge<19.f && MenuAge>=19.f) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/MainMenuCredits.png"),true,false); }
        if(PreviousMenuAge<24.f && MenuAge>=24.f) { SetMenuInfoPage(2); }
    }
    if(FParse::Value(FCommandLine::Get(),TEXT("DCMenuAutoStart="),AutoStart) && AutoStart>0 && MenuAge>=AutoStart) { StartVoyage(); }
}

void ADCGameMode::StartVoyage()
{
    if(!bAwaitingStart || MenuReveal<1.f) { return; }
    bAwaitingStart=false;
    if(ADCAudio* Audio=Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(this,ADCAudio::StaticClass())))
    {
        Audio->FadeMenuMusicOut();
    }
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
