#pragma once
#include "DCRideDirector.h"
#include "DCTroubles.h"
#include "DCAudio.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

/** Shared by the fog giants and boss: the same rock, orange light, arc and fire impact. */
namespace DCBossFireball
{
    constexpr float Flight=3.f;
    inline AActor* Spawn(UWorld* World,AActor* Owner,const FVector& From)
    {
        FActorSpawnParameters P; P.Owner=Owner; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* B=World->SpawnActor<AStaticMeshActor>(From,FRotator::ZeroRotator,P);
        if(!B) return nullptr;
        auto* M=B->GetStaticMeshComponent(); M->SetMobility(EComponentMobility::Movable);
        auto* Rock=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ParagonRampage/FX/Meshes/Debris/SM_Rock_To_Hold.SM_Rock_To_Hold"),nullptr,LOAD_NoWarn|LOAD_Quiet);
        M->SetStaticMesh(Rock?Rock:LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        M->SetCollisionEnabled(ECollisionEnabled::NoCollision); M->SetCastShadow(false);
        if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_Glow.M_Glow")))
        {
            auto* Hot=UMaterialInstanceDynamic::Create(Base,B); Hot->SetVectorParameterValue(TEXT("Color"),FLinearColor(11.f,2.6f,.3f));
            for(int32 Slot=0;Slot<M->GetNumMaterials();++Slot) M->SetMaterial(Slot,Hot);
        }
        const float Through=FMath::Max(M->GetStaticMesh()?static_cast<float>(M->GetStaticMesh()->GetBoundingBox().GetSize().GetMax()):100.f,1.f);
        B->SetActorScale3D(FVector(260.f/Through));
        auto* Light=NewObject<UPointLightComponent>(B); Light->SetMobility(EComponentMobility::Movable);
        Light->SetupAttachment(M); Light->SetIntensity(120000.f); Light->SetAttenuationRadius(4500.f);
        Light->SetLightColor(FLinearColor(1.f,.45f,.12f)); Light->SetCastShadows(false); Light->RegisterComponent();
        return B;
    }
    inline FVector Position(const FVector& From,const FVector& To,float Age)
    {
        const float U=FMath::Clamp(Age/Flight,0.f,1.f);
        return FMath::Lerp(From,To,U)+FVector(0,0,4.f*4500.f*U*(1.f-U));
    }
    inline void Impact(UWorld* World,ADCRideDirector* Director,const FVector& To)
    {
        if(!Director) return;
        if(auto* Sound=Cast<ADCAudio>(UGameplayStatics::GetActorOfClass(World,ADCAudio::StaticClass())))
            Sound->FireballImpact(Director,2.f);
        else Director->DamageShip(2.f);
        if(auto* T=ADCTroubles::Get(Director); T && T->DeckPoints.Num() && T->NumFires()<T->MaxFires)
        {
            FVector Best=T->DeckPoints[0];
            for(const FVector& P:T->DeckPoints) if(FVector::DistSquared2D(P,To)<FVector::DistSquared2D(Best,To)) Best=P;
            T->StartFire(Best);
        }
    }
}
