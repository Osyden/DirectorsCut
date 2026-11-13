#include "DCCrew.h"
#include "DCTreadmill.h"
#include "DCRideDirector.h"
#include "DCVoice.h"
#include "DCGunnery.h"
#include "DCTroubles.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include <initializer_list>
#include "HAL/IConsoleManager.h"

// ---------------------------------------------------------------- Intent parser

namespace
{
	bool HasAny(const FString& Text, std::initializer_list<const TCHAR*> Words)
	{
		for (const TCHAR* W : Words)
		{
			if (Text.Contains(W)) { return true; }
		}
		return false;
	}
}

FDCIntent DCIntentParser::Parse(const FString& InText, const TArray<FString>& CrewNames)
{
	FDCIntent I;
	I.RawText = InText;
	const FString T = FString(TEXT(" ")) + InText.ToLower() + TEXT(" ");

	for (const FString& Name : CrewNames)
	{
		if (T.Contains(Name.ToLower())) { I.CrewName = Name; break; }
	}

	if (HasAny(T, { TEXT("starboard"), TEXT(" right") })) { I.Side = EDCSide::Starboard; }
	else if (HasAny(T, { TEXT(" port"), TEXT("larboard"), TEXT(" left") })) { I.Side = EDCSide::Port; }

	const bool bExtinguish = HasAny(T, { TEXT("put out"), TEXT("extinguish"), TEXT("douse"), TEXT("bucket"), TEXT("flames"), TEXT("burning"), TEXT("on fire") });
	const bool bHelmWord = HasAny(T, { TEXT("helm"), TEXT("wheel"), TEXT("steer"), TEXT("rudder") });
	const bool bGunWord = HasAny(T, { TEXT("cannon"), TEXT("gun"), TEXT("battery"), TEXT("broadside") });
	const bool bRigWord = HasAny(T, { TEXT("sail"), TEXT("rigging"), TEXT("rope"), TEXT("mast"), TEXT("repair"), TEXT("mend"), TEXT("fix") });
	const bool bManWord = HasAny(T, { TEXT("man the"), TEXT("take the"), TEXT("get on"), TEXT("get to"), TEXT("go to"), TEXT(" take ") });
	const bool bTurnWord = HasAny(T, { TEXT("hard to"), TEXT("hard a"), TEXT("turn"), TEXT("come about"), TEXT("bear"), TEXT("veer"), TEXT("evade"), TEXT("dodge") });

	if (bExtinguish)
	{
		I.Station = EDCStationType::Fire; I.Action = EDCAction::Extinguish;
	}
	else if (HasAny(T, { TEXT("hold fire"), TEXT("cease"), TEXT("stop firing"), TEXT("hold your fire") }))
	{
		I.Station = EDCStationType::Cannons; I.Action = EDCAction::HoldFire;
	}
	else if (HasAny(T, { TEXT("steady"), TEXT("midships"), TEXT("straight"), TEXT("hold course"), TEXT("as she goes") }))
	{
		I.Station = EDCStationType::Helm; I.Action = EDCAction::Steady;
	}
	else if (bTurnWord && I.Side != EDCSide::None)
	{
		I.Station = EDCStationType::Helm;
		I.Action = I.Side == EDCSide::Port ? EDCAction::TurnPort : EDCAction::TurnStarboard;
	}
	else if (bHelmWord)
	{
		I.Station = EDCStationType::Helm;
		I.Action = (I.Side == EDCSide::Port) ? EDCAction::TurnPort : (I.Side == EDCSide::Starboard ? EDCAction::TurnStarboard : EDCAction::Man);
		if (bManWord) { I.Action = EDCAction::Man; I.Side = EDCSide::None; }
	}
	else if (HasAny(T, { TEXT(" fire"), TEXT("shoot"), TEXT("blast"), TEXT("sink"), TEXT("blow ") }) || (bGunWord && !bManWord))
	{
		I.Station = EDCStationType::Cannons; I.Action = EDCAction::Fire;
	}
	else if (bGunWord)
	{
		I.Station = EDCStationType::Cannons; I.Action = EDCAction::Man;
	}
	else if (bRigWord)
	{
		I.Station = EDCStationType::Rigging; I.Action = EDCAction::Repair;
	}
	else if (HasAny(T, { TEXT("report"), TEXT("status"), TEXT("how are we") }))
	{
		I.Action = EDCAction::Report;
	}
	else if (I.Side != EDCSide::None && InText.Len() < 24)
	{
		// A bare "starboard!" is a helm order.
		I.Station = EDCStationType::Helm;
		I.Action = I.Side == EDCSide::Port ? EDCAction::TurnPort : EDCAction::TurnStarboard;
	}

	I.bValid = I.Action != EDCAction::None;
	return I;
}

