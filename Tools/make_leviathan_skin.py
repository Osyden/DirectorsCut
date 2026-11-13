"""Build the private Claw Kaiju skin without modifying the Fab import.

Run with the clone editor closed:
  UnrealEditor-Cmd.exe <clone>/DirectorsCut.uproject -run=pythonscript \
    -script=<clone>/Tools/make_leviathan_skin.py -unattended -nullrhi

DC_KAIJU_TEXTURES may point to the purchased pack's extracted textures folder.
Otherwise the script searches the local Fab download cache. No purchased pixels
belong in Git: Content/DC/Maelstrom/LeviathanSkin/ must already be ignored.

The previous repair imported *.1001.png directly. Interchange collected all seven
UDIM source images again for every purported tile, then VT was disabled. Each
ordinary texture therefore contained the wrong image/block. Here each tile is
copied to a name without a UDIM number before import, and its single source file
is checked. The original source, mesh, and Fab assets remain untouched.
"""
import glob
import os
from pathlib import Path
import shutil
import subprocess

import unreal


DEST = "/Game/DC/Maelstrom/LeviathanSkin"
PROJECT = Path(unreal.Paths.project_dir()).resolve()
OUTPUT = PROJECT / "Content/DC/Maelstrom/LeviathanSkin"
STAGE = PROJECT / "Saved/LeviathanSkinImport"
SCHEMA = "single-source-tile-v1"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TC = unreal.TextureCompressionSettings
ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty


def report(message):
    unreal.log_warning("DC_RESULT LeviathanSkin " + message)


def source_folder():
    explicit = os.environ.get("DC_KAIJU_TEXTURES")
    if explicit:
        candidates = [Path(explicit)]
    else:
        local = Path(os.environ.get("LOCALAPPDATA", ""))
        pattern = str(local / "Temp/FabLibrary/**/ClawKaiju_BaseColor.1001.png")
        candidates = [Path(p).parent for p in glob.glob(pattern, recursive=True)]
    for folder in candidates:
        if all((folder / ("ClawKaiju_%s.%d.%s" % (kind, tile, config[0]))).is_file()
               for kind, config in KINDS.items() for tile in range(1001, 1008)):
            return folder
    raise RuntimeError("Purchased Claw Kaiju texture set not found. Set DC_KAIJU_TEXTURES to its extracted textures folder.")


KINDS = {
    "BaseColor": ("png", TC.TC_DEFAULT, True, ST.SAMPLERTYPE_COLOR, 2048),
    "Normal": ("png", TC.TC_NORMALMAP, False, ST.SAMPLERTYPE_NORMAL, 2048),
    "Roughness": ("png", TC.TC_MASKS, False, ST.SAMPLERTYPE_MASKS, 1024),
    "Metallic": ("png", TC.TC_MASKS, False, ST.SAMPLERTYPE_MASKS, 1024),
    "AO": ("png", TC.TC_MASKS, False, ST.SAMPLERTYPE_MASKS, 1024),
    "Emissive": ("exr", TC.TC_HDR_COMPRESSED, False, ST.SAMPLERTYPE_LINEAR_COLOR, 1024),
}

# Fail before creating licensed derivatives if somebody runs this in an unsafe checkout.
ignored = subprocess.run(["git", "-C", str(PROJECT), "check-ignore", "--quiet",
                          "Content/DC/Maelstrom/LeviathanSkin/probe.uasset"],
                         capture_output=True)
if ignored.returncode != 0:
    raise RuntimeError("Ignore Content/DC/Maelstrom/LeviathanSkin/ before generating licensed derivatives.")

