#include "DCObstacleWaterContact.h"
#include "DCRideDirector.h"
#include "DCOcean.h"
#include "DCTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "ProceduralMeshComponent.h"
#include "UObject/UnrealType.h"

namespace { constexpr float FoundationDepth=600.f; }

UDCObstacleWaterContact::UDCObstacleWaterContact()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
    PrimaryComponentTick.TickInterval=.066f;
}

void UDCObstacleWaterContact::Attach(AActor* Pillars)
{
    if(!Pillars || Pillars->FindComponentByClass<UDCObstacleWaterContact>()) { return; }
    auto* Contact=NewObject<UDCObstacleWaterContact>(Pillars);
    Pillars->AddInstanceComponent(Contact);
    Contact->RegisterComponent();
    Contact->Build();
}

void UDCObstacleWaterContact::Build()
{
    AActor* Owner=GetOwner();
    Director=ADCRideDirector::Get(this);
    WaterLibrary=FindObject<UClass>(nullptr,TEXT("/Script/DynamicRealWater.DRW_BlueprintLibrary"));
    SurfaceFunction=WaterLibrary?WaterLibrary->FindFunctionByName(TEXT("GetWaterSurfaceAtLocation")):nullptr;
    // One stable asynchronous query channel per pillar; avoid the hull's 0..223.
    static int32 NextChannel=1000;
    SampleChannel=NextChannel; NextChannel+=8;
    UStaticMesh* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    TInlineComponentArray<UStaticMeshComponent*> Parts(Owner);
    for(UStaticMeshComponent* Part:Parts)
    {
        if(!Part->GetStaticMesh()) { continue; }
        const FVector Extent=Part->Bounds.BoxExtent;
        // Upright columns only. Fallen drums/debris are already submerged.
        if(Extent.Z<FMath::Max(Extent.X,Extent.Y)*1.15f || Feet.Num()>=6) { continue; }
        const FVector Bottom=Part->Bounds.Origin-FVector(0,0,Extent.Z);
        const float Radius=FMath::Clamp(static_cast<float>(FMath::Min(Extent.X,Extent.Y))*.92f,55.f,500.f);
        FFoot& Foot=Feet.AddDefaulted_GetRef();
        Foot.LocalCenter=Owner->GetActorTransform().InverseTransformPosition(Bottom);
        Foot.Radius=Radius; Foot.Height=Extent.Z*2.f; Foot.NextSplash=.4f+Feet.Num()*.37f;
        if(Cylinder)
        {
            // Stone remains at mean sea level. Extend each foot six metres
            // down so a passing wave trough reveals a continuous foundation.
            constexpr float Overlap=60.f;
            auto* Foundation=NewObject<UStaticMeshComponent>(Owner);
            Foundation->SetupAttachment(Owner->GetRootComponent());
            Foundation->SetMobility(EComponentMobility::Movable);
            Foundation->SetStaticMesh(Cylinder);
            Foundation->SetMaterial(0,Part->GetMaterial(0));
            Foundation->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Foundation->RegisterComponent();
            Foundation->SetWorldLocation(Bottom+FVector(0,0,(Overlap-FoundationDepth)*.5f));
            Foundation->SetWorldScale3D(FVector(Radius/50.f,Radius/50.f,(FoundationDepth+Overlap)/100.f));
            // The treadmill enables the same stone feeler queries as the pillars.
        }
    }
    if(Feet.IsEmpty()) { SetComponentTickEnabled(false); return; }
    Foam=NewObject<UProceduralMeshComponent>(Owner);
    Foam->SetupAttachment(Owner->GetRootComponent());
    Foam->SetUsingAbsoluteLocation(true); Foam->SetUsingAbsoluteRotation(true); Foam->SetUsingAbsoluteScale(true);
    Foam->SetCollisionEnabled(ECollisionEnabled::NoCollision); Foam->SetCastShadow(false);
    Foam->RegisterComponent(); Foam->SetWorldTransform(FTransform::Identity);
    if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_Wake.M_Wake")))
    {
        FoamMaterial=UMaterialInstanceDynamic::Create(Base,this); Foam->SetMaterial(0,FoamMaterial);
        FoamMaterial->SetScalarParameterValue(TEXT("Speed"),180.f);
    }
    // Cascade RequiredModule SubUV data cannot be duplicated from a cooked
    // package. Local-space emitter settings must be authored BEFORE cooking.
    Splash=LoadObject<UParticleSystem>(nullptr,TEXT("/Game/DC/FX/P_PillarWaterContact.P_PillarWaterContact"),nullptr,LOAD_NoWarn | LOAD_Quiet);
    if(!Splash)
    {
        // Missing generated content may lose attachment fidelity, but must not
        // crash or mutate the vendor asset. The release cook includes the copy.
        Splash=LoadObject<UParticleSystem>(nullptr,TEXT("/Game/ParagonRampage/FX/Particles/Abilities/RipNToss/FX/P_RipNToss_WaterImpact.P_RipNToss_WaterImpact"));
        UE_LOG(LogDC,Warning,TEXT("PILLAR_CONTACT using original splash fallback; run Tools/make_pillar_splash.py before release cook"));
    }
    UE_LOG(LogDC,Log,TEXT("PILLAR_CONTACT built feet=%d foundationDepth=%.0f actor=%s splash=%s"),Feet.Num(),FoundationDepth,*Owner->GetName(),*GetPathNameSafe(Splash));
}

