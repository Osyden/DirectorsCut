#pragma once

// Original authoring triangles, never Nanite/fallback geometry. Coordinates and
// distances are caller-owned arena centimetres. Unlike DCCliffQueryMesh this
// does not project to XY, and can intersect walls, undersides and overhangs.
// Supplied arrays must outlive the surface and remain immutable until Clear().
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

namespace DCCliffTriangleSurface
{
struct FVec3
{
    double X=0, Y=0, Z=0;
    double Axis(unsigned I) const { return I==0?X:(I==1?Y:Z); }
    FVec3 operator+(const FVec3& B) const { return {X+B.X,Y+B.Y,Z+B.Z}; }
    FVec3 operator-(const FVec3& B) const { return {X-B.X,Y-B.Y,Z-B.Z}; }
    FVec3 operator*(double S) const { return {X*S,Y*S,Z*S}; }
};
inline double Dot(const FVec3& A,const FVec3& B){return A.X*B.X+A.Y*B.Y+A.Z*B.Z;}
inline FVec3 Cross(const FVec3& A,const FVec3& B)
{return {A.Y*B.Z-A.Z*B.Y,A.Z*B.X-A.X*B.Z,A.X*B.Y-A.Y*B.X};}
inline bool Finite(const FVec3& P){return std::isfinite(P.X)&&std::isfinite(P.Y)&&std::isfinite(P.Z);}

struct FMeshView
{
    const FVec3* Vertices=nullptr;
    std::size_t VertexCount=0;
    const std::uint32_t* Indices=nullptr; // consecutive original triangle triples
    std::size_t TriangleCount=0;
};
enum class ECullMode { TwoSided, CullBackFaces, CullFrontFaces };
struct FHit
{
    double DistanceCm=0; // input direction is normalized internally
    FVec3 Position, Normal; // Normal preserves cross(B-A,C-A); never auto-flipped
    std::uint32_t Triangle=0;
    std::uint32_t Vertices[3]={0,0,0};
    double Barycentric[3]={0,0,0};
    bool FrontFace=false; // Dot(source-winding normal, ray direction)<0
};

class FSurface
{
    struct FBounds
    {
        FVec3 Min={INFINITY,INFINITY,INFINITY},Max={-INFINITY,-INFINITY,-INFINITY};
        void Add(const FVec3& P)
        {
            Min={std::min(Min.X,P.X),std::min(Min.Y,P.Y),std::min(Min.Z,P.Z)};
            Max={std::max(Max.X,P.X),std::max(Max.Y,P.Y),std::max(Max.Z,P.Z)};
        }
        void Add(const FBounds& B){Add(B.Min);Add(B.Max);}
    };
    struct FNode { FBounds Bounds; std::uint32_t First=0,Count=0,Left=0,Right=0; };
    FMeshView Mesh;
    std::vector<FNode> Nodes;
    std::vector<std::uint32_t> Order;
    std::vector<FBounds> TriangleBounds;
    std::size_t Degenerates=0;

    std::uint32_t BuildNode(std::uint32_t First,std::uint32_t Count)
    {
        const auto Index=static_cast<std::uint32_t>(Nodes.size());Nodes.emplace_back();
        FBounds Bounds,Centroids;
        for(std::uint32_t I=First;I<First+Count;++I)
        {
            const auto& B=TriangleBounds[Order[I]];Bounds.Add(B);Centroids.Add((B.Min+B.Max)*.5);
        }
        Nodes[Index].Bounds=Bounds;
        if(Count<=8){Nodes[Index].First=First;Nodes[Index].Count=Count;return Index;}
        const FVec3 Extent=Centroids.Max-Centroids.Min;
        const unsigned Axis=Extent.X>=Extent.Y?(Extent.X>=Extent.Z?0:2):(Extent.Y>=Extent.Z?1:2);
        const auto Middle=First+Count/2;
        std::nth_element(Order.begin()+First,Order.begin()+Middle,Order.begin()+First+Count,
            [&](std::uint32_t A,std::uint32_t B)
            {
                const double CA=TriangleBounds[A].Min.Axis(Axis)+TriangleBounds[A].Max.Axis(Axis);
                const double CB=TriangleBounds[B].Min.Axis(Axis)+TriangleBounds[B].Max.Axis(Axis);
                return CA==CB?A<B:CA<CB;
            });
        const auto Left=BuildNode(First,Middle-First),Right=BuildNode(Middle,First+Count-Middle);
        Nodes[Index].Left=Left;Nodes[Index].Right=Right;return Index;
    }

