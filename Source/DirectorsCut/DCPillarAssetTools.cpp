#include "DCPillarAssetTools.h"
#include "DCTypes.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "UObject/Package.h"

int32 UDCPillarAssetTools::MakePillarSplashLocal(UParticleSystem* Asset)
{
#if WITH_EDITOR
    // Never mutate the purchased source or a cooked runtime object. Python
    // duplicates the source in the editor and passes only this authoring copy.
    if(!GIsEditor || !Asset || Asset->GetOutermost()->HasAnyPackageFlags(PKG_Cooked)
        || Asset->GetPathName()!=TEXT("/Game/DC/FX/P_PillarWaterContact.P_PillarWaterContact"))
    {
        UE_LOG(LogDC,Error,TEXT("PILLAR_SPLASH authoring requires the uncooked /Game/DC/FX/P_PillarWaterContact copy in editor"));
        return -1;
    }
    Asset->Modify();
    int32 Count=0;
    for(UParticleEmitter* Emitter:Asset->Emitters)
    {
        if(!Emitter) { continue; }
        for(UParticleLODLevel* LOD:Emitter->LODLevels)
        {
            if(!LOD || !LOD->RequiredModule) { continue; }
            LOD->RequiredModule->Modify();
            LOD->RequiredModule->bUseLocalSpace=true;
            ++Count;
        }
    }
    Asset->PostEditChange();
    Asset->MarkPackageDirty();
    UE_LOG(LogDC,Log,TEXT("PILLAR_SPLASH authored local_space_modules=%d asset=%s"),Count,*Asset->GetPathName());
    return Count;
#else
    return -1;
#endif
}
