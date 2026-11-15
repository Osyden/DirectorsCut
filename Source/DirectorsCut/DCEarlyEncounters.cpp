#include "DCEarlyEncounters.h"
#include "DCFogCreatureMotion.h"
#include "DCCombatCue.h"
#include "DCCombatCuePolicy.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "DCGunnery.h"
#include "DCTroubles.h"
#include "DCCrew.h"
#include "DCRuins.h"
#include "DCHazards.h"
#include "DCDeckFireEffect.h"
#include "DCAudio.h"
#include "Animation/SkeletalMeshActor.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	constexpr float GroupTimes[] = {90.f, 132.f, 178.f};
	constexpr float StatueTimes[] = {112.f, 158.f, 208.f};
	UAnimSequence* Anim(const TCHAR* Name)
	{
		return LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	AActor* Empty(UWorld* World, const FVector& Position)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Position, FRotator::ZeroRotator, Params);
		if (!Actor) { return nullptr; }
		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Root->SetMobility(EComponentMobility::Movable);
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		Actor->SetActorLocation(Position);
		return Actor;
	}
	UMaterialInstanceDynamic* Glow(UObject* Owner, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Glow.M_Glow"));
		if (!Base) { return nullptr; }
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Owner);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		return MID;
	}
	UStaticMeshComponent* Shape(AActor* Owner, const TCHAR* Mesh, FLinearColor Color)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
		Part->SetupAttachment(Owner->GetRootComponent());
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Mesh));
		Part->SetMaterial(0, Glow(Owner, Color));
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->RegisterComponent();
		return Part;
	}
	void RetireFireball(AActor* Orb)
	{
		if (!Orb) { return; }
		TArray<AActor*> Children; Orb->GetAttachedActors(Children);
		for (AActor* Child : Children)
		{
			if (ADCDeckFireEffect* F = Cast<ADCDeckFireEffect>(Child))
			{
				F->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				F->Extinguish();
			}
		}
		Orb->Destroy();
	}
}

ADCEarlyEncounters::ADCEarlyEncounters()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ADCEarlyEncounters* ADCEarlyEncounters::Get(const UObject* Context)
{
	if (UWorld* World = Context ? Context->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCEarlyEncounters> It(World); It; ++It) { return *It; }
	}
	return nullptr;
}

AActor* ADCEarlyEncounters::MakeTarget(const FVector& Position, float Radius, float Health)
{
	AActor* Target = Empty(GetWorld(), Position);
	if (!Target) { return nullptr; }
	Target->Tags.Add(TEXT("DCEarlyThreat"));
	USphereComponent* Hit = NewObject<USphereComponent>(Target);
	Hit->SetupAttachment(Target->GetRootComponent());
	Hit->SetSphereRadius(Radius * .6f);
	Hit->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Hit->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hit->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Hit->RegisterComponent();
	Treadmill->AddExternalHostile(Target, Radius, Health, true);
	return Target;
}

AActor* ADCEarlyEncounters::MakeFireball(const FVector& Position)
{
	AActor* Orb = Empty(GetWorld(), Position);
	if (!Orb) { return nullptr; }
	UStaticMeshComponent* Core = Shape(Orb, TEXT("/Engine/BasicShapes/Sphere.Sphere"), FLinearColor(3.5f, .72f, .045f));
	Core->SetRelativeScale3D(FVector(1.4f));
	FActorSpawnParameters Params;
	Params.Owner = Orb;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ADCDeckFireEffect* Flame = GetWorld()->SpawnActor<ADCDeckFireEffect>(Position, FRotator::ZeroRotator, Params))
	{
		Flame->Initialize(float(ImpactCount * 7 + Threats.Num()));
		Flame->SetStrength(.9f);
		Flame->SetActorScale3D(FVector(1.6f));
		Flame->AttachToActor(Orb, FAttachmentTransformRules::KeepWorldTransform);
	}
	return Orb;
}

