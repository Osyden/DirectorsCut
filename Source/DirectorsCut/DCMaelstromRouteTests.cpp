#include "DCMaelstromRoute.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCMaelstromRouteTest,"DirectorsCut.Maelstrom.RouteAndSteering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCMaelstromRouteTest::RunTest(const FString& Parameters)
{
    using namespace DCMaelstromRoute;
    TestTrue(TEXT("Caldera drift uses a small heading amplitude"),MaximumSteeringYaw==10.f);
    TestTrue(TEXT("Heading cannot jump on a full counter-rudder"),AdvanceHeading(10.f,-10.f,.1f)>=9.4f);
    TestTrue(TEXT("Physical deck follows a substantial downhill grade"),DeckPitch(-29.f,12.f)<-28.f);
    TestTrue(TEXT("Deck pitch does not precede the visible inlet"),DeckPitch(-29.f,-8.f)==0.f);
    TestTrue(TEXT("Deck settles level on the lake"),DeckPitch(0.f,40.f)==0.f);
    TestTrue(TEXT("Extreme grade cannot overturn the ship"),DeckPitch(-80.f,12.f)==-34.f);
    TestTrue(TEXT("Landing compresses before rebounding"),LandingHeave(.15f)<-50.f && LandingHeave(.5f)>0.f);
    TestTrue(TEXT("Landing has no residual heave after three seconds"),LandingHeave(-1.f)==0.f && LandingHeave(3.f)==0.f && LandingHeave(10.f)==0.f);
    TestTrue(TEXT("Pitched bow touches lake before the centre"),BowLakeClearance(LakeHeight+500.f,-15.f,2369.f)<0.f);
    TestTrue(TEXT("Level bow above the lake has not landed"),BowLakeClearance(LakeHeight+500.f,0.f,2369.f)>25.f);
    TestTrue(TEXT("Inlet starts with full steering corridor"),InletOutwardLimit(16.f,160000.f)==11000.f);
    TestTrue(TEXT("Outward lane closes before the unsupported side shelf"),InletOutwardLimit(30.f,160000.f)==0.f && InletOutwardLimit(50.f,160000.f)==0.f);
    TestTrue(TEXT("Outward steering restores after actual inlet clearance"),InletOutwardLimit(75.f,160000.f)==11000.f);
    // Continuous route through the drop, acceleration and phase boundaries.
    for (float T : {0.f,8.f,12.f,22.f,34.f,40.f,85.f,158.f,160.f,165.f,180.f})
    {
        const float Distance = (Point(T+.01f,160000.f)-Point(T-.01f,160000.f)).Size();
        TestTrue(TEXT("No teleport at route boundary"),Distance<100.f);
    }
    TestTrue(TEXT("At least sixty-five degrees LEFT around the lake before the finale"),Angle(22.f)-Angle(160.f)>1.13f);
    for(float T : {-4.f,0.f,8.f,20.f,40.f,90.f})
    {
        const FVector2D Travel=Point(T+.05f,160000.f)-Point(T-.05f,160000.f);
        TestTrue(TEXT("Inlet and lip retain forward speed"),Travel.Size()/.1f>500.f);
        const FVector2D Forward=Travel.GetSafeNormal(),Right(-Forward.Y,Forward.X);
        TestTrue(TEXT("Starboard helm remains right after left-orbit change"),
            FVector2D::DotProduct(Point(T,160000.f,1000.f)-Point(T,160000.f),Right)>0.f);
        TestTrue(TEXT("Heading remains continuous through the lip"),
            FVector2D::DotProduct(Tangent(T-.05f,160000.f),Tangent(T+.05f,160000.f))>.999f);
    }
    TestTrue(TEXT("Entry reaches the lake radius"),FMath::Abs(RadialFraction(32.f)-.58f)<.00001f);
    for (float T : {25.f,40.f,90.f,158.f,175.f,180.f})
    {
        const float Wrapped=FMath::UnwindRadians(Angle(T));
        TestTrue(TEXT("Banks follow the same route across the atan seam"),FMath::Abs(SecondsAtAngle(Wrapped)-T)<.005f);
    }
    TestTrue(TEXT("Finale remains on the level lake orbit"),RadialFraction(180.f)==RadialFraction(32.f));
    TestTrue(TEXT("Constant lake orbit speed is about eight metres per second"),
        FMath::Abs((Point(90.05f,160000.f)-Point(89.95f,160000.f)).Size()/.1f-835.2f)<3.f);
    for(float T : {24.f,32.f})
    {
        const FVector2D Before=(Point(T,160000.f)-Point(T-.1f,160000.f))/.1f;
        const FVector2D After=(Point(T+.1f,160000.f)-Point(T,160000.f))/.1f;
        TestTrue(TEXT("Entry and lake angular velocity join continuously"),(After-Before).Size()<40.f);
    }
    for(float T : {20.f,65.f,100.f,160.f})
    {
        TestTrue(TEXT("Neutral hull heading follows the route"),FMath::Abs(SteeringYaw(T,160000.f,0.f,0.f))<.001f);
        const float Port=SteeringYaw(T,160000.f,0.f,-1000.f);
        const float Starboard=SteeringYaw(T,160000.f,0.f,1000.f);
        TestTrue(TEXT("Hull headings turn with opposing steering trajectories"),Port*Starboard<0.f);
        TestTrue(TEXT("Steered visual heading stays comfortable"),FMath::Abs(Port)<=MaximumSteeringYaw && FMath::Abs(Starboard)<=MaximumSteeringYaw);
        TestTrue(TEXT("Lateral drift carries a small visible bow response"),FMath::Abs(Port)>2.f && FMath::Abs(Starboard)>2.f);
    }
    for(float T=0.f;T<180.f;T+=1.f)
    {
        TestTrue(TEXT("Entry approaches inward without a post-entry spiral"),RadialFraction(T+1.f)<=RadialFraction(T));
        const float W=WidthScale(T);
        TestTrue(TEXT("No artificial raised banks on the lake"),
            BankedHeight(14500.f*W,T)==0.f && BankedHeight(-12500.f*W,T)==0.f);
        TestTrue(TEXT("Route centre stays on the defined surface"),FMath::Abs(BankedHeight(0.f,T))<.01f);
        TestTrue(TEXT("Steering corridor stays constant on lake"),FMath::Abs(WidthScale(T+1.f)-W)<.02f);
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
            TestTrue(TEXT("Full held rudder remains inside the steering corridor"),Peak<=SteeringLimit);
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
