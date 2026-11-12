"""Builds M_Stone (rocks, ruins, statues) and M_Ghost (spectral ships) in /Game/DC/Materials.

Run with the editor closed:  Tools/py.sh Tools/make_misc_materials.py
"""
import unreal

PKG = "/Game/DC/Materials"
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def new_material(name):
    path = PKG + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    return tools.create_asset(name, PKG, unreal.Material, unreal.MaterialFactoryNew()), path


def vparam(mat, name, value, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*value))
    return n


def custom(mat, code, out_type, names, x, y):
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


def const(mat, v, x, y):
    c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x, y)
    c.set_editor_property("r", v)
    return c


# ---- M_Stone: weathered rock. "Color" tints it; "Glow" lights the cracks (hell channel).
mat, path = new_material("M_Stone")
wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 0)
col = vparam(mat, "Color", (0.08, 0.09, 0.09, 1), -900, 200)
glow = vparam(mat, "Glow", (0, 0, 0, 1), -900, 400)
STONE = """
float3 q = P * 0.004;
float n = sin(q.x * 1.3 + sin(q.z * 2.1)) * sin(q.y * 1.7 + sin(q.x * 1.9)) * sin(q.z * 1.1 + q.y * 0.7);
float strata = sin(P.z * 0.012 + n * 3.0) * 0.5 + 0.5;
float wet = saturate(1.0 - P.z / 700.0);
return Color.rgb * (0.55 + 0.5 * strata + 0.35 * n) * (1.0 - 0.6 * wet);
"""
CRACK = """
float3 q = P * 0.0035;
float c = abs(sin(q.x * 2.3 + sin(q.z * 3.1 + q.y) * 1.7) * sin(q.z * 1.9 + sin(q.y * 2.7) * 1.3));
return Glow.rgb * pow(saturate(1.0 - c * 5.0), 3.0);
"""
base = custom(mat, STONE, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["P", "Color"], -500, 0)
mel.connect_material_expressions(wp, "", base, "P")
mel.connect_material_expressions(col, "", base, "Color")
mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
crack = custom(mat, CRACK, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["P", "Glow"], -500, 300)
mel.connect_material_expressions(wp, "", crack, "P")
mel.connect_material_expressions(glow, "", crack, "Glow")
mel.connect_material_property(crack, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(const(mat, 0.8, -300, 500), "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Stone saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))

# ---- M_Ghost: see-through, glowing at the edges.
mat, path = new_material("M_Ghost")
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
glow = vparam(mat, "Glow", (0.2, 0.9, 0.8, 1), -900, 0)
fres = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -900, 250)
fres.set_editor_property("exponent", 2.0)
fres.set_editor_property("base_reflect_fraction", 0.08)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -900, 450)
wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 550)
GHOST = """
float flicker = 0.75 + 0.25 * sin(T * 3.0 + P.z * 0.004) * sin(T * 1.3 + P.x * 0.002);
return float4(Glow.rgb * (0.25 + 2.5 * F) * flicker, saturate(0.10 + 0.75 * F) * flicker);
"""
g = custom(mat, GHOST, unreal.CustomMaterialOutputType.CMOT_FLOAT4, ["Glow", "F", "T", "P"], -500, 100)
mel.connect_material_expressions(glow, "", g, "Glow")
mel.connect_material_expressions(fres, "", g, "F")
mel.connect_material_expressions(time, "", g, "T")
mel.connect_material_expressions(wp, "", g, "P")
rgb = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -250, 50)
rgb.set_editor_property("r", True); rgb.set_editor_property("g", True); rgb.set_editor_property("b", True); rgb.set_editor_property("a", False)
mel.connect_material_expressions(g, "", rgb, "")
alpha = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -250, 250)
alpha.set_editor_property("r", False); alpha.set_editor_property("g", False); alpha.set_editor_property("b", False); alpha.set_editor_property("a", True)
mel.connect_material_expressions(g, "", alpha, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Ghost saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))

# ---- M_Flame: glowing tongue of fire for a stretched cone. Hot at the root, fading to nothing at the tip.
mat, path = new_material("M_Flame")
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
lp = mel.create_material_expression(mat, unreal.MaterialExpressionLocalPosition, -900, 0)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -900, 200)
wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 300)
FLAME = """
float h = saturate(L.z / 100.0 + 0.5);
float flick = 0.7 + 0.3 * sin(T * 13.0 + P.x * 0.05) * sin(T * 9.0 + P.y * 0.07);
float3 hot = lerp(float3(9.0, 3.2, 0.5), float3(3.0, 0.35, 0.02), h);
return hot * pow(1.0 - h, 1.5) * flick;
"""
f = custom(mat, FLAME, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["L", "T", "P"], -500, 100)
mel.connect_material_expressions(lp, "", f, "L")
mel.connect_material_expressions(time, "", f, "T")
mel.connect_material_expressions(wp, "", f, "P")
mel.connect_material_property(f, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Flame saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))