AActor* ADCEarlyEncounters::MakeMarker(const FVector& DeckPoint)
{
	AActor* Marker = Empty(GetWorld(), Director->DeckToWorld(DeckPoint));
	if (!Marker) { return nullptr; }
	UProceduralMeshComponent* Ring = NewObject<UProceduralMeshComponent>(Marker);
	Ring->SetupAttachment(Marker->GetRootComponent());
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ring->SetCastShadow(false);
	Ring->RegisterComponent();
	TArray<FVector> V, N;
	TArray<FVector2D> UV;
	TArray<int32> Tri;
	for (int32 I = 0; I <= 48; ++I)
	{
		const float A = float(I) / 48.f * UE_TWO_PI;
		for (float Radius : {90.f, 100.f})
		{
			V.Add(FVector(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, 7.f));
			N.Add(FVector::UpVector); UV.Add(FVector2D(float(I) / 48.f, Radius == 90.f ? 0.f : 1.f));
		}
		if (I < 48) { const int32 B = I * 2; Tri.Append({B, B + 1, B + 2, B + 1, B + 3, B + 2}); }
	}
	Ring->CreateMeshSection(0, V, Tri, N, UV, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	Ring->SetMaterial(0, Glow(Marker, FLinearColor(2.5f, .45f, .025f)));
	return Marker;
}

void ADCEarlyEncounters::SpawnGroup(int32 Group)
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Skins/Tier2/Elemental/Meshes/Rampage_Elemental.Rampage_Elemental"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mesh) { UE_LOG(LogDC, Warning, TEXT("Early encounters: Rampage mesh unavailable")); return; }
	const float Side = Group == 1 ? 1.f : -1.f;
	const int32 Count = Group == 2 ? 3 : 2;
	for (int32 I = 0; I < Count; ++I)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASkeletalMeshActor* Body = GetWorld()->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (!Body) { continue; }
		USkeletalMeshComponent* C = Body->GetSkeletalMeshComponent();
		C->SetSkeletalMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		FThreat G;
		G.Body = Body; G.Size = 10.f + I * .8f; G.Side = Side;
		G.X = 23500.f + I * 3800.f;
		G.HoldX = 3000.f + I * 4600.f;
		G.SeaY = Treadmill->GetLaneOffset() + Side * (9000.f + I * 650.f);
		G.InitialWait = 8.f + I * 2.8f;
		G.bGroupLeader = I == 0;
		Body->SetActorScale3D(FVector(G.Size));
		G.GaitPhase = I * .23f;
		UpdateCreaturePose(G, 0.f, 0.f, 0.f);
		G.Target = MakeTarget(FVector(G.X, G.SeaY, Director->SeaLevelZ + G.Size * 175.f), 1050.f, 3.f);
		Threats.Add(G);
	}
	UE_LOG(LogDC, Display, TEXT("Encounter group %d: %d creatures on %s at %.1f"), Group + 1, Count, Side < 0 ? TEXT("port") : TEXT("starboard"), Director->RideTime);
}

