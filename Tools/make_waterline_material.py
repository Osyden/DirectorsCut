"""Builds /Game/DC/Materials/M_Waterline: the wash line where stone stands in the sea.

Run with the editor closed:  Tools/py.sh Tools/make_waterline_material.py
Used as an overlay on every ruin piece (DCRuins.cpp). It only shows near sea level: a band of foam
that laps up and down the stone, and above it a darker, wet zone that fades out a few metres up.
  Sea    x = sea level Z (cm), y = height of the wet zone (cm), z = foam band thickness (cm), w = lap height (cm)
  Foam   colour (rgb) and strength (a)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Waterline"
mel = unreal.MaterialEditingLibrary

OPACITY = """
// The sea rises and falls against the stone, not in step everywhere.
float lap = Sea.w * (0.6 * sin(T * 1.1 + P.x * 0.0031 + P.y * 0.0023) + 0.4 * sin(T * 1.9 - P.x * 0.0087 + P.y * 0.0061));
float h = P.z - Sea.x - lap;
// Broken edge to the foam so it is not a ruled line.
float n = 0.5 + 0.25 * sin(P.x * 0.021 + T * 0.7) + 0.25 * sin(P.y * 0.017 - P.z * 0.03 - T * 0.9);
float foam = saturate(1.0 - abs(h - Sea.z * 0.7) / (Sea.z * (0.6 + 0.8 * n))) * step(-Sea.z, h);
float wet = saturate(1.0 - h / Sea.y) * step(0.0, h);
return float2(saturate(foam * Foam.a), saturate(wet * 0.75));
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)


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


def vparam(name, value, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*value))
    app = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, x + 250, y)
    mel.connect_material_expressions(n, "", app, "A")
    mel.connect_material_expressions(n, "A", app, "B")
    return n, app


wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1200, 0)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1200, 200)
sea, sea4 = vparam("Sea", (230.0, 800.0, 170.0, 70.0), -1200, 400)
foam, foam4 = vparam("Foam", (0.5, 0.54, 0.58, 0.85), -1200, 650)

o = custom(OPACITY, unreal.CustomMaterialOutputType.CMOT_FLOAT2, ["P", "T", "Sea", "Foam"], -600, 200)
mel.connect_material_expressions(wp, "", o, "P")
mel.connect_material_expressions(time, "", o, "T")
mel.connect_material_expressions(sea4, "", o, "Sea")
mel.connect_material_expressions(foam4, "", o, "Foam")

fm = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -300, 150)
fm.set_editor_property("r", True); fm.set_editor_property("g", False); fm.set_editor_property("b", False); fm.set_editor_property("a", False)
wm = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -300, 300)
wm.set_editor_property("r", False); wm.set_editor_property("g", True); wm.set_editor_property("b", False); wm.set_editor_property("a", False)
mel.connect_material_expressions(o, "", fm, "")
mel.connect_material_expressions(o, "", wm, "")

# Colour: foam where there is foam, near black (a wet stain) elsewhere.
dark = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 0)
dark.set_editor_property("constant", unreal.LinearColor(0.012, 0.014, 0.016, 1.0))
col = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 0, 0)
mel.connect_material_expressions(dark, "", col, "A")
mel.connect_material_expressions(foam, "", col, "B")
mel.connect_material_expressions(fm, "", col, "Alpha")
mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)

op = mel.create_material_expression(mat, unreal.MaterialExpressionMax, 0, 250)
mel.connect_material_expressions(fm, "", op, "A")
mel.connect_material_expressions(wm, "", op, "B")
mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)

# Wet stone and foam are both smooth-ish.
rough = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 0, 450)
c1 = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 450)
c1.set_editor_property("r", 0.12)
c2 = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 520)
c2.set_editor_property("r", 0.7)
mel.connect_material_expressions(c1, "", rough, "A")
mel.connect_material_expressions(c2, "", rough, "B")
mel.connect_material_expressions(fm, "", rough, "Alpha")
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Waterline saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
