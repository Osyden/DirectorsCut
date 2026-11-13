#include "DCLeviathan.h"
#include "DCAudio.h"
#include "DCCrew.h"
#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "Animation/AnimSequence.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "ProceduralMeshComponent.h"
#include "Sound/SoundBase.h"

namespace
{
	constexpr int32 ArmRings = 64;
	constexpr int32 ArmSides = 12;
	constexpr float ArmCycleSeconds = 28.f;
	constexpr float FirstAttackAt = 52.f;

	float BowlHeight(float Distance, float Radius, float Depth)
	{
		const float R = Distance / FMath::Max(Radius, 1.f);
		return -Depth * FMath::Pow(FMath::Max(0.f, 1.f - R * R), 1.25f);
	}

	UAnimSequence* LoadKaijuAnimation(const TCHAR* Name)
	{
		const FString Asset = FString(TEXT("ClawKaiju_allAnimationClawKaiju_")) + Name;
		return LoadObject<UAnimSequence>(nullptr, *(TEXT("/Game/Fab/Claw_Kaiju_Monster/") + Asset + TEXT(".") + Asset));
	}
}

ADCLeviathan::ADCLeviathan()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("EncounterRoot"));
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeviathanBody"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->bEnableUpdateRateOptimizations = false;
	Arms = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ReachingArms"));
	Arms->SetupAttachment(RootComponent);
	Arms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PainAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("LeviathanPain"));
	PainAudio->SetupAttachment(RootComponent);
	PainAudio->bAutoActivate = false;
	PainAudio->bAllowSpatialization = false;
	SetActorHiddenInGame(true);
}

void ADCLeviathan::BeginPlay()
{
	Super::BeginPlay();
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimation.ClawKaiju_allAnimation"));
	if (!Mesh)
	{
		UE_LOG(LogDC, Error, TEXT("Leviathan cannot load the Claw Kaiju skeletal mesh"));
		return;
	}
	Body->SetSkeletalMesh(Mesh);
	MeshBoundsCenter = Mesh->GetBounds().Origin;
	MeshBoundsExtent = Mesh->GetBounds().BoxExtent;
	FeetOffset = MeshBoundsCenter.Z - MeshBoundsExtent.Z;
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/LeviathanSkin/M_LeviathanSkin.M_LeviathanSkin")))
	{
		for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index) { Body->SetMaterial(Index, Material); }
	}
	else
	{
		UE_LOG(LogDC, Error, TEXT("Leviathan skin missing; run Tools/make_leviathan_skin.py before play or packaging"));
	}
	IntimidateAnimation = LoadKaijuAnimation(TEXT("Intimidate"));
	CrawlAnimation = LoadKaijuAnimation(TEXT("Crawling"));
	AttackAnimation = LoadKaijuAnimation(TEXT("Attack"));
	HitAnimation = LoadKaijuAnimation(TEXT("HitReact"));
	DefeatedAnimation = LoadKaijuAnimation(TEXT("Defeated"));
	PainAudio->SetSound(LoadObject<USoundBase>(nullptr, TEXT("/Game/Sonniss/kraken_pain.kraken_pain"),
		nullptr, LOAD_NoWarn | LOAD_Quiet));
	if (UMaterialInterface* Skin = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Kraken.M_Kraken")))
	{
		ArmMaterial = UMaterialInstanceDynamic::Create(Skin, this);
		ArmMaterial->SetVectorParameterValue(TEXT("Skin"), FLinearColor(0.035f, 0.048f, 0.045f));
		ArmMaterial->SetVectorParameterValue(TEXT("Belly"), FLinearColor(0.22f, 0.11f, 0.055f));
	}
	TargetMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Glow.M_Glow")))
	{
		TargetMaterial = UMaterialInstanceDynamic::Create(Glow, this);
		TargetMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(3.5f, 0.75f, 0.13f));
	}
	for (int32 Ring = 0; Ring < ArmRings - 1; ++Ring)
	{
		for (int32 Side = 0; Side < ArmSides; ++Side)
		{
			const int32 A = Ring * (ArmSides + 1) + Side;
			const int32 B = A + ArmSides + 1;
			ArmTriangles.Append({ A, B, A + 1, A + 1, B, B + 1 });
		}
	}
	bAssetsReady = true;
	UE_LOG(LogDC, Log, TEXT("Leviathan loaded at scale %.1f, Intimidate %.1fs, Crawling %.1fs"), BodyScale,
		IntimidateAnimation ? IntimidateAnimation->GetPlayLength() : 0.f, CrawlAnimation ? CrawlAnimation->GetPlayLength() : 0.f);
}

