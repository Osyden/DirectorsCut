"""Builds /Game/DC/Materials/M_Splash: rain striking the deck.

Run with the editor closed:  Tools/py.sh Tools/make_splash_material.py
The mesh (DCWeatherFX.cpp) is a few thousand splash points on the deck, each a small quad lying
flat (vertex alpha 0) and two crossed upright cards (vertex alpha 1). Each plays a splash over and
over, alive for a fifth of its cycle: a spike and a little crown of thrown water on the cards, a
point of light and a quick thin ring on the flat quad. The vertex
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
// A drop's splash lives a fraction of a second: most of each cycle the spot is bare deck.
float life = 0.2;
float a = age / life;
float on = step(h1 * 0.999, saturate(Tint.a)) * step(age, life);
float o;
if (VC.a > 0.5)
{
    // The crown: an upright card (two, crossed). A thin shell of water thrown up and outward, which
    // rises, opens and falls back, with a spike in the middle at the first instant.
    float u = abs(UV.x - 0.5) * 2.0;
    float v = UV.y;
    float h = max(sin(saturate(a) * 3.14159), 0.001);
    float open = 0.12 + 0.75 * a;
    float shell = saturate(1.0 - abs(u - open * (0.35 + 0.65 * v / h)) / 0.16);
    float crown = shell * step(v, h) * (1.0 - 0.7 * v / h);
    float spike = saturate(1.0 - u / 0.1) * step(v, h * 0.9) * saturate(1.0 - a * 2.5);
    o = (crown * 0.55 + spike * 0.8) * (1.0 - 0.5 * a);
}
else
{
    // On the deck itself: a point of light at the strike, then a thin ring that widens and is gone.
    float2 p = (UV - 0.5) * 2.0;
    float r = length(p);
    float ring = saturate(1.0 - abs(r - 0.2 - a * 0.7) / 0.07) * (1.0 - a) * (1.0 - a);
    float core = saturate(1.0 - r / 0.2) * saturate(1.0 - a * 4.0);
    o = ring * 0.22 + core * 0.6;
}
return saturate(o * on);
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
vc3 = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1100, 550)
vc = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -850, 550)
mel.connect_material_expressions(vc3, "", vc, "A")
mel.connect_material_expressions(vc3, "A", vc, "B")
tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1100, 750)
tint.set_editor_property("parameter_name", "Tint")
tint.set_editor_property("default_value", unreal.LinearColor(0.6, 0.7, 0.8, 1.0))
tint4 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -850, 750)
mel.connect_material_expressions(tint, "", tint4, "A")
mel.connect_material_expressions(tint, "A", tint4, "B")
rate = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1100, 950)
rate.set_editor_property("parameter_name", "Rate")
rate.set_editor_property("default_value", 1.6)

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
