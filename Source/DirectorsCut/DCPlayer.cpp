#include "DCPlayer.h"
#include "DCCrew.h"
#include "DCTreadmill.h"
#include "DCGunnery.h"
#include "DCRideDirector.h"
#include "DCTroubles.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
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
	GetCharacterMovement()->MaxWalkSpeed = 420.f;
	GetCharacterMovement()->MaxAcceleration = 2400.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2200.f;
	GetCharacterMovement()->AirControl = 0.35f;
}

void ADCPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) { return; }

	if (bTyping) { return; }
	if (PC->WasInputKeyJustPressed(EKeys::Enter)) { OpenOrderBox(PC); return; }
	TickMeta(PC);
	TickCameraFeel(DeltaSeconds);
	TickGraphicsKeys(PC);

	switch (ActiveType)
	{
	case EDCStationType::Helm: TickHelm(PC, DeltaSeconds); break;
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
	GetCharacterMovement()->MaxWalkSpeed = PC->IsInputKeyDown(EKeys::LeftShift) ? 610.f : 420.f;
	const FVector Move = YawRot.Vector() * F + FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y) * R;
	AddMovementInput(Move.GetClampedToMaxSize(1.f));
	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) { Jump(); }
	if (PC->WasInputKeyJustReleased(EKeys::SpaceBar)) { StopJumping(); }

	NearbyStation = INDEX_NONE;
	// Troubles the captain can deal with by hand.
	Prompt.Reset();
	if (ADCTroubles* Troubles = ADCTroubles::Get(this))
	{
		const float Dt = GetWorld()->GetDeltaSeconds();
		FVector Fire;
		if (Troubles->NearestFire(GetActorLocation(), Fire) && FVector::Dist(Fire, GetActorLocation()) < 280.f)
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
				if (Rig && FVector::Dist(Rig->Location, GetActorLocation()) < InteractRange)
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
	ADCGunnery* Guns = ADCGunnery::Get(this);
	float BestDist = InteractRange;
	const TArray<FDCStation>& Stations = Crew->GetStations();
	for (int32 i = 0; i < Stations.Num(); ++i)
	{
		if (Stations[i].Type != EDCStationType::Helm && Stations[i].Type != EDCStationType::Cannons) { continue; }
		if (Stations[i].Type == EDCStationType::Cannons && (!Guns || Guns->NumGuns(Stations[i].Side) == 0)) { continue; }
		const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const FVector D = Stations[i].Location - Feet;
		if (FMath::Abs(D.Z) < 115.f && D.Size() < BestDist)
		{
			BestDist = D.Size();
			NearbyStation = i;
		}
	}
	if (NearbyStation != INDEX_NONE)
	{
		const FDCStation& S = Stations[NearbyStation];
		Prompt = S.Type == EDCStationType::Helm ? TEXT("E   take the helm")
			: (S.Side == EDCSide::Port ? TEXT("E   man the port guns") : TEXT("E   man the starboard guns"));
		if (PC->WasInputKeyJustPressed(EKeys::E)) { EnterStation(PC, NearbyStation); }
	}
}

