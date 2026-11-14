"""Import original generated VDB loops and build scene-lit whitewater materials.

Run in this checkout's UnrealEditor-Cmd Python commandlet. DC_WATERFALL_DIR
may point to another worker's Saved/GeneratedWaterfalls directory. Generated
assets are ignored by Git; /Game/DC is already included in project cooking.
"""
import json
import os
from pathlib import Path

import unreal

ROOT = "/Game/DC/Maelstrom/ProceduralWaterfalls"
materials_only = os.environ.get("DC_WATERFALL_MATERIALS_ONLY", "0") == "1"
source = Path(os.environ.get("DC_WATERFALL_DIR", str(Path(unreal.Paths.project_saved_dir()) / "GeneratedWaterfalls"))).resolve()
manifest_path = source / "manifest.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig")) if manifest_path.is_file() else {}
fitted = manifest.get("fitted_profile")
if fitted:
    feed_index = fitted.get("feed_index")
    if not isinstance(feed_index, int) or not 0 <= feed_index <= 999:
        raise RuntimeError("Invalid fitted waterfall feed index in manifest")
    ROOT += "/Feed%02d" % feed_index
    unreal.log_warning("DC_WATERFALL_IMPORT fitted feed=%d profile=%s" % (feed_index, fitted.get("profile_sha1")))
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
records = []

for name in ("Falling", "Impact"):
    files = sorted((source / name).glob("*.vdb"))
    if len(files) < 2:
        raise RuntimeError("Missing animated VDB sequence: " + str(source / name))
    if not materials_only:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(files[0]))
        task.set_editor_property("destination_path", ROOT)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        task.set_editor_property("factory", unreal.SparseVolumeTextureFactory())
        tools.import_asset_tasks([task])
    # Native sequence import derives its name from the numbered source prefix.
    volume = unreal.load_asset(ROOT + "/" + name)
    if not volume or volume.get_num_frames() != len(files):
        raise RuntimeError("VDB sequence did not import with the expected frame count: " + name)

    material_path = ROOT + "/M_" + name
    mat = unreal.load_asset(material_path)
    if not mat:
        mat = tools.create_asset("M_" + name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_VOLUME)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("used_with_heterogeneous_volumes", True)
    svt = mel.create_material_expression(mat, unreal.MaterialExpressionSparseVolumeTextureSampleParameter, -600, 0)
    svt.set_editor_property("parameter_name", "SparseVolumeTexture")
    svt.set_editor_property("sparse_volume_texture", volume)
    channel = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -400, 0)
    channel.set_editor_property("r", True)
    for key in ("g", "b", "a"):
        channel.set_editor_property(key, False)
    mel.connect_material_expressions(svt, "Attributes A", channel, "")
    density = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
    density.set_editor_property("parameter_name", "Density")
    density.set_editor_property("default_value", .004 if name == "Falling" else .0015)
    extinction = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 0)
    mel.connect_material_expressions(channel, "", extinction, "A")
    mel.connect_material_expressions(density, "", extinction, "B")
    mel.connect_material_property(extinction, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    albedo = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, -200)
    albedo.set_editor_property("constant", unreal.LinearColor(.92, .95, .97, 1))
    mel.connect_material_property(albedo, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Zero by default; runtime may enable sampled-density emission solely to
    # diagnose native coordinates/occlusion without depending on scene light.
    diagnostic = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 400)
    diagnostic.set_editor_property("parameter_name", "DiagnosticEmission")
    diagnostic.set_editor_property("default_value", 0.)
    emission = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 400)
    if not mel.connect_material_expressions(channel, "", emission, "A"):
        raise RuntimeError("Failed to connect density to diagnostic emission")
    if not mel.connect_material_expressions(diagnostic, "", emission, "B"):
        raise RuntimeError("Failed to connect diagnostic emission multiplier")
    if not mel.connect_material_property(emission, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Failed to connect diagnostic emission output")
    mel.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_asset(material_path, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + material_path)
    record = {"name": name, "frames": volume.get_num_frames(), "resolution": [volume.get_size_x(), volume.get_size_y(), volume.get_size_z()], "asset": volume.get_path_name()}
    records.append(record)
    unreal.log_warning("DC_WATERFALL_IMPORT " + json.dumps(record))

report = Path(unreal.Paths.project_saved_dir()) / "ProceduralWaterfallImport.json"
report.write_text(json.dumps({"source": str(source), "root": ROOT, "fitted_profile": fitted, "materials_only": materials_only, "volumes": records}, indent=2), encoding="utf-8")
unreal.log_warning("DC_WATERFALL_IMPORT complete " + str(report))
