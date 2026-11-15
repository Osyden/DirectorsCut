// Explicitly opt-in native gameplay footage capture. No delegate is registered in normal play.
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "DCRideDirector.h"
#include "DCCrew.h"
#include "DCLeviathan.h"

namespace DCTrailer
{
struct FCapture
{
    FString Mode, Folder, MemberName = TEXT("Ruby"), Times;
    TWeakObjectPtr<UWorld> BoundWorld;
    TWeakObjectPtr<ACameraActor> Camera;
    float Age = 0.f, Warmup = 5.f, Duration = 12.f;
    int32 Frame = 0;
    bool Started = false, Done = false, Realtime = false;
    void Tick(UWorld* World, ELevelTick, float Delta)
    {
        if (Done || !World || !World->IsGameWorld() || !World->HasBegunPlay()) return;
        if (BoundWorld.IsValid() && BoundWorld.Get() != World) return;
        APlayerController* PC = World->GetFirstPlayerController();
        ADCRideDirector* Director = ADCRideDirector::Get(World);
        if (!PC || !Director) return;
        if (!Started)
        {
            Started = true; BoundWorld = World;
            FParse::Value(FCommandLine::Get(), TEXT("DCTrailerSeconds="), Duration);
            FParse::Value(FCommandLine::Get(), TEXT("DCTrailerWarmup="), Warmup);
            FParse::Value(FCommandLine::Get(), TEXT("DCTrailerCrew="), MemberName);
            Realtime = FParse::Param(FCommandLine::Get(), TEXT("DCTrailerRealtime"));
            Duration = FMath::Clamp(Duration, 1.f, 60.f);
            if (!Realtime) { FApp::SetFixedDeltaTime(1.0 / 30.0); FApp::SetUseFixedTimeStep(true); }
            Folder = FPaths::ProjectSavedDir() / TEXT("Trailer") / (Mode + TEXT("-") + FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")));
            FParse::Value(FCommandLine::Get(), TEXT("DCTrailerOutput="), Folder);
            IFileManager::Get().MakeDirectory(*Folder, true);
            if (Mode != TEXT("combat") && Mode != TEXT("bosscombat") && Mode != TEXT("deck"))
            {
                Camera = World->SpawnActor<ACameraActor>();
                Camera->GetCameraComponent()->bConstrainAspectRatio = false;
                PC->SetViewTarget(Camera.Get());
            }
            UE_LOG(LogTemp, Display, TEXT("TRAILER begin mode=%s output=%s duration=%.1f"), *Mode, *Folder, Duration);
        }
        Age += Delta;
        const float T = FMath::Clamp((Age - Warmup) / Duration, 0.f, 1.f);
        const float Ease = T*T*(3.f-2.f*T);
        if (Camera.IsValid())
        {
            FVector Position, Focus;
            float Fov = 60.f;
            if (Mode == TEXT("crew") || Mode == TEXT("jobs"))
            {
                ADCCrewMember* Member = nullptr;
                for (TActorIterator<ADCCrewMember> It(World); It; ++It)
                    if (It->CrewName.Equals(MemberName, ESearchCase::IgnoreCase)) { Member = *It; break; }
                if (!Member) return;
                USkeletalMeshComponent* Figure = Member->FindComponentByClass<USkeletalMeshComponent>();
                Focus = Figure && Figure->DoesSocketExist(TEXT("head")) ? Figure->GetSocketLocation(TEXT("head")) : Member->GetActorLocation()+FVector(0,0,70);
                const float Yaw = Member->GetActorRotation().Yaw;
                const FVector Forward = FRotator(0,Yaw,0).Vector(), Right = FRotator(0,Yaw+90.f,0).Vector();
                if (Mode == TEXT("crew")) { Position = Focus + Forward*FMath::Lerp(260.f,220.f,Ease) + Right*FMath::Lerp(-95.f,-45.f,Ease) + FVector(0,0,10); Fov=38.f; }
                else { Focus.Z-=40.f; Position=Focus+Forward*560.f+Right*FMath::Lerp(-420.f,-230.f,Ease)+FVector(0,0,160.f); Fov=56.f; }
            }
            else if (Mode == TEXT("kraken"))
            {
                Position=Director->DeckToWorld(FVector(-1900.f,FMath::Lerp(-1100.f,-450.f,Ease),1700.f));
                Focus=Director->DeckToWorld(FVector(1000,0,1400));
                for (TActorIterator<ADCLeviathan> It(World); It; ++It) { Focus=It->GetMouthPosition(); break; }
                Fov=FMath::Lerp(69.f,56.f,Ease);
            }
            else
            {
                const bool Fog=Mode==TEXT("fog");
                Focus=Director->DeckToWorld(FVector(200,0,Fog?1000.f:1100.f));
                const float Angle=FMath::DegreesToRadians(FMath::Lerp(Fog?205.f:142.f,Fog?188.f:113.f,Ease));
                const float Radius=FMath::Lerp(Fog?6200.f:7200.f,Fog?5100.f:6100.f,Ease);
                Position=Focus+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,FMath::Lerp(1700.f,1100.f,Ease));
                Fov=Fog?64.f:57.f;
            }
            Camera->SetActorLocationAndRotation(Position,(Focus-Position).Rotation());
            Camera->GetCameraComponent()->SetFieldOfView(Fov);
            PC->SetViewTarget(Camera.Get());
        }
        if (Age < Warmup) return;
        if (Age >= Warmup + Duration)
        {
            FFileHelper::SaveStringToFile(Times, *(Folder / TEXT("frames.csv")));
            UE_LOG(LogTemp, Display, TEXT("TRAILER complete frames=%d output=%s"), Frame, *Folder);
            Done=true; FApp::SetUseFixedTimeStep(false); FPlatformMisc::RequestExit(false); return;
        }
        const FString Name=FString::Printf(TEXT("frame_%05d.png"),Frame++);
        Times+=FString::Printf(TEXT("%s,%.6f\n"),*Name,Age-Warmup);
        FScreenshotRequest::RequestScreenshot(Folder / Name,false,false);
    }
};
static FCapture Capture;
static FDelegateHandle RegisterCapture()
{
    if (!FParse::Value(FCommandLine::Get(),TEXT("DCTrailer="),Capture.Mode)) return FDelegateHandle();
    return FWorldDelegates::OnWorldPostActorTick.AddStatic(+[](UWorld* W,ELevelTick Tick,float Delta){ Capture.Tick(W,Tick,Delta); });
}
static FDelegateHandle Handle=RegisterCapture();
}
