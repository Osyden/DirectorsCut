#include "DCCombatCue.h"
#include "DCCombatCuePolicy.h"
#include "DCTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
struct FGeometry
{
    TArray<FVector> V;
    TArray<int32> I;
    TArray<FLinearColor> C;
    void Quad(const FVector& A,const FVector& B,const FVector& D,const FVector& E,const FLinearColor& Color)
    {
        const int32 N=V.Num(); V.Append({A,B,D,E}); C.Append({Color,Color,Color,Color});
        I.Append({N,N+1,N+2,N,N+2,N+3});
    }
    // XY annulus. Transparent outer/inner edges make a soft, thin halo.
    void Arc(float Radius,float Width,float Start,float End,float Z,FLinearColor Color)
    {
        constexpr int32 Segments=32;
        const int32 First=V.Num();
        for(int32 S=0;S<=Segments;++S)
        {
            const float A=FMath::Lerp(Start,End,S/float(Segments));
            for(int32 R=0;R<3;++R)
            {
                const float Distance=Radius+(R-1)*Width;
                V.Add(FVector(FMath::Cos(A)*Distance,FMath::Sin(A)*Distance,Z));
                FLinearColor Tint=Color; Tint.A*=R==1?1.f:0.f; C.Add(Tint);
            }
        }
        for(int32 S=0;S<Segments;++S) for(int32 R=0;R<2;++R)
        {
            const int32 N=First+S*3+R; I.Append({N,N+3,N+1,N+1,N+3,N+4});
        }
    }
    // An axial strip around the tentacle: visible on the flesh from either
    // battery, unlike a flat reticle that disappears when seen edge-on.
    void Band(float Radius,float HalfWidth,float Start,float End,FLinearColor Color)
    {
        constexpr int32 Segments=16;
        const int32 First=V.Num();
        for(int32 S=0;S<=Segments;++S)
        {
            const float A=FMath::Lerp(Start,End,S/float(Segments));
            for(int32 Row=0;Row<3;++Row)
            {
                V.Add(FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,(Row-1)*HalfWidth));
                FLinearColor Tint=Color; Tint.A*=Row==1?1.f:0.f; C.Add(Tint);
            }
        }
        for(int32 S=0;S<Segments;++S) for(int32 R=0;R<2;++R)
        {
            const int32 N=First+S*3+R; I.Append({N,N+3,N+1,N+1,N+3,N+4});
        }
    }
};
}

UDCCombatCueComponent* UDCCombatCueComponent::Create(AActor* Owner,FName Name)
{
    if(!Owner || !Owner->GetRootComponent()) return nullptr;
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Materials/M_CombatCue.M_CombatCue"));
    if(!Material)
    {
        UE_LOG(LogDC,Warning,TEXT("Combat cue material missing; run Tools/make_combat_cue_material.py before packaging"));
        return nullptr;
    }
    auto* Cue=NewObject<UDCCombatCueComponent>(Owner,Name);
    Owner->AddInstanceComponent(Cue);
    Cue->SetupAttachment(Owner->GetRootComponent());
    Cue->SetAbsolute(true,true,true);
    Cue->SetMobility(EComponentMobility::Movable);
    Cue->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Cue->SetGenerateOverlapEvents(false);
    Cue->SetCanEverAffectNavigation(false);
    Cue->SetCastShadow(false);
    Cue->bReceivesDecals=false;
    Cue->PrimaryComponentTick.bCanEverTick=false;
    Cue->SetVisibility(false);
    Cue->RegisterComponent();
    Cue->CueMaterial=UMaterialInstanceDynamic::Create(Material,Cue);
    Cue->SetMaterial(0,Cue->CueMaterial);
    return Cue;
}

