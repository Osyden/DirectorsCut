#include "DCGame.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "DCCrew.h"
#include "DCVoice.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/WindDirectionalSource.h"
#include "Camera/CameraActor.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"

// ---------------------------------------------------------------- Player

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

	float DX = 0.f, DY = 0.f;
	PC->GetInputMouseDelta(DX, DY);
	FRotator Rot = PC->GetControlRotation();
	Rot.Yaw += DX * LookSensitivity * 10.f;
	Rot.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rot.Pitch) + DY * LookSensitivity * 10.f, -85.f, 85.f);
	Rot.Roll = 0.f;
	PC->SetControlRotation(Rot);

	const FRotator YawRot(0.f, Rot.Yaw, 0.f);
	const FVector Forward = YawRot.Vector();
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	float F = 0.f, R = 0.f;
	if (PC->IsInputKeyDown(EKeys::W)) { F += 1.f; }
	if (PC->IsInputKeyDown(EKeys::S)) { F -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::D)) { R += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A)) { R -= 1.f; }
	AddMovementInput(Forward, F);
	AddMovementInput(Right, R);
	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) { Jump(); }

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

// ---------------------------------------------------------------- HUD

void ADCHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) { return; }

	ADCRideDirector* D = ADCRideDirector::Get(this);
	ADCTreadmill* T = ADCTreadmill::Get(this);
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;

	if (D)
	{
		const int32 Sec = FMath::FloorToInt(D->RideTime);
		DrawText(FString::Printf(TEXT("%s   %d:%02d   Hull %d%%   Score %d"), DCPhaseName(D->GetPhase()), Sec / 60, Sec % 60,
			FMath::RoundToInt(D->ShipHealth), D->Score), FLinearColor::White, 40.f, 30.f, Font, 1.4f);
	}
	if (T)
	{
		DrawText(FString::Printf(TEXT("Helm %+.1f   Lane %+.0f m   Hostiles %d"), T->GetSteer(), T->GetLaneOffset() / 100.f, T->NumHostiles()),
			FLinearColor(0.7f, 0.8f, 0.9f), 40.f, 60.f, Font, 1.1f);
	}
	if (ADCVoice* V = ADCVoice::Get(this))
	{
		DrawText(FString::Printf(TEXT("Voice: speech-in %s  crew-mind %s  speech-out %s   [hold V to talk, 1-6 quick orders]"),
			V->IsSTTReady() ? TEXT("on") : TEXT("off"), V->IsLLMReady() ? TEXT("on") : TEXT("off"), V->IsTTSReady() ? TEXT("on") : TEXT("off")),
			FLinearColor(0.6f, 0.7f, 0.6f), 40.f, 84.f, Font, 1.f);
		if (V->IsListening())
		{
			DrawText(TEXT("LISTENING..."), FLinearColor(1.f, 0.3f, 0.2f), Canvas->SizeX * 0.5f - 70.f, Canvas->SizeY - 110.f, Font, 1.8f);
		}
	}

	if (UDCCrewSubsystem* Sub = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		float Y = Canvas->SizeY - 60.f - Sub->RecentLines.Num() * 26.f;
		for (const FString& Line : Sub->RecentLines)
		{
			DrawText(Line, Line.StartsWith(TEXT("Captain")) ? FLinearColor(1.f, 0.85f, 0.4f) : FLinearColor::White, 60.f, Y, Font, 1.3f);
			Y += 26.f;
		}
	}
}

// ---------------------------------------------------------------- Game mode

ADCGameMode::ADCGameMode()
{
	DefaultPawnClass = ADCPlayerCharacter::StaticClass();
	HUDClass = ADCHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	ShipClass = FSoftClassPath(TEXT("/Game/Brigantine/BP/BP_Brigantine.BP_Brigantine_C"));
}

FVector ADCGameMode::DeckAt(float X, float Y, float TopZ) const
{
	// Trace down from just above the deck we want, so we land on it and not on rigging above or the hold below.
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("DCDeck"), true);
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(X, Y, TopZ), FVector(X, Y, TopZ - 500.f), ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}
	return FVector(X, Y, TopZ - 250.f);
}

FVector ADCGameMode::ShipPart(const TCHAR* Name, const FVector& Fallback) const
{
	if (Ship)
	{
		TArray<UPrimitiveComponent*> Prims;
		Ship->GetComponents(Prims);
		for (UPrimitiveComponent* P : Prims)
		{
			if (P->GetName() == Name) { return P->Bounds.Origin; }
		}
	}
	return Fallback;
}

