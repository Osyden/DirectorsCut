#pragma once
#include <algorithm>
#include <cmath>

namespace DCFogRouteLayout
{
// Reserve the complete helm envelope plus the yawed hull, not just the centreline.
inline float ClearEdge(float HelmLimit, float HullHalfLength, float HullHalfWidth)
{ return HelmLimit + std::sqrt(HullHalfLength*HullHalfLength + HullHalfWidth*HullHalfWidth) + 250.f; }
// Scallop only the spare inboard strip. The continuous walking bench begins
// 12m outside ClearEdge and remains unchanged at every longitudinal position.
constexpr float MaximumShoreInset=900.f;
constexpr float ShoreHeadlandHalfWidth=450.f;
constexpr float ShoreHeadlandCentre=500.f;
inline float ShoreInset(float Along,float Side)
{ return MaximumShoreInset*(.5f+.32f*std::sin(Along/17000.f+Side*1.7f)+.18f*std::sin(Along/6300.f-Side*.8f)); }
inline float ShoreOuterExtension(float Along,float Side)
{ return 2500.f+1800.f*std::sin(Along/24000.f+Side*2.3f); }
constexpr float BankWidth = 6000.f;
constexpr float BankTop = 1600.f;
constexpr float BankBottom = -2200.f;
inline float BankCentre(float Clear) { return Clear + BankWidth*.5f; }
inline float SceneryEdge(float Clear) { return Clear + BankWidth + 500.f; }
inline float WalkerOffset(int Index, float MeshRadius)
{ return std::min(Index*650.f, std::max(0.f, BankWidth*.5f-MeshRadius-100.f)); }
// Bounds are already rotated into route space and relative to the actor origin.
inline float FitSide(float RequestedY, float MinY, float MaxY, float Clear)
{ return RequestedY < 0.f ? std::min(RequestedY, -Clear-MaxY) : std::max(RequestedY, Clear-MinY); }
constexpr float GroupTimes[] = {90.f,126.f,162.f,198.f};
constexpr float StatueTimes[] = {105.f,139.f,173.f,211.f};
// Keep the opening lesson unchanged; one additional staggered walker per later wave.
inline int GroupSize(int Group) { return Group==0 ? 3 : 4; }
inline float WalkerHoldX(int Index,int Count) { return 3000.f+Index*(Count>3?3300.f:4600.f); }
inline float GroupSide(int Group) { return Group%2==0 ? -1.f : 1.f; }
}
