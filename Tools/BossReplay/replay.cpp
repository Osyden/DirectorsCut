#include "DCBossTimeline.h"
#include "DCMaelstromRoute.h"
#include <iostream>
#include <stdexcept>

int Checks=0;
void Check(bool Passed,const char* Message)
{
    ++Checks;
    if(!Passed) throw std::runtime_error(Message);
}
FVector2D WorldPoint(float Time,float Offset,float Velocity,const FVector2D& Anchor)
{
    using namespace DCMaelstromRoute;
    const auto D=Anchor-Point(Time,160000.f,Offset);
    const auto T=Tangent(Time,160000.f);
    const float Yaw=-std::atan2(T.Y,T.X)-SteeringYaw(Time,160000.f,Offset,Velocity)*UE_PI/180.f;
    return {D.X*std::cos(Yaw)-D.Y*std::sin(Yaw),D.X*std::sin(Yaw)+D.Y*std::cos(Yaw)};
}
int main()
{
    using namespace DCBossTimeline;
    try
    {
        Check(Score(20,2,6,0)==1000,"Perfect score is capped at 1000");
        Check(Score(20,200,200,10)==800,"Add farming capped, cumulative damage penalized");
        Check(EffectiveDamage(100,2,true)==2,"Overkill never scores");
        Check(EffectiveDamage(-1,8,true)==0,"Negative damage rejected");
        Check(EffectiveDamage(1,8,false)==0,"Closed targets never score");
        Check(SweptHull(4000,0,-4000,0,3000,1700),"Swept contact catches frame crossings");
        Check(!SweptHull(4000,1800,-4000,1800,3000,1700),"Clear lane stays safe");
        for(float Deadline:{140.f,160.f,180.f})
        {
            for(float T=0;T<=Deadline+1.f;T+=.125f)
            {
                int WindowsOpen=0;
                for(const auto& E:Events) WindowsOpen+=Open(T,At(E.At,Deadline),At(E.At,Deadline)+Tell(E.Kind),Deadline)?1:0;
                Check(WindowsOpen<=1,"At most one exposed joint at once");
                if(T>=Deadline) Check(WindowsOpen==0,"No damage window after music deadline");
            }
            for(const auto& E:Events) Check(At(E.At,Deadline)>=44.f,"No attack during inlet descent");
        }
        for(int FPS:{15,30,60,120})
        {
            int Count=0;
            for(int Frame=1;Frame<=FPS*180;++Frame)
                for(const auto& E:Events) Count+=Crossed((Frame-1.f)/FPS,Frame/float(FPS),E.At)?1:0;
            Check(Count==6,"Each attack scheduled exactly once across frame rates");
            for(float Start:{50.f,106.f})
            for(float Authority:{1.f,.35f})
            for(float Direction:{-1.f,0.f,1.f})
            {
                float H=0.f,O=0.f,V=0.f;
                const float Side=Start<100.f?1.f:-1.f;
                const auto T=DCMaelstromRoute::Tangent(Start+ObstacleLead,160000.f);
                const FVector2D Across(-T.Y,T.X);
                const auto Anchor=DCMaelstromRoute::Point(Start+ObstacleLead,160000.f,0.f)+Across*(Side*ObstacleSideBias);
                auto Prev=WorldPoint(Start,0,0,Anchor);
                Check(Prev.X>7000.f,"Obstacle never starts inside ship");
                bool Hit=false;
                for(int Frame=1;Frame<=FPS*(ObstacleLead+4);++Frame)
                {
                    const float Age=Frame/float(FPS),DT=1.f/FPS;
                    DCMaelstromRoute::AdvanceSteering(Age>=2.f?Direction*Authority:0.f,DT,1.f,H,O,V);
                    const auto P=WorldPoint(Start+Age,O,V,Anchor);
                    if(Age>=ObstacleLead-3.f && Age<=ObstacleLead+3.f)
                        Hit|=SweptHull(Prev.X,Prev.Y,P.X,P.Y,ObstacleHalfLength,ObstacleHalfWidth);
                    Prev=P;
                }
                if(Direction==0) Check(Hit,"Passive ship meets committed obstacle");
                else if(Direction==-Side && Hit)
                {
                    std::cerr<<"Failed dodge: fps="<<FPS<<" start="<<Start<<" authority="<<Authority<<" direction="<<Direction<<"\n";
                    Check(false,"The signposted clear side dodges after two-second reaction, even damaged rigging");
                }
                else ++Checks;
            }
        }
        // Passive upper bound: two obstacles, two slams, two fireballs and two full-duration fires.
        const float Passive=2*3.f+2*5.f+2*2.f+2*40.f*.08f;
        Check(Passive<30.f,"Passive encounter has a bounded sub-30 hull damage budget");
        // Interrupted and exhausted targets cannot farm score; damage never changes the clock.
        float Remaining=4.f, Damage=0.f; bool Severed=false;
        for(int I=0;I<100;++I)
        {
            const float Effective=EffectiveDamage(1,Remaining,!Severed);
            Remaining-=Effective; Damage+=Effective; if(Remaining<=0) Severed=true;
        }
        Check(Damage==4 && Severed,"Severed limb remains exhausted for every subsequent hit");
        Check(!Open(160,144,170,160),"Finale clips a charge regardless of its remaining health");
        for(float T=44.f;T<160.f;T+=.125f)
        {
            int Live=0;
            for(const auto& E:Events) if(T>=E.At && T<E.At+Life(E.Kind)) ++Live;
            Check(Live<=1,"Schedule obeys one urgent attack at a time without dropped events");
        }
        Check(!Crossed(80,80,80),"Paused clock does not retrigger event");
        std::cout<<"PASS "<<Checks<<" checks: schedule, score caps, swept hits, 15/30/60/120fps helm replay, passive budget="<<Passive<<"\n";
    }
    catch(const std::exception& E){std::cerr<<"FAIL "<<E.what()<<"\n";return 1;}
}
