#include "DCGame.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "DCCrew.h"
#include "DCVoice.h"
#include "DCPlayer.h"
#include "DCGunnery.h"
#include "DCKraken.h"
#include "DCKrakenHead.h"
#include "DCAudio.h"
#include "DCWeatherFX.h"
#include "DCTroubles.h"
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
#include "Misc/App.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------- HUD

void ADCHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) { return; }

	ADCRideDirector* D = ADCRideDirector::Get(this);
	ADCTreadmill* T = ADCTreadmill::Get(this);
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* Big = GEngine ? GEngine->GetLargeFont() : Font;
	const float W = Canvas->SizeX, H = Canvas->SizeY;
	const float S = FMath::Max(1.f, H / 720.f); // everything scales with resolution

	// Text with a drop shadow so it reads over fog, fire and sea alike.
	auto Label = [this, S](const FString& Str, float X, float Y, float Scale, const FLinearColor& Color, UFont* F)
	{
		DrawText(Str, FLinearColor(0.f, 0.f, 0.f, 0.85f * Color.A), X + 2.f * S, Y + 2.f * S, F, Scale * S);
		DrawText(Str, Color, X, Y, F, Scale * S);
	};
	auto Centered = [this, W, S, &Label](const FString& Str, float Y, float Scale, const FLinearColor& Color, UFont* F)
	{
		float TW = 0.f, TH = 0.f;
		GetTextSize(Str, TW, TH, F, Scale * S);
		Label(Str, (W - TW) * 0.5f, Y, Scale, Color, F);
	};
	const FLinearColor Gold(1.f, 0.85f, 0.5f), White(1.f, 1.f, 1.f), Dim(0.75f, 0.8f, 0.85f);

	// Top left: hull bar and where we are. Top right: score.
	if (D)
	{
		const float X = 40.f * S, Y = 32.f * S, BW = 260.f * S, BH = 12.f * S;
		const float Frac = FMath::Clamp(D->ShipHealth / 100.f, 0.f, 1.f);
		const FLinearColor Bar = Frac > 0.5f ? FMath::Lerp(FLinearColor(0.95f, 0.75f, 0.2f), FLinearColor(0.35f, 0.85f, 0.45f), (Frac - 0.5f) * 2.f)
			: FMath::Lerp(FLinearColor(0.9f, 0.15f, 0.1f), FLinearColor(0.95f, 0.75f, 0.2f), Frac * 2.f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X - 3.f, Y - 3.f, BW + 6.f, BH + 6.f);
		DrawRect(Bar, X, Y, BW * Frac, BH);
		Label(FString::Printf(TEXT("HULL  %d%%"), FMath::RoundToInt(D->ShipHealth)), X, Y + BH + 6.f * S, 1.1f, White, Font);
		Label(FString(DCPhaseName(D->GetPhase())).ToUpper(), X + BW + 26.f * S, Y - 8.f * S, 1.5f, Gold, Font);

		const FString Score = FString::Printf(TEXT("SCORE  %d"), D->Score);
		float TW = 0.f, TH = 0.f;
		GetTextSize(Score, TW, TH, Font, 1.4f * S);
		Label(Score, W - TW - 40.f * S, Y - 6.f * S, 1.4f, White, Font);
	}

	// Alerts.
	float AlertY = 84.f * S;
	if (ADCTroubles* Troubles = ADCTroubles::Get(this))
	{
		if (Troubles->NumFires() > 0)
		{
			Label(FString::Printf(TEXT("FIRE ON DECK  x%d"), Troubles->NumFires()), 40.f * S, AlertY, 1.5f, FLinearColor(1.f, 0.45f, 0.1f), Font);
			AlertY += 30.f * S;
		}
		if (Troubles->IsRiggingLoose())
		{
			Label(TEXT("RIGGING LOOSE   helm sluggish"), 40.f * S, AlertY, 1.5f, FLinearColor(1.f, 0.85f, 0.3f), Font);
			AlertY += 30.f * S;
		}
	}

	// Developer line, only with -DCDebug.
	static const bool bDebug = FParse::Param(FCommandLine::Get(), TEXT("DCDebug"));
	if (bDebug && D && T)
	{
		const int32 Sec = FMath::FloorToInt(D->RideTime);
		Label(FString::Printf(TEXT("%d:%02d   helm %+.1f   lane %+.0f m   hostiles %d"), Sec / 60, Sec % 60, T->GetSteer(), T->GetLaneOffset() / 100.f, T->NumHostiles()),
			40.f * S, AlertY, 1.f, Dim, Font);
	}

	// Bottom right: how to give orders, and which parts of the voice loop are up.
	{
		FString Help = TEXT("V  talk     Enter  type     1-6  quick orders     E  take a station     F5  DLSS mode     F6  frame generation");
		if (ADCVoice* V = ADCVoice::Get(this))
		{
			if (!V->IsSTTReady()) { Help = TEXT("Enter  type     1-6  quick orders     E  take a station     (voice starting...)"); }
			if (V->IsListening())
			{
				Centered(TEXT("LISTENING..."), H - 150.f * S, 2.f, FLinearColor(1.f, 0.35f, 0.25f), Font);
			}
		}
		float TW = 0.f, TH = 0.f;
		GetTextSize(Help, TW, TH, Font, 1.f * S);
		Label(Help, W - TW - 40.f * S, H - 34.f * S, 1.f, Dim, Font);
	}

	// Subtitles, bottom left, on a dark plate.
	if (UDCCrewSubsystem* Sub = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		const int32 Num = FMath::Min(4, Sub->RecentLines.Num());
		const float LineH = 28.f * S;
		float Y = H - 70.f * S - Num * LineH;
		for (int32 i = Sub->RecentLines.Num() - Num; i < Sub->RecentLines.Num(); ++i)
		{
			const FString& Line = Sub->RecentLines[i];
			float TW = 0.f, TH = 0.f;
			GetTextSize(Line, TW, TH, Font, 1.3f * S);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), 34.f * S, Y - 2.f * S, TW + 16.f * S, LineH - 2.f * S);
			// The newest line is brightest.
			const float A = i == Sub->RecentLines.Num() - 1 ? 1.f : 0.7f;
			Label(Line, 42.f * S, Y, 1.3f, Line.StartsWith(TEXT("Captain")) ? FLinearColor(1.f, 0.85f, 0.4f, A) : FLinearColor(1.f, 1.f, 1.f, A), Font);
			Y += LineH;
		}
	}

	// What E does right now.
	if (ADCPlayerCharacter* Captain = Cast<ADCPlayerCharacter>(GetOwningPawn()))
	{
		if (!Captain->GetPrompt().IsEmpty())
		{
			Centered(Captain->GetPrompt(), H * 0.74f, 1.5f, FLinearColor(1.f, 0.95f, 0.8f), Font);
		}
	}

	// Title card at the start, score card at the end.
	if (D)
	{
		if (D->RideTime < 16.f)
		{
			const float A = FMath::Clamp((16.f - D->RideTime) / 3.f, 0.f, 1.f);
			const FLinearColor G(Gold.R, Gold.G, Gold.B, A), Wh(1.f, 1.f, 1.f, A);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f * A), W * 0.12f, H * 0.17f, W * 0.76f, 300.f * S);
			Centered(TEXT("DIRECTOR'S CUT"), H * 0.17f + 22.f * S, 4.f, G, Big);
			Centered(TEXT("You are the captain. You cannot do it all yourself. Give the orders."), H * 0.17f + 118.f * S, 1.6f, Wh, Font);
			Centered(TEXT("Hold V and speak   or press Enter and type   or use keys 1 to 6"), H * 0.17f + 162.f * S, 1.4f, Wh, Font);
			Centered(TEXT("\"Finch, hard to starboard\"    \"Port battery, fire\"    \"Odessa, put out the fire\"    \"Repair the rigging\""), H * 0.17f + 202.f * S, 1.2f, Wh, Font);
			Centered(TEXT("WASD  walk the deck        E  take the helm or a gun battery yourself"), H * 0.17f + 240.f * S, 1.2f, Wh, Font);
		}
		if (D->GetPhase() == EDCPhase::Ending)
		{
			const float A = FMath::Clamp((D->RideTime - D->GetRideLength() + 14.f) / 3.f, 0.f, 1.f);
			const FLinearColor G(Gold.R, Gold.G, Gold.B, A), Wh(1.f, 1.f, 1.f, A);
			int32 Orders = 0;
			if (UDCCrewSubsystem* Sub = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Orders = Sub->OrdersGiven; }
			const int32 Hull = FMath::RoundToInt(D->ShipHealth);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f * A), W * 0.2f, H * 0.2f, W * 0.6f, 220.f * S);
			Centered(Hull > 60 ? TEXT("SHE CAME THROUGH") : (Hull > 20 ? TEXT("BATTERED, BUT AFLOAT") : TEXT("BARELY AFLOAT")), H * 0.2f + 22.f * S, 3.5f, G, Big);
			Centered(FString::Printf(TEXT("Hull %d%%        Score %d        Orders given %d"), Hull, D->Score, Orders), H * 0.2f + 112.f * S, 1.8f, Wh, Font);
			Centered(TEXT("R  sail again          Esc  quit"), H * 0.2f + 160.f * S, 1.4f, Wh, Font);
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
	World->SpawnActor<ADCAudio>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ADCWeatherFX>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
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
		ADCGunnery* Guns = World->SpawnActor<ADCGunnery>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		Guns->Init(Ship, SeaLevelZ);
		World->SpawnActor<ADCKraken>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		World->SpawnActor<ADCKrakenHead>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
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

	// Light the ship's own lanterns.
	Director->AddLamp(ShipPart(TEXT("SM_Lamp"), FVector(-2067.f, 0.f, 908.f)));
	{
		const FVector Mid = ShipPart(TEXT("SM_Lamp-brigantine1"), FVector(-618.f, 0.f, 826.f));
		Director->AddLamp(Mid + FVector(0.f, -75.f, 0.f));
		Director->AddLamp(Mid + FVector(0.f, 75.f, 0.f));
		Director->AddLamp(ShipPart(TEXT("SM_Lamp-brigantine2"), FVector(607.f, 0.f, 731.f)));
	}

	// Troubles: where fires may start (main deck and quarterdeck, clear of the stations).
	ADCTroubles* Troubles = World->SpawnActor<ADCTroubles>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	for (const FVector2D& P : { FVector2D(GunS.X + 250.f, -120.f), FVector2D(GunS.X - 80.f, 140.f), FVector2D(GunS.X + 520.f, 60.f), FVector2D(GunS.X - 330.f, -150.f) })
	{
		Troubles->DeckPoints.Add(DeckAt(P.X, P.Y, MainTop));
	}
	Troubles->DeckPoints.Add(DeckAt(Wheel.X - 330.f, -170.f, Wheel.Z + 120.f));

	Crew->SpawnCrew(TEXT("Finch"), FLinearColor(0.9f, 0.7f, 0.2f), DeckAt(Wheel.X - 60.f, -180.f, Wheel.Z + 120.f));
	Crew->SpawnCrew(TEXT("Marlow"), FLinearColor(0.3f, 0.7f, 0.9f), DeckAt(GunS.X + 150.f, 80.f, MainTop));
	Crew->SpawnCrew(TEXT("Odessa"), FLinearColor(0.8f, 0.3f, 0.5f), DeckAt(GunS.X - 350.f, -90.f, MainTop));

	Super::StartPlay();

	// Put the captain on the quarterdeck, looking forward.
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			const FVector Deck = DeckAt(Wheel.X - 170.f, 190.f, Wheel.Z + 120.f);
			Pawn->SetActorLocation(Deck + FVector(0.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator::ZeroRotator);
			UE_LOG(LogDC, Log, TEXT("Captain placed at %s"), *Pawn->GetActorLocation().ToString());
		}
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}

	Crew->ExecuteOrder(TEXT("Finch, take the helm"));

	// Unattended test hooks: -DCOrders=hard_to_port;port_battery_fire and -DCStation=helm|port|starboard.
	FString OrderList;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCOrders="), OrderList, false))
	{
		TArray<FString> Orders;
		OrderList.ParseIntoArray(Orders, TEXT(";"));
		for (const FString& O : Orders) { Crew->ExecuteOrder(O.Replace(TEXT("_"), TEXT(" "))); }
	}
	FString Late;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCOrdersLate="), Late, false)) { Late.ParseIntoArray(LateOrders, TEXT(";")); }
	FString StationName;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCStation="), StationName))
	{
		if (ADCPlayerCharacter* Captain = Cast<ADCPlayerCharacter>(World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr))
		{
			if (StationName == TEXT("helm")) { Captain->DebugTakeStation(EDCStationType::Helm, EDCSide::None); }
			else { Captain->DebugTakeStation(EDCStationType::Cannons, StationName == TEXT("port") ? EDCSide::Port : EDCSide::Starboard); }
		}
	}

	// Test runs: speed the whole world up evenly so crew, guns and hazards stay in step.
	float TestDilation = 1.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCTimeScale="), TestDilation)) { UGameplayStatics::SetGlobalTimeDilation(World, TestDilation); }
	FString Shots;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCAutoShots="), Shots, false))
	{
		TArray<FString> Parts;
		Shots.ParseIntoArray(Parts, TEXT(","));
		for (const FString& P : Parts) { AutoShotTimes.Add(FCString::Atof(*P)); }
		AutoShotWait = 6.f;
		if (AutoShotTimes.Num() && !FParse::Param(FCommandLine::Get(), TEXT("DCRunThrough"))) { Director->Seek(AutoShotTimes[0]); }

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
	// Frame rate in real time, whatever the test time dilation.
	SmoothedFps = FMath::Lerp(SmoothedFps, 1.f / FMath::Max(FApp::GetDeltaTime(), 0.0001), 0.05f);
	if (LateOrders.Num())
	{
		ADCVoice* Voice = ADCVoice::Get(this);
		LateOrderWait -= DeltaSeconds;
		if (Voice && Voice->IsLLMReady() && LateOrderWait <= 0.f)
		{
			GetWorld()->GetSubsystem<UDCCrewSubsystem>()->ExecuteOrder(LateOrders[0].Replace(TEXT("_"), TEXT(" ")));
			LateOrders.RemoveAt(0);
			LateOrderWait = 4.f;
		}
	}
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

	// -DCRunThrough plays the ride without seeking: each shot waits for the ride to reach its time.
	const bool bRunThrough = FParse::Param(FCommandLine::Get(), TEXT("DCRunThrough"));
	AutoShotWait -= DeltaSeconds;
	if (bRunThrough ? (!bAutoShotTaken && Director->RideTime < AutoShotTimes[AutoShotIndex]) : (AutoShotWait > 0.f)) { return; }
	if (bRunThrough && bAutoShotTaken && AutoShotWait > 0.f) { return; }

	if (!bAutoShotTaken)
	{
		const FString File = FPaths::ProjectSavedDir() / TEXT("Screenshots/Auto") / FString::Printf(TEXT("shot_%04d.png"), FMath::RoundToInt(AutoShotTimes[AutoShotIndex]));
		FScreenshotRequest::RequestScreenshot(File, false, false);
		UE_LOG(LogDC, Log, TEXT("Auto shot %s at ride time %.1f, %.0f fps"), *FPaths::GetCleanFilename(File), Director->RideTime, SmoothedFps);
		bAutoShotTaken = true;
		AutoShotWait = 1.f;
		return;
	}

	bAutoShotTaken = false;
	++AutoShotIndex;
	if (AutoShotTimes.IsValidIndex(AutoShotIndex))
	{
		if (!bRunThrough) { Director->Seek(AutoShotTimes[AutoShotIndex]); }
		AutoShotWait = 5.f;
	}
	else
	{
		FPlatformMisc::RequestExit(false);
	}
}
