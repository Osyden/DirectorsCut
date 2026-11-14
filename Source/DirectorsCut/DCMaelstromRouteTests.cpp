#include "DCMaelstromRoute.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMaelstromRouteTest,"DirectorsCut.Maelstrom.RouteAndSteering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCMaelstromRouteTest::RunTest(const FString& Parameters)
{
    using namespace DCMaelstromRoute;
    // Continuous route through the drop, acceleration and phase boundaries.
    for (float T : {0.f,8.f,12.f,22.f,34.f,40.f,85.f,158.f,160.f})
    {
        const float Distance = (Point(T+.01f,160000.f)-Point(T-.01f,160000.f)).Size();
        TestTrue(TEXT("No teleport at route boundary"),Distance<70.f);
    }
    TestTrue(TEXT("At least seventy degrees LEFT around the crater before the finale"),Angle(22.f)-Angle(160.f)>1.22f);
    for(float T : {-4.f,0.f,8.f,20.f,40.f,90.f})
    {
        const FVector2D Travel=Point(T+.05f,160000.f)-Point(T-.05f,160000.f);
        TestTrue(TEXT("Inlet and lip retain forward speed"),Travel.Size()/.1f>1000.f);
        const FVector2D Forward=Travel.GetSafeNormal(),Right(-Forward.Y,Forward.X);
        TestTrue(TEXT("Starboard helm remains right after left-orbit change"),
            FVector2D::DotProduct(Point(T,160000.f,1000.f)-Point(T,160000.f),Right)>0.f);
        TestTrue(TEXT("Heading remains continuous through the lip"),
            FVector2D::DotProduct(Tangent(T-.05f,160000.f),Tangent(T+.05f,160000.f))>.999f);
    }
    TestTrue(TEXT("Route descends inside the overflow face before the final orbit"),RadialFraction(100.f)<.77f);
    for (float T : {25.f,40.f,90.f,158.f})
    {
        const float Wrapped=FMath::UnwindRadians(Angle(T));
        TestTrue(TEXT("Banks follow the same route across the atan seam"),FMath::Abs(SecondsAtAngle(Wrapped)-T)<.005f);
    }
    float Final30=0.f,Final120=0.f;
    for (int32 FPS : {30,120})
    {
        for (float Input : {-1.f,1.f})
        {
            float Helm=0.f,Offset=0.f,Velocity=0.f,Peak=0.f;
            for (int32 I=0;I<FPS*120;++I)
            {
                AdvanceSteering(Input,1.f/FPS,1.f,Helm,Offset,Velocity);
                Peak=FMath::Max(Peak,FMath::Abs(Offset));
            }
            TestTrue(TEXT("Full held rudder is contained by the visible banks"),Peak<10500.f);
            TestTrue(TEXT("Steering produces useful lateral travel"),Offset*Input>5000.f);
            if(Input>0) { if(FPS==30) {Final30=Offset;} else {Final120=Offset;} }
            for(int32 I=0;I<FPS*20;++I) { AdvanceSteering(0.f,1.f/FPS,1.f,Helm,Offset,Velocity); }
            TestTrue(TEXT("Released helm settles without endless sideways skating"),FMath::Abs(Velocity)<5.f);
        }
    }
    TestTrue(TEXT("Thirty and 120 fps reach the same steering balance"),FMath::Abs(Final30-Final120)<50.f);
    return true;
}
#endif
