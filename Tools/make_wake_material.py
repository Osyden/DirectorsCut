"""Builds /Game/DC/Materials/M_Wake: white water along the hull and the wake astern.

Run with the editor closed:  Tools/py.sh Tools/make_wake_material.py
The mesh (DCWake.cpp) is a ribbon down each side of the ship and out behind the stern. U runs from
the hull (0) outward (1); the vertex alpha is how strong the wash is at that point along the ship.
The foam pattern is the engine's cloud-noise volume read in world space and carried aft, with time
as the third axis so the foam boils as it goes.
  Tint    colour (rgb) and overall amount (a)
  Speed   how fast the pattern runs aft, cm a second
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Wake"
mel = unreal.MaterialEditingLibrary

OPACITY = """
// The engine's noise volume sits in a narrow band near white; stretch it to 0..1.
#define NZ(p) saturate((Texture3DSampleLevel(Shape, ShapeSampler, (p), 0.0).r - 0.835) / 0.125)
float2 q = float2(P.x + T * Speed, P.y);
float big = NZ(float3(q * 0.00042, T * 0.05));
float fine = NZ(float3(q * 0.0017 + 0.31, T * 0.11 + 0.5));
float lace = NZ(float3(q * 0.0052 + 0.67, T * 0.17));
// Thick white against the hull, breaking into lace and then streaks further out.
float edge = saturate(1.0 - UV.x);
float body = pow(edge, 1.8);
float n = big * 0.45 + fine * 0.35 + lace * 0.2;
float foam = saturate((n - lerp(0.7, 0.26, body)) * 2.2);
// The outer edge dies away with no outline; the inner edge lies under the hull.
// Thinner where the foam is old: the water shows through it.
return saturate(foam * (0.25 + 0.6 * body + 0.25 * lace) * pow(edge, 0.7) * VC.a * Tint.a);
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


wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1200, 0)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1200, 150)
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1200, 250)
vc = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1200, 400)
vc4 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -950, 400)
mel.connect_material_expressions(vc, "", vc4, "A")
mel.connect_material_expressions(vc, "A", vc4, "B")
tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1200, 600)
tint.set_editor_property("parameter_name", "Tint")
tint.set_editor_property("default_value", unreal.LinearColor(0.85, 0.92, 0.97, 0.8))
tint4 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -950, 600)
mel.connect_material_expressions(tint, "", tint4, "A")
mel.connect_material_expressions(tint, "A", tint4, "B")
speed = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1200, 800)
speed.set_editor_property("parameter_name", "Speed")
speed.set_editor_property("default_value", 1200.0)
shape = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -1200, 950)
shape.set_editor_property("texture", unreal.load_asset("/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128"))

o = custom(OPACITY, unreal.CustomMaterialOutputType.CMOT_FLOAT1, ["P", "T", "UV", "VC", "Tint", "Speed", "Shape"], -500, 300)
mel.connect_material_expressions(wp, "", o, "P")
mel.connect_material_expressions(time, "", o, "T")
mel.connect_material_expressions(uv, "", o, "UV")
mel.connect_material_expressions(vc4, "", o, "VC")
mel.connect_material_expressions(tint4, "", o, "Tint")
mel.connect_material_expressions(speed, "", o, "Speed")
mel.connect_material_expressions(shape, "", o, "Shape")
mel.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Wake saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
