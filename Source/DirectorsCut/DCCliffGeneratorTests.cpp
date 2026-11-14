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
                }
            }
        }
        for(float A:{0.f,.7f,2.3f,5.7f}) for(float Boundary:{.20f,.84f})
        { Connected &= FVector::Dist(Point(S,A,Boundary-.000001f,0),Point(S,A,Boundary+.000001f,0))<10.f; }
    }
    TestTrue(TEXT("Finite bounded generated geometry"),Finite);
    TestTrue(TEXT("Closed circumferential seam"),Seam);
    TestTrue(TEXT("Toe, wall and cap connect"),Connected);
    TestTrue(TEXT("Even narrow inlet cliffs stay outside receiving water"),OutsideWater);
    TestTrue(TEXT("Cliff face normals point toward the crater"),InwardFaces);
    return true;
}
#endif
