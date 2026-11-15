#pragma once

// Seconds of one deliberate crew rag stroke. Contact happens once during the
// downstroke; recovery never applies more suppression, even after a long frame.
namespace DCCrewWorkPolicy
{
constexpr float Contact = .42f;
constexpr float Duration = 1.05f;
inline bool CrossedContact(float Before, float After)
{
    return Before < Contact && After >= Contact;
}
}