// ---------------------------------------------------------------- Crew member

ADCCrewMember::ADCCrewMember()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Mesh.Succeeded()) { Body->SetStaticMesh(Mesh.Object); }
	// The root is left unscaled: with a stretched root the figure under it was drawn with a false
	// velocity every frame and smeared by motion blur and the upscaler.

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Body);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 122.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(16.f);

	// Stand-in body: the mannequin that ships with the vessel asset, if present.
	Figure = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Figure"));
	Figure->SetupAttachment(Body);
	Figure->SetRelativeLocation(FVector(0.f, 0.f, -89.f));
	Figure->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADCCrewMember::Init(const FString& InName, const FLinearColor& Color)
{
	CrewName = InName;
	const bool bFemale = InName == TEXT("Odessa");
	const TCHAR* Dir = TEXT("/Game/Brigantine/Demo/Characters/Mannequins");
	USkeletalMesh* SkelMesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s/Meshes/%s"), Dir, bFemale ? TEXT("SKM_Quinn_Simple.SKM_Quinn_Simple") : TEXT("SKM_Manny_Simple.SKM_Manny_Simple")));
	IdleAnim = LoadObject<UAnimationAsset>(nullptr, *FString::Printf(TEXT("%s/Animations/%s"), Dir, bFemale ? TEXT("Quinn/MF_Idle.MF_Idle") : TEXT("Manny/MM_Idle.MM_Idle")));
	WalkAnim = LoadObject<UAnimationAsset>(nullptr, *FString::Printf(TEXT("%s/Animations/%s"), Dir, bFemale ? TEXT("Quinn/MF_Walk_Fwd.MF_Walk_Fwd") : TEXT("Manny/MM_Walk_Fwd.MM_Walk_Fwd")));
	if (SkelMesh)
	{
		Figure->SetSkeletalMeshAsset(SkelMesh);
		Figure->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (IdleAnim) { Figure->PlayAnimation(IdleAnim, true); }
		Body->SetVisibility(false);
		Label->SetRelativeLocation(FVector(0.f, 0.f, 108.f));
		// The stock mannequin is polished metal, which glitters and crawls under the upscaler. Until
		// there are proper sailors, paint it: matte, dark, each hand in a coat of their own colour.
		for (int32 Slot = 0; Slot < Figure->GetNumMaterials(); ++Slot)
		{
			if (UMaterialInstanceDynamic* Paint = Figure->CreateDynamicMaterialInstance(Slot))
			{
				const FLinearColor Coat = Slot == 0 ? FLinearColor(Color.R * 0.05f + 0.012f, Color.G * 0.05f + 0.012f, Color.B * 0.05f + 0.012f) : FLinearColor(0.035f, 0.028f, 0.022f);
				Paint->SetScalarParameterValue(TEXT("MetalPaintMetallic"), 0.f);
				Paint->SetScalarParameterValue(TEXT("Metal_Brightness"), 0.7f);
				Paint->SetScalarParameterValue(TEXT("Plastic_Brightness"), 0.7f);
				Paint->SetVectorParameterValue(TEXT("Tint"), Coat);
				for (const TCHAR* Logo : { TEXT("LogoLayer0_Brightness"), TEXT("LogoLayer1_Brightness"), TEXT("LogoLayer2_Brightness") }) { Paint->SetScalarParameterValue(Logo, 0.f); }
			}
		}
	}
	Label->SetText(FText::FromString(InName));
	Label->SetTextRenderColor(Color.ToFColor(true));
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		Body->SetMaterial(0, MID);
	}
}

