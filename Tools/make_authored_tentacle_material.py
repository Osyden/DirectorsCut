"""Mask non-limb material slots on the isolated authored Kraken skin. No vendor edits."""
import unreal

path='/Game/DC/Materials/M_BossLimbHidden'
mel=unreal.MaterialEditingLibrary
material=unreal.load_asset(path)
if not material:
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_BossLimbHidden','/Game/DC/Materials',unreal.Material,unreal.MaterialFactoryNew())
if not material:
    raise RuntimeError('Could not create authored-limb body mask')
mel.delete_all_material_expressions(material)
material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided',False)
material.set_editor_property('disable_depth_test',False)
zero=mel.create_material_expression(material,unreal.MaterialExpressionConstant)
zero.set_editor_property('r',0.)
assert mel.connect_material_property(zero,'',unreal.MaterialProperty.MP_OPACITY_MASK)
mel.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
mel.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False)
unreal.log_warning('DC_RESULT authored Kraken limb body mask saved; original mesh/materials untouched')