    static bool BoundsHit(const FBounds& B,const FVec3& O,const FVec3& D,double Low,double High,double& Enter)
    {
        constexpr double PaddingCm=1.e-6;
        for(unsigned Axis=0;Axis<3;++Axis)
        {
            const double Min=B.Min.Axis(Axis)-PaddingCm,Max=B.Max.Axis(Axis)+PaddingCm;
            const double Direction=D.Axis(Axis),Origin=O.Axis(Axis);
            if(Direction==0){if(Origin<Min||Origin>Max)return false;continue;}
            double A=(Min-Origin)/Direction,C=(Max-Origin)/Direction;
            if(A>C)std::swap(A,C);
            Low=std::max(Low,A);High=std::min(High,C);
            if(Low>High)return false;
        }
        Enter=Low;return true;
    }

    bool TriangleHit(std::uint32_t T,const FVec3& O,const FVec3& D,double Low,double High,ECullMode Cull,FHit& Hit) const
    {
        const auto* ID=Mesh.Indices+std::size_t(T)*3;
        const auto& A=Mesh.Vertices[ID[0]];const auto& B=Mesh.Vertices[ID[1]];const auto& C=Mesh.Vertices[ID[2]];
        const FVec3 E1=B-A,E2=C-A,N=Cross(E1,E2),P=Cross(D,E2);
        const double NormalLength=std::sqrt(Dot(N,N)),Det=Dot(E1,P);
        // Relative parallel test; degenerate faces were removed during Build.
        if(std::abs(Det)<=NormalLength*1.e-14)return false;
        const bool Front=Det>0;
        if((Cull==ECullMode::CullBackFaces&&!Front)||(Cull==ECullMode::CullFrontFaces&&Front))return false;
        const FVec3 AO=O-A,Q=Cross(AO,E1);
        const double U=Dot(AO,P)/Det,V=Dot(D,Q)/Det;
        constexpr double EdgeTolerance=1.e-10;
        if(U<-EdgeTolerance||V<-EdgeTolerance||U+V>1+EdgeTolerance)return false;
        const double Distance=Dot(E2,Q)/Det;
        if(!std::isfinite(Distance)||Distance<Low||Distance>High)return false;
        Hit.DistanceCm=Distance;Hit.Position=O+D*Distance;Hit.Normal=N*(1/NormalLength);
        Hit.Triangle=T;Hit.FrontFace=Front;
        const double W[3]={1-U-V,U,V};double Sum=0;
        for(int I=0;I<3;++I){Hit.Vertices[I]=ID[I];Hit.Barycentric[I]=std::max(0.,W[I]);Sum+=Hit.Barycentric[I];}
        for(double& Value:Hit.Barycentric)Value/=Sum;
        return Finite(Hit.Position)&&Finite(Hit.Normal);
    }

public:
    void Clear(){Mesh={};Nodes.clear();Order.clear();TriangleBounds.clear();Degenerates=0;}
    bool Build(const FMeshView& Source)
    {
        Clear();
        if(!Source.Vertices||!Source.Indices||Source.VertexCount==0||Source.TriangleCount==0
            ||Source.VertexCount>std::numeric_limits<std::uint32_t>::max()
            ||Source.TriangleCount>std::numeric_limits<std::uint32_t>::max()/2)return false;
        for(std::size_t I=0;I<Source.VertexCount;++I)if(!Finite(Source.Vertices[I]))return false;
        Mesh=Source;TriangleBounds.resize(Source.TriangleCount);Order.reserve(Source.TriangleCount);
        for(std::size_t T=0;T<Source.TriangleCount;++T)
        {
            const auto* ID=Source.Indices+T*3;
            if(ID[0]>=Source.VertexCount||ID[1]>=Source.VertexCount||ID[2]>=Source.VertexCount){Clear();return false;}
            const auto& A=Source.Vertices[ID[0]];const auto& B=Source.Vertices[ID[1]];const auto& C=Source.Vertices[ID[2]];
            const FVec3 E1=B-A,E2=C-A,N=Cross(E1,E2);
            const double EdgeSquared=std::max({Dot(E1,E1),Dot(E2,E2),Dot(C-B,C-B)}),AreaSquared=Dot(N,N);
            if(!std::isfinite(EdgeSquared)||!std::isfinite(AreaSquared)){Clear();return false;}
            if(AreaSquared<=std::max(1.e-24,EdgeSquared*EdgeSquared*1.e-24)){++Degenerates;continue;}
            TriangleBounds[T].Add(A);TriangleBounds[T].Add(B);TriangleBounds[T].Add(C);
            Order.push_back(static_cast<std::uint32_t>(T));
        }
        if(Order.empty()){Clear();return false;}
        Nodes.reserve(Order.size()*2);BuildNode(0,static_cast<std::uint32_t>(Order.size()));return true;
    }
    std::size_t TriangleCount() const{return Order.size();}
    std::size_t DegenerateCount() const{return Degenerates;}
    std::size_t NodeCount() const{return Nodes.size();}

