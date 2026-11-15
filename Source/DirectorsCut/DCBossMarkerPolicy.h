#pragma once
#include <algorithm>
#include <cmath>

namespace DCBossMarkerPolicy
{
    struct FPlacement { float X,Y,LabelX,LabelY; bool AtEdge; };
    inline FPlacement Place(float X,float Y,bool Projected,float Width,float Height,float Scale)
    {
        const float PX=std::clamp(X,45.f*Scale,Width-45.f*Scale);
        float PY=std::clamp(Y,185.f*Scale,Height-210.f*Scale);
        // The right edge carries the three crew cards. Keep directional
        // brackets below them rather than drawing through a crew portrait.
        if(PX>Width-330.f*Scale && PY<475.f*Scale)
            PY=std::min(475.f*Scale,Height-210.f*Scale);
        return {PX,PY,std::clamp(PX+48.f*Scale,24.f*Scale,Width-630.f*Scale),
            std::clamp(PY-86.f*Scale,185.f*Scale,Height-220.f*Scale),
            !Projected || std::abs(PX-X)>1 || std::abs(PY-Y)>1};
    }
    inline float Urgency(float Remaining) { return std::clamp((3.f-Remaining)/3.f,0.f,1.f); }
    inline FPlacement PlaceBody(float X,float Y,bool Projected,float Width,float Height,float Scale,bool Urgent)
    {
        auto P=Place(X,Y,Projected,Width,Height,Scale);
        // Keep the quiet body edge cue below the urgent attack's edge cue.
        if(Urgent && P.AtEdge) P.Y=Height-260.f*Scale;
        return P;
    }
    inline bool ShowTarget(bool HasTarget,bool Open,bool Interrupted,bool Cancelled,bool Applied)
    { return HasTarget && Open && !Interrupted && !Cancelled && !Applied; }
}
