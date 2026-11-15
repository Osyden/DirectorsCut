#include "DCMaelstrom.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"

namespace
{
// Unequal overlapping impact lobes follow the river's actual lower surface.
// Small time-based drift softens the silhouette without crossing the nominal
// radius928m orbit or accumulating offsets during seeks. Outer steering can
// enter this bank; the full steering envelope extends to radius1038m.
FVector CalderaImpactMist(int32 I,float Time,float& RadiusM,float& Density)
{
    static const float Radii[]={46.f,43.f,45.f,48.f,43.f,47.f,46.f,44.f,42.f};
    static const float Centres[]={996.f,1008.f,989.f,1007.f,993.f,1018.f,1000.f,990.f,1008.f};
    static const float Heights[]={2.f,4.f,8.f,2.f,4.f,6.f,3.f,7.f,1.f};
    static const float Strengths[]={.85f,1.10f,.95f,1.20f,.90f,1.10f,1.f,.80f,.90f};
    I=FMath::Clamp(I,0,8); RadiusM=Radii[I]; Density=Strengths[I];
    const float R=Centres[I]+2.f*FMath::Sin(Time*.12f+I*2.31f);
    const float Across=(I-4)*45.f+5.f*FMath::Sin(I*1.71f);
    const float Q=FMath::SmoothStep(.58f,.985f,R/1600.f);
    const float A=UE_PI-.40f*(1.f-Q)+Across/R;
    return FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,
        -400.f+400.f*Q+Heights[I]+1.8f*FMath::Sin(Time*.18f+I*1.73f));
}
}

