#include "DCPlayer.h"
#include "DCCrew.h"
#include "DCTreadmill.h"
#include "DCGunnery.h"
#include "DCRideDirector.h"
#include "DCTroubles.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

ADCPlayerCharacter::ADCPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
}

void ADCPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) { return; }

	if (bTyping) { return; }
	if (PC->WasInputKeyJustPressed(EKeys::Enter)) { OpenOrderBox(PC); return; }
	TickMeta(PC);

	switch (ActiveType)
	{
	case EDCStationType::Helm: TickHelm(PC); break;
	case EDCStationType::Cannons: TickCannons(PC); break;
	default: TickOnFoot(PC); break;
	}

	// Quick orders for players without a microphone.
	static const TPair<FKey, const TCHAR*> QuickOrders[] = {
		{ EKeys::One, TEXT("Hard to port") }, { EKeys::Two, TEXT("Hard to starboard") }, { EKeys::Three, TEXT("Steady as she goes") },
		{ EKeys::Four, TEXT("Port battery, fire") }, { EKeys::Five, TEXT("Starboard battery, fire") }, { EKeys::Six, TEXT("Hold fire") } };
	for (const TPair<FKey, const TCHAR*>& Q : QuickOrders)
	{
		if (PC->WasInputKeyJustPressed(Q.Key))
		{
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->ExecuteOrder(Q.Value); }
		}
	}
}

void ADCPlayerCharacter::Look(APlayerController* PC)
{
	float DX = 0.f, DY = 0.f;
	PC->GetInputMouseDelta(DX, DY);
	FRotator Rot = PC->GetControlRotation();
	Rot.Yaw += DX * LookSensitivity * 10.f;
	Rot.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rot.Pitch) + DY * LookSensitivity * 10.f, -85.f, 85.f);
	Rot.Roll = 0.f;
	PC->SetControlRotation(Rot);
}

void ADCPlayerCharacter::TickOnFoot(APlayerController* PC)
{
	Look(PC);

	const FRotator YawRot(0.f, PC->GetControlRotation().Yaw, 0.f);
	float F = 0.f, R = 0.f;
	if (PC->IsInputKeyDown(EKeys::W)) { F += 1.f; }
	if (PC->IsInputKeyDown(EKeys::S)) { F -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::D)) { R += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A)) { R -= 1.f; }
	AddMovementInput(YawRot.Vector(), F);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), R);
	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) { Jump(); }

	// Troubles the captain can deal with by hand.
	Prompt.Reset();
	if (ADCTroubles* Troubles = ADCTroubles::Get(this))
	{
		const float Dt = GetWorld()->GetDeltaSeconds();
		FVector Fire;
		if (Troubles->NearestFire(GetActorLocation(), Fire) && FVector::Dist2D(Fire, GetActorLocation()) < 280.f)
		{
			Prompt = TEXT("Hold E   beat out the flames");
			HoldTime = PC->IsInputKeyDown(EKeys::E) ? HoldTime + Dt : 0.f;
			if (HoldTime > 1.8f) { HoldTime = 0.f; Troubles->ExtinguishNear(Fire, 350.f); }
			return;
		}
		if (Troubles->IsRiggingLoose())
		{
			if (UDCCrewSubsystem* CrewSub = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
			{
				const FDCStation* Rig = CrewSub->GetStation(CrewSub->FindStation(EDCStationType::Rigging, EDCSide::None));
				if (Rig && FVector::Dist2D(Rig->Location, GetActorLocation()) < InteractRange)
				{
					Prompt = FString::Printf(TEXT("Hold E   haul the sail in   %d%%"), FMath::RoundToInt(Troubles->GetRiggingProgress() * 100.f));
					if (PC->IsInputKeyDown(EKeys::E)) { Troubles->RepairRigging(Dt / 4.f); }
					return;
				}
			}
		}
	}
	HoldTime = 0.f;

	// Nearest station the captain can take personally.
	Prompt.Reset();
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	if (!Crew) { return; }
	int32 Best = INDEX_NONE;
	float BestDist = InteractRange;
	const TArray<FDCStation>& Stations = Crew->GetStations();
	for (int32 i = 0; i < Stations.Num(); ++i)
	{
		if (Stations[i].Type != EDCStationType::Helm && Stations[i].Type != EDCStationType::Cannons) { continue; }
		const FVector D = Stations[i].Location - GetActorLocation();
		if (FMath::Abs(D.Z) < 250.f && D.Size2D() < BestDist)
		{
			BestDist = D.Size2D(); Best = i;
		}
	}
	if (Best != INDEX_NONE)
	{
		const FDCStation& S = Stations[Best];
		Prompt = S.Type == EDCStationType::Helm ? TEXT("E   take the helm")
			: (S.Side == EDCSide::Port ? TEXT("E   man the port guns") : TEXT("E   man the starboard guns"));
		if (PC->WasInputKeyJustPressed(EKeys::E)) { EnterStation(PC, Best); }
	}
}

