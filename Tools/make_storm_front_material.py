"""Builds /Game/DC/Materials/M_StormFrontVdb: the storm front's tapered, twisting throat.

Run with the editor closed: Tools/py.sh Tools/make_storm_front_material.py
The shared M_VdbCloud and purchased textures are untouched. DCFogFront supplies the volume at runtime.
StormTime is ride time in seconds, so seeking/restarting reproduces the same material twist.
For the engine's heterogeneous volumes, playing a bought looping volume (the cloud tunnel). The
texture is not referenced here: the game sets it on an instance, so this asset holds nothing of the
pack's. Unlike the pack's own material it has a floor, so the cloud stands on the sea instead of
passing through it, and light of its own for lightning.
  SparseVolumeTexture  the volume (set by the game)
  Tint     albedo
  Density  scales the volume's density
  Cut      density below this is taken as empty (0 = use the volume as it is)
  Crown    r = height of the ceiling in the volume's units, g = how far it rolls up and down, b = length of a roll (r huge = no ceiling)
  Phase    where along its length the rolls fall
  Floor    r = height of the floor in the volume's own units, g = how far above it the cloud reaches full density
  Glow     light of its own where the volume's second attribute (temperature) is high: the folds
  Flare    light of its own everywhere (lightning)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_StormFrontVdb"
mel = unreal.MaterialEditingLibrary

EXT = """
// Cut takes off the thin haze that fills the volume's whole box, which shows as a box when the cloud is thin.
float d = saturate((A.r - Cut) / max(1.0 - Cut, 0.001)) * Density;
d *= saturate((LP.z - Floor.r) / max(Floor.g, 0.001));
// Crown: nothing above a rolling ceiling, so a long volume's top is hills and saddles, not one level line.
float top = Crown.r + Crown.g * (sin(LP.y / max(Crown.b, 1.0) + Phase) * 0.65 + sin(LP.y / max(Crown.b, 1.0) * 2.7 + Phase * 1.9) * 0.35);
float box = min(min(min(UV.x, UV.y), UV.z), min(min(1-UV.x, 1-UV.y), 1-UV.z));
return d * saturate((top - LP.z) / 14.0) * saturate(box * 40.0);
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    mat = unreal.load_asset(path)
    mel.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_VOLUME)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("used_with_heterogeneous_volumes", True)


def scalar(name, value, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1300, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


def vector(name, value, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1300, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(*value))
    return p


svt = mel.create_material_expression(mat, unreal.MaterialExpressionSparseVolumeTextureSampleParameter, -1300, -500)
svt.set_editor_property("parameter_name", "SparseVolumeTexture")
svt.set_editor_property("sparse_volume_texture", unreal.load_asset("/Engine/EngineResources/EmptySparseVolumeTexture"))
lp = mel.create_material_expression(mat, unreal.MaterialExpressionLocalPosition, -1300, -300)
bound = mel.create_material_expression(mat, unreal.MaterialExpressionObjectLocalBounds, -1800, -300)
clock = scalar("StormTime", 0.0, -800)
warp = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -1500, -700)
warp.set_editor_property("code", """
float3 uv = (LP - BMin) / BSize;
float2 p = (uv.xz - 0.5) * 2.0;
float depth = saturate(uv.y);
// A broad lip coils into a narrower throat. Twist the existing billows, not just an opaque ring.
float taper = lerp(1.0, 0.65, smoothstep(0.0, 1.0, depth));
// Broad helical folds make the lip curl into the throat instead of reading as a smooth arch.
taper *= 1.0 + 0.14 * sin(atan2(p.y, p.x) * 3.0 + depth * 14.0 - T * 0.12);
float angle = depth * 5.5 + length(p) * 0.7 - T * 0.025;
float sn = sin(angle), cs = cos(angle);
p = float2(cs*p.x - sn*p.y, sn*p.x + cs*p.y) / taper;
return float3(p.x * 0.5 + 0.5, uv.y, p.y * 0.5 + 0.5);
""")
warp.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
warp_inputs = []
for n in ["LP", "BMin", "BSize", "T"]:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); warp_inputs.append(ci)
warp.set_editor_property("inputs", warp_inputs)
assert mel.connect_material_expressions(lp, "", warp, "LP")
assert mel.connect_material_expressions(bound, "Min", warp, "BMin")
assert mel.connect_material_expressions(bound, "Extents", warp, "BSize")
assert mel.connect_material_expressions(clock, "", warp, "T")
inputs = mel.get_material_expression_input_names(svt)
unreal.log_warning("DC_RESULT SVT inputs: %s" % inputs)
assert mel.connect_material_expressions(warp, "", svt, str(inputs[0]))
tint = vector("Tint", (0.78, 0.83, 0.92, 1.0), 0)
density = scalar("Density", 1.0, 200)
cut = scalar("Cut", 0.0, 250)
crown = vector("Crown", (100000.0, 0.0, 100.0, 0.0), 400)
phase = scalar("Phase", 0.0, 450)
floor = vector("Floor", (-100000.0, 1.0, 0.0, 0.0), 300)
glow = vector("Glow", (0.0, 0.0, 0.0, 0.0), 500)
flare = vector("Flare", (0.0, 0.0, 0.0, 0.0), 700)

e = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 300)
e.set_editor_property("code", EXT)
e.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
names = ["A", "LP", "Density", "Floor", "Cut", "Crown", "Phase", "UV"]
ins = []
for n in names:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
e.set_editor_property("inputs", ins)
unreal.log_warning("DC_RESULT svt outputs: A=%s B=%s" % (mel.connect_material_expressions(svt, "Attributes A", e, "A"), 0))
for node, name in zip([lp, density, floor, cut, crown, phase, warp], names[1:]):
    mel.connect_material_expressions(node, "", e, name)
mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(e, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
# Light of its own: (Glow * temperature + Flare) * extinction.
temp = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -900, -450)
temp.set_editor_property("r", True); temp.set_editor_property("g", False); temp.set_editor_property("b", False); temp.set_editor_property("a", False)
unreal.log_warning("DC_RESULT svt B=%s" % mel.connect_material_expressions(svt, "Attributes B", temp, ""))
gm = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, 550)
mel.connect_material_expressions(glow, "", gm, "A")
mel.connect_material_expressions(temp, "", gm, "B")
ga = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -400, 600)
mel.connect_material_expressions(gm, "", ga, "A")
mel.connect_material_expressions(flare, "", ga, "B")
em = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 700)
mel.connect_material_expressions(ga, "", em, "A")
mel.connect_material_expressions(e, "", em, "B")
mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_StormFrontVdb saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
