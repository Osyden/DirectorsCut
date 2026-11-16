#include "DCBossCombat.h"
#include "DCCombatCue.h"
#include "DCCombatCuePolicy.h"
#include "DCBossMarkerPolicy.h"
#include "DCBossDamagePolicy.h"
#include "DCGunnery.h"
#include "DCLeviathan.h"
#include "DCBossFireball.h"
#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
#include "DCTreadmill.h"
#include "DCCrew.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/HUD.h"
#include "ProceduralMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

ADCBossCombat::ADCBossCombat()
{
    PrimaryActorTick.bCanEverTick=false;
    LooseRopes=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("LooseRigging"));
    RootComponent=LooseRopes; LooseRopes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ADCBossCombat::BeginPlay()
{
    Super::BeginPlay(); Director=ADCRideDirector::Get(this); Stage=ADCMaelstrom::Get(this);
    Creature=Cast<ADCLeviathan>(GetOwner());
    Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Boss/M_BossTell.M_BossTell"));
    Wet=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_LeviathanWet.M_LeviathanWet"));
    LooseRopes->SetMaterial(0,Wet);
    HUDHandle=AHUD::OnHUDPostRender.AddUObject(this,&ADCBossCombat::DrawStatus);
}
void ADCBossCombat::EndPlay(const EEndPlayReason::Type Reason)
{ AHUD::OnHUDPostRender.Remove(HUDHandle); ResetEncounterState(false); Super::EndPlay(Reason); }
void ADCBossCombat::ClearAttack()
{
    for(auto* A:{Attack.Target.Get(),Attack.Projectile.Get()}) if(A) A->Destroy();
    Attack=FAttack(); PoseVisible=false;
}
void ADCBossCombat::ResetEncounterState(bool ResetScore)
{
    ClearAttack();
    if(auto* Target=BodyTarget.Get()) Target->Destroy();
    BodyTarget.Reset();
    for(auto& S:Splinters) if(auto* V=S.Visual.Get()) V->Destroy();
    Splinters.Reset(); Previous=-1;
    if(ResetScore)
    {
        if(Director) { Director->Score-=AppliedScore; Director->ArmsBroken=0; }
        AppliedScore=0; BossDamage=BodyDamage=ShipDamage=0; Interrupts=PerformanceScore=0;
        Severed[0]=Severed[1]=false; DamageBaseline=Director?Director->DamageTaken:0;
        HitAt=-100; HitMarkerText.Reset(); Finished=false;
        BodyHitAt=HitMarkerAt=LastContactAt=-100; LastContactDamage=0;
    }
}
void ADCBossCombat::Report(const FString& S)
{ if(auto* C=GetWorld()->GetSubsystem<UDCCrewSubsystem>()) C->Report(S,true); }
void ADCBossCombat::TryAttackWarning(bool Incoming)
{
    if(Attack.Index==INDEX_NONE || Now<Attack.NextWarningAt || (Incoming?Attack.IncomingReported:Attack.TelegraphReported)) return;
    const TWeakObjectPtr<ADCBossCombat> Weak(this);
    const TWeakObjectPtr<AActor> Subject=Incoming?Attack.Projectile:Attack.Target;
    const int32 Event=Attack.Index;
    const float Start=Attack.Start;
    const auto Kind=Attack.Kind;
    const bool Port=Director && Subject.IsValid() && Director->DeckTransform.InverseTransformPosition(Subject->GetActorLocation()).Y<0;
    auto StillRelevant=[Weak,Subject,Event,Start,Kind,Incoming,Port]()
    {
        const ADCBossCombat* Self=Weak.Get();
        if(!Self || !Self->Director || !Self->Running || Self->Finished || !Subject.IsValid()
            || Self->Director->GetPhase()!=EDCPhase::Maelstrom) return false;
        const FAttack& Live=Self->Attack;
        if(Live.Index!=Event || Live.Start!=Start || Live.Kind!=Kind || Live.Cancelled || Live.Applied || Live.Interrupted) return false;
        const float Time=Self->Director->RideTime-Self->RideOrigin;
        if(Incoming)
            return Live.Projectile==Subject && Live.Released && Time>=Live.Strike
                && Time<Live.Strike+DCBossFireball::Flight && Time<Self->Deadline;
        if(Live.Target!=Subject || Live.Released || !DCBossTimeline::Open(Time,Live.Start,Live.Strike,Self->Deadline)) return false;
        // If the ship turns enough to change the named battery, discard this
        // queued line rather than speaking a stale direction. Obstacle advice
        // names its committed clear lane, not the current screen/battery side.
        return Kind==DCBossTimeline::EAttack::Obstacle
            || (Self->Director->DeckTransform.InverseTransformPosition(Subject->GetActorLocation()).Y<0)==Port;
    };
    if(!StillRelevant()) return;
    FString Text;
    if(Incoming) Text=TEXT("Fire incoming! Brace, then smother the flames!");
    else if(Kind==DCBossTimeline::EAttack::Obstacle)
        Text=Attack.Side>0?TEXT("Tentacle ahead! Clear to port, or shoot the bright joint!"):TEXT("Tentacle ahead! Clear to starboard, or shoot the bright joint!");
    else if(Kind==DCBossTimeline::EAttack::Slam)
        Text=Port?TEXT("Port tentacle is drawing back! Shoot its joint before it slams!"):TEXT("Starboard tentacle is drawing back! Shoot its joint before it slams!");
    else Text=Port?TEXT("Its mouth is charging! Port battery, fire into the glow!"):TEXT("Its mouth is charging! Starboard battery, fire into the glow!");
    Attack.NextWarningAt=Now+.25f;
    if(auto* Crew=GetWorld()->GetSubsystem<UDCCrewSubsystem>())
    {
        const bool Accepted=Crew->Report(Text,true,MoveTemp(StillRelevant));
        if(Incoming) Attack.IncomingReported=Accepted; else Attack.TelegraphReported=Accepted;
    }
}
void ADCBossCombat::AddScore()
{
    PerformanceScore=DCBossTimeline::Score(BossDamage,int(Severed[0])+int(Severed[1]),Interrupts,ShipDamage)+FMath::RoundToInt(BodyDamage*25.f);
    if(Director) Director->Score+=PerformanceScore-AppliedScore;
    AppliedScore=PerformanceScore;
}
float ADCBossCombat::AcceptBodyHit(float Damage)
{
    const float Time=Director?Director->RideTime-RideOrigin:Deadline;
    const float Effective=DCBossDamagePolicy::BodyDamage(Damage,Time,DCBossTimeline::CombatStart,Deadline,Running,Finished,!Director || Director->TimeScale<=0);
    if(Effective<=0) return 0;
    BodyDamage+=Effective; BodyHitAt=Time;
    // A regular body shot reacts audibly/physically without interrupting a tell.
    // Throttle the body flinch to avoid rapid crew volleys pinning the animation.
    const bool Urgent=Attack.Index!=INDEX_NONE && !Attack.Interrupted && !Attack.Cancelled && !Attack.Applied && Time<Attack.Strike;
    if(!Urgent && Time-HitAt>.65f) HitAt=Time;
    AddScore();
    UE_LOG(LogDC,Log,TEXT("BOSS_BODY_HIT t=%.2f effective=%.1f body=%.1f total=%.1f"),Time,Effective,BodyDamage,BodyDamage+BossDamage);
    return Effective;
}
void ADCBossCombat::UpdateBodyTarget()
{
    if(!Running || Finished || Now<DCBossTimeline::CombatStart || Now>=Deadline)
    {
        if(auto* Target=BodyTarget.Get()) Target->Destroy();
        BodyTarget.Reset(); return;
    }
    // The face above the mouth is a separate, nonurgent hit region. Its sphere
    // cannot engulf the mouth interrupt sphere or steal a shot aimed into it.
    const FVector Position=Mouth()+FVector(0,0,DCBossDamagePolicy::BodyAboveMouth);
    if(!BodyTarget.IsValid())
    {
        auto* Target=MakeMesh(Sphere,nullptr,Position); if(!Target) return;
        Target->FindComponentByClass<UStaticMeshComponent>()->SetVisibility(false);
        Target->Tags.Add(TEXT("DCBossBodyTarget"));
        if(auto* T=ADCTreadmill::Get(this))
            T->AddExternalHostile(Target,DCBossDamagePolicy::BodyRadius,100000.f,false,FDCExternalDamage::CreateUObject(this,&ADCBossCombat::AcceptBodyHit));
        BodyTarget=Target;
        UE_LOG(LogDC,Log,TEXT("BOSS_BODY_TARGET position=%s radius=%.0f; cumulative damage, timed finale unchanged"),*Position.ToCompactString(),DCBossDamagePolicy::BodyRadius);
    }
    BodyTarget->SetActorLocation(Position);
}
bool ADCBossCombat::GetReviewTarget(bool Body,FVector& Position) const
{
    if(!Running || Finished || !Director || Director->TimeScale<=0) return false;
    const float Time=Director->RideTime-RideOrigin;
    const AActor* Target=Body?BodyTarget.Get():Attack.Target.Get();
    if(!IsValid(Target) || Time<DCBossTimeline::CombatStart || Time>=Deadline) return false;
    if(!Body && !DCBossMarkerPolicy::ShowTarget(true,DCBossTimeline::Open(Time,Attack.Start,Attack.Strike,Deadline),Attack.Interrupted,Attack.Cancelled,Attack.Applied)) return false;
    Position=Target->GetActorLocation();
    return true;
}
float ADCBossCombat::AcceptTargetHit(float Damage,int32 Event)
{
    const float T=Director?Director->RideTime-RideOrigin:Deadline;
    if(!Running || !Director || Director->TimeScale<=0 || Event!=Attack.Index || Attack.Interrupted || Attack.Cancelled || Attack.Applied) return 0;
    const float Effective=DCBossTimeline::EffectiveDamage(Damage,Attack.Remaining,DCBossTimeline::Open(T,Attack.Start,Attack.Strike,Deadline));
    if(Effective<=0) return 0;
    Attack.Remaining-=Effective; BossDamage+=Effective; HitAt=HitMarkerAt=T;
    HitMarkerPosition=Attack.Target.IsValid()?Attack.Target->GetActorLocation():PoseJoint;
    HitMarkerText=FString::Printf(TEXT("HIT!  %.0f MORE TO STOP IT"),FMath::CeilToFloat(Attack.Remaining));
    if(Attack.Remaining<=0)
    {
        Attack.Interrupted=true; Attack.InterruptedAt=T; Attack.End=FMath::Min(Deadline,T+3.f); ++Interrupts;
        if(auto* Cue=Attack.WeakPointCue.Get()) Cue->SetVisibility(false);
        HitMarkerText=Attack.Kind==DCBossTimeline::EAttack::Slam?TEXT("TENTACLE SEVERED!"):
            (Attack.Kind==DCBossTimeline::EAttack::Mouth?TEXT("FIRE CHARGE STOPPED!"):TEXT("TENTACLE CLEARED!"));
        if(Attack.Kind==DCBossTimeline::EAttack::Slam && Attack.Limb>=0)
        {
            Severed[Attack.Limb]=true; Director->ArmsBroken=int(Severed[0])+int(Severed[1]);
            Report(TEXT("Tentacle severed! That limb will not strike again!"));
        }
        else Report(Attack.Kind==DCBossTimeline::EAttack::Mouth?TEXT("Mouth hit! The fire is choking out!"):TEXT("Tentacle broken! The way is clear!"));
        UE_LOG(LogDC,Log,TEXT("BOSS_INTERRUPT event=%d t=%.2f limb=%d"),Event,T,Attack.Limb);
        if(Stage && Attack.Kind!=DCBossTimeline::EAttack::Mouth) Stage->EmitWaterContact(PoseJoint,2.f);
    }
    AddScore();
    UE_LOG(LogDC,Log,TEXT("BOSS_HIT event=%d t=%.2f effective=%.1f remaining=%.1f total=%.1f"),Event,T,Effective,Attack.Remaining,BossDamage);
    return Effective;
}
FVector ADCBossCombat::Mouth() const { return Creature?Creature->GetMouthPosition():FVector::ZeroVector; }
bool ADCBossCombat::CanReach(const FVector& P) const
{
    if(!Director || !Stage) return false;
    const auto* Guns=ADCGunnery::Get(this);
    const FVector LocalPoint=Director->DeckTransform.InverseTransformPosition(P);
    const float Side=LocalPoint.Y<0?-1.f:1.f;
    const int32 GunIndex=Guns?Guns->GetPlayerGun(Side<0?EDCSide::Port:EDCSide::Starboard):INDEX_NONE;
    const FVector Gun=GunIndex!=INDEX_NONE?Guns->GetMuzzle(GunIndex):Director->DeckToWorld(FVector(0,Side*650.f,Director->SeaLevelZ+800.f));
    const FVector D=P-Gun;
    const FVector Local=Director->DeckTransform.InverseTransformVectorNoScale(D);
    const float Dist=FVector2D(D.X,D.Y).Size();
    // Five degrees spare inside the captain's yaw/elevation limits and 5m spare in range.
    if(Dist<3000.f || Dist>20500.f || FMath::Abs(Local.X)>FMath::Abs(Local.Y)*1.54f || D.Z>Dist*.176327f || D.Z<-Dist*.36f) return false;
    for(int I=1;I<=16;++I)
    {
        const FVector Q=FMath::Lerp(Gun,P,I/16.f);
        if(Q.Z<Stage->SampleSurfaceWorld(Q)+100.f) return false;
    }
    return true;
}
AActor* ADCBossCombat::MakeMesh(UStaticMesh* Mesh,UMaterialInterface* Mat,const FVector& At)
{
    FActorSpawnParameters P; P.Owner=this; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(At,FRotator::ZeroRotator,P);
    if(!A) return nullptr;
    auto* C=A->GetStaticMeshComponent(); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Mesh);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(false); if(Mat) C->SetMaterial(0,Mat);
    return A;
}
void ADCBossCombat::MakeTarget(const FVector& P)
{
    AActor* A=MakeMesh(Sphere,Glow,P); if(!A) return;
    if(Attack.Kind==DCBossTimeline::EAttack::Obstacle) A->Tags.Add(TEXT("DCMovingWeakPoint"));
    A->GetRootComponent()->ComponentVelocity=FVector::ZeroVector; // Spawn/reconstruct never inherits a teleport velocity.
    // A flattened exposed band sits in the flesh; the generous invisible hit
    // radius remains forgiving while the visible target avoids a floating orb.
    A->SetActorScale3D(Attack.Kind==DCBossTimeline::EAttack::Mouth?FVector(2.4f):FVector(2.4f,2.4f,1.f));
    auto* C=A->FindComponentByClass<UStaticMeshComponent>(); C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    if(Glow)
    {
        auto* MID=UMaterialInstanceDynamic::Create(Glow,A); C->SetMaterial(0,MID); Attack.MID=MID;
    }
    if(Attack.Kind==DCBossTimeline::EAttack::Mouth)
    {
        auto* Light=NewObject<UPointLightComponent>(A); Light->SetupAttachment(C);
        Light->SetMobility(EComponentMobility::Movable); Light->SetLightColor(FLinearColor(1.f,.24f,.035f));
        Light->SetAttenuationRadius(1800.f); Light->SetCastShadows(false); Light->SetIntensity(0); Light->RegisterComponent();
    }
    else
    {
        // A visual child of the existing target: removing or severing that
        // target also removes this cue. No extra collision/aim target is made.
        Attack.WeakPointCue=UDCCombatCueComponent::Create(A,TEXT("ExposedJointBand"),true);
    }
    // Route appendages travel roughly 8m/s through the ship's frame. Allow
    // for that travel during a staggered cannon volley; stationary slam and
    // mouth targets retain the tighter hit region.
    const float HitRadius=Attack.Kind==DCBossTimeline::EAttack::Obstacle?1400.f:450.f;
    if(auto* T=ADCTreadmill::Get(this)) T->AddExternalHostile(A,HitRadius,Attack.Remaining,true,FDCExternalDamage::CreateUObject(this,&ADCBossCombat::AcceptTargetHit,Attack.Index));
    Attack.Target=A;
}
void ADCBossCombat::StartAttack(int32 Index,float Start,const FTransform& Arena,float Radius,bool Reconstruct)
{
    ClearAttack(); const auto& E=DCBossTimeline::Events[Index];
    Attack.Index=Index; Attack.Kind=E.Kind; Attack.Side=E.Side; Attack.Limb=E.Limb; Attack.Start=Start;
    if(E.Kind==DCBossTimeline::EAttack::Slam && IsLimbSevered(E.Limb)) { Attack.Kind=DCBossTimeline::EAttack::Mouth; Attack.Limb=INDEX_NONE; }
    Attack.Strike=Start+DCBossTimeline::Tell(Attack.Kind); Attack.End=Start+DCBossTimeline::Life(Attack.Kind);
    Attack.Remaining=DCBossTimeline::Health(Attack.Kind);
    if(Attack.Strike>=Deadline) { ClearAttack(); return; }
    if(Attack.Kind==DCBossTimeline::EAttack::Mouth && !CanReach(Mouth()))
    {
        UE_LOG(LogDC,Log,TEXT("BOSS_CHARGE_SKIPPED event=%d mouth=%s outside usable battery arc"),Index,*Mouth().ToCompactString());
        ClearAttack(); return;
    }
    const float Offset=FMath::Clamp(Stage->GetRouteOffset(),-6500.f,6500.f);
    const FVector2D P=DCMaelstromRoute::Point(Attack.Strike,Radius,Offset);
    Attack.Anchor=FVector(P.X,P.Y,-40000.f);
    Attack.Previous=Arena.TransformPosition(Attack.Anchor);
    // Replay reconstructs the pose without retroactive strikes or fire release.
    if(Reconstruct && Now>=Attack.Strike) Attack.Applied=Attack.Cancelled=true;
    UpdateAttack(0,Arena);
    if(Now<Attack.Strike) MakeTarget(Attack.Kind==DCBossTimeline::EAttack::Mouth?Mouth():PoseJoint);
    if(Attack.Kind==DCBossTimeline::EAttack::Obstacle)
    {
        // The tell comes from disturbed water at the committed root, not a neon disk.
        Stage->EmitWaterContact(Attack.Previous,1.5f);
    }
    TryAttackWarning(false);
    UE_LOG(LogDC,Log,TEXT("BOSS_ATTACK event=%d kind=%d tell=%.2f strike=%.2f"),Index,int(Attack.Kind),Start,Attack.Strike);
}
void ADCBossCombat::Impact(const FVector& Contact,bool Slam)
{
    // StrikeHull supplies timber impact notifications, a hull impulse and exactly one damage application.
    const float Before=Director->ShipHealth;
    Director->StrikeHull(Contact,Slam?DCBossDamagePolicy::SlamStrength:.6666667f);
    LastContactAt=Now; LastContactWasSlam=Slam; LastContactDamage=Before-Director->ShipHealth;
    if(Slam) if(auto* T=ADCTroubles::Get(this)) T->TearRigging();
    Stage->EmitWaterContact(Contact,Slam?3.f:2.f,!Slam);
    if(auto* Sound=Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(),ADCAudio::StaticClass()))) Sound->Boom(Slam?1.f:.6f);
    for(int I=0;I<12;++I)
    {
        auto* V=MakeMesh(Cube,Wet,Contact); if(!V) continue;
        V->SetActorScale3D(FVector(.12f,.12f,.5f));
        const float Angle=I*UE_TWO_PI/12.f;
        Splinters.Add({V,Contact,FVector(FMath::Cos(Angle)*650.f,FMath::Sin(Angle)*650.f,450.f+I*25.f),Now});
    }
    UE_LOG(LogDC,Log,TEXT("BOSS_CONTACT event=%d slam=%d t=%.2f position=%s hullBefore=%.1f hullAfter=%.1f damage=%.1f strength=%.2f"),Attack.Index,Slam,Now,*Contact.ToCompactString(),Before,Director->ShipHealth,LastContactDamage,Slam?DCBossDamagePolicy::SlamStrength:.6666667f);
}
void ADCBossCombat::UpdateAttack(float DT,const FTransform& Arena)
{
    if(Attack.Index==INDEX_NONE) return;
    const float Age=Now-Attack.Start,Side=float(Attack.Side);
    const float Recoil=Attack.Interrupted?FMath::SmoothStep(0.f,3.f,Now-Attack.InterruptedAt):0.f;
    const float Withdraw=FMath::Max(Recoil,FMath::SmoothStep(Attack.End-2.f,Attack.End,Now));
    if(Attack.Kind==DCBossTimeline::EAttack::Obstacle)
    {
        FVector Anchor=Arena.TransformPosition(Attack.Anchor); Anchor.Z=Stage->SampleSurfaceWorld(Anchor);
        // Offsets are transformed with the arena: the entire appendage stays committed after its tell.
        const FVector2D Tangent=DCMaelstromRoute::Tangent(Attack.Strike,Stage->Radius);
        const FVector Along=Arena.TransformVectorNoScale(FVector(Tangent.X,Tangent.Y,0));
        const FVector Across=Arena.TransformVectorNoScale(FVector(-Tangent.Y,Tangent.X,0));
        const float Rise=FMath::SmoothStep(1.f,4.f,Age);
        PoseBase=Anchor+Across*(Side*6500.f)-FVector(0,0,5000.f);
        // Keep the joint in the forward view and below the gun's elevation
        // limit; the tall curved shaft supplies the silhouette above the sails.
        PoseJoint=Anchor+Across*(Side*DCBossTimeline::ObstacleJointSide)+Along*1000.f+FVector(0,0,1400.f);
        PoseBend=Anchor+Across*(Side*4500.f)-Along*500.f+FVector(0,0,6500.f);
        // Uncoil after the initial water tell, then hold the obstruction steady.
        PoseTip=Anchor+Across*(Side*1800.f*(1.f-Rise))+FVector(0,0,1000.f+900.f*FMath::Sin(Rise*UE_PI));
        const float Submerge=(1.f-Rise)*6000.f+Withdraw*6500.f;
        PoseJoint.Z-=Submerge; PoseBend.Z-=Submerge; PoseTip.Z-=Submerge;
        PoseVisible=Withdraw<1.f;
        const FVector Bias=Across*(Side*DCBossTimeline::ObstacleSideBias);
        if(!Attack.Applied && !Attack.Interrupted && !Attack.Cancelled && DT>0 && Now>=Attack.Strike-3.f && Now<=Attack.Strike+3.f &&
            DCBossTimeline::SweptHull(Attack.Previous.X+Bias.X,Attack.Previous.Y+Bias.Y,Anchor.X+Bias.X,Anchor.Y+Bias.Y,DCBossTimeline::ObstacleHalfLength,DCBossTimeline::ObstacleHalfWidth))
        { Attack.Applied=true; Impact(FVector(2000.f,Side*450.f,Director->SeaLevelZ+450.f),false); }
        Attack.Previous=Anchor;
    }
    else if(Attack.Kind==DCBossTimeline::EAttack::Slam)
    {
        // A slam follows the ship, unlike a committed route obstacle. Keep its
        // held joint beside the actual battery as the ship yaws into a turn.
        const FVector JointDeck=Director->DeckToWorld(FVector(DCBossDamagePolicy::SlamJointForward,Side*DCBossDamagePolicy::SlamJointSide,Director->SeaLevelZ));
        const float Water=Stage->SampleSurfaceWorld(JointDeck);
        PoseBase=Director->DeckToWorld(FVector(2000,Side*10000,Director->SeaLevelZ)); PoseBase.Z=Water-5000;
        PoseJoint=JointDeck; PoseJoint.Z=Water+1300;
        PoseBend=Director->DeckToWorld(FVector(-1500,Side*8500,Director->SeaLevelZ)); PoseBend.Z=Water+3300;
        const float Tension=FMath::SmoothStep(Attack.Start+2.f,Attack.Strike-1.f,Now);
        // Pull the authored tip high and back, then hold the exposed joint
        // stationary throughout the cannon window. The last second is the downswing.
        PoseBend.Z+=Tension*1600.f;
        FVector Raised=Director->DeckToWorld(FVector(800-1800.f*Tension,Side*(6500.f+350.f*Tension),Director->SeaLevelZ));
        Raised.Z=Water+4200.f+1600.f*Tension;
        const FVector Contact=Director->DeckToWorld(FVector(800,Side*650,620));
        const float Swing=FMath::Square(FMath::Clamp(Now-(Attack.Strike-1.f),0.f,1.f));
        PoseTip=FMath::Lerp(Raised,Contact,Attack.Interrupted?0.f:Swing);
        if(Attack.Interrupted)
        {
            const float Kick=FMath::Sin(FMath::Clamp((Now-Attack.InterruptedAt)/.5f,0.f,1.f)*UE_PI)*900.f;
            PoseTip.Z+=Kick; PoseTip.Y+=Side*Kick*.5f;
        }
        const float Rise=FMath::SmoothStep(0.f,2.f,Age);
        const float Submerge=(1.f-Rise)*6500.f+Withdraw*7000.f;
        PoseJoint.Z-=Submerge; PoseBend.Z-=Submerge; PoseTip.Z-=Submerge;
        PoseVisible=Withdraw<1.f;
        if(!Attack.ReachLogged && Age>=2.05f && !Attack.Interrupted)
        {
            Attack.ReachLogged=true;
            UE_LOG(LogDC,Log,TEXT("BOSS_SLAM_READY event=%d t=%.2f joint=%s reachable=%d"),Attack.Index,Now,*PoseJoint.ToCompactString(),CanReach(PoseJoint));
        }
        if(DCBossDamagePolicy::SlamContact(Attack.Applied,Attack.Interrupted,Attack.Cancelled,DT,Now,Attack.Strike))
        { Attack.Applied=true; Impact(Contact,true); }
    }
    else
    {
        PoseVisible=false;
        if(!Attack.Released && !Attack.Interrupted && !Attack.Cancelled && !CanReach(Mouth()))
        {
            Attack.Cancelled=true;
            if(auto* T=Attack.Target.Get()) T->Destroy(); Attack.Target.Reset();
            UE_LOG(LogDC,Log,TEXT("BOSS_CHARGE_CANCELLED event=%d left usable cannon arc"),Attack.Index);
        }
        if(!Attack.Released && !Attack.Interrupted && !Attack.Cancelled && Now>=Attack.Strike && DT>0)
        {
            Attack.Released=true; Attack.FireFrom=Arena.InverseTransformPosition(Mouth());
            const FVector2D P=DCMaelstromRoute::Point(Now+DCBossFireball::Flight,Stage->Radius,Stage->GetRouteOffset());
            Attack.FireTo=FVector(P.X,P.Y,-40000.f+620.f);
            Attack.Projectile=DCBossFireball::Spawn(GetWorld(),this,Mouth());
            Attack.NextWarningAt=Now;
        }
        if(Attack.Released)
        {
            FVector To=Arena.TransformPosition(Attack.FireTo); To.Z=Director->SeaLevelZ+620.f;
            const float FlightAge=Now-Attack.Strike;
            if(auto* P=Attack.Projectile.Get())
            { P->SetActorLocation(DCBossFireball::Position(Arena.TransformPosition(Attack.FireFrom),To,FlightAge)); P->SetActorRotation(FRotator(FlightAge*260,FlightAge*170,0)); }
            if(!Attack.Applied && FlightAge>=DCBossFireball::Flight && DT>0)
            {
                Attack.Applied=true;
                if(FMath::Abs(To.X)<2200 && FMath::Abs(To.Y)<850) DCBossFireball::Impact(GetWorld(),Director,To);
                else Stage->EmitWaterContact(To,2.f);
                if(auto* P=Attack.Projectile.Get()) P->Destroy(); Attack.Projectile.Reset();
                UE_LOG(LogDC,Log,TEXT("BOSS_FIRE_IMPACT event=%d t=%.2f hit=%d"),Attack.Index,Now,FMath::Abs(To.X)<2200 && FMath::Abs(To.Y)<850);
            }
        }
    }
    if(Attack.Interrupted || Attack.Cancelled || Attack.Applied || Now>=Attack.Strike)
    { if(auto* T=Attack.Target.Get()) T->Destroy(); Attack.Target.Reset(); }
    else if(auto* T=Attack.Target.Get())
    {
        const FVector Before=T->GetActorLocation();
        T->SetActorLocation(Attack.Kind==DCBossTimeline::EAttack::Mouth?Mouth():PoseJoint);
        // Projectile flight/reload uses world seconds, whereas DT is ride time.
        // Publish world-space motion per world second, including helm rotation.
        const float WorldDT=GetWorld()->GetDeltaSeconds();
        const FVector Movement=T->GetActorLocation()-Before;
        const bool Continuous=DT>0.f && DT<.5f && WorldDT>UE_SMALL_NUMBER && Movement.SizeSquared()<FMath::Square(5000.f);
        T->GetRootComponent()->ComponentVelocity=Attack.Kind==DCBossTimeline::EAttack::Obstacle && Continuous
            ? (Movement/WorldDT).GetClampedToMaxSize(3500.f) : FVector::ZeroVector;
        if(Attack.Kind!=DCBossTimeline::EAttack::Mouth)
            T->SetActorRotation(FRotationMatrix::MakeFromZ((PoseJoint-PoseBend).GetSafeNormal()).Rotator());
        if(auto* Light=T->FindComponentByClass<UPointLightComponent>()) Light->SetIntensity(15000.f+85000.f*FMath::Square(GetCharge()));
    }
    TryAttackWarning(Attack.Released);
    if(auto* Cue=Attack.WeakPointCue.Get())
    {
        const bool Open=DCBossTimeline::Open(Now,Attack.Start,Attack.Strike,Deadline);
        const bool Visible=DCCombatCuePolicy::WeakPointVisible(Open,Attack.Kind==DCBossTimeline::EAttack::Mouth,
            Attack.Target.IsValid(),Attack.Interrupted,Attack.Cancelled,Attack.Applied);
        const float Progress=FMath::Clamp((Now-Attack.Start)/FMath::Max(Attack.Strike-Attack.Start,.01f),0.f,1.f);
        const float Flash=FMath::Clamp(1.f-(Now-HitMarkerAt)/.22f,0.f,1.f);
        // Same center and local axis as the authoritative cannon weak point.
        Cue->ShowWeakPoint(PoseJoint,(PoseJoint-PoseBend).GetSafeNormal(),180.f,Progress,Age,Flash,Visible);
    }
    if(auto* MID=Attack.MID.Get())
    {
        const float Urgency=FMath::SmoothStep(Attack.Start,Attack.Strike,Now);
        const float Pulse=Now-HitMarkerAt<.2f?3.f:(Attack.Kind==DCBossTimeline::EAttack::Mouth?.4f+GetCharge()*1.6f:
            .8f+.25f*FMath::Sin(Now*(4.f+Urgency*5.f))+Urgency*.45f);
        MID->SetVectorParameterValue(TEXT("Color"),Attack.Kind==DCBossTimeline::EAttack::Mouth
            ? FLinearColor(1.2f,.28f,.07f)*Pulse : FLinearColor(.62f,.32f,.075f)*FMath::Min(Pulse,1.2f));
    }
    if(PoseVisible && Now>Attack.LastSpray+.4f)
    {
        const bool FirstContact=Attack.LastSpray<Attack.Start;
        Stage->EmitWaterContact(PoseBase,Age<4.f?1.2f:.45f,true,FirstContact);
        if(Attack.Kind==DCBossTimeline::EAttack::Obstacle && Age<4.f)
            Stage->EmitWaterContact(Arena.TransformPosition(Attack.Anchor),1.f,true,false);
        Attack.LastSpray=Now;
    }
}
bool ADCBossCombat::GetAppendagePose(FVector& Base,FVector& Bend,FVector& Joint,FVector& Tip) const
{ Base=PoseBase; Bend=PoseBend; Joint=PoseJoint; Tip=PoseTip; return PoseVisible; }
bool ADCBossCombat::IsMouthCharging() const
{ return Attack.Index!=INDEX_NONE && Attack.Kind==DCBossTimeline::EAttack::Mouth && !Attack.Interrupted && !Attack.Cancelled && Now<Attack.Strike; }
float ADCBossCombat::GetCharge() const
{ return IsMouthCharging()?FMath::Clamp((Now-Attack.Start)/(Attack.Strike-Attack.Start),0.f,1.f):0.f; }
float ADCBossCombat::GetRoar() const { return IsMouthCharging()?GetCharge()*.7f:0.f; }