void ADCPlayerCharacter::EnterStation(APlayerController* PC, int32 Index)
{
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	FDCStation* S = Crew ? Crew->GetStation(Index) : nullptr;
	if (!S) { return; }
	if (S->Type == EDCStationType::Cannons)
	{
		const ADCGunnery* Guns = ADCGunnery::Get(this);
		if (!Guns || Guns->NumGuns(S->Side) == 0) { return; }
	}

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
	ConsumeMovementInputVector();
	StopJumping();

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
		// Look over the barrel from the deck, keeping the rail in view for orientation.
		GunCamera->SetActorLocationAndRotation(S->Location + FVector(0.f, -Sign * 80.f, 250.f), FRotator(-11.f, Sign * 90.f, 0.f));
		PC->SetViewTargetWithBlend(GunCamera, 0.35f);

		bGunSolution = false;
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	else if (ActiveType == EDCStationType::Helm)
	{
		SetActorLocation(S->Location + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator(-5.f, 0.f, 0.f));
		const ADCGunnery* Guns = ADCGunnery::Get(this);
		if (AActor* Ship = Guns ? Guns->GetGunShip() : nullptr)
		{
			TArray<USceneComponent*> Parts;
			Ship->GetComponents(Parts);
			for (USceneComponent* Part : Parts)
			{
				if (Part->GetFName() != FName(TEXT("SK_Wheel"))) { continue; }
				HelmWheel = Part;
				WheelRestRotation = Part->GetRelativeRotation().Quaternion();
				const USceneComponent* Parent = Part->GetAttachParent();
				WheelAxisInParent = Parent
					? Parent->GetComponentTransform().InverseTransformVectorNoScale(Ship->GetActorForwardVector())
					: Ship->GetActorForwardVector();
				break;
			}
		}
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
		RestoreWheel();
		if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this)) { Treadmill->SetSteer(0.f); }
	}
	if (ActiveType == EDCStationType::Cannons)
	{
		PC->SetViewTargetWithBlend(this, 0.3f);
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
		bGunSolution = false;
	}
	bClearHelmView = false;
	Prompt.Reset();
	ActiveStation = INDEX_NONE;
	ActiveType = EDCStationType::None;
	ActiveSide = EDCSide::None;
}

void ADCPlayerCharacter::RestoreWheel()
{
	if (USceneComponent* Wheel = HelmWheel.Get()) { Wheel->SetRelativeRotation(WheelRestRotation); }
	HelmWheel.Reset();
	WheelAngle = 0.f;
}

void ADCPlayerCharacter::TickHelm(APlayerController* PC, float DeltaSeconds)
{
	Look(PC);
	float Steer = 0.f;
	if (PC->IsInputKeyDown(EKeys::D)) { Steer += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A)) { Steer -= 1.f; }
	if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this)) { Treadmill->SetSteer(Steer); }
	if (USceneComponent* Wheel = HelmWheel.Get())
	{
		WheelAngle = FMath::FInterpTo(WheelAngle, Steer * 65.f, DeltaSeconds, 5.f);
		Wheel->SetRelativeRotation(FQuat(WheelAxisInParent, FMath::DegreesToRadians(WheelAngle)) * WheelRestRotation);
	}
	if (PC->WasInputKeyJustPressed(EKeys::C)) { bClearHelmView = !bClearHelmView; }
	Prompt = bClearHelmView ? TEXT("A/D  steer     C  wheel view     E  leave helm")
		: TEXT("A/D  steer     C  clear view     E  leave helm");
	if (PC->WasInputKeyJustPressed(EKeys::E)) { LeaveStation(PC); }
}

void ADCPlayerCharacter::TickCannons(APlayerController* PC)
{
	if (PC->WasInputKeyJustPressed(EKeys::E)) { LeaveStation(PC); return; }

	const float SideYaw = ActiveSide == EDCSide::Port ? -90.f : 90.f;
	float DX = 0.f, DY = 0.f;
	PC->GetInputMouseDelta(DX, DY);
	FRotator AimRotation = GunCamera->GetActorRotation();
	AimRotation.Yaw = SideYaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(SideYaw, AimRotation.Yaw) + DX * LookSensitivity * 10.f, -62.f, 62.f);
	AimRotation.Pitch = FMath::Clamp(AimRotation.Pitch + DY * LookSensitivity * 10.f, -62.f, 12.f);
	GunCamera->SetActorRotation(AimRotation);

	ADCGunnery* Guns = ADCGunnery::Get(this);
	FVector Target;
	bGunSolution = Guns && Guns->ResolveAim(ActiveSide, GunCamera->GetActorLocation(), AimRotation.Vector(), Target);
	const bool bAutoFire = FParse::Param(FCommandLine::Get(), TEXT("DCAutoFire"));
	if (bGunSolution && Guns->IsReady(ActiveSide) && (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) || bAutoFire))
	{
		Guns->FireBroadside(ActiveSide, Target, 6, true);
	}
	Prompt = TEXT("Mouse  aim     LMB  fire broadside     E  leave guns");
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
		PC->SetInputMode(FInputModeGameOnly());
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void ADCPlayerCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	CloseOrderBox();
	RestoreWheel();
	if (GunCamera) { GunCamera->Destroy(); }
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