float UDCObstacleWaterContact::SurfaceAt(const FVector& Position,int32 Channel)
{
    const float Mean=Director?Director->SeaLevelZ:230.f;
    if(!SurfaceFunction || !WaterLibrary) { return Mean; }
    uint8* Params=static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(SurfaceFunction->ParmsSize,1)));
    FMemory::Memzero(Params,SurfaceFunction->ParmsSize);
    FStructProperty* Sample=nullptr;
    for(TFieldIterator<FProperty> It(SurfaceFunction);It && It->HasAnyPropertyFlags(CPF_Parm);++It)
    {
        It->InitializeValue_InContainer(Params);
        if(auto* O=CastField<FObjectProperty>(*It)) { O->SetObjectPropertyValue_InContainer(Params,this); }
        else if(auto* I=CastField<FIntProperty>(*It)) { I->SetPropertyValue_InContainer(Params,Channel); }
        else if(auto* S=CastField<FStructProperty>(*It))
        {
            if(S->Struct==TBaseStructure<FVector>::Get()) { *S->ContainerPtrToValuePtr<FVector>(Params)=DCOcean::WaveSamplePosition(this,Position); }
            else { Sample=S; }
        }
    }
    WaterLibrary->GetDefaultObject()->ProcessEvent(SurfaceFunction,Params);
    float Height=Mean;
    if(Sample)
    {
        const void* Data=Sample->ContainerPtrToValuePtr<void>(Params);
        const auto* Valid=FindFProperty<FBoolProperty>(Sample->Struct,TEXT("bValid"));
        const auto* Value=FindFProperty<FFloatProperty>(Sample->Struct,TEXT("Height"));
        if(Valid && Value && Valid->GetPropertyValue_InContainer(Data)) { Height=Value->GetPropertyValue_InContainer(Data); }
    }
    for(TFieldIterator<FProperty> It(SurfaceFunction);It && It->HasAnyPropertyFlags(CPF_Parm);++It) { It->DestroyValue_InContainer(Params); }
    return Height;
}

void UDCObstacleWaterContact::TickComponent(float DT,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DT,TickType,TickFunction);
    AActor* Owner=GetOwner();
    if(!Owner || !Foam || !Director) { return; }
    const bool Visible=!Owner->IsHidden() && Director->GetPhase()<=EDCPhase::Fog
        && FVector2D(Owner->GetActorLocation()).Size()<22000.f;
    Foam->SetVisibility(Visible && FoamMaterial!=nullptr);
    Pulses.RemoveAll([](const TWeakObjectPtr<UParticleSystemComponent>& P){return !P.IsValid() || !P->IsActive();});
    if(!Visible) { for(auto& P:Pulses) { if(P.IsValid()) { P->DeactivateSystem(); } } return; }
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<FColor> Colors; TArray<int32> Tri;
    const float Time=Director->RideTime;
    constexpr int32 Segments=24;
    for(int32 Index=0;Index<Feet.Num();++Index)
    {
        FFoot& Foot=Feet[Index];
        FVector Center=Owner->GetActorTransform().TransformPosition(Foot.LocalCenter);
        const float Water=SurfaceAt(Center,SampleChannel+Index);
        const float Rise=Foot.bSampled?(Water-Foot.PreviousWater)/FMath::Max(DT,.001f):0.f;
        Foot.PreviousWater=Water; Foot.bSampled=true;
        // Skip a fully buried column top or a stone lifted out of reach on exit.
        if(Center.Z-FoundationDepth>Water || Center.Z+Foot.Height<Water) { continue; }
        Center.Z=Water+18.f;
        const int32 Start=V.Num();
        for(int32 S=0;S<=Segments;++S)
        {
            const float A=S*UE_TWO_PI/Segments;
            const float Wash=90.f+35.f*FMath::Sin(A*3.f+Time*1.3f+Index);
            for(int32 Edge=0;Edge<2;++Edge)
            {
                const float R=Foot.Radius-15.f+Edge*Wash;
                V.Add(Center+FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,0.f));
                N.Add(FVector::UpVector); UV.Add(FVector2D(Edge,S/static_cast<float>(Segments)));
                Colors.Add(FColor(255,255,255,220));
            }
            if(S<Segments) { const int32 K=Start+S*2; Tri.Append({K,K+2,K+1,K+1,K+2,K+3}); }
        }
        Foot.NextSplash-=DT;
        if(Splash && Foot.NextSplash<=0.f && Pulses.Num()<6 && (Rise>20.f || Foot.NextSplash<-1.3f))
        {
            const FVector At=Center+FVector(Foot.Radius*.9f,0.f,0.f);
            const float Scale=FMath::Clamp(.18f+FMath::Max(0.f,Rise)*.0009f,.18f,.45f);
            auto* P=UGameplayStatics::SpawnEmitterAttached(Splash,Owner->GetRootComponent(),NAME_None,At,FRotator::ZeroRotator,FVector(Scale),EAttachLocation::KeepWorldPosition,true);
            if(P) { P->SetCastShadow(false); Pulses.Add(P); }
            Foot.NextSplash=1.4f+Index*.27f;
        }
    }
    Foam->CreateMeshSection(0,V,Tri,N,UV,Colors,TArray<FProcMeshTangent>(),false);
    if(FoamMaterial)
    {
        const float Day=FMath::Clamp(Director->GetEnv().SunIntensity/6.f,.22f,1.f);
        FoamMaterial->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.74f*Day,.83f*Day,.88f*Day,.85f));
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DCPillarLog")) && (LogTimer-=DT)<=0.f)
    {
        LogTimer=1.f;
        UE_LOG(LogDC,Log,TEXT("PILLAR_CONTACT t=%.1f actor=%s feet=%d pulses=%d position=%s water=%.1f"),Time,*Owner->GetName(),Feet.Num(),Pulses.Num(),*Owner->GetActorLocation().ToCompactString(),Feet[0].PreviousWater);
    }
}
