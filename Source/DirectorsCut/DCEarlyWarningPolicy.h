#pragma once

namespace DCEarlyWarningPolicy
{
    enum class EKind { Approach, Beam, Defeat };
    inline bool ThreatRelevant(EKind Kind,bool InFog,bool Visible,bool Alive,bool CanShoot,bool Statue,int Stage,float StateAge=0.f)
    {
        if (!InFog || !Visible) { return false; }
        if (Kind==EKind::Defeat) { return !Alive && Stage==4 && StateAge<6.f; }
        if (!Alive || !CanShoot || Stage==4) { return false; }
        if (Kind==EKind::Beam) { return Statue && Stage==2; }
        // An "eyes waking ... before it fires" report cannot describe a beam
        // already on deck. Raiders remain a valid battery target after throwing.
        return !Statue || Stage==0 || Stage==1;
    }
    inline bool ProjectileRelevant(bool InFog,bool Visible,float Age,float Duration)
    { return InFog && Visible && Age>=0.f && Age<Duration; }
    // Admission still requires visibility, aim and event freshness above. Once
    // speaking, brief aim/occlusion changes do not make the sentence untrue.
    inline bool ThreatContinues(EKind Kind,bool InFog,bool Present,bool SameSide,bool Alive,bool Statue,int Stage)
    {
        if (!InFog) { return false; }
        if (Kind==EKind::Defeat) { return true; } // completed fact; its start window is not a sentence timeout
        if (!Present || !SameSide || !Alive || Stage==4) { return false; }
        if (Kind==EKind::Beam) { return Statue && Stage==2; }
        return !Statue || Stage!=2; // actual firing ends "before it fires"
    }
}