void ADCPlayerCharacter::TickCameraFeel(float DeltaSeconds)
{
	// Shake from blows and thunder, a steady rumble and a wider lens while the sea drops away.
	ADCRideDirector* Director = ADCRideDirector::Get(this);
	if (!Director || !Camera) { return; }
	const FDCEnvState& Env = Director->GetEnv();
	const float Rush = FMath::Clamp(Env.Drop / 260.f, 0.f, 1.f);
	const float Shake = FMath::Square(Director->Trauma) + Rush * 0.12f + Env.Tube * 0.03f;
	const float T = GetWorld()->GetTimeSeconds();
	const FRotator Jitter(
		Shake * 2.2f * FMath::PerlinNoise1D(T * 17.f),
		Shake * 1.6f * FMath::PerlinNoise1D(T * 15.f + 40.f),
		Shake * 3.5f * FMath::PerlinNoise1D(T * 13.f + 80.f));
	Camera->SetRelativeRotation(Jitter + Director->DeckSway);
	const FVector Eye = bClearHelmView
		? GetActorTransform().InverseTransformPosition(GetActorLocation() + FVector(0.f, 320.f, 125.f))
		: FVector(0.f, 0.f, 64.f);
	const FVector Bob(0.f, 0.f, IsAtStation() ? 0.f : FMath::Sin(T * 10.f) * FMath::Min(1.f, GetVelocity().Size2D() / 420.f) * 2.f);
	const FVector Offset(0.f, Shake * 6.f * FMath::PerlinNoise1D(T * 19.f + 7.f), Shake * 8.f * FMath::PerlinNoise1D(T * 21.f + 3.f));
	Camera->SetRelativeLocation(FMath::VInterpTo(Camera->GetRelativeLocation(), Eye + Bob + Offset, DeltaSeconds, 12.f));
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, 90.f + 16.f * Rush, DeltaSeconds, 2.f));
}

void ADCPlayerCharacter::TickGraphicsKeys(APlayerController* PC)
{
	// F5 cycles the DLSS quality mode, F6 toggles DLSS frame generation. The ship's log confirms.
	auto SetCVar = [](const TCHAR* Name, const FString& Value)
	{
		if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(Name)) { C->Set(*Value, ECVF_SetByGameOverride); return true; }
		return false;
	};
	UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	if (PC->WasInputKeyJustPressed(EKeys::F5))
	{
		static const TPair<const TCHAR*, float> Modes[] = { { TEXT("Quality"), 67.f }, { TEXT("Balanced"), 58.f }, { TEXT("Performance"), 50.f }, { TEXT("DLAA (native)"), 100.f } };
		DLSSModeIndex = (DLSSModeIndex + 1) % UE_ARRAY_COUNT(Modes);
		const bool bOk = SetCVar(TEXT("r.NGX.DLSS.Enable"), TEXT("1")) && SetCVar(TEXT("r.ScreenPercentage"), FString::SanitizeFloat(Modes[DLSSModeIndex].Value));
		if (Crew) { Crew->Say(TEXT("Ship's log"), bOk ? FString::Printf(TEXT("DLSS Super Resolution: %s"), Modes[DLSSModeIndex].Key) : FString(TEXT("DLSS is not available on this machine"))); }
	}
	if (PC->WasInputKeyJustPressed(EKeys::F6))
	{
		bFrameGen = !bFrameGen;
		const bool bOk = SetCVar(TEXT("r.Streamline.DLSSG.Enable"), bFrameGen ? TEXT("1") : TEXT("0"));
		if (Crew) { Crew->Say(TEXT("Ship's log"), bOk ? FString::Printf(TEXT("DLSS Frame Generation: %s"), bFrameGen ? TEXT("on") : TEXT("off")) : FString(TEXT("Frame generation is not available on this machine"))); }
	}
}
