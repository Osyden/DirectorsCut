#pragma once
#include <algorithm>
namespace DCAuthoredLimbPolicy
{
    /** Pin one real authored bone exactly to the unchanged cannon joint. */
    inline float CurveU(float SourceU,float JointSourceU)
    {
        SourceU=std::clamp(SourceU,0.f,1.f);
        JointSourceU=std::clamp(JointSourceU,.1f,.9f);
        return SourceU<=JointSourceU?SourceU/JointSourceU*.72f:
            .72f+(SourceU-JointSourceU)/(1.f-JointSourceU)*.28f;
    }
}
