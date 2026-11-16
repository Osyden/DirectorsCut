#include "DCCityObstacleLayout.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCCityObstacleLayoutTest,"DirectorsCut.Navigation.CityPillarClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCCityObstacleLayoutTest::RunTest(const FString& Parameters)
{
    using namespace DCCityObstacleLayout;
    TestTrue(TEXT("Minimum interval leaves over 300m between longitudinal hull-clear volumes"),Interval*1500.f-2.f*(2200.f+MaximumFootprint)>30000.f);
    const float HullRadius=FMath::Sqrt(2200.f*2200.f+500.f*500.f);
    for(int32 I=0;I<Count;++I)
    {
        TestTrue(TEXT("Opposite passing lane clears even the fully yawed hull"),PassingClearance(I,HullRadius)>500.f);
        TestTrue(TEXT("Route remains far inside visible banks"),FMath::Abs(Lane(I))+MaximumFootprint<9000.f);
        const float Top=MinimumPillarTop(230.f,Lane(I),9000.f,1100.f);
        for(float FootY=-MaximumFootprint;FootY<=MaximumFootprint;FootY+=100.f)
        {
            const float U=FMath::Clamp(FMath::Abs(Lane(I)+FootY)/9000.f,0.f,1.f);
            const float HighestWater=230.f+1100.f*U*U*(3.f-2.f*U)+WaveAllowance;
            TestTrue(TEXT("Visible upright clears highest local wave by twenty-four metres"),Top-HighestWater>=MinimumVisiblePillar-.1f);
        }
    }
    TestTrue(TEXT("First cluster challenges an unattended centre route"),FMath::Abs(Lane(0))<MaximumFootprint+500.f);
    TestFalse(TEXT("The same small water ruin cannot cause repeated heavy impacts"),CanImpact(true,0.f));
    TestTrue(TEXT("Fresh ruin can impact once outside the shared cooldown"),CanImpact(false,0.f));
    TestEqual(TEXT("Fixed footing uses the same ship-frame vertical subtraction as water"),AnchoredZ(-100.f,700.f),-800.f);
    return true;
}
#endif
