#include "DCFogRouteLayout.h"
#include <cstdio>
#include <cstdlib>

int main()
{
    using namespace DCFogRouteLayout;
    int Checks=0;
    auto Check=[&](bool Good,const char* Description) { ++Checks; if(!Good) { std::fprintf(stderr,"FAIL %s\n",Description); std::exit(1); } };
    const float Clear=ClearEdge(9000,2200,500), Scenery=SceneryEdge(Clear);
    // Full hard-over range, every hull yaw; no generic steering reduction.
    for(float Lane=-9000;Lane<=9000;Lane+=100)
    for(float Yaw=-180;Yaw<=180;Yaw+=1)
    for(float X : {-2200.f,2200.f})
    for(float Y : {-500.f,500.f})
    {
        const float RotatedY=X*std::sin(Yaw*.01745329252f)+Y*std::cos(Yaw*.01745329252f);
        Check(std::abs(Lane+RotatedY)<=Clear-249.9f,"Hull clears visible bank across full helm range and every yaw");
    }
    // The reported long colonnade and rotated complex, plus arbitrary rotations.
    for(float HalfX : {1000.f,31500.f,55000.f})
    for(float HalfY : {500.f,23100.f})
    for(float Yaw=-180;Yaw<=180;Yaw+=3)
    {
        const float R=Yaw*.01745329252f;
        const float Extent=HalfX*std::abs(std::sin(R))+HalfY*std::abs(std::cos(R));
        for(float Side : {-1.f,1.f})
        {
            const float Placed=FitSide(Side*24000,-Extent,Extent,Scenery);
            Check(std::abs(Placed)-Extent>=Scenery-.1f,"Rotated architectural plinth stays beyond bank and firing lane");
        }
    }
    Check(BankTop>1100+250,"Fixed bank surface clears peak city trough plus wave allowance");
    Check(BankBottom<-250,"Foundation extends below wave trough");
    int Creatures=0;
    for(int I=0;I<4;++I)
    {
        Creatures+=GroupSize(I);
        if(I) { Check(GroupTimes[I]-GroupTimes[I-1]>=34,"Only modest group overlap"); }
        Check(GroupTimes[I]+34<240,"All pursuit ends before tunnel");
        for(int J=0;J<GroupSize(I);++J)
        {
            // Installed imported bounds, including the off-centre mesh origin.
            const float Radius=(std::sqrt(137.114f*137.114f+92.421f*92.421f)+13.227f)*(10.f+J*.8f);
            Check(WalkerOffset(J,Radius)+Radius<=BankWidth*.5f-99.9f,"Whole rotated walker reference footprint fits supported bank width");
        }
    }
    Check(Creatures==9,"Nine creatures total, previously seven");
    Check(sizeof(StatueTimes)/sizeof(float)==4,"Four attack statues total, previously three");
    const float Y=BankCentre(Clear)+1300;
    Check(std::sqrt(7600.f*7600+Y*Y+4000.f*4000)<21000,"Farthest grouped target remains inside normal centre-lane cannon range");
    std::printf("PASS %d fog route clearance, support and encounter checks\n",Checks);
}