void ADCEarlyEncounters::SpawnStatue(int32 Index)
{
	FThreat S;
	S.bStatue = true; S.Side = Index % 2 == 0 ? 1.f : -1.f; S.Size = 3600.f;
	S.X = 26000.f; S.SeaY = Treadmill->GetLaneOffset() + S.Side * 6800.f;
	S.InitialWait = 5.f;
	AActor* Body = DCRuins::Available() ? DCRuins::SpawnColossus(GetWorld(), S.Size)
		: DCHazards::SpawnSentinel(GetWorld(), S.Size, true, FLinearColor(.075f, .085f, .085f));
	if (!Body) { return; }
	S.Body = Body;
	for (int32 Eye = 0; Eye < 2; ++Eye)
	{
		// Use the actual statue mesh bounds, including its authored pivot and plinth.
		FVector EyePosition(180.f, Eye == 0 ? -75.f : 75.f, S.Size * .9f);
		TInlineComponentArray<UStaticMeshComponent*> Pieces(Body);
		for (UStaticMeshComponent* Piece : Pieces)
		{
			if (!Piece->GetStaticMesh() || Piece->GetStaticMesh()->GetName() != TEXT("SM_Statue_01")) { continue; }
			const FBox B = Piece->GetStaticMesh()->GetBoundingBox();
			// This sculpture raises one arm above its head; the eyes sit at 76% of
			// mesh height, not near the highest point of its overall bounds.
			const FVector P(B.GetCenter().X - B.GetSize().X * .12f, B.GetCenter().Y + (Eye ? .065f : -.065f) * B.GetSize().Y, B.Min.Z + B.GetSize().Z * .76f);
			EyePosition = Piece->GetComponentTransform().TransformPosition(P);
			break;
		}
		S.EyeLocal[Eye] = Body->GetActorTransform().InverseTransformPosition(EyePosition);
		UE_LOG(LogDC, Log, TEXT("Sentinel eye %d local=%s figure bounds=%s"), Eye, *S.EyeLocal[Eye].ToCompactString(), *Body->GetComponentsBoundingBox(true).GetSize().ToCompactString());
		UStaticMeshComponent* GlowEye = Shape(Body, TEXT("/Engine/BasicShapes/Sphere.Sphere"), FLinearColor(.4f, .12f, .015f));
		GlowEye->SetRelativeLocation(S.EyeLocal[Eye]);
		GlowEye->SetWorldScale3D(FVector(.35f, .6f, .24f));
		S.Eyes.Add(GlowEye);
		UStaticMeshComponent* Beam = Shape(Body, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FLinearColor(8.f, 2.8f, .22f));
		Beam->SetVisibility(false); S.Beams.Add(Beam);
	}
	S.Target = MakeTarget(FVector(S.X, S.SeaY, Director->SeaLevelZ + S.Size * .8f), 1500.f, 3.f);
	Threats.Add(S);
	UE_LOG(LogDC, Display, TEXT("Sentinel %d placed on %s at %.1f"), Index + 1, S.Side < 0 ? TEXT("port") : TEXT("starboard"), Director->RideTime);
}

FVector ADCEarlyEncounters::Hand(const FThreat& T) const
{
	if (ASkeletalMeshActor* A = Cast<ASkeletalMeshActor>(T.Body.Get()))
	{
		USkeletalMeshComponent* Mesh = A->GetSkeletalMeshComponent();
		if (Mesh->DoesSocketExist(TEXT("hand_r"))) { return Mesh->GetSocketLocation(TEXT("hand_r")); }
	}
	return T.Body.IsValid() ? T.Body->GetActorLocation() + FVector(0, 0, T.Size * 180.f) : FVector::ZeroVector;
}

UAnimSequence* ADCEarlyEncounters::CreatureClip(FName Name)
{
	if (const TObjectPtr<UAnimSequence>* Found = CreatureClips.Find(Name)) { return Found->Get(); }
	UAnimSequence* Clip = Anim(*Name.ToString());
	CreatureClips.Add(Name, Clip);
	if (!Clip) { UE_LOG(LogDC, Warning, TEXT("Fog creature animation missing: %s"), *Name.ToString()); }
	return Clip;
}

