#include "DCLeviathan.h"
#include "DCBossCombat.h"
#include "DCBossTimeline.h"
#include "Animation/AnimSequence.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "DCAudio.h"
#include "DCCrew.h"
#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
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
	constexpr float FirstPeekAt = 36.f;
	constexpr float ArmWidthScale = 0.25f;
}

ADCLeviathan::ADCLeviathan()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("EncounterRoot"));
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeviathanBody"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// The body stays on a bounded inner-lake swim path; it never pursues helm offsets.
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
    const bool Kraken=FParse::Param(FCommandLine::Get(),TEXT("DCBossKraken"));
    const TCHAR* BodyPath=Kraken?TEXT("/Game/Kraken/Meshes/KRAKEN.KRAKEN"):TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimation.ClawKaiju_allAnimation");
	if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr,BodyPath,nullptr,LOAD_NoWarn|LOAD_Quiet))
	{
		Body->SetSkeletalMesh(Mesh);
		MeshBoundsCenter = Mesh->GetBounds().Origin;
		MeshBoundsExtent = Mesh->GetBounds().BoxExtent;
	}
    const TCHAR* ClawClips[]={TEXT("Idle"),TEXT("Intimidate"),TEXT("Attack"),TEXT("HitReact"),TEXT("Defeated")};
    const TCHAR* KrakenClips[]={TEXT("idle"),TEXT("smashAttackForward"),TEXT("smashAttack"),TEXT("getHit"),TEXT("death")};
    for(int32 I=0;I<5;++I)
    {
        const FString Asset=Kraken?FString::Printf(TEXT("/Game/Kraken/Animations/KRAKEN_%s.KRAKEN_%s"),KrakenClips[I],KrakenClips[I]):
            FString::Printf(TEXT("/Game/Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimationClawKaiju_%s.ClawKaiju_allAnimationClawKaiju_%s"),ClawClips[I],ClawClips[I]);
        Poses.Add(LoadObject<UAnimSequence>(nullptr,*Asset,nullptr,LOAD_NoWarn|LOAD_Quiet));
    }
    if(!Kraken)
        if(auto* Skin=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/LeviathanSkin/M_LeviathanSkin.M_LeviathanSkin"),nullptr,LOAD_NoWarn|LOAD_Quiet))
            for(int32 Slot=0;Slot<Body->GetNumMaterials();++Slot) Body->SetMaterial(Slot,Skin);
    FActorSpawnParameters CombatParams; CombatParams.Owner=this;
    Combat=GetWorld()->SpawnActor<ADCBossCombat>(FVector::ZeroVector,FRotator::ZeroRotator,CombatParams);
	PainAudio->SetSound(LoadObject<USoundBase>(nullptr, TEXT("/Game/Sonniss/kraken_pain.kraken_pain"),
		nullptr, LOAD_NoWarn | LOAD_Quiet));
	if (UMaterialInterface* Skin = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Maelstrom/M_LeviathanWet.M_LeviathanWet")))
	{
		ArmMaterial = UMaterialInstanceDynamic::Create(Skin, this);
		ArmMaterial->SetVectorParameterValue(TEXT("Skin"), FLinearColor(0.035f, 0.048f, 0.045f));
		ArmMaterial->SetVectorParameterValue(TEXT("Belly"), FLinearColor(0.22f, 0.11f, 0.055f));
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
	UE_LOG(LogDC,Log,TEXT("Caldera boss: three response attacks, fixed finale %.1f, Kraken candidate=%d"),FinaleAt,Kraken);
}

void ADCLeviathan::ResetEncounter()
{
    PainAudio->Stop(); Arms->ClearAllMeshSections();
    for(FArm& Arm:ArmState) Arm=FArm();
    ArmsBroken=0; DefeatedAt=-1.f; bAnnounced=false; LastElapsed=-1.f;
    CurrentPose=INDEX_NONE; LastHitAt=-100.f; NextBodyContact=-100.f;
}

void ADCLeviathan::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetEncounter();
	if(Combat) { Combat->Destroy(); }
	Super::EndPlay(Reason);
}

void ADCLeviathan::UpdateEncounter(float Elapsed, const FTransform& Arena, float Radius, float Depth, bool bActive)
{
	if (!bAssetsReady) { return; }
	if(UpdateKrakenAudition(Elapsed,bActive)) { return; }
	const ADCRideDirector* Ride = ADCRideDirector::Get(this);
	const float ExpectedAdvance = GetWorld()->GetDeltaSeconds() * (Ride ? Ride->TimeScale : 1.f);
	const bool bSeeking = LastElapsed >= 0.f && (Elapsed < LastElapsed || FMath::Abs(Elapsed - LastElapsed - ExpectedAdvance) > 0.5f);
	if (bSeeking || (!bActive && bWasActive)) { ResetEncounter(); }
	bWasActive = bActive;

	if (!bActive)
	{
		SetActorHiddenInGame(true);
        if(Combat) Combat->Update(Elapsed,FinaleAt,Arena,Radius,false,bSeeking);
		LastElapsed = Elapsed;
		return;
	}
	SetActorHiddenInGame(Elapsed < FirstPeekAt);
	ADCRideDirector* Director = ADCRideDirector::Get(this);
	ShipPosition = FVector(0.f, 0.f, Director ? Director->SeaLevelZ : 0.f);
    // Set the fixed deadline before updating any target or pose, including direct finale seeks.
    const bool JustDefeated=DefeatedAt<0.f && Elapsed>=FinaleAt;
    if(JustDefeated) DefeatedAt=FinaleAt;
    UpdateBody(Elapsed,Arena);

    if(Combat) Combat->Update(Elapsed,FinaleAt,Arena,Radius,bActive,bSeeking);
    FVector Base,Bend,Joint,Tip;
    const bool Appendage=Combat && Combat->GetAppendagePose(Base,Bend,Joint,Tip) && Elapsed<FinaleAt;
    if(Appendage)
    {
        BuildArm(0,Base,Bend,Joint,Tip,Elapsed);
        if(auto* Stage=ADCMaelstrom::Get(this); Stage && ArmState[0].bHasWaterContact && Elapsed>=ArmState[0].NextSprayAt)
        { Stage->EmitWaterContact(ArmState[0].WaterContact,.8f); ArmState[0].NextSprayAt=Elapsed+.35f; }
    }
    Arms->SetMeshSectionVisible(0,Appendage);
    ArmsBroken=Combat?int(Combat->IsLimbSevered(0))+int(Combat->IsLimbSevered(1)):0;
	// The end comes on the clock, so the music can be cut to it. What the guns did is in the score.
	if (JustDefeated)
	{
		DefeatedAt = FinaleAt;
		if (PainAudio->Sound)
		{
			const ADCAudio* Mix = Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(), ADCAudio::StaticClass()));
			PainAudio->SetVolumeMultiplier(1.f * (Mix ? Mix->MasterVolume : 0.8f));
			PainAudio->SetPitchMultiplier(0.6f);
			PainAudio->Play();
		}
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(ArmsBroken >= 2 ? TEXT("It is letting go! We have broken it, Captain! Hold our course!") : TEXT("It is letting go! The sea is taking it back! Hold our course, Captain!"), true);
		}
		UE_LOG(LogDC, Log, TEXT("Leviathan: beaten on the clock at %.1f s after the breach, %d arms broken"), Elapsed, ArmsBroken);
	}
	if (!bAnnounced && Elapsed >= FirstPeekAt + 6.f)
	{
		bAnnounced = true;
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(TEXT("Tentacles ahead: steer clear or shoot their joints. Save a broadside for its mouth!"), true);
		}
	}
	LastElapsed = Elapsed;
}

