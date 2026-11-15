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
#include "DCWake.h"
#include "DCFogFront.h"
#include "DCWaveCurl.h"
#include "DCMaelstrom.h"
#include "DCTroubles.h"
#include "DCEarlyEncounters.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Font.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "RenderTimer.h"
#include "RHI.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimationAsset.h"
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

namespace
{
	bool IsReviewSail(UMaterialInterface* Material)
	{
		for (int32 Depth=0; Material && Depth<8; ++Depth)
		{
			if (Material->GetName()==TEXT("MI_Sail_2-wp")) { return true; }
			const UMaterialInstance* Instance=Cast<UMaterialInstance>(Material);
			Material=Instance ? Instance->Parent.Get() : nullptr;
		}
		return false;
	}
	void ReviewSailMaterials(AActor* Ship, bool bApply)
	{
		float Normal=-1.f, SSS=-1.f;
		const bool HasNormal=FParse::Value(FCommandLine::Get(),TEXT("DCSailMaskNormal="),Normal);
		const bool HasSSS=FParse::Value(FCommandLine::Get(),TEXT("DCSailSSS="),SSS);
		if (!Ship || (!HasNormal && !HasSSS && !FParse::Param(FCommandLine::Get(),TEXT("DCSailReview")))) { return; }
		for (UActorComponent* C:Ship->GetComponents())
		{
			UMeshComponent* Mesh=Cast<UMeshComponent>(C);
			if (!Mesh) { continue; }
			for(int32 Slot=0;Slot<Mesh->GetNumMaterials();++Slot)
			{
				UMaterialInterface* Material=Mesh->GetMaterial(Slot);
				if (!IsReviewSail(Material)) { continue; }
				if(bApply && (HasNormal || HasSSS))
				{
					if(auto* MID=Mesh->CreateDynamicMaterialInstance(Slot))
					{
						if(HasNormal) MID->SetScalarParameterValue(TEXT("Mask_Normal_Strengh"),Normal);
						if(HasSSS) MID->SetScalarParameterValue(TEXT("SSS"),SSS);
						Material=MID;
					}
				}
				float ActualNormal=-1.f,ActualSSS=-1.f; FLinearColor Camera=FLinearColor::Black;
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Mask_Normal_Strengh")),ActualNormal);
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SSS")),ActualSSS);
				Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Camera")),Camera);
				UE_LOG(LogDC,Log,TEXT("SAIL_REVIEW apply=%d component=%s material=%s normal=%.4f sss=%.4f camera=%s"),bApply,*Mesh->GetName(),*Material->GetPathName(),ActualNormal,ActualSSS,*Camera.ToString());
			}
		}
	}
	FSlateFontInfo HudFont(UFont* Vector, UFont* SizeOf, float Scale)
	{
		// The same height on screen as the bitmap font it replaces, at the scale asked for.
		const float Pixels = (SizeOf ? SizeOf->GetMaxCharHeight() : 16.f) * Scale;
		const bool bLarge = GEngine && SizeOf == GEngine->GetLargeFont();
		return FSlateFontInfo(Vector, FMath::Max(6, FMath::RoundToInt(Pixels / 1.55f)), bLarge ? FName(TEXT("Bold")) : FName(TEXT("Regular")));
	}
}

void ADCHUD::CrispText(const FString& Str, FLinearColor Color, float X, float Y, UFont* SizeOf, float Scale)
{
	if (!VectorFont) { VectorFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")); }
	if (!VectorFont || !Canvas || !FSlateApplication::IsInitialized()) { DrawText(Str, Color, X, Y, SizeOf, Scale); return; }
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Str), HudFont(VectorFont, SizeOf, Scale), Color);
	Canvas->DrawItem(Item);
}

void ADCHUD::CrispSize(const FString& Str, float& OutWidth, float& OutHeight, UFont* SizeOf, float Scale)
{
	if (!VectorFont) { VectorFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")); }
	if (!VectorFont || !FSlateApplication::IsInitialized()) { GetTextSize(Str, OutWidth, OutHeight, SizeOf, Scale); return; }
	const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str, HudFont(VectorFont, SizeOf, Scale));
	OutWidth = Size.X; OutHeight = Size.Y;
}

void ADCHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) { return; }
	if (const ADCGameMode* Game = GetWorld()->GetAuthGameMode<ADCGameMode>(); Game && Game->IsAwaitingStart()) { return; }
	// -DCNoHUD: clean pictures for the store page and the dev log.
	static const bool bNoHUD = FParse::Param(FCommandLine::Get(), TEXT("DCNoHUD"));
	if (bNoHUD) { return; }

	ADCRideDirector* D = ADCRideDirector::Get(this);
	ADCTreadmill* T = ADCTreadmill::Get(this);
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* Big = GEngine ? GEngine->GetLargeFont() : Font;
	const float W = Canvas->SizeX, H = Canvas->SizeY;
	const float S = FMath::Max(1.f, H / 720.f); // everything scales with resolution

	// Text with a drop shadow so it reads over fog, fire and sea alike.
	auto Label = [this, S](const FString& Str, float X, float Y, float Scale, const FLinearColor& Color, UFont* F)
	{
		CrispText(Str, FLinearColor(0.f, 0.f, 0.f, 0.85f * Color.A), X + 2.f * S, Y + 2.f * S, F, Scale * S);
		CrispText(Str, Color, X, Y, F, Scale * S);
	};
	auto Centered = [this, W, S, &Label](const FString& Str, float Y, float Scale, const FLinearColor& Color, UFont* F)
	{
		float TW = 0.f, TH = 0.f;
		CrispSize(Str, TW, TH, F, Scale * S);
		Label(Str, (W - TW) * 0.5f, Y, Scale, Color, F);
	};
	const FLinearColor Gold(1.f, 0.85f, 0.5f), White(1.f, 1.f, 1.f), Dim(0.75f, 0.8f, 0.85f);

	// Hull condition and current location.
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

	if (ADCEarlyEncounters* Encounters = ADCEarlyEncounters::Get(this))
	{
		const FString Threat = Encounters->GetThreatText();
		if (!Threat.IsEmpty()) { Centered(Threat, 72.f * S, 1.15f, Gold, Font); }
	}

	// Developer line, only with -DCDebug.
	static const bool bDebug = FParse::Param(FCommandLine::Get(), TEXT("DCDebug"));
	if (bDebug && D && T)
	{
		const int32 Sec = FMath::FloorToInt(D->RideTime);
		Label(FString::Printf(TEXT("%d:%02d   helm %+.1f   lane %+.0f m   hostiles %d"), Sec / 60, Sec % 60, T->GetSteer(), T->GetLaneOffset() / 100.f, T->NumHostiles()),
			40.f * S, AlertY, 1.f, Dim, Font);
	}

	if (ADCVoice* Voice=ADCVoice::Get(this); Voice && Voice->IsListening())
    { Centered(TEXT("LISTENING..."), H*.58f, 1.5f, Gold, Font); }

	// Subtitles, bottom left, on a dark plate.
	DrawCrewPanel();

	DrawSpeechBubble();

	// Station names are drawn in screen space so weather and lighting cannot wash them out.
	if (ADCPlayerCharacter* Captain = Cast<ADCPlayerCharacter>(GetOwningPawn()))
	{
		APlayerController* PC = GetOwningPlayerController();
		ADCGunnery* Guns = ADCGunnery::Get(this);
		// The repair destination must survive mast/deck occlusion, including from
		// the helm. Keep its compact HUD plate clear of crew cards and prompts.
		if (ADCTroubles* Troubles=ADCTroubles::Get(this); Troubles && Troubles->IsRiggingLoose()
			&& PC && (!D || D->GetPhase()!=EDCPhase::Ending))
		{
			FVector RepairDeck;
			if (Troubles->GetRiggingRepairLocation(RepairDeck))
			{
				const FVector RepairWorld=D ? D->DeckToWorld(RepairDeck) : RepairDeck;
				const FVector Target=D ? D->DeckToWorld(RepairDeck+FVector(0,0,95)) : RepairDeck+FVector(0,0,95);
				FVector2D Screen;
				const bool Projected=PC->ProjectWorldLocationToScreen(Target,Screen);
				if (!Projected)
				{
					FVector Eye; FRotator View;
					PC->GetPlayerViewPoint(Eye,View);
					const float Side=FVector::DotProduct(Target-Eye,FRotationMatrix(View).GetUnitAxis(EAxis::Y));
					Screen=FVector2D(Side<0 ? -W : W*2.f,H*.48f);
				}
				const float MS=FMath::Clamp(FMath::Min(W/1280.f,H/720.f),.78f,1.6f);
				const float CrewS=FMath::Clamp(FMath::Min(W/1280.f,H/720.f),.78f,2.f);
				const FString Text=FString::Printf(TEXT("REPAIR RIGGING  |  %.0f m"),FVector::Distance(Captain->GetActorLocation(),RepairWorld)/100.f);
				float TW=0.f,TH=0.f;
				CrispSize(Text,TW,TH,Font,MS);
				const float Pad=8.f*MS, PW=TW+2.f*Pad, PH=TH+2.f*Pad;
				// Reserve the entire crew-card column and bottom speech/cannon UI.
				const float Left=24.f*MS+PW*.5f;
				const float Right=FMath::Max(Left,W-266.f*CrewS-PW*.5f);
				const float Top=FMath::Min(200.f*MS,H*.38f);
				const float Bottom=FMath::Max(Top,H*.58f-PH);
				const FVector2D Pin(FMath::Clamp(float(Screen.X),Left,Right),FMath::Clamp(float(Screen.Y),Top,Bottom));
				const bool AtEdge=!Projected || FVector2D::Distance(Pin,Screen)>2.f;
				auto RepairLine=[&](FVector2D A,FVector2D B)
				{
					DrawLine(A.X,A.Y,B.X,B.Y,FLinearColor(.01f,.02f,.025f,.95f),4.f*MS);
					DrawLine(A.X,A.Y,B.X,B.Y,Gold,1.5f*MS);
				};
				if (AtEdge)
				{
					const FVector2D Direction=(Screen-Pin).GetSafeNormal();
					const FVector2D Perp(-Direction.Y,Direction.X);
					const FVector2D Tip=Pin+Direction*(9.f*MS),Tail=Pin-Direction*(4.f*MS);
					RepairLine(Tip,Tail+Perp*(5.f*MS)); RepairLine(Tip,Tail-Perp*(5.f*MS));
				}
				else
				{
					const float R=6.f*MS;
					RepairLine(Pin+FVector2D(0,-R),Pin+FVector2D(R,0));
					RepairLine(Pin+FVector2D(R,0),Pin+FVector2D(0,R));
					RepairLine(Pin+FVector2D(0,R),Pin+FVector2D(-R,0));
					RepairLine(Pin+FVector2D(-R,0),Pin+FVector2D(0,-R));
				}
				const float X=Pin.X-PW*.5f,Y=Pin.Y+13.f*MS;
				DrawRect(FLinearColor(.012f,.022f,.03f,.92f),X,Y,PW,PH);
				CrispText(Text,Gold,X+Pad,Y+Pad,Font,MS);
			}
		}
		if (!Captain->IsTypingOrder() && !Captain->IsAtStation() && !Captain->IsShipView() && PC)
		{
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
			{
				const TArray<FDCStation>& Stations = Crew->GetStations();
				for (int32 Index = 0; Index < Stations.Num(); ++Index)
				{
					const FDCStation& Station = Stations[Index];
					const bool bGun = Station.Type == EDCStationType::Cannons && Guns && Guns->NumGuns(Station.Side) > 0;
					if ((!bGun && Station.Type != EDCStationType::Helm) || (bGun && !Station.bPlayerGun)) { continue; }
					const FVector Spot = (D ? D->DeckToWorld(Station.Location) : Station.Location) + FVector(0.f, 0.f, 150.f);   // the ship moves on the sea; stations are kept for the ship at rest
					if (FVector::DistSquared(Spot, Captain->GetActorLocation()) > FMath::Square(3600.f)) { continue; }
					FVector2D Screen;
					if (!PC->ProjectWorldLocationToScreen(Spot, Screen) ||
						Screen.X < 75.f * S || Screen.X > W - 75.f * S || Screen.Y < 115.f * S || Screen.Y > H * 0.72f) { continue; }

					const bool bNear = Index == Captain->GetNearbyStation();
					const TCHAR* Name = !bGun ? TEXT("HELM") : Station.Side == EDCSide::Port ? TEXT("PORT CANNON") : TEXT("STARBOARD CANNON");
					const FString Text = bNear ? FString::Printf(TEXT("[E]  %s"), Name) : FString(Name);
					float TW, TH;
					CrispSize(Text, TW, TH, Font, S);
					const float X = Screen.X - TW * 0.5f;
					DrawRect(FLinearColor(0.015f, 0.02f, 0.025f, 0.85f), X - 8.f * S, Screen.Y - 3.f * S, TW + 16.f * S, TH + 6.f * S);
					Label(Text, X, Screen.Y, 1.f, bNear ? Gold : White, Font);
					DrawLine(Screen.X, Screen.Y + TH + 4.f * S, Screen.X, Screen.Y + TH + 14.f * S, Gold, 2.f * S);
				}
			}
		}

		const EDCSide Side = Captain->GetGunSide();
		if (!Captain->IsTypingOrder() && Guns && Side != EDCSide::None)
		{
			const float X = W * 0.5f, Y = H * 0.5f;
			const bool bInRange = Captain->HasGunSolution();
			const bool bReady = Guns->IsReady(Side);
			const FLinearColor Sight = !bInRange ? Dim : (bReady ? Gold : FLinearColor(0.98f, 0.52f, 0.24f));
			for (const FVector2D& Axis : { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) })
			{
				const FVector2D Center(X, Y);
				const FVector2D Inner = Center + Axis * 7.f * S, Outer = Center + Axis * 21.f * S;
				DrawLine(Inner.X, Inner.Y, Outer.X, Outer.Y, FLinearColor::Black, 5.f * S);
				DrawLine(Inner.X, Inner.Y, Outer.X, Outer.Y, Sight, 2.f * S);
			}
			DrawRect(Sight, X - S, Y - S, 2.f * S, 2.f * S);

			FString State = Side == EDCSide::Port ? TEXT("PORT CANNON") : TEXT("STARBOARD CANNON");
			if (!bReady) { State += FString::Printf(TEXT("   RELOADING %.1fs"), Guns->GetReloadSeconds(Side)); }
			else { State += TEXT("   READY"); }

			DrawRect(FLinearColor(0.015f, 0.02f, 0.025f, 0.85f), X - 230.f * S, H * 0.61f, 460.f * S, 52.f * S);
			Centered(State, H * 0.61f + 8.f * S, 1.f, Sight, Font);
			const float Bar = 1.f - Guns->GetReloadFraction(Side);
			DrawRect(FLinearColor(0.15f, 0.17f, 0.19f), X - 95.f * S, H * 0.61f + 36.f * S, 190.f * S, 4.f * S);
			DrawRect(Sight, X - 95.f * S, H * 0.61f + 36.f * S, 190.f * S * Bar, 4.f * S);

			const FString Impact = Guns->GetPlayerImpactLabel(Side);
			if (!Impact.IsEmpty()) { Centered(Impact, Y + 29.f * S, 1.2f, Impact == TEXT("HIT") ? Gold : White, Font); }
		}

		if (!Captain->IsTypingOrder() && !Captain->IsAtStation() && !Captain->IsShipView() && !Captain->GetFireFeedback().IsEmpty())
		{
			Centered(Captain->GetFireFeedback(), H * .69f, 1.1f, FLinearColor(.72f, .9f, .72f), Font);
		}
		if (!Captain->IsTypingOrder() && !Captain->GetPrompt().IsEmpty())
		{
			Centered(Captain->GetPrompt(), H * 0.94f, 1.25f, Gold, Font);
		}
	}

	// Outcome at the end; instructions live in the main menu.
	if (D)
	{
		if (D->GetPhase() == EDCPhase::Ending)
		{
			const float A = FMath::Clamp((D->RideTime - D->GetRideLength() + 14.f) / 3.f, 0.f, 1.f);
			const FLinearColor G(Gold.R, Gold.G, Gold.B, A), Wh(1.f, 1.f, 1.f, A);
			const int32 Hull = FMath::RoundToInt(D->ShipHealth);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f * A), W * 0.2f, H * 0.2f, W * 0.6f, 220.f * S);
			Centered(Hull > 60 ? TEXT("SHE CAME THROUGH") : (Hull > 20 ? TEXT("BATTERED, BUT AFLOAT") : TEXT("BARELY AFLOAT")), H * 0.2f + 22.f * S, 3.5f, G, Big);
			Centered(FString::Printf(TEXT("Hull %d%%     Damage taken %d     Limbs severed %d"), Hull, FMath::RoundToInt(D->DamageTaken), D->ArmsBroken), H * 0.2f + 112.f * S, 1.6f, Wh, Font);
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
	PrimaryActorTick.bTickEvenWhenPaused = true;
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
    const TCHAR* MenuCommandLine = FCommandLine::Get();
    const bool Review = FParse::Param(MenuCommandLine,TEXT("unattended")) || FParse::Param(MenuCommandLine,TEXT("DCSkipIntro")) || FParse::Param(MenuCommandLine,TEXT("DCNoMenu")) || FCString::Strifind(MenuCommandLine,TEXT("DCStartAt=")) || FCString::Strifind(MenuCommandLine,TEXT("DCMaelstromStart="));
    bAwaitingStart = FParse::Param(MenuCommandLine,TEXT("DCShowMenu")) || !Review;
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

	// Test hook: -DCDumpClass=/Game/Path/BP.BP_C[;...] writes each class's own variables and defaults to Saved/class_<name>.txt, then quits.
	{
		FString Classes;
		if (FParse::Value(FCommandLine::Get(), TEXT("DCDumpClass="), Classes, false))
		{
			TArray<FString> List;
			Classes.ParseIntoArray(List, TEXT(";"));
			for (const FString& Path : List)
			{
				UClass* Cls = LoadObject<UClass>(nullptr, *Path);
				if (!Cls) { UE_LOG(LogDC, Warning, TEXT("DumpClass: %s not found"), *Path); continue; }
				FString Out;
				const UObject* CDO = Cls->GetDefaultObject();
				for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::ExcludeSuper); It; ++It)
				{
					FString Value;
					It->ExportText_InContainer(0, Value, CDO, CDO, nullptr, PPF_None);
					Value.ReplaceInline(TEXT("\n"), TEXT(" "));
					FString Category;
#if WITH_EDITOR
					Category = It->GetMetaData(TEXT("Category"));
#endif
					Out += FString::Printf(TEXT("%s | %s | %s | %s\n"), *Category, *It->GetName(), *It->GetCPPType(), *Value.Left(120));
				}
				for (TFieldIterator<UFunction> It(Cls, EFieldIteratorFlags::ExcludeSuper); It; ++It)
				{
					if (It->HasAnyFunctionFlags(FUNC_BlueprintCallable) && !It->GetName().StartsWith(TEXT("ExecuteUbergraph"))) { Out += FString::Printf(TEXT("FUNC | %s\n"), *It->GetName()); }
				}
				FFileHelper::SaveStringToFile(Out, *(FPaths::ProjectSavedDir() / FString::Printf(TEXT("class_%s.txt"), *Cls->GetName())));
				UE_LOG(LogDC, Display, TEXT("DumpClass: %s written"), *Cls->GetName());
			}
			FPlatformMisc::RequestExit(false);
			return;
		}
	}

	// Test hook: -DCKaiju stands the Claw Kaiju (Fab asset, not in the repo) off the bow playing an animation, to check its import.
	if (FParse::Param(FCommandLine::Get(), TEXT("DCKaiju")))
	{
		USkeletalMesh* KMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimation.ClawKaiju_allAnimation"));
		UAnimationAsset* KAnim = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimationClawKaiju_Intimidate.ClawKaiju_allAnimationClawKaiju_Intimidate"));
		if (KMesh)
		{
			ASkeletalMeshActor* K = World->SpawnActor<ASkeletalMeshActor>(FVector(9000.f, 2500.f, SeaLevelZ - 600.f), FRotator(0.f, 200.f, 0.f), Params);
			K->GetSkeletalMeshComponent()->SetMobility(EComponentMobility::Movable);
			K->GetSkeletalMeshComponent()->SetSkeletalMeshAsset(KMesh);
			K->GetSkeletalMeshComponent()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			if (KAnim) { K->GetSkeletalMeshComponent()->PlayAnimation(KAnim, true); }
			if (UMaterialInterface* KMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/M_ClawKaiju.M_ClawKaiju")))
			{
				for (int32 s = 0; s < K->GetSkeletalMeshComponent()->GetNumMaterials(); ++s) { K->GetSkeletalMeshComponent()->SetMaterial(s, KMat); }
			}
		}
		UE_LOG(LogDC, Log, TEXT("Kaiju test: mesh %s anim %s"), KMesh ? TEXT("ok") : TEXT("missing"), KAnim ? TEXT("ok") : TEXT("missing"));
	}

	// Test hook: -DCHide=Name hides every ship component whose name contains Name.
	{
		FString Hide;
		if (Ship && FParse::Value(FCommandLine::Get(), TEXT("DCHide="), Hide))
		{
			for (UActorComponent* C : Ship->GetComponents())
			{
				USceneComponent* SC = Cast<USceneComponent>(C);
				if (SC && SC->GetName().Contains(Hide)) { SC->SetVisibility(false, true); }
			}
		}
	}

	// Test hook: -DCExec=cmd~arg;cmd~arg runs console commands at start ("~" stands for a space).
	{
		FString Cmds;
		if (FParse::Value(FCommandLine::Get(), TEXT("DCExec="), Cmds, false))
		{
			TArray<FString> List;
			Cmds.ParseIntoArray(List, TEXT(";"));
			for (FString& Cmd : List)
			{
				Cmd.ReplaceInline(TEXT("~"), TEXT(" "));
				if (GEngine) { GEngine->Exec(World, *Cmd); }
			}
		}
	}

	Director = World->SpawnActor<ADCRideDirector>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Director->SeaLevelZ = SeaLevelZ;
	Treadmill = World->SpawnActor<ADCTreadmill>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ADCAudio>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ADCWeatherFX>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ADCWake>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ADCEarlyEncounters>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ADCFogFront>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ADCWaveCurl>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
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
		// The asset's own blueprint also bobs the ship on its water system every frame. Pinning it back
		// each frame left the hull trembling a centimetre or two under a crew that stood still, so the
		// blueprint's own tick is switched off; its components (sails, flags) keep theirs.
		if (!FParse::Param(FCommandLine::Get(), TEXT("DCShipTick"))) { Ship->SetActorTickEnabled(false); }
		if (FParse::Param(FCommandLine::Get(), TEXT("DCShipDump")))
		{
			UE_LOG(LogDC, Log, TEXT("ShipDump actor class %s, root %s (%s), mobility %d"), *Ship->GetClass()->GetName(), *Ship->GetRootComponent()->GetName(), *Ship->GetRootComponent()->GetClass()->GetName(), static_cast<int32>(Ship->GetRootComponent()->Mobility));
			for (UActorComponent* C : Ship->GetComponents())
			{
				const UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C);
				if (!P || P->IsSimulatingPhysics() || C->IsComponentTickEnabled())
				{
					UE_LOG(LogDC, Log, TEXT("ShipDump %s (%s) tick=%d sim=%d"), *C->GetName(), *C->GetClass()->GetName(), C->IsComponentTickEnabled() ? 1 : 0, (P && P->IsSimulatingPhysics()) ? 1 : 0);
				}
			}
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
	if (ADCGunnery* Guns = ADCGunnery::Get(this))
	{
		for (EDCSide Side : { EDCSide::Port, EDCSide::Starboard })
		{
			if (FDCStation* Player = Crew->GetStation(Crew->FindStation(EDCStationType::Cannons, Side)))
			{ Player->GunIndex = Guns->GetPlayerGun(Side); }
		}
		for (int32 Gun = 0; Gun < Guns->GetGunCount(); ++Gun)
		{
			const EDCSide Side = Guns->GetGunSide(Gun);
			if (Gun == Guns->GetPlayerGun(Side)) { continue; }
			// Accessible upper-deck guns only; below-deck ports have no walkable route.
			const FVector Spot = Guns->GetGunStation(Gun);
			if (Spot.Z < MainTop - 80.f) { continue; }
			Crew->AddStation(EDCStationType::Cannons, Side, DeckAt(Spot.X, Spot.Y, MainTop));
			FDCStation* Station = Crew->GetStation(Crew->GetStations().Num() - 1);
			Station->GunIndex = Gun;
			Station->bPlayerGun = false;
		}
	}
	Crew->AddStation(EDCStationType::Fire, EDCSide::None, DeckAt(GunS.X - 200.f, 60.f, MainTop));
	Crew->AddStation(EDCStationType::Rigging, EDCSide::None, DeckAt(GunS.X + 420.f, 120.f, MainTop));

	// Light the ship's own lanterns.
	Director->AdoptLampGlass(Ship);
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

	Crew->SpawnCrew(TEXT("Rex"), FLinearColor(0.9f, 0.7f, 0.2f), DeckAt(Wheel.X - 60.f, -180.f, Wheel.Z + 120.f));
	Crew->SpawnCrew(TEXT("Duncan"), FLinearColor(0.3f, 0.7f, 0.9f), DeckAt(GunS.X + 150.f, 80.f, MainTop));
	Crew->SpawnCrew(TEXT("Ruby"), FLinearColor(0.8f, 0.3f, 0.5f), DeckAt(GunS.X - 350.f, -90.f, MainTop));

	Super::StartPlay();
	ReviewSailMaterials(Ship,true); // after Blueprint BeginPlay material setup

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

	Crew->ExecuteOrder(TEXT("Rex, take the helm"));
	Crew->ExecuteOrder(TEXT("Duncan, man the port guns"));
	Crew->ExecuteOrder(TEXT("Ruby, man the starboard guns"));

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
	float StartAt = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCStartAt="), StartAt)) { Director->Seek(StartAt); }
	FString Shots;
	if (FParse::Value(FCommandLine::Get(), TEXT("DCAutoShots="), Shots, false))
	{
		TArray<FString> Parts;
		Shots.ParseIntoArray(Parts, TEXT(","));
		for (const FString& P : Parts) { AutoShotTimes.Add(FCString::Atof(*P)); }
		AutoShotWait = 6.f;
		if (AutoShotTimes.Num() && !FParse::Param(FCommandLine::Get(), TEXT("DCRunThrough"))) { Director->Seek(AutoShotTimes[0]); }

		FString Cam;
		FString CrewCloseup;
		if (FParse::Value(FCommandLine::Get(), TEXT("DCShotCrew="), CrewCloseup))
		{
			for (ADCCrewMember* Member : Crew->GetCrew())
			{
				if (Member->CrewName != CrewCloseup) { continue; }
				const FVector Focus = Member->GetActorLocation() + FVector(0.f, 0.f, 65.f);
				const FVector Position = Focus + FVector(230.f, -120.f, 10.f);
				ACameraActor* Closeup = World->SpawnActor<ACameraActor>(Position, (Focus - Position).Rotation(), Params);
				Closeup->AttachToActor(Member, FAttachmentTransformRules::KeepWorldTransform);
				Closeup->GetCameraComponent()->bConstrainAspectRatio = false;
				Closeup->GetCameraComponent()->SetFieldOfView(40.f);
				if (APlayerController* PC = World->GetFirstPlayerController()) { PC->SetViewTarget(Closeup); }
				break;
			}
		}
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
	if (bAwaitingStart) { ShowMainMenu(); }
}

void ADCGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAwaitingStart) { TickMainMenu(DeltaSeconds); return; }
	// Frame rate in real time, whatever the test time dilation.
	SmoothedFps = FMath::Lerp(SmoothedFps, 1.f / FMath::Max(FApp::GetDeltaTime(), 0.0001), 0.05f);
	// Test hook: -DCHitch logs every frame longer than 25 ms, and -DCCrewLog logs where each hand's
	// body and head are every frame for ride times 20 to 24 s, to catch jitter.
	{
		static const bool bHitch = FParse::Param(FCommandLine::Get(), TEXT("DCHitch"));
		static const bool bCrewLog = FParse::Param(FCommandLine::Get(), TEXT("DCCrewLog"));
		const ADCRideDirector* Dir = ADCRideDirector::Get(this);
		const double Ms = FApp::GetDeltaTime() * 1000.0;
		if (bHitch && Dir && Ms > 25.0 && GetWorld()->GetTimeSeconds() > 5.f) { UE_LOG(LogDC, Log, TEXT("Hitch %.0f ms at ride time %.1f"), Ms, Dir->RideTime); }
		static float LogFrom = 20.f; static bool bFromRead = false; if (!bFromRead) { bFromRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCCrewLogFrom="), LogFrom); }
		if (bCrewLog && Dir && Dir->RideTime > LogFrom && Dir->RideTime < LogFrom + 4.f)
		{
			if (Ship) { const UPrimitiveComponent* Hull = nullptr; TArray<UPrimitiveComponent*> Ps; Ship->GetComponents(Ps); for (UPrimitiveComponent* P : Ps) { if (P->GetName() == TEXT("HullShip")) { Hull = P; } } const FVector SL = Ship->GetActorLocation(); const FRotator SR = Ship->GetActorRotation(); const FVector HL = Hull ? Hull->GetComponentLocation() : FVector::ZeroVector; const FRotator HR = Hull ? Hull->GetComponentRotation() : FRotator::ZeroRotator; UE_LOG(LogDC, Log, TEXT("ShipLog t=%.3f ship=%.3f,%.3f,%.3f rot=%.3f,%.3f,%.3f hull=%.3f,%.3f,%.3f hullrot=%.3f,%.3f,%.3f"), Dir->RideTime, SL.X, SL.Y, SL.Z, SR.Pitch, SR.Yaw, SR.Roll, HL.X, HL.Y, HL.Z, HR.Pitch, HR.Yaw, HR.Roll); }
			if (const APawn* Me = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr) { const FVector OnDeck = Dir->DeckTransform.InverseTransformPosition(Me->GetActorLocation()); FVector CamLoc; FRotator CamRot; GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(CamLoc, CamRot); UE_LOG(LogDC, Log, TEXT("PlayerLog t=%.3f ondeck=%.2f,%.2f,%.2f world=%.2f cam=%.2f,%.2f,%.2f camrot=%.2f,%.2f,%.2f"), Dir->RideTime, OnDeck.X, OnDeck.Y, OnDeck.Z, Me->GetActorLocation().Z, CamLoc.X, CamLoc.Y, CamLoc.Z, CamRot.Pitch, CamRot.Yaw, CamRot.Roll); }
			for (TActorIterator<ADCCrewMember> It(GetWorld()); It; ++It)
			{
				const USkeletalMeshComponent* Fig = It->FindComponentByClass<USkeletalMeshComponent>();
				const FVector A = It->GetActorLocation();
				const FVector Head = Fig ? Fig->GetBoneLocation(TEXT("head")) : FVector::ZeroVector;
				const FVector Pelvis = Fig ? Fig->GetBoneLocation(TEXT("pelvis")) : FVector::ZeroVector;
				UE_LOG(LogDC, Log, TEXT("CrewLog %s t=%.3f actor=%.2f,%.2f,%.2f head=%.2f,%.2f,%.2f pelvisZ=%.2f yaw=%.1f"), *It->CrewName, Dir->RideTime, A.X, A.Y, A.Z, Head.X, Head.Y, Head.Z, Pelvis.Z, It->GetActorRotation().Yaw);
			}
		}
	}
	// -DCRestartAt=20 reloads the level once after that many seconds, as the R key does (tests that a second voyage works).
	static bool bRestartTested = false;
	float RestartAt = 0.f;
	if (!bRestartTested && FParse::Value(FCommandLine::Get(), TEXT("DCRestartAt="), RestartAt) && GetWorld()->GetTimeSeconds() > RestartAt)
	{
		bRestartTested = true;
		UE_LOG(LogDC, Log, TEXT("Restart test: reloading the level"));
		UGameplayStatics::ClearSoundMixModifiers(this);
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
		return;
	}
	// -DCBot: a stand-in captain for balance runs. Mans both batteries, then pulls one gunner off for
	// each fire or torn sail and sends them back when it is dealt with. Roughly a competent player.
	static const bool bBot = FParse::Param(FCommandLine::Get(), TEXT("DCBot"));
	if (bBot)
	{
		UDCCrewSubsystem* CrewSub = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
		ADCTroubles* Troubles = ADCTroubles::Get(this);
		BotWait -= DeltaSeconds;
		if (CrewSub && Troubles && BotWait <= 0.f)
		{
			BotWait = 2.5f; // reaction time
			const bool bFire = Troubles->NumFires() > 0;
			const bool bRig = Troubles->IsRiggingLoose();
			if (BotStage == 0)
			{
				CrewSub->ExecuteOrder(TEXT("Duncan, man the port guns"));
				CrewSub->ExecuteOrder(TEXT("Ruby, man the starboard guns"));
				BotStage = 1;
			}
			if (bFire != bBotOdessaOnFire)
			{
				bBotOdessaOnFire = bFire;
				CrewSub->ExecuteOrder(bFire ? TEXT("Ruby, put out the fire") : TEXT("Ruby, man the starboard guns"));
			}
			if (bRig != bBotMarlowOnRigging)
			{
				bBotMarlowOnRigging = bRig;
				CrewSub->ExecuteOrder(bRig ? TEXT("Duncan, repair the rigging") : TEXT("Duncan, man the port guns"));
			}
		}
	}

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
	TickShipMotion(DeltaSeconds);
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
		ReviewSailMaterials(Ship,false);
		// -DCTenths names shots by tenths of a second and allows them 0.1 s apart (with -DCRunThrough), for checking flicker.
		const bool bTenths = FParse::Param(FCommandLine::Get(), TEXT("DCTenths"));
		const FString File = FPaths::ProjectSavedDir() / TEXT("Screenshots/Auto") / (bTenths ? FString::Printf(TEXT("shot_%05d.png"), FMath::RoundToInt(AutoShotTimes[AutoShotIndex] * 10.f)) : FString::Printf(TEXT("shot_%04d.png"), FMath::RoundToInt(AutoShotTimes[AutoShotIndex])));
		if (FParse::Param(FCommandLine::Get(), TEXT("DCDumpNear")))
		{
			// Test hook: list everything visible whose bounds come within 60 m of the ship.
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				TInlineComponentArray<UPrimitiveComponent*> Prims(*It);
				for (UPrimitiveComponent* P : Prims)
				{
					if (!P->IsVisible() || P->GetOwner() == Ship) { continue; }
					const FBoxSphereBounds B = P->Bounds;
					if (B.GetBox().ComputeSquaredDistanceToPoint(FVector(0.f, 0.f, 1000.f)) < FMath::Square(6000.f) && B.SphereRadius < 400000.f)
					{
						UE_LOG(LogDC, Display, TEXT("NEAR %s.%s origin %s extent %s actor %s rot %s"), *It->GetName(), *P->GetName(), *B.Origin.ToCompactString(), *B.BoxExtent.ToCompactString(), *It->GetActorLocation().ToString(), *It->GetActorRotation().ToCompactString());
					}
				}
			}
		}
		FScreenshotRequest::RequestScreenshot(File, false, false);
		// -DCProfile also writes a GPU timing breakdown to the log at each shot.
		if (FParse::Param(FCommandLine::Get(), TEXT("DCProfile")) && GEngine) { GEngine->Exec(GetWorld(), TEXT("ProfileGPU")); }
		UE_LOG(LogDC, Log, TEXT("Auto shot %s at ride time %.1f, %.0f fps (game %.1f ms, render %.1f ms, gpu %.1f ms)"), *FPaths::GetCleanFilename(File), Director->RideTime, SmoothedFps, FPlatformTime::ToMilliseconds(GGameThreadTime), FPlatformTime::ToMilliseconds(GRenderThreadTime), FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
		const ADCTroubles* ShotTroubles = ADCTroubles::Get(this);
		UE_LOG(LogDC, Log, TEXT("Capture state: hull=%.1f score=%d fires=%d rigging=%d at %.1f"),
			Director->ShipHealth, Director->Score, ShotTroubles ? ShotTroubles->NumFires() : 0,
			ShotTroubles && ShotTroubles->IsRiggingLoose(), Director->RideTime);
		const FDCEnvState& ShotWeather = Director->GetEnv();
		UE_LOG(LogDC, Log, TEXT("Capture weather: rain=%.3f lightning=%.3f waves=%.1f at %.1f"),
			ShotWeather.Rain, ShotWeather.LightningRate, ShotWeather.WaveHeight, Director->RideTime);
		bAutoShotTaken = true;
		AutoShotWait = bTenths ? 0.05f : 1.f;
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

void ADCGameMode::TickShipMotion(float DeltaSeconds)
{
	if (!Ship) { return; }
	ADCRideDirector* Dir = ADCRideDirector::Get(this);

	// The ship's blueprint switches physics back on for its base plate after it begins play. With no
	// water system of its own to float on, the hull then fell a few centimetres every frame and was
	// put back every frame: a constant tremble under a crew that stood still. Stop the simulation
	// itself, whenever it is found running.
	static int32 Frame = 0;
	UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Ship->GetRootComponent());
	if ((Root && Root->IsSimulatingPhysics()) || (++Frame % 120) == 0)
	{
		TArray<UPrimitiveComponent*> ShipPrims;
		Ship->GetComponents(ShipPrims);
		for (UPrimitiveComponent* P : ShipPrims)
		{
			if (P->IsSimulatingPhysics())
			{
				UE_LOG(LogDC, Log, TEXT("Ship: physics found running on %s at ride time %.1f; stopped"), *P->GetName(), Dir ? Dir->RideTime : 0.f);
				P->SetPhysicsLinearVelocity(FVector::ZeroVector);
				P->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				P->SetSimulatePhysics(false);
			}
		}
	}

	// She rides the plugin sea: the wake already knows the height of the water down both sides of the
	// hull, so the hull is leaned and lifted toward that plane, a good deal less than the water itself
	// (a ship averages the waves under her) and slowly. Off in the tunnel and the maelstrom, where the
	// stage moves the world round her instead. -DCNoShipMotion keeps her still.
	static const bool bStill = FParse::Param(FCommandLine::Get(), TEXT("DCNoShipMotion"));
	static float Amount = 1.5f;
	static bool bRead = false;
	if (!bRead) { bRead = true; FParse::Value(FCommandLine::Get(), TEXT("DCShipMotion="), Amount); }
	float Rise = 0.f, SlopeX = 0.f, SlopeY = 0.f;
	const ADCWake* Wake = bStill ? nullptr : ADCWake::Get(this);
	if (!Wake || !Wake->GetSeaUnderHull(Rise, SlopeX, SlopeY)) { Rise = SlopeX = SlopeY = 0.f; }
	const float MaxPitch = FMath::Tan(FMath::DegreesToRadians(5.5f)), MaxRoll = FMath::Tan(FMath::DegreesToRadians(10.f));
	// The waves under the hull are short and average out; what a deck really does is the long slow
	// roll and pitch of the swell, so she is given one of her own, bigger as the sea gets up.
	const bool bRiding = Wake && (Rise != 0.f || SlopeX != 0.f);
	const float Sea = (bRiding && Dir) ? FMath::Clamp(Dir->GetEnv().WaveHeight / 120.f, 0.6f, 2.2f) * Amount / 1.5f : 0.f;
	const float T = GetWorld()->GetTimeSeconds();
	const float SwellRoll = FMath::Tan(FMath::DegreesToRadians((FMath::Sin(T * 0.83f) * 2.4f + FMath::Sin(T * 0.47f + 1.3f) * 1.3f) * Sea));
	const float SwellPitch = FMath::Tan(FMath::DegreesToRadians((FMath::Sin(T * 1.07f + 0.6f) * 1.3f + FMath::Sin(T * 0.61f) * 0.7f) * Sea));
	const float SwellHeave = (FMath::Sin(T * 1.07f + 2.1f) * 22.f + FMath::Sin(T * 0.53f) * 14.f) * Sea;
	ShipHeave = FMath::FInterpTo(ShipHeave, FMath::Clamp(Rise * Amount, -150.f, 150.f) + SwellHeave, DeltaSeconds, 1.6f);
	ShipSlope.X = FMath::FInterpTo(ShipSlope.X, FMath::Clamp(SlopeX * Amount + SwellPitch, -MaxPitch, MaxPitch), DeltaSeconds, 1.3f);
	// On the side of the trough she lies on the slope of the sea there.
	const ADCTreadmill* Lane = ADCTreadmill::Get(this);
	const float Bank = (Dir && Lane && bRiding) ? Dir->SeaRiseSlope(Lane->GetLaneOffset()) : 0.f;
	ShipSlope.Y = FMath::FInterpTo(ShipSlope.Y, FMath::Clamp(SlopeY * Amount + SwellRoll, -MaxRoll, MaxRoll) + Bank, DeltaSeconds, 1.3f);

	// A blow to the hull: she lurches and rights herself over a second or two.
	if (Dir)
	{
		JoltSpeed += Dir->PendingJolt;
		Dir->PendingJolt = FVector::ZeroVector;
	}
	const float StepDt = FMath::Min(DeltaSeconds, 0.05f);
	JoltSpeed += (-30.f * JoltAngle - 4.2f * JoltSpeed) * StepDt;
	JoltAngle += JoltSpeed * StepDt;
	JoltAngle = JoltAngle.BoundToCube(9.f);

	// Deck up = the normal of that plane; turned about the waterline amidships.
	const FVector Up = FVector(-ShipSlope.X, -ShipSlope.Y, 1.f).GetSafeNormal();
	// Under helm her head swings toward the turn and she heels a little away from it. Only on the open
	// sea and in Atlantis; the maelstrom stage turns the world round her itself.
	const ADCTreadmill* Mill = ADCTreadmill::Get(this);
	// A valid wave sample may be exactly zero; steering still turns the ship.
	// The slide/caldera own their transforms and must not inherit this yaw.
	const bool bHelmMotion = !bStill && Mill && Dir && Dir->GetPhase() <= EDCPhase::Fog;
	ShipTurn = FMath::FInterpTo(ShipTurn, bHelmMotion ? Mill->GetTurn() : 0.f, DeltaSeconds, 2.4f);
	const ADCMaelstrom* Stage = ADCMaelstrom::Get(this);
	const float DescentPitch = !bStill && Stage ? Stage->GetDeckPitch() : 0.f;
	const float LandingHeave = !bStill && Stage ? Stage->GetLandingHeave() : 0.f;
	const FQuat Helm = FRotator(DescentPitch + JoltAngle.Y, ShipTurn * 8.f + JoltAngle.Z, -ShipTurn * 3.f + JoltAngle.X).Quaternion();
	const FQuat Lean = FRotationMatrix::MakeFromZX(Up, FVector::ForwardVector).ToQuat() * Helm;
	const FVector Pivot(0.f, 0.f, Dir ? Dir->SeaLevelZ : 0.f);
	const FTransform Deck(Lean, Pivot - Lean.RotateVector(Pivot) + FVector(0.f, 0.f, ShipHeave + LandingHeave));
	if (Dir) { Dir->DeckTransform = Deck; }
	const FTransform Rest(ShipSpawnRotation, ShipSpawnLocation);
	const FTransform Now = Rest * Deck;
	Ship->SetActorLocationAndRotation(Now.GetLocation(), Now.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
}