void ADCEarlyEncounters::UpdateCreaturePose(FThreat& T, float Dt, float GroundSpeed, float Facing)
{
	ASkeletalMeshActor* Actor = Cast<ASkeletalMeshActor>(T.Body.Get());
	if (!Actor || T.bStatue) { return; }
	using namespace DCFogCreatureMotion;
	USkeletalMeshComponent* Mesh = Actor->GetSkeletalMeshComponent();
	UAnimSequence* Rip = CreatureClip(TEXT("Ability_RipNToss_Rip"));
	UAnimSequence* Toss = CreatureClip(TEXT("Ability_RipNToss_Toss"));
	const FPose PoseState = Pose(T.Stage, T.Clock, Rip ? Rip->GetPlayLength() : 1.f, Toss ? Toss->GetPlayLength() : 1.f);
	const EDirection Travel = Direction(GroundSpeed, Facing);
	const bool bLoop = PoseState.Action == EAction::Travel || PoseState.Action == EAction::Hold;
	const bool bHolding = PoseState.Action == EAction::Hold;
	const TCHAR* Name = TEXT("Idle_Biped");
	if (PoseState.Action == EAction::Rip) { Name = TEXT("Ability_RipNToss_Rip"); }
	else if (PoseState.Action == EAction::Toss) { Name = TEXT("Ability_RipNToss_Toss"); }
	else if (PoseState.Action == EAction::Death) { Name = TEXT("Death_A"); }
	else
	{
		switch (Travel)
		{
		case EDirection::Forward: Name = bHolding ? TEXT("Ability_RipNToss_Jog_Fwd") : TEXT("Jog_Biped_Fwd"); break;
		case EDirection::Backward: Name = bHolding ? TEXT("Ability_RipNToss_Jog_Bwd") : TEXT("Jog_Biped_Bwd"); break;
		case EDirection::Left: Name = bHolding ? TEXT("Ability_RipNToss_Jog_Lft") : TEXT("Jog_Biped_Lft"); break;
		case EDirection::Right: Name = bHolding ? TEXT("Ability_RipNToss_Jog_Rgt") : TEXT("Jog_Biped_Rt"); break;
		default: Name = bHolding ? TEXT("Ability_RipNToss_Idle") : TEXT("Idle_Biped"); break;
		}
	}
	UAnimSequence* Clip = CreatureClip(Name);
	if (!Clip) { Name = TEXT("Idle_Biped"); Clip = CreatureClip(Name); }
	if (!Clip) { return; }
	const float Length = FMath::Max(.01f, Clip->GetPlayLength());
	const float PhaseRate = Travel == EDirection::Idle ? 1.f / Length : GaitCyclesPerSecond(GroundSpeed, T.Size);
	const float Rate = bLoop ? PhaseRate * Length : (PoseState.Action == EAction::Rip ? Length / FMath::Min(Length, 1.25f) : 1.f);
	// Keep normalized footfall phase when changing directional or carrying loops.
	// Never restart a loop every tick and never leave an ended one-shot holding a travelling actor.
	if (bLoop) { T.GaitPhase = FMath::Fmod(T.GaitPhase + Dt * PhaseRate, 1.f); }
	if (T.CurrentClip != FName(Name))
	{
		Mesh->PlayAnimation(Clip, bLoop);
		Mesh->SetComponentTickEnabled(false);
		if (UAnimSingleNodeInstance* Instance = Mesh->GetSingleNodeInstance()) { Instance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion); }
		T.CurrentClip = Name;
	}
	const float Position = bLoop ? T.GaitPhase * Length : FMath::Clamp(PoseState.Seconds, 0.f, Length);
	Mesh->SetPosition(Position, false);
	Mesh->TickAnimation(0.f, false);
	Mesh->RefreshBoneTransforms();
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("DCFogAnimLog")) && T.Age - T.LastAnimLog >= .5f)
	{
		T.LastAnimLog = T.Age;
		UE_LOG(LogDC, Display, TEXT("FogAnim body=%s ride=%.2f stage=%d clock=%.2f ground=%.1f facing=%.1f clip=%s pos=%.3f/%.3f rate=%.2f loop=%d"),
			*Actor->GetName(), Director->RideTime, T.Stage, T.Clock, GroundSpeed, Facing, Name, Position, Length, Rate, bLoop);
	}
#endif
}

void ADCEarlyEncounters::UpdateAlert(FThreat& T, bool bConcealed)
{
	AActor* Body = T.Body.Get();
	if (!Body) { return; }
	const bool bVisible = DCCombatCuePolicy::AlertVisible(T.Stage, T.Target.IsValid(), bConcealed, Body->IsHidden());
	if (!T.AlertCue.IsValid() && bVisible) { T.AlertCue = UDCCombatCueComponent::Create(Body, TEXT("AttackAlert")); }
	if (UDCCombatCueComponent* Cue = T.AlertCue.Get())
	{
		FVector Head = Body->GetActorLocation();
		if (ASkeletalMeshActor* Skeletal = Cast<ASkeletalMeshActor>(Body))
		{
			// Only the creature mesh: actor bounds would include the cue and make it climb itself.
			const FBoxSphereBounds Bounds = Skeletal->GetSkeletalMeshComponent()->Bounds;
			Head = Bounds.Origin + FVector(0, 0, Bounds.BoxExtent.Z + 150.f);
		}
		else { Head = Body->GetActorTransform().TransformPosition((T.EyeLocal[0] + T.EyeLocal[1]) * .5f) + FVector(0, 0, 650.f); }
		Cue->ShowAlert(Head, T.bStatue ? 350.f : 220.f, FMath::Clamp(T.Clock / (T.bStatue ? 3.5f : 3.f), 0.f, 1.f), T.Age, bVisible);
	}
}

