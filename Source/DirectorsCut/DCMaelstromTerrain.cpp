#include "DCMaelstrom.h"
#include "DCMaelstromRoute.h"
#include "DCCliffGenerator.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

FVector ADCMaelstrom::GetShorelinePoint(float Angle) const
{
    const FVector2D P=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*WaterEdgeRadius(Angle);
    return FVector(P,HeightAt(P));
}

float ADCMaelstrom::WaterfallAngle(int32 Index) const
{
    // Nine drainage notches with unequal spacing: three groups separated by
    // hundreds of metres of exposed rock. Periodic falls make a vast crater
    // read as a garden fountain, whatever its world-space dimensions.
    static const float Angles[]={-2.77f,-2.63f,-1.81f,-.93f,-.82f,.12f,1.09f,1.24f,2.35f};
    return Index<UE_ARRAY_COUNT(Angles) ? Angles[Index] : 0.f;
}

float ADCMaelstrom::WaterfallWidth(int32 Index) const
{
    // Half-width in cm; distant falls must be fine features against a 3.2km rim.
    static const float Widths[]={1700.f,600.f,1100.f,2200.f,700.f,950.f,1600.f,550.f,1250.f};
    return Index<UE_ARRAY_COUNT(Widths) ? Widths[Index] : 0.f;
}

float ADCMaelstrom::WaterEdgeRadius(float Angle) const
{
    const FVector2D Direction(FMath::Cos(Angle),FMath::Sin(Angle));
    const float Lower=Radius*(.66f+.035f*FMath::PerlinNoise2D(Direction*5.3f+FVector2D(7,13)));
    const float T=DCMaelstromRoute::SecondsAtAngle(Angle);
    const float Channel=Radius*DCMaelstromRoute::RadialFraction(T)+16000.f*DCMaelstromRoute::WidthScale(T);
    // Keep the incoming river and full steering envelope. The rest of the
    // shoreline lies hundreds of metres inside the rock rim.
    const float Inlet=1.f-FMath::SmoothStep(20.f,45.f,T);
    const float Shore=FMath::Min(GetRimRadius(Angle)*(1.f+Inlet*.08f),FMath::Max(Lower,Channel));
    // SecondsAtAngle deliberately wraps at the inlet. It is a route coordinate,
    // not a periodic shoreline: using it directly jumped the coast by 658m
    // between adjacent sectors and pulled a 427m triangular wall across water.
    // Return smoothly to the inlet on its UNUSED backward side. The complete
    // playable river/spiral and its existing full-steer clearance stay unchanged.
    const float BackAngle=FMath::UnwindRadians(Angle-UE_PI);
    if(BackAngle>0.f && BackAngle<.55f)
    {
        const float Entry=FMath::Min(GetRimRadius(Angle)*1.08f,Radius*.985f+16000.f);
        return FMath::Lerp(Entry,Shore,FMath::SmoothStep(0.f,.55f,BackAngle));
    }
    return Shore;
}

FVector ADCMaelstrom::CliffSurfacePoint(float Angle,float V) const
{
    const float Toe=WaterEdgeRadius(Angle)*.995f;
    const float Lip=FMath::Max(GetRimRadius(Angle)*1.015f
        +1600.f*DCCliffGenerator::Facet(Angle/UE_TWO_PI,2.1f,48),Toe+5000.f);
    const FVector2D D(FMath::Cos(Angle),FMath::Sin(Angle));
    float Drainage=0.f;
    for(int32 I=0;I<WaterfallCount;++I)
    {
        const float W=WaterfallWidth(I);
        if(W<=0.f) { continue; }
        const float Across=FMath::Abs(FMath::UnwindRadians(Angle-WaterfallAngle(I)))*Lip;
        Drainage=FMath::Max(Drainage,1.f-FMath::SmoothStep(W*1.2f,W*2.8f,Across));
    }
    const DCCliffGenerator::FSection Section={Toe,Lip,FMath::Max(GetRimRadius(Angle)*1.32f,Lip+30000.f),
        HeightAt(D*Toe,false)-900.f,RimGroundHeight(Lip,Angle)};
    FVector Point=DCCliffGenerator::Point(Section,Angle,V,Drainage);
    if(V>=.84f)
    {
        for(float Degrees:DCCliffGenerator::TempleAngles)
        {
            const float A=FMath::DegreesToRadians(Degrees);
            const float PadR=FMath::Max(GetRimRadius(A)*1.015f
                +1600.f*DCCliffGenerator::Facet(A/UE_TWO_PI,2.1f,48),WaterEdgeRadius(A)*.995f+5000.f)+12000.f;
            const float Across=FMath::Abs(FMath::UnwindRadians(Angle-A))*PadR;
            const float Along=FMath::Abs(static_cast<float>(Point.Size2D())-PadR);
            const float Pad=1.f-FMath::SmoothStep(11500.f,15500.f,FMath::Max(Across,Along));
            Point.Z=FMath::Lerp(Point.Z,static_cast<double>(RimGroundHeight(PadR,A)),Pad);
        }
    }
    return Point;
}

