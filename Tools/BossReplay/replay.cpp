#include "DCBossTimeline.h"
#include "DCBossDamagePolicy.h"
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
        Check(Score(8,2,6,0)==1000,"Perfect score is capped at 1000");
        Check(Score(20,200,200,10)==800,"Add farming capped, cumulative damage penalized");
        Check(EffectiveDamage(100,2,true)==2,"Overkill never scores");
        Check(EffectiveDamage(-1,8,true)==0,"Negative damage rejected");
        Check(EffectiveDamage(1,8,false)==0,"Closed targets never score");
        for(const auto& E:Events)
        {
            // Two seconds to recognize/aim, up to one reload already in progress,
            // three seconds between shots, and the cannon's maximum three-second flight.
            const float LastImpact=2.f+3.f+(Health(E.Kind)-1.f)*3.f+3.f;
            Check(LastImpact<Tell(E.Kind),"A solo individual cannon can interrupt within the warning");
        }
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
                    DCMaelstromRoute::AdvanceSteering(Age>=2.f?Direction*DCMaelstromRoute::SteeringAuthority(Authority):0.f,DT,1.f,H,O,V);
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
        // Carried turn response: quick enough to dodge, no position teleport,
        // no invisible spring pulling the released ship back toward mid-lane.
        float ReversalWorst=0.f, CoastWorst=0.f;
        for(int FPS:{15,30,60,120})
        {
            float H=0.f,O=0.f,V=0.f;
            for(int I=0;I<FPS*4;++I) DCMaelstromRoute::AdvanceSteering(1,1.f/FPS,1,H,O,V);
            const float InitialO=O;
            float ReverseAt=0;
            for(int I=0;I<FPS*3;++I)
            {
                const float Before=O;
                DCMaelstromRoute::AdvanceSteering(-1,1.f/FPS,1,H,O,V);
                Check(std::abs(O-Before)<=1250.f/FPS+.01f,"Helm cannot instantly strafe");
                if(V<0 && ReverseAt==0) ReverseAt=(I+1.f)/FPS;
            }
            ReversalWorst=std::max(ReversalWorst,ReverseAt);
            Check(ReverseAt>0 && ReverseAt<1.25f,"Full counter-rudder reverses carried turn within1.25s");
            Check(O<InitialO,"Opposite helm makes actual inward progress");
            const float ReleaseO=O;
            for(int I=0;I<FPS*8;++I) DCMaelstromRoute::AdvanceSteering(0,1.f/FPS,1,H,O,V);
            CoastWorst=std::max(CoastWorst,std::abs(O-ReleaseO));
            Check(std::abs(V)<1.f && std::abs(O-ReleaseO)<1200.f,"Neutral settles in selected lane with bounded coasting");
            for(int I=0;I<FPS*20;++I) DCMaelstromRoute::AdvanceSteering(-1,1.f/FPS,1,H,O,V);
            Check(O>=-11000 && O<-9000,"Inner foam limit and helm containment coincide");
            Check(DCMaelstromRoute::InnerBoundaryRadius(160000)+2500.f==160000*.58f-11000.f,"Foam no-go ring touches full inward hull envelope");
            // Full outward input through actual moving inlet guard, not a static limit surrogate.
            H=O=V=0;
            for(int I=0;I<FPS*90;++I)
            {
                const float T=I/float(FPS),Limit=DCMaelstromRoute::InletOutwardLimit(T,160000);
                DCMaelstromRoute::AdvanceSteering(1,1.f/FPS,1,H,O,V,Limit);
                Check(O<=Limit+.01f && O>=-11000,"Steering preserves inlet side-shelf protection");
            }
        }
        // Actual committed obstacle sweep, including two/three-second reaction,
        // torn rigging, initial momentum and both sides of the current lane.
        int DodgeCases=0;
        for(int FPS:{15,30,60,120})
        for(float Start:{50.f,106.f})
        for(float InitialOffset:{-9500.f,-6000.f,0.f,6000.f,9500.f})
        for(float InitialV:{-900.f,0.f,900.f})
        for(float Reaction:{2.f,3.f})
        for(float Authority:{1.f,.35f})
        {
            if(InitialOffset>DCMaelstromRoute::InletOutwardLimit(Start,160000)) continue;
            const float Side=Start<100 ? 1.f:-1.f;
            const auto T=DCMaelstromRoute::Tangent(Start+ObstacleLead,160000);
            const FVector2D Across(-T.Y,T.X);
            const auto Anchor=DCMaelstromRoute::Point(Start+ObstacleLead,160000,std::clamp(InitialOffset,-6500.f,6500.f));
            float H=0,O=InitialOffset,V=InitialV;
            auto Previous=WorldPoint(Start,O,V,Anchor);
            bool Hit=false;
            for(int Frame=1;Frame<=FPS*(ObstacleLead+3);++Frame)
            {
                const float Age=Frame/float(FPS),Now=Start+Age;
                DCMaelstromRoute::AdvanceSteering(Age>=Reaction?-Side*DCMaelstromRoute::SteeringAuthority(Authority):0,1.f/FPS,1,H,O,V,
                    DCMaelstromRoute::InletOutwardLimit(Now,160000));
                const auto Current=WorldPoint(Now,O,V,Anchor);
                const auto Bias=WorldPoint(Now,O,V,Anchor+Across*(Side*ObstacleSideBias))-Current;
                if(FPS==60 && Authority==.35f && Reaction==2.f && InitialV==Side*900.f
                    && InitialOffset==(Start<100?0.f:-9500.f) && (Frame==FPS*9 || Frame==FPS*12))
                    std::cout<<"Damaged dodge start="<<Start<<" age="<<Age<<" offset="<<O*.01f
                        <<"m speed="<<V*.01f<<"m/s obstacle=("<<(Current.X+Bias.X)*.01f<<","<<(Current.Y+Bias.Y)*.01f<<")m\n";
                if(Age>=ObstacleLead-3 && Age<=ObstacleLead+3)
                    Hit|=SweptHull(Previous.X+Bias.X,Previous.Y+Bias.Y,Current.X+Bias.X,Current.Y+Bias.Y,ObstacleHalfLength,ObstacleHalfWidth);
                Previous=Current;
            }
            if(Hit) std::cerr<<"Dodge fail start="<<Start<<" offset="<<InitialOffset<<" v="<<InitialV<<" reaction="<<Reaction<<" authority="<<Authority<<" fps="<<FPS<<"\n";
            Check(!Hit,"Signposted dodge clears committed obstacle with initial momentum and rigging damage");
            ++DodgeCases;
        }
        const float SlamBearing=std::atan2(7000.f,2200.f)*180.f/UE_PI;
        Check(SlamBearing-DCMaelstromRoute::MaximumSteeringYaw>std::atan(1.f/1.9f)*180.f/UE_PI,"Maximum heading retains fixed slam joint cannon arc");
        std::cout<<"Helm metrics: reversal="<<ReversalWorst<<"s coast="<<CoastWorst*.01f<<"m committed dodges="<<DodgeCases<<"\n";
        // Use the production inlet height and the same smoothed physical deck
        // pitch to detect first bow contact with the lake, including held helm.
        float FirstNeutralLanding=0.f;
        for(int FPS:{15,30,60,120})
        for(float Input:{-1.f,0.f,1.f})
        {
            using namespace DCMaelstromRoute;
            float H=0,O=0,V=0,Pitch=0,BowAt=-1,CentreAt=-1;
            for(int Frame=0;Frame<=FPS*50;++Frame)
            {
                const float T=-8.f+Frame/float(FPS),DT=1.f/FPS;
                AdvanceSteering(Input,DT,FMath::SmoothStep(-4,12,T),H,O,V,InletOutwardLimit(T,160000));
                const auto P=Point(T,160000,O),A=Point(T+.1f,160000,O),B=Point(T-.1f,160000,O);
                const float Height=MeanSurfaceHeight(P,160000);
                const float Grade=std::atan2(MeanSurfaceHeight(A,160000)-MeanSurfaceHeight(B,160000),std::max(1.f,float((A-B).Size())))*180.f/UE_PI;
                Pitch=FMath::FInterpTo(Pitch,DeckPitch(Grade,T),DT,5);
                const float Bow=BowLakeClearance(Height,Pitch,2369.f);
                if(BowAt<0 && Bow<=25.f) BowAt=T;
                if(CentreAt<0 && Height-LakeHeight<=25.f) CentreAt=T;
                if(BowAt<0) Check(Bow>25.f,"Landing cannot trigger before physical bow reaches lake");
            }
            Check(BowAt>0 && BowAt<CentreAt,"Pitched bow reaches lake before amidships for every inlet steering path");
            Check(BowLakeClearance(LakeHeight,0,2369)==0,"Seek on flat lake initializes already-landed state");
            if(Input==0)
            {
                if(FirstNeutralLanding==0) FirstNeutralLanding=BowAt;
                Check(std::abs(BowAt-FirstNeutralLanding)<.15f,"Landing contact is frame-rate stable");
                std::cout<<"Landing fps="<<FPS<<" bow="<<BowAt<<"s centre="<<CentreAt<<"s earlier="<<CentreAt-BowAt<<"s\n";
            }
        }
        // Passive upper bound: two obstacles, two slams, two fireballs and two full-duration fires.
        const float Passive=2*3.f+2*DCBossDamagePolicy::SlamDamage+2*2.f+2*40.f*.08f;
        Check(std::abs(Passive-36.4f)<.01f && Passive<40.f,"Two heavier slams preserve a survivable 36.4 hull damage budget");
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