void ADCCrewMember::AssignStation(int32 NewStation)
{
	UDCCrewSubsystem* Sub = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	if (FDCStation* Old = Sub->GetStation(StationIndex))
	{
		if (Old->Operator.Get() == this) { Old->Operator = nullptr; }
		if (Old->Type == EDCStationType::Helm)
		{
			if (ADCTreadmill* Treadmill = ADCTreadmill::Get(this)) { Treadmill->SetSteer(0.f); }
		}
	}
	StationIndex = NewStation;
	if (FDCStation* New = Sub->GetStation(NewStation))
	{
		New->Operator = this;
		State = EDCCrewState::Moving;
	}
	else
	{
		State = EDCCrewState::Idle;
	}
}

void ADCCrewMember::SetHelmOrder(EDCAction Action)
{
	HelmOrder = Action;
	HelmOrderTimeLeft = (Action == EDCAction::Steady) ? 0.f : 4.f;
}

void ADCCrewMember::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Face the name tag at the player.
	if (APlayerCameraManager* Cam = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->PlayerCameraManager : nullptr)
	{
		const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}

	UDCCrewSubsystem* Sub = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	FDCStation* Station = Sub ? Sub->GetStation(StationIndex) : nullptr;
	if (!Station)
	{
		State = EDCCrewState::Idle;
		return;
	}

	if (State == EDCCrewState::Moving)
	{
		const FVector Target = Station->Location + FVector(0.f, 0.f, 90.f);
		const FVector Here = GetActorLocation();
		const FVector NewLoc = FMath::VInterpConstantTo(Here, Target, DeltaSeconds, MoveSpeed);
		SetActorLocation(NewLoc);
		const FVector Dir = (Target - Here).GetSafeNormal2D();
		if (!Dir.IsNearlyZero()) { SetActorRotation(FRotator(0.f, Dir.Rotation().Yaw, 0.f)); }
		SetWalking(true);
		if (FVector::DistSquared(NewLoc, Target) < 25.f)
		{
			State = EDCCrewState::Operating;
			// Face the work: the helmsman looks forward, gunners look out over their rail.
			const float Yaw = Station->Type == EDCStationType::Cannons ? (Station->Side == EDCSide::Port ? -90.f : 90.f) : 0.f;
			SetActorRotation(FRotator(0.f, Yaw, 0.f));
			SetWalking(false);
		}
		return;
	}

	if (State == EDCCrewState::Operating)
	{
		ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
		if (!Treadmill) { return; }
		switch (Station->Type)
		{
		case EDCStationType::Helm: TickHelm(DeltaSeconds, Treadmill); break;
		case EDCStationType::Cannons: TickCannons(DeltaSeconds, Treadmill, Station->Side); break;
		default: TickTroubles(DeltaSeconds, Station); break;
		}
	}
}

void ADCCrewMember::TickHelm(float DeltaSeconds, ADCTreadmill* Treadmill)
{
	if (HelmOrderTimeLeft > 0.f)
	{
		HelmOrderTimeLeft -= DeltaSeconds;
		Treadmill->SetSteer(HelmOrder == EDCAction::TurnPort ? -1.f : 1.f);
		return;
	}

	// Autopilot: pick the clearest water within sight and ease toward it. In thick fog the helmsman
	// sees less far ahead, so the captain and the lookout matter more.
	const ADCRideDirector* Director = ADCRideDirector::Get(this);
	const float Sight = Director ? FMath::Clamp(1600.f / FMath::Max(Director->GetEnv().FogDensity, 0.01f), 9000.f, 20000.f) : 15000.f;
	const float Target = Treadmill->FindClearLane(Sight);
	Treadmill->SetSteer(FMath::Clamp((Target - Treadmill->GetLaneOffset()) / 250.f, -1.f, 1.f));
}