void ADCBossCombat::UpdateDamagedRigging()
{
    const auto* Trouble=ADCTroubles::Get(this);
    const bool Show=Running && !Finished && Trouble && Trouble->IsRiggingLoose();
    LooseRopes->SetVisibility(Show);
    if(!Show || !Director) return;
    TArray<FVector> Vertices,Normals; TArray<FVector2D> UV; TArray<int32> Triangles;
    TArray<FColor> Colors; TArray<FProcMeshTangent> Tangents;
    // Four slack shrouds hanging from the mast, in deck space. They follow ship rocking.
    for(int32 Rope=0;Rope<4;++Rope)
    {
        const float Side=Rope%2?-1.f:1.f;
        for(int32 J=0;J<=16;++J)
        {
            const float U=J/16.f;
            FVector P=FMath::Lerp(FVector(350.f,Side*80.f,2800.f-Rope*160.f),FVector(1300.f-Rope*200.f,Side*450.f,780.f),U);
            P.Z-=FMath::Sin(U*UE_PI)*450.f;
            P.Y+=FMath::Sin(U*UE_PI)*FMath::Sin(Now*2.f+Rope)*120.f;
            for(int32 Edge=0;Edge<2;++Edge)
            {
                const FVector W=Director->DeckToWorld(P+FVector(Edge?12.f:-12.f,0,0));
                Vertices.Add(LooseRopes->GetComponentTransform().InverseTransformPosition(W));
                Normals.Add(FVector::RightVector); UV.Add(FVector2D(U*8,Edge)); Colors.Add(FColor::White);
                Tangents.Add(FProcMeshTangent(FVector::ForwardVector,false));
            }
            if(J<16)
            {
                const int32 A=Rope*34+J*2;
                Triangles.Append({A,A+2,A+1,A+1,A+2,A+3,A+1,A+2,A,A+3,A+2,A+1});
            }
        }
    }
    if(!LooseRopes->GetProcMeshSection(0)) LooseRopes->CreateMeshSection(0,Vertices,Triangles,Normals,UV,Colors,Tangents,false);
    else LooseRopes->UpdateMeshSection(0,Vertices,Normals,UV,Colors,Tangents);
}