# ---- M_Glow: plain self-lit colour (kraken eyes). "Color" may be far brighter than 1.
mat, path = new_material("M_Glow")
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
col = vparam(mat, "Color", (3.0, 1.6, 0.2, 1), -500, 0)
fres = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -500, 250)
fres.set_editor_property("exponent", 1.5)
fres.set_editor_property("base_reflect_fraction", 0.0)
# Dark slit pupil feel: brightest in the middle of the eye, falling off toward the rim.
inv = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -300, 250)
mel.connect_material_expressions(fres, "", inv, "")
mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 100)
mel.connect_material_expressions(col, "", mul, "A")
mel.connect_material_expressions(inv, "", mul, "B")
mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Glow saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))

# ---- M_Bolt: flat self-lit colour seen from both sides (lightning ribbons).
mat, path = new_material("M_Bolt")
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
col = vparam(mat, "Color", (60.0, 70.0, 90.0, 1), -400, 0)
mel.connect_material_property(col, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Bolt saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))

# ---- M_Cloud: a soft billow for the fog front. Dense in the middle, fading at the edge, dark at the
# foot and lit at the top, and it can glow from inside when lightning fires ("Flash", 0..1).
mat, path = new_material("M_Cloud")
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
top = vparam(mat, "Top", (0.55, 0.5, 0.48, 1), -900, 0)
bottom = vparam(mat, "Bottom", (0.03, 0.045, 0.06, 1), -900, 200)
flash = vparam(mat, "Flash", (0, 0, 0, 0.9), -900, 400)
fres = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -900, 600)
fres.set_editor_property("exponent", 1.6)
fres.set_editor_property("base_reflect_fraction", 0.0)
wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 750)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -900, 900)
CLOUD = """
float core = saturate(1.0 - F);
float h = saturate(P.z / 45000.0);
// Slow churn so the billows are never still.
float churn = 0.85 + 0.15 * sin(P.x * 0.0004 + T * 0.3) * sin(P.y * 0.0005 - T * 0.23 + P.z * 0.0003);
float3 col = lerp(Bottom.rgb, Top.rgb, h * h * h) * churn + Flash.rgb * core * core;
// Ragged edges and mottled bodies, so a billow never shows as a clean ball.
float n1 = sin(P.x * 0.00021 + sin(P.y * 0.00033) * 2.0) * sin(P.y * 0.00027 + sin(P.z * 0.0004) * 2.0) * sin(P.z * 0.00031 + P.x * 0.0002 + T * 0.05);
float n2 = sin(P.x * 0.0009 + P.z * 0.0007 + T * 0.1) * sin(P.y * 0.0011 - P.z * 0.0005);
col *= 0.78 + 0.3 * n1 + 0.12 * n2;
float alpha = saturate(pow(core, 1.9) * 1.9 * (0.55 + 0.6 * n1 + 0.35 * n2)) * Flash.a;
return float4(col, alpha);
"""
c = custom(mat, CLOUD, unreal.CustomMaterialOutputType.CMOT_FLOAT4, ["F", "P", "T", "Top", "Bottom", "Flash"], -500, 200)
flash4 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -700, 400)
mel.connect_material_expressions(flash, "", flash4, "A")
mel.connect_material_expressions(flash, "A", flash4, "B")
mel.connect_material_expressions(fres, "", c, "F")
mel.connect_material_expressions(wp, "", c, "P")
mel.connect_material_expressions(time, "", c, "T")
mel.connect_material_expressions(top, "", c, "Top")
mel.connect_material_expressions(bottom, "", c, "Bottom")
mel.connect_material_expressions(flash4, "", c, "Flash")
rgb = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -250, 150)
rgb.set_editor_property("r", True); rgb.set_editor_property("g", True); rgb.set_editor_property("b", True); rgb.set_editor_property("a", False)
mel.connect_material_expressions(c, "", rgb, "")
alpha = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -250, 350)
alpha.set_editor_property("r", False); alpha.set_editor_property("g", False); alpha.set_editor_property("b", False); alpha.set_editor_property("a", True)
mel.connect_material_expressions(c, "", alpha, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Cloud saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
