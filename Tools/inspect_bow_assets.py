"""Read-only audit of existing water materials and bow splash emitters. Never saves assets."""
import unreal

mel = unreal.MaterialEditingLibrary
for name in ('M_DRW_High', 'M_DRW_Med'):
    material = unreal.load_asset('/DynamicRealWater/Materials/OceanLOD/' + name)
    if not material:
        continue
    for parameter in mel.get_scalar_parameter_names(material):
        if any(word in str(parameter).lower() for word in ('foam', 'shore', 'interact', 'wake')):
            unreal.log_warning('DC_BOW scalar %s %s' % (name, parameter))
system = unreal.load_asset('/Game/ParagonRampage/FX/Particles/Abilities/RipNToss/FX/P_RipNToss_WaterImpact')
unreal.log_warning('DC_BOW splash=%s' % (system.get_path_name() if system else 'missing'))
# ParticleSystem.Emitters is protected from Python. The game's -DCBowAssetAudit
# reads emitter names/materials/local-space flags through the native API.