void ADCMaelstrom::UpdateInletMist()
{
    if(!IsCalderaLake()||!bActive||Elapsed<-7.f) { ClearInletMist(); return; }
    // Small volumes distributed around the shoreline need more instances,
    // rather than kilometre-scale clouds. Restore the shared budget on exit.
    if(IConsoleVariable* Budget=IConsoleManager::Get().FindConsoleVariable(TEXT("r.LocalFogVolume.TileMaxInstanceCount")))
    {
        if(PreviousCalderaFogBudget<0) { PreviousCalderaFogBudget=Budget->GetInt(); }
        AppliedCalderaFogBudget=FMath::Max(256,PreviousCalderaFogBudget);
        if(Budget->GetInt()!=AppliedCalderaFogBudget) { Budget->SetWithCurrentPriority(AppliedCalderaFogBudget); }
    }
    const FName ShoreTag(TEXT("DCCalderaShoreline")),SprayTag(TEXT("DCCalderaRiverSpray")),FootTag(TEXT("DCCalderaImpactFog"));
    if(!bInletMistTried)
    {
        bInletMistTried=true;
        auto Add=[&](FVector P,float RadiusM,FName Tag,float Density,FVector Up)
        {
            ULocalFogVolumeComponent* Fog=NewObject<ULocalFogVolumeComponent>(this);
            AddInstanceComponent(Fog); Fog->SetupAttachment(Arena);
            Fog->SetMobility(EComponentMobility::Movable);
            Fog->SetRelativeLocation(P*100.f);
            Fog->SetRelativeRotation(FRotationMatrix::MakeFromZ(Up).Rotator());
            Fog->SetRelativeScale3D(FVector(RadiusM*100.f/ULocalFogVolumeComponent::GetBaseVolumeSize()));
            // UE combines radial and height coverage multiplicatively. Zero
            // radial extinction makes the shoreline volumes fully invisible.
            Fog->RadialFogExtinction=Tag==ShoreTag?.09f:0.f;
            Fog->HeightFogExtinction=Density;
            Fog->HeightFogFalloff=Tag==ShoreTag?500.f:(Tag==FootTag?350.f:420.f);
            Fog->FogAlbedo=FLinearColor(.56f,.63f,.68f);
            Fog->FogEmissive=FLinearColor::Black; Fog->FogPhaseG=.20f;
            Fog->ComponentTags.Add(Tag); Fog->RegisterComponent(); InletMistVolumes.Add(Fog);
        };
        TArray<FVector> Shore;
        FString Text; TSharedPtr<FJsonObject> Data;
        if(FFileHelper::LoadFileToString(Text,*(CliffArenaDataDirectory/TEXT("CompleteArena.json")))
            &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Data)&&Data.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* Points=nullptr;
            if((Data->TryGetArrayField(TEXT("shore_mist_coverage_centers_m"),Points)&&!Points->IsEmpty())
                ||Data->TryGetArrayField(TEXT("shore_mist_centers_m"),Points))
            {
                for(const TSharedPtr<FJsonValue>& Point:*Points)
                {
                    const TArray<TSharedPtr<FJsonValue>>* XYZ=nullptr;
                    if(Point->TryGetArray(XYZ)&&XYZ->Num()==3)
                    { Shore.Add(FVector((*XYZ)[0]->AsNumber(),(*XYZ)[1]->AsNumber(),-398.f)); }
                }
            }
        }
        // Coverage-aware contacts come from the actual waterline triangles.
        // A 46m footprint and ~9m density e-folding height make a low veil,
        // keeping the upper cliffs clear. Older installations retain128 points.
        const int32 ShoreCount=FMath::Clamp(Shore.Num(),128,160);
        for(int32 I=0;I<ShoreCount;++I)
        {
            const float A=UE_TWO_PI*I/ShoreCount;
            FVector P(FMath::Cos(A)*1050.f,FMath::Sin(A)*1050.f,-398.f);
            if(!Shore.IsEmpty())
            {
                const float S=I*Shore.Num()/static_cast<float>(ShoreCount); const int32 J=FMath::FloorToInt(S);
                P=FMath::Lerp(Shore[J%Shore.Num()],Shore[(J+1)%Shore.Num()],S-J);
            }
            Add(P,46.f,ShoreTag,.28f,FVector::UpVector);
        }
        // Rotate the shallow density profile to the river normal. Horizontal
        // layers cut a steep river into bright stripes. Staggered, overlapping
        // volumes instead form a continuous thin veil above the moving water.
        FRandomStream SprayRandom(151128);
        for(int32 Row=0;Row<16;++Row)
        {
            for(int32 Side=-2;Side<=2;++Side)
            {
                const float R=FMath::Lerp(1545.f,930.f,Row/15.f)+SprayRandom.FRandRange(-9.f,9.f);
                const float U=FMath::Clamp((R/1600.f-.58f)/.405f,0.f,1.f);
                const float Q=U*U*(3.f-2.f*U);
                const float Across=Side*74.f+SprayRandom.FRandRange(-20.f,20.f);
                const float A=UE_PI-.40f*(1.f-Q)+Across/R;
                const float Slope=400.f*6.f*U*(1.f-U)/(1600.f*.405f);
                const FVector Up=FVector(-Slope*FMath::Cos(A),-Slope*FMath::Sin(A),1.f).GetSafeNormal();
                const FVector P(FMath::Cos(A)*R,FMath::Sin(A)*R,-400.f+400.f*Q);
                Add(P-Up*2.f,SprayRandom.FRandRange(37.f,43.f),SprayTag,.07f,Up);
            }
        }
        // Dense, connected impact fog conceals the artificial river/lake join.
        // Keep its horizontal footprint at the foot, not across the whole lake.
        for(int32 I=0;I<9;++I)
        {
            // Keep the inner fog boundary outside the radius928m battle lane;
            // the ship must clear the impact cloud as soon as it levels out.
            float RadiusM,Density;
            const FVector P=CalderaImpactMist(I,Elapsed,RadiusM,Density);
            Add(P,RadiusM,FootTag,1.25f*Density,FVector::UpVector);
        }
        UE_LOG(LogTemp,Display,TEXT("DC_CALDERA_MIST shoreline=%d radius=46m radial_extinction=0.09 height_falloff=500 river_spray=80 radius=37-43m impact_fog=9 radius=42-48m slope_aligned=1 billboards=0"),ShoreCount);
    }
    const float Gather=FMath::SmoothStep(26.f,42.f,Elapsed);
    int32 FootIndex=0;
    for(ULocalFogVolumeComponent* Fog:InletMistVolumes)
    {
        if(!Fog) { continue; }
        if(Fog->ComponentHasTag(SprayTag))
        {
            Fog->SetHeightFogExtinction(.07f+.33f*Gather);
            Fog->SetRadialFogExtinction(.015f+.045f*Gather);
        }
        if(Fog->ComponentHasTag(FootTag))
        {
            float RadiusM,Density;
            Fog->SetRelativeLocation(CalderaImpactMist(FootIndex++,Elapsed,RadiusM,Density)*100.f);
            Fog->SetHeightFogExtinction((1.25f+.75f*Gather)*Density);
            Fog->SetRadialFogExtinction((.025f+.075f*Gather)*Density);
        }
        Fog->SetVisibility(true);
    }
}

void ADCMaelstrom::ClearInletMist()
{
    for(ULocalFogVolumeComponent* Fog:InletMistVolumes) { if(Fog) { Fog->SetVisibility(false); } }
    if(PreviousCalderaFogBudget>=0)
    {
        if(IConsoleVariable* Budget=IConsoleManager::Get().FindConsoleVariable(TEXT("r.LocalFogVolume.TileMaxInstanceCount")))
            { if(Budget->GetInt()==AppliedCalderaFogBudget) { Budget->SetWithCurrentPriority(PreviousCalderaFogBudget); } }
        PreviousCalderaFogBudget=-1;
    }
}
