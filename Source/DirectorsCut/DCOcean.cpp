#include "DCOcean.h"
#include "DCOceanCoordinates.h"
#include "DCTreadmill.h"
#include "DCRideDirector.h"

FVector DCOcean::WaveSamplePosition(const UObject* Context, const FVector& StagePosition)
{
    const ADCTreadmill* Treadmill = ADCTreadmill::Get(Context);
    return FVector(StagePosition.X,WaveY(StagePosition.Y,Treadmill ? Treadmill->GetLaneOffset() : 0.f),StagePosition.Z);
}

float DCOcean::RenderedHeightOffset(const UObject* Context, const FVector& StagePosition)
{
    const ADCRideDirector* Director=ADCRideDirector::Get(Context);
    if(!Director) { return 0.f; }
    const ADCTreadmill* Treadmill=ADCTreadmill::Get(Context);
    const float Lane=Treadmill?Treadmill->GetLaneOffset():0.f;
    // Exact DCTrough material WPO, after raw wave sampling. Do not feed the
    // already shifted WaveSamplePosition into this function a second time.
    return Director->SeaRise(StagePosition.Y+Lane)-Director->SeaRise(Lane);
}
