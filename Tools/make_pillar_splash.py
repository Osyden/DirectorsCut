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

required_count = 0
for emitter in asset.get_editor_property('emitters'):
    if not emitter:
        continue
    for lod in emitter.get_editor_property('lod_levels'):
        if not lod:
            continue
        required = lod.get_editor_property('required_module')
        if not required:
            continue
        required.set_editor_property('use_local_space', True)
        if not required.get_editor_property('use_local_space'):
            raise RuntimeError('Pillar splash failed to retain local-space setting')
        required_count += 1

if required_count == 0:
    raise RuntimeError('Pillar splash has no required emitter modules')
if not eal.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError('Could not save the pillar splash asset')
unreal.log_warning('PILLAR_SPLASH saved=%s local_space_modules=%d' % (destination, required_count))
