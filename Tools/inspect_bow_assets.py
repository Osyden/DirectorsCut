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
for name in ('M_Water_Footfall', 'M_Water_Drops_Master', 'M_WaterMist', 'M_WaterImpact_Strand'):
    material = unreal.load_asset('/Game/ParagonRampage/FX/Materials/Water/' + name)
    if not material:
        continue
    for flag in ('used_with_particle_sprites', 'used_with_niagara_sprites',
                 'blend_mode', 'material_domain', 'automatically_set_usage_in_editor'):
        unreal.log_warning('DC_BOW usage %s %s=%s' % (name, flag, material.get_editor_property(flag)))
    for parameter in mel.get_scalar_parameter_names(material):
        value = mel.get_material_default_scalar_parameter_value(material, parameter)
        unreal.log_warning('DC_BOW material %s %s=%s' % (name, parameter, value))
    seen = set()
    def inspect(node):
        if not node or node.get_path_name() in seen:
            return
        seen.add(node.get_path_name())
        if node.get_class().get_name() == 'MaterialExpressionDynamicParameter':
            unreal.log_warning('DC_BOW dynamic %s %s' % (name, node.get_editor_property('default_value')))
        for child in mel.get_inputs_for_material_expression(material, node):
            inspect(child)
    for output in (unreal.MaterialProperty.MP_OPACITY, unreal.MaterialProperty.MP_EMISSIVE_COLOR,
                   unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET):
        inspect(mel.get_material_property_input_node(material, output))