void ADCLeviathan::RemoveTarget(FArm& Arm)
{
	if (AActor* Target = Arm.Target.Get()) { Target->Destroy(); }
	Arm.Target.Reset();
	Arm.bTargetRegistered = false;
}

void ADCLeviathan::ResetEncounter()
{
	PainAudio->Stop();
	for (FArm& Arm : ArmState)
	{
		RemoveTarget(Arm);
		Arm = FArm();
	}
	ArmsBroken = 0;
	LastHitAt = -1000.f;
	DefeatedAt = -1.f;
	bAnnounced = false;
	LastElapsed = -1.f;
}

void ADCLeviathan::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetEncounter();
	Super::EndPlay(Reason);
}

void ADCLeviathan::Animate(UAnimSequence* Animation, float Position, bool bLoop)
{
	if (!Animation) { Animation = CrawlAnimation; }
	if (!Animation) { return; }
	if (CurrentAnimation != Animation)
	{
		CurrentAnimation = Animation;
		Body->PlayAnimation(Animation, bLoop);
		Body->SetPlayRate(0.f);
	}
	const float Length = FMath::Max(Animation->GetPlayLength(), 0.01f);
	const float Time = bLoop ? FMath::Fmod(FMath::Max(0.f, Position), Length) : FMath::Clamp(Position, 0.f, Length);
	Body->SetPosition(Time, false);
}