source = source_folder()
STAGE.mkdir(parents=True, exist_ok=True)
textures = {}
for kind, (ext, compression, srgb, sampler, limit) in KINDS.items():
    textures[kind] = []
    for i in range(7):
        name = "T_Leviathan_%s_Tile%s" % (kind, chr(ord("A") + i))
        asset_path = DEST + "/" + name
        texture = unreal.load_asset(asset_path) if EAL.does_asset_exist(asset_path) else None
        valid = texture and EAL.get_metadata_tag(texture, "DC_SkinSchema") == SCHEMA
        if not valid or os.environ.get("DC_KAIJU_REIMPORT") == "1":
            # Alphabetical tile suffix defeats automatic UDIM sibling discovery.
            staged = STAGE / (name + "." + ext)
            shutil.copy2(source / ("ClawKaiju_%s.%d.%s" % (kind, 1001 + i, ext)), staged)
            task = unreal.AssetImportTask()
            task.filename = str(staged)
            task.destination_path = DEST
            task.destination_name = name
            task.automated = True
            task.replace_existing = True
            task.save = False
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            texture = unreal.load_asset(asset_path)
        if not texture:
            raise RuntimeError("Texture import failed: " + asset_path)
        sources = texture.get_editor_property("asset_import_data").extract_filenames()
        if len(sources) != 1:
            raise RuntimeError("UDIM discovery recurred for %s: %s" % (name, sources))
        texture.set_editor_property("compression_settings", compression)
        texture.set_editor_property("srgb", srgb)
        texture.set_editor_property("max_texture_size", limit)
        texture.set_editor_property("virtual_texture_streaming", False)
        EAL.set_metadata_tag(texture, "DC_SkinSchema", SCHEMA)
        EAL.save_loaded_asset(texture, only_if_is_dirty=False)
        textures[kind].append(texture)
    report("%s: seven independent source tiles verified" % kind)

path = DEST + "/M_LeviathanSkin"
material = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
if material:
    # UE 5.8's bulk deletion mutates its iteration list and can leave old nodes.
    for previous in list(MEL.get_material_expressions(material)):
        MEL.delete_material_expression(material, previous)
    if MEL.get_num_material_expressions(material) != 0:
        raise RuntimeError("Leviathan material graph did not clear completely")
else:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_LeviathanSkin", DEST, unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("used_with_skeletal_mesh", True)
uv = MEL.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1900, 0)


def tile_sample(kind, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, -900, y)
    node.set_editor_property("code", "float tile = floor(UV.x);\nreturn " + " + ".join(
        "S%d * saturate(1.0 - abs(tile - %d.0))" % (i, i) for i in range(7)) + ";")
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for name in ["UV"] + ["S%d" % i for i in range(7)]:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        inputs.append(entry)
    node.set_editor_property("inputs", inputs)
    MEL.connect_material_expressions(uv, "", node, "UV")
    for i, texture in enumerate(textures[kind]):
        sample = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSample, -1500, y + i * 55)
        sample.set_editor_property("texture", texture)
        sample.set_editor_property("sampler_type", KINDS[kind][3])
        sample.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        # Preserve derivatives across integer boundaries. The shared wrap sampler
        # maps each horizontal UDIM to its own 0..1 image, after tile selection.
        MEL.connect_material_expressions(uv, "", sample, "Coordinates")
        MEL.connect_material_expressions(sample, "RGB", node, "S%d" % i)
    return node


def scalar(name, value, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -650, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def code_node(code, inputs, y, output=unreal.CustomMaterialOutputType.CMOT_FLOAT1):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, -300, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output)
    entries = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    node.set_editor_property("inputs", entries)
    for name, source_node in inputs.items():
        MEL.connect_material_expressions(source_node, "", node, name)
    return node


# Keep the authored hide and grooves. Wetness is bounded so the animal does not
# turn into a mirror; the authored normal map remains tangent-space decoded.
base = code_node("return Base * float3(0.78, 0.88, 0.90);", {"Base": tile_sample("BaseColor", 0)}, 0,
                 unreal.CustomMaterialOutputType.CMOT_FLOAT3)
MEL.connect_material_property(base, "", MP.MP_BASE_COLOR)
MEL.connect_material_property(tile_sample("Normal", 500), "", MP.MP_NORMAL)
roughness = code_node("return clamp(lerp(Rough.r, 0.22, saturate(Wetness)), 0.22, 0.85);",
                      {"Rough": tile_sample("Roughness", 1000), "Wetness": scalar("Wetness", 0.40, 1350)}, 1000)
MEL.connect_material_property(roughness, "", MP.MP_ROUGHNESS)
metal = code_node("return saturate(Metal.r);", {"Metal": tile_sample("Metallic", 1500)}, 1500)
MEL.connect_material_property(metal, "", MP.MP_METALLIC)
ao = code_node("return saturate(AO.r);", {"AO": tile_sample("AO", 2000)}, 2000)
MEL.connect_material_property(ao, "", MP.MP_AMBIENT_OCCLUSION)
glow = code_node("return Emissive * Glow;", {"Emissive": tile_sample("Emissive", 2500),
                 "Glow": scalar("Glow", 2.0, 2850)}, 2500, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
MEL.connect_material_property(glow, "", MP.MP_EMISSIVE_COLOR)
MEL.recompile_material(material)
EAL.save_loaded_asset(material, only_if_is_dirty=False)
report("ready: " + path + "; original Fab assets unchanged")
