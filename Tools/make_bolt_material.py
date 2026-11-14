"""Builds /Game/DC/Materials/M_Bolt: a lightning bolt's white-hot core.

Run with the editor closed:  Tools/py.sh Tools/make_bolt_material.py
Unlit and additive, and the fog is not laid over it: a bolt half a kilometre off in thick mist is
still seen, as a real one is, where an ordinary glowing mesh is fogged out to nothing.
  Color   its light (rgb, well above 1)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Bolt"
mel = unreal.MaterialEditingLibrary
path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
mat.set_editor_property("use_translucency_vertex_fog", False)
mat.set_editor_property("used_with_instanced_static_meshes", True)
p = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
p.set_editor_property("parameter_name", "Color")
p.set_editor_property("default_value", unreal.LinearColor(60.0, 75.0, 110.0, 1.0))
mel.connect_material_property(p, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Bolt saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
