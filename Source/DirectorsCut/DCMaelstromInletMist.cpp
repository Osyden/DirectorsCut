#include "DCMaelstrom.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

void ADCMaelstrom::UpdateInletMist()
{
    const float Gather=FMath::SmoothStep(26.f,42.f,Elapsed);
    if(!IsCalderaLake()||!bActive||Gather<=0.f)
    { ClearInletMist(); return; }

    if(!bInletMistTried)
    {
        bInletMistTried=true;
        UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EasyFog/Meshes/SM_easyFogCard_02.SM_easyFogCard_02"));
        UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/EasyFog/Materials/M_BP_EasyFog.M_BP_EasyFog"));
        const TCHAR* Masks[]={
            TEXT("/Game/EasyFog/Textures/Fog/T_mountainFog_02_mask.T_mountainFog_02_mask"),
            TEXT("/Game/EasyFog/Textures/Fog/T_mountainFog_05_mask.T_mountainFog_05_mask"),
            TEXT("/Game/EasyFog/Textures/Fog/T_mountainFog_07_mask.T_mountainFog_07_mask"),
            TEXT("/Game/EasyFog/Textures/Fog/T_smoothRadial_mask.T_smoothRadial_mask")};
        UTexture* Textures[4]={};
        for(int32 I=0;I<4;++I) { Textures[I]=LoadObject<UTexture>(nullptr,Masks[I]); }
        if(!Mesh||!Base||!Textures[3])
        {
            UE_LOG(LogTemp,Warning,TEXT("DC_CALDERA_INLET_MIST missing EasyFog mesh/material/soft mask"));
            return;
        }
        for(int32 I=0;I<8;++I)
        {
            UStaticMeshComponent* Card=NewObject<UStaticMeshComponent>(this);
            AddInstanceComponent(Card);
            Card->SetupAttachment(Arena);
            Card->SetMobility(EComponentMobility::Movable);
            Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Card->SetStaticMesh(Mesh);
            Card->SetCastShadow(false);
            Card->SetVisibility(false);
            Card->TranslucencySortPriority=10+I;
            UMaterialInstanceDynamic* MID=UMaterialInstanceDynamic::Create(Base,this);
            MID->SetTextureParameterValue(TEXT("Opacity Map"),Textures[I%4]?Textures[I%4]:Textures[3]);
            MID->SetScalarParameterValue(TEXT("Fog Density"),0.f);
            MID->SetScalarParameterValue(TEXT("Fading Distance"),2000.f);
            MID->SetScalarParameterValue(TEXT("Camera Fading Distance"),1800.f);
            MID->SetScalarParameterValue(TEXT("Emissive Intensity"),0.f);
            MID->SetVectorParameterValue(TEXT("Base Color Tint"),FLinearColor(.76f,.82f,.86f));
            Card->SetMaterial(0,MID);
            Card->RegisterComponent();
            InletMistCards.Add(Card); InletMistMIDs.Add(MID);
        }
        UE_LOG(LogTemp,Display,TEXT("DC_CALDERA_INLET_MIST cards=8 width_m=380..520 scene_lit=1 emissive=0 gather_elapsed=26..42"));
    }

    FVector Eye; FRotator View;
    APlayerController* Player=GetWorld()->GetFirstPlayerController();
    if(!Player) { ClearInletMist(); return; }
    Player->GetPlayerViewPoint(Eye,View);
    const FTransform Frame=GetArenaTransform();
    static const float AcrossM[]={-65.f,70.f,-35.f,80.f,-75.f,55.f,-25.f,35.f};
    static const float WidthM[]={450.f,380.f,520.f,410.f,480.f,390.f,510.f,460.f};
    for(int32 I=0;I<InletMistCards.Num();++I)
    {
        UStaticMeshComponent* Card=InletMistCards[I];
        UMaterialInstanceDynamic* MID=InletMistMIDs[I];
        if(!Card||!MID||!Card->GetStaticMesh()) { continue; }
        const float R=FMath::Lerp(.975f,.595f,I/7.f)*Radius;
        const float Q=FMath::SmoothStep(.58f,.985f,R/Radius);
        const float Across=AcrossM[I]*100.f+FMath::Sin(Elapsed*.18f+I*1.4f)*550.f;
        const float A=UE_PI-.40f*(1.f-Q)+Across/R;
        const float Lift=10000.f+1800.f*FMath::Sin(I*2.1f);
        const FVector Local(FMath::Cos(A)*R,FMath::Sin(A)*R,-40000.f+40000.f*Q+Lift);
        const FVector Center=Frame.TransformPosition(Local);
        FVector Normal=(Eye-Center).GetSafeNormal();
        if(Normal.IsNearlyZero()) { Normal=-View.Vector(); }
        // This card's authored face is XY, normal +Z (the same orientation
        // used by the existing spout skirt). Keep its Y axis visually upright
        // while its normal follows the actual view, including review cameras.
        const FVector Upright=FMath::Abs(Normal.Z)>.98f?FVector::ForwardVector:FVector::UpVector;
        const FQuat Facing=FRotationMatrix::MakeFromZY(Normal,Upright).ToQuat();
        const FBox Bounds=Card->GetStaticMesh()->GetBoundingBox();
        const FVector Size=Bounds.GetSize();
        const float Wide=WidthM[I]*100.f;
        const FVector Scale(Wide/FMath::Max(1.f,static_cast<float>(Size.X)),
            Wide*.72f/FMath::Max(1.f,static_cast<float>(Size.Y)),
            Wide/FMath::Max(1.f,static_cast<float>(Size.X))*.25f);
        const FVector Origin=Center-Facing.RotateVector(Bounds.GetCenter()*Scale);
        Card->SetWorldTransform(FTransform(Facing,Origin,Scale));
        MID->SetScalarParameterValue(TEXT("Fog Density"),Gather*(1.55f+.15f*FMath::Sin(I*1.9f)));
        Card->SetVisibility(true);
    }
}

void ADCMaelstrom::ClearInletMist()
{
    for(UStaticMeshComponent* Card:InletMistCards)
    { if(Card) { Card->SetVisibility(false); } }
}
