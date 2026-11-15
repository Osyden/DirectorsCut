#include "DCCrewWorkAnim.h"
#include "DCCrewWorkPolicy.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"
#include "BonePose.h"

namespace
{
struct FDCCrewWorkProxy final : FAnimSingleNodeInstanceProxy
{
    explicit FDCCrewWorkProxy(UAnimInstance* Instance) : FAnimSingleNodeInstanceProxy(Instance) {}
    float Age = 10.f;
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimSingleNodeInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        Age = CastChecked<UDCCrewWorkAnim>(Instance)->SwatAge;
    }
    virtual bool Evaluate(FPoseContext& Output) override
    {
        const bool Result = FAnimSingleNodeInstanceProxy::Evaluate(Output);
        if (Age < 0.f || Age >= DCCrewWorkPolicy::Duration) { return Result; }
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        auto Index = [&Bones](const TCHAR* Name)
        {
            return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(FName(Name))));
        };
        const float Weight = DCCrewWorkPolicy::PoseWeight(Age);
        if (Weight <= 0.f) { return Result; }
        const float Down = FMath::SmoothStep(.23f,DCCrewWorkPolicy::Contact,Age);
        const float Recover = FMath::SmoothStep(.52f,.95f,Age);
        // Manny's mesh faces +Y. Bend the torso toward the work without moving
        // the pelvis, root or planted feet, or writing bones after evaluation.
        for (const TCHAR* Name : {TEXT("spine_01"),TEXT("spine_02")})
        {
            const auto Bone = Index(Name);
            if (Bone.GetInt() == INDEX_NONE) { continue; }
            FCSPose<FCompactPose> CS; CS.InitPose(Output.Pose);
            FTransform T = CS.GetComponentSpaceTransform(Bone);
            T.SetRotation((FQuat(FVector::ForwardVector,FMath::DegreesToRadians(-14.f*Weight))*T.GetRotation()).GetNormalized());
            const auto Parent = Bones.GetParentBoneIndex(Bone);
            Output.Pose[Bone] = T.GetRelativeTransform(CS.GetComponentSpaceTransform(Parent));
        }
        const auto Upper = Index(TEXT("upperarm_r"));
        const auto Lower = Index(TEXT("lowerarm_r"));
        const auto Hand = Index(TEXT("hand_r"));
        if (Upper.GetInt()==INDEX_NONE || Lower.GetInt()==INDEX_NONE || Hand.GetInt()==INDEX_NONE) { return Result; }
        // This solver requires the actual two-bone chain. Never rewrite a local
        // transform relative to a guessed parent on another skeleton/LOD.
        if (Bones.GetParentBoneIndex(Lower)!=Upper || Bones.GetParentBoneIndex(Hand)!=Lower) { return Result; }
        FCSPose<FCompactPose> CS; CS.InitPose(Output.Pose);
        FTransform U = CS.GetComponentSpaceTransform(Upper), L = CS.GetComponentSpaceTransform(Lower), H = CS.GetComponentSpaceTransform(Hand);
        const FVector Root = U.GetLocation(), OldElbow=L.GetLocation(), OldHand=H.GetLocation();
        // Keep the windup ahead of the shoulder rather than folded behind the
        // head; the assembled pirate sleeve has no overhead corrective pose.
        const FVector Raised = Root+FVector(12.f,36.f,5.f);
        const FVector Strike = Root+FVector(12.f,45.f,-68.f);
        const FVector Target = FMath::Lerp(FMath::Lerp(Raised,Strike,Down),Raised,Recover);
        const float A=FVector::Distance(Root,OldElbow), B=FVector::Distance(OldElbow,OldHand);
        if (A<1.f || B<1.f) { return Result; }
        const FVector Aim=(Target-Root).GetSafeNormal();
        const float Distance=FMath::Clamp(FVector::Distance(Target,Root),FMath::Abs(A-B)+.1f,A+B-.1f);
        const float Along=(A*A-B*B+Distance*Distance)/(2.f*Distance);
        const float Height=FMath::Sqrt(FMath::Max(0.f,A*A-Along*Along));
        const FVector OldAim=(OldHand-Root).GetSafeNormal();
        FVector Pole=(OldElbow-Root)-OldAim*FVector::DotProduct(OldElbow-Root,OldAim);
        // Transport the existing elbow plane with the reach, preserving its
        // anatomical bend instead of forcing an unrelated fixed world pole.
        if (Pole.IsNearlyZero()) { Pole=FVector::ForwardVector-OldAim*FVector::DotProduct(FVector::ForwardVector,OldAim); }
        Pole=FQuat::FindBetweenNormals(OldAim,Aim).RotateVector(Pole.GetSafeNormal());
        if (Pole.IsNearlyZero()) { return Result; }
        const FVector Elbow=Root+Aim*Along+Pole*Height, End=Root+Aim*Distance;
        U.SetRotation((FQuat::FindBetweenNormals((OldElbow-Root).GetSafeNormal(),(Elbow-Root).GetSafeNormal())*U.GetRotation()).GetNormalized());
        const FQuat Forearm=FQuat::FindBetweenNormals((OldHand-OldElbow).GetSafeNormal(),(End-Elbow).GetSafeNormal());
        L.SetLocation(Elbow); L.SetRotation((Forearm*L.GetRotation()).GetNormalized());
        const FQuat UpperRotation=U.GetRelativeTransform(CS.GetComponentSpaceTransform(Bones.GetParentBoneIndex(Upper))).GetRotation();
        const FQuat LowerRotation=L.GetRelativeTransform(U).GetRotation();
        // Blend the complete joint rotations, not just the hand's target. A
        // zero-weight hand target can still produce a very different elbow.
        // Preserve every local translation/scale and the wrist/finger pose so
        // the arm cannot stretch or separate while its envelope fades.
        Output.Pose[Upper].SetRotation(FQuat::Slerp(Output.Pose[Upper].GetRotation(),UpperRotation,Weight).GetNormalized());
        Output.Pose[Lower].SetRotation(FQuat::Slerp(Output.Pose[Lower].GetRotation(),LowerRotation,Weight).GetNormalized());
        Output.Pose.NormalizeRotations();
        return Result;
    }
};
}
FAnimInstanceProxy* UDCCrewWorkAnim::CreateAnimInstanceProxy() { return new FDCCrewWorkProxy(this); }
void UDCCrewWorkAnim::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
