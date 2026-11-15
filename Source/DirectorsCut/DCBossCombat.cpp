#include "DCBossCombat.h"
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
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"

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
    for(auto& S:Splinters) if(auto* V=S.Visual.Get()) V->Destroy();
    Splinters.Reset(); Previous=-1;
    if(ResetScore)
    {
        if(Director) { Director->Score-=AppliedScore; Director->ArmsBroken=0; }
        AppliedScore=0; BossDamage=ShipDamage=0; Interrupts=PerformanceScore=0;
        Severed[0]=Severed[1]=false; DamageBaseline=Director?Director->DamageTaken:0;
        HitAt=-100; Finished=false;
    }
}
void ADCBossCombat::Report(const FString& S)
{ if(auto* C=GetWorld()->GetSubsystem<UDCCrewSubsystem>()) C->Report(S,true); }
void ADCBossCombat::AddScore()
{
    PerformanceScore=DCBossTimeline::Score(BossDamage,int(Severed[0])+int(Severed[1]),Interrupts,ShipDamage);
    if(Director) Director->Score+=PerformanceScore-AppliedScore;
    AppliedScore=PerformanceScore;
}
float ADCBossCombat::AcceptTargetHit(float Damage,int32 Event)
{
    const float T=Director?Director->RideTime-RideOrigin:Deadline;
    if(!Running || !Director || Director->TimeScale<=0 || Event!=Attack.Index || Attack.Interrupted || Attack.Cancelled || Attack.Applied) return 0;
    const float Effective=DCBossTimeline::EffectiveDamage(Damage,Attack.Remaining,DCBossTimeline::Open(T,Attack.Start,Attack.Strike,Deadline));
    if(Effective<=0) return 0;
    Attack.Remaining-=Effective; BossDamage+=Effective; HitAt=T;
    if(Attack.Remaining<=0)
    {
        Attack.Interrupted=true; Attack.InterruptedAt=T; Attack.End=FMath::Min(Deadline,T+3.f); ++Interrupts;
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
    const float Side=P.Y<0?-1.f:1.f;
    const FVector Gun(0,Side*650.f,Director->SeaLevelZ+800.f);
    const FVector D=P-Gun;
    const float Dist=FVector2D(D.X,D.Y).Size();
    // Five degrees spare inside the captain's yaw/elevation limits and 10m spare in range.
    if(Dist<3000.f || Dist>17000.f || FMath::Abs(D.X)>FMath::Abs(D.Y)*1.54f || D.Z>Dist*.176327f || D.Z<-Dist*.36f) return false;
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
        Report(Attack.Side>0?TEXT("Tentacle ahead! Clear to port, or shoot the bright joint!"):TEXT("Tentacle ahead! Clear to starboard, or shoot the bright joint!"));
    }
    else if(Attack.Kind==DCBossTimeline::EAttack::Slam)
        Report(Attack.Side<0?TEXT("Port tentacle is drawing back! Shoot its joint before it slams!"):TEXT("Starboard tentacle is drawing back! Shoot its joint before it slams!"));
    else Report(Mouth().Y<0?TEXT("Its mouth is charging! Port battery, fire into the glow!"):TEXT("Its mouth is charging! Starboard battery, fire into the glow!"));
    UE_LOG(LogDC,Log,TEXT("BOSS_ATTACK event=%d kind=%d tell=%.2f strike=%.2f"),Index,int(Attack.Kind),Start,Attack.Strike);
}
void ADCBossCombat::Impact(const FVector& Contact,bool Slam)
{
    // StrikeHull supplies timber impact notifications, a hull impulse and exactly one damage application.
    Director->StrikeHull(Contact,Slam?1.3333333f:.6666667f);
    if(Slam) if(auto* T=ADCTroubles::Get(this)) T->TearRigging();
    Stage->EmitWaterContact(Contact,Slam?3.f:2.f);
    if(auto* Sound=Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(GetWorld(),ADCAudio::StaticClass()))) Sound->Boom(Slam?1.f:.6f);
    for(int I=0;I<12;++I)
    {
        auto* V=MakeMesh(Cube,Wet,Contact); if(!V) continue;
        V->SetActorScale3D(FVector(.12f,.12f,.5f));
        const float Angle=I*UE_TWO_PI/12.f;
        Splinters.Add({V,Contact,FVector(FMath::Cos(Angle)*650.f,FMath::Sin(Angle)*650.f,450.f+I*25.f),Now});
    }
    UE_LOG(LogDC,Log,TEXT("BOSS_CONTACT event=%d slam=%d t=%.2f position=%s"),Attack.Index,Slam,Now,*Contact.ToCompactString());
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
        PoseJoint=Anchor+Across*(Side*3500.f)+Along*1000.f+FVector(0,0,1400.f);
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
        const float Water=Stage->SampleSurfaceWorld(FVector(2000,Side*8000,0));
        PoseBase=FVector(2000,Side*10000,Water-5000);
        PoseJoint=FVector(2200,Side*7000,Water+1400);
        PoseBend=FVector(-1500,Side*8500,Water+3300);
        const float Tension=FMath::SmoothStep(Attack.Start+2.f,Attack.Strike-1.f,Now);
        PoseBend.Z+=Tension*550.f;
        const FVector Raised(800-500.f*Tension,Side*(6500.f+350.f*Tension),Water+2500.f+900.f*Tension);
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
        if(!Attack.Applied && !Attack.Interrupted && !Attack.Cancelled && DT>0 && Now>=Attack.Strike)
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
            Report(TEXT("Fire incoming! Brace and ready the buckets!"));
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
        T->SetActorLocation(Attack.Kind==DCBossTimeline::EAttack::Mouth?Mouth():PoseJoint);
        if(Attack.Kind!=DCBossTimeline::EAttack::Mouth)
            T->SetActorRotation(FRotationMatrix::MakeFromZ((PoseJoint-PoseBend).GetSafeNormal()).Rotator());
        if(auto* Light=T->FindComponentByClass<UPointLightComponent>()) Light->SetIntensity(15000.f+85000.f*FMath::Square(GetCharge()));
        // Review-only mouse aim. Real play has no camera steering or auto-aim.
        if(FParse::Param(FCommandLine::Get(),TEXT("DCBossReviewAim")))
            if(auto* PC=GetWorld()->GetFirstPlayerController())
                if(auto* Camera=Cast<ACameraActor>(PC->GetViewTarget()))
                    if(Camera->GetActorRightVector().X*T->GetActorLocation().Y<0)
                    {
                        FRotator Aim=(T->GetActorLocation()-Camera->GetActorLocation()).Rotation();
                        const float SideYaw=T->GetActorLocation().Y<0?-90.f:90.f;
                        Aim.Yaw=SideYaw+FMath::Clamp(FMath::FindDeltaAngleDegrees(SideYaw,Aim.Yaw),-62.f,62.f);
                        Aim.Pitch=FMath::Clamp(Aim.Pitch,-62.f,12.f);
                        Camera->SetActorRotation(Aim);
                    }
    }
    if(auto* MID=Attack.MID.Get())
    {
        const float Urgency=FMath::SmoothStep(Attack.Start,Attack.Strike,Now);
        const float Pulse=Now-HitAt<.2f?3.f:(Attack.Kind==DCBossTimeline::EAttack::Mouth?.4f+GetCharge()*1.6f:
            .8f+.25f*FMath::Sin(Now*(4.f+Urgency*5.f))+Urgency*.45f);
        MID->SetVectorParameterValue(TEXT("Color"),FLinearColor(1.2f,.28f,.07f)*Pulse);
    }
    if(PoseVisible && Now>Attack.LastSpray+.4f)
    {
        Stage->EmitWaterContact(PoseBase,Age<4.f?1.2f:.45f);
        if(Attack.Kind==DCBossTimeline::EAttack::Obstacle && Age<4.f)
            Stage->EmitWaterContact(Arena.TransformPosition(Attack.Anchor),1.f);
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
            Finished=true; ClearAttack();
            if(auto* T=ADCTroubles::Get(this)) T->ResetEncounterTroubles();
            UE_LOG(LogDC,Log,TEXT("BOSS_RESULT deadline=%.2f damage=%.1f taken=%.1f severed=%d interrupts=%d score=%d"),Deadline,BossDamage,ShipDamage,int(Severed[0])+int(Severed[1]),Interrupts,PerformanceScore);
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
    UpdateDamagedRigging(); Previous=Now;
}
void ADCBossCombat::DrawStatus(AHUD* HUD,UCanvas* Canvas)
{
    if(!HUD || HUD->GetWorld()!=GetWorld() || !Canvas || !GEngine || Now<DCBossTimeline::CombatStart || (!Running && !Finished) || FParse::Param(FCommandLine::Get(),TEXT("DCNoHUD"))) return;
    const float S=FMath::Clamp(Canvas->SizeY/900.f,.7f,1.5f),X=Canvas->SizeX*.5f-250.f*S,Y=90.f*S;
    HUD->DrawRect(FLinearColor(.015f,.025f,.035f,.8f),X-12.f*S,Y-8.f*S,520.f*S,78.f*S);
    HUD->DrawText(FString::Printf(TEXT("%s   |   %d / 2 LIMBS SEVERED"),Finished?TEXT("KRAKEN DEFEATED"):TEXT("KRAKEN"),int(Severed[0])+int(Severed[1])),FLinearColor(1,.87f,.55f),X,Y,GEngine->GetSmallFont(),S*1.3f);
    FString Cue=TEXT("Watch for tentacles. Ready the batteries and repair crew.");
    if(Attack.Index!=INDEX_NONE && !Attack.Cancelled)
    {
        if(Attack.Interrupted) Cue=Attack.Kind==DCBossTimeline::EAttack::Slam?TEXT("TENTACLE SEVERED - this limb stays broken"):TEXT("ATTACK INTERRUPTED");
        else if(Attack.Applied) Cue=Attack.Kind==DCBossTimeline::EAttack::Slam?TEXT("SLAM HIT - repair the hull and rigging"):TEXT("IMPACT - check the ship and ready the crew");
        else if(Attack.Kind==DCBossTimeline::EAttack::Obstacle) Cue=FString::Printf(TEXT("TENTACLE AHEAD: steer %s or shoot its joint"),Attack.Side>0?TEXT("PORT"):TEXT("STARBOARD"));
        else if(Attack.Kind==DCBossTimeline::EAttack::Slam) Cue=FString::Printf(TEXT("%s SLAM: joint needs %.0f hit(s)  %.0fs"),Attack.Side<0?TEXT("PORT"):TEXT("STARBOARD"),FMath::CeilToFloat(Attack.Remaining),FMath::Max(0.f,Attack.Strike-Now));
        else Cue=Attack.Released?TEXT("FIRE INCOMING - turn clear, ready the buckets"):FString::Printf(TEXT("MOUTH CHARGE: fire into the glow  %.0fs"),FMath::Max(0.f,Attack.Strike-Now));
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
}
