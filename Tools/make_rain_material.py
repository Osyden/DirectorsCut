"""Builds /Game/DC/Materials/M_Rain: camera-wrapped streaks for rain and rising embers.

Run with the editor closed:  Tools/py.sh Tools/make_rain_material.py
The mesh (DCWeatherFX.cpp) is thousands of quads whose four corners all sit at one random point
in a box; this material moves that point, wraps it around the camera and spreads the corners
into a streak that faces the camera.
  Motion  x = fall speed cm/s (negative rises), y = streak length cm, z = width cm, w = wind slant
  Tint    colour (rgb) and overall amount (a)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Rain"
mel = unreal.MaterialEditingLibrary

WPO = """
float3 box = float3(3200.0, 3200.0, 2600.0);
float3 s = Seed;
s.z -= T * Motion.x * (0.7 + 0.6 * VC.r);
s.x -= T * Motion.w * 420.0;
s.y += sin(T * 0.7 + VC.g * 40.0) * 60.0 * VC.b;
float3 rel = frac((s - Cam) / box) * box - box * 0.5;
float3 center = Cam + rel;
float3 dir = normalize(float3(-Motion.w * 0.4, 0.0, -sign(Motion.x)));
float3 toCam = normalize(Cam - center);
float3 side = normalize(cross(dir, toCam));
float3 pos = center + dir * (UV.y - 0.5) * Motion.y + side * (UV.x - 0.5) * Motion.z;
return pos - Seed;
"""

OPACITY = """
float across = saturate(1.0 - abs(UV.x - 0.5) * 2.0);
float along = saturate(1.0 - abs(UV.y - 0.5) * 2.0);
return saturate(across * sqrt(along) * (0.35 + 0.65 * VC.g) * Tint.a);
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


def vparam(name, value, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*value))
    app = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, x + 250, y)
    mel.connect_material_expressions(n, "", app, "A")
    mel.connect_material_expressions(n, "A", app, "B")
    return n, app


wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1100, 0)
wp.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
cam = mel.create_material_expression(mat, unreal.MaterialExpressionCameraPositionWS, -1100, 150)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1100, 300)
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1100, 400)
vc = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1100, 550)
motion, motion4 = vparam("Motion", (2600.0, 90.0, 1.6, 0.3), -1100, 750)
tint, tint4 = vparam("Tint", (0.5, 0.6, 0.7, 0.35), -1100, 950)

w = custom(WPO, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["Seed", "Cam", "T", "UV", "VC", "Motion"], -500, 0)
mel.connect_material_expressions(wp, "", w, "Seed")
mel.connect_material_expressions(cam, "", w, "Cam")
mel.connect_material_expressions(time, "", w, "T")
mel.connect_material_expressions(uv, "", w, "UV")
mel.connect_material_expressions(vc, "", w, "VC")
mel.connect_material_expressions(motion4, "", w, "Motion")
mel.connect_material_property(w, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

o = custom(OPACITY, unreal.CustomMaterialOutputType.CMOT_FLOAT1, ["UV", "VC", "Tint"], -500, 400)
mel.connect_material_expressions(uv, "", o, "UV")
mel.connect_material_expressions(vc, "", o, "VC")
mel.connect_material_expressions(tint4, "", o, "Tint")
mel.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_Rain saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