void ADCLeviathan::UpdateEncounter(float Elapsed, const FTransform& Arena, float Radius, float Depth, bool bActive)
{
	if (!bAssetsReady) { return; }
	const ADCRideDirector* Ride = ADCRideDirector::Get(this);
	const float ExpectedAdvance = GetWorld()->GetDeltaSeconds() * (Ride ? Ride->TimeScale : 1.f);
	const bool bSeeking = LastElapsed >= 0.f && (Elapsed < LastElapsed || FMath::Abs(Elapsed - LastElapsed - ExpectedAdvance) > 0.5f);
	if (bSeeking || (!bActive && bWasActive)) { ResetEncounter(); }
	bWasActive = bActive;
	if (!bActive)
	{
		SetActorHiddenInGame(true);
		LastElapsed = Elapsed;
		return;
	}
	SetActorHiddenInGame(Elapsed < 8.f);
	const float Emergence = FMath::SmoothStep(8.f, 32.f, Elapsed);
	ADCRideDirector* Director = ADCRideDirector::Get(this);
	ShipPosition = FVector(0.f, 0.f, Director ? Director->SeaLevelZ : 0.f);
	const FVector ShipLocal = Arena.InverseTransformPosition(ShipPosition);
	const float Chase = FMath::SmoothStep(42.f, 108.f, Elapsed);
	const FVector Start(-Radius * 0.2f, Radius * 0.15f, 0.f);
	// Keep the enlarged body clear of the deck while its arms close the remaining distance.
	const float Clearance = FMath::Max(62000.f, FMath::Max(MeshBoundsExtent.X, MeshBoundsExtent.Y) * BodyScale + 20000.f);
	const FVector Alongside = ShipLocal + FVector(30000.f, Clearance, 0.f);
	FVector Position = FMath::Lerp(Start, Alongside, Chase);
	Position.Z = BowlHeight(Position.Size2D(), Radius, Depth) - FeetOffset * BodyScale;
	// Start the entire posed silhouette below the basin floor before revealing the actor.
	const float Submergence = MeshBoundsExtent.Z * 2.f * BodyScale * 1.15f + 6000.f;
	Position.Z -= (1.f - Emergence) * Submergence;
	Position.Z += FMath::Sin(Elapsed * 0.18f) * 300.f * Emergence;
	if (DefeatedAt >= 0.f) { Position.Z -= FMath::SmoothStep(3.f, 18.f, Elapsed - DefeatedAt) * Submergence; }
	Body->SetWorldLocation(Arena.TransformPosition(Position));
	FRotator Facing = (ShipLocal - Position).Rotation();
	Facing.Pitch = 0.f;
	Facing.Yaw -= MeshForwardYaw;
	Facing.Roll = 0.f;
	Body->SetWorldRotation(Arena.TransformRotation(Facing.Quaternion()));
	Body->SetWorldScale3D(FVector(BodyScale));

	const bool bCanDamage = LastElapsed >= 0.f && !bSeeking;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ArmState); ++Index)
	{
		UpdateArm(Index, Elapsed, Emergence, bCanDamage);
	}
	if (DefeatedAt >= 0.f)
	{
		Animate(DefeatedAnimation, (Elapsed - DefeatedAt) * 0.5f, false);
	}
	else if (Elapsed - LastHitAt < 4.f)
	{
		Animate(HitAnimation, (Elapsed - LastHitAt) * 0.6f, false);
	}
	else if (Elapsed < 42.f)
	{
		Animate(IntimidateAnimation, (Elapsed - 8.f) * 0.35f, true);
	}
	else
	{
		const float AttackPhase = FMath::Fmod(FMath::Max(0.f, Elapsed - FirstAttackAt), ArmCycleSeconds);
		if (Elapsed > FirstAttackAt && AttackPhase < 6.f) { Animate(AttackAnimation, AttackPhase * 0.5f, false); }
		else { Animate(CrawlAnimation, Elapsed * 0.32f, true); }
	}
	if (!bAnnounced && Elapsed >= 24.f)
	{
		bAnnounced = true;
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(TEXT("Captain... that is the bottom moving. Aim for the bright joints when its arms reach us!"), true);
		}
	}
	LastElapsed = Elapsed;
}

void ADCLeviathan::SpawnTarget(FArm& Arm, const FVector& Position)
{
	ADCTreadmill* Treadmill = ADCTreadmill::Get(this);
	if (!Treadmill || !TargetMesh) { return; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	AStaticMeshActor* Target = GetWorld()->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator, Params);
	if (!Target) { return; }
	UStaticMeshComponent* Component = Target->GetStaticMeshComponent();
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetStaticMesh(TargetMesh);
	Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Component->SetCollisionResponseToAllChannels(ECR_Ignore);
	Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Component->SetCastShadow(false);
	if (TargetMaterial) { Component->SetMaterial(0, TargetMaterial); }
	Target->SetActorScale3D(FVector(5.f, 5.f, 8.f));
	Treadmill->AddExternalHostile(Target, 550.f, 8.f);
	Arm.Target = Target;
	Arm.bTargetRegistered = true;
}

