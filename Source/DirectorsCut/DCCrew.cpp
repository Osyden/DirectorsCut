#include "DCCrew.h"
#include "DCTreadmill.h"
#include "DCRideDirector.h"
#include "DCVoice.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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

	const bool bExtinguish = HasAny(T, { TEXT("put out"), TEXT("extinguish"), TEXT("douse"), TEXT("bucket"), TEXT("water"), TEXT("flames"), TEXT("burning") });
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
	else if (HasAny(T, { TEXT(" fire"), TEXT("shoot"), TEXT("blast"), TEXT("sink") }) || (bGunWord && !bManWord))
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
	else if (I.Side != EDCSide::None)
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
	Body->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.8f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Body);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(16.f);
	Label->SetUsingAbsoluteScale(true);
}

void ADCCrewMember::Init(const FString& InName, const FLinearColor& Color)
{
	CrewName = InName;
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
		const FVector NewLoc = FMath::VInterpConstantTo(GetActorLocation(), Target, DeltaSeconds, MoveSpeed);
		SetActorLocation(NewLoc);
		if (FVector::DistSquared(NewLoc, Target) < 25.f)
		{
			State = EDCCrewState::Operating;
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
		default: break;
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

	// Autopilot: steer away from the nearest obstacle ahead, otherwise hold course.
	float X = 0.f, Y = 0.f;
	if (Treadmill->FindObstacleAhead(14000.f, X, Y))
	{
		Treadmill->SetSteer(Y >= 0.f ? -1.f : 1.f);
	}
	else
	{
		Treadmill->SetSteer(0.f);
	}
}

void ADCCrewMember::TickCannons(float DeltaSeconds, ADCTreadmill* Treadmill, EDCSide Side)
{
	ReloadLeft -= DeltaSeconds;
	if (bHoldFire || ReloadLeft > 0.f) { return; }

	const int32 Target = Treadmill->FindHostile(Side, 12000.f);
	if (Target == INDEX_NONE) { return; }

	ReloadLeft = 3.f;
	const FVector From = GetActorLocation() + FVector(0.f, Side == EDCSide::Port ? -250.f : 250.f, 0.f);
	const FVector To = Treadmill->GetObjectLocation(Target);
	DrawDebugLine(GetWorld(), From, To, FColor(255, 180, 60), false, 0.25f, 0, 12.f);
	DrawDebugSphere(GetWorld(), To, 250.f, 12, FColor::Orange, false, 0.4f);
	Treadmill->DamageObject(Target, 1.f);
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
	TArray<FString> Names;
	for (ADCCrewMember* M : Crew) { if (M) { Names.Add(M->CrewName); } }
	const FDCIntent Intent = DCIntentParser::Parse(Text, Names);
	Say(TEXT("Captain"), Text);
	ExecuteIntent(Intent);
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
