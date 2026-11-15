#include "DCMaelstrom.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"

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
            Fog->RadialFogExtinction=0.f;
            Fog->HeightFogExtinction=Density;
            Fog->HeightFogFalloff=Tag==ShoreTag?1100.f:(Tag==FootTag?200.f:420.f);
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
            if(Data->TryGetArrayField(TEXT("shore_mist_centers_m"),Points))
            {
                for(const TSharedPtr<FJsonValue>& Point:*Points)
                {
                    const TArray<TSharedPtr<FJsonValue>>* XYZ=nullptr;
                    if(Point->TryGetArray(XYZ)&&XYZ->Num()==3)
                    { Shore.Add(FVector((*XYZ)[0]->AsNumber(),(*XYZ)[1]->AsNumber(),-399.f)); }
                }
            }
        }
        // Sphere boundaries confine haze to 40 m from the surveyed waterline.
        // Height extinction leaves a low veil, not large round cloud puffs.
        for(int32 I=0;I<128;++I)
        {
            const float A=UE_TWO_PI*I/128.f;
            FVector P(FMath::Cos(A)*1050.f,FMath::Sin(A)*1050.f,-399.f);
            if(!Shore.IsEmpty())
            {
                const float S=I*Shore.Num()/128.f; const int32 J=FMath::FloorToInt(S);
                P=FMath::Lerp(Shore[J%Shore.Num()],Shore[(J+1)%Shore.Num()],S-J);
            }
            Add(P,40.f,ShoreTag,.30f,FVector::UpVector);
        }
        // Rotate the shallow density profile to the river normal. Horizontal
        // layers cut a steep river into bright stripes. Staggered, overlapping
        // volumes instead form a continuous thin veil above the moving water.
        FRandomStream SprayRandom(151128);
        for(int32 Row=0;Row<22;++Row)
        {
            for(int32 Side=-2;Side<=2;++Side)
            {
                const float R=FMath::Lerp(1545.f,930.f,Row/21.f)+SprayRandom.FRandRange(-9.f,9.f);
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
        for(int32 I=-4;I<=4;++I)
        {
            // Keep the inner fog boundary outside the radius928m battle lane;
            // the ship must clear the impact cloud as soon as it levels out.
            const float R=995.f+SprayRandom.FRandRange(-7.f,7.f);
            const float Across=I*45.f+SprayRandom.FRandRange(-6.f,6.f);
            const float Q=FMath::SmoothStep(.58f,.985f,R/1600.f);
            const float A=UE_PI-.40f*(1.f-Q)+Across/R;
            Add(FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,-383.f),
                48.f,FootTag,1.25f,FVector::UpVector);
        }
        UE_LOG(LogTemp,Display,TEXT("DC_CALDERA_MIST shoreline=128 radius=40m river_spray=110 radius=37-43m impact_fog=9 radius=48m slope_aligned=1 billboards=0"));
    }
    const float Gather=FMath::SmoothStep(26.f,42.f,Elapsed);
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
            Fog->SetHeightFogExtinction(1.25f+.75f*Gather);
            Fog->SetRadialFogExtinction(.12f+.22f*Gather);
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
