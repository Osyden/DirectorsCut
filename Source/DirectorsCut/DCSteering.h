#pragma once
#include <algorithm>
#include <cmath>

// Stage-space helm dynamics shared by live steering and collision prediction.
// Distances are centimetres; velocity/acceleration use seconds.
namespace DCSteering
{
struct State { float Position = 0, Velocity = 0, Acceleration = 0; };
inline float Smooth(float A, float B, float X)
{
    const float U = std::clamp((X-A)/(B-A),0.f,1.f);
    return U*U*(3.f-2.f*U);
}
inline void Advance(State& S, float Helm, float Speed, float Limit, float Drift, float BankPull, float Dt)
{
    Speed = std::max(Speed,1.f); Limit = std::max(Limit,1.f);
    // Fixed upper step size makes both a slow frame and the navigation forecast
    // follow the same carried turn, without frame-rate-dependent interpolation.
    const int Steps = std::max(1,int(std::ceil(std::max(Dt,0.f)*120.f)));
    const float H = std::max(Dt,0.f)/Steps;
    for (int I=0; I<Steps; ++I)
    {
        const float MaxAcceleration = Speed*.95f;
        const float TotalVelocity = S.Velocity+Drift;
        const float BrakeDistance = TotalVelocity*std::abs(TotalVelocity)/(2.f*MaxAcceleration);
        const float Predicted = S.Position + TotalVelocity*.45f + BrakeDistance;
        const float Side = Predicted < 0.f ? -1.f : 1.f;
        const float Recovery = Smooth(.67f,.94f,std::abs(Predicted)/Limit);
        const float Requested = std::clamp(Helm,-1.f,1.f)*Speed;
        // At the bank steer gently inward, even if the wheel is still held out.
        // Braking-distance anticipation avoids an abrupt clamp at the boundary.
        const float Target = Requested*(1.f-Recovery) - Side*Speed*.8f*Recovery;
        const float Response = std::abs(Helm)<.05f && Recovery<.01f ? 2.1f : 1.25f;
        const float Wanted = std::clamp((Target-S.Velocity)/Response-BankPull,-MaxAcceleration,MaxAcceleration);
        const float JerkStep = Speed*2.2f*H;
        S.Acceleration += std::clamp(Wanted-S.Acceleration,-JerkStep,JerkStep);
        S.Velocity += S.Acceleration*H;
        S.Position += (S.Velocity+Drift)*H;
        // Emergency containment for teleports/large collision corrections only.
        if (std::abs(S.Position)>Limit)
        {
            S.Position=std::clamp(S.Position,-Limit,Limit);
            if (S.Position*S.Velocity>0.f) { S.Velocity=0.f; }
            S.Acceleration=0.f;
        }
    }
}
inline float VisualTurn(const State& S, float Drift, float Speed)
{
    return std::clamp((S.Velocity+Drift+S.Acceleration*.3f)/std::max(Speed,1.f),-1.35f,1.35f);
}
}
