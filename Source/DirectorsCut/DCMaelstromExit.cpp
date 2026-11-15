#include "DCMaelstrom.h"
#include "DCLeviathan.h"
#include "DCRideDirector.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void ADCMaelstrom::UpdateExitTransition()
{
    if(!Director) { ResetExitTransition(); return; }
    const float Time=Director->RideTime;
    // The current boss finale is EndingStart-20. Give its death/withdrawal
    // ten seconds, then engulf the ship. The stage and sky switch normally
    // at EndingStart, fully concealed inside this four-second opaque hold.
    const float FogBegins=EndingStart-10.f;
    const float FullyHidden=EndingStart-2.f;
    const float ClearingBegins=EndingStart+2.f;
    const float ClearAt=EndingStart+7.f;
    if(Time>=FogBegins&&Director->WhiteOut>0.f)
    {
        // The director ticks before us and has already applied its old
        // ending exposure flash. Remove that exact contribution, including
        // direct seeks past the fog window, then prevent it on later frames.
        TInlineComponentArray<UPostProcessComponent*> Grades(Director);
        for(UPostProcessComponent* Grade:Grades)
        { Grade->Settings.AutoExposureBias-=7.f*FMath::Square(Director->WhiteOut); }
        Director->WhiteOut=0.f;
    }
    if(Time<FogBegins||Time>=ClearAt)
    {
        // Covers backward/forward seeks, restart and completion. No timer can
        // leave a held camera fade behind when ride time jumps elsewhere.
        ResetExitTransition();
        return;
    }

    const float Engulf=FMath::SmoothStep(FogBegins,FullyHidden,Time);
    const float Release=1.f-FMath::SmoothStep(ClearingBegins,ClearAt,Time);
    const float Fade=FMath::SmoothStep(FogBegins+2.f,FullyHidden,Time)*Release;
    const float Density=Engulf*Release;
    const FLinearColor FogColor(.58f,.66f,.70f);

    if(ExitFogVolumes.IsEmpty())
    {
        static const float RadiiM[]={145.f,170.f,125.f,205.f};
        for(int32 I=0;I<4;++I)
        {
            ULocalFogVolumeComponent* Fog=NewObject<ULocalFogVolumeComponent>(this);
            AddInstanceComponent(Fog);
            Fog->SetupAttachment(RootComponent);
            Fog->SetMobility(EComponentMobility::Movable);
            Fog->SetRelativeScale3D(FVector(RadiiM[I]*100.f/ULocalFogVolumeComponent::GetBaseVolumeSize()));
            Fog->RadialFogExtinction=0.f;
            Fog->HeightFogExtinction=0.f;
            Fog->FogAlbedo=FLinearColor(.76f,.82f,.85f);
            Fog->FogEmissive=FLinearColor::Black;
            Fog->FogPhaseG=.35f;
            Fog->FogStartDistance=0.f;
            Fog->FogSortPriority=127;
            Fog->RegisterComponent();
            ExitFogVolumes.Add(Fog);
        }
    }
    // Unequal soft volumes drift in from around the hull before the final
    // pale occlusion. Their ship/world frame remains stable through the sea
    // swap; they do not inherit the crater's moving arena transform.
    static const FVector Banks[]={FVector(26000,-14000,3000),FVector(15000,22000,1800),
        FVector(-18000,7000,2400),FVector(6000,-5000,4500)};
    const FVector ShipEye(0.f,0.f,Director->SeaLevelZ+800.f);
    for(int32 I=0;I<ExitFogVolumes.Num();++I)
    {
        ULocalFogVolumeComponent* Fog=ExitFogVolumes[I];
        if(!Fog) { continue; }
        FVector Offset=Banks[I];
        Offset.X*=1.f-.82f*Engulf;
        Offset.Y*=1.f-.82f*Engulf;
        Offset.Y+=FMath::Sin((Time-FogBegins)*.45f+I*1.7f)*1400.f*Engulf;
        Fog->SetWorldLocation(ShipEye+Offset);
        Fog->SetRadialFogExtinction(Density*(I==3?.35f:.70f));
        Fog->SetVisibility(Density>UE_SMALL_NUMBER);
    }

    APlayerController* PC=GetWorld()->GetFirstPlayerController();
    APlayerCameraManager* Camera=PC?PC->PlayerCameraManager.Get():nullptr;
    if(ExitFogCamera.Get()!=Camera&&bExitFogOwnsFade)
    {
        if(APlayerCameraManager* Previous=ExitFogCamera.Get()) { Previous->StopCameraFade(); }
        bExitFogOwnsFade=false;
    }
    if(Camera)
    {
        Camera->SetManualCameraFade(Fade,FogColor,false);
        ExitFogCamera=Camera;
        bExitFogOwnsFade=true;
    }
    // The director's legacy ending white-out is only suppressed within this
    // local transition. It is fully hidden on its first Ending tick, before
    // the normal open-sea exposure resumes on the following frame.
    Director->WhiteOut=0.f;
    const int32 Phase=Time<FullyHidden?0:(Time<=ClearingBegins?1:2);
    if(Phase!=ExitFogPhase)
    {
        ExitFogPhase=Phase;
        UE_LOG(LogTemp,Display,TEXT("DC_MAEL_EXIT_FOG phase=%d ride=%.2f alpha=%.3f local_volumes=%d"),Phase,Time,Fade,ExitFogVolumes.Num());
    }
    // Preserve the existing boss-completion hook, but never expose its cut.
    // Current clock-driven completion normally coincides with EndingStart.
    if(bActive&&Boss&&Boss->ShouldExitEncounter(Elapsed)&&Fade>=.999f)
    {
        Director->Seek(EndingStart);
    }
}

void ADCMaelstrom::ResetExitTransition()
{
    for(ULocalFogVolumeComponent* Fog:ExitFogVolumes)
    { if(Fog) { Fog->SetVisibility(false); } }
    if(bExitFogOwnsFade)
    {
        if(APlayerCameraManager* Camera=ExitFogCamera.Get()) { Camera->StopCameraFade(); }
        UE_LOG(LogTemp,Display,TEXT("DC_MAEL_EXIT_FOG cleared"));
    }
    ExitFogCamera.Reset();
    bExitFogOwnsFade=false;
    ExitFogPhase=INDEX_NONE;
}
