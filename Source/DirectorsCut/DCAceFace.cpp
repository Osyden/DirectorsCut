#include "DCAceFace.h"
#include "ACEAudioCurveSourceComponent.h"
#include "AnimNode_ApplyACEAnimation.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/PoseAsset.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "AnimNodes/AnimNode_PoseBlendNode.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

struct FDCAceFaceProxy : FAnimInstanceProxy
{
    FAnimNode_CopyPoseFromMesh Body;
    FAnimNode_ApplyACEAnimation ACE{};
    FAnimNode_PoseBlendNode Mapping;

    FDCAceFaceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
    {
        Body.bUseAttachedParent = true;
        ACE.Source.SetLinkNode(&Body);
        ACE.bBlendOutToZero = true;
        ACE.BlendOutRate = 12.f;
        Mapping.SourcePose.SetLinkNode(&ACE);
    }
    virtual void Initialize(UAnimInstance* Instance) override
    {
        auto* FaceInstance = CastChecked<UDCAceFaceAnimInstance>(Instance);
        FaceInstance->Mapping = LoadObject<UPoseAsset>(nullptr,
            TEXT("/Game/MetaHumans/Common/Common/Mocap/mh_arkit_mapping_pose.mh_arkit_mapping_pose"));
        Mapping.PoseAsset = FaceInstance->Mapping;
        FAnimInstanceProxy::Initialize(Instance);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Mapping; }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        // Native graph nodes are not in a generated Blueprint's PreUpdate list.
        Body.PreUpdate(Instance);
        ACE.PreUpdate(Instance);
    }
};

void UDCAceFaceAnimInstance::NativeInitializeAnimation()
{
    Mapping = LoadObject<UPoseAsset>(nullptr,
        TEXT("/Game/MetaHumans/Common/Common/Mocap/mh_arkit_mapping_pose.mh_arkit_mapping_pose"));
    Super::NativeInitializeAnimation();
}
FAnimInstanceProxy* UDCAceFaceAnimInstance::CreateAnimInstanceProxy() { return new FDCAceFaceProxy(this); }
void UDCAceFaceAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }

UACEAudioCurveSourceComponent* DCAceFace::Bind(AActor* Avatar)
{
    if (!Avatar) { return nullptr; }
    USkeletalMeshComponent* Face = nullptr;
    TInlineComponentArray<USkeletalMeshComponent*> Meshes(Avatar);
    for (USkeletalMeshComponent* Mesh : Meshes)
    {
        if (Mesh->GetName().Equals(TEXT("Face"), ESearchCase::IgnoreCase)) { Face = Mesh; break; }
    }
    if (!Face) { return nullptr; }
    UACEAudioCurveSourceComponent* Curves = Avatar->FindComponentByClass<UACEAudioCurveSourceComponent>();
    if (!Curves)
    {
        Curves = NewObject<UACEAudioCurveSourceComponent>(Avatar, TEXT("CrewACE"));
        Avatar->AddInstanceComponent(Curves);
        Curves->SetupAttachment(Face);
        Curves->Volume = 1.f;
        Curves->RegisterComponent();
    }
    Face->SetLeaderPoseComponent(nullptr);
    Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Face->SetAnimInstanceClass(UDCAceFaceAnimInstance::StaticClass());
    UE_LOG(LogTemp, Log, TEXT("ACE face: bound %s; post-process=%s"), *Avatar->GetName(), *GetNameSafe(Face->GetPostProcessInstance()));
    return Curves;
}