FVector ADCLeviathan::KeepTipReachable(const FVector& DesiredTip, float Side) const
{
	const ADCMaelstrom* Maelstrom = ADCMaelstrom::Get(this);
	if (!Maelstrom || !Maelstrom->IsActive()) { return DesiredTip; }
	// Aim from a representative upper-deck gun. A joint must clear both the water
	// under it and the rising water between it and the battery, with two degrees
	// spare under the captain's +12-degree elevation limit.
	const FVector Gun = ShipPosition + FVector(0.f, Side * 650.f, 800.f);
	const FVector BulbOffset(0.f, Side * 950.f, 350.f);
	const FVector DesiredBulb = DesiredTip + BulbOffset;
	FVector BestBulb = DesiredBulb;
	float BestExcess = TNumericLimits<float>::Max();
	for (int32 Candidate = 0; Candidate <= 12; ++Candidate)
	{
		FVector Bulb = DesiredBulb;
		// Positive X is downhill from both broadsides in the arena's 19-28deg bearing.
		// Forward motion is needed only when the port-side slope masks a tell.
		Bulb.X = FMath::Lerp(DesiredBulb.X, ShipPosition.X + 8000.f, Candidate / 12.f);
		float RequiredZ = FMath::Max(Bulb.Z, Maelstrom->SampleSurfaceWorld(Bulb) + 500.f);
		for (int32 Step = 1; Step <= 16; ++Step)
		{
			const float Fraction = Step / 16.f;
			const FVector Point = FMath::Lerp(Gun, Bulb, Fraction);
			const float WaterZ = Maelstrom->SampleSurfaceWorld(Point) + 150.f;
			RequiredZ = FMath::Max(RequiredZ, Gun.Z + (WaterZ - Gun.Z) / Fraction);
		}
		Bulb.Z = RequiredZ;
		const float Distance = FVector2D(Bulb.X - Gun.X, Bulb.Y - Gun.Y).Size();
		const float Excess = RequiredZ - (Gun.Z + Distance * 0.176327f); // tan(10deg)
		if (Excess < BestExcess) { BestExcess = Excess; BestBulb = Bulb; }
		if (Excess <= 0.f) { return Bulb - BulbOffset; }
	}
	return BestBulb - BulbOffset;
}