void ADCPlayerCharacter::EnterStation(APlayerController* PC, int32 Index)
{
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	FDCStation* S = Crew ? Crew->GetStation(Index) : nullptr;
	if (!S) { return; }

	// Whoever was here steps aside.
	if (ADCCrewMember* Old = S->Operator.Get())
	{
		Old->AssignStation(INDEX_NONE);
		Crew->Reply(Old->CrewName, TEXT("She's yours, Captain."));
	}
	S->bPlayerOperated = true;
	ActiveStation = Index;
	ActiveType = S->Type;
	ActiveSide = S->Side;
	GetCharacterMovement()->StopMovementImmediately();

	if (ActiveType == EDCStationType::Cannons)
	{
		const float Sign = ActiveSide == EDCSide::Port ? -1.f : 1.f;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (!GunCamera)
		{
			GunCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			GunCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			GunCamera->GetCameraComponent()->SetFieldOfView(80.f);
		}
		// Just outboard of the shrouds and above the rail, so no rigging hides the field of fire.
		GunCamera->SetActorLocationAndRotation(FVector(S->Location.X - 100.f, Sign * 720.f, S->Location.Z + 620.f), FRotator(-20.f, Sign * 90.f, 0.f));
		PC->SetViewTargetWithBlend(GunCamera, 0.35f);

		if (!AimMarker)
		{
			AimMarker = GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			UStaticMeshComponent* Comp = AimMarker->GetStaticMeshComponent();
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetCastShadow(false);
			AimMarker->SetActorScale3D(FVector(5.f, 5.f, 0.03f));
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
			{
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, AimMarker);
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.25f, 0.05f));
				Comp->SetMaterial(0, MID);
			}
		}
		AimMarker->SetActorHiddenInGame(false);

		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	else if (ActiveType == EDCStationType::Helm)
	{
		SetActorLocation(S->Location + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator(-5.f, 0.f, 0.f));
	}
}

void ADCPlayerCharacter::LeaveStation(APlayerController* PC)
{
	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		if (FDCStation* S = Crew->GetStation(ActiveStation)) { S->bPlayerOperated = false; }
	}
	if (ActiveType == EDCStationType::Helm)
	{
		if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this)) { Treadmill->SetSteer(0.f); }
	}
	if (ActiveType == EDCStationType::Cannons)
	{
		PC->SetViewTargetWithBlend(this, 0.3f);
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
		if (AimMarker) { AimMarker->SetActorHiddenInGame(true); }
	}
	ActiveStation = INDEX_NONE;
	ActiveType = EDCStationType::None;
	ActiveSide = EDCSide::None;
}

void ADCPlayerCharacter::TickHelm(APlayerController* PC)
{
	Look(PC);
	float Steer = 0.f;
	if (PC->IsInputKeyDown(EKeys::D)) { Steer += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A)) { Steer -= 1.f; }
	if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this)) { Treadmill->SetSteer(Steer); }
	Prompt = TEXT("A / D   steer        E   step away");
	if (PC->WasInputKeyJustPressed(EKeys::E)) { LeaveStation(PC); }
}

