#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DCPillarAssetTools.generated.h"

/** Narrow editor authoring bridge for Cascade properties hidden from Python. */
UCLASS()
class DIRECTORSCUT_API UDCPillarAssetTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Returns authored emitter LOD count, or -1 outside editor/for wrong asset. */
    UFUNCTION(BlueprintCallable, Category="DirectorsCut|Authoring")
    static int32 MakePillarSplashLocal(class UParticleSystem* Asset);
};
