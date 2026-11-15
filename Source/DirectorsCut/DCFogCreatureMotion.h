#pragma once

#include <algorithm>
#include <cmath>

// Pure animation policy; combat stages, positions and release deadlines remain authoritative.
namespace DCFogCreatureMotion
{
enum class EDirection { Idle, Forward, Backward, Left, Right };
enum class EAction { Travel, Rip, Hold, Toss, Death };
struct FPose { EAction Action; float Seconds; };

inline float GroundSpeed(float ShipSpeed, float PreviousX, float X, float Dt)
{
    return Dt > .00001f ? std::max(0.f, ShipSpeed + (X - PreviousX) / Dt) : 0.f;
}
inline EDirection Direction(float Speed, float FacingDegrees)
{
    if (Speed < 20.f) { return EDirection::Idle; }
    const float Angle = FacingDegrees * .01745329252f;
    const float Forward = std::cos(Angle), Right = -std::sin(Angle);
    if (std::abs(Forward) > std::abs(Right)) { return Forward > 0.f ? EDirection::Forward : EDirection::Backward; }
    return Right > 0.f ? EDirection::Right : EDirection::Left;
}
inline float GaitCyclesPerSecond(float Speed, float Scale)
{
    // Approximate 3m stride at asset scale. Shared cycle cadence preserves footfalls
    // when switching between 1s normal and .533s carrying clips.
    return std::clamp(Speed / (std::max(Scale, .01f) * 300.f), .25f, 1.5f);
}
inline float ReleasePoseTime(float TossLength) { return std::min(.45f, std::max(.01f, TossLength) * .6f); }
inline FPose Pose(int Stage, float Clock, float RipLength, float TossLength)
{
    const float Release = ReleasePoseTime(TossLength);
    if (Stage == 4) { return {EAction::Death, Clock}; }
    if (Stage == 1)
    {
        if (Clock >= 3.f - Release) { return {EAction::Toss, std::min(Release, Clock - (3.f - Release))}; }
        const float RipDuration = std::min(std::max(.01f, RipLength), 1.25f);
        if (Clock < RipDuration) { return {EAction::Rip, Clock * RipLength / RipDuration}; }
        return {EAction::Hold, 0.f};
    }
    if (Stage == 2 && Clock + Release < TossLength) { return {EAction::Toss, Clock + Release}; }
    return {EAction::Travel, 0.f};
}
}