void ADCGameMode::StartPlay()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// NVIDIA DLSS (plugin). Super Resolution renders below native and upscales; frame generation is optional.
	if (bEnableDLSS && !FParse::Param(FCommandLine::Get(), TEXT("DCNoDLSS")))
	{
		auto SetCVar = [](const TCHAR* Name, const FString& Value)
		{
			if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(Name)) { C->Set(*Value, ECVF_SetByGameOverride); }
			else { UE_LOG(LogDC, Warning, TEXT("DLSS: console variable %s not found (plugin missing?)"), Name); }
		};
		SetCVar(TEXT("r.NGX.DLSS.Enable"), TEXT("1"));
		SetCVar(TEXT("r.ScreenPercentage"), FString::SanitizeFloat(DLSSScreenPercentage));
		SetCVar(TEXT("r.Streamline.DLSSG.Enable"), bEnableFrameGeneration ? TEXT("1") : TEXT("0"));
		UE_LOG(LogDC, Log, TEXT("DLSS requested: super resolution at %.0f%%, frame generation %s"), DLSSScreenPercentage, bEnableFrameGeneration ? TEXT("on") : TEXT("off"));
	}
	// The ship blueprint looks up a wind source on BeginPlay and errors every frame without one.
	World->SpawnActor<AWindDirectionalSource>(FVector(0.f, 0.f, 3000.f), FRotator::ZeroRotator, Params);

	// Ship: adopt a tagged one from the map, otherwise spawn the configured class.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(ShipTag)) { Ship = *It; break; }
	}
	if (!Ship)
	{
		if (UClass* Cls = ShipClass.TryLoadClass<AActor>())
		{
			Ship = World->SpawnActor<AActor>(Cls, ShipSpawnLocation, ShipSpawnRotation, Params);
		}
		else
		{
			UE_LOG(LogDC, Warning, TEXT("Ship class %s not found; running without a ship."), *ShipClass.ToString());
		}
	}

	Director = World->SpawnActor<ADCRideDirector>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Director->SeaLevelZ = SeaLevelZ;
	Treadmill = World->SpawnActor<ADCTreadmill>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCNoVoice")))
	{
		World->SpawnActor<ADCVoice>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}
	FParse::Value(FCommandLine::Get(), TEXT("DCTestWav="), TestWavPath, false);

	FBox Bounds(FVector(-2000.f, -400.f, 0.f), FVector(2000.f, 400.f, 600.f));
	if (Ship)
	{
		Bounds = Ship->GetComponentsBoundingBox(true);
		TArray<UPrimitiveComponent*> ShipPrims;
		Ship->GetComponents(ShipPrims);
		for (UPrimitiveComponent* P : ShipPrims)
		{
			// The asset floats on its own water system; we have none, so it must not simulate.
			if (P->IsSimulatingPhysics()) { UE_LOG(LogDC, Log, TEXT("Disabling physics on %s"), *P->GetName()); P->SetSimulatePhysics(false); }
			if (P->GetName() == TEXT("HullShip")) { Bounds = P->Bounds.GetBox(); }
		}
		Ship->SetActorLocationAndRotation(ShipSpawnLocation, ShipSpawnRotation, false, nullptr, ETeleportType::ResetPhysics);
		UE_LOG(LogDC, Log, TEXT("Ship %s bounds min %s max %s"), *Ship->GetName(), *Bounds.Min.ToString(), *Bounds.Max.ToString());
		Treadmill->ShipHalfLength = Bounds.GetExtent().X;
		Treadmill->ShipHalfWidth = Bounds.GetExtent().Y;
		{
			TArray<UPrimitiveComponent*> Prims;
			Ship->GetComponents(Prims);
			int32 Hidden = 0;
			for (UPrimitiveComponent* P : Prims) { if (!P->IsVisible() || P->bHiddenInGame) { ++Hidden; } }
			UE_LOG(LogDC, Log, TEXT("Ship has %d primitives, %d hidden, root at %s scale %s, actor hidden %d"), Prims.Num(), Hidden,
				*Ship->GetActorLocation().ToString(), *Ship->GetActorScale3D().ToString(), Ship->IsHidden() ? 1 : 0);
			for (int32 i = 0; i < Prims.Num(); ++i)
			{
				const FBoxSphereBounds B = Prims[i]->Bounds;
				UE_LOG(LogDC, Verbose, TEXT("  %s (%s) vis %d origin %s ext %s"), *Prims[i]->GetName(), *Prims[i]->GetClass()->GetName(),
					Prims[i]->IsVisible() ? 1 : 0, *B.Origin.ToCompactString(), *B.BoxExtent.ToCompactString());
			}
		}
	}

	// Deck heights on this hull: quarterdeck ~Z 700 (wheel), main deck ~Z 500 (upper guns), gun deck ~Z 300.
	const FVector Wheel = ShipPart(TEXT("SK_Wheel"), FVector(-930.f, 0.f, 757.f));
	const FVector GunP = ShipPart(TEXT("SK_Cannon_2-L9"), FVector(-110.f, -317.f, 542.f));
	const FVector GunS = ShipPart(TEXT("SK_Cannon_2-R9"), FVector(-110.f, 317.f, 542.f));
	const float MainTop = GunS.Z + 180.f;

	UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>();
	Crew->AddStation(EDCStationType::Helm, EDCSide::None, DeckAt(Wheel.X - 110.f, Wheel.Y, Wheel.Z + 120.f));
	Crew->AddStation(EDCStationType::Cannons, EDCSide::Port, DeckAt(GunP.X, GunP.Y + 150.f, MainTop));
	Crew->AddStation(EDCStationType::Cannons, EDCSide::Starboard, DeckAt(GunS.X, GunS.Y - 150.f, MainTop));
	Crew->AddStation(EDCStationType::Fire, EDCSide::None, DeckAt(GunS.X - 200.f, 60.f, MainTop));
	Crew->AddStation(EDCStationType::Rigging, EDCSide::None, DeckAt(GunS.X + 420.f, 120.f, MainTop));

	Crew->SpawnCrew(TEXT("Finch"), FLinearColor(0.9f, 0.7f, 0.2f), DeckAt(Wheel.X - 60.f, -180.f, Wheel.Z + 120.f));
	Crew->SpawnCrew(TEXT("Marlow"), FLinearColor(0.3f, 0.7f, 0.9f), DeckAt(GunS.X + 150.f, 80.f, MainTop));
	Crew->SpawnCrew(TEXT("Odessa"), FLinearColor(0.8f, 0.3f, 0.5f), DeckAt(GunS.X - 350.f, -90.f, MainTop));

	Super::StartPlay();

	// Put the captain on the quarterdeck, looking forward.
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			const FVector Deck = DeckAt(Wheel.X - 260.f, 90.f, Wheel.Z + 120.f);
			Pawn->SetActorLocation(Deck + FVector(0.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator::ZeroRotator);
			UE_LOG(LogDC, Log, TEXT("Captain placed at %s"), *Pawn->GetActorLocation().ToString());
		}
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}

	Crew->ExecuteOrder(TEXT("Finch, take the helm"));

	FString Shots;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCAutoShots="), Shots, false))
	{
		TArray<FString> Parts;
		Shots.ParseIntoArray(Parts, TEXT(","));
		for (const FString& P : Parts) { AutoShotTimes.Add(FCString::Atof(*P)); }
		AutoShotWait = 6.f;
		if (AutoShotTimes.Num()) { Director->Seek(AutoShotTimes[0]); }

		FString Cam;
		if (FParse::Value(FCommandLine::Get(), TEXT("DCShotCam="), Cam, false))
		{
			TArray<FString> C;
			Cam.ParseIntoArray(C, TEXT(","));
			if (C.Num() >= 5)
			{
				ACameraActor* CamActor = World->SpawnActor<ACameraActor>(
					FVector(FCString::Atof(*C[0]), FCString::Atof(*C[1]), FCString::Atof(*C[2])),
					FRotator(FCString::Atof(*C[3]), FCString::Atof(*C[4]), 0.f), Params);
				CamActor->GetCameraComponent()->bConstrainAspectRatio = false;
				if (APlayerController* PC = World->GetFirstPlayerController()) { PC->SetViewTarget(CamActor); }
			}
		}
	}
}

void ADCGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!TestWavPath.IsEmpty())
	{
		// Unattended voice test: once speech-to-text is up, feed it a recorded order.
		ADCVoice* Voice = ADCVoice::Get(this);
		if (Voice && Voice->IsSTTReady() && GetWorld()->GetTimeSeconds() > 3.f)
		{
			Voice->TranscribeWavFile(TestWavPath);
			TestWavPath.Empty();
		}
	}
	if (Ship && !Ship->GetActorLocation().Equals(ShipSpawnLocation, 1.f))
	{
		UE_LOG(LogDC, Verbose, TEXT("Ship drifted to %s; pinning"), *Ship->GetActorLocation().ToString());
		Ship->SetActorLocationAndRotation(ShipSpawnLocation, ShipSpawnRotation, false, nullptr, ETeleportType::ResetPhysics);
	}
	if (!AutoShotTimes.IsValidIndex(AutoShotIndex))
	{
		return;
	}

	AutoShotWait -= DeltaSeconds;
	if (AutoShotWait > 0.f) { return; }

	if (!bAutoShotTaken)
	{
		const FString File = FPaths::ProjectSavedDir() / TEXT("Screenshots/Auto") / FString::Printf(TEXT("shot_%04d.png"), FMath::RoundToInt(AutoShotTimes[AutoShotIndex]));
		FScreenshotRequest::RequestScreenshot(File, false, false);
		UE_LOG(LogDC, Log, TEXT("Auto shot %s at ride time %.1f, ship at %s"), *FPaths::GetCleanFilename(File), Director->RideTime, Ship ? *Ship->GetActorLocation().ToString() : TEXT("none"));
		bAutoShotTaken = true;
		AutoShotWait = 1.f;
		return;
	}

	bAutoShotTaken = false;
	++AutoShotIndex;
	if (AutoShotTimes.IsValidIndex(AutoShotIndex))
	{
		Director->Seek(AutoShotTimes[AutoShotIndex]);
		AutoShotWait = 5.f;
	}
	else
	{
		FPlatformMisc::RequestExit(false);
	}
}
