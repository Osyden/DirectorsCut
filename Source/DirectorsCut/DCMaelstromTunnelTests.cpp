#include "DCMaelstromTunnel.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMaelstromTunnelTest,"DirectorsCut.Maelstrom.TunnelHullClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCMaelstromTunnelTest::RunTest(const FString& Parameters)
{
    using namespace DCMaelstromTunnel;
    // Test the full authored noise bound, flare and entrance width range.
    // Every hull-side point must remain below the deck and match the queried trough.
    for(float Scale : {1.f,2.f,30000.f/9000.f})
    {
        for(float Side : {-1100.f,-650.f,0.f,650.f,1100.f})
        {
            const float Angle=FMath::Asin(Side/(Radius*Scale));
            const float Expected=(Radius-FMath::Sqrt(Radius*Radius-FMath::Square(Side/Scale)))*Scale;
            for(float Ripple : {-1220.f,0.f,1220.f})
            {
                for(float Mouth : {0.f,.5f,1.f})
                {
                    const float Floor=(Radius-FMath::Cos(Angle)*ShapedRadius(Angle,Ripple,Mouth))*Scale;
                    TestTrue(TEXT("Tunnel floor agrees with surface query across hull"),FMath::Abs(Floor-Expected)<.02f);
                    TestTrue(TEXT("Water floor stays below the 580cm deck at sea level 230cm"),Floor+230.f<580.f);
                }
            }
        }
    }
    for(int32 I=0;I<=128;++I)
    {
        const float Angle=I*UE_TWO_PI/128.f;
        TestTrue(TEXT("No tunnel lip projects unsupported over the falls"),Lip(Angle,1.f)<=.01f);
    }
    TestTrue(TEXT("Tunnel ceiling retains authored silhouette"),ShapedRadius(UE_PI,1000.f,0.f)>Radius+999.f);
    return true;
}
#endif