float ADCLeviathan::GetCombatRoar() const
{
    return Combat ? Combat->GetRoar() : 0.f;
}

FVector ADCLeviathan::GetMouthPosition() const
{
    if(Body->DoesSocketExist(TEXT("KRAKEN_BEAK_UPPER")))
        return (Body->GetSocketLocation(TEXT("KRAKEN_BEAK_UPPER"))+Body->GetSocketLocation(TEXT("KRAKEN_BEAK_LOWER")))*.5f;
    return Body->Bounds.Origin+Body->GetForwardVector()*Body->Bounds.BoxExtent.X*.8f;
}

void ADCLeviathan::UpdateBody(float Elapsed,const FTransform& Arena)
{
    if(!Body->GetSkeletalMeshAsset()) return;
    const auto* Stage=ADCMaelstrom::Get(this);
    const float Radius=Stage?Stage->Radius:160000.f;
    // Swim around the inner lake on an authored orbit, independent of the helm offset.
    // A 30m reference footprint plus animation reserve stays clear even at full inward helm.
    const FVector2D Local=DCMaelstromRoute::Point(Elapsed,Radius,-18000.f);
    FVector Anchor=Arena.TransformPosition(FVector(Local.X,Local.Y,-40000.f));
    const float Water=Stage?Stage->SampleSurfaceWorld(Anchor):ShipPosition.Z;
    const float Horizontal=FVector2D(MeshBoundsExtent.X,MeshBoundsExtent.Y).Size();
    const float Scale=FMath::Min(5000.f/FMath::Max(1.f,static_cast<float>(MeshBoundsExtent.Z)*2.f),3000.f/FMath::Max(1.f,Horizontal));
    int32 Pose=0; float PoseTime=Elapsed-36.f;
    for(const auto& Event:DCBossTimeline::Events)
    {
        const float Since=Elapsed-DCBossTimeline::At(Event.At,FinaleAt);
        if(Event.Kind==DCBossTimeline::EAttack::Slam && Since>=0.f && Since<DCBossTimeline::SlamTell+1.f)
        { Pose=2; PoseTime=Since/DCBossTimeline::SlamTell*(Poses[2]?Poses[2]->GetPlayLength():1.f); }
    }
    if(Combat && Combat->IsMouthCharging()) { Pose=1; PoseTime=Poses[1]?Poses[1]->GetPlayLength()*.45f:0.f; }
    if(Combat && Elapsed-Combat->GetHitTime()<.7f) { Pose=3; PoseTime=Elapsed-Combat->GetHitTime(); }
    if(DefeatedAt>=0.f) { Pose=4; PoseTime=Elapsed-DefeatedAt; }
    if(Poses.IsValidIndex(Pose) && Poses[Pose])
    {
        if(CurrentPose!=Pose) { Body->PlayAnimation(Poses[Pose],Pose==0); Body->SetComponentTickEnabled(false); CurrentPose=Pose; }
        const float Length=FMath::Max(.01f,Poses[Pose]->GetPlayLength());
        Body->SetPosition(Pose==0?FMath::Fmod(FMath::Max(0.f,PoseTime),Length):FMath::Clamp(PoseTime,0.f,Length),false);
        Body->TickAnimation(0.f,false); Body->RefreshBoneTransforms();
    }
    float MouthBearing=0.f;
    if(Body->DoesSocketExist(TEXT("KRAKEN_BEAK_UPPER")))
    {
        const FVector LocalMouth=(Body->GetSocketTransform(TEXT("KRAKEN_BEAK_UPPER"),RTS_Component).GetLocation()
            +Body->GetSocketTransform(TEXT("KRAKEN_BEAK_LOWER"),RTS_Component).GetLocation())*.5f-MeshBoundsCenter;
        MouthBearing=FMath::RadiansToDegrees(FMath::Atan2(LocalMouth.Y,LocalMouth.X));
    }
    const FQuat Facing=FRotator(0.f,(ShipPosition-Anchor).Rotation().Yaw-MouthBearing,0.f).Quaternion();
    const float Rise=FMath::SmoothStep(36.f,44.f,Elapsed);
    const float Sink=DefeatedAt>=0.f?FMath::SmoothStep(0.f,9.f,Elapsed-DefeatedAt):0.f;
    FVector Position=Anchor-Facing.RotateVector(FVector(MeshBoundsCenter.X,MeshBoundsCenter.Y,0.f)*Scale);
    Position.Z=Water-(MeshBoundsCenter.Z+MeshBoundsExtent.Z*.3f)*Scale;
    Body->SetWorldLocationAndRotation(Position,Facing); Body->SetWorldScale3D(FVector(Scale));
    // Put the actual mouth above the lake and within a flat broadside arc; never target the crater floor.
    Body->AddWorldOffset(FVector(0,0,Water+1200.f-GetMouthPosition().Z-(1.f-Rise+Sink)*7000.f));
    Body->SetVisibility(Elapsed>=36.f && Sink<1.f,true); Body->SetHiddenInGame(Elapsed<36.f || Sink>=1.f,true);
    for(int32 I=0;I<2;++I)
    {
        const FName Bone=I==0?TEXT("KRAKEN_ARM_L"):TEXT("KRAKEN_ARM_R");
        if(Body->GetBoneIndex(Bone)==INDEX_NONE) continue;
        if(Combat && Combat->IsLimbSevered(I)) Body->HideBoneByName(Bone,EPhysBodyOp::PBO_None);
        else Body->UnHideBoneByName(Bone);
    }
    if(Combat && Combat->GetHitTime()>LastHitAt)
    {
        LastHitAt=Combat->GetHitTime();
        if(PainAudio->Sound)
        {
            const auto* Mix=Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(),ADCAudio::StaticClass()));
            PainAudio->SetVolumeMultiplier(.55f*(Mix?Mix->MasterVolume:.8f)); PainAudio->SetPitchMultiplier(.9f); PainAudio->Play();
        }
    }
    if(Stage && Elapsed>=36.f && Sink<1.f && Elapsed>=NextBodyContact)
    {
        for(int32 I=0;I<6;++I)
        {
            const float Angle=I*UE_TWO_PI/6.f;
            const FVector Contact=Anchor+FVector(FMath::Cos(Angle)*Horizontal*Scale*.7f,FMath::Sin(Angle)*Horizontal*Scale*.7f,0);
            const_cast<ADCMaelstrom*>(Stage)->EmitWaterContact(Contact,DefeatedAt>=0.f?2.2f:.7f);
        }
        NextBodyContact=Elapsed+.6f;
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
			const float RadiusHere=ArmWidthScale*FMath::Lerp(2200.f,130.f,FMath::Pow(U,.7f));
            const float Safe=FMath::Lerp(1400.f,650.f,FMath::SmoothStep(.86f,1.f,U))+RadiusHere;
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

#include "DCKrakenAudition.inl"
