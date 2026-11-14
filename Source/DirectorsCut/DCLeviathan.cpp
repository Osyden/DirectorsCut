#include "DCLeviathan.h"
#include "DCAudio.h"
#include "DCCrew.h"
#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
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
	constexpr int32 ArmRings = 96;
	constexpr int32 ArmSides = 24;
	constexpr float ArmCycleSeconds = 28.f;
	constexpr float FirstPeekAt = 22.f;
	constexpr float FirstAttackAt = 65.f;
	constexpr float ArmWidthScale = 0.45f;
}

ADCLeviathan::ADCLeviathan()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("EncounterRoot"));
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeviathanBody"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// The water material does not guarantee submerged skeletal-mesh clipping.
	// Never reveal the carapace or its shadow; scale is conveyed by the crater and arms.
	Body->SetVisibility(false, true);
	Body->SetHiddenInGame(true, true);
	Body->SetCastShadow(false);
	Body->bCastHiddenShadow = false;
	Body->SetComponentTickEnabled(false);
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
	if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimation.ClawKaiju_allAnimation"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Body->SetSkeletalMesh(Mesh);
		MeshBoundsCenter = Mesh->GetBounds().Origin;
		MeshBoundsExtent = Mesh->GetBounds().BoxExtent;
	}
	PainAudio->SetSound(LoadObject<USoundBase>(nullptr, TEXT("/Game/Sonniss/kraken_pain.kraken_pain"),
		nullptr, LOAD_NoWarn | LOAD_Quiet));
	if (UMaterialInterface* Skin = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_LeviathanWet.M_LeviathanWet")))
	{
		ArmMaterial = UMaterialInstanceDynamic::Create(Skin, this);
		ArmMaterial->SetVectorParameterValue(TEXT("Skin"), FLinearColor(0.035f, 0.048f, 0.045f));
		ArmMaterial->SetVectorParameterValue(TEXT("Belly"), FLinearColor(0.22f, 0.11f, 0.055f));
	}
	TargetMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_LeviathanWeakpoint.M_LeviathanWeakpoint")))
	{
		TargetMaterial = UMaterialInstanceDynamic::Create(Glow, this);
		TargetMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.2f, 0.28f, 0.07f));
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
	const int32 EndCap = ArmRings * (ArmSides + 1);
	const int32 BaseCap = EndCap + 1;
	const int32 LastRing = (ArmRings - 1) * (ArmSides + 1);
	for (int32 Side = 0; Side < ArmSides; ++Side)
	{
		// UE uses clockwise fronts: these agree with +T at the tip and -T at the root.
		ArmTriangles.Append({ LastRing + Side, EndCap, LastRing + Side + 1, Side, Side + 1, BaseCap });
	}
	bAssetsReady = true;
	UE_LOG(LogDC, Log, TEXT("Leviathan body concealed; local arms peek after %.0fs, attacks after %.0fs"), FirstPeekAt, FirstAttackAt);
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
	DefeatedAt = -1.f;
	bAnnounced = false;
	LastElapsed = -1.f;
}

