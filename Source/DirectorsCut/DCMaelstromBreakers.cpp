#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
#include "DCRideDirector.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

void ADCMaelstrom::BuildBreakers()
{
    FRandomStream Random(14011);
    // These are height-field crests in the SAME water mesh. No overlapping
    // opaque ribbons, detached edges or second water volume interfaces.
    for(int32 Wave=0;Wave<48;++Wave)
    {
        const float A=Wave<4 ? UE_PI+.10f+Wave*.13f : Random.FRandRange(-UE_PI,UE_PI);
        const float R=Radius*(Wave<4 ? .90f-Wave*.09f : Random.FRandRange(.28f,.89f));
        const float RouteR=Radius*DCMaelstromRoute::RadialFraction(DCMaelstromRoute::SecondsAtAngle(A));
        if(FMath::Abs(R-RouteR)<10000.f) { continue; }
        const FVector2D Across(FMath::Cos(A),FMath::Sin(A));
        const FVector2D Along(-Across.Y,Across.X);
        BreakingWaves.Add({Across*R,Along,Across,static_cast<float>(Random.FRandRange(12000.f,26000.f)),
            static_cast<float>(Random.FRandRange(2200.f,4200.f)),static_cast<float>(Random.FRandRange(1000.f,2300.f)),static_cast<float>(Random.FRandRange(0.f,20.f))});
    }
    for(const FBreakingWave& W:BreakingWaves) for(int32 J=-2;J<=2;++J)
    {
        const float Along=J*W.Length*.12f;
        const float Bend=Along*.22f+FMath::Sin(Along/W.Length*9.f+W.Seed)*650.f;
        const FVector2D P=W.Center+W.Along*Along-W.Across*Bend;
        BreakerSprayPoints.Add(FVector(P,HeightAt(P)+150.f));
    }
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<FColor> C;
    TArray<int32> I; TArray<FProcMeshTangent> Tangents;
    for(int32 Index=0;Index<BreakingWaves.Num();++Index)
    {
        const FBreakingWave& W=BreakingWaves[Index];
        // Broken wind-blown spray is anchored to actual crests. Short, lit
        // sheets carry the detail; no solid white cone or full foam ring.
        for(int32 Sheet=0;Sheet<2;++Sheet)
        {
            const FVector2D P=W.Center+W.Along*((Sheet-.5f)*W.Length*.3f);
            const FVector Foot(P,HeightAt(P)-250.f);
            const float H=W.Height*(1.f+Sheet*.35f),Width=W.Length*.18f;
            const int32 First=V.Num();
            constexpr int32 Rows=6,Cols=8;
            for(int32 Y=0;Y<=Rows;++Y) for(int32 X=0;X<=Cols;++X)
            {
                const float U=static_cast<float>(X)/Cols,T=static_cast<float>(Y)/Rows;
                const FVector2D Shift=W.Along*((U-.5f)*Width+T*H*.25f)-W.Across*T*T*H*.5f;
                V.Add(Foot+FVector(Shift,T*H)); UV.Add(FVector2D(U,T));
                C.Add(FLinearColor(.65f,FMath::Frac(W.Seed),Sheet*.5f,.6f).ToFColor(false));
                if(Y<Rows && X<Cols) { const int32 A=First+Y*(Cols+1)+X,B=A+Cols+1; I.Append({A,B,A+1,A+1,B,B+1}); }
            }
        }
    }
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V,I,UV,N,Tangents);
    CrestSpray->CreateMeshSection(0,V,I,N,UV,C,Tangents,false);
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_MaelstromPlume.M_MaelstromPlume")))
    {
        CrestSprayMID=UMaterialInstanceDynamic::Create(M,this);
        CrestSprayMID->SetScalarParameterValue(TEXT("Density"),.75f);
        CrestSpray->SetMaterial(0,CrestSprayMID);
    }
}

