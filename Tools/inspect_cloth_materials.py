"""Read-only parameter audit for reusing the ship's cloth in the captain's rag."""
import unreal

mel = unreal.MaterialEditingLibrary
for path in (
    '/Game/Brigantine/Material/Sail/MI_Sail_2-wp',
    '/Game/Brigantine/Material/Sail/MI_Sail_2-wp1',
    '/Game/Brigantine/Material/Root/M_Master',
    '/Game/Brigantine/Material/Root/M_2',
):
    material = unreal.load_asset(path)
    if not material:
        continue
    unreal.log_warning('DC_CLOTH ' + path)
    for name in mel.get_scalar_parameter_names(material):
        value = mel.get_material_instance_scalar_parameter_value(material, name) if isinstance(material, unreal.MaterialInstanceConstant) else '?'
        unreal.log_warning('DC_CLOTH scalar %s=%s' % (name, value))
    for name in mel.get_vector_parameter_names(material):
        value = mel.get_material_instance_vector_parameter_value(material, name) if isinstance(material, unreal.MaterialInstanceConstant) else '?'
        unreal.log_warning('DC_CLOTH vector %s=%s' % (name, value))
    for name in mel.get_texture_parameter_names(material):
        value = mel.get_material_instance_texture_parameter_value(material, name) if isinstance(material, unreal.MaterialInstanceConstant) else None
        unreal.log_warning('DC_CLOTH texture %s=%s' % (name, value.get_path_name() if value else '?'))
