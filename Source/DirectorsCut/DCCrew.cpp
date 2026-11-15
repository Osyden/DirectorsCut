#include "DCCrew.h"
#include "DCSpeech.h"
#include "DCTreadmill.h"
#include "DCRideDirector.h"
#include "DCVoice.h"
#include "DCGunnery.h"
#include "DCTroubles.h"
#include "DCCrewWorkAnim.h"
#include "DCCrewWorkPolicy.h"
#include "ProceduralMeshComponent.h"
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
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// ---------------------------------------------------------------- Intent parser

namespace
{
	bool HasAny(const FString& Text, std::initializer_list<const TCHAR*> Words)
	{
		for (const TCHAR* W : Words)
		{
			if (Text.Contains(TEXT(" ") + FString(W).TrimStartAndEnd() + TEXT(" "))) { return true; }
		}
		return false;
	}
}

FDCIntent DCIntentParser::Parse(const FString& InText, const TArray<FString>& CrewNames)
{
	FDCIntent I;
	I.RawText = InText;
	FString Normalized = DCSpeech::NormalizeCrewAddress(InText).ToLower().Replace(TEXT("don't"), TEXT("do not")).Replace(TEXT("don’t"), TEXT("do not"));
	for (TCHAR& C : Normalized) { if (!FChar::IsAlnum(C)) { C = TEXT(' '); } }
	TArray<FString> Tokens;
	Normalized.ParseIntoArrayWS(Tokens);
	const FString T = TEXT(" ") + FString::Join(Tokens, TEXT(" ")) + TEXT(" ");

	for (const FString& Name : CrewNames)
	{
		if (T.Contains(TEXT(" ") + Name.ToLower() + TEXT(" "))) { I.CrewName = Name; break; }
	}

	if (HasAny(T, { TEXT("starboard"), TEXT(" right") })) { I.Side = EDCSide::Starboard; }
	else if (HasAny(T, { TEXT(" port"), TEXT("larboard"), TEXT(" left") })) { I.Side = EDCSide::Port; }

	const bool bExtinguish = HasAny(T, { TEXT("put out"), TEXT("extinguish"), TEXT("douse"), TEXT("bucket"), TEXT("swat"), TEXT("smother"), TEXT("flames"), TEXT("burning"), TEXT("on fire") });
	const bool bHelmWord = HasAny(T, { TEXT("helm"), TEXT("wheel"), TEXT("steer"), TEXT("rudder") });
	const bool bGunWord = HasAny(T, { TEXT("cannon"), TEXT("cannons"), TEXT("gun"), TEXT("guns"), TEXT("battery"), TEXT("broadside") });
	const bool bRigWord = HasAny(T, { TEXT("sail"), TEXT("sails"), TEXT("rigging"), TEXT("rope"), TEXT("ropes"), TEXT("mast"), TEXT("repair"), TEXT("mend"), TEXT("fix") });
	const bool bManWord = HasAny(T, { TEXT("man"), TEXT("take the"), TEXT("get on"), TEXT("get to"), TEXT("go to"), TEXT(" take ") });
	const bool bTurnWord = HasAny(T, { TEXT("hard to"), TEXT("hard a"), TEXT("turn"), TEXT("come about"), TEXT("bear"), TEXT("veer"), TEXT("evade"), TEXT("dodge") });

	if (HasAny(T, { TEXT("report"), TEXT("status"), TEXT("how are we") }))
	{
		I.Action = EDCAction::Report; I.Side = EDCSide::None;
	}
	else if (HasAny(T, { TEXT("hold fire"), TEXT("cease"), TEXT("stop firing"), TEXT("hold your fire"), TEXT("do not fire"), TEXT("do not shoot") }))
	{
		I.Station = EDCStationType::Cannons; I.Action = EDCAction::HoldFire;
	}
	else if (bExtinguish)
	{
		I.Station = EDCStationType::Fire; I.Action = EDCAction::Extinguish;
	}
	else if (HasAny(T, { TEXT("fire at will") }))
	{
		I.Station = EDCStationType::Cannons; I.Action = EDCAction::Fire;
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
	const bool bFemale = InName == TEXT("Ruby");
	const TCHAR* Dir = TEXT("/Game/Brigantine/Demo/Characters/Mannequins");
	USkeletalMesh* SkelMesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s/Meshes/%s"), Dir, bFemale ? TEXT("SKM_Quinn_Simple.SKM_Quinn_Simple") : TEXT("SKM_Manny_Simple.SKM_Manny_Simple")));
	IdleAnim = LoadObject<UAnimationAsset>(nullptr, *FString::Printf(TEXT("%s/Animations/%s"), Dir, bFemale ? TEXT("Quinn/MF_Idle.MF_Idle") : TEXT("Manny/MM_Idle.MM_Idle")));
	WalkAnim = LoadObject<UAnimationAsset>(nullptr, *FString::Printf(TEXT("%s/Animations/%s"), Dir, bFemale ? TEXT("Quinn/MF_Walk_Fwd.MF_Walk_Fwd") : TEXT("Manny/MM_Walk_Fwd.MM_Walk_Fwd")));
	if (SkelMesh)
	{
		Figure->SetSkeletalMeshAsset(SkelMesh);
		Figure->SetAnimInstanceClass(UDCCrewWorkAnim::StaticClass());
		Figure->AddTickPrerequisiteActor(this);
		if (auto* Anim = Cast<UDCCrewWorkAnim>(Figure->GetAnimInstance())) { Anim->SetAnimationAsset(IdleAnim,true,1.f); }
		UE_LOG(LogDC,Log,TEXT("CREW_WORK_DRIVER name=%s anim=%s"),*CrewName,*GetNameSafe(Figure->GetAnimInstance()));
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
	CancelFireWork();
	WalkRoute.Reset(); RoutePoint=0; RouteTarget=FVector(FLT_MAX); RouteRetry=0.f;
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
		if (New->Type == EDCStationType::Cannons && !New->bPlayerGun)
		{
			for (int32 I = 0; I < Sub->GetStations().Num(); ++I)
			{
				FDCStation* Previous = Sub->GetStation(I);
				if (I != NewStation && Previous->AssignedGunner.Get() == this) { Previous->AssignedGunner.Reset(); }
			}
			New->AssignedGunner = this;
		}
		New->Operator = this;
		UE_LOG(LogDC, Log, TEXT("Crew station: %s station=%d gun=%d playerGun=%d"), *CrewName, NewStation, New->GunIndex, New->bPlayerGun);
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

void ADCCrewMember::VacateStation(const FVector& StationLocation)
{
	AssignStation(INDEX_NONE);
	// Stand aft and to port: the normal wheel view and the starboard clear-view
	// camera both stay free. Assignment alone left the idle body at the wheel.
	DeckPos = StationLocation + FVector(-250.f,-230.f,90.f);
	if (auto* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		FVector Feet;
		if (Crew->ProjectToDeck(DeckPos-FVector(0,0,90),Feet)) { DeckPos=Feet+FVector(0,0,90); }
	}
	DeckRot = FRotator::ZeroRotator;
	SetWalking(false);
	const ADCRideDirector* Director = ADCRideDirector::Get(this);
	const FTransform Deck = Director ? Director->DeckTransform : FTransform::Identity;
	SetActorLocationAndRotation(Deck.TransformPosition(DeckPos), Deck.GetRotation());
	LastPlaced = GetActorLocation();
	UE_LOG(LogDC, Log, TEXT("Crew cleared player station: %s deck=%s"), *CrewName, *DeckPos.ToCompactString());
}

void ADCCrewMember::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const ADCRideDirector* Dir = ADCRideDirector::Get(this);
	const FTransform Deck = Dir ? Dir->DeckTransform : FTransform::Identity;
	// Placed by someone else since last frame (spawned, or sent to a new post)? Take that as the new place on deck.
	if (!GetActorLocation().Equals(LastPlaced, 0.05f))
	{
		DeckPos = Deck.InverseTransformPosition(GetActorLocation());
		DeckRot = FRotator(0.f, (Deck.GetRotation().Inverse()*GetActorQuat()).Rotator().Yaw, 0.f);
		bDeckGrounded=false;
		WalkRoute.Reset(); RouteTarget=FVector(FLT_MAX);
	}
	// SpawnCrew precedes Super::StartPlay; asynchronous ship collision may not
	// exist at that point. Retry initial placement when the deck becomes queryable.
	if (!bDeckGrounded)
	{
		if (auto* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			FVector Feet;
			if (Crew->ProjectToDeck(DeckPos-FVector(0,0,90),Feet))
			{
				DeckPos=Feet+FVector(0,0,90); bDeckGrounded=true;
				UE_LOG(LogDC,Log,TEXT("CREW_GROUNDED name=%s feet=%s"),*CrewName,*Feet.ToCompactString());
			}
		}
	}
	TickCrew(DeltaSeconds);
	SetActorLocationAndRotation(Deck.TransformPosition(DeckPos), Deck.GetRotation() * DeckRot.Quaternion());
	UpdateWorkRag();
	LastPlaced = GetActorLocation();
}

void ADCCrewMember::TickCrew(float DeltaSeconds)
{

	// Face the name tag at the player.
	if (APlayerCameraManager* Cam = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->PlayerCameraManager : nullptr)
	{
		const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}

	UDCCrewSubsystem* Sub = GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	FDCStation* Station = Sub ? Sub->GetStation(StationIndex) : nullptr;
	if (Station && Station->bPlayerOperated)
	{
		VacateStation(Station->Location);
		return;
	}
	if (!Station)
	{
		CancelFireWork();
		State = EDCCrewState::Idle;
		SetWalking(false);
		return;
	}

	if (State == EDCCrewState::Moving)
	{
		const FVector Target = Station->Location + FVector(0.f, 0.f, 90.f);
		if (MoveToward(Target,DeltaSeconds))
		{
			State = EDCCrewState::Operating;
			// Face the work: the helmsman looks forward, gunners look out over their rail.
			const float Yaw = Station->Type == EDCStationType::Cannons ? (Station->Side == EDCSide::Port ? -90.f : 90.f) : 0.f;
			DeckRot = FRotator(0.f, Yaw, 0.f);
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
	// He steers for where she will be in a second, not where she is: she carries her swing, and with the wheel thrown
	// hard over for every 2.5 m of error she swung past each gap and on to the stone beyond it (seen in the log, 14 Nov).
	const float Carried = Treadmill->GetLateralVelocity() * 0.7f;
	Treadmill->SetSteer(FMath::Clamp((Target - Treadmill->GetLaneOffset() - Carried) / 700.f, -1.f, 1.f));
}

void ADCCrewMember::TickCannons(float DeltaSeconds, ADCTreadmill* Treadmill, EDCSide Side)
{
	if (bHoldFire) { return; }
	ADCGunnery* Guns = ADCGunnery::Get(this);
	const FDCStation* Station = GetWorld()->GetSubsystem<UDCCrewSubsystem>()->GetStation(StationIndex);
	if (!Guns || !Station || !Guns->IsGunReady(Station->GunIndex)) { return; }

	// 200 m (was 130): the creature's young come in at 13 m a second, and at 130 m a battery had time for one broadside.
	FVector Target;
	if (Guns->FindCrewTarget(Station->GunIndex, Target)) { Guns->FireGun(Station->GunIndex, Target); }
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
	FVector Feet=Location;
	ProjectToDeck(Location,Feet);
	const ADCRideDirector* Director=ADCRideDirector::Get(this);
	const FTransform Deck=Director ? Director->DeckTransform : FTransform::Identity;
	ADCCrewMember* Member = GetWorld()->SpawnActor<ADCCrewMember>(Deck.TransformPosition(Feet + FVector(0.f, 0.f, 90.f)), Deck.Rotator(), Params);
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

int32 UDCCrewSubsystem::FindCrewGun(EDCSide Side, const FString& Name) const
{
	// Keep each hand at their gun; new hands take a vacant gun, never evict another.
	for (int32 I = 0; I < Stations.Num(); ++I)
	{
		const FDCStation& S = Stations[I];
		if (S.Type != EDCStationType::Cannons || S.Side != Side || S.bPlayerGun) { continue; }
		if (S.AssignedGunner.IsValid() && !Name.IsEmpty() && S.AssignedGunner->CrewName.Equals(Name, ESearchCase::IgnoreCase)) { return I; }
	}
	for (int32 I = 0; I < Stations.Num(); ++I)
	{
		const FDCStation& S = Stations[I];
		if (S.Type == EDCStationType::Cannons && S.Side == Side && !S.bPlayerGun && !S.Operator.IsValid() && !S.AssignedGunner.IsValid()) { return I; }
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
		Voice->ResolveOrder(DCSpeech::NormalizeCrewAddress(Text)); // answers a moment later
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
		ADCCrewMember* Reporter = I.CrewName.IsEmpty() ? (Crew.Num() ? Crew[0].Get() : nullptr) : PickCrew(I.CrewName, INDEX_NONE);
		Reply(Reporter ? Reporter->CrewName : TEXT("Crew"),
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
		else if (!I.CrewName.IsEmpty())
		{
			for (const FDCStation& S : Stations)
			{
				if (S.Type == EDCStationType::Cannons && S.AssignedGunner.IsValid()
					&& S.AssignedGunner->CrewName.Equals(I.CrewName, ESearchCase::IgnoreCase))
				{ Sides.Add(S.Side); break; }
			}
			if (!Sides.Num()) { Sides.Add(EDCSide::Starboard); }
		}
		else
		{
			for (const FDCStation& S : Stations)
			{
				if (S.Type == EDCStationType::Cannons && S.Operator.IsValid()) { Sides.AddUnique(S.Side); }
			}
			if (!Sides.Num()) { Sides.Add(EDCSide::Starboard); }
		}
		for (EDCSide Side : Sides)
		{
			// An unnamed order applies to every hand already on that side.
			bool bOrdered = false;
			if (I.CrewName.IsEmpty())
			{
				for (FDCStation& Station : Stations)
				{
					if (Station.Type == EDCStationType::Cannons && Station.Side == Side && Station.Operator.IsValid())
					{ Station.Operator->SetHoldFire(I.Action == EDCAction::HoldFire); bOrdered = true; }
				}
			}
			if (bOrdered) { Reply(TEXT("Crew"), I.Action == EDCAction::HoldFire ? TEXT("Holding fire, Captain.") : TEXT("Guns ready, Captain.")); continue; }
			ADCCrewMember* M = EnsureOperator(FindCrewGun(Side, I.CrewName), I.CrewName);
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
			Reply(M->CrewName, I.Station == EDCStationType::Fire ? TEXT("Smothering the flames, Captain!") : TEXT("At the repair lines, aye."));
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
	if (bWalk) { CancelFireWork(); }
	if (bWalking == bWalk || !Figure || !Figure->GetSkeletalMeshAsset()) { return; }
	bWalking = bWalk;
	if (UAnimationAsset* Anim = bWalk ? WalkAnim : IdleAnim)
	{
		if (auto* WorkAnim = Cast<UDCCrewWorkAnim>(Figure->GetAnimInstance())) { WorkAnim->SetAnimationAsset(Anim,true,1.f); }
		Figure->SetPlayRate(1.f);
	}
}

void UDCCrewSubsystem::Report(const FString& Text, bool bUrgent)
{
	if (!Crew.Num()) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (const ADCRideDirector* Director = ADCRideDirector::Get(this); Director && Director->GetPhase() != LastReportPhase)
	{
		LastReportPhase = Director->GetPhase();
		LastReportTime = -100.0; // The preceding scene cannot suppress the new phase's call.
	}
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
	UDCCrewSubsystem* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>();
	if (!Crew) { return false; }
	RouteRetry=FMath::Max(0.f,RouteRetry-DeltaSeconds);
	const bool bNewGoal=!Target.Equals(RouteTarget,65.f);
	if (bNewGoal || (WalkRoute.IsEmpty() && RouteRetry<=0.f))
	{
		RouteTarget=Target; RoutePoint=0; RouteRetry=1.f;
		if (!Crew->FindDeckRoute(DeckPos-FVector(0,0,90),Target-FVector(0,0,90),WalkRoute))
		{
			// Never fall back to the old straight line through a wall or a cabin.
			if (bNewGoal) { UE_LOG(LogDC,Warning,TEXT("CREW_ROUTE_BLOCKED name=%s from=%s target=%s"),*CrewName,*DeckPos.ToCompactString(),*Target.ToCompactString()); }
			SetWalking(false); WalkingSpeed=0.f; return false;
		}
		UE_LOG(LogDC,Log,TEXT("CREW_ROUTE name=%s points=%d start=%s end=%s"),*CrewName,WalkRoute.Num(),*WalkRoute[0].ToCompactString(),*WalkRoute.Last().ToCompactString());
	}
	while (WalkRoute.IsValidIndex(RoutePoint) && FVector::DistSquared2D(DeckPos,WalkRoute[RoutePoint])<4.f) { ++RoutePoint; }
	if (!WalkRoute.IsValidIndex(RoutePoint))
	{
		SetWalking(false); WalkingSpeed=0.f;
		return !WalkRoute.IsEmpty();
	}
	const FVector Before=DeckPos;
	const FVector Waypoint=WalkRoute[RoutePoint]+FVector(0,0,90);
	const FVector Direction=(Waypoint-DeckPos).GetSafeNormal2D();
	const float DesiredYaw=Direction.Rotation().Yaw;
	DeckRot.Yaw=FMath::FixedTurn(DeckRot.Yaw,DesiredYaw,300.f*DeltaSeconds);
	const float Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(DeckRot.Yaw,DesiredYaw));
	const float FinalDistance=FVector::Dist2D(DeckPos,WalkRoute.Last());
	const float DesiredSpeed=FMath::Min(MoveSpeed,FMath::Sqrt(FMath::Max(1.f,FinalDistance)*600.f))*FMath::Clamp(1.f-Turn/100.f,0.f,1.f);
	WalkingSpeed=FMath::FInterpConstantTo(WalkingSpeed,DesiredSpeed,DeltaSeconds,400.f);
	float Travel=WalkingSpeed*DeltaSeconds;
	while (Travel>0.f && WalkRoute.IsValidIndex(RoutePoint))
	{
		const FVector Next=WalkRoute[RoutePoint]+FVector(0,0,90);
		const float Distance=FVector::Dist2D(DeckPos,Next);
		if (Distance<=Travel)
		{
			DeckPos=Next; Travel-=Distance; ++RoutePoint;
		}
		else { DeckPos=FMath::Lerp(DeckPos,Next,Travel/Distance); Travel=0.f; }
	}
	FVector Feet=DeckPos-FVector(0,0,90);
	Crew->GroundFeet(Feet); DeckPos=Feet+FVector(0,0,90);
	SetWalking(FVector::DistSquared2D(Before,DeckPos)>.01f);
	if (bWalking && Figure) { Figure->SetPlayRate(FMath::Clamp(WalkingSpeed/MoveSpeed,.35f,1.05f)); }
	return RoutePoint>=WalkRoute.Num();
}

bool UDCCrewSubsystem::EnsureNavigation()
{
	if (bNavigationAttempted) { return !Navigation.Nodes.IsEmpty(); }
	if (!GetWorld()->HasBegunPlay()) { return false; }
	auto* Director=ADCRideDirector::Get(this);
	if (!Director || !Director->ShipActor) { return false; }
	const double Now=GetWorld()->GetRealTimeSeconds();
	if (Now<NextNavigationAttempt) { return false; }
	NextNavigationAttempt=Now+.5;
	const double Started=FPlatformTime::Seconds();
	const bool Built=Navigation.Build(Director);
	bNavigationAttempted=Built; // An empty early query is retryable, never permanent.
	UE_LOG(LogDC,Log,TEXT("CREW_DECK_GRAPH nodes=%d ready=%d buildMs=%.1f"),Navigation.Nodes.Num(),Built,(FPlatformTime::Seconds()-Started)*1000.0);
	if (Built && FParse::Param(FCommandLine::Get(),TEXT("DCCrewRouteVerify")))
	{
		int32 Passed=0, Failed=0;
		for (int32 A=0; A<Stations.Num(); ++A)
		{
			FVector Projected=FVector::ZeroVector;
			const bool Reachable=Navigation.Project(Stations[A].Location,Projected);
			UE_LOG(LogDC,Log,TEXT("CREW_STATION index=%d type=%d raw=%s projected=%d feet=%s"),A,int32(Stations[A].Type),*Stations[A].Location.ToCompactString(),Reachable,*Projected.ToCompactString());
			for (int32 B=A+1; B<Stations.Num(); ++B)
			{
				TArray<FVector> Route;
				if (Navigation.FindRoute(Stations[A].Location,Stations[B].Location,Route)) { ++Passed; }
				else { ++Failed; UE_LOG(LogDC,Error,TEXT("CREW_ROUTE_VERIFY unreachable stations=%d,%d"),A,B); }
			}
		}
		UE_LOG(LogDC,Display,TEXT("CREW_ROUTE_VERIFY passed=%d failed=%d"),Passed,Failed);
	}
	return Built;
}

bool UDCCrewSubsystem::ProjectToDeck(const FVector& Feet, FVector& OnDeck)
{
	return EnsureNavigation() && Navigation.Project(Feet,OnDeck);
}

bool UDCCrewSubsystem::FindDeckRoute(const FVector& FromFeet, const FVector& ToFeet, TArray<FVector>& Route)
{
	return EnsureNavigation() && Navigation.FindRoute(FromFeet,ToFeet,Route);
}

bool UDCCrewSubsystem::GroundFeet(FVector& Feet)
{
	return EnsureNavigation() && Navigation.Ground(ADCRideDirector::Get(this),Feet);
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

	// Three visible rag strokes use the same retained suppression as the captain.
	// A reassignment, vanished fire or lost reach cancels before another contact.
	if (CrewSwatAge < DCCrewWorkPolicy::Duration)
	{
		if ((!bCrewSwatContact && Troubles->SwatsRemaining(CrewSwatTarget,1.f)==0) || FVector::Dist2D(DeckPos,CrewSwatTarget)>240.f)
		{
			CancelFireWork(); return;
		}
		const float Before=CrewSwatAge;
		CrewSwatAge+=DeltaSeconds;
		if (auto* Anim=Cast<UDCCrewWorkAnim>(Figure->GetAnimInstance())) { Anim->SwatAge=CrewSwatAge; }
		if (DCCrewWorkPolicy::CrossedContact(Before,CrewSwatAge))
		{
			bCrewSwatContact=true;
			Troubles->SwatNear(CrewSwatTarget,1.f);
			UE_LOG(LogDC,Log,TEXT("CREW_SWAT name=%s remaining=%d at=%s"),*CrewName,Troubles->SwatsRemaining(CrewSwatTarget,1.f),*CrewSwatTarget.ToCompactString());
			if (Troubles->SwatsRemaining(CrewSwatTarget,1.f)==0)
			{
				Sub->Report(Troubles->NumFires() ? TEXT("That one is out! More still burning!") : TEXT("Fire is out, Captain!"),true);
			}
		}
		return;
	}
	CancelFireWork();
	FVector Fire;
	if (Troubles->NearestFire(DeckPos, Fire))
	{
		FVector Away = (DeckPos - Fire).GetSafeNormal2D();
		if (Away.IsNearlyZero()) { Away = FVector(-1.f, 0.f, 0.f); }
		if (MoveToward(Fire + Away * 120.f + FVector(0.f, 0.f, 90.f), DeltaSeconds))
		{
			SetWalking(false);
			DeckRot = FRotator(0.f, (-Away).Rotation().Yaw, 0.f);
			CrewSwatTarget=Fire; CrewSwatAge=0.f; bCrewSwatContact=false;
			if (auto* Anim=Cast<UDCCrewWorkAnim>(Figure->GetAnimInstance())) { Anim->SwatAge=0.f; }
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

void ADCCrewMember::CancelFireWork()
{
	CrewSwatAge=10.f; WorkTime=0.f; bCrewSwatContact=false;
	if (Figure) { if (auto* Anim=Cast<UDCCrewWorkAnim>(Figure->GetAnimInstance())) { Anim->SwatAge=10.f; } }
	if (WorkRag) { WorkRag->SetVisibility(false); }
}

void ADCCrewMember::UpdateWorkRag()
{
	const bool Active=CrewSwatAge<DCCrewWorkPolicy::Duration;
	if (!Active) { if (WorkRag) { WorkRag->SetVisibility(false); } return; }
	if (!WorkRag)
	{
		WorkRag=NewObject<UProceduralMeshComponent>(this,TEXT("CrewWorkRag"));
		WorkRag->SetupAttachment(Figure,TEXT("hand_r"));
		// Follow the evaluated hand socket in this frame; keep cloth hanging in
		// deck orientation rather than inheriting the wrist's twist.
		WorkRag->SetAbsolute(false,true,true);
		WorkRag->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorkRag->SetCastShadow(false);
		WorkRag->SetCanEverAffectNavigation(false);
		WorkRag->RegisterComponent();
		if (auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_Stone.M_Stone")))
		{
			auto* Mat=UMaterialInstanceDynamic::Create(Base,this);
			Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.32f,.24f,.14f));
			Mat->SetVectorParameterValue(TEXT("Glow"),FLinearColor::Black);
			WorkRag->SetMaterial(0,Mat);
		}
	}
	WorkRag->SetVisibility(true);
	WorkRag->SetRelativeLocation(FVector::ZeroVector);
	WorkRag->SetWorldRotation(GetActorQuat());
	TArray<FVector> V; TArray<int32> Tri; TArray<FVector2D> UV;
	for (int32 Row=0;Row<5;++Row) for (int32 Col=0;Col<3;++Col)
	{
		const float T=Row/4.f, Side=Col-1.f;
		V.Add(FVector(T*15.f+FMath::Sin(T*8.f-CrewSwatAge*10.f)*T*4.f,Side*(2.f+12.f*T),-T*48.f));
		UV.Add(FVector2D(Col/2.f,T));
	}
	for (int32 R=0;R<4;++R) for (int32 C=0;C<2;++C)
	{
		const int32 N=R*3+C;
		Tri.Append({N,N+3,N+1,N+1,N+3,N+4,N+1,N+3,N,N+4,N+3,N+1});
	}
	if (WorkRag->GetProcMeshSection(0)) { WorkRag->UpdateMeshSection_LinearColor(0,V,TArray<FVector>(),UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>()); }
	else { WorkRag->CreateMeshSection_LinearColor(0,V,Tri,TArray<FVector>(),UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false); }
}
