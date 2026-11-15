#include "DCPirateCastReview.h"
#include "DCCrew.h"
#include "DCPlayer.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

ADCPirateCastReviewGameMode::ADCPirateCastReviewGameMode()
{
	DefaultPawnClass = nullptr;
	PrimaryActorTick.bCanEverTick = true;
}

void ADCPirateCastReviewGameMode::BeginPlay()
{
	Super::BeginPlay();
	Output = FPaths::ProjectSavedDir() / TEXT("PirateCast/Review");
	FParse::Value(FCommandLine::Get(), TEXT("PirateReviewOutput="), Output);
	IFileManager::Get().MakeDirectory(*Output, true);
	AStaticMeshActor* Floor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0,0,-5), FRotator::ZeroRotator);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(8,10,.1));
	ADirectionalLight* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-35,160,0));
	Key->GetLightComponent()->SetIntensity(5.f);
	APointLight* Fill = GetWorld()->SpawnActor<APointLight>(FVector(300,-350,250), FRotator::ZeroRotator);
	Fill->PointLightComponent->SetIntensity(12000.f);
	Fill->PointLightComponent->SetAttenuationRadius(1500.f);
	if (FParse::Param(FCommandLine::Get(), TEXT("PirateReviewStandalone")))
	{
		UClass* Standalone = LoadClass<AActor>(nullptr, TEXT("/Game/PirateCast/Assembled/Odessa/BP_Odessa.BP_Odessa_C"));
		for (int32 Variant = 0; Variant < 3; ++Variant)
		{
			AActor* Actor = GetWorld()->SpawnActor<AActor>(Standalone, FVector(0, (Variant-1)*160, 0), FRotator(0,-90,0));
			TInlineComponentArray<UGroomComponent*> Grooms(Actor);
			for (UGroomComponent* Groom : Grooms)
			{
				if (Groom->GetName() != TEXT("Hair")) { continue; }
				if (Variant == 1) { Groom->SetBindingAsset(nullptr); }
				if (Variant == 2)
				{
					UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/HairStrands/Materials/HairDefaultMaterial.HairDefaultMaterial"));
					for (int32 Slot=0; Slot<Groom->GetNumMaterials(); ++Slot) { Groom->SetMaterial(Slot, Material); }
				}
			}
		}
		ReviewCamera = GetWorld()->SpawnActor<ACameraActor>(FVector(800,0,95), FRotator(0,180,0));
		ReviewCamera->GetCameraComponent()->SetFieldOfView(45.f);
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->SetViewTarget(ReviewCamera); }
		return;
	}
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADCPlayerCharacter* Captain = GetWorld()->SpawnActor<ADCPlayerCharacter>(FVector(0,-240,90), FRotator::ZeroRotator, Spawn);
	Captain->GetCharacterMovement()->DisableMovement();
	Captain->GetCharacterMovement()->SetComponentTickEnabled(false);
	Captain->SetActorTickEnabled(false);
	Captain->SetActorEnableCollision(false);
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	Crew->SpawnCrew(TEXT("Rex"), FLinearColor(.9f,.7f,.25f), FVector(0,-80,0));
	Crew->SpawnCrew(TEXT("Duncan"), FLinearColor(.35f,.7f,.95f), FVector(0,80,0));
	Crew->SpawnCrew(TEXT("Ruby"), FLinearColor(.9f,.4f,.5f), FVector(0,240,0));
	if (FParse::Param(FCommandLine::Get(), TEXT("CrewPortraits")))
	{
		for (ADCCrewMember* Member : Crew->GetCrew())
		{
			if (UTextRenderComponent* Label = Member->FindComponentByClass<UTextRenderComponent>()) { Label->SetHiddenInGame(true); }
		}
	}
	ReviewCamera = GetWorld()->SpawnActor<ACameraActor>(FVector(800,0,100), FRotator(0,180,0));
	ReviewCamera->GetCameraComponent()->SetFieldOfView(45.f);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->Possess(Captain); PC->SetViewTarget(ReviewCamera); }
}

void ADCPirateCastReviewGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	if (FParse::Param(FCommandLine::Get(), TEXT("CrewPortraits")))
	{
		if (Elapsed < 8.f + Stage * 2.f) { return; }
		if (Stage >= 6) { FPlatformMisc::RequestExit(false); return; }
		const TCHAR* Names[] = { TEXT("Rex"),TEXT("Duncan"),TEXT("Ruby") };
		if (Stage % 2 == 0)
		{
			UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
			for (ADCCrewMember* Member : Crew->GetCrew())
			{
				if (Member->CrewName != Names[Stage / 2]) { continue; }
				UChildActorComponent* Child = Member->FindComponentByClass<UChildActorComponent>();
				if (!Child || !Child->GetChildActor()) { continue; }
				TInlineComponentArray<USkeletalMeshComponent*> Meshes(Child->GetChildActor());
				for (USkeletalMeshComponent* Mesh : Meshes)
				{
					if (Mesh->GetName() != TEXT("Face")) { continue; }
					const FVector Focus = Mesh->GetBoneLocation(TEXT("FACIAL_C_Jaw")) + FVector(0,0,6);
					ReviewCamera->SetActorLocation(Focus + FVector(200,0,0));
					ReviewCamera->SetActorRotation(FRotator(0,180,0));
					ReviewCamera->GetCameraComponent()->SetFieldOfView(22.f);
                    ReviewCamera->GetCameraComponent()->bConstrainAspectRatio = false;
					ReviewCamera->GetCameraComponent()->PostProcessSettings.bOverride_AutoExposureBias = true;
					ReviewCamera->GetCameraComponent()->PostProcessSettings.AutoExposureBias = -1.f;
				}
			}
		}
		else { FScreenshotRequest::RequestScreenshot(Output / FString::Printf(TEXT("T_Crew_%s.png"),Names[Stage/2]),false,false); }
		++Stage;
		return;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("PirateReviewStandalone")) && Elapsed > 14.f)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("PirateReviewAutoQuit"))) { FPlatformMisc::RequestExit(false); }
		return;
	}
	if (Stage == 0 && Elapsed > 12.f)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->ConsoleCommand(TEXT("r.HairStrands.Dump.GroomComponent 1"), true); }
		for (const TCHAR* Name : { TEXT("r.HairStrands.Enable"), TEXT("r.HairStrands.Strands"), TEXT("r.HairStrands.UseCardsInsteadOfStrands"), TEXT("r.SkinCache.Mode"), TEXT("r.SkinCache.DefaultBehavior") })
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name)) { UE_LOG(LogTemp, Log, TEXT("Pirate render: %s=%s"), Name, *Var->GetString()); }
		}
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			TInlineComponentArray<UGroomComponent*> Grooms(*It);
			for (UGroomComponent* Groom : Grooms)
			{
				if (Groom->GroomAsset)
				{
					for (const FHairGroupInfoWithVisibility& Group : Groom->GroomAsset->GetHairGroupsInfo())
					{
						UE_LOG(LogTemp, Log, TEXT("Pirate groom data: %s/%s curves=%d guides=%d visible=%d valid=%d"), *It->GetName(), *Groom->GetName(), Group.NumCurves, Group.NumGuides, Group.bIsVisible, Groom->GroomAsset->IsValid());
					}
				}
				UE_LOG(LogTemp, Log, TEXT("Pirate groom: %s/%s visible=%d hidden=%d cards=%d LOD=%d bounds=%s binding=%s parent=%s"),
					*It->GetName(), *Groom->GetName(), Groom->IsVisible(), Groom->bHiddenInGame, Groom->bUseCards,
					Groom->GetForcedLOD(), *Groom->Bounds.ToString(), *GetNameSafe(Groom->BindingAsset), *GetNameSafe(Groom->GetAttachParent()));
			}
			TInlineComponentArray<USkeletalMeshComponent*> Meshes(*It);
			for (USkeletalMeshComponent* Mesh : Meshes)
			{
				UE_LOG(LogTemp, Log, TEXT("Pirate review: %s actor=%s mesh=%s world=%s relative=%s visible=%d hidden=%d ownerNoSee=%d"),
					*It->GetName(), *It->GetActorLocation().ToString(), *Mesh->GetName(), *Mesh->GetComponentLocation().ToString(),
					*Mesh->GetRelativeLocation().ToString(), Mesh->IsVisible(), Mesh->bHiddenInGame, Mesh->bOwnerNoSee);
			}
		}
		FScreenshotRequest::RequestScreenshot(Output / TEXT("00-cast.png"), false, false);
		Stage = 1;
	}
	const TCHAR* Roles[] = { TEXT("Captain"), TEXT("Rex"), TEXT("Duncan"), TEXT("Ruby") };
	if (Stage >= 1 && Stage <= 8)
	{
		const int32 Index = (Stage - 1) / 2;
		if (Elapsed > 14.f + (Stage - 1) * 3.f)
		{
			if (Stage % 2 == 1)
			{
				ReviewCamera->SetActorLocation(FVector(450,-240 + Index * 160,95));
				ReviewCamera->GetCameraComponent()->SetFieldOfView(42.f);
			}
			else { FScreenshotRequest::RequestScreenshot(Output / FString::Printf(TEXT("%02d-%s.png"), Index + 1, Roles[Index]), false, false); }
			++Stage;
		}
	}
	if (Stage == 9 && Elapsed > 40.f)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->SetViewTarget(PC->GetPawn());
			PC->SetControlRotation(FRotator::ZeroRotator);
		}
		Stage = 10;
	}
	if (Stage == 10 && Elapsed > 44.f)
	{
		FScreenshotRequest::RequestScreenshot(Output / TEXT("05-first-person.png"), false, false);
		Stage = 11;
	}
	if (Stage == 11 && Elapsed > 45.f)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->SetControlRotation(FRotator(-60,0,0)); }
		Stage = 12;
	}
	if (Stage == 12 && Elapsed > 48.f)
	{
		FScreenshotRequest::RequestScreenshot(Output / TEXT("06-first-person-down.png"), false, false);
		Stage = 13;
	}
	if (Stage == 13 && Elapsed > 50.f)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->SetViewTarget(ReviewCamera); }
		ReviewCamera->SetActorLocation(FVector(800,0,100));
		ReviewCamera->GetCameraComponent()->SetFieldOfView(45.f);
		UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
		for (ADCCrewMember* Member : Crew->GetCrew())
		{
			const int32 Index = Crew->GetStations().Num();
			Crew->AddStation(EDCStationType::Helm, EDCSide::None, Member->GetActorLocation() + FVector(1000,0,-90));
			Member->AssignStation(Index);
		}
		Stage = 14;
	}
	if (Stage == 14 && Elapsed > 50.6f)
	{
		FScreenshotRequest::RequestScreenshot(Output / TEXT("07-walking.png"), false, false);
		for (ADCCrewMember* Member : GetWorld()->GetSubsystem<UDCCrewSubsystem>()->GetCrew())
		{
			UE_LOG(LogTemp, Log, TEXT("Pirate walking: %s state=%d location=%s"), *Member->CrewName, static_cast<int32>(Member->State), *Member->GetActorLocation().ToString());
		}
		Stage = 15;
	}
	if (Stage == 15 && Elapsed > 54.f)
	{
		UE_LOG(LogTemp, Log, TEXT("Pirate cast review complete: %s"), *Output);
		if (FParse::Param(FCommandLine::Get(), TEXT("PirateReviewAutoQuit"))) { FPlatformMisc::RequestExit(false); }
		Stage = 16;
	}
}
