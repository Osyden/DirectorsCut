#pragma once
#include "CoreMinimal.h"

/** Short free-flight reaches between measured cliff contacts, in physical cm.
 * V still addresses the original descending cliff profile; only XY changes.
 * Each lateral track is a constant-horizontal-velocity, -980cm/s^2 trajectory.
 * Keeping endpoint heights/contact parameters prevents a synthetic toe curl. */
namespace DCWaterfallTrajectory
{
inline constexpr float Gravity=980.f;
struct FSegment
{
    float StartV=0.f,EndV=0.f,InitialDown=0.f;
    float PeakSeparation=0.f,MinClearance=0.f,Duration=0.f;
};

inline float TimeForDrop(float Drop,float InitialDown)
{ return 2.f*FMath::Max(0.f,Drop)/(FMath::Sqrt(InitialDown*InitialDown+2.f*Gravity*FMath::Max(0.f,Drop))+InitialDown); }

template<typename SurfaceSampler>
inline float ParameterAtHeight(const SurfaceSampler& Surface,float Height,float LowV,float HighV)
{
    for(int32 I=0;I<24;++I)
    {
        const float Mid=(LowV+HighV)*.5f;
        if(Surface(Mid,0.f).Z<Height) { LowV=Mid; } else { HighV=Mid; }
    }
    return (LowV+HighV)*.5f;
}

template<typename SurfaceSampler>
inline FVector Sample(const FSegment& Segment,const SurfaceSampler& Surface,float V,float Across)
{
    const FVector Start=Surface(Segment.StartV,Across),End=Surface(Segment.EndV,Across);
    const float Duration=TimeForDrop(Start.Z-End.Z,Segment.InitialDown);
    const float Z=Surface(V,Across).Z;
    const float T=TimeForDrop(Start.Z-Z,Segment.InitialDown);
    FVector P=FMath::Lerp(Start,End,FMath::Clamp(T/FMath::Max(.001f,Duration),0.f,1.f));
    P.Z=Z;
    return P;
}

inline const FSegment* Find(const TArray<FSegment>& Segments,float V)
{
    return Segments.FindByPredicate([V](const FSegment& S){return V<=S.StartV && V>=S.EndV;});
}

/** A candidate must clear the measured profile across its complete width,
 * remain above covered receiving water, and have bounded downward launch speed.
 * Surface is the rendered-triangle water-contact surface, including its existing
 * clearance from solid rock. Rejecting any negative separation is conservative. */
template<typename SurfaceSampler,typename WaterSampler>
inline bool Validate(FSegment& S,const SurfaceSampler& Surface,const WaterSampler& WaterAt,float HalfWidth)
{
    S.PeakSeparation=0.f; S.MinClearance=MAX_flt;
    const FVector Start=Surface(S.StartV,0.f),End=Surface(S.EndV,0.f);
    if(Start.Z-End.Z<300.f || S.InitialDown<=0.f) { return false; }
    S.Duration=TimeForDrop(Start.Z-End.Z,S.InitialDown);
    const FVector Launch=(End-Start)/S.Duration;
    if(Launch.Size2D()>3500.f || S.Duration>4.f) { return false; }
    for(int32 Lane=0;Lane<9;++Lane) for(int32 Step=0;Step<=96;++Step)
    {
        const float Across=HalfWidth*(Lane/4.f-1.f),V=FMath::Lerp(S.StartV,S.EndV,Step/96.f);
        const FVector Rock=Surface(V,Across),P=Sample(S,Surface,V,Across);
        // At equal height, smaller basin radius is the air side of this cliff.
        const float Clearance=Rock.Size2D()-P.Size2D();
        if(P.ContainsNaN() || Clearance<-.5f) { return false; }
        float H=0.f;
        if(WaterAt(FVector2D(P),H) && P.Z<=H) { return false; }
        S.MinClearance=FMath::Min(S.MinClearance,Clearance);
        S.PeakSeparation=FMath::Max(S.PeakSeparation,Clearance);
    }
    return S.PeakSeparation>=120.f && S.PeakSeparation<=1800.f;
}

template<typename SurfaceSampler,typename WaterSampler>
inline TArray<FSegment> Build(const SurfaceSampler& Surface,const WaterSampler& WaterAt,float HalfWidth,float FootZ)
{
    TArray<FSegment> Candidates,Result;
    // Do not alter supported shoreline/talus or the terminal contact. Only the
    // lower80m face is considered; each gap lands back on the real surface.
    const float LowerV=.215f,HighestZ=FMath::Min(static_cast<float>(Surface(.84f,0.f).Z),FootZ+8000.f);
    const float UpperV=ParameterAtHeight(Surface,HighestZ,LowerV,.84f);
    if(UpperV<=LowerV) { return Result; }
    for(int32 Index=0;Index<60;++Index)
    {
        const float StartV=FMath::Lerp(UpperV,LowerV,Index/60.f);
        const FVector Start=Surface(StartV,0.f);
        const FVector Incoming=(Start-Surface(FMath::Min(.84f,StartV+.008f),0.f)).GetSafeNormal();
        if(Incoming.Z>=-.08f) { continue; }
        const float FlowSpeed=FMath::Clamp(FMath::Sqrt(600.f*600.f+2.f*Gravity*FMath::Max(0.0,Surface(.84f,0.f).Z-Start.Z))*.45f,900.f,2800.f);
        const float InitialDown=FMath::Clamp(static_cast<float>(-Incoming.Z)*FlowSpeed,250.f,2400.f);
        for(float Drop:{600.f,1000.f,1600.f,2200.f})
        {
            if(Start.Z-Drop<Surface(LowerV,0.f).Z) { continue; }
            FSegment S; S.StartV=StartV; S.EndV=ParameterAtHeight(Surface,Start.Z-Drop,LowerV,StartV); S.InitialDown=InitialDown;
            const FVector End=Surface(S.EndV,0.f);
            const float Duration=TimeForDrop(Start.Z-End.Z,InitialDown);
            const FVector Velocity((End.X-Start.X)/Duration,(End.Y-Start.Y)/Duration,-InitialDown);
            // Constrained shooting solution, not an arbitrary upward fountain.
            // Reject a launch direction incompatible with the incoming flow.
            if(FVector::DotProduct(Velocity.GetSafeNormal(),Incoming)<.72f) { continue; }
            if(Validate(S,Surface,WaterAt,HalfWidth)) { Candidates.Add(S); }
        }
    }
    Candidates.Sort([](const FSegment& A,const FSegment& B){return A.PeakSeparation>B.PeakSeparation;});
    for(const FSegment& Candidate:Candidates)
    {
        const bool bOverlaps=Result.ContainsByPredicate([&](const FSegment& S)
        { return Candidate.EndV<S.StartV+.025f && Candidate.StartV>S.EndV-.025f; });
        if(!bOverlaps) { Result.Add(Candidate); }
        if(Result.Num()==3) { break; }
    }
    Result.Sort([](const FSegment& A,const FSegment& B){return A.StartV>B.StartV;});
    return Result;
}
}