void ADCLeviathan::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetEncounter();
	Super::EndPlay(Reason);
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
	SetActorHiddenInGame(Elapsed < FirstPeekAt);
	ADCRideDirector* Director = ADCRideDirector::Get(this);
	ShipPosition = FVector(0.f, 0.f, Director ? Director->SeaLevelZ : 0.f);
	ADCMaelstrom* Maelstrom = ADCMaelstrom::Get(this);
	FVector HiddenBodyPosition = Arena.TransformPosition(FVector(Radius * 0.1f, 0.f, 0.f));
	const float SurfaceZ = Maelstrom ? Maelstrom->SampleSurfaceWorld(HiddenBodyPosition) : ShipPosition.Z;
	HiddenBodyPosition.Z = SurfaceZ - (MeshBoundsCenter.Z + MeshBoundsExtent.Z) * BodyScale - FMath::Max(Depth, 20000.f);
	Body->SetWorldLocationAndRotation(HiddenBodyPosition, Arena.GetRotation());
	Body->SetWorldScale3D(FVector(BodyScale));
	Body->SetVisibility(false, true);
	Body->SetHiddenInGame(true, true);

	const bool bCanDamage = LastElapsed >= 0.f && !bSeeking && Elapsed > LastElapsed + UE_SMALL_NUMBER;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ArmState); ++Index)
	{
		UpdateArm(Index, Elapsed, bCanDamage);
	}
	// The end comes on the clock, so the music can be cut to it. What the guns did is in the score.
	if (DefeatedAt < 0.f && Elapsed >= FinaleAt && LastElapsed >= 0.f)
	{
		DefeatedAt = Elapsed;
		if (PainAudio->Sound)
		{
			const ADCAudio* Mix = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(), ADCAudio::StaticClass()));
			PainAudio->SetVolumeMultiplier(1.f * (Mix ? Mix->MasterVolume : 0.8f));
			PainAudio->SetPitchMultiplier(0.6f);
			PainAudio->Play();
		}
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(ArmsBroken >= 4 ? TEXT("It is letting go! We have broken it, Captain! Hold our course!") : TEXT("It is letting go! The sea is taking it back! Hold our course, Captain!"), true);
		}
		UE_LOG(LogDC, Log, TEXT("Leviathan: beaten on the clock at %.1f s after the breach, %d arms broken"), Elapsed, ArmsBroken);
	}
	if (!bAnnounced && Elapsed >= FirstPeekAt + 6.f)
	{
		bAnnounced = true;
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(TEXT("Something immense is beneath us. Ready the guns and watch for the bright joints!"), true);
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
	Target->SetActorScale3D(FVector(2.8f, 1.2f, 3.6f));
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
	const float Outboard = FMath::Abs(DesiredBulb.Y - Gun.Y);
	const float ForwardLimit = FMath::Min(8000.f, Outboard * 1.8f);
	FVector BestBulb = DesiredBulb;
	float BestExcess = TNumericLimits<float>::Max();
	for (int32 Candidate = 0; Candidate <= 12; ++Candidate)
	{
		FVector Bulb = DesiredBulb;
		// Search toward the bow when a rising slope masks a tell. Keep the joint
		// inside the broadside's yaw envelope, including during the deck strike.
		const float StartX = FMath::Clamp(DesiredBulb.X - Gun.X, -ForwardLimit, ForwardLimit);
		Bulb.X = Gun.X + FMath::Lerp(StartX, ForwardLimit, Candidate / 12.f);
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

void ADCLeviathan::UpdateArm(int32 Index, float Elapsed, bool bCanDamage)
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
		Arm.bEmerged=false;
	}
	if (Arm.bTargetRegistered && !Arm.Target.IsValid())
	{
		Arm.bTargetRegistered = false;
		Arm.bBrokenThisCycle = true;
		Arm.BrokenAt = Elapsed;
		++ArmsBroken;
		if (ADCRideDirector* Tally = ADCRideDirector::Get(this)) { Tally->ArmsBroken = ArmsBroken; Tally->Score += 150; }
		if (PainAudio->Sound)
		{
			const ADCAudio* Mix = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(), ADCAudio::StaticClass()));
			PainAudio->SetVolumeMultiplier(0.9f * (Mix ? Mix->MasterVolume : 0.8f));
			PainAudio->SetPitchMultiplier(DefeatedAt >= 0.f ? 0.65f : FMath::FRandRange(0.75f, 0.9f));
			PainAudio->Play();
		}
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(TEXT("That arm is pulling back! Keep firing at the bright joints!"), true);
		}
	}
	const float Side = Index % 2 == 0 ? -1.f : 1.f;
	const float Longitudinal = Index < 2 ? 1000.f : -1100.f;
	const ADCMaelstrom* Maelstrom = ADCMaelstrom::Get(this);
	auto WaterAt = [this, Maelstrom](const FVector& Point)
	{
		return Maelstrom ? Maelstrom->SampleSurfaceWorld(Point) : ShipPosition.Z;
	};
	// Each continuous tube begins well below nearby water. The unseen connection
	// to the creature stays submerged; no horizon-spanning mesh crosses the vista.
	FVector Base = ShipPosition + FVector(Index < 2 ? 4500.f : -1800.f, Side * (Index < 2 ? 10500.f : 11500.f), 0.f);
	Base.Z = WaterAt(Base) - 6000.f;
	const float TellX = Index < 2 ? 4000.f : 2600.f;
	const FVector IdleTip = ShipPosition + FVector(TellX + 600.f * FMath::Sin(Elapsed * 0.2f + Index), Side * 8000.f, 1250.f);
	const FVector RaisedTip = ShipPosition + FVector(TellX, Side * 6500.f, 1150.f);
	const FVector StrikeTip = ShipPosition + FVector(Longitudinal, Side * 1850.f, -50.f);
	FVector Tip = IdleTip;
	if (Cycle != INDEX_NONE && Phase < 6.f) { Tip = FMath::Lerp(IdleTip, RaisedTip, FMath::SmoothStep(0.f, 6.f, Phase)); }
	else if (Phase < 7.1f) { Tip = FMath::Lerp(RaisedTip, StrikeTip, FMath::SmoothStep(6.f, 7.1f, Phase)); }
	else if (Phase < 7.7f) { Tip = StrikeTip; }
	else if (Phase < 13.f) { Tip = FMath::Lerp(StrikeTip, IdleTip, FMath::SmoothStep(7.7f, 13.f, Phase)); }
	const float PeekAt = FirstPeekAt + Index * 2.5f;
	const float Rise = FMath::SmoothStep(PeekAt, PeekAt + 2.f, Elapsed);
	if (Cycle == INDEX_NONE)
	{
		FVector SurfacePeek = IdleTip;
		SurfacePeek.Z = WaterAt(SurfacePeek) + 100.f;
		Tip = FMath::Lerp(SurfacePeek, IdleTip, FMath::SmoothStep(PeekAt + 2.f, FirstAttackAt + Index * 6.f, Elapsed));
	}
	const FVector DeckTip = Tip;
	if (Cycle != INDEX_NONE && !Arm.bBrokenThisCycle && DefeatedAt < 0.f) { Tip = KeepTipReachable(Tip, Side); }
	FVector WeakPoint = Tip + FVector(0.f, Side * 950.f, 350.f);
	// The strike lands outside the rail. Its vulnerable joint remains
	// on the arm just outboard, above the local sea and inside the cannon envelope.
	const float StrikeBlend = FMath::SmoothStep(6.f, 7.1f, Phase) * (1.f - FMath::SmoothStep(7.7f, 9.f, Phase));
	Tip = FMath::Lerp(Tip, DeckTip, StrikeBlend);

	float Retract = Arm.bBrokenThisCycle ? FMath::SmoothStep(0.f, 3.f, Elapsed - Arm.BrokenAt) : 0.f;
	if (DefeatedAt >= 0.f) { Retract = FMath::Max(Retract, FMath::SmoothStep(0.f, 5.f, Elapsed - DefeatedAt)); }
	FVector Bend = FMath::Lerp(Base, WeakPoint, 0.55f);
	Bend.Z = FMath::Max(WeakPoint.Z + 800.f, WaterAt(Bend) + 900.f) + 220.f * FMath::Sin(Elapsed * 0.23f + Index);
	const float Submerge = (1.f - Rise) * 6000.f + Retract * 8000.f;
	Tip.Z -= Submerge;
	WeakPoint.Z -= Submerge;
	Bend.Z -= Submerge;
	BuildArm(Index, Base, Bend, WeakPoint, Tip, Elapsed);
	if (Maelstrom && Arm.bHasWaterContact && Elapsed>=PeekAt && Retract<.7f)
	{
		// Contact belongs at the actual spline/water intersection, not the
		// submerged root tens of metres away from the visible arm.
		if (!Arm.bEmerged && Rise>.15f)
		{
			const_cast<ADCMaelstrom*>(Maelstrom)->EmitWaterContact(Arm.WaterContact,2.f);
			Arm.bEmerged=true;
		}
		if (Elapsed>Arm.NextSprayAt)
		{
			const_cast<ADCMaelstrom*>(Maelstrom)->EmitWaterContact(Arm.WaterContact,.25f);
			Arm.NextSprayAt=Elapsed+.25f;
		}
	}
	Arms->SetMeshSectionVisible(Index, Elapsed >= PeekAt && (DefeatedAt < 0.f || Elapsed - DefeatedAt < 6.f));

	const bool bExposed = Cycle != INDEX_NONE && Phase < 13.f && !Arm.bBrokenThisCycle && DefeatedAt < 0.f;
	// The bulb stays outboard until the final strike, reachable with both batteries.
	if (bExposed)
	{
		if (!Arm.bTargetRegistered) { SpawnTarget(Arm, WeakPoint); }
		if (AActor* Target = Arm.Target.Get()) { Target->SetActorLocation(WeakPoint + FVector(0.f, -Side * 180.f, 0.f)); }
	}
	else { RemoveTarget(Arm); }
	if (Phase >= 7.1f && Phase < 7.7f && !Arm.bStruck && bCanDamage && !Arm.bBrokenThisCycle && DefeatedAt < 0.f)
	{
		Arm.bStruck = true;
		if (Maelstrom) { const_cast<ADCMaelstrom*>(Maelstrom)->EmitWaterContact(Tip,2.2f); }
		if (ADCRideDirector* Director = ADCRideDirector::Get(this)) { Director->DamageShip(StrikeDamage); }
	}
}

