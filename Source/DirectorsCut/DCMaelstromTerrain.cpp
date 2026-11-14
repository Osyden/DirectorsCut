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
    if(V>0.f && V<.20f)
    {
        // The old quadratic rubble shelf stayed 6-16m below the receiving sea
        // at its visible boundary. Fit only this low shore join; keep the buried
        // toe, upper rock and all radial positions unchanged.
        const float Edge=WaterEdgeRadius(Angle);
        const float EdgeZ=HeightAt(D*Edge,false);
        const float RockR=Point.Size2D();
        const float Across=RockR-Edge;
        float ShoreZ;
        if(Across<=0.f) { ShoreZ=HeightAt(D*RockR,false); }
        else
        {
            // Outside coverage this is a dry geological ramp, not extrapolated
            // water. Match the bank tangent briefly, then approach a gentle slope
            // so the original higher talus can take over without a raised ridge.
            const float BankSlope=FMath::Clamp((EdgeZ-HeightAt(D*(Edge-300.f),false))/300.f,0.f,2.f);
            ShoreZ=EdgeZ+.04f*Across+(BankSlope-.04f)*200.f*(1.f-FMath::Exp(-Across/200.f));
        }
        // One metre of exposed shoreline headroom also survives interpolation
        // across the first coarse talus triangle beside the buried toe.
        const float Clearance=FMath::Clamp(100.f+.08f*Across,-100.f,160.f);
        // Fit the actual rock, independent of waterfall mesh clearance. The
        // supported bank submerges naturally toward its existing buried toe.
        const float TargetZ=ShoreZ+Clearance;
        const float Lift=FMath::Max(0.f,TargetZ-static_cast<float>(Point.Z));
        const float Blend=FMath::SmoothStep(0.f,.004f,V)*(1.f-FMath::SmoothStep(.14f,.20f,V));
        Point.Z+=Lift*FMath::SmoothStep(0.f,80.f,Lift)*Blend;
    }
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
    // BEGIN_CLIFF_CREASE_NORMALS (also extracted by the standalone CPU probe)
    // Keep section zero's complete position grid: waterfall contact samples it
    // directly. Only triangles that actually need a split normal move to the
    // second section, with identical positions, winding and triangle diagonals.
    const int32 GridCount=V.Num(),FaceCount=I.Num()/3;
    const float CreaseCos=FMath::Cos(FMath::DegreesToRadians(40.f));
    const auto Canonical=[&](int32 Vertex)
    { return Vertex%(Sectors+1)==Sectors ? Vertex-Sectors : Vertex; };
    struct FVertexFan { int32 Faces[6]={}; int32 Count=0; };
    TArray<FVertexFan> Fans; Fans.SetNum(GridCount);
    TArray<FVector> Weighted,FaceNormal; Weighted.SetNum(FaceCount); FaceNormal.SetNum(FaceCount);
    for(int32 Face=0;Face<FaceCount;++Face)
    {
        const int32 A=I[Face*3],B=I[Face*3+1],D=I[Face*3+2];
        Weighted[Face]=FVector::CrossProduct(V[D]-V[A],V[B]-V[A]);
        FaceNormal[Face]=Weighted[Face].GetSafeNormal();
        for(int32 Corner=0;Corner<3;++Corner)
        {
            FVertexFan& Fan=Fans[Canonical(I[Face*3+Corner])];
            check(Fan.Count<6);
            Fan.Faces[Fan.Count++]=Face;
        }
    }
    TArray<int32> Resolved=I,ExtraOriginal;
    TArray<FVector> ExtraNormals;
    int32 SplitFans=0;
    for(int32 Vertex=0;Vertex<GridCount;++Vertex)
    {
        const FVertexFan& Fan=Fans[Vertex];
        if(Fan.Count==0) { continue; }
        int32 Group[6]; for(int32 K=0;K<Fan.Count;++K) { Group[K]=K; }
        const auto Root=[&](int32 K) { while(Group[K]!=K) { K=Group[K]; } return K; };
        const float T=static_cast<float>(Vertex/(Sectors+1))/Rows;
        const bool PreserveSmooth=T<=.20f || T>=.84f;
        for(int32 A=0;A<Fan.Count;++A) for(int32 B=A+1;B<Fan.Count;++B)
        {
            // Smooth only through an actual shared edge. The duplicate seam
            // vertices participate in the same fan, so they cannot open a seam.
            int32 Shared=0;
            for(int32 CA=0;CA<3;++CA) for(int32 CB=0;CB<3;++CB)
            { Shared+=Canonical(I[Fan.Faces[A]*3+CA])==Canonical(I[Fan.Faces[B]*3+CB]) ? 1 : 0; }
            if(PreserveSmooth || (Shared==2 && FVector::DotProduct(FaceNormal[Fan.Faces[A]],FaceNormal[Fan.Faces[B]])>=CreaseCos))
            { Group[Root(B)]=Root(A); }
        }
        FVector Sum[6]; float Area[6]={};
        for(int32 K=0;K<6;++K) { Sum[K]=FVector::ZeroVector; }
        for(int32 K=0;K<Fan.Count;++K)
        { const int32 G=Root(K); Sum[G]+=Weighted[Fan.Faces[K]]; Area[G]+=Weighted[Fan.Faces[K]].Size(); }
        int32 Main=Root(0),Groups=0;
        for(int32 K=0;K<Fan.Count;++K) if(Root(K)==K)
        { ++Groups; if(Area[K]>Area[Main]) { Main=K; } Sum[K].Normalize(); }
        SplitFans+=Groups>1 ? 1 : 0;
        N[Vertex]=Sum[Main];
        if(Vertex%(Sectors+1)==0) { N[Vertex+Sectors]=Sum[Main]; }
        int32 SplitIndex[6][2];
        for(int32 G=0;G<6;++G) { SplitIndex[G][0]=SplitIndex[G][1]=INDEX_NONE; }
        for(int32 K=0;K<Fan.Count;++K) for(int32 Corner=0;Corner<3;++Corner)
        {
            const int32 Index=Fan.Faces[K]*3+Corner,Original=I[Index],G=Root(K);
            if(Canonical(Original)!=Vertex || G==Main) { continue; }
            const int32 SeamSide=Original==Vertex ? 0 : 1;
            int32& Split=SplitIndex[G][SeamSide];
            if(Split==INDEX_NONE)
            { Split=GridCount+ExtraOriginal.Add(Original); ExtraNormals.Add(Sum[G]); }
            Resolved[Index]=Split;
        }
    }
    TArray<int32> SmoothIndices,CreaseIndices,CreaseSources,Lookup;
    TArray<FVector> CreaseNormals;
    Lookup.Init(INDEX_NONE,GridCount+ExtraOriginal.Num());
    for(int32 Face=0;Face<FaceCount;++Face)
    {
        const int32 Base=Face*3;
        const bool Split=Resolved[Base]>=GridCount || Resolved[Base+1]>=GridCount || Resolved[Base+2]>=GridCount;
        for(int32 Corner=0;Corner<3;++Corner)
        {
            const int32 Index=Resolved[Base+Corner];
            if(!Split) { SmoothIndices.Add(Index); continue; }
            if(Lookup[Index]==INDEX_NONE)
            {
                const int32 Original=Index<GridCount ? Index : ExtraOriginal[Index-GridCount];
                Lookup[Index]=CreaseSources.Add(Original);
                CreaseNormals.Add(Index<GridCount ? N[Index] : ExtraNormals[Index-GridCount]);
            }
            CreaseIndices.Add(Lookup[Index]);
        }
    }
    check(SmoothIndices.Num()+CreaseIndices.Num()==I.Num());
    // END_CLIFF_CREASE_NORMALS
    for(int32 Row=0;Row<=Rows;++Row)
    {
        const int32 First=Row*(Sectors+1);
        for(int32 Side=0;Side<=Sectors;++Side)
        {
            const int32 Index=First+Side;
            N[Index].Normalize();
            const FVector Along=V[First+(Side+1)%Sectors]-V[First+(Side+Sectors-1)%Sectors];
            Tangents[Index]=FProcMeshTangent((Along-N[Index]*FVector::DotProduct(Along,N[Index])).GetSafeNormal(),false);
        }
    }
    TArray<FVector> CreaseV; TArray<FVector2D> CreaseUV; TArray<FColor> CreaseC;
    TArray<FProcMeshTangent> CreaseTangents;
    for(int32 K=0;K<CreaseSources.Num();++K)
    {
        const int32 Original=CreaseSources[K],Side=Original%(Sectors+1),First=Original-Side;
        const FVector Along=V[First+(Side+1)%Sectors]-V[First+(Side+Sectors-1)%Sectors];
        const FVector Normal=CreaseNormals[K];
        CreaseV.Add(V[Original]); CreaseUV.Add(UV[Original]); CreaseC.Add(C[Original]);
        CreaseTangents.Add(FProcMeshTangent((Along-Normal*FVector::DotProduct(Along,Normal)).GetSafeNormal(),false));
    }
    // UE clockwise winding faces into the basin and up from the plateau.
    Cliffs->ClearAllMeshSections();
    Cliffs->CreateMeshSection(0,V,SmoothIndices,N,UV,C,Tangents,false);
    if(!CreaseIndices.IsEmpty()) { Cliffs->CreateMeshSection(1,CreaseV,CreaseIndices,CreaseNormals,CreaseUV,CreaseC,CreaseTangents,false); }
    if(UMaterialInterface* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/DC/Maelstrom/M_CraterRock.M_CraterRock")))
    { Cliffs->SetMaterial(0,M); Cliffs->SetMaterial(1,M); }
    UE_LOG(LogTemp,Display,TEXT("DC_CLIFF_CREASES 40deg split_fans=%d extra_groups=%d section1_vertices=%d (%.2f%% overhead) section1_triangles=%d/%d; canonical grid unchanged"),
        SplitFans,ExtraOriginal.Num(),CreaseV.Num(),100.f*CreaseV.Num()/GridCount,CreaseIndices.Num()/3,FaceCount);
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
