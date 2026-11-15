"""Author the pillar's local-space Cascade copy BEFORE cooking.

The vendor particle system is preserved. Runtime code only loads this asset:
duplicating cooked Cascade required modules can fail on cooked SubUV data.
Run through the project's Unreal Python commandlet before the release cook.
"""
import unreal

source = '/Game/ParagonRampage/FX/Particles/Abilities/RipNToss/FX/P_RipNToss_WaterImpact'
destination = '/Game/DC/FX/P_PillarWaterContact'
eal = unreal.EditorAssetLibrary
asset = unreal.load_asset(destination) if eal.does_asset_exist(destination) else eal.duplicate_asset(source, destination)
if not isinstance(asset, unreal.ParticleSystem):
    raise RuntimeError('Unable to create the pillar splash ParticleSystem')

# UE5.8 protects ParticleSystem.Emitters from Python reflection. Traverse its
# public C++ API through this editor-only helper, which also marks the copy dirty.
required_count = unreal.DCPillarAssetTools.make_pillar_splash_local(asset)

if required_count <= 0:
    raise RuntimeError('Pillar splash authoring failed or has no required emitter modules')
if not eal.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError('Could not save the pillar splash asset')
unreal.log_warning('PILLAR_SPLASH saved=%s local_space_modules=%d' % (destination, required_count))