void ADCLeviathan::BuildArm(int32 Index, const FVector& Base, const FVector& Bend, const FVector& Joint, const FVector& Tip, float Elapsed)
{
	const FVector Control = Base + FVector(0.f, 0.f, 2800.f);
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
			// Zero displacement AND derivative at the joint preserves the C1 tail join.
			Point.Z += FMath::Sin(U * 11.f - Elapsed * 0.4f + Index) * FMath::Square(FMath::Sin(T * UE_PI)) * 350.f;
		}
		else
		{
			const float T = (U - JointU) / (1.f - JointU);
			const float V = 1.f - T;
			const FVector TailControl = Joint + (Joint - Bend) * ((1.f - JointU) / JointU);
			Point = Joint * V * V * V + TailControl * 3.f * V * V * T
				+ FMath::Lerp(Tip, Joint, 0.35f) * 3.f * V * T * T + Tip * T * T * T;
		}
		const float ShipX=Point.X-ShipPosition.X;
		if (FMath::Abs(ShipX)<4000.f && Point.Z>ShipPosition.Z-250.f)
		{
			const float Safe=1400.f+ArmWidthScale*FMath::Lerp(2200.f,130.f,FMath::Pow(U,.7f));
			const float Sign=Base.Y<0.f ? -1.f : 1.f;
			Point.Y=ShipPosition.Y+Sign*FMath::Max(FMath::Abs(Point.Y-ShipPosition.Y),Safe);
		}
		Points.Add(Point);
	}
	FArm& State=ArmState[Index];
	State.bHasWaterContact=false;
	if (const ADCMaelstrom* Stage=ADCMaelstrom::Get(this); Stage && Elapsed>=FirstPeekAt+Index*2.5f)
	{
		float PreviousDepth=Points[0].Z-Stage->SampleSurfaceWorld(Points[0]);
		for (int32 Ring=4;Ring<Points.Num()+3;Ring+=4)
		{
			const int32 Current=FMath::Min(Ring,Points.Num()-1);
			const float DepthHere=Points[Current].Z-Stage->SampleSurfaceWorld(Points[Current]);
			if (PreviousDepth<0.f && DepthHere>=0.f)
			{
				State.WaterContact=FMath::Lerp(Points[Ring-4],Points[Current],-PreviousDepth/FMath::Max(.01f,DepthHere-PreviousDepth));
				State.bHasWaterContact=true; break;
			}
			PreviousDepth=DepthHere;
		}
	}
	TArray<FVector> Vertices, Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	const int32 VertexCount = ArmRings * (ArmSides + 1) + 2;
	Vertices.Reserve(VertexCount); Normals.Reserve(VertexCount);
	UVs.Reserve(VertexCount); Colors.Reserve(VertexCount); Tangents.Reserve(VertexCount);
	const FTransform MeshTransform = Arms->GetComponentTransform();
	auto WidthAt = [](float U)
	{
		return ArmWidthScale * FMath::Lerp(2200.f, 130.f, FMath::Pow(U, 0.7f)) * (1.f + 0.06f * FMath::Sin(U * 85.f));
	};
	FVector PreviousAcross = FVector::ForwardVector;
	FVector PreviousTangent = FVector::UpVector;
	FVector FirstAcross = FVector::ForwardVector;
	FVector FirstTangent = FVector::UpVector;
	float Distance = 0.f;
	for (int32 Ring = 0; Ring < ArmRings; ++Ring)
	{
		const float U = static_cast<float>(Ring) / (ArmRings - 1);
		if (Ring > 0) { Distance += FVector::Distance(Points[Ring], Points[Ring - 1]); }
		const int32 Before = FMath::Max(0, Ring - 1);
		const int32 After = FMath::Min(Ring + 1, ArmRings - 1);
		const FVector Span = Points[After] - Points[Before];
		FVector Tangent = Span.GetSafeNormal();
		if (Tangent.IsNearlyZero()) { Tangent = PreviousTangent; }
		FVector Across;
		if (Ring == 0)
		{
			const FVector Reference = FMath::Abs(Tangent.X) < 0.9f ? FVector::ForwardVector : FVector::RightVector;
			Across = (Reference - Tangent * FVector::DotProduct(Reference, Tangent)).GetSafeNormal();
			FirstAcross = Across;
			FirstTangent = Tangent;
		}
		else
		{
			// Transport the previous frame instead of choosing a new world axis at
			// every ring. Independent bases flipped the tube at steep bends.
			Across = (PreviousAcross - Tangent * FVector::DotProduct(PreviousAcross, Tangent)).GetSafeNormal();
			if (Across.IsNearlyZero())
			{
				Across = FQuat::FindBetweenNormals(PreviousTangent, Tangent).RotateVector(PreviousAcross).GetSafeNormal();
			}
		}
		const FVector Up = FVector::CrossProduct(Tangent, Across).GetSafeNormal();
		PreviousAcross = Across;
		PreviousTangent = Tangent;
		const float Width = WidthAt(U);
		const float WidthSlope = (WidthAt(After / float(ArmRings - 1)) - WidthAt(Before / float(ArmRings - 1))) / FMath::Max(Span.Size(), 1.0);
		for (int32 Side = 0; Side <= ArmSides; ++Side)
		{
			const float Around = static_cast<float>(Side) / ArmSides;
			const float Angle = Around * UE_TWO_PI;
			const FVector Radial = Across * FMath::Cos(Angle) + Up * FMath::Sin(Angle);
			const FVector Normal = (Radial - Tangent * WidthSlope).GetSafeNormal();
			const FVector SurfaceTangent = (Tangent - Normal * FVector::DotProduct(Tangent, Normal)).GetSafeNormal();
			Vertices.Add(MeshTransform.InverseTransformPosition(Points[Ring] + Radial * Width));
			Normals.Add(MeshTransform.InverseTransformVectorNoScale(Normal));
			Tangents.Add(FProcMeshTangent(MeshTransform.InverseTransformVectorNoScale(SurfaceTangent), true));
			UVs.Add(FVector2D(Distance / 700.f, Around));
			Colors.Add(FLinearColor(FMath::Max(0.f, -Normal.Z), Around, U, 1.f).ToFColor(false));
		}
	}
	auto AddCap = [&](const FVector& Point, const FVector& Normal, const FVector& Across, float U)
	{
		Vertices.Add(MeshTransform.InverseTransformPosition(Point));
		Normals.Add(MeshTransform.InverseTransformVectorNoScale(Normal));
		Tangents.Add(FProcMeshTangent(MeshTransform.InverseTransformVectorNoScale(Across), false));
		UVs.Add(FVector2D(U, 0.5f));
		Colors.Add(FLinearColor(FMath::Max(0.f, -Normal.Z), 0.5f, U > 0.f ? 1.f : 0.f, 1.f).ToFColor(false));
	};
	AddCap(Points.Last(), PreviousTangent, PreviousAcross, Distance / 700.f);
	AddCap(Points[0], -FirstTangent, FirstAcross, 0.f);
	const FProcMeshSection* Existing = Arms->GetProcMeshSection(Index);
	if (!Existing || Existing->ProcVertexBuffer.Num() != Vertices.Num())
	{
		Arms->CreateMeshSection(Index, Vertices, ArmTriangles, Normals, UVs, Colors, Tangents, false);
		if (ArmMaterial) { Arms->SetMaterial(Index, ArmMaterial); }
	}
	else { Arms->UpdateMeshSection(Index, Vertices, Normals, UVs, Colors, Tangents); }
}
