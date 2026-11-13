"""Builds /Game/DC/Materials/M_HeteroCloud: cloud for the engine's heterogeneous volumes.

Run with the editor closed:  Tools/py.sh Tools/make_heterocloud_material.py
A heterogeneous volume is ray-marched at its own resolution and shadows itself against the sun, so
unlike the fog grid it can be sharp, self-shadowed and seen at any distance. This is the same
ellipsoid of drifting noise as M_FogVolume, but worked in the volume's own units, which is where the
engine evaluates these materials: positions run from minus to plus half the component's
VolumeResolution, whatever its size in the world, and extinction is per one of those units too.
Radii is in those units; Unit says how many cm of world one of them is along each axis, so that
Size and Drift can be given in cm and the billows keep their shape in a box that is not a cube.
Density is per unit, not per cm.
  Tint     albedo
  Density  extinction at the heart
  Centre   leave at zero
  Radii    half the component's VolumeResolution
  Size     cm across of the biggest billows
  Erode    0 = smooth .. 1 = torn
  Drift    cm a second the billows travel
  Unit     cm of world to one unit of the volume, per axis (the component's scale)
  Seed     offsets the pattern
  Glow     light of its own per unit of density (lightning, or a floor under the sun's light)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_HeteroCloud"
mel = unreal.MaterialEditingLibrary

EXT = """
#define NZ(p) saturate((Texture3DSampleLevel(Shape, ShapeSampler, (p), 0.0).r - 0.835) / 0.125)
float3 lp = (LP - Centre.xyz) / Radii.xyz;
float r = length(lp);
float body = saturate(1.0 - r);
body = body * body * (3.0 - 2.0 * body);
float3 q = (LP * Unit.xyz - Drift.xyz * T) / Size + Seed * 0.173;
float n = NZ(q) * 0.6 + NZ(q * 2.63 + 0.37) * 0.28 + NZ(q * 6.9 + 0.71) * 0.12;
float d = saturate(body * (1.0 + 1.6 * Erode) - n * 1.9 * Erode);
d = smoothstep(0.0, 0.6, d);
// Nothing below the bottom fifth of the box: a flat base, as a bank lying on the sea has.
return Density * d * saturate((lp.z + 0.8) * 5.0);
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
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


lp = mel.create_material_expression(mat, unreal.MaterialExpressionLocalPosition, -1300, -300)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1300, -150)
tint = vector("Tint", (0.85, 0.88, 0.95, 1.0), 0)
density = scalar("Density", 0.02, 200)
half = vector("Centre", (0.0, 0.0, 0.0, 0.0), 300)
voxel = vector("Radii", (1000.0, 1000.0, 1000.0, 0.0), 400)
size = scalar("Size", 1500.0, 600)
erode = scalar("Erode", 0.6, 700)
drift = vector("Drift", (-200.0, 60.0, 0.0, 0.0), 800)
seed = scalar("Seed", 0.0, 1000)
unit = vector("Unit", (1.0, 1.0, 1.0, 0.0), 1200)
glow = vector("Glow", (0.0, 0.0, 0.0, 0.0), 1100)
shape = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -1300, 1300)
shape.set_editor_property("texture", unreal.load_asset("/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128"))

e = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 300)
e.set_editor_property("code", EXT)
e.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
names = ["LP", "T", "Density", "Centre", "Radii", "Size", "Erode", "Drift", "Seed", "Unit", "Shape"]
ins = []
for n in names:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
e.set_editor_property("inputs", ins)
for node, name in zip([lp, time, density, half, voxel, size, erode, drift, seed, unit, shape], names):
    mel.connect_material_expressions(node, "", e, name)
mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(e, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
em = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 700)
mel.connect_material_expressions(glow, "", em, "A")
mel.connect_material_expressions(e, "", em, "B")
mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_HeteroCloud saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