void ADCLeviathan::UpdateArm(int32 Index, float Elapsed, float Emergence, bool bCanDamage)
{
	FArm& Arm = ArmState[Index];
	const float AttackTime = Elapsed - FirstAttackAt - Index * 6.f;
	const int32 Cycle = AttackTime >= 0.f ? FMath::FloorToInt(AttackTime / ArmCycleSeconds) : INDEX_NONE;
	const float Phase = AttackTime >= 0.f ? FMath::Fmod(AttackTime, ArmCycleSeconds) : ArmCycleSeconds;
	if (Cycle != Arm.Cycle)
	{
		RemoveTarget(Arm);
		Arm.Cycle = Cycle;
		Arm.bBrokenThisCycle = false;
		Arm.bStruck = false;
	}
	if (Arm.bTargetRegistered && !Arm.Target.IsValid())
	{
		Arm.bTargetRegistered = false;
		Arm.bBrokenThisCycle = true;
		Arm.BrokenAt = Elapsed;
		LastHitAt = Elapsed;
		++ArmsBroken;
		if (ArmsBroken >= BreaksToDefeat && DefeatedAt < 0.f) { DefeatedAt = Elapsed; }
		if (PainAudio->Sound)
		{
			const ADCAudio* Mix = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(), ADCAudio::StaticClass()));
			PainAudio->SetVolumeMultiplier(0.9f * (Mix ? Mix->MasterVolume : 0.8f));
			PainAudio->SetPitchMultiplier(DefeatedAt >= 0.f ? 0.65f : FMath::FRandRange(0.75f, 0.9f));
			PainAudio->Play();
		}
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(DefeatedAt >= 0.f ? TEXT("It is falling! Hold our course, Captain!") : TEXT("That hurt it! The whole beast is recoiling!"), true);
		}
	}
	const float Side = Index % 2 == 0 ? -1.f : 1.f;
	const float Longitudinal = Index < 2 ? 1000.f : -1100.f;
	// Two pairs grow from the lower flanks. Mesh-space anchors scale and turn with the body.
	const float Flank = Index % 2 == 0 ? -1.f : 1.f;
	FVector Attachment = MeshBoundsCenter;
	Attachment.X += Flank * MeshBoundsExtent.X * 0.55f;
	Attachment.Y += MeshBoundsExtent.Y * (Index < 2 ? -0.25f : 0.25f);
	Attachment.Z = FeetOffset + MeshBoundsExtent.Z * 2.f * (Index < 2 ? 0.40f : 0.32f);
	const FVector Base = Body->GetComponentTransform().TransformPosition(Attachment);
	const float TellX = Index < 2 ? 4000.f : 2600.f;
	const FVector IdleTip = ShipPosition + FVector(TellX + 600.f * FMath::Sin(Elapsed * 0.2f + Index), Side * 8000.f, 1800.f);
	const FVector RaisedTip = ShipPosition + FVector(TellX, Side * 6500.f, 1700.f);
	const FVector StrikeTip = ShipPosition + FVector(Longitudinal, Side * 220.f, 500.f);
	FVector Tip = IdleTip;
	if (Cycle != INDEX_NONE && Phase < 6.f) { Tip = FMath::Lerp(IdleTip, RaisedTip, FMath::SmoothStep(0.f, 6.f, Phase)); }
	else if (Phase < 7.1f) { Tip = FMath::Lerp(RaisedTip, StrikeTip, FMath::SmoothStep(6.f, 7.1f, Phase)); }
	else if (Phase < 7.7f) { Tip = StrikeTip; }
	else if (Phase < 13.f) { Tip = FMath::Lerp(StrikeTip, IdleTip, FMath::SmoothStep(7.7f, 13.f, Phase)); }
	const FVector DeckTip = Tip;
	if (!Arm.bBrokenThisCycle && DefeatedAt < 0.f) { Tip = KeepTipReachable(Tip, Side); }
	FVector WeakPoint = Tip + FVector(0.f, Side * 950.f, 350.f);
	// The striking extremity still reaches the deck. Its vulnerable joint remains
	// on the arm just outboard, above the local sea and inside the cannon envelope.
	const float StrikeBlend = FMath::SmoothStep(6.f, 7.1f, Phase) * (1.f - FMath::SmoothStep(7.7f, 9.f, Phase));
	Tip = FMath::Lerp(Tip, DeckTip, StrikeBlend);

	float Retract = Arm.bBrokenThisCycle ? FMath::SmoothStep(0.f, 3.f, Elapsed - Arm.BrokenAt) : 0.f;
	if (DefeatedAt >= 0.f) { Retract = FMath::Max(Retract, FMath::SmoothStep(0.f, 5.f, Elapsed - DefeatedAt)); }
	Tip.Z -= (1.f - Emergence) * 60000.f + Retract * 17000.f;
	WeakPoint.Z -= (1.f - Emergence) * 60000.f + Retract * 17000.f;
	FVector Bend = ShipPosition + FVector(WeakPoint.X - ShipPosition.X + 1800.f, Side * 8500.f,
		FMath::Max(4500.f, WeakPoint.Z - ShipPosition.Z + 1450.f) + 500.f * FMath::Sin(Elapsed * 0.23f + Index));
	const float Reach = FMath::SmoothStep(36.f + Index * 3.f, FirstAttackAt + Index * 6.f, Elapsed);
	Tip = FMath::Lerp(Base + FVector(0.f, Side * 7000.f, 4000.f), Tip, Reach);
	WeakPoint = FMath::Lerp(Base + FVector(0.f, Side * 7950.f, 4350.f), WeakPoint, Reach);
	Bend = FMath::Lerp(Base + FVector(0.f, Side * 5000.f, 8000.f), Bend, Reach);
	BuildArm(Index, Base, Bend, WeakPoint, Tip, Elapsed);
	Arms->SetMeshSectionVisible(Index, Emergence > 0.1f && Elapsed > 28.f);

	const bool bExposed = Cycle != INDEX_NONE && Phase < 13.f && !Arm.bBrokenThisCycle && DefeatedAt < 0.f;
	// The bulb stays outboard until the final strike, reachable with both batteries.
	if (bExposed)
	{
		if (!Arm.bTargetRegistered) { SpawnTarget(Arm, WeakPoint); }
		if (AActor* Target = Arm.Target.Get()) { Target->SetActorLocation(WeakPoint); }
	}
	else { RemoveTarget(Arm); }
	if (Phase >= 7.1f && Phase < 7.7f && !Arm.bStruck && bCanDamage && !Arm.bBrokenThisCycle && DefeatedAt < 0.f)
	{
		Arm.bStruck = true;
		if (ADCRideDirector* Director = ADCRideDirector::Get(this)) { Director->DamageShip(StrikeDamage); }
	}
}

