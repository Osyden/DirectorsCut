#pragma once
#include "CoreMinimal.h"

namespace DCTentacleCurve
{
    constexpr float JointU=.72f;
    inline FVector Point(float U,const FVector& Base,const FVector& Bend,const FVector& Joint,const FVector& Tip)
    {
        U=FMath::Clamp(U,0.f,1.f);
        if(U<JointU)
        {
            const float T=U/JointU,V=1.f-T;
            return Base*V*V*V+(Base+FVector(0,0,2800))*3.f*V*V*T+Bend*3.f*V*T*T+Joint*T*T*T;
        }
        const float T=(U-JointU)/(1.f-JointU),V=1.f-T;
        const FVector Control=Joint+(Joint-Bend)*((1.f-JointU)/JointU);
        return Joint*V*V*V+Control*3.f*V*V*T+FMath::Lerp(Tip,Joint,.35f)*3.f*V*T*T+Tip*T*T*T;
    }
    inline FVector Tangent(float U,const FVector& Base,const FVector& Bend,const FVector& Joint,const FVector& Tip)
    {
        return (Point(U+.001f,Base,Bend,Joint,Tip)-Point(U-.001f,Base,Bend,Joint,Tip)).GetSafeNormal();
    }
}
