#pragma once

// Seconds of one deliberate crew rag stroke. Contact happens once during the
// downstroke; recovery never applies more suppression, even after a long frame.
namespace DCCrewWorkPolicy
{
constexpr float Contact = .42f;
constexpr float Duration = 1.05f;
inline float SmoothRamp(float Begin, float End, float Age)
{
    const float T = Age <= Begin ? 0.f : Age >= End ? 1.f : (Age-Begin)/(End-Begin);
    return T*T*(3.f-2.f*T);
}
inline float PoseWeight(float Age)
{
    return SmoothRamp(0.f,.12f,Age)*(1.f-SmoothRamp(.80f,Duration,Age));
}
inline bool CrossedContact(float Before, float After)
{
    return Before < Contact && After >= Contact;
}
}
