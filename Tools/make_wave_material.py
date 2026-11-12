"""Builds /Game/DC/Materials/M_Wave: the standing, curling walls of water beside the ship in the maelstrom.

Run with the editor closed:  Tools/py.sh Tools/make_wave_material.py
The mesh is generated in code (DCWaveCurl.cpp). It supplies:
  UV0            x = distance along the wall, y = distance up the face
  vertex colour  R = foam amount (foot and lip), G = thinness (0 thick body .. 1 thin lip the light comes through)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Wave"
mel = unreal.MaterialEditingLibrary

COMMON = """
// Water runs up the face and over the lip.
float2 q = float2(UV.x, UV.y - T * 0.16);
float n = sin(q.x * 7.0 + sin(q.y * 5.0) * 1.5) * sin(q.y * 9.0 + q.x * 2.0);
float fine = sin(q.x * 31.0 + sin(q.y * 11.0) * 2.0) * sin(q.y * 27.0 + q.x * 5.0);
float streak = pow(saturate(sin(q.x * 19.0 + sin(q.y * 2.3) * 2.5) * 0.5 + 0.5), 5.0) * (0.55 + 0.45 * sin(q.y * 5.0 + q.x * 3.0));
float foam = saturate(VC.r * (0.55 + 0.6 * n + 0.5 * fine) + streak * (0.18 + VC.r * 1.2) + saturate(fine - 0.55) * (0.4 + VC.r));
"""
COLOR = COMMON + """
float3 water = lerp(Deep.rgb, Scatter.rgb, saturate(VC.g * 0.45 + n * 0.1)) * (0.85 + 0.25 * n);
return lerp(water, float3(0.82, 0.88, 0.9), foam);
"""
GLOW = COMMON + """
// Light coming through the thin water near the lip.
return Scatter.rgb * VC.g * VC.g * (1.0 - foam) * (0.8 + 0.3 * n) * Scatter.a;
"""
ROUGH = COMMON + """
return lerp(0.06, 0.75, foam);
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("two_sided", True)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)

vc = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -900, 0)
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 200)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -900, 300)
deep = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 400)
deep.set_editor_property("parameter_name", "Deep")
deep.set_editor_property("default_value", unreal.LinearColor(0.004, 0.03, 0.04, 1))
scatter = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 600)
scatter.set_editor_property("parameter_name", "Scatter")
scatter.set_editor_property("default_value", unreal.LinearColor(0.03, 0.3, 0.32, 1))
# Vector parameter pins: RGB is output 0, A is output 4; append them so the Custom node sees a float4.
scatter4 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -700, 600)
mel.connect_material_expressions(scatter, "", scatter4, "A")
mel.connect_material_expressions(scatter, "A", scatter4, "B")


def node(code, out_type, y):
    c = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -400, y)
    c.set_editor_property("code", code)
    c.set_editor_property("output_type", out_type)
    inputs = []
    for n in ["VC", "UV", "T", "Deep", "Scatter"]:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    c.set_editor_property("inputs", inputs)
    mel.connect_material_expressions(vc, "", c, "VC")
    mel.connect_material_expressions(uv, "", c, "UV")
    mel.connect_material_expressions(time, "", c, "T")
    mel.connect_material_expressions(deep, "", c, "Deep")
    mel.connect_material_expressions(scatter4, "", c, "Scatter")
    return c


mel.connect_material_property(node(COLOR, unreal.CustomMaterialOutputType.CMOT_FLOAT3, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(node(GLOW, unreal.CustomMaterialOutputType.CMOT_FLOAT3, 250), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(node(ROUGH, unreal.CustomMaterialOutputType.CMOT_FLOAT1, 500), "", unreal.MaterialProperty.MP_ROUGHNESS)
spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 700)
spec.set_editor_property("r", 0.6)
mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

MASK = COMMON + """
// The lip is torn into ragged fingers instead of ending in a straight edge.
float edge = saturate((VC.b - 0.7) / 0.3);
float rag = 0.5 + 0.5 * sin(UV.x * 37.0 + sin(q.y * 4.0) * 2.0) * sin(UV.x * 11.0 + q.y * 2.0);
return 1.0 - edge * (0.45 + 1.0 * rag);
"""
mel.connect_material_property(node(MASK, unreal.CustomMaterialOutputType.CMOT_FLOAT1, 750), "", unreal.MaterialProperty.MP_OPACITY_MASK)
mel.recompile_material(mat)
ok = unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_Wave saved=%s" % ok)