void ADCLeviathan::BuildArm(int32 Index, const FVector& Base, const FVector& Bend, const FVector& Joint, const FVector& Tip, float Elapsed)
{
	const FVector Control = Base + FVector(0.f, 0.f, -12000.f);
	TArray<FVector> Points;
	Points.Reserve(ArmRings);
	for (int32 Ring = 0; Ring < ArmRings; ++Ring)
	{
		const float U = static_cast<float>(Ring) / (ArmRings - 1);
		constexpr float JointU = 0.86f;
		FVector Point;
		if (U < JointU)
		{
			const float T = U / JointU;
			const float V = 1.f - T;
			Point = Base * V * V * V + Control * 3.f * V * V * T + Bend * 3.f * V * T * T + Joint * T * T * T;
			Point.Z += FMath::Sin(U * 11.f - Elapsed * 0.4f + Index) * FMath::Sin(T * UE_PI) * 1100.f;
		}
		else
		{
			const float T = (U - JointU) / (1.f - JointU);
			const float V = 1.f - T;
			const FVector TailControl = Joint + (Joint - Bend) * ((1.f - JointU) / JointU);
			Point = Joint * V * V * V + TailControl * 3.f * V * V * T
				+ FMath::Lerp(Tip, Joint, 0.35f) * 3.f * V * T * T + Tip * T * T * T;
		}
		Points.Add(Point);
	}
	TArray<FVector> Vertices, Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	float Distance = 0.f;
	for (int32 Ring = 0; Ring < ArmRings; ++Ring)
	{
		const float U = static_cast<float>(Ring) / (ArmRings - 1);
		if (Ring > 0) { Distance += FVector::Distance(Points[Ring], Points[Ring - 1]); }
		const FVector Tangent = (Points[FMath::Min(Ring + 1, ArmRings - 1)] - Points[FMath::Max(0, Ring - 1)]).GetSafeNormal();
		FVector Across, Up;
		Tangent.FindBestAxisVectors(Across, Up);
		const float Width = FMath::Lerp(2200.f, 130.f, FMath::Pow(U, 0.7f)) * (1.f + 0.06f * FMath::Sin(U * 85.f));
		for (int32 Side = 0; Side <= ArmSides; ++Side)
		{
			const float Around = static_cast<float>(Side) / ArmSides;
			const float Angle = Around * UE_TWO_PI;
			const FVector Normal = Across * FMath::Cos(Angle) + Up * FMath::Sin(Angle);
			Vertices.Add(Arms->GetComponentTransform().InverseTransformPosition(Points[Ring] + Normal * Width));
			Normals.Add(Arms->GetComponentTransform().InverseTransformVectorNoScale(Normal));
			UVs.Add(FVector2D(Distance / 700.f, Around));
			Colors.Add(FLinearColor(FMath::Max(0.f, -Normal.Z), Around, U, 1.f).ToFColor(false));
		}
	}
	if (!Arms->GetProcMeshSection(Index))
	{
		Arms->CreateMeshSection(Index, Vertices, ArmTriangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
		if (ArmMaterial) { Arms->SetMaterial(Index, ArmMaterial); }
	}
	else { Arms->UpdateMeshSection(Index, Vertices, Normals, UVs, Colors, TArray<FProcMeshTangent>()); }
}
