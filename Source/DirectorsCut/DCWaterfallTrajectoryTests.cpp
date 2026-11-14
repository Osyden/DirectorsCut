#include "DCWaterfallTrajectory.h"
#include "DCCliffGenerator.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCWaterfallBallisticTest,"DirectorsCut.Maelstrom.DetachedWaterfallTrajectory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDCWaterfallBallisticTest::RunTest(const FString& Parameters)
{
    using namespace DCWaterfallTrajectory;
    const auto Surface=[](float V,float Across){return FVector(100000.f+15000.f*V,Across,10000.f*V);};
    const auto Water=[](const FVector2D& P,float& H){H=100.f; return P.X<100500.f;};
    FSegment S; S.StartV=.65f; S.EndV=.43f; S.InitialDown=700.f;
    TestTrue(TEXT("Measured sloping face admits a downward gravity gap"),Validate(S,Surface,Water,1250.f));
    TestTrue(TEXT("Launch meets original flow without a disconnected lip"),Sample(S,Surface,S.StartV,0.f).Equals(Surface(S.StartV,0.f),.01));
    TestTrue(TEXT("Landing rejoins original flow without an upward blend"),Sample(S,Surface,S.EndV,0.f).Equals(Surface(S.EndV,0.f),.01));
    bool Downward=true,AirSide=true,ConstantHorizontal=true;
    FVector Previous=Sample(S,Surface,S.StartV,0.f),PreviousVelocity=FVector::ZeroVector;
    const float Dt=S.Duration/64.f;
    float MaxGravityError=0.f;
    for(int32 Step=1;Step<=64;++Step)
    {
        const float Time=Step*Dt;
        const float Z=Surface(S.StartV,0.f).Z-S.InitialDown*Time-.5f*Gravity*Time*Time;
        const float V=ParameterAtHeight(Surface,Z,S.EndV,S.StartV);
        const FVector P=Sample(S,Surface,V,0.f),Velocity=(P-Previous)/Dt;
        Downward &= P.Z<Previous.Z;
        AirSide &= P.Size2D()<=Surface(V,0.f).Size2D()+.05;
        if(Step>1)
        {
            ConstantHorizontal &= FVector2D(Velocity-PreviousVelocity).Size()<1.f;
            MaxGravityError=FMath::Max(MaxGravityError,static_cast<float>(FMath::Abs((Velocity.Z-PreviousVelocity.Z)/Dt+Gravity)));
        }
        Previous=P; PreviousVelocity=Velocity;
    }
    TestTrue(TEXT("Gravity reach never curls upward"),Downward);
    TestTrue(TEXT("Gravity reach remains on air side of actual profile"),AirSide);
    TestTrue(TEXT("Horizontal velocity is constant along a ballistic track"),ConstantHorizontal);
    TestTrue(TEXT("Measured acceleration is terrestrial gravity in cm units"),MaxGravityError<8.f);
    const auto Blocked=[&](float V,float Across)
    {
        FVector P=Surface(V,Across);
        if(V>.47f && V<.61f) { P.X-=1600.f; }
        return P;
    };
    FSegment Obstructed=S;
    TestFalse(TEXT("A real cliff protrusion invalidates the arc rather than penetrating rock"),Validate(Obstructed,Blocked,Water,1250.f));
    const auto Flood=[](const FVector2D&,float& H){H=6000.f; return true;};
    FSegment Flooded=S;
    TestFalse(TEXT("First covered water cannot be bypassed by a detached arc"),Validate(Flooded,Surface,Flood,1250.f));
    const TArray<FSegment> Selected=Build(Surface,Water,1250.f,100.f);
    TestTrue(TEXT("Selection finds a bounded candidate on a supported sloping face"),Selected.Num()>0 && Selected.Num()<=3);
    const auto OriginalContact=DCCliffGenerator::FirstWaterContact([&](float T){return Surface(.84f*(1.f-T),0.f);},Water);
    const auto ChangedContact=DCCliffGenerator::FirstWaterContact([&](float T)
    {
        const float V=.84f*(1.f-T);
        if(const FSegment* Gap=Find(Selected,V)) { return Sample(*Gap,Surface,V,0.f); }
        return Surface(V,0.f);
    },Water);
    TestTrue(TEXT("Protected toe keeps first covered receiving-water contact unchanged"),OriginalContact.bFound && ChangedContact.bFound
        && OriginalContact.Position.Equals(ChangedContact.Position,.01));
    return true;
}
#endif
