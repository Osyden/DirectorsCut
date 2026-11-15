#include "DCCityObstacleLayout.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCityObstacleLayoutTest,"DirectorsCut.Navigation.CityPillarClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCityObstacleLayoutTest::RunTest(const FString& Parameters)
{
    using namespace DCCityObstacleLayout;
    TestTrue(TEXT("A ten-metre hull keeps at least ten metres spare on the central route"),HullClearance(500.f)>=1000.f);
    TestTrue(TEXT("Minimum interval gives over 100m between full footprints at city speed"),Interval*.9f*1500.f-2.f*MaximumFootprint>10000.f);
    for(int32 I=0;I<20;++I)
    {
        TestTrue(TEXT("Successive obstacles have opposite clear flanks"),Side(I)*Side(I+1)<0.f);
        for(float Y : {MinimumSide,MaximumSide})
        {
            for(float A=0.f;A<UE_TWO_PI;A+=.1f)
            {
                const float Inner=FMath::Abs(Side(I)*Y+FMath::Sin(A)*MaximumFootprint);
                TestTrue(TEXT("Any rotated point of the fitted cluster clears the central hull"),Inner>500.f+1000.f);
            }
        }
    }
    return true;
}
#endif