void ADCEarlyEncounters::Ignite(const FVector& DeckPoint, const TCHAR* Cause)
{
#if !UE_BUILD_SHIPPING
	if (FCString::Strcmp(Cause, TEXT("fireball")) == 0) { ++VerifyFireballImpacts; }
	else { ++VerifyBeamImpacts; }
#endif
	const int32 Before = Troubles->NumFires();
	Troubles->StartFire(DeckPoint);
	Director->DamageShip(1.2f);
	++ImpactCount;
	if (ADCAudio* Sound = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(this, ADCAudio::StaticClass()))) { Sound->Boom(.45f); }
	UE_LOG(LogDC, Display, TEXT("Encounter impact: %s deck=%s fires=%d->%d at %.1f"), Cause, *DeckPoint.ToCompactString(), Before, Troubles->NumFires(), Director->RideTime);
}

void ADCEarlyEncounters::TickThreat(FThreat& T, float Dt)
{
	AActor* Body = T.Body.Get();
	if (!Body) { return; }
	T.Age += Dt; T.Clock += Dt;
	// Dt already includes the ride time scale. Scale movement exactly once so
	// accelerated reviews retain the same firing windows as normal play.
	const float Speed = Director->GetEnv().ShipSpeed;
	const float PreviousX = T.X;
	if (T.bStatue || T.Age > 34.f || T.Stage == 4) { T.X -= Speed * Dt; }
	else { T.X = FMath::FInterpConstantTo(T.X, T.HoldX, Dt, T.X > T.HoldX + 1800.f ? Speed : Speed * .25f); }
	const float GroundSpeed = DCFogCreatureMotion::GroundSpeed(Speed, PreviousX, T.X, Dt);
	const FVector Local(T.X, T.SeaY - Treadmill->GetLaneOffset(), Director->SeaLevelZ + Director->SeaRise(T.SeaY) - Director->SeaRise(Treadmill->GetLaneOffset()) - (T.bStatue ? 150.f : 260.f));
	const float Facing = T.bStatue ? (T.Side < 0.f ? -90.f : 90.f) : FVector(-Local.X, -Local.Y, 0).Rotation().Yaw;
	Body->SetActorLocationAndRotation(Director->GetWorldRock().TransformPosition(Local), FRotator(0, Facing + (T.bStatue ? 0.f : -90.f), 0));
	const bool bFogConcealed = Director->RideTime < 72.f || Local.X < Director->CloudTunnelFarX;
	Body->SetActorHiddenInGame(bFogConcealed);
	if (AActor* Target = T.Target.Get())
	{
		FVector Aim = Body->GetActorLocation() + FVector(0, 0, T.Size * (T.bStatue ? .8f : 175.f));
		if (T.bStatue) { Aim = Body->GetActorTransform().TransformPosition((T.EyeLocal[0] + T.EyeLocal[1]) * .5f); }
		const FVector Before = Target->GetActorLocation();
		Target->SetActorLocation(Aim); Target->SetActorHiddenInGame(bFogConcealed);
		Target->GetRootComponent()->ComponentVelocity = ((Aim - Before) / FMath::Max(Dt, .001f)).GetClampedToMaxSize(3500.f);
		Target->SetActorEnableCollision(!bFogConcealed);
	}
	if (!T.Target.IsValid() && T.Stage != 4)
	{
#if !UE_BUILD_SHIPPING
		if (Body == VerifyEnemy.Get() && T.Stage < 2) { bVerifyInterrupted = true; }
#endif
		T.Stage = 4; T.Clock = 0.f;
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->Report(T.bStatue ? TEXT("The eyes are dark! Statue silenced!") : TEXT("One down! Keep that battery firing!")); }
		RetireFireball(T.Charge.Get()); T.Charge.Reset();
		if (T.Marker.IsValid()) { T.Marker->Destroy(); }
		for (auto& Beam : T.Beams) { if (Beam.IsValid()) { Beam->SetVisibility(false); } }
		UE_LOG(LogDC, Display, TEXT("Encounter defeated: %s at %.1f"), T.bStatue ? TEXT("sentinel") : TEXT("creature"), Director->RideTime);
	}
	if (T.Stage == 4)
	{
		UpdateCreaturePose(T, Dt, GroundSpeed, Facing);
		UpdateAlert(T, bFogConcealed);
		// A silenced monument remains in the city with dark eyes. Creatures sink
		// fully below the water before their actors retire, avoiding a visible pop.
		if (!T.bStatue) { Body->AddActorWorldOffset(FVector(0, 0, -FMath::SmoothStep(.7f, 4.8f, T.Clock) * T.Size * 290.f)); }
		for (auto& Eye : T.Eyes) { if (Eye.IsValid()) { Eye->SetVisibility(false); } }
		return;
	}
	const EDCSide Side = T.Side < 0 ? EDCSide::Port : EDCSide::Starboard;
	const int32 PlayerGun = Guns->GetPlayerGun(Side);
	const FVector AimDelta = T.Target.IsValid() ? T.Target->GetActorLocation() - Guns->GetMuzzle(PlayerGun) : FVector::ZeroVector;
	const bool bCanShoot = !bFogConcealed && AimDelta.Z < AimDelta.Size2D() * .55f
		&& Guns->HasClearShot(PlayerGun, T.Target.Get());
	// A target passing behind masonry cannot keep burning the deck through it.
	// An already released fireball remains a physical, visible incoming threat.
	if (!bCanShoot && (T.Stage == 1 || (T.bStatue && T.Stage == 2)))
	{
		T.Stage = 3; T.Clock = 0.f;
		RetireFireball(T.Charge.Get()); T.Charge.Reset();
		if (T.Marker.IsValid()) { T.Marker->Destroy(); T.Marker.Reset(); }
		for (auto& Beam : T.Beams) { if (Beam.IsValid()) { Beam->SetVisibility(false); } }
		UE_LOG(LogDC, Log, TEXT("Encounter charge cancelled: firing window closed delta=%s clear=%d at %.1f"), *AimDelta.ToCompactString(), Guns->HasClearShot(PlayerGun, T.Target.Get()), Director->RideTime);
	}
	if (bCanShoot)
	{
		T.Exposure += Dt;
		if (!T.bAnnounced)
		{
			T.bAnnounced = true;
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>(); Crew && T.bGroupLeader)
			{
				Crew->Report(T.bStatue ? (T.Side < 0 ? TEXT("Eyes waking to port! Shoot the statue before it fires!") : TEXT("Eyes waking to starboard! Shoot the statue before it fires!"))
					: (T.Side < 0 ? TEXT("Raiders grouped off port! Port battery, clear them!") : TEXT("Raiders grouped off starboard! Starboard battery, clear them!")), true);
			}
			UE_LOG(LogDC, Display, TEXT("Encounter firing window: %s %s distance=%.0f at %.1f"), T.bStatue ? TEXT("sentinel") : TEXT("creature"), T.Side < 0 ? TEXT("port") : TEXT("starboard"), T.Target->GetActorLocation().Size(), Director->RideTime);
		}
	}
	if (T.Stage == 0 && T.Exposure >= T.InitialWait && bCanShoot && AttackSpacing <= 0.f)
	{
		if (Troubles->FindIgnitionPoint(ImpactCount + FMath::RoundToInt(T.SeaY / 100.f), T.DeckTarget))
		{
			T.Stage = 1; T.Clock = 0.f; T.bImpact = false; AttackSpacing = 4.f;
			T.Marker = MakeMarker(T.DeckTarget);
			if (!T.bStatue) { T.Charge = MakeFireball(Hand(T)); }
			UE_LOG(LogDC, Display, TEXT("Encounter charge: %s exposure=%.1f at %.1f"), T.bStatue ? TEXT("sentinel") : TEXT("creature"), T.Exposure, Director->RideTime);
		}
	}
	// Evaluate bones before following the held fireball or sampling its release position.
	UpdateCreaturePose(T, Dt, GroundSpeed, Facing);
	if (T.Marker.IsValid())
	{
		T.Marker->SetActorLocationAndRotation(Director->DeckToWorld(T.DeckTarget), Director->DeckTransform.GetRotation());
		T.Marker->SetActorScale3D(FVector(1.f + .08f * FMath::Sin(T.Clock * 8.f)));
	}
	if (T.Charge.IsValid()) { T.Charge->SetActorLocation(Hand(T)); }
	for (auto& Eye : T.Eyes)
	{
		if (Eye.IsValid())
		{
			const float Charge = T.Stage == 1 ? FMath::Clamp(T.Clock / 3.5f, 0.f, 1.f) : (T.Stage == 2 ? 1.f : .06f);
			if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Eye->GetMaterial(0))) { MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .34f, .035f) * (.35f + Charge * 7.f)); }
		}
	}
	if (T.Stage == 1 && T.Clock >= (T.bStatue ? 3.5f : 3.f))
	{
		T.Stage = 2; T.Clock = 0.f;
		if (!T.bStatue)
		{
			FProjectile P; P.From = Hand(T); P.DeckTarget = T.DeckTarget;
			P.Visual = T.Charge.IsValid() ? T.Charge.Get() : MakeFireball(P.From);
			T.Charge.Reset(); Projectiles.Add(P);
		}
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->Report(T.bStatue ? TEXT("Beam on deck! Watch that marked spot!") : TEXT("Fireball incoming!"), true); }
	}
	if (T.Stage == 2)
	{
		if (T.bStatue)
		{
			const FVector End = Director->DeckToWorld(T.DeckTarget);
			for (int32 I = 0; I < T.Beams.Num(); ++I)
			{
				if (UStaticMeshComponent* Beam = T.Beams[I].Get())
				{
					const FVector Start = Body->GetActorTransform().TransformPosition(T.EyeLocal[I]);
					const FVector D = End - Start;
					Beam->SetVisibility(true);
					Beam->SetWorldLocationAndRotation((Start + End) * .5f, FRotationMatrix::MakeFromZ(D).Rotator());
					const float Width = .10f + .025f * FMath::Sin(T.Clock * 43.f + I);
					Beam->SetWorldScale3D(FVector(Width, Width, D.Size() / 100.f));
				}
			}
			if (!T.bImpact && T.Clock >= .7f) { T.bImpact = true; Ignite(T.DeckTarget, TEXT("sentinel beam")); }
		}
		if (T.Clock >= (T.bStatue ? 1.2f : 2.4f))
		{
			T.Stage = 3; T.Clock = 0.f;
			for (auto& Beam : T.Beams) { if (Beam.IsValid()) { Beam->SetVisibility(false); } }
			if (T.Marker.IsValid()) { T.Marker->Destroy(); T.Marker.Reset(); }
		}
	}
	if (T.Stage == 3 && T.Clock >= 11.f) { T.Stage = 0; T.Exposure = 0.f; T.InitialWait = 2.5f; }
	UpdateAlert(T, bFogConcealed);
}

