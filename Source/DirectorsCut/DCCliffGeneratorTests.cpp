#include "DCCliffGenerator.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCliffGeneratorTest,"DirectorsCut.Maelstrom.CliffContinuity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCliffGeneratorTest::RunTest(const FString& Parameters)
{
    using namespace DCCliffGenerator;
    // Include the narrow river opening as well as a 500m tall far cliff.
    const FSection Sections[]={{105000,162000,210000,-50000,1800},{168000,173000,210000,-5000,1800}};
    bool Finite=true,OutsideWater=true,Seam=true,Connected=true,InwardFaces=true;
    bool NarrowTalus=true,LowCap=true;
    float MinFan=MAX_flt,MaxFan=0.f;
    int32 SteepSamples=0,FaceSamples=0;
    float MinFaceRadius=MAX_flt,MaxFaceRadius=0.f;
    for(const FSection& S:Sections)
    {
        for(int32 Row=0;Row<=160;++Row)
        {
            const float V=Row/160.f;
            Seam &= FVector::Dist(Point(S,0,V,.5f),Point(S,UE_TWO_PI,V,.5f))<5.f;
            for(int32 Side=0;Side<192;++Side)
            {
                const float A=Side*UE_TWO_PI/192;
                const FVector P=Point(S,A,V,.5f);
                Finite &= !P.ContainsNaN() && P.Size()<1.e7;
                OutsideWater &= P.Size2D()>=S.ToeRadius-1.f;
                if(V>.21f && V<.83f)
                {
                    const FVector Up=Point(S,A,V+.0001f,.5f)-Point(S,A,V-.0001f,.5f);
                    const FVector Along=Point(S,A+.0001f,V,.5f)-Point(S,A-.0001f,V,.5f);
                    const FVector Normal=FVector::CrossProduct(Up,Along).GetSafeNormal();
                    InwardFaces &= FVector::DotProduct(Normal,FVector(FMath::Cos(A),FMath::Sin(A),0))<0;
                    if(S.LipRadius-S.ToeRadius>30000.f)
                    {
                        const float RadialRise=Point(S,A,V+.0001f,.5f).Size2D()
                            -Point(S,A,V-.0001f,.5f).Size2D();
                        ++FaceSamples;
                        SteepSamples+=FMath::Abs(RadialRise/Up.Z)<.25f ? 1 : 0;
                    }
                }
            }
        }
        for(int32 Side=0;Side<192;++Side)
        {
            const float A=Side*UE_TWO_PI/192;
            const FVector Toe=Point(S,A,0.f,0.f),Foot=Point(S,A,.20f,0.f);
            const float Fan=Foot.Size2D()-Toe.Size2D();
            // This catches the previous broad lit apron, which occupied most
            // of the horizontal toe-to-lip gap and hid the monumental face.
            NarrowTalus &= Fan<=(S.LipRadius-S.ToeRadius)*.30f+1.f;
            NarrowTalus &= Foot.Z-Toe.Z<=(S.TopZ-S.WaterZ)*.18f;
            if(S.LipRadius-S.ToeRadius>30000.f)
            {
                MinFan=FMath::Min(MinFan,Fan); MaxFan=FMath::Max(MaxFan,Fan);
                const float FaceR=Point(S,A,.456f,0.f).Size2D();
                MinFaceRadius=FMath::Min(MinFaceRadius,FaceR);
                MaxFaceRadius=FMath::Max(MaxFaceRadius,FaceR);
            }
            for(float CapV:{.84f,.90f,.96f,1.f})
            { LowCap &= FMath::Abs(Point(S,A,CapV,0.f).Z-S.TopZ)<350.f; }
        }
        for(float A:{0.f,.7f,2.3f,5.7f}) for(float Boundary:{.20f,.84f})
        { Connected &= FVector::Dist(Point(S,A,Boundary-.000001f,0),Point(S,A,Boundary+.000001f,0))<10.f; }
    }
    TestTrue(TEXT("Finite bounded generated geometry"),Finite);
    TestTrue(TEXT("Closed circumferential seam"),Seam);
    TestTrue(TEXT("Toe, wall and cap connect"),Connected);
    TestTrue(TEXT("Even narrow inlet cliffs stay outside receiving water"),OutsideWater);
    TestTrue(TEXT("Cliff face normals point toward the crater"),InwardFaces);
    TestTrue(TEXT("Talus stays a minority of cliff footprint and height"),NarrowTalus);
    TestTrue(TEXT("Debris fans vary instead of forming a constant-width apron"),MaxFan>MinFan*1.25f);
    TestTrue(TEXT("Plateau behind temples stays low"),LowCap);
    // The earlier 220m-minimum range assertion rewarded the rejected giant
    // terraces. Bound this instead: visible rock masses, without moving almost
    // the complete shoreline-to-rim gap into shelves at one elevation. Image
    // review remains the acceptance test for geological appearance.
    TestTrue(TEXT("Fractured escarpment includes near-vertical local faces"),SteepSamples>FaceSamples*.10f);
    const float RadialRange=MaxFaceRadius-MinFaceRadius;
    TestTrue(TEXT("Rock masses vary without becoming crater-wide shelves"),RadialRange>2000.f && RadialRange<(Sections[0].LipRadius-Sections[0].ToeRadius)*.60f);
    float MaxRelief=0.f,AcrossChange=0.f,UpChange=0.f;
    for(int32 Side=0;Side<256;++Side) for(int32 Row=8;Row<56;++Row)
    {
        const float A=Side/256.f,T=Row/64.f;
        const float Offset=FaceRelief(A,T).Offset;
        MaxRelief=FMath::Max(MaxRelief,FMath::Abs(Offset));
        // Approximately equal 10m steps on a 10km circumference / 500m face.
        AcrossChange+=FMath::Abs(FaceRelief(A+.001f,T).Offset-Offset);
        UpChange+=FMath::Abs(FaceRelief(A,T+.02f).Offset-Offset);
    }
    TestTrue(TEXT("Local fracture projection stays below 25m, not giant cap shelves"),MaxRelief<2500.f);
    TestTrue(TEXT("Fracture relief persists vertically rather than making horizontal shelves"),AcrossChange>UpChange*2.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCliffWaterContactTest,"DirectorsCut.Maelstrom.CliffWaterContact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCliffWaterContactTest::RunTest(const FString& Parameters)
{
    using namespace DCCliffGenerator;
    // An overhanging rock face: raising an inward-offset point embeds it in the
    // ledge. This reproduces the sign error that cut dark bands through falls.
    const auto Vertex=[](int32 Side,int32 Row)
    {
        const float Angle=Side*UE_TWO_PI/MeshSectors;
        const float R=300000.f-Row*800.f;
        return FVector(FMath::Cos(Angle)*R,FMath::Sin(Angle)*R,Row*100.f);
    };
    const float Angle=12.33f*UE_TWO_PI/MeshSectors,V=60.37f/MeshRows;
    const FVector Surface=SampleRenderedSurface(Angle,V,Vertex);
    const FVector P00=Vertex(12,60),P10=Vertex(13,60),P01=Vertex(12,61);
    const FVector Normal=FVector::CrossProduct(P01-P00,P10-P00).GetSafeNormal();
    TestTrue(TEXT("Contact follows the rendered triangle plane"),FMath::Abs(FVector::DotProduct(Surface-P00,Normal))<.1f);
    TestTrue(TEXT("Radial contact offset clears overhanging rock"),FVector::DotProduct(WaterContactOffset(Angle,V),Normal)>10.f);
    TestTrue(TEXT("Regression fixture reproduces old upward-offset penetration"),
        FVector::DotProduct(WaterContactOffset(Angle,V)+FVector(0,0,60),Normal)<0.f);
    TestTrue(TEXT("Rendered contact sampler wraps negative angles"),
        FVector::Dist(SampleRenderedSurface(-.02f,.45f,Vertex),SampleRenderedSurface(UE_TWO_PI-.02f,.45f,Vertex))<1.f);

    // A folded quad is not a bilinear patch. Check both actual triangle halves;
    // sampling the smooth analytic function again would restore the mismatch.
    const auto Fold=[](int32 Side,int32 Row)
    { return FVector(Side*100.f,Row*100.f,(Side==8 && Row==51)?800.f:0.f); };
    const FVector Lower=SampleRenderedSurface(7.2f*UE_TWO_PI/MeshSectors,50.3f/MeshRows,Fold);
    const FVector Upper=SampleRenderedSurface(7.8f*UE_TWO_PI/MeshSectors,50.7f/MeshRows,Fold);
    TestTrue(TEXT("Lower triangle excludes the opposite folded corner"),FMath::Abs(Lower.Z)<.1f);
    TestTrue(TEXT("Upper triangle follows the mesh diagonal"),FMath::Abs(Upper.Z-400.f)<.1f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCFirstReceivingWaterTest,"DirectorsCut.Maelstrom.FirstReceivingWaterContact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCFirstReceivingWaterTest::RunTest(const FString& Parameters)
{
    using namespace DCCliffGenerator;
    const auto Water=[](const FVector2D& P,float& Height)
    {
        if(P.SizeSquared()>100.f || P.SizeSquared()<1.f) { return false; }
        Height=0.f; return true;
    };
    const auto SideEntry=FirstWaterContact([](float T){return FVector(20.f-15.f*T,0,-5);},Water);
    TestTrue(TEXT("Extrapolated water outside coverage cannot become a contact"),SideEntry.bFound && FMath::Abs(SideEntry.Progress-2.f/3.f)<.00001f);
    TestTrue(TEXT("Submerged side-entry endpoint is not lifted into an upward curl"),FMath::Abs(SideEntry.Position.Z+5.f)<.001f);
    TestTrue(TEXT("Impact can use mean water separately from the core endpoint"),SideEntry.WaterHeight==0.f);

    const auto DipThenRise=FirstWaterContact([](float T){return FVector(5,0,1000.f*(1.f-4.f*T+3.f*T*T));},Water);
    TestTrue(TEXT("First intersection wins over a later upward return"),DipThenRise.bFound && FMath::Abs(DipThenRise.Progress-1.f/3.f)<.00001f);
    const auto NeverCovered=FirstWaterContact([](float T){return FVector(20,0,-T*100.f);},Water);
    TestFalse(TEXT("A path entirely outside the water mesh has no receiving contact"),NeverCovered.bFound);

    bool BranchesCovered=true,NoEarlierContact=true;
    for(int32 Lane=0;Lane<33;++Lane)
    {
        const float Across=(Lane-16)*.18f;
        const auto Path=[Across](float T){return FVector(20.f-15.f*T,Across,50.f-80.f*T);};
        const auto Hit=FirstWaterContact(Path,Water);
        float H=0.f;
        BranchesCovered &= Hit.bFound && Water(FVector2D(Hit.Position),H) && Hit.Position.Z<=H;
        const FVector Earlier=Path(FMath::Max(0.f,Hit.Progress-.0001f));
        NoEarlierContact &= !Water(FVector2D(Earlier),H) || Earlier.Z>H;
    }
    TestTrue(TEXT("All lateral branch endpoints lie in covered water at or below mean surface"),BranchesCovered);
    TestTrue(TEXT("Lateral branch paths stop at first entry"),NoEarlierContact);
    return true;
}
#endif
