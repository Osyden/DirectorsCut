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
    for(int32 Wave=0;Wave<180;++Wave)
    {
        const bool InChannel=Wave>=12 && Wave<32;
        const float A=Wave<12 ? DCMaelstromRoute::Angle(5.f+Wave*13.f) : (InChannel
            ? DCMaelstromRoute::Angle(4.f+(Wave-12)*8.f) : Random.FRandRange(-UE_PI,UE_PI));
        const float RouteR=Radius*DCMaelstromRoute::RadialFraction(DCMaelstromRoute::SecondsAtAngle(A));
        const float Width=DCMaelstromRoute::WidthScale(DCMaelstromRoute::SecondsAtAngle(A));
        const float R=Wave<12 ? RouteR+(Wave%2 ? 14500.f : -8500.f)*Width : (InChannel
            ? RouteR+Random.FRandRange(-5000.f,5000.f) : Radius*Random.FRandRange(.22f,.91f));
        if(!InChannel && FMath::Abs(R-RouteR)<8500.f) { continue; }
        const FVector2D Across(FMath::Cos(A),FMath::Sin(A));
        const FVector2D Along(Across.Y,-Across.X);
        if(R>WaterEdgeRadius(A)-1200.f) { continue; }
        BreakingWaves.Add({Across*R,Along,Across,static_cast<float>(Random.FRandRange(2200.f,6500.f)),
            static_cast<float>(InChannel ? Random.FRandRange(650.f,1300.f) : Random.FRandRange(500.f,1100.f)),
            static_cast<float>(InChannel ? Random.FRandRange(160.f,330.f) : Random.FRandRange(220.f,550.f)),
            static_cast<float>(Random.FRandRange(0.f,20.f))});
    }
    for(const FBreakingWave& W:BreakingWaves) for(int32 J=-2;J<=2;++J)
    {
        const float Along=J*W.Length*.12f;
        const float Bend=Along*.29f+Along*Along/(2.f*W.Center.Size())+FMath::Sin(Along/W.Length*9.f+W.Seed)*100.f;
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
            const float Along=(Sheet-.5f)*W.Length*.3f;
            const float Bend=Along*.29f+Along*Along/(2.f*W.Center.Size())+FMath::Sin(Along/W.Length*9.f+W.Seed)*100.f;
            const FVector2D P=W.Center+W.Along*Along-W.Across*Bend;
            const FVector Foot(P,HeightAt(P)-40.f);
            const float H=FMath::Max(280.f,W.Height*1.6f),Width=W.Length*.12f;
            const int32 First=V.Num();
            constexpr int32 Rows=6,Cols=8;
            for(int32 Y=0;Y<=Rows;++Y) for(int32 X=0;X<=Cols;++X)
            {
                const float U=static_cast<float>(X)/Cols,T=static_cast<float>(Y)/Rows;
                const FVector2D Shift=W.Along*((U-.5f)*Width+T*H*.95f)-W.Across*T*H*.35f;
                V.Add(Foot+FVector(Shift,FMath::Sin(T*UE_PI*.85f)*H)); UV.Add(FVector2D(U,T));
                C.Add(FLinearColor(.65f,FMath::Frac(W.Seed),Sheet*.5f,.6f).ToFColor(false));
                if(Y<Rows && X<Cols) { const int32 A=First+Y*(Cols+1)+X,B=A+Cols+1; I.Append({A,B,A+1,A+1,B,B+1}); }
            }
        }
    }
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V,I,UV,N,Tangents);
    CrestSpray->CreateMeshSection(0,V,I,N,UV,C,Tangents,false);
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_MaelstromImpactMist.M_MaelstromImpactMist")))
    {
        CrestSprayMID=UMaterialInstanceDynamic::Create(M,this);
        CrestSprayMID->SetScalarParameterValue(TEXT("Density"),.35f);
        CrestSprayMID->SetScalarParameterValue(TEXT("DepthFadeDistance"),80.f);
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
        const float Across=FVector2D::DotProduct(Delta,W.Across)+Along*.29f
            +Along*Along/(2.f*W.Center.Size())+FMath::Sin(Along/W.Length*9.f+W.Seed)*100.f;
        // Long water face and short spilling edge; ordinary crests only 1–4m
        // high. Packet curvature follows the descending leftward current.
        const float Width=W.Width*(Across<0.f ? .60f : 2.0f);
        const float X=FMath::Abs(Across)/Width;
        if(X>1.f) { continue; }
        const float Edge=1-FMath::SmoothStep(.25f,.5f,FMath::Abs(Along)/W.Length);
        const float Profile=1.f-FMath::SmoothStep(0.f,1.f,X);
        const float Broken=.75f+.25f*FMath::PerlinNoise2D(Point/1500.f+FVector2D(W.Seed,0));
        Height=FMath::Max(Height,W.Height*Profile*Edge*Broken);
        Cap=FMath::Max(Cap,(1.f-FMath::SmoothStep(65.f,340.f,FMath::Abs(Across)))*Edge*Broken);
    }
    if(Foam) { *Foam=Cap; }
    return Height;
}