void ADCCrewMember::TickCannons(float DeltaSeconds, ADCTreadmill* Treadmill, EDCSide Side)
{
	if (bHoldFire) { return; }
	ADCGunnery* Guns = ADCGunnery::Get(this);
	if (!Guns || !Guns->IsReady(Side)) { return; }

	const int32 Target = Treadmill->FindHostile(Side, 13000.f);
	if (Target == INDEX_NONE) { return; }
	Guns->FireBroadside(Side, Treadmill->GetObjectLocation(Target), 4);
}

// ---------------------------------------------------------------- Crew subsystem

void UDCCrewSubsystem::AddStation(EDCStationType Type, EDCSide Side, const FVector& Location)
{
	FDCStation& S = Stations.AddDefaulted_GetRef();
	S.Type = Type; S.Side = Side; S.Location = Location;
}

ADCCrewMember* UDCCrewSubsystem::SpawnCrew(const FString& Name, const FLinearColor& Color, const FVector& Location)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADCCrewMember* Member = GetWorld()->SpawnActor<ADCCrewMember>(Location + FVector(0.f, 0.f, 90.f), FRotator::ZeroRotator, Params);
	if (Member)
	{
		Member->Init(Name, Color);
		Crew.Add(Member);
	}
	return Member;
}

int32 UDCCrewSubsystem::FindStation(EDCStationType Type, EDCSide Side) const
{
	for (int32 i = 0; i < Stations.Num(); ++i)
	{
		if (Stations[i].Type == Type && (Side == EDCSide::None || Stations[i].Side == EDCSide::None || Stations[i].Side == Side))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UDCCrewSubsystem::Say(const FString& Speaker, const FString& Text)
{
	UE_LOG(LogDC, Log, TEXT("[%s] %s"), *Speaker, *Text);
	RecentLines.Add(FString::Printf(TEXT("%s: %s"), *Speaker, *Text));
	while (RecentLines.Num() > 5) { RecentLines.RemoveAt(0); }
	OnCrewLine.Broadcast(Speaker, Text);
}

ADCCrewMember* UDCCrewSubsystem::PickCrew(const FString& Name, int32 ForStation)
{
	if (!Name.IsEmpty())
	{
		for (ADCCrewMember* M : Crew)
		{
			if (M && M->CrewName.Equals(Name, ESearchCase::IgnoreCase)) { return M; }
		}
	}
	// Idle hands first, then anyone not on the helm, nearest wins.
	const FVector Target = Stations.IsValidIndex(ForStation) ? Stations[ForStation].Location : FVector::ZeroVector;
	ADCCrewMember* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (ADCCrewMember* M : Crew)
	{
		if (!M) { continue; }
		float ScoreValue = FVector::Dist(M->GetActorLocation(), Target);
		if (M->State != EDCCrewState::Idle)
		{
			const FDCStation* S = GetStation(M->StationIndex);
			ScoreValue += (S && S->Type == EDCStationType::Helm) ? 200000.f : 100000.f;
		}
		if (ScoreValue < BestScore) { BestScore = ScoreValue; Best = M; }
	}
	return Best;
}

ADCCrewMember* UDCCrewSubsystem::EnsureOperator(int32 StationIdx, const FString& PreferredName)
{
	FDCStation* S = GetStation(StationIdx);
	if (!S) { return nullptr; }
	if (S->bPlayerOperated)
	{
		if (Crew.Num() && Crew[0]) { Reply(Crew[0]->CrewName, TEXT("You have that station yourself, Captain!")); }
		return nullptr;
	}
	ADCCrewMember* Current = S->Operator.Get();
	if (Current && (PreferredName.IsEmpty() || Current->CrewName.Equals(PreferredName, ESearchCase::IgnoreCase)))
	{
		return Current;
	}
	ADCCrewMember* Picked = PickCrew(PreferredName, StationIdx);
	if (Picked && Picked != Current)
	{
		if (Current) { Current->AssignStation(INDEX_NONE); }
		Picked->AssignStation(StationIdx);
	}
	return Picked;
}

FString UDCCrewSubsystem::ExecuteOrder(const FString& Text)
{
	++OrdersGiven;
	TArray<FString> Names;
	for (ADCCrewMember* M : Crew) { if (M) { Names.Add(M->CrewName); } }
	const FDCIntent Intent = DCIntentParser::Parse(Text, Names);
	Say(TEXT("Captain"), Text);
	// With the language model up it reads every order (it copes with loose phrasing); the keyword
	// grammar is the instant fallback when it is not running or cannot make sense of the words.
	ADCVoice* Voice = ADCVoice::Get(this);
	// Short, clear orders (a helm call in a hurry) go straight through; anything wordier goes to the model.
	const bool bQuick = Intent.bValid && Text.Len() < 30;
	if (Voice && Voice->IsLLMReady() && !bQuick)
	{
		Voice->ResolveOrder(Text); // answers a moment later
	}
	else
	{
		ExecuteIntent(Intent);
	}
	return RecentLines.Num() ? RecentLines.Last() : FString();
}

void UDCCrewSubsystem::Reply(const FString& Speaker, const FString& Text)
{
	// With the voice loop running the line is rephrased in character and spoken; otherwise it is shown as-is.
	if (ADCVoice* Voice = ADCVoice::Get(this))
	{
		Voice->CrewReply(Speaker, Text, CurrentOrderText);
	}
	else
	{
		Say(Speaker, Text);
	}
}

void UDCCrewSubsystem::ExecuteIntent(const FDCIntent& I)

{
	CurrentOrderText = I.RawText;
	if (!I.bValid)
	{
		Reply(Crew.Num() && Crew[0] ? Crew[0]->CrewName : TEXT("Crew"), TEXT("Say again, Captain?"));
		return;
	}

	if (I.Action == EDCAction::Report)
	{
		ADCRideDirector* D = ADCRideDirector::Get(this);
		Reply(Crew.Num() && Crew[0] ? Crew[0]->CrewName : TEXT("Crew"),
			FString::Printf(TEXT("Hull at %d percent, Captain."), D ? FMath::RoundToInt(D->ShipHealth) : 100));
		return;
	}


	switch (I.Station)
	{
	case EDCStationType::Helm:
	{
		ADCCrewMember* M = EnsureOperator(FindStation(EDCStationType::Helm, EDCSide::None), I.CrewName);
		if (!M) { return; }
		if (I.Action == EDCAction::TurnPort || I.Action == EDCAction::TurnStarboard)
		{
			M->SetHelmOrder(I.Action);
			Reply(M->CrewName, FString::Printf(TEXT("Hard to %s, aye!"), I.Action == EDCAction::TurnPort ? TEXT("port") : TEXT("starboard")));
		}
		else if (I.Action == EDCAction::Steady)
		{
			M->SetHelmOrder(EDCAction::Steady);
			Reply(M->CrewName, TEXT("Steady as she goes."));
		}
		else
		{
			Reply(M->CrewName, TEXT("Aye, I have the helm."));
		}
		break;
	}
	case EDCStationType::Cannons:
	{
		// No side given: both batteries if already manned, otherwise the starboard one.
		TArray<EDCSide> Sides;
		if (I.Side != EDCSide::None) { Sides.Add(I.Side); }
		else
		{
			for (const FDCStation& S : Stations)
			{
				if (S.Type == EDCStationType::Cannons && S.Operator.IsValid()) { Sides.Add(S.Side); }
			}
			if (!Sides.Num()) { Sides.Add(EDCSide::Starboard); }
		}
		for (EDCSide Side : Sides)
		{
			ADCCrewMember* M = EnsureOperator(FindStation(EDCStationType::Cannons, Side), Sides.Num() == 1 ? I.CrewName : FString());
			if (!M) { continue; }
			const TCHAR* S = Side == EDCSide::Port ? TEXT("Port") : TEXT("Starboard");
			if (I.Action == EDCAction::HoldFire)
			{
				M->SetHoldFire(true);
				Reply(M->CrewName, FString::Printf(TEXT("%s guns holding fire."), S));
			}
			else
			{
				M->SetHoldFire(false);
				Reply(M->CrewName, I.Action == EDCAction::Fire ? FString::Printf(TEXT("%s battery, firing!"), S) : FString::Printf(TEXT("%s guns manned and ready."), S));
			}
		}
		break;
	}
	case EDCStationType::Fire:
	case EDCStationType::Rigging:
	{
		ADCCrewMember* M = EnsureOperator(FindStation(I.Station, EDCSide::None), I.CrewName);
		if (M)
		{
			Reply(M->CrewName, I.Station == EDCStationType::Fire ? TEXT("Buckets! On it, Captain!") : TEXT("Aloft and mending, aye."));
		}
		break;
	}
	default:
		break;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GDCOrderCmd(
	TEXT("dc.Order"), TEXT("Give the crew an order in plain words, e.g. dc.Order hard to starboard"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UDCCrewSubsystem* Sub = World ? World->GetSubsystem<UDCCrewSubsystem>() : nullptr)
		{
			Sub->ExecuteOrder(FString::Join(Args, TEXT(" ")));
		}
	}));

void ADCCrewMember::SetWalking(bool bWalk)
{
	if (bWalking == bWalk || !Figure || !Figure->GetSkeletalMeshAsset()) { return; }
	bWalking = bWalk;
	if (UAnimationAsset* Anim = bWalk ? WalkAnim : IdleAnim)
	{
		Figure->PlayAnimation(Anim, true);
	}
}

void UDCCrewSubsystem::Report(const FString& Text, bool bUrgent)
{
	if (!Crew.Num()) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastReportTime < (bUrgent ? 1.5 : 7.0)) { return; }
	LastReportTime = Now;

	// Take turns, so the whole crew has a voice.
	ADCCrewMember* Speaker = Crew[NextReporter++ % Crew.Num()];
	if (!Speaker) { return; }
	if (ADCVoice* Voice = ADCVoice::Get(this))
	{
		Voice->CrewReply(Speaker->CrewName, Text, FString());
	}
	else
	{
		Say(Speaker->CrewName, Text);
	}
}

bool ADCCrewMember::MoveToward(const FVector& Target, float DeltaSeconds)
{
	const FVector Here = GetActorLocation();
	if (FVector::DistSquared(Here, Target) < 25.f)
	{
		SetWalking(false);
		return true;
	}
	SetActorLocation(FMath::VInterpConstantTo(Here, Target, DeltaSeconds, MoveSpeed));
	const FVector Dir = (Target - Here).GetSafeNormal2D();
	if (!Dir.IsNearlyZero()) { SetActorRotation(FRotator(0.f, Dir.Rotation().Yaw, 0.f)); }
	SetWalking(true);
	return false;
}

void ADCCrewMember::TickTroubles(float DeltaSeconds, FDCStation* Station)
{
	ADCTroubles* Troubles = ADCTroubles::Get(this);
	UDCCrewSubsystem* Sub = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	if (!Troubles || !Sub) { return; }

	if (Station->Type == EDCStationType::Rigging)
	{
		// Six seconds of hauling makes the sail fast again.
		if (Troubles->IsRiggingLoose() && Troubles->RepairRigging(DeltaSeconds / 6.f))
		{
			Sub->Report(TEXT("Sail is made fast again, Captain."), true);
		}
		return;
	}

	// Fire watch: go to the nearest fire, work on it, come back to the buckets when the deck is clear.
	FVector Fire;
	if (Troubles->NearestFire(GetActorLocation(), Fire))
	{
		FVector Away = (GetActorLocation() - Fire).GetSafeNormal2D();
		if (Away.IsNearlyZero()) { Away = FVector(-1.f, 0.f, 0.f); }
		if (MoveToward(Fire + Away * 120.f + FVector(0.f, 0.f, 90.f), DeltaSeconds))
		{
			SetActorRotation(FRotator(0.f, (-Away).Rotation().Yaw, 0.f));
			WorkTime += DeltaSeconds;
			if (WorkTime > 3.f)
			{
				WorkTime = 0.f;
				Troubles->ExtinguishNear(Fire);
				Sub->Report(Troubles->NumFires() ? TEXT("That one is out! More still burning!") : TEXT("Fire is out, Captain!"), true);
			}
		}
		else
		{
			WorkTime = 0.f;
		}
	}
	else
	{
		MoveToward(Station->Location + FVector(0.f, 0.f, 90.f), DeltaSeconds);
	}
}
