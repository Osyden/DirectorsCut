#include "DCAuthoredTentacle.h"
#include "DCTentacleCurve.h"
#include "DCAuthoredLimbPolicy.h"
#include "DCTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAsset.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UDCAuthoredTentacle* UDCAuthoredTentacle::Create(AActor* Owner,USkeletalMesh* Mesh)
{
    if(!Owner || !Mesh || !Owner->GetRootComponent()) { return nullptr; }
    const FReferenceSkeleton& Skeleton=Mesh->GetRefSkeleton();
    const int32 Root=Skeleton.FindBoneIndex(TEXT("KRAKEN_TENTACLE_BASE_H_1"));
    auto* Hidden=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_BossLimbHidden.M_BossLimbHidden"));
    if(Root==INDEX_NONE || !Hidden)
    {
        UE_LOG(LogDC,Error,TEXT("KRAKEN_LIMB unavailable: H_1 bone or M_BossLimbHidden missing; authored limbs cannot be accepted"));
        return nullptr;
    }
    auto* Limb=NewObject<UDCAuthoredTentacle>(Owner,TEXT("AuthoredAttackTentacle"));
    Owner->AddInstanceComponent(Limb);
    Limb->SetupAttachment(Owner->GetRootComponent());
    Limb->SetAbsolute(true,true,true);
    Limb->SetMobility(EComponentMobility::Movable);
    Limb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Limb->SetGenerateOverlapEvents(false);
    Limb->SetCanEverAffectNavigation(false);
    Limb->SetCastShadow(false);
    Limb->bReceivesDecals=false;
    Limb->SetSkinnedAssetAndUpdate(Mesh,true);
    Limb->RegisterComponent();
    Limb->SetWorldTransform(FTransform::Identity);
    Limb->SetComponentTickEnabled(false);
    Limb->SetVisibility(false);
    Limb->Reference.SetNum(Skeleton.GetNum());
    Limb->Keep.Init(false,Skeleton.GetNum());
    for(int32 I=0;I<Skeleton.GetNum();++I)
    {
        const int32 Parent=Skeleton.GetParentIndex(I);
        Limb->Reference[I]=Parent>=0?Skeleton.GetRefBonePose()[I]*Limb->Reference[Parent]:Skeleton.GetRefBonePose()[I];
        if(I==Root || (Parent>=Root && Limb->Chain.Contains(Parent))) { Limb->Chain.Add(I); Limb->Keep[I]=true; }
    }
    // The verified H branch is linear; reject an unexpected asset instead of
    // displaying a mangled limb or silently substituting the procedural tube.
    for(int32 I=1;I<Limb->Chain.Num();++I)
        if(Skeleton.GetParentIndex(Limb->Chain[I])!=Limb->Chain[I-1])
        { UE_LOG(LogDC,Error,TEXT("KRAKEN_LIMB non-linear H branch")); Limb->DestroyComponent(); return nullptr; }
    if(Limb->Chain.Num()<4)
    { UE_LOG(LogDC,Error,TEXT("KRAKEN_LIMB incomplete H branch")); Limb->DestroyComponent(); return nullptr; }
    for(int32 I=Root;I>=0;I=Skeleton.GetParentIndex(I)) { Limb->Keep[I]=true; }
    float Length=0;
    for(int32 I=0;I<Limb->Chain.Num();++I)
    {
        if(I) Length+=FVector::Distance(Limb->Reference[Limb->Chain[I]].GetLocation(),Limb->Reference[Limb->Chain[I-1]].GetLocation());
        Limb->ChainU.Add(Length);
    }
    if(Length<1)
    { UE_LOG(LogDC,Error,TEXT("KRAKEN_LIMB zero-length H branch")); Limb->DestroyComponent(); return nullptr; }
    for(float& U:Limb->ChainU) { U/=Length; }
    float ClosestJoint=1.f;
    for(float U:Limb->ChainU)
        if(FMath::Abs(U-.72f)<ClosestJoint) { ClosestJoint=FMath::Abs(U-.72f); Limb->JointSourceU=U; }
    int32 SkinSlots=0;
    for(int32 Slot=0;Slot<Mesh->GetMaterials().Num();++Slot)
    {
        UMaterialInterface* Material=Mesh->GetMaterials()[Slot].MaterialInterface;
        if(Material && Material->GetName().Contains(TEXT("TENTACLES"))) { ++SkinSlots; }
        else { Limb->SetMaterial(Slot,Hidden); }
    }
    if(!SkinSlots)
    { UE_LOG(LogDC,Error,TEXT("KRAKEN_LIMB authored tentacle material slot missing")); Limb->DestroyComponent(); return nullptr; }
    UE_LOG(LogDC,Log,TEXT("KRAKEN_LIMB ready source=%s root=H_1 bones=%d restLength=%.1f skinSlots=%d; authored geometry/UV/material, no tube fallback"),
        *Mesh->GetPathName(),Limb->Chain.Num(),Length,SkinSlots);
    return Limb;
}