void ADCMaelstrom::UpdateBreakers()
{
    if(CrestSprayMID)
    {
        CrestSprayMID->SetScalarParameterValue(TEXT("Reveal"),FMath::SmoothStep(0.f,8.f,Elapsed));
    }
    if(Elapsed<5.f || Elapsed<NextBreakerSpray || BreakerSprayPoints.IsEmpty()) { return; }
    NextBreakerSpray=Elapsed+.08f;
    const FTransform Frame=GetArenaTransform();
    for(int32 K=0;K<12;++K)
    {
        const FVector P=Frame.TransformPosition(BreakerSprayPoints[FMath::RandHelper(BreakerSprayPoints.Num())]);
        if(P.Size2D()>4500.f && P.Size2D()<40000.f)
        {
            // Separate crest droplets from explosion-like contact splashes.
            // Follow the current, then fall under gravity in the basin frame.
            const FVector Local=Frame.InverseTransformPosition(P);
            const FVector2D Radial=FVector2D(Local).GetSafeNormal();
            const FVector Current(Radial.Y*800.f-Radial.X*250.f,-Radial.X*800.f-Radial.Y*250.f,0.f);
            for(int32 Drop=0;Drop<22 && ContactParticles.Num()<900;++Drop)
            {
                const FVector Jitter=FVector(FMath::FRandRange(-180.f,180.f),FMath::FRandRange(-180.f,180.f),0.f);
                const FVector Launch=Current+FVector(FMath::FRandRange(-160.f,160.f),FMath::FRandRange(-160.f,160.f),FMath::FRandRange(300.f,850.f));
                ContactParticles.Add({Local+Jitter,Launch,0.f,FMath::FRandRange(.7f,1.7f),FMath::FRandRange(4.f,20.f)});
            }
            break;
        }
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
    TArray<uint8> Pixels; Pixels.SetNumZeroed(Size*Size*4);
    FColor* Colors=reinterpret_cast<FColor*>(Pixels.GetData());
    TArray<float> Fresh,Wake;
    Fresh.Init(0.f,Size*Size); Wake.Init(0.f,Size*Size);
    const float Pixel=Span/Size;
    // Rasterize filtered source footprints instead of point-sampling a 1m
    // lip in a roughly 5m texel: that discarded most breaking fronts entirely.
    auto Stamp=[&](TArray<float>& Field,const FVector2D& P,float Width,float Strength)
    {
        const float Filtered=FMath::Sqrt(Width*Width+Pixel*Pixel*.25f);
        const float Energy=Strength*Width/Filtered;
        const int32 CX=FMath::FloorToInt((P.X/Span+.5f)*Size),CY=FMath::FloorToInt((P.Y/Span+.5f)*Size);
        const int32 Reach=FMath::CeilToInt(Filtered*2.4f/Pixel);
        for(int32 Y=FMath::Max(0,CY-Reach);Y<=FMath::Min(Size-1,CY+Reach);++Y)
        for(int32 X=FMath::Max(0,CX-Reach);X<=FMath::Min(Size-1,CX+Reach);++X)
        {
            const FVector2D At(((X+.5f)/Size-.5f)*Span,((Y+.5f)/Size-.5f)*Span);
            const float D=(At-P).SizeSquared()/(Filtered*Filtered);
            Field[Y*Size+X]=FMath::Max(Field[Y*Size+X],Energy*FMath::Exp(-D*1.6f));
        }
    };
    for(const FBreakingWave& W:BreakingWaves)
    {
        const int32 Samples=FMath::Max(8,FMath::CeilToInt(W.Length/Pixel*2.f));
        for(int32 Sample=0;Sample<=Samples;++Sample)
        {
            const float Along=(static_cast<float>(Sample)/Samples-.5f)*W.Length;
            const float Edge=1.f-FMath::SmoothStep(.25f,.50f,FMath::Abs(Along)/W.Length);
            if(Edge<.01f) { continue; }
            const float Bend=Along*.29f+Along*Along/(2.f*W.Center.Size())+FMath::Sin(Along/W.Length*9.f+W.Seed)*100.f;
            FVector2D P=W.Center+W.Along*Along-W.Across*Bend;
            Stamp(Fresh,P,240.f,Edge);
            // A twenty-second foam wake spreads behind the breaking front,
            // taking the same current direction as the waterfall impact trails.
            for(int32 Step=0;Step<21;++Step)
            {
                const float Age=static_cast<float>(Step);
                Stamp(Wake,P,600.f+Age*90.f,Edge*.90f*FMath::Exp(-Age/15.f));
                const float T=DCMaelstromRoute::SecondsAtAngle(FMath::Atan2(P.Y,P.X));
                P+=DCMaelstromRoute::Tangent(FMath::Clamp(T,-7.f,179.f),Radius)*1800.f;
            }
        }
    }
    // Green is a separate whitewater source, stamped along the actual current.
    // Advected material samples carry bubbles downstream; the stationary sample
    // keeps each impact attached to its tributary rather than moving the source.
    TArray<float> Impact; Impact.Init(0.f,Size*Size);
    for(const FWaterFeed& Feed:WaterFeeds) for(int32 Step=0;Step<Feed.Trail.Num();++Step)
    {
        const FVector2D P=Feed.Trail[Step];
        const float Age=Step*1.2f,Width=Feed.Width*(.8f+Age*.035f);
        const float Strength=FMath::Exp(-Age/12.f);
        const int32 CX=FMath::RoundToInt((P.X/Span+.5f)*Size),CY=FMath::RoundToInt((P.Y/Span+.5f)*Size);
        const int32 Reach=FMath::CeilToInt(Width*2.2f/Pixel);
        for(int32 Y=FMath::Max(0,CY-Reach);Y<=FMath::Min(Size-1,CY+Reach);++Y)
        for(int32 X=FMath::Max(0,CX-Reach);X<=FMath::Min(Size-1,CX+Reach);++X)
        {
            const FVector2D At(((X+.5f)/Size-.5f)*Span,((Y+.5f)/Size-.5f)*Span);
            const float D=(At-P).SizeSquared()/(Width*Width);
            Impact[Y*Size+X]=FMath::Max(Impact[Y*Size+X],Strength*FMath::Exp(-D*1.8f));
        }
    }
    for(int32 Index=0;Index<Size*Size;++Index)
    {
        Colors[Index]=FLinearColor(Fresh[Index],Impact[Index],Wake[Index],1.f).ToFColorSRGB();
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
