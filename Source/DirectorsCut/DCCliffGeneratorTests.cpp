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
    return true;
}
#endif
