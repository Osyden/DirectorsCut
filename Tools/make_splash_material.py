"""Builds /Game/DC/Materials/M_Splash: rain striking the deck.

Run with the editor closed:  Tools/py.sh Tools/make_splash_material.py
The mesh (DCWeatherFX.cpp) is a few thousand small quads lying on the deck. Each quad plays a
splash over and over: a bright point of impact, then a ring that widens and fades. The vertex
colour gives each quad its own timing; every cycle the quad shifts a little, so drops do not
keep landing on the same spots.
  Tint    colour (rgb) and amount (a): at low amounts only some of the quads are active
  Rate    splashes per second for one quad
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Splash"
mel = unreal.MaterialEditingLibrary

COMMON = """
float cyc = T * Rate * (0.7 + 0.6 * VC.r) + VC.g * 17.0;
float age = frac(cyc);
float id = floor(cyc);
float h1 = frac(sin(id * 12.9898 + VC.b * 78.233) * 43758.5453);
float h2 = frac(sin(id * 39.3468 + VC.b * 11.135) * 24634.6345);
"""

WPO = COMMON + """
return float3((h1 - 0.5) * 36.0, (h2 - 0.5) * 36.0, 0.0);
"""

OPACITY = COMMON + """
float on = step(h1 * 0.999, saturate(Tint.a));
float2 p = (UV - 0.5) * 2.0;
float r = length(p);
float fade = (1.0 - age) * (1.0 - age);
float ring = saturate(1.0 - abs(r - 0.15 - age * 0.75) / 0.06) * fade;
float core = saturate(1.0 - r / 0.16) * saturate(1.0 - age * 6.0);
return saturate((ring * 0.28 + core * 0.7) * on);
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)


def custom(code, out_type, names, x, y):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", out_type)
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    node.set_editor_property("inputs", inputs)
    return node


time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1100, 300)
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1100, 400)
vc = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1100, 550)
tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1100, 750)
tint.set_editor_property("parameter_name", "Tint")
tint.set_editor_property("default_value", unreal.LinearColor(0.6, 0.7, 0.8, 1.0))
tint4 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -850, 750)
mel.connect_material_expressions(tint, "", tint4, "A")
mel.connect_material_expressions(tint, "A", tint4, "B")
rate = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1100, 950)
rate.set_editor_property("parameter_name", "Rate")
rate.set_editor_property("default_value", 2.2)

w = custom(WPO, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["T", "VC", "Rate"], -500, 0)
mel.connect_material_expressions(time, "", w, "T")
mel.connect_material_expressions(vc, "", w, "VC")
mel.connect_material_expressions(rate, "", w, "Rate")
mel.connect_material_property(w, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

o = custom(OPACITY, unreal.CustomMaterialOutputType.CMOT_FLOAT1, ["T", "UV", "VC", "Tint", "Rate"], -500, 400)
mel.connect_material_expressions(time, "", o, "T")
mel.connect_material_expressions(uv, "", o, "UV")
mel.connect_material_expressions(vc, "", o, "VC")
mel.connect_material_expressions(tint4, "", o, "Tint")
mel.connect_material_expressions(rate, "", o, "Rate")
mel.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Splash saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
