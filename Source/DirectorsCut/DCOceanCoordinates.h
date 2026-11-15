#pragma once
namespace DCOcean
{
    // Opposite to the scenery translation (BaseY - LaneOffset).
    inline double WaveY(double StageY, double LaneOffset) { return StageY+LaneOffset; }
}