void UDCCombatCueComponent::Present(const FVector& Position,const FQuat& Rotation,
    const TArray<FVector>& Vertices,const TArray<int32>& Indices,const TArray<FLinearColor>& Colors,float Glow)
{
    SetWorldLocationAndRotation(Position,Rotation);
    SetWorldScale3D(FVector::OneVector);
    if(CueMaterial) CueMaterial->SetScalarParameterValue(TEXT("Glow"),Glow);
    const FProcMeshSection* Existing=GetProcMeshSection(0);
    if(Existing && Existing->ProcVertexBuffer.Num()==Vertices.Num() && Existing->ProcIndexBuffer.Num()==Indices.Num())
        UpdateMeshSection_LinearColor(0,Vertices,TArray<FVector>(),TArray<FVector2D>(),Colors,TArray<FProcMeshTangent>(),false);
    else
        CreateMeshSection_LinearColor(0,Vertices,Indices,TArray<FVector>(),TArray<FVector2D>(),Colors,TArray<FProcMeshTangent>(),false,false);
    SetVisibility(true);
}

void UDCCombatCueComponent::ShowAlert(const FVector& Position,float Radius,float Progress,float Age,bool bVisible)
{
    if(!bVisible) { SetVisibility(false); return; }
    const APlayerController* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
    if(!PC || !PC->PlayerCameraManager) { SetVisibility(false); return; }
    const auto Look=DCCombatCuePolicy::Look(Progress,Age);
    Radius=FMath::Clamp(Radius,100.f,450.f)*Look.RadiusScale;
    FGeometry G;
    const FLinearColor Gold(.85f,.49f,.15f,.85f), Muted(.30f,.45f,.40f,.32f);
    G.Arc(Radius,.07f*Radius,0,UE_TWO_PI,0,Muted);
    G.Arc(Radius,.055f*Radius,UE_HALF_PI,UE_HALF_PI+UE_TWO_PI*Look.ArcFraction,1,Gold);
    // A tapered exclamation, not font/text rendering: consistent at distance.
    G.Quad(FVector(-.11f*Radius,.52f*Radius,2),FVector(.11f*Radius,.52f*Radius,2),
        FVector(.065f*Radius,-.08f*Radius,2),FVector(-.065f*Radius,-.08f*Radius,2),Gold);
    G.Quad(FVector(0,-.22f*Radius,2),FVector(.12f*Radius,-.34f*Radius,2),
        FVector(0,-.46f*Radius,2),FVector(-.12f*Radius,-.34f*Radius,2),Gold);
    const FRotator View=PC->PlayerCameraManager->GetCameraRotation();
    // Local X = camera right, local Y = camera up; no rotating/screen-size pop.
    const FQuat Facing=FRotationMatrix::MakeFromXY(FRotationMatrix(View).GetUnitAxis(EAxis::Y),FRotationMatrix(View).GetUnitAxis(EAxis::Z)).ToQuat();
    Present(Position,Facing,G.V,G.I,G.C,Look.Glow);
}

void UDCCombatCueComponent::ShowWeakPoint(const FVector& Position,const FVector& Axis,float Radius,float Progress,float Age,float HitFlash,bool bVisible)
{
    if(!bVisible || Axis.IsNearlyZero()) { SetVisibility(false); return; }
    const auto Look=DCCombatCuePolicy::Look(Progress,Age,HitFlash);
    Radius=FMath::Clamp(Radius,140.f,260.f)*Look.RadiusScale;
    FGeometry G;
    const FLinearColor Gold=FMath::Lerp(FLinearColor(.86f,.54f,.18f,.8f),FLinearColor(1.f,.86f,.55f,.9f),FMath::Clamp(HitFlash,0.f,1.f));
    // Four broken gold bands closely hug the existing vulnerable joint.
    // Narrow sea-green rims separate the exposed band from dark wet flesh.
    for(int32 A=0;A<4;++A)
    {
        const float Start=A*UE_HALF_PI+.10f, End=(A+1)*UE_HALF_PI-.10f;
        G.Band(Radius,Radius*.32f,Start,End,Gold);
        G.Arc(Radius,Radius*.035f,Start,End,-Radius*.22f,FLinearColor(.20f,.52f,.43f,.5f));
        G.Arc(Radius,Radius*.035f,Start,End,Radius*.22f,FLinearColor(.20f,.52f,.43f,.5f));
    }
    Present(Position,FRotationMatrix::MakeFromZ(Axis).ToQuat(),G.V,G.I,G.C,Look.Glow);
}
