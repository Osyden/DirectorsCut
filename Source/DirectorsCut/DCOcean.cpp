#include "DCOcean.h"
#include "DCOceanCoordinates.h"
#include "DCTreadmill.h"

FVector DCOcean::WaveSamplePosition(const UObject* Context, const FVector& StagePosition)
{
    const ADCTreadmill* Treadmill = ADCTreadmill::Get(Context);
    return FVector(StagePosition.X,WaveY(StagePosition.Y,Treadmill ? Treadmill->GetLaneOffset() : 0.f),StagePosition.Z);
}