float ADCMaelstrom::BreakerHeight(const FVector2D& Point,float* Foam) const
{
    float Height=0.f,Cap=0.f;
    for(const FBreakingWave& W:BreakingWaves)
    {
        const FVector2D Delta=Point-W.Center;
        const float Along=FVector2D::DotProduct(Delta,W.Along);
        if(FMath::Abs(Along)>W.Length*.5f) { continue; }
        const float Across=FVector2D::DotProduct(Delta,W.Across)+Along*.22f
            +FMath::Sin(Along/W.Length*9.f+W.Seed)*650.f;
        // Steep leading face and broad trailing shoulder, smoothly joined.
        const float Width=W.Width*FMath::Lerp(.62f,1.35f,FMath::SmoothStep(-W.Width*.3f,W.Width*.3f,Across));
        const float X=FMath::Abs(Across)/Width;
        if(X>2.5f) { continue; }
        const float Edge=1-FMath::SmoothStep(.25f,.5f,FMath::Abs(Along)/W.Length);
        const float Profile=FMath::Exp(-X*X*2.f);
        const float Broken=.7f+.3f*FMath::PerlinNoise2D(Point/6500.f+FVector2D(W.Seed,0));
        Height=FMath::Max(Height,W.Height*Profile*Edge*Broken);
        Cap=FMath::Max(Cap,FMath::SmoothStep(.62f,.94f,Profile)*Edge);
    }
    if(Foam) { *Foam=Cap; }
    return Height;
}

void ADCMaelstrom::UpdateBreakers()
{
    if(CrestSprayMID)
    {
        CrestSprayMID->SetScalarParameterValue(TEXT("Clock"),Elapsed);
        CrestSprayMID->SetScalarParameterValue(TEXT("Reveal"),FMath::SmoothStep(0.f,8.f,Elapsed));
    }
    if(Elapsed<5.f || Elapsed<NextBreakerSpray || BreakerSprayPoints.IsEmpty()) { return; }
    NextBreakerSpray=Elapsed+.10f;
    const FTransform Frame=GetArenaTransform();
    for(int32 K=0;K<12;++K)
    {
        const FVector P=Frame.TransformPosition(BreakerSprayPoints[FMath::RandHelper(BreakerSprayPoints.Num())]);
        if(P.Size2D()>7000.f && P.Size2D()<85000.f) { EmitWaterContact(P,.95f,false); break; }
    }
}

void ADCMaelstrom::BuildFoamSource()
{
    if(!WaterMID) { return; }
    // An original transient crest field, sampled in metres by the water shader.
    // It lets foam leave the breaking ridge rather than painting that ridge white.
    // No imported textures are modified or copied into this map.
    constexpr int32 Size=768;
    const float Span=Radius*2.48f;
    TArray<uint8> Pixels; Pixels.SetNumUninitialized(Size*Size*4);
    FColor* Colors=reinterpret_cast<FColor*>(Pixels.GetData());
    for(int32 Y=0;Y<Size;++Y) for(int32 X=0;X<Size;++X)
    {
        const FVector2D P(((X+.5f)/Size-.5f)*Span,((Y+.5f)/Size-.5f)*Span);
        float Crest=0.f;
        if(P.Size()<GetRimRadius(FMath::Atan2(P.Y,P.X)))
        {
            const float Breaking=BreakerHeight(P,&Crest);
            const float Swell=HeightAt(P)-HeightAt(P,false)-Breaking;
            Crest=FMath::Max(Crest,FMath::Clamp(Swell/2200.f,0.f,1.f)*.9f);
        }
        Colors[Y*Size+X]=FLinearColor(Crest,Crest,Crest,1.f).ToFColorSRGB();
    }
    FoamSourceTexture=UTexture2D::CreateTransient(Size,Size,PF_B8G8R8A8,NAME_None,
        TConstArrayView64<uint8>(Pixels.GetData(),Pixels.Num()));
    if(!FoamSourceTexture) { return; }
    FoamSourceTexture->SRGB=true;
    FoamSourceTexture->NeverStream=true;
    FoamSourceTexture->Filter=TF_Bilinear;
    FoamSourceTexture->AddressX=TA_Clamp; FoamSourceTexture->AddressY=TA_Clamp;
    FoamSourceTexture->UpdateResource();
    WaterMID->SetTextureParameterValue(TEXT("CrestSource"),FoamSourceTexture);
    WaterMID->SetScalarParameterValue(TEXT("CrestSourceAvailable"),1.f);
    WaterMID->SetScalarParameterValue(TEXT("CrestSourceInvSpan"),100.f/Span);
}
