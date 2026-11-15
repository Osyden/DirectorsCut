#pragma once
namespace DCWheelVisual
{
    // +X points away from the helmsman. Negative rotation moves the top to +Y/starboard.
    inline float Degrees(float Steer) { return -65.f*Steer; }
}
