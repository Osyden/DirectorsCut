#include "DCLeviathan.h"
#include "DCMaelstrom.h"
#include "DCRideDirector.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Opt-in only. Auditions purchased source in place; never modifies or exports it.
bool ADCLeviathan::UpdateKrakenAudition(float Elapsed,bool Active)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DCKrakenAudition"))) return false;
    SetActorHiddenInGame(!Active); Arms->SetVisibility(false,true);
    if(!Active) return true;
    const TCHAR* Names[]={TEXT("idle"),TEXT("smashAttack"),TEXT("sweepAttack"),TEXT("getHit"),TEXT("death"),TEXT("idle"),TEXT("idle")};
    const int32 Pose=FMath::Clamp(FMath::FloorToInt((Elapsed-40.f)/8.f),0,6);
    if(CurrentPose!=100+Pose)
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
        Body->UnHideBoneByName(TEXT("KRAKEN_TENTACLE_BASE_A"));
        Body->UnHideBoneByName(TEXT("KRAKEN_ARM_L"));
        if(Pose==5) Body->HideBoneByName(TEXT("KRAKEN_TENTACLE_BASE_A"),EPhysBodyOp::PBO_None);
        if(Pose==6) Body->HideBoneByName(TEXT("KRAKEN_ARM_L"),EPhysBodyOp::PBO_None);
        CurrentPose=100+Pose;
        UE_LOG(LogDC,Display,TEXT("KRAKEN_AUDITION pose=%s length=%.3f bounds=%s hiddenA=%d bones=%d"),Names[Pose],Clip?Clip->GetPlayLength():0.f,*MeshBoundsExtent.ToString(),Pose==5,Mesh->GetRefSkeleton().GetNum());
        if(Pose==0)
            for(int32 I=0;I<Mesh->GetRefSkeleton().GetNum();++I)
                UE_LOG(LogDC,Display,TEXT("KRAKEN_BONE %d %s parent=%d"),I,*Mesh->GetRefSkeleton().GetBoneName(I).ToString(),Mesh->GetRefSkeleton().GetParentIndex(I));
    }
    const float Scale=4500.f/FMath::Max(1.f,static_cast<float>(MeshBoundsExtent.Z)*2.f);
    const FQuat Facing=FRotator(0,-90.f,0).Quaternion();
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
    return true;
}
