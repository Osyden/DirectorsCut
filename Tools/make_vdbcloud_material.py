"""Builds /Game/DC/Materials/M_VdbCloud: storm cloud from an animated sparse volume texture.

Run with the editor closed:  Tools/py.sh Tools/make_vdbcloud_material.py
For the engine's heterogeneous volumes, playing a bought looping volume (the cloud tunnel). The
texture is not referenced here: the game sets it on an instance, so this asset holds nothing of the
pack's. Unlike the pack's own material it has a floor, so the cloud stands on the sea instead of
passing through it, and light of its own for lightning.
  SparseVolumeTexture  the volume (set by the game)
  Tint     albedo
  Density  scales the volume's density
  Floor    r = height of the floor in the volume's own units, g = how far above it the cloud reaches full density
  Glow     light of its own where the volume's second attribute (temperature) is high: the folds
  Flare    light of its own everywhere (lightning)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_VdbCloud"
mel = unreal.MaterialEditingLibrary

EXT = """
float d = A.r * Density;
return d * saturate((LP.z - Floor.r) / max(Floor.g, 0.001));
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


svt = mel.create_material_expression(mat, unreal.MaterialExpressionSparseVolumeTextureSampleParameter, -1300, -500)
svt.set_editor_property("parameter_name", "SparseVolumeTexture")
svt.set_editor_property("sparse_volume_texture", unreal.load_asset("/Engine/EngineResources/EmptySparseVolumeTexture"))
lp = mel.create_material_expression(mat, unreal.MaterialExpressionLocalPosition, -1300, -300)
tint = vector("Tint", (0.78, 0.83, 0.92, 1.0), 0)
density = scalar("Density", 1.0, 200)
floor = vector("Floor", (-100000.0, 1.0, 0.0, 0.0), 300)
glow = vector("Glow", (0.0, 0.0, 0.0, 0.0), 500)
flare = vector("Flare", (0.0, 0.0, 0.0, 0.0), 700)

e = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 300)
e.set_editor_property("code", EXT)
e.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
names = ["A", "LP", "Density", "Floor"]
ins = []
for n in names:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
e.set_editor_property("inputs", ins)
unreal.log_warning("DC_RESULT svt outputs: A=%s B=%s" % (mel.connect_material_expressions(svt, "Attributes A", e, "A"), 0))
for node, name in zip([lp, density, floor], names[1:]):
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
unreal.log_warning("DC_RESULT M_VdbCloud saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
