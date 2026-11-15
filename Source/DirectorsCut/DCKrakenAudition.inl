#include "DCLeviathan.h"
#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"

// Opt-in only. Auditions purchased source in place; never modifies or exports it.
bool ADCLeviathan::UpdateKrakenAudition(float Elapsed,bool Active)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DCKrakenAudition"))) return false;
    SetActorHiddenInGame(!Active); Arms->SetVisibility(false,true);
    if(!Active) return true;
    const TCHAR* Names[]={TEXT("idle"),TEXT("smashAttack"),TEXT("sweepAttack"),TEXT("getHit"),TEXT("death"),TEXT("idle"),TEXT("idle")};
    const int32 Pose=FMath::Clamp(FMath::FloorToInt((Elapsed-40.f)/8.f),0,6);
    const bool Changed=CurrentPose!=100+Pose;
    if(Changed)
    {
        auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Kraken/Meshes/KRAKEN.KRAKEN"));
        if(!Mesh) return true;
        Body->SetSkeletalMesh(Mesh);
        for(int32 I=0;I<Mesh->GetMaterials().Num();++I) Body->SetMaterial(I,Mesh->GetMaterials()[I].MaterialInterface);
        MeshBoundsCenter=Mesh->GetBounds().Origin; MeshBoundsExtent=Mesh->GetBounds().BoxExtent;
        const FString Path=FString::Printf(TEXT("/Game/Kraken/Animations/KRAKEN_%s.KRAKEN_%s"),Names[Pose],Names[Pose]);
        auto* Clip=LoadObject<UAnimSequence>(nullptr,*Path);
        Poses.Reset(); Poses.Add(Clip);
        if(Clip) Body->PlayAnimation(Clip,false);
        Body->SetComponentTickEnabled(false);
        Body->UnHideBoneByName(TEXT("KRAKEN_TENTACLE_BASE_A_1"));
        Body->UnHideBoneByName(TEXT("KRAKEN_ARM_L"));
        Body->UnHideBoneByName(TEXT("KRAKEN_TENTACLE_BASE_H_1"));
        Body->UnHideBoneByName(TEXT("KRAKEN_TENTACLE_BASE_B_1"));
        if(Pose>=5) Body->HideBoneByName(TEXT("KRAKEN_TENTACLE_BASE_H_1"),EPhysBodyOp::PBO_None);
        if(Pose==6) Body->HideBoneByName(TEXT("KRAKEN_TENTACLE_BASE_B_1"),EPhysBodyOp::PBO_None);
        CurrentPose=100+Pose;
        UE_LOG(LogDC,Display,TEXT("KRAKEN_AUDITION pose=%s length=%.3f bounds=%s hiddenTentacles=%d bones=%d"),Names[Pose],Clip?Clip->GetPlayLength():0.f,*MeshBoundsExtent.ToString(),FMath::Max(0,Pose-4),Mesh->GetRefSkeleton().GetNum());
        if(Pose==0)
            for(int32 I=0;I<Mesh->GetRefSkeleton().GetNum();++I)
                UE_LOG(LogDC,Display,TEXT("KRAKEN_BONE %d %s parent=%d"),I,*Mesh->GetRefSkeleton().GetBoneName(I).ToString(),Mesh->GetRefSkeleton().GetParentIndex(I));
    }
    const float Scale=4500.f/FMath::Max(1.f,static_cast<float>(MeshBoundsExtent.Z)*2.f);
    const FQuat Facing=FRotator(0,0.f,0).Quaternion();
    FVector Center(1500,12000,0);
    const auto* Stage=ADCMaelstrom::Get(this);
    Center.Z=Stage?Stage->SampleSurfaceWorld(Center):230.f;
    Center.Z-=(MeshBoundsCenter.Z+MeshBoundsExtent.Z*.25f)*Scale;
    Center-=Facing.RotateVector(FVector(MeshBoundsCenter.X,MeshBoundsCenter.Y,0)*Scale);
    Body->SetWorldLocationAndRotation(Center,Facing); Body->SetWorldScale3D(FVector(Scale));
    Body->SetVisibility(true,true); Body->SetHiddenInGame(false,true);
    if(Poses.Num() && Poses[0])
    {
        const float U=FMath::Clamp((Elapsed-(40.f+Pose*8.f))/8.f,0.f,1.f);
        Body->SetPosition(U*Poses[0]->GetPlayLength(),false);
        Body->TickAnimation(0,false); Body->RefreshBoneTransforms();
    }
    const FVector LocalMouth=(Body->GetSocketTransform(TEXT("KRAKEN_BEAK_UPPER"),RTS_Component).GetLocation()
        +Body->GetSocketTransform(TEXT("KRAKEN_BEAK_LOWER"),RTS_Component).GetLocation())*.5f;
    const FVector Face=LocalMouth-Body->GetSocketTransform(TEXT("KRAKEN_SHELL"),RTS_Component).GetLocation();
    if(Changed)
        for(const TCHAR* Bone:{TEXT("KRAKEN_SHELL"),TEXT("KRAKEN_BEAK_UPPER"),TEXT("KRAKEN_BEAK_LOWER"),TEXT("KRAKEN_TENTACLE_BASE_A_1"),TEXT("KRAKEN_TENTACLE_BASE_H_1"),TEXT("KRAKEN_TENTACLE_BASE_B_1"),TEXT("KRAKEN_TENTACLE_BASE_G_1")})
            UE_LOG(LogDC,Display,TEXT("KRAKEN_SOCKET pose=%d %s %s"),Pose,Bone,*Body->GetSocketTransform(Bone,RTS_Component).GetLocation().ToCompactString());
    const float Bearing=FMath::RadiansToDegrees(FMath::Atan2(Face.Y,Face.X));
    Body->SetWorldRotation(FRotator(0,-90.f-Bearing,0));
    Body->AddWorldOffset(FVector(0,0,(Stage?Stage->SampleSurfaceWorld(Center):230.f)+650.f-GetMouthPosition().Z));
    // Aim the existing battery camera, just as mouse input would. Its default
    // downward barrel view cannot show a creature at combat distance.
    if(auto* PC=GetWorld()->GetFirstPlayerController())
        if(auto* Camera=Cast<ACameraActor>(PC->GetViewTarget()))
            Camera->SetActorRotation((GetMouthPosition()+FVector(0,0,350)-Camera->GetActorLocation()).Rotation());
    return true;
}
