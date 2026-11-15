#include "DCMaelstrom.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
    // Centimetres in the same polar chart as the entry water and ship route.
    float InletPhase(float R,float Radius)
    { return FMath::SmoothStep(.58f,.985f,R/Radius); }

    float InletHalfWidth(float R,float Radius)
    {
        const float Q=InletPhase(R,Radius);
        return FMath::Lerp(41000.f,55000.f,Q)+3500.f*FMath::Sin(Q*4.7f+.3f);
    }

    FVector InletPoint(float R,float Across,float Radius)
    {
        const float A=UE_PI-.40f*(1.f-InletPhase(R,Radius))+Across/R;
        return FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,0.f);
    }

    float InletBedHeight(float R,float Across,float Radius)
    {
        const float Q=InletPhase(R,Radius);
        const float Ramp=FMath::Lerp(-40000.f,0.f,Q)
            -(1.f-FMath::SmoothStep(0.f,.03f,Q))*150.f;
        const float X=FMath::Abs(Across),Width=InletHalfWidth(R,Radius);
        const FVector P=InletPoint(R,Across,Radius);
        const float Bank=FMath::SmoothStep(19500.f,24000.f,X);
        const float SideFade=1.f-FMath::SmoothStep(28500.f,Width,X);
        const float Emergence=FMath::SmoothStep(.55f,.63f,R/Radius);
        const float Relief=550.f*FMath::PerlinNoise2D(FVector2D(P.X,P.Y)/4800.f)
            +220.f*FMath::PerlinNoise2D(FVector2D(P.X+7319.f,P.Y-11803.f)/1700.f);
        // Bed stays below the cataract; weathered banks and flanks form one
        // landform instead of an unsupported surface-water ribbon.
        const float Bed=Ramp-400.f;
        const float Shoulder=Bed+(1800.f+Relief)*Bank*Emergence;
        const float Flank=FMath::Lerp(-44000.f,Shoulder,SideFade);
        const float Foot=FMath::SmoothStep(.52f,.58f,R/Radius);
        return FMath::Lerp(-44000.f,Flank,Foot);
    }
}

void ADCMaelstrom::BuildInletBed()
{
    constexpr int32 Rows=192,Sides=160;
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<int32> I;
    TArray<FColor> C; TArray<FProcMeshTangent> T;
    V.Reserve((Rows+1)*(Sides+1));UV.Reserve(V.Max());C.Reserve(V.Max());
    const float UOrigin=529.f*160000.f*UE_TWO_PI/1536.f;
    for(int32 Row=0;Row<=Rows;++Row)
    {
        const float R=Radius*FMath::Lerp(1.20f,.52f,static_cast<float>(Row)/Rows);
        const float Width=InletHalfWidth(R,Radius);
        for(int32 Side=0;Side<=Sides;++Side)
        {
            const float Across=FMath::Lerp(-Width,Width,static_cast<float>(Side)/Sides);
            FVector P=InletPoint(R,Across,Radius);P.Z=InletBedHeight(R,Across,Radius);V.Add(P);
            float A=FMath::Atan2(P.Y,P.X);if(A<0.f) { A+=UE_TWO_PI; }
            // Identical full-ring context coordinates, with radial roof
            // continuation. R=0 selects the complete unique geology maps.
            UV.Add(FVector2D((A*Radius-UOrigin)/49741.8836818384,
                (P.Z+.35f*FMath::Max(0.f,R-Radius)+20000.f)/28000.f));
            C.Add(FColor(0,0,0,255));
            if(Row<Rows&&Side<Sides)
            { const int32 K=Row*(Sides+1)+Side,J=K+Sides+1;I.Append({K,J,K+1,K+1,J,J+1}); }
        }
    }
    N.SetNum(V.Num());T.SetNum(V.Num());
    for(int32 Row=0;Row<=Rows;++Row)for(int32 Side=0;Side<=Sides;++Side)
    {
        const int32 K=Row*(Sides+1)+Side;
        const FVector Across=V[Row*(Sides+1)+FMath::Min(Sides,Side+1)]-V[Row*(Sides+1)+FMath::Max(0,Side-1)];
        const FVector Down=V[FMath::Min(Rows,Row+1)*(Sides+1)+Side]-V[FMath::Max(0,Row-1)*(Sides+1)+Side];
        N[K]=FVector::CrossProduct(Across,Down).GetSafeNormal();
        T[K]=FProcMeshTangent(Across.GetSafeNormal(),false);
    }
    InletBed->CreateMeshSection(0,V,I,N,UV,C,T,false);
    UMaterialInterface* Rock=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffScanSurface.M_CliffScanSurface"));
    if(!Rock) { Rock=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_CraterRock.M_CraterRock")); }
    if(Rock) { InletBed->SetMaterial(0,Rock); }

    for(UInstancedStaticMeshComponent* Old:InletBoulders) { if(Old) { Old->DestroyComponent(); } }
    InletBoulders.Reset();
    const TCHAR* Paths[]={
        TEXT("/Game/LocalScans/vefkehbva/SM_Huge_Nordic_Coastal_Cliff_vefkehbva_Raw_53074843a3.SM_Huge_Nordic_Coastal_Cliff_vefkehbva_Raw_53074843a3"),
        TEXT("/Game/LocalScans/vfmjadufa/SM_Massive_Nordic_Coastal_Cliff_vfmjadufa_Raw_4064086b07.SM_Massive_Nordic_Coastal_Cliff_vfmjadufa_Raw_4064086b07")};
    FRandomStream Random(151126);
    for(int32 Kind=0;Kind<2;++Kind)
    {
        UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,Paths[Kind]);if(!Mesh) { continue; }
        UInstancedStaticMeshComponent* Rocks=NewObject<UInstancedStaticMeshComponent>(this);
        Rocks->SetupAttachment(InletBed);Rocks->SetMobility(EComponentMobility::Movable);
        Rocks->SetCollisionEnabled(ECollisionEnabled::NoCollision);Rocks->SetStaticMesh(Mesh);
        Rocks->RegisterComponent();InletBoulders.Add(Rocks);
        const FBox Bounds=Mesh->GetBoundingBox();
        for(int32 Index=0;Index<34;++Index)
        {
            const float R=Radius*Random.FRandRange(.61f,1.07f);
            const float Sign=(Index%2)==0?-1.f:1.f;
            const float Across=Sign*Random.FRandRange(25100.f,31500.f);
            FVector P=InletPoint(R,Across,Radius);P.Z=InletBedHeight(R,Across,Radius);
            const float Scale=Random.FRandRange(.85f,1.65f);
            const float A=FMath::Atan2(P.Y,P.X);
            const FRotator Rotation(Random.FRandRange(-6.f,6.f),
                FMath::RadiansToDegrees(A)+(Sign>0.f?180.f:0.f)+Random.FRandRange(-28.f,28.f),Random.FRandRange(-5.f,5.f));
            const FQuat Q=Rotation.Quaternion();
            // Bury the low source apron. These are bank accents, never rows
            // of enlarged scan tiles or the supporting landform itself.
            const FVector Anchor(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Min.Z+Bounds.GetSize().Z*.27f);
            P-=Q.RotateVector(Anchor*Scale);
            Rocks->AddInstance(FTransform(Q,P,FVector(Scale)));
        }
    }
}