void ADCEarlyEncounters::RemoveThreat(FThreat& T)
{
	RetireFireball(T.Charge.Get());
	for (TWeakObjectPtr<AActor> Actor : {T.Body, T.Target, T.Marker}) { if (Actor.IsValid()) { Actor->Destroy(); } }
}

void ADCEarlyEncounters::Clear()
{
	for (FThreat& T : Threats) { RemoveThreat(T); }
	Threats.Reset();
	for (FProjectile& P : Projectiles) { RetireFireball(P.Visual.Get()); }
	Projectiles.Reset();
}

void ADCEarlyEncounters::EndPlay(const EEndPlayReason::Type Reason)
{
	Clear(); Super::EndPlay(Reason);
}

FString ADCEarlyEncounters::GetThreatText() const
{
	for (const FThreat& T : Threats)
	{
		if (!T.Body.IsValid() || T.Body->IsHidden() || T.Stage == 4 || !T.bAnnounced) { continue; }
		if (T.Stage == 1 || T.Stage == 2)
		{
			const TCHAR* Cue = T.Stage == 2 ? (T.bStatue ? TEXT("BEAM ON DECK — SHOOT THE EYES") : TEXT("FIREBALL INCOMING — WATCH THE DECK"))
				: (T.bStatue ? TEXT("STATUE CHARGING — SHOOT THE EYES") : TEXT("FIREBALL CHARGING — FIRE NOW"));
			return FString::Printf(TEXT("%s  %s"), T.Side < 0 ? TEXT("PORT") : TEXT("STARBOARD"), Cue);
		}
	}
	return FString();
}

