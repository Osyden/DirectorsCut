#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCBossTimeline.h"
#include "DCBossCombat.generated.h"

class ADCLeviathan;
class ADCMaelstrom;
class ADCRideDirector;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class AHUD;
class UCanvas;

/** One urgent response at a time: avoid/clear an obstacle, interrupt a slam, interrupt a mouth charge. */
UCLASS()
class ADCBossCombat : public AActor
{
    GENERATED_BODY()
public:
    ADCBossCombat();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void Update(float Time,float Finale,const FTransform& Arena,float Radius,bool Active,bool Seeking);
    void ResetEncounterState(bool ResetScore);
    float GetHitTime() const { return HitAt; }
    float GetRoar() const;
    bool IsLimbSevered(int32 Index) const { return Index>=0 && Index<2 && Severed[Index]; }
    bool IsMouthCharging() const;
    float GetCharge() const;
    bool GetAppendagePose(FVector& Base,FVector& Bend,FVector& Joint,FVector& Tip) const;
    UPROPERTY(BlueprintReadOnly) float BossDamage=0.f;
    UPROPERTY(BlueprintReadOnly) float BodyDamage=0.f;
    UPROPERTY(BlueprintReadOnly) float ShipDamage=0.f;
    UPROPERTY(BlueprintReadOnly) int32 Interrupts=0;
    UPROPERTY(BlueprintReadOnly) int32 PerformanceScore=0;

private:
    struct FAttack
    {
        DCBossTimeline::EAttack Kind=DCBossTimeline::EAttack::Obstacle;
        int32 Index=INDEX_NONE,Side=1,Limb=INDEX_NONE;
        float Start=0,Strike=0,End=0,Remaining=0,InterruptedAt=-1,LastSpray=-100;
        bool Applied=false,Interrupted=false,Cancelled=false,Released=false,ReachLogged=false;
        FVector Anchor=FVector::ZeroVector,Previous=FVector::ZeroVector;
        FVector FireFrom=FVector::ZeroVector,FireTo=FVector::ZeroVector;
        TWeakObjectPtr<AActor> Target,Projectile;
        TWeakObjectPtr<UMaterialInstanceDynamic> MID;
        TWeakObjectPtr<class UDCCombatCueComponent> WeakPointCue;
    } Attack;
    struct FSplinter { TWeakObjectPtr<AActor> Visual; FVector From,Velocity; float At; };
    TArray<FSplinter> Splinters;
    float AcceptTargetHit(float Damage,int32 Event);
    float AcceptBodyHit(float Damage);
    void UpdateBodyTarget();
    void StartAttack(int32 Index,float Start,const FTransform& Arena,float Radius,bool Reconstruct);
    void UpdateAttack(float DT,const FTransform& Arena);
    void ClearAttack();
    AActor* MakeMesh(UStaticMesh* Mesh,UMaterialInterface* Material,const FVector& Position);
    void MakeTarget(const FVector& Position);
    void Impact(const FVector& Contact,bool Slam);
    void DrawStatus(AHUD* HUD,UCanvas* Canvas);
    void Report(const FString& Message);
    void AddScore();
    void UpdateDamagedRigging();
    bool CanReach(const FVector& Point) const;
    FVector Mouth() const;

    UPROPERTY() TObjectPtr<ADCRideDirector> Director;
    UPROPERTY() TObjectPtr<ADCMaelstrom> Stage;
    UPROPERTY() TObjectPtr<ADCLeviathan> Creature;
    UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UMaterialInterface> Glow;
    UPROPERTY() TObjectPtr<UMaterialInterface> Wet;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> LooseRopes;
    FDelegateHandle HUDHandle;
    TWeakObjectPtr<AActor> BodyTarget;
    float BodyHitAt=-100.f, HitMarkerAt=-100.f, LastContactAt=-100.f, LastContactDamage=0.f;
    bool LastContactWasSlam=false;
    FVector PoseBase,PoseBend,PoseJoint,PoseTip;
    bool PoseVisible=false,Severed[2]={false,false};
    float Now=-1,Previous=-1,Deadline=160,RideOrigin=270,DamageBaseline=0,HitAt=-100;
    FVector HitMarkerPosition=FVector::ZeroVector;
    FString HitMarkerText;
    int32 AppliedScore=0;
    bool Running=false,Finished=false;
};
