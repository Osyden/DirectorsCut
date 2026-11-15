#include "DCFogCreatureMotion.h"
#include <cstdio>
#include <cstdlib>

int main()
{
    using namespace DCFogCreatureMotion;
    int Checks=0;
    auto Check=[&](bool Good,const char* Description) { ++Checks; if(!Good) { std::fprintf(stderr,"FAIL %s\n",Description); std::exit(1); } };
    Check(GroundSpeed(1500,10000,9850,.1f)==0,"Stationary bank creature is not running with scenery scroll");
    Check(GroundSpeed(1500,3000,3000,.1f)==1500,"Creature holding ship-relative position still travels through world");
    Check(GroundSpeed(1500,5000,4962.5,.1f)==1125,"Approach slowdown becomes correct world travel");
    Check(GroundSpeed(1500,5000,5000,0)==0,"Paused frame stays finite");
    Check(Direction(0,90)==EDirection::Idle,"Stationary creature idles");
    Check(Direction(1500,110)==EDirection::Left,"Port creature strafes left while facing ship");
    Check(Direction(1500,-110)==EDirection::Right,"Starboard creature strafes right while facing ship");
    Check(Direction(1500,0)==EDirection::Forward,"Forward facing uses forward gait");
    Check(Direction(1500,180)==EDirection::Backward,"Backward facing uses backward gait");
    Check(std::abs(GaitCyclesPerSecond(1500,10)-.5f)<.001f,"Giant gait matches world travel and scale");
    constexpr float Rip=1.1333333f,Toss=.8666667f;
    Check(Pose(1,1.f,Rip,Toss).Action==EAction::Rip,"Rip plays through its authored motion");
    Check(Pose(1,1.2f,Rip,Toss).Action==EAction::Hold,"Ended rip transitions to live carrying gait");
    Check(Pose(1,2.54f,Rip,Toss).Action==EAction::Hold,"Charge remains held until toss preroll");
    Check(Pose(1,2.56f,Rip,Toss).Action==EAction::Toss,"Toss starts before authoritative release");
    Check(std::abs(Pose(1,3.f,Rip,Toss).Seconds-Pose(2,0,Rip,Toss).Seconds)<.00001f,"Release pose continuous across combat stage boundary");
    Check(Pose(2,.4f,Rip,Toss).Action==EAction::Toss,"Throw follows through after release");
    Check(Pose(2,.42f,Rip,Toss).Action==EAction::Travel,"Ended toss resumes travel before projectile finishes");
    Check(Pose(4,1.5f,Rip,Toss).Action==EAction::Death,"Defeat remains authoritative");
    for(float Dt : {.1f,1.f/30,1.f/60,1.f/120})
    {
        for(float Time=0;Time<11.f;Time+=Dt)
            Check(Pose(3,Time,Rip,Toss).Action==EAction::Travel,"Entire cooldown remains locomotion, at all frame rates");
        for(float Time=0;Time<3.f;Time+=Dt)
        {
            const FPose P=Pose(1,Time,Rip,Toss);
            Check(P.Action==EAction::Hold || (P.Action==EAction::Rip && P.Seconds<Rip) || (P.Action==EAction::Toss && P.Seconds<Toss),"Charge never freezes an exhausted one-shot");
        }
    }
    std::printf("PASS %d creature motion and attack-continuity checks\n",Checks);
}
