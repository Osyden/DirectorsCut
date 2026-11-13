"""Builds /Game/DC/Materials/M_FogVolume: a bank of fog that is really there in the air.

Run with the editor closed:  Tools/py.sh Tools/make_fogvolume_material.py
A volume-domain material: the engine's volumetric fog fills the inside of the mesh it is on (a
sphere, scaled into whatever ellipsoid is wanted; DCFogBank.cpp) with fog of this density, and
lights it like the rest of the air: sun shafts, lightning, the ship's lanterns, shadows of ruins.
The density falls off toward the skin of the ellipsoid and is eaten away by drifting noise, so a
bank has billows, holes and a ragged edge and changes as it is watched.
  Tint     albedo (rgb)
  Density  extinction at the heart of the bank
  Size     cm across of the biggest billows
  Erode    0 = a smooth blob .. 1 = torn to rags
  Drift    cm a second the billows travel (world axes), w unused
  Stretch  how much the billows are drawn out along each world axis (1,1,1 = round)
  Wisp     above 0, only noise over this level is fog: streaks with clear air between (deck wisps)
  Glow     light the fog gives off per unit of density (set every frame: sky light and lightning)
  Seed     offsets the pattern so two banks do not match
  Radii    half the length of the ellipsoid along each world axis, cm (the mesh is scaled to match)
  Floor    world height (cm) below which there is no fog (the sea), w = cm over which it fades in
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_FogVolume"
mel = unreal.MaterialEditingLibrary

EXT = """
// The engine's noise volume sits in a narrow band near white; stretch it to 0..1.
#define NZ(p) saturate((Texture3DSampleLevel(Shape, ShapeSampler, (p), 0.0).r - 0.835) / 0.125)
float3 lp = (WP - Centre) / Radii.xyz;        // the bank is an ellipsoid standing square to the world
float r = length(lp);
float body = saturate(1.0 - r);
body = body * body * (3.0 - 2.0 * body);
float3 q = (WP - Drift.xyz * T) / (Size * Stretch.xyz) + Seed * 0.173;
float n = NZ(q) * 0.62 + NZ(q * 2.63 + 0.37) * 0.38;
// Noise eats the bank from its skin inward: the heart stays thick, the edge is ragged.
float d = saturate(body * (1.0 + 1.6 * Erode) - n * 1.9 * Erode);
// Sculpted, not smeared: the density climbs quickly from nothing to full, so a billow has an edge.
d = smoothstep(0.0, 0.45, d);
// Wisps: only the crests of the noise are fog at all, the rest is clear air between streaks.
if (Wisp > 0.0) { float w = saturate((n - Wisp) / (1.0 - Wisp)); d = body * w * sqrt(w); }
float lift = saturate((WP.z - Floor.x) / max(Floor.w, 1.0));
return Density * d * lift;
"""

# How much of its light a point of the bank gives off. A real cloud is bright where it faces the sky
# and dark in its folds and underneath; the fog grid is too coarse to shadow a bank against itself, so
# the modelling is put in here: tops and outer billows bright, the hollows between them and the
# underside dim. This is what makes a bank read as a solid rolling form instead of an even glow.
SHADE = """
#define NZ(p) saturate((Texture3DSampleLevel(Shape, ShapeSampler, (p), 0.0).r - 0.835) / 0.125)
float3 lp = (WP - Centre) / Radii.xyz;
float3 q = (WP - Drift.xyz * T) / (Size * Stretch.xyz) + Seed * 0.173;
float n = NZ(q) * 0.62 + NZ(q * 2.63 + 0.37) * 0.38;
// The same noise a little higher up: where there is more cloud above than here, this point is in a fold.
float3 qa = q + float3(0.0, 0.0, 0.22 / Stretch.z);
float above = NZ(qa) * 0.62 + NZ(qa * 2.63 + 0.37) * 0.38;
float fold = saturate(0.5 + (above - n) * 2.2);          // 1 = open to the sky, 0 = under an overhang
float height = saturate(lp.z * 0.5 + 0.5);
return lerp(0.18, 1.0, fold) * lerp(0.45, 1.15, height);
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_VOLUME)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
# Also usable on the engine's heterogeneous volumes (ray-marched, with their own shadows), not only in the fog grid.
mat.set_editor_property("used_with_heterogeneous_volumes", True)


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


def scalar(name, value, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1300, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


def vector(name, value, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1300, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(*value))
    a = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -1050, y)
    mel.connect_material_expressions(p, "", a, "A")
    mel.connect_material_expressions(p, "A", a, "B")
    return p, a


wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1300, -300)
centre = mel.create_material_expression(mat, unreal.MaterialExpressionObjectPositionWS, -1050, -300)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1300, -150)
tint, _ = vector("Tint", (0.8, 0.86, 0.95, 1.0), 0)
density = scalar("Density", 0.02, 200)
size = scalar("Size", 6000.0, 300)
erode = scalar("Erode", 0.6, 400)
_, drift = vector("Drift", (-300.0, 120.0, 0.0, 0.0), 500)
_, stretch = vector("Stretch", (1.0, 1.0, 1.0, 0.0), 700)
seed = scalar("Seed", 0.0, 900)
_, floor = vector("Floor", (-100000.0, 0.0, 0.0, 1.0), 1000)
_, radii = vector("Radii", (5000.0, 5000.0, 2000.0, 0.0), 1100)
shape = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -1300, 1200)
shape.set_editor_property("texture", unreal.load_asset("/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128"))

wisp = scalar("Wisp", 0.0, 1300)
glow, _ = vector("Glow", (0.0, 0.0, 0.0, 0.0), 1400)
e = custom(EXT, unreal.CustomMaterialOutputType.CMOT_FLOAT1, ["WP", "Centre", "Radii", "T", "Density", "Size", "Erode", "Drift", "Stretch", "Seed", "Floor", "Wisp", "Shape"], -500, 300)
for node, name in [(wp, "WP"), (centre, "Centre"), (radii, "Radii"), (time, "T"), (density, "Density"), (size, "Size"), (erode, "Erode"), (drift, "Drift"), (stretch, "Stretch"), (seed, "Seed"), (floor, "Floor"), (wisp, "Wisp"), (shape, "Shape")]:
    mel.connect_material_expressions(node, "", e, name)
# In the volume domain, base colour is the albedo and subsurface colour the extinction.
mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(e, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
# Its own light: thick fog glows with the light of the whole sky, and flares when lightning is inside it.
sh = custom(SHADE, unreal.CustomMaterialOutputType.CMOT_FLOAT1, ["WP", "Centre", "Radii", "T", "Size", "Drift", "Stretch", "Seed", "Shape"], -500, 800)
for node, name in [(wp, "WP"), (centre, "Centre"), (radii, "Radii"), (time, "T"), (size, "Size"), (drift, "Drift"), (stretch, "Stretch"), (seed, "Seed"), (shape, "Shape")]:
    mel.connect_material_expressions(node, "", sh, name)
lit = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 600)
mel.connect_material_expressions(glow, "", lit, "A")
mel.connect_material_expressions(sh, "", lit, "B")
em = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 600)
mel.connect_material_expressions(lit, "", em, "A")
mel.connect_material_expressions(e, "", em, "B")
mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_FogVolume saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