float ADCMaelstrom::CliffHeightAt(const FVector2D& P) const
{
    // Conservative topmost radial intersection, only for legacy contact queries.
    // Geometry and waterfalls use the shared parametric surface directly.
    const float A=FMath::Atan2(P.Y,P.X),R=P.Size();
    float Z=HeightAt(P,false)-900.f;
    FVector Previous=CliffSurfacePoint(A,0.f);
    for(int32 I=1;I<=80;++I)
    {
        const FVector Next=CliffSurfacePoint(A,I/80.f);
        const float R0=Previous.Size2D(),R1=Next.Size2D();
        if(R>=FMath::Min(R0,R1) && R<=FMath::Max(R0,R1) && FMath::Abs(R1-R0)>.01f)
        { Z=FMath::Max(Z,FMath::Lerp(Previous.Z,Next.Z,(R-R0)/(R1-R0))); }
        Previous=Next;
    }
    return Z;
}

void ADCMaelstrom::BuildTerrain()
{
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<FColor> C;
    TArray<int32> I; TArray<FProcMeshTangent> Tangents;
    constexpr int32 Sectors=DCCliffGenerator::MeshSectors,Rows=DCCliffGenerator::MeshRows;
    for(int32 Row=0;Row<=Rows;++Row) for(int32 Side=0;Side<=Sectors;++Side)
    {
        const float A=Side*UE_TWO_PI/Sectors,T=static_cast<float>(Row)/Rows;
        const FVector P=CliffSurfacePoint(A,T);
        V.Add(P); N.Add(FVector::ZeroVector); Tangents.Add(FProcMeshTangent());
        UV.Add(FVector2D(A*Radius*.01f,P.Z*.01f));
        const float ToeZ=CliffSurfacePoint(A,0.f).Z+900.f;
        const float Wet=1.f-FMath::SmoothStep(150.f,1800.f,static_cast<float>(P.Z-ToeZ));
        // Recess shading follows actual geometric cleaves, not an unrelated
        // noise field painted on the wall.
        const float Cavity=(T>.2f && T<.84f)
            ? DCCliffGenerator::FaceRelief(A/UE_TWO_PI,(T-.2f)/.64f).Recess*.65f : 0.f;
        C.Add(FLinearColor(Wet,Cavity,0,1).ToFColor(false));
        if(Row<Rows && Side<Sectors)
        { const int32 B=Row*(Sectors+1)+Side,D=B+Sectors+1; I.Append({B,B+1,D,B+1,D+1,D}); }
    }
    // Lighting must describe the rendered triangles. Sub-cell analytic normals
    // exposed fractures absent from this mesh, making close faces look folded.
    for(int32 Triangle=0;Triangle<I.Num();Triangle+=3)
    {
        const int32 A=I[Triangle],B=I[Triangle+1],D=I[Triangle+2];
        const FVector WeightedNormal=FVector::CrossProduct(V[D]-V[A],V[B]-V[A]);
        N[A]+=WeightedNormal; N[B]+=WeightedNormal; N[D]+=WeightedNormal;
    }
    for(int32 Row=0;Row<=Rows;++Row)
    {
        const int32 First=Row*(Sectors+1),Last=First+Sectors;
        const FVector SeamNormal=N[First]+N[Last];
        N[First]=SeamNormal; N[Last]=SeamNormal;
        for(int32 Side=0;Side<=Sectors;++Side)
        {
            const int32 Index=First+Side;
            N[Index].Normalize();
            const FVector Along=V[First+(Side+1)%Sectors]-V[First+(Side+Sectors-1)%Sectors];
            Tangents[Index]=FProcMeshTangent((Along-N[Index]*FVector::DotProduct(Along,N[Index])).GetSafeNormal(),false);
        }
    }
    // UE clockwise winding faces into the basin and up from the plateau.
    Cliffs->CreateMeshSection(0,V,I,N,UV,C,Tangents,false);
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_CraterRock.M_CraterRock")))
    { Cliffs->SetMaterial(0,M); }
    RimLand->ClearAllMeshSections(); Falls->ClearAllMeshSections();
    float MinimumClearance=MAX_flt;
    for(int32 T=0;T<=178;++T)
    {
        const float Required=Radius*DCMaelstromRoute::RadialFraction(T)
            +11000.f*DCMaelstromRoute::WidthScale(T)+1500.f;
        MinimumClearance=FMath::Min(MinimumClearance,WaterEdgeRadius(DCMaelstromRoute::Angle(T))-Required);
    }
    ensureMsgf(MinimumClearance>0.f,TEXT("Rock shoreline enters the ship steering envelope"));
    ensureMsgf(FVector::Dist(CliffSurfacePoint(0,.6f),CliffSurfacePoint(UE_TWO_PI,.6f))<5.f,TEXT("Cliff seam is open"));
    UE_LOG(LogTemp,Display,TEXT("DC_CLIFFS parametric wall %d vertices, %d triangles, clearance %.1fm; no scaled rock props"),V.Num(),I.Num()/3,MinimumClearance*.01f);
}

void ADCMaelstrom::UpdateTerrainShadows()
{
    // The connected procedural face casts its own shadows. No proxy rocks.
}