void UDCAuthoredTentacle::Pose(const FVector& Base,const FVector& Bend,const FVector& Joint,const FVector& Tip,float WidthScale)
{
    if(!GetSkinnedAsset() || Chain.IsEmpty()) { return; }
    if(BoneSpaceTransforms.Num()!=Reference.Num() && !AllocateTransformData()) { return; }
    const FReferenceSkeleton& Skeleton=GetSkinnedAsset()->GetRefSkeleton();
    WidthScale=FMath::Clamp(WidthScale,1.f,8.f);
    TArray<FTransform> Desired=Reference;
    const FVector RefRoot=Reference[Chain[0]].GetLocation();
    FVector PrevRefTangent=(Reference[Chain[1]].GetLocation()-RefRoot).GetSafeNormal();
    FVector PrevTangent=DCTentacleCurve::Tangent(0,Base,Bend,Joint,Tip);
    FVector RefAcross=FVector::CrossProduct(FMath::Abs(PrevRefTangent.Z)<.9f?FVector::UpVector:FVector::RightVector,PrevRefTangent).GetSafeNormal();
    const FQuat InitialTurn=FQuat::FindBetweenNormals(PrevRefTangent,PrevTangent);
    FVector Across=InitialTurn.RotateVector(RefAcross);
    // Keep ancestors alive for the selected skin weights, but leave their
    // geometry underwater; unrelated branches collapse and body slots are masked.
    for(int32 I=0;I<Reference.Num();++I)
        if(Keep[I]) Desired[I]=FTransform(InitialTurn*Reference[I].GetRotation(),
            Base+InitialTurn.RotateVector((Reference[I].GetLocation()-RefRoot)*WidthScale),Reference[I].GetScale3D()*WidthScale);
    for(int32 Index=0;Index<Chain.Num();++Index)
    {
        const int32 Bone=Chain[Index];
        const int32 Before=Chain[FMath::Max(0,Index-1)],After=Chain[FMath::Min(Chain.Num()-1,Index+1)];
        FVector RefTangent=(Reference[After].GetLocation()-Reference[Before].GetLocation()).GetSafeNormal();
        const float U=DCAuthoredLimbPolicy::CurveU(ChainU[Index],JointSourceU);
        FVector Tangent=DCTentacleCurve::Tangent(U,Base,Bend,Joint,Tip);
        if(RefTangent.IsNearlyZero()) RefTangent=PrevRefTangent;
        if(Tangent.IsNearlyZero()) Tangent=PrevTangent;
        RefAcross=FQuat::FindBetweenNormals(PrevRefTangent,RefTangent).RotateVector(RefAcross).GetSafeNormal();
        Across=FQuat::FindBetweenNormals(PrevTangent,Tangent).RotateVector(Across).GetSafeNormal();
        const FQuat Turn=FRotationMatrix::MakeFromXY(Tangent,Across).ToQuat()*FRotationMatrix::MakeFromXY(RefTangent,RefAcross).ToQuat().Inverse();
        Desired[Bone]=FTransform(Turn*Reference[Bone].GetRotation(),DCTentacleCurve::Point(U,Base,Bend,Joint,Tip),Reference[Bone].GetScale3D()*WidthScale);
        PrevRefTangent=RefTangent; PrevTangent=Tangent;
    }
    for(int32 I=0;I<Reference.Num();++I)
    {
        const int32 Parent=Skeleton.GetParentIndex(I);
        if(Keep[I]) BoneSpaceTransforms[I]=Parent>=0?Desired[I].GetRelativeTransform(Desired[Parent]):Desired[I];
        else { BoneSpaceTransforms[I]=Skeleton.GetRefBonePose()[I]; BoneSpaceTransforms[I].SetScale3D(FVector::ZeroVector); }
    }
    PoseBounds=FBox(ForceInit);
    for(int32 I=0;I<=32;++I) { PoseBounds+=DCTentacleCurve::Point(I/32.f,Base,Bend,Joint,Tip); }
    PoseBounds=PoseBounds.ExpandBy(1800.f);
    RefreshBoneTransforms();
}

FBoxSphereBounds UDCAuthoredTentacle::CalcBounds(const FTransform& LocalToWorld) const
{
    return PoseBounds.IsValid?FBoxSphereBounds(PoseBounds):Super::CalcBounds(LocalToWorld);
}