void ADCEarlyEncounters::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { Director = ADCRideDirector::Get(this); }
	if (!Treadmill) { Treadmill = ADCTreadmill::Get(this); if (Treadmill) { AddTickPrerequisiteActor(Treadmill); } }
	if (!Guns) { Guns = ADCGunnery::Get(this); }
	if (!Troubles) { Troubles = ADCTroubles::Get(this); }
	if (!Director || !Treadmill || !Guns || !Troubles) { return; }
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("DCEarlyVerify"))) { TickVerification(DeltaSeconds); }
	if (FParse::Param(FCommandLine::Get(), TEXT("DCEncounterVerify"))) { TickEncounterVerification(); }
#endif
	const float T = Director->RideTime;
	if (LastRideTime >= 0.f && (T < LastRideTime - .5f || T > LastRideTime + 3.f)) { Clear(); NextGroup = NextStatue = 0; AttackSpacing = 0.f; }
	LastRideTime = T;
	if (Director->GetPhase() != EDCPhase::Fog || FParse::Param(FCommandLine::Get(), TEXT("DCNoEarlyEncounters"))) { Clear(); return; }
	const float Dt = DeltaSeconds * Director->TimeScale;
	AttackSpacing -= Dt;
	while (NextGroup < UE_ARRAY_COUNT(GroupTimes) && T >= GroupTimes[NextGroup])
	{
		if (T < GroupTimes[NextGroup] + 20.f) { SpawnGroup(NextGroup); }
		++NextGroup;
	}
	while (NextStatue < UE_ARRAY_COUNT(StatueTimes) && T >= StatueTimes[NextStatue])
	{
		if (T < StatueTimes[NextStatue] + 15.f) { SpawnStatue(NextStatue); }
		++NextStatue;
	}
	for (int32 I = Threats.Num() - 1; I >= 0; --I)
	{
		TickThreat(Threats[I], Dt);
		FThreat& Threat = Threats[I];
		if (!Threat.Body.IsValid() || Threat.X < -16000.f || (!Threat.bStatue && Threat.Stage == 4 && Threat.Clock > 5.f)) { RemoveThreat(Threat); Threats.RemoveAt(I); }
	}
	for (int32 I = Projectiles.Num() - 1; I >= 0; --I)
	{
		FProjectile& P = Projectiles[I]; P.Age += Dt;
		const float U = FMath::Clamp(P.Age / P.Duration, 0.f, 1.f);
		const FVector To = Director->DeckToWorld(P.DeckTarget);
		const FVector Position = FMath::Lerp(P.From, To, U) + FVector(0, 0, 4.f * 650.f * U * (1.f - U));
		if (P.Visual.IsValid()) { P.Visual->SetActorLocation(Position); }
		if (U >= 1.f)
		{
			Ignite(P.DeckTarget, TEXT("fireball"));
			RetireFireball(P.Visual.Get());
			Projectiles.RemoveAt(I);
		}
	}
}
