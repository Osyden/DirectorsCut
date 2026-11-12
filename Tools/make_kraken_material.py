"""Builds /Game/DC/Materials/M_Kraken: wet dark skin with a pale, suckered underside.

Run with the editor closed:  Tools/py.sh Tools/make_kraken_material.py
The tentacle mesh is generated in code (DCKraken.cpp). It supplies:
  UV0.x          distance along the arm (1 unit = 4 m)
  vertex colour  R = belly amount, G = position across the belly (0.5 = centre line), B = 0 at base .. 1 at tip
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Kraken"
mel = unreal.MaterialEditingLibrary

COLOR = """
// Two rows of suckers along the belly, shrinking toward the tip.
float rows = abs(VC.g - 0.5) * 2.0;            // 0 at centre line, 1 at belly edge
float along = frac(UV.x * 3.2 + step(0.5, VC.g) * 0.5) - 0.5;
float across = (rows - 0.42) * 1.6;
float d = length(float2(along, across));
float size = lerp(0.30, 0.18, VC.b);
float ring = smoothstep(size, size - 0.05, d);
float hole = smoothstep(size * 0.45, size * 0.45 - 0.04, d);
// Mottled skin from cheap layered sines.
float m = sin(UV.x * 9.0 + sin(UV.y * 14.0) * 2.0) * sin(UV.y * 23.0 + UV.x * 3.0);
float3 skin = Skin.rgb * (0.75 + 0.5 * m);
float3 belly = lerp(Belly.rgb, Belly.rgb * 1.6 + 0.05, ring);
belly = lerp(belly, Belly.rgb * 0.25, hole);
return lerp(skin, belly, VC.r);
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("two_sided", True)

vc = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -900, 0)
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 200)
skin = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 350)
skin.set_editor_property("parameter_name", "Skin")
skin.set_editor_property("default_value", unreal.LinearColor(0.012, 0.035, 0.03, 1))
belly = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 550)
belly.set_editor_property("parameter_name", "Belly")
belly.set_editor_property("default_value", unreal.LinearColor(0.30, 0.13, 0.12, 1))

node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 100)
node.set_editor_property("code", COLOR)
node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for n in ["VC", "UV", "Skin", "Belly"]:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
node.set_editor_property("inputs", inputs)
mel.connect_material_expressions(vc, "", node, "VC")
mel.connect_material_expressions(uv, "", node, "UV")
mel.connect_material_expressions(skin, "", node, "Skin")
mel.connect_material_expressions(belly, "", node, "Belly")
mel.connect_material_property(node, "", unreal.MaterialProperty.MP_BASE_COLOR)

rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 400)
rough.set_editor_property("r", 0.18)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 500)
spec.set_editor_property("r", 0.6)
mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

mel.recompile_material(mat)
ok = unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_Kraken saved=%s" % ok)