void ADCBossCombat::Update(float Time,float Finale,const FTransform& Arena,float Radius,bool Active,bool Seeking)
{
    if(!Director || !Stage) return;
    const bool Reconstruct=Seeking || (Active && !Running);
    if(Reconstruct)
    {
        ResetEncounterState(true);
        if(auto* T=ADCTroubles::Get(this); T && (Running || Active)) T->ResetEncounterTroubles();
    }
    if(!Active)
    {
        if(Running) ResetEncounterState(false);
        Running=false; Now=Time; UpdateDamagedRigging(); return;
    }
    Running=true; Now=Time; Deadline=Finale; RideOrigin=Director->RideTime-Time;
    const float DT=Previous>=0 && !Seeking?FMath::Max(0.f,Now-Previous):0.f;
    if(Now<DCBossTimeline::CombatStart) DamageBaseline=Director->DamageTaken;
    if(!Finished && Now>=DCBossTimeline::CombatStart)
    { ShipDamage=FMath::Max(0.f,Director->DamageTaken-DamageBaseline); AddScore(); }
    if(Now>=Deadline)
    {
        if(!Finished)
        {
            Finished=true; ClearAttack(); UpdateBodyTarget();
            if(auto* T=ADCTroubles::Get(this)) T->ResetEncounterTroubles();
            UE_LOG(LogDC,Log,TEXT("BOSS_RESULT deadline=%.2f damage=%.1f taken=%.1f severed=%d interrupts=%d score=%d"),Deadline,BossDamage,ShipDamage,int(Severed[0])+int(Severed[1]),Interrupts,PerformanceScore);
            UE_LOG(LogDC,Log,TEXT("BOSS_BODY_RESULT damage=%.1f total=%.1f"),BodyDamage,BodyDamage+BossDamage);
        }
        UpdateDamagedRigging(); Previous=Now; return;
    }
    for(int32 I=Splinters.Num()-1;I>=0;--I)
    {
        const float Age=Now-Splinters[I].At;
        if(auto* V=Splinters[I].Visual.Get())
        {
            V->SetActorLocation(Splinters[I].From+Splinters[I].Velocity*Age-FVector(0,0,600.f*Age*Age));
            V->SetActorRotation(FRotator(Age*160.f,Age*230.f,I*29.f));
            if(Age>1.8f) V->Destroy();
        }
        if(Age>1.8f) Splinters.RemoveAt(I);
    }
    if(Reconstruct)
        for(int32 I=0;I<UE_ARRAY_COUNT(DCBossTimeline::Events);++I)
        {
            const auto& E=DCBossTimeline::Events[I]; const float At=DCBossTimeline::At(E.At,Deadline);
            if(Now>=At && Now<At+DCBossTimeline::Life(E.Kind)) StartAttack(I,At,Arena,Radius,true);
        }
    if(Attack.Index!=INDEX_NONE)
    {
        UpdateAttack(DT,Arena);
        if(Now>=Attack.End) ClearAttack();
    }
    if(DT>0)
        for(int32 I=0;I<UE_ARRAY_COUNT(DCBossTimeline::Events);++I)
        {
            const float At=DCBossTimeline::At(DCBossTimeline::Events[I].At,Deadline);
            if(DCBossTimeline::Crossed(Previous,Now,At)) StartAttack(I,At,Arena,Radius,false);
        }
    UpdateBodyTarget(); UpdateDamagedRigging(); Previous=Now;
}
void ADCBossCombat::DrawStatus(AHUD* HUD,UCanvas* Canvas)
{
    if(!HUD || HUD->GetWorld()!=GetWorld() || !Canvas || !GEngine || Now<DCBossTimeline::CombatStart || (!Running && !Finished) || FParse::Param(FCommandLine::Get(),TEXT("DCNoHUD"))) return;
    const float S=FMath::Clamp(Canvas->SizeY/900.f,.7f,1.5f),X=Canvas->SizeX*.5f-250.f*S,Y=90.f*S;
    HUD->DrawRect(FLinearColor(.015f,.025f,.035f,.8f),X-12.f*S,Y-8.f*S,520.f*S,78.f*S);
    HUD->DrawText(FString::Printf(TEXT("%s | %.0f DAMAGE | %d / 2 LIMBS"),Finished?TEXT("KRAKEN DEFEATED"):TEXT("KRAKEN"),BodyDamage+BossDamage,int(Severed[0])+int(Severed[1])),FLinearColor(1,.87f,.55f),X,Y,GEngine->GetSmallFont(),S*1.3f);
    FString Cue=TEXT("Keep firing at the body. Stop its marked attacks first.");
    if(Attack.Index!=INDEX_NONE && !Attack.Cancelled)
    {
        if(Attack.Interrupted) Cue=Attack.Kind==DCBossTimeline::EAttack::Slam?TEXT("TENTACLE SEVERED - this limb stays broken"):TEXT("ATTACK INTERRUPTED");
        else if(Attack.Applied) Cue=Attack.Kind==DCBossTimeline::EAttack::Slam?TEXT("SLAM HIT - check hull and repair rigging"):TEXT("IMPACT - check the ship and ready the crew");
        else if(Attack.Kind==DCBossTimeline::EAttack::Obstacle) Cue=FString::Printf(TEXT("TENTACLE AHEAD: steer %s or shoot its joint"),Attack.Side>0?TEXT("PORT"):TEXT("STARBOARD"));
        else if(Attack.Kind==DCBossTimeline::EAttack::Slam) Cue=FString::Printf(TEXT("%s SLAM: joint needs %.0f hit(s)  %.0fs"),Attack.Side<0?TEXT("PORT"):TEXT("STARBOARD"),FMath::CeilToFloat(Attack.Remaining),FMath::Max(0.f,Attack.Strike-Now));
        else Cue=Attack.Released?TEXT("FIRE INCOMING - turn clear, then smother the flames"):FString::Printf(TEXT("MOUTH CHARGE: fire into the glow  %.0fs"),FMath::Max(0.f,Attack.Strike-Now));
    }
    if(Finished) Cue=FString::Printf(TEXT("%d limbs severed   %d attacks interrupted"),int(Severed[0])+int(Severed[1]),Interrupts);
    HUD->DrawText(Cue,FLinearColor::White,X,Y+28.f*S,GEngine->GetSmallFont(),S*1.3f);
    if(Attack.Index!=INDEX_NONE && !Attack.Cancelled && !Attack.Interrupted && !Attack.Applied && Now<Attack.Strike)
    {
        const float Contact=Attack.Strike-(Attack.Kind==DCBossTimeline::EAttack::Obstacle?3.f:0.f);
        const float Remaining=FMath::Clamp((Contact-Now)/(Contact-Attack.Start),0.f,1.f);
        HUD->DrawRect(FLinearColor(.18f,.16f,.13f),X,Y+56.f*S,490.f*S,4.f*S);
        HUD->DrawRect(FLinearColor(1.f,.45f,.12f),X,Y+56.f*S,490.f*S*Remaining,4.f*S);
    }
    // Explicit screen-space guidance is separate from the depth-tested world
    // band: it remains legible beside the exact weak point through rigging.
    const bool ShowTarget=DCBossMarkerPolicy::ShowTarget(Attack.Target.IsValid(),
        DCBossTimeline::Open(Now,Attack.Start,Attack.Strike,Deadline),Attack.Interrupted,Attack.Cancelled,Attack.Applied);
    const bool RecentHit=!HitMarkerText.IsEmpty() && Now-HitMarkerAt>=0 && Now-HitMarkerAt<1.1f && !Finished;
    // A partial crew/player hit must never replace the remaining urgent target.
    const bool ShowHit=DCBossDamagePolicy::ShowHitInsteadOfTarget(ShowTarget,RecentHit,Attack.Interrupted);
    APlayerController* PC=HUD->GetOwningPlayerController();
    if(ShowTarget && Attack.Kind==DCBossTimeline::EAttack::Slam)
    {
        const FLinearColor Alert(1.f,.45f,.13f);
        HUD->DrawRect(FLinearColor(.09f,.018f,.008f,.96f),X-12*S,Y+73*S,520*S,49*S);
        HUD->DrawText(FString::Printf(TEXT("%s SLAM - SHOOT THE MARKED JOINT"),Attack.Side<0?TEXT("PORT"):TEXT("STARBOARD")),Alert,X,Y+77*S,GEngine->GetSmallFont(),1.25f*S);
        HUD->DrawText(FString::Printf(TEXT("%.1fs  |  %.0f hit(s) to stop  |  10 hull damage if missed"),FMath::Max(0.f,Attack.Strike-Now),FMath::CeilToFloat(Attack.Remaining)),FLinearColor::White,X,Y+99*S,GEngine->GetSmallFont(),S);
    }
    if(LastContactWasSlam && Now-LastContactAt>=0 && Now-LastContactAt<2.5f)
    {
        const float Flash=FMath::Clamp(1.f-(Now-LastContactAt)/.45f,0.f,1.f);
        HUD->DrawRect(FLinearColor(.45f,.08f,.015f,Flash*.18f),0,0,Canvas->SizeX,Canvas->SizeY);
        HUD->DrawRect(FLinearColor(.08f,.015f,.008f,.94f),X-12*S,Y+73*S,520*S,49*S);
        HUD->DrawText(FString::Printf(TEXT("SLAM IMPACT  -%.0f HULL"),LastContactDamage),FLinearColor(1.f,.42f,.15f),X,Y+77*S,GEngine->GetSmallFont(),1.4f*S);
        HUD->DrawText(TEXT("RIGGING RUPTURED - order a crew member to repair"),FLinearColor::White,X,Y+100*S,GEngine->GetSmallFont(),S);
    }
    if(BodyTarget.IsValid() && PC && PC->PlayerCameraManager)
    {
        FVector2D BodyScreen=FVector2D::ZeroVector, WeakScreen;
        const bool Projected=PC->ProjectWorldLocationToScreen(BodyTarget->GetActorLocation(),BodyScreen);
        if(!Projected)
        {
            const FVector Delta=BodyTarget->GetActorLocation()-PC->PlayerCameraManager->GetCameraLocation();
            const FRotationMatrix View(PC->PlayerCameraManager->GetCameraRotation());
            BodyScreen=FVector2D(FVector::DotProduct(Delta,View.GetUnitAxis(EAxis::Y))<0?-100.f:Canvas->SizeX+100.f,Canvas->SizeY*.45f);
        }
        const bool NearWeak=Projected && ShowTarget && PC->ProjectWorldLocationToScreen(Attack.Target->GetActorLocation(),WeakScreen)
            && FVector2D::Distance(BodyScreen,WeakScreen)<145.f*S;
        const auto BodyPlacement=DCBossMarkerPolicy::PlaceBody(BodyScreen.X,BodyScreen.Y,Projected,Canvas->SizeX,Canvas->SizeY,S,ShowTarget);
        const FVector2D UnclampedBody=BodyScreen;
        BodyScreen=FVector2D(BodyPlacement.X,BodyPlacement.Y);
        if(!NearWeak)
        {
            const FLinearColor C=Now-BodyHitAt<.6f?FLinearColor(.45f,1.f,.75f):FLinearColor(.65f,.87f,.90f,.85f);
            const float R=17.f*S;
            for(int Sign:{-1,1})
            {
                HUD->DrawLine(BodyScreen.X-R,BodyScreen.Y,BodyScreen.X,BodyScreen.Y+Sign*R,FLinearColor::Black,5*S);
                HUD->DrawLine(BodyScreen.X,BodyScreen.Y+Sign*R,BodyScreen.X+R,BodyScreen.Y,FLinearColor::Black,5*S);
                HUD->DrawLine(BodyScreen.X-R,BodyScreen.Y,BodyScreen.X,BodyScreen.Y+Sign*R,C,2*S);
                HUD->DrawLine(BodyScreen.X,BodyScreen.Y+Sign*R,BodyScreen.X+R,BodyScreen.Y,C,2*S);
            }
            if(BodyPlacement.AtEdge)
            {
                const FVector2D D=(UnclampedBody-BodyScreen).GetSafeNormal(),Side(-D.Y,D.X);
                const FVector2D Tip=BodyScreen+D*12*S,Tail=BodyScreen-D*6*S;
                for(int Sign:{-1,1})
                {
                    const FVector2D End=Tail+Side*Sign*9*S;
                    HUD->DrawLine(Tip.X,Tip.Y,End.X,End.Y,FLinearColor::Black,5*S);
                    HUD->DrawLine(Tip.X,Tip.Y,End.X,End.Y,C,2*S);
                }
            }
            const bool Port=Director->DeckTransform.InverseTransformPosition(BodyTarget->GetActorLocation()).Y<0;
            const FString Label=Now-BodyHitAt<.6f?TEXT("BODY HIT"):(ShowTarget?TEXT("BODY"):
                FString::Printf(TEXT("BODY - %s CANNONS"),Port?TEXT("PORT"):TEXT("STARBOARD")));
            const float LabelX=FMath::Clamp(static_cast<float>(BodyScreen.X-100*S),20*S,Canvas->SizeX-238*S);
            HUD->DrawRect(FLinearColor(.012f,.022f,.03f,.8f),LabelX,BodyScreen.Y+22*S,218*S,25*S);
            HUD->DrawText(Label,C,LabelX+7*S,BodyScreen.Y+25*S,GEngine->GetSmallFont(),S);
        }
    }
    if((ShowTarget || ShowHit) && PC && PC->PlayerCameraManager)
    {
        FVector Point=ShowHit?HitMarkerPosition:Attack.Target->GetActorLocation();
        const bool Rising=ShowTarget && Attack.Kind!=DCBossTimeline::EAttack::Mouth && Stage
            && Point.Z<Stage->SampleSurfaceWorld(Point)+80.f;
        if(Rising) { Point.Z=Stage->SampleSurfaceWorld(Point)+150.f; }
        FVector2D Screen;
        const bool Projected=PC->ProjectWorldLocationToScreen(Point,Screen);
        if(!Projected)
        {
            const FVector Delta=Point-PC->PlayerCameraManager->GetCameraLocation();
            const FRotationMatrix View(PC->PlayerCameraManager->GetCameraRotation());
            const float Side=FVector::DotProduct(Delta,View.GetUnitAxis(EAxis::Y));
            Screen=FVector2D(Side<0?-100.f:Canvas->SizeX+100.f,Canvas->SizeY*.45f);
        }
        auto P=DCBossMarkerPolicy::Place(Screen.X,Screen.Y,Projected,Canvas->SizeX,Canvas->SizeY,S);
        if(Attack.Kind==DCBossTimeline::EAttack::Slam) P.LabelY=FMath::Max(P.LabelY,235.f*S);
        const float Seconds=FMath::Max(0.f,Attack.Strike-Now);
        const FLinearColor Color=ShowHit?FLinearColor(.45f,1.f,.75f):
            FMath::Lerp(FLinearColor(1.f,.78f,.30f),FLinearColor(1.f,.25f,.08f),DCBossMarkerPolicy::Urgency(Seconds));
        const float R=(32.f+2.f*FMath::Sin(Now*5.f))*S;
        auto Line=[&](float AX,float AY,float BX,float BY)
        {
            HUD->DrawLine(AX,AY,BX,BY,FLinearColor(.015f,.025f,.03f),6.f*S);
            HUD->DrawLine(AX,AY,BX,BY,Color,2.5f*S);
        };
        if(!ShowHit)
        {
            for(int SX:{-1,1}) for(int SY:{-1,1})
            {
                Line(P.X+SX*R,P.Y+SY*R,P.X+SX*(R-13*S),P.Y+SY*R);
                Line(P.X+SX*R,P.Y+SY*R,P.X+SX*R,P.Y+SY*(R-13*S));
            }
            if(P.AtEdge)
            {
                const FVector2D Direction=(Screen-FVector2D(P.X,P.Y)).GetSafeNormal();
                const FVector2D Perpendicular(-Direction.Y,Direction.X);
                const FVector2D Tip=FVector2D(P.X,P.Y)+Direction*(18*S);
                const FVector2D Tail=FVector2D(P.X,P.Y)-Direction*(8*S);
                Line(Tip.X,Tip.Y,Tail.X+Perpendicular.X*13*S,Tail.Y+Perpendicular.Y*13*S);
                Line(Tip.X,Tip.Y,Tail.X-Perpendicular.X*13*S,Tail.Y-Perpendicular.Y*13*S);
            }
        }
        Line(P.X,P.Y-R,P.LabelX+14*S,P.LabelY+60*S);
        HUD->DrawRect(FLinearColor(.012f,.022f,.03f,.94f),P.LabelX,P.LabelY,288*S,66*S);
        HUD->DrawRect(Color,P.LabelX,P.LabelY,4*S,66*S);
        FString Title=HitMarkerText, Instruction;
        if(!ShowHit)
        {
            const bool Port=Director->DeckTransform.InverseTransformPosition(Point).Y<0;
            const TCHAR* Battery=Port?TEXT("PORT"):TEXT("STARBOARD");
            const auto* Guns=ADCGunnery::Get(this);
            const bool InCannonArc=Guns && Guns->CanAimGun(Guns->GetPlayerGun(Port?EDCSide::Port:EDCSide::Starboard),Attack.Target->GetActorLocation());
            Title=Rising?TEXT("!  TENTACLE RISING"):(Attack.Kind==DCBossTimeline::EAttack::Mouth?TEXT("SHOOT THE MOUTH"):
                (Attack.Kind==DCBossTimeline::EAttack::Slam?TEXT("SHOOT SLAM JOINT"):TEXT("SHOOT THIS JOINT")));
            if(P.AtEdge) Title=Rising?FString::Printf(TEXT("%s: TENTACLE RISING"),Battery):
                FString::Printf(TEXT("SHOOT %s %s"),Battery,Attack.Kind==DCBossTimeline::EAttack::Mouth?TEXT("MOUTH"):TEXT("JOINT"));
            Instruction=Rising?FString::Printf(TEXT("Prepare %s cannon"),Battery):
                FString::Printf(TEXT("%.0f HIT%s  |  %.1fs TO STOP IT"),FMath::CeilToFloat(Attack.Remaining),Attack.Remaining>1?TEXT("S"):TEXT(""),Seconds);
            if(RecentHit && !Rising) Instruction=FString::Printf(TEXT("HIT! %.0f MORE  |  %.1fs TO STOP IT"),FMath::CeilToFloat(Attack.Remaining),Seconds);
            if(Attack.Kind==DCBossTimeline::EAttack::Obstacle)
                Instruction=FString::Printf(TEXT("%.0f HIT%s or STEER %s | %.1fs"),FMath::CeilToFloat(Attack.Remaining),Attack.Remaining>1?TEXT("S"):TEXT(""),Attack.Side>0?TEXT("PORT"):TEXT("STARBOARD"),FMath::Max(0.f,Seconds-3.f));
            if(!Rising && (P.AtEdge || !InCannonArc))
            {
                const TCHAR* Kind=Attack.Kind==DCBossTimeline::EAttack::Mouth?TEXT("MOUTH"):
                    Attack.Kind==DCBossTimeline::EAttack::Slam?TEXT("SLAM JOINT"):TEXT("TENTACLE JOINT");
                Title=FString::Printf(TEXT("%s: %s"),Battery,Kind);
                if(Attack.Kind==DCBossTimeline::EAttack::Obstacle)
                    Instruction=FString::Printf(TEXT("STEER %s or prepare cannon | %.1fs"),Attack.Side>0?TEXT("PORT"):TEXT("STARBOARD"),FMath::Max(0.f,Seconds-3.f));
                else Instruction=InCannonArc?FString::Printf(TEXT("USE %s CANNONS | %.1fs"),Battery,Seconds):
                    FString::Printf(TEXT("WAIT FOR CANNON SHOT | %.1fs"),Seconds);
            }
        }
        else Instruction=Attack.Interrupted?TEXT("ATTACK STOPPED - READY NEXT GUN"):TEXT("RELOAD OR ORDER CREW TO FIRE");
        HUD->DrawText(Title,Color,P.LabelX+13*S,P.LabelY+9*S,GEngine->GetSmallFont(),1.35f*S);
        HUD->DrawText(Instruction,FLinearColor::White,P.LabelX+13*S,P.LabelY+35*S,GEngine->GetSmallFont(),1.04f*S);
    }
}
