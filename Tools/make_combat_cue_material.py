"""Generate the small depth-tested combat cue material; no vendor edits.

Root-owned editor workflow:
 UnrealEditor-Cmd project.uproject -run=pythonscript -script=<this file>
 -AllowCommandletRendering -unattended -nosplash
Then cook before packaged visual review. Safe to rerun.
"""
import unreal

path='/Game/DC/Materials/M_CombatCue'
mel=unreal.MaterialEditingLibrary
material=unreal.load_asset(path)
if not material:
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_CombatCue','/Game/DC/Materials',unreal.Material,unreal.MaterialFactoryNew())
if not material:
    raise RuntimeError('Could not create combat cue material')
mel.delete_all_material_expressions(material)
material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_ADDITIVE)
material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided',True)
material.set_editor_property('disable_depth_test',False)
vertex=mel.create_material_expression(material,unreal.MaterialExpressionVertexColor)
glow=mel.create_material_expression(material,unreal.MaterialExpressionScalarParameter)
glow.set_editor_property('parameter_name','Glow')
glow.set_editor_property('default_value',.8)
multiply=mel.create_material_expression(material,unreal.MaterialExpressionMultiply)
assert mel.connect_material_expressions(vertex,'',multiply,'A')
assert mel.connect_material_expressions(glow,'',multiply,'B')
assert mel.connect_material_property(multiply,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert mel.connect_material_property(vertex,'A',unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False)
unreal.log_warning('DC_RESULT combat cue material generated: depth-tested, additive, no texture dependencies')
