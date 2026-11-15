#pragma once

// Original authoring-grid query, never a Nanite/fallback/render-LOD query.
// Arena coordinates are centimetres before the moving ship-centred actor pose.
// The caller owns/parses JSON arrays and validates the view once after loading.
// This is a single-valued XY heightfield: no overhang, raycast or 3D collision.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace DCCliffQueryMesh
{
struct FPoint { double X=0, Y=0, Z=0; };

struct FGridView
{
    std::uint32_t Rows=0, Columns=0;
    double CellSizeCm=0;
    FPoint Origin, BasisU, BasisV;
    const FPoint* Vertices=nullptr;
    std::size_t VertexCount=0;
    const std::int32_t* GridToVertex=nullptr; // row-major; -1 means absent
    std::size_t GridCount=0;
};

struct FHit
{
    double HeightCm=0;
    FPoint Normal;
    std::uint32_t Row=0, Column=0;
    std::uint8_t Triangle=0; // 0=(a,b,d), 1=(a,d,c), exactly the FBX exporter
    std::int32_t Vertices[3]={-1,-1,-1};
    double Barycentric[3]={0,0,0};
};

inline bool Finite(const FPoint& P)
{ return std::isfinite(P.X) && std::isfinite(P.Y) && std::isfinite(P.Z); }

inline bool ValidFrame(const FGridView& G)
{
    if(G.Rows<2 || G.Columns<2 || !std::isfinite(G.CellSizeCm) || G.CellSizeCm<=0
        || !Finite(G.Origin) || !Finite(G.BasisU) || !Finite(G.BasisV)) return false;
    const auto& U=G.BasisU; const auto& V=G.BasisV;
    constexpr double E=1.e-8;
    return std::abs(U.Z)<E && std::abs(V.Z)<E
        && std::abs(U.X*U.X+U.Y*U.Y-1)<E
        && std::abs(V.X*V.X+V.Y*V.Y-1)<E
        && std::abs(U.X*V.X+U.Y*V.Y)<E
        && U.X*V.Y-U.Y*V.X>0; // exporter requires right-handed XY frame
}

inline bool Validate(const FGridView& G, double PositionToleranceCm=1.e-4)
{
    if(!ValidFrame(G) || !G.Vertices || !G.GridToVertex || G.VertexCount==0
        || G.VertexCount>std::size_t(std::numeric_limits<std::int32_t>::max())
        || G.GridCount!=std::size_t(G.Rows)*G.Columns
        || !std::isfinite(PositionToleranceCm) || PositionToleranceCm<0) return false;
    for(std::size_t I=0; I<G.VertexCount; ++I) if(!Finite(G.Vertices[I])) return false;
    for(std::size_t I=0; I<G.GridCount; ++I)
    {
        const auto V=G.GridToVertex[I];
        if(V==-1) continue;
        if(V<0 || std::size_t(V)>=G.VertexCount) return false;
        const double U=double(I%G.Columns)*G.CellSizeCm;
        const double R=double(I/G.Columns)*G.CellSizeCm;
        const auto& P=G.Vertices[V];
        if(std::abs(P.X-(G.Origin.X+G.BasisU.X*U+G.BasisV.X*R))>PositionToleranceCm
            || std::abs(P.Y-(G.Origin.Y+G.BasisU.Y*U+G.BasisV.Y*R))>PositionToleranceCm) return false;
    }
    return true;
}

inline bool SampleTriangle(const FGridView& G, std::uint32_t Row, std::uint32_t Col,
    std::uint8_t Triangle, double U, double V, double Epsilon, FHit& Out)
{
    double W[3];
    if(Triangle==0) { W[0]=1-U; W[1]=U-V; W[2]=V; }
    else { W[0]=1-V; W[1]=U; W[2]=V-U; }
    if(W[0]<-Epsilon || W[1]<-Epsilon || W[2]<-Epsilon) return false;
    const std::size_t A=std::size_t(Row)*G.Columns+Col;
    const std::size_t Corner[3]={A, Triangle==0 ? A+1 : A+G.Columns+1,
                                  Triangle==0 ? A+G.Columns+1 : A+G.Columns};
    FHit Hit; Hit.Row=Row; Hit.Column=Col; Hit.Triangle=Triangle;
    double Sum=0;
    for(int I=0; I<3; ++I)
    {
        if(Corner[I]>=G.GridCount) return false;
        Hit.Vertices[I]=G.GridToVertex[Corner[I]];
        if(Hit.Vertices[I]<0 || std::size_t(Hit.Vertices[I])>=G.VertexCount) return false;
        Hit.Barycentric[I]=std::max(0., W[I]); Sum+=Hit.Barycentric[I];
    }
    if(Sum<=0) return false;
    for(int I=0; I<3; ++I)
    { Hit.Barycentric[I]/=Sum; Hit.HeightCm+=Hit.Barycentric[I]*G.Vertices[Hit.Vertices[I]].Z; }
    const auto& P=G.Vertices[Hit.Vertices[0]];
    const auto& Q=G.Vertices[Hit.Vertices[1]];
    const auto& R=G.Vertices[Hit.Vertices[2]];
    const FPoint A1={Q.X-P.X,Q.Y-P.Y,Q.Z-P.Z}, B={R.X-P.X,R.Y-P.Y,R.Z-P.Z};
    Hit.Normal={A1.Y*B.Z-A1.Z*B.Y, A1.Z*B.X-A1.X*B.Z, A1.X*B.Y-A1.Y*B.X};
    const double Length=std::sqrt(Hit.Normal.X*Hit.Normal.X+Hit.Normal.Y*Hit.Normal.Y+Hit.Normal.Z*Hit.Normal.Z);
    if(!std::isfinite(Hit.HeightCm) || !std::isfinite(Length) || Length<=0) return false;
    Hit.Normal.X/=Length; Hit.Normal.Y/=Length; Hit.Normal.Z/=Length;
    Out=Hit; return true;
}

inline bool QueryHeight(const FGridView& G, double ArenaXcm, double ArenaYcm, FHit& Out)
{
    // Validate() is the one-time load check; these constant-time guards protect
    // empty views and non-finite/out-of-domain calls. Failed calls leave Out intact.
    if(!ValidFrame(G) || !G.Vertices || !G.GridToVertex
        || G.GridCount!=std::size_t(G.Rows)*G.Columns
        || !std::isfinite(ArenaXcm) || !std::isfinite(ArenaYcm)) return false;
    const double X=ArenaXcm-G.Origin.X, Y=ArenaYcm-G.Origin.Y;
    double U=(X*G.BasisU.X+Y*G.BasisU.Y)/G.CellSizeCm;
    double V=(X*G.BasisV.X+Y*G.BasisV.Y)/G.CellSizeCm;
    constexpr double E=1.e-8; // grid-coordinate tolerance; 2e-6cm at the 2m grid
    if(U<-E || V<-E || U>double(G.Columns-1)+E || V>double(G.Rows-1)+E) return false;
    U=std::clamp(U,0.,double(G.Columns-1)); V=std::clamp(V,0.,double(G.Rows-1));
    // World-frame dot products can put an exact source vertex just below an
    // integer. Snap only within the declared tolerance before choosing cells.
    if(std::abs(U-std::round(U))<=E) U=std::round(U);
    if(std::abs(V-std::round(V))<=E) V=std::round(V);
    const auto Col=std::min(std::uint32_t(std::floor(U)),G.Columns-2);
    const auto Row=std::min(std::uint32_t(std::floor(V)),G.Rows-2);
    // On an exact grid edge, the forward cell may be absent while a neighbouring
    // triangle still owns that edge. Check at most four cells; never bridge holes.
    const bool Left=Col>0 && std::abs(U-double(Col))<=E;
    const bool Below=Row>0 && std::abs(V-double(Row))<=E;
    for(unsigned DY=0; DY<=(Below ? 1u : 0u); ++DY)
    for(unsigned DX=0; DX<=(Left ? 1u : 0u); ++DX)
    {
        const auto C=Col-DX, R=Row-DY;
        const double LocalU=U-C, LocalV=V-R;
        if(SampleTriangle(G,R,C,0,LocalU,LocalV,E,Out)
            || SampleTriangle(G,R,C,1,LocalU,LocalV,E,Out)) return true;
    }
    return false;
}
} // namespace DCCliffQueryMesh
