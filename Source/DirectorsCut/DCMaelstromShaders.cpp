#include "CoreMinimal.h"
#include "Misc/DelayedAutoRegister.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

// Keep the maelstrom shader bridge self-contained; no change to the shared
// game module or vendor plugin. The delayed callback also handles late loads.
static const FDelayedAutoRegisterHelper RegisterMaelstromShaders(
    EDelayedRegisterRunPhase::ShaderTypesReady, []
    {
        const FString Directory=FPaths::Combine(FPaths::ProjectDir(),TEXT("Shaders/Maelstrom"));
        if(FPaths::DirectoryExists(Directory) && !AllShaderSourceDirectoryMappings().Contains(TEXT("/DCMaelstrom")))
        {
            AddShaderSourceDirectoryMapping(TEXT("/DCMaelstrom"),Directory);
        }
    });