void ADCPlayerCharacter::TickCannons(APlayerController* PC)
{
	const float Sign = ActiveSide == EDCSide::Port ? -1.f : 1.f;
	ADCRideDirector* Director = ADCRideDirector::Get(this);
	const float SeaZ = Director ? Director->SeaLevelZ : 0.f;

	// Cursor ray onto the sea, kept on this side of the ship and inside gun range.
	FVector Origin, Dir;
	FVector Aim(0.f, Sign * 5000.f, SeaZ);
	if (PC->DeprojectMousePositionToWorld(Origin, Dir) && Dir.Z < -0.01f)
	{
		Aim = Origin + Dir * ((SeaZ - Origin.Z) / Dir.Z);
	}
	Aim.Y = Sign * FMath::Clamp(Sign * Aim.Y, 1200.f, 14000.f);
	Aim.X = FMath::Clamp(Aim.X, -9000.f, 12000.f);
	Aim.Z = SeaZ + 10.f;
	if (AimMarker) { AimMarker->SetActorLocation(Aim); }

	ADCGunnery* Guns = ADCGunnery::Get(this);
	const bool bReady = Guns && Guns->IsReady(ActiveSide);
	if (bReady && (PC->IsInputKeyDown(EKeys::LeftMouseButton) || FParse::Param(FCommandLine::Get(), TEXT("DCAutoFire"))))
	{
		Guns->FireBroadside(ActiveSide, Aim, 6);
	}
	Prompt = bReady ? TEXT("Mouse   aim        Click   fire        E   step away") : TEXT("Reloading...        E   step away");
	if (PC->WasInputKeyJustPressed(EKeys::E)) { LeaveStation(PC); }
}

void ADCPlayerCharacter::DebugTakeStation(EDCStationType Type, EDCSide Side)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	if (!PC || !Crew) { return; }
	if (ActiveStation != INDEX_NONE) { LeaveStation(PC); }
	const int32 Index = Crew->FindStation(Type, Side);
	if (Index != INDEX_NONE) { EnterStation(PC, Index); }
}

void ADCPlayerCharacter::OpenOrderBox(APlayerController* PC)
{
	if (bTyping || !GEngine || !GEngine->GameViewport) { return; }
	bTyping = true;

	TWeakObjectPtr<ADCPlayerCharacter> Weak(this);
	SAssignNew(OrderBox, SEditableTextBox)
		.HintText(FText::FromString(TEXT("Type an order for the crew, then press Enter")))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
		.OnTextCommitted_Lambda([Weak](const FText& Text, ETextCommit::Type Type)
		{
			ADCPlayerCharacter* Self = Weak.Get();
			if (!Self) { return; }
			if (Type == ETextCommit::OnEnter && !Text.IsEmpty())
			{
				if (UDCCrewSubsystem* Crew = Self->GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->ExecuteOrder(Text.ToString()); }
			}
			Self->CloseOrderBox();
		});

	OrderRoot = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 150.f))
		[
			SNew(SBox).WidthOverride(720.f)[ OrderBox.ToSharedRef() ]
		];
	GEngine->GameViewport->AddViewportWidgetContent(OrderRoot.ToSharedRef(), 50);

	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(OrderBox);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
	FSlateApplication::Get().SetKeyboardFocus(OrderBox, EFocusCause::SetDirectly);
}

void ADCPlayerCharacter::CloseOrderBox()
{
	if (!bTyping) { return; }
	bTyping = false;
	if (GEngine && GEngine->GameViewport && OrderRoot.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(OrderRoot.ToSharedRef());
	}
	OrderRoot.Reset();
	OrderBox.Reset();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// The gun station keeps its cursor; everywhere else goes back to mouse-look.
		if (ActiveType != EDCStationType::Cannons)
		{
			PC->SetInputMode(FInputModeGameOnly());
		}
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void ADCPlayerCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	CloseOrderBox();
	Super::EndPlay(Reason);
}

void ADCPlayerCharacter::TickMeta(APlayerController* PC)
{
	// Restart or quit from the score card (and quit at any time).
	ADCRideDirector* Director = ADCRideDirector::Get(this);
	if (Director && Director->GetPhase() == EDCPhase::Ending && PC->WasInputKeyJustPressed(EKeys::R))
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
	}
	if (PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}
