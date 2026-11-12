"""Repairs the Claw Kaiju import (Content/Fab/Claw_Kaiju_Monster, a Fab asset that is not in the repo).

The Fab import brought the creature's seven-tile (UDIM) texture sets in as virtual textures, of
which only the first tile showed in game, left normal and roughness as 1x1 placeholders, and put
all seven material slots on an untextured legacy material. This imports every tile of every map
from the Fab download cache as an ordinary texture (capped at 2K), builds one material that picks
the tile from the mesh's UVs, and assigns it to every slot.
Run with the editor closed:  Tools/py.sh Tools/fix_kaiju.py 40
"""
import os
import unreal

D = "/Game/Fab/Claw_Kaiju_Monster"
SRC = os.path.join(os.environ["LOCALAPPDATA"], "Temp", "FabLibrary", "450a7af0-6025-419b-9e68-60e4b177d6a0",
                   "clawkaiju_fbx_animation_extracted", "ClawKaiju_FBX-animation", "textures")
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tc = unreal.TextureCompressionSettings
st = unreal.MaterialSamplerType
P = unreal.MaterialProperty


def log(s):
    unreal.log_warning("DC_RESULT " + s)


# kind: (file extension, compression, sRGB, sampler type, size cap)
KINDS = {
    "BaseColor": ("png", tc.TC_DEFAULT, True, st.SAMPLERTYPE_COLOR, 2048),
    "Normal": ("png", tc.TC_NORMALMAP, False, st.SAMPLERTYPE_NORMAL, 2048),
    "Roughness": ("png", tc.TC_MASKS, False, st.SAMPLERTYPE_MASKS, 2048),
    "Metallic": ("png", tc.TC_MASKS, False, st.SAMPLERTYPE_MASKS, 1024),
    "AO": ("png", tc.TC_MASKS, False, st.SAMPLERTYPE_MASKS, 1024),
    "Emissive": ("exr", tc.TC_HDR_COMPRESSED, False, st.SAMPLERTYPE_LINEAR_COLOR, 1024),
}
tiles = {}
for kind, (ext, comp, srgb, sampler, cap) in KINDS.items():
    tiles[kind] = []
    for i in range(7):
        name = "ClawKaiju_%s_%d" % (kind, 1001 + i)
        apath = "%s/Tiles/%s" % (D, name)
        if not eal.does_asset_exist(apath):
            task = unreal.AssetImportTask()
            task.filename = os.path.join(SRC, "ClawKaiju_%s.%d.%s" % (kind, 1001 + i, ext))
            task.destination_path = D + "/Tiles"
            task.destination_name = name
            task.automated = True
            task.replace_existing = True
            task.save = False
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        t = unreal.load_asset(apath)
        if not t:
            log("FAILED to import %s" % name)
            continue
        t.set_editor_property("compression_settings", comp)
        t.set_editor_property("srgb", srgb)
        t.set_editor_property("max_texture_size", cap)
        t.set_editor_property("virtual_texture_streaming", False)
        eal.save_asset(apath, only_if_is_dirty=False)
        tiles[kind].append(t)
    log("%s tiles: %d" % (kind, len(tiles[kind])))

path = D + "/M_ClawKaiju"
if eal.does_asset_exist(path):
    eal.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_ClawKaiju", D, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1900, 300)


def tiled(kind, y):
    sampler = KINDS[kind][3]
    code = "float t = floor(UV.x);\nreturn " + " + ".join("S%d * saturate(1.0 - abs(t - %d.0))" % (i, i) for i in range(len(tiles[kind]))) + ";"
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -1000, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for n in ["UV"] + ["S%d" % i for i in range(len(tiles[kind]))]:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    node.set_editor_property("inputs", ins)
    mel.connect_material_expressions(uv, "", node, "UV")
    for i, t in enumerate(tiles[kind]):
        s = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -1500, y + i * 60)
        s.set_editor_property("texture", t)
        s.set_editor_property("sampler_type", sampler)
        s.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        mel.connect_material_expressions(s, "RGB", node, "S%d" % i)
    return node


def red(node, y):
    m = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -750, y)
    m.set_editor_property("r", True); m.set_editor_property("g", False); m.set_editor_property("b", False); m.set_editor_property("a", False)
    mel.connect_material_expressions(node, "", m, "")
    return m


def scalar(name, value, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -750, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", value)
    return n


def times(a, b, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -450, y)
    mel.connect_material_expressions(a, "", n, "A")
    mel.connect_material_expressions(b, "", n, "B")
    return n


mel.connect_material_property(tiled("BaseColor", 0), "", P.MP_BASE_COLOR)
mel.connect_material_property(tiled("Normal", 500), "", P.MP_NORMAL)
# Wet hide: the creature has just come out of the sea.
mel.connect_material_property(times(red(tiled("Roughness", 1000), 1000), scalar("Wet", 0.6, 1100), 1000), "", P.MP_ROUGHNESS)
mel.connect_material_property(red(tiled("Metallic", 1500), 1500), "", P.MP_METALLIC)
mel.connect_material_property(red(tiled("AO", 2000), 2000), "", P.MP_AMBIENT_OCCLUSION)
mel.connect_material_property(times(tiled("Emissive", 2500), scalar("Glow", 3.0, 2600), 2500), "", P.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
log("material saved=%s" % eal.save_asset(path, only_if_is_dirty=False))

# The mesh keeps its imported slot materials (they could not be reassigned from a script); the game
# puts M_ClawKaiju on every slot when it spawns the creature.
