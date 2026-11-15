#include "DCFogRouteLayout.h"
#include "DCEarlyEncounterPolicy.h"
#include <vector>
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
    Check(Creatures==12,"Twelve creatures total, three more within existing waves");
    Check(sizeof(StatueTimes)/sizeof(float)==4,"Four attack statues total, previously three");
    const float Y=BankCentre(Clear)+1300;
    Check(std::sqrt(12200.f*12200+Y*Y+4000.f*4000)<21000,"Farthest grouped target remains inside normal centre-lane cannon range");
    using namespace DCEarlyEncounterPolicy;
    Check(StatueHealth==4 && StatueHealth>3,"Statue survives three hits and falls to the fourth");
    const float SoloLastImpact=2.f+(StatueHealth-1.f)*3.f+3.f;
    Check(SoloLastImpact<StatueFirstExposure+StatueCharge,"One cannon has recognition, reload and flight time before first statue beam");
    // Actual installed cannon arc/range and measured ship/bank dimensions.
    // Held-helm native runs settle at7219cm. The opposite bank can correctly
    // be outside210m range; such targets cannot attack or receive a HUD marker.
    const float BankY=BankCentre(ClearEdge(9000,2368.6f,567.f));
    float ShortestPass=1000;
    for(float Lane:{-7219.f,0.f,7219.f}) for(float Side:{-1.f,1.f})
    {
        const float Across=std::abs(Side*BankY-Lane-Side*500.f),Height=4000.f;
        if(Across*Across+Height*Height>=21000.f*21000.f) continue;
        const float HalfX=std::min(1.9f*Across,std::sqrt(21000.f*21000.f-Across*Across-Height*Height));
        const float Pass=(HalfX+std::min(HalfX,16000.f))/1500.f;
        ShortestPass=std::min(ShortestPass,Pass);
        Check(Pass>StatueFirstExposure+StatueCharge+.7f,"Ignored statue can release and land beam during actual cannon pass");
        Check(Pass>SoloLastImpact,"Solo four-hit preemption fits the visible broadside pass");
    }
    for(int Bits=0;Bits<16;++Bits) for(int Stage=0;Stage<=4;++Stage)
    {
        const bool Statue=Bits&1,Target=Bits&2,Hidden=Bits&4,Reachable=Bits&8;
        Check(ShowStatueMarker(Statue,Target,Hidden,Reachable,Stage)==(Statue&&Target&&!Hidden&&Reachable&&Stage!=4),
            "Marker never leaks concealed, dead, unreachable or non-statue targets");
        Check(ShowStatueAwareness(Statue,Target,Hidden,Stage)==(Statue&&Target&&!Hidden&&Stage!=4),
            "Visible living statue remains identified from helm and approach before cannon admission");
    }
    Check(ShowStatueAwareness(true,true,false,0) && !ShowStatueMarker(true,true,false,false,0),
        "Approaching statue has an eye marker without granting a blocked cannon/review shot");
    Check(!CanBeginCharge(0,60.f,StatueFirstExposure,false,0.f),
        "Awareness never allows an unreachable statue to start charging");
    // Replay all new waves plus statues, repeated attacks and cooldowns. The
    // same production charge gate must leave only one urgent aim demand.
    for(int FPS:{15,30,60,120})
    {
        struct Threat { float Spawn,Wait,Exposure=0,Clock=0; int Stage=0; bool Statue; };
        std::vector<Threat> Sim;
        for(int G=0;G<4;++G) for(int I=0;I<GroupSize(G);++I)
            Sim.push_back({GroupTimes[G],8.f+I*2.8f,0,0,0,false});
        for(float Spawn:StatueTimes) Sim.push_back({Spawn,StatueFirstExposure,0,0,0,true});
        float Spacing=0; int Charges=0;
        for(int Frame=0;Frame<FPS*150;++Frame)
        {
            const float T=90+Frame/float(FPS),DT=1.f/FPS;
            Spacing-=DT;
            for(auto& Enemy:Sim)
            {
                if(T<Enemy.Spawn || T>Enemy.Spawn+(Enemy.Statue?28.f:52.f)) { Enemy.Stage=4; continue; }
                if(Enemy.Stage==4) Enemy.Stage=0;
                Enemy.Clock+=DT; Enemy.Exposure+=DT;
                if(CanBeginCharge(Enemy.Stage,Enemy.Exposure,Enemy.Wait,true,Spacing))
                { Enemy.Stage=1; Enemy.Clock=0; Spacing=ChargeSpacing; ++Charges; }
                if(Enemy.Stage==1 && Enemy.Clock>=(Enemy.Statue?StatueCharge:CreatureCharge)) { Enemy.Stage=2; Enemy.Clock=0; }
                if(Enemy.Stage==2 && Enemy.Clock>=(Enemy.Statue?1.2f:2.4f)) { Enemy.Stage=3; Enemy.Clock=0; }
                if(Enemy.Stage==3 && Enemy.Clock>=11) { Enemy.Stage=0; Enemy.Exposure=0; Enemy.Wait=2.5f; }
            }
            int Charging=0; for(const auto& Enemy:Sim) Charging+=Enemy.Stage==1;
            Check(Charging<=1,"Extra enemies cannot create simultaneous charge demands");
        }
        Check(Charges>16,"All-wave replay exercises repeated attacks rather than a vacuous schedule");
    }
    std::printf("Statue timing: solo last impact %.2fs; first beam %.2fs; shortest reachable native held-helm pass %.2fs\n",
        SoloLastImpact,StatueFirstExposure+StatueCharge,ShortestPass);
    std::printf("PASS %d fog route clearance, support and encounter checks\n",Checks);
}