    bool TraceNearest(const FVec3& Origin,const FVec3& Direction,FHit& Out,
        double MinDistanceCm=0,double MaxDistanceCm=std::numeric_limits<double>::infinity(),
        ECullMode Cull=ECullMode::TwoSided) const
    {
        if(Nodes.empty()||!Finite(Origin)||!Finite(Direction)||!std::isfinite(MinDistanceCm)
            ||MinDistanceCm<0||std::isnan(MaxDistanceCm)||MaxDistanceCm<MinDistanceCm)return false;
        const double Length=std::hypot(Direction.X,Direction.Y,Direction.Z);
        if(!std::isfinite(Length)||Length<=0)return false;
        // Divide components, rather than multiply by reciprocal: a finite
        // subnormal direction can have a reciprocal larger than DBL_MAX.
        const FVec3 D={Direction.X/Length,Direction.Y/Length,Direction.Z/Length};
        // Balanced median construction has depth <=32 for the uint32 triangle
        // limit. This stack does not allocate during per-ray queries.
        std::array<std::uint32_t,64> Stack;unsigned Size=1;Stack[0]=0;
        bool Found=false;FHit Best;double High=MaxDistanceCm;
        while(Size)
        {
            const auto& Node=Nodes[Stack[--Size]];double Enter=0;
            if(!BoundsHit(Node.Bounds,Origin,D,MinDistanceCm,High,Enter))continue;
            if(Node.Count)
            {
                for(std::uint32_t I=Node.First;I<Node.First+Node.Count;++I)
                {
                    FHit Hit;
                    if(TriangleHit(Order[I],Origin,D,MinDistanceCm,High,Cull,Hit))
                    {
                        if(!Found||Hit.DistanceCm<Best.DistanceCm
                            ||(Hit.DistanceCm==Best.DistanceCm&&Hit.Triangle<Best.Triangle))
                        {Best=Hit;High=Hit.DistanceCm;Found=true;}
                    }
                }
            }
            else
            {
                double LeftEnter=0,RightEnter=0;
                const bool L=BoundsHit(Nodes[Node.Left].Bounds,Origin,D,MinDistanceCm,High,LeftEnter);
                const bool R=BoundsHit(Nodes[Node.Right].Bounds,Origin,D,MinDistanceCm,High,RightEnter);
                if(L&&R)
                {
                    const bool LeftFirst=LeftEnter<=RightEnter;
                    Stack[Size++]=LeftFirst?Node.Right:Node.Left;Stack[Size++]=LeftFirst?Node.Left:Node.Right;
                }
                else if(L)Stack[Size++]=Node.Left;
                else if(R)Stack[Size++]=Node.Right;
            }
        }
        if(Found)Out=Best;return Found; // failed calls do not mutate caller's hit
    }
};
} // namespace DCCliffTriangleSurface
