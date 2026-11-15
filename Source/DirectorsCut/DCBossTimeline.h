#pragma once
#include <algorithm>
#include <cmath>

/** Engine-independent policy for the three-response naval encounter. */
namespace DCBossTimeline
{
    constexpr float CombatStart=44.f, DefaultFinale=160.f;
    constexpr float ObstacleLead=12.f, SlamTell=9.f, MouthTell=10.f, FireFlight=3.f;
    constexpr float ObstacleHalfLength=3000.f, ObstacleHalfWidth=4100.f, ObstacleSideBias=2400.f;
    enum class EAttack { Obstacle, Slam, Mouth };
    struct FEvent { float At; EAttack Kind; int Side; int Limb; };
    inline constexpr FEvent Events[]={
        {50.f,EAttack::Obstacle,1,-1}, {70.f,EAttack::Slam,-1,0}, {88.f,EAttack::Mouth,-1,-1},
        {106.f,EAttack::Obstacle,-1,-1}, {126.f,EAttack::Slam,1,1}, {144.f,EAttack::Mouth,-1,-1}
    };
    inline float At(float Nominal,float Finale)
    { return CombatStart+(Nominal-CombatStart)*(Finale-CombatStart)/(DefaultFinale-CombatStart); }
    inline float Tell(EAttack K) { return K==EAttack::Obstacle?ObstacleLead:(K==EAttack::Slam?SlamTell:MouthTell); }
    inline float Life(EAttack K) { return Tell(K)+(K==EAttack::Mouth?FireFlight:4.f); }
    inline float Health(EAttack K) { return K==EAttack::Mouth?2.f:4.f; }
    inline bool Crossed(float Before,float Now,float At) { return Before<At && Now>=At; }
    inline bool Open(float Time,float Start,float Strike,float Finale)
    { return Time>=Start && Time<Strike && Time<Finale; }
    inline float EffectiveDamage(float Requested,float Remaining,bool Open)
    { return Open?std::clamp(Requested,0.f,std::max(0.f,Remaining)):0.f; }
    inline int Score(float Damage,int Severed,int Interrupts,float ShipDamage)
    { return int(std::lround(Damage*25.f))+std::clamp(Severed,0,2)*100+std::clamp(Interrupts,0,6)*50-int(std::lround(std::max(0.f,ShipDamage)*20.f)); }
    inline bool SweptHull(float AX,float AY,float BX,float BY,float HalfX,float HalfY)
    {
        float Enter=0.f,Leave=1.f;
        const float A[]={AX,AY},B[]={BX,BY},Extent[]={HalfX,HalfY};
        for(int I=0;I<2;++I)
        {
            const float D=B[I]-A[I];
            if(std::abs(D)<1.e-6f) { if(std::abs(A[I])>Extent[I]) return false; continue; }
            float U=(-Extent[I]-A[I])/D,V=(Extent[I]-A[I])/D;
            if(U>V) std::swap(U,V);
            Enter=std::max(Enter,U); Leave=std::min(Leave,V);
            if(Enter>Leave) return false;
        }
        return true;
    }
}
