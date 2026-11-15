"""Import an original cliff FBX into an isolated, ignored Nanite preview folder.

DC_CLIFF_PREVIEW_FBX must name the worker's generated FBX. This is authoring
validation only: no map is opened/saved and no runtime terrain is replaced.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> -nullrhi.
The optional DC_CLIFF_EXPECTED_METRES=x,y,z validates physical bounds.
"""
import hashlib
import json
import math
import os
import re
from pathlib import Path

import unreal


ROOT = "/Game/DC/Maelstrom/GeneratedCliffPreview"
source_text = os.environ.get("DC_CLIFF_PREVIEW_FBX", "")
if not source_text:
    raise RuntimeError("Set DC_CLIFF_PREVIEW_FBX to an original generated FBX")
source = Path(source_text).resolve()
if not source.is_file() or source.suffix.lower() != ".fbx":
    raise RuntimeError("Missing FBX: " + str(source))
source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
asset_name = os.environ.get("DC_CLIFF_PREVIEW_NAME", "SM_CliffSector_" + source_hash[:12])
if not re.fullmatch(r"SM_CliffSector_[A-Za-z0-9_]+", asset_name):
    raise RuntimeError("Choose a named isolated SM_CliffSector_* preview")
if unreal.EditorAssetLibrary.does_asset_exist(ROOT + "/" + asset_name):
    raise RuntimeError("Use a fresh preview name: unattended FBX reimport can restore an Ignore vertex-color policy")
# ExecCmds can run after the Python commandlet. Kismet processes console
# variables synchronously, before needing a world; this is process-local only.
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
if unreal.SystemLibrary.get_console_variable_int_value("Interchange.FeatureFlags.Import.FBX") != 0:
    raise RuntimeError("Could not select the explicit legacy FBX options before import")
sidecar_path = source.with_suffix(".json")
sidecar = json.loads(sidecar_path.read_text(encoding="utf-8")) if sidecar_path.is_file() else {}
if sidecar and (sidecar.get("schema") != "directorscut.cliff_preview_fbx.v1" or sidecar.get("fbx_sha256") != source_hash):
    raise RuntimeError("Cliff export sidecar schema/hash does not match FBX")

options = unreal.FbxImportUI()
options.automated_import_should_detect_type = False
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
options.import_mesh = True
options.import_as_skeletal = False
options.import_materials = False
options.import_textures = False
options.static_mesh_import_data.combine_meshes = True
options.static_mesh_import_data.convert_scene = True
options.static_mesh_import_data.convert_scene_unit = True
options.static_mesh_import_data.import_uniform_scale = 1.0
options.static_mesh_import_data.generate_lightmap_u_vs = False
options.static_mesh_import_data.auto_generate_collision = False
options.static_mesh_import_data.vertex_color_import_option = unreal.VertexColorImportOption.REPLACE
options.static_mesh_import_data.build_nanite = True
options.static_mesh_import_data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS

task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = ROOT
task.destination_name = asset_name
task.automated = True
task.replace_existing = False
task.save = False
task.factory = unreal.FbxFactory()
task.options = options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
meshes = [unreal.load_asset(path) for path in task.imported_object_paths]
meshes = [asset for asset in meshes if isinstance(asset, unreal.StaticMesh)]
if len(meshes) != 1:
    raise RuntimeError("Expected one original preview mesh, got " + str(task.imported_object_paths))
mesh = meshes[0]
if not mesh.get_path_name().startswith(ROOT + "/"):
    raise RuntimeError("Importer escaped preview folder")

bounds = mesh.get_bounding_box()
size = bounds.max - bounds.min
size_m = [float(size.x)*.01, float(size.y)*.01, float(size.z)*.01]
if not all(math.isfinite(v) and v > 0 for v in size_m):
    raise RuntimeError("Invalid imported bounds: " + str(size_m))
expected_text = os.environ.get("DC_CLIFF_EXPECTED_METRES", "")
expected = [float(v) for v in expected_text.split(",")] if expected_text else sidecar.get("local_dimensions_m")
if expected:
    if len(expected) != 3 or any(not math.isclose(a, b, rel_tol=.002, abs_tol=.05)
                                for a, b in zip(sorted(size_m), sorted(expected))):
        raise RuntimeError("Imported unit/axis mismatch: actual=%s expected=%s" % (size_m, expected))

# The editor subsystem is not instantiated in the Python commandlet. Nanite
# was requested at import; inspect the actual UE5.8 mesh setting directly.
if not mesh.get_editor_property("nanite_settings").enabled:
    raise RuntimeError("FBX import did not enable Nanite")

tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
clay = unreal.load_asset(ROOT + "/M_CliffClay")
if clay is None:
    clay = tools.create_asset("M_CliffClay", ROOT, unreal.Material, unreal.MaterialFactoryNew())
clay.set_editor_property("used_with_nanite", True)
mel.delete_all_material_expressions(clay)
base = mel.create_material_expression(clay, unreal.MaterialExpressionConstant3Vector, -200, 0)
base.constant = unreal.LinearColor(.3, .3, .3, 1)
rough = mel.create_material_expression(clay, unreal.MaterialExpressionConstant, -200, 150)
rough.r = .85
if not mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR):
    raise RuntimeError("Clay base connection failed")
if not mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS):
    raise RuntimeError("Clay roughness connection failed")
mel.recompile_material(clay)
mesh.set_material(0, clay)
if not unreal.EditorAssetLibrary.save_loaded_asset(clay) or not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
    raise RuntimeError("Failed to save preview assets")

report = {
    "source_fbx": str(source), "source_sha256": source_hash,
    "source_kind": sidecar.get("source_kind", "explicit unit/import probe"),
    "export_sidecar": str(sidecar_path) if sidecar else None,
    "asset": mesh.get_path_name(), "size_metres": size_m, "nanite_enabled": True,
    "import_data_class": mesh.get_editor_property("asset_import_data").get_class().get_name(),
    "render_lod0_triangles": mesh.get_num_triangles(0),
    "material": clay.get_path_name(), "runtime_integrated": False,
    "limitations": "Import/bounds/settings validation only; no GPU render, arena orientation/vertex-mask verification, surface sampler or waterfall integration.",
}
report_path = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / "CliffLandformPreviewImport.json"
report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log_warning("DC_CLIFF_PREVIEW_IMPORT " + json.dumps(report))
