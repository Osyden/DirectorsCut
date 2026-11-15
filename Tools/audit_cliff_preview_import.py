"""Read-only UE source-mesh audit against the original cliff FBX authoring data.

Prepare a compact reference using bundled Blender Python (numpy, no Blender UI):
  python THIS.py --prepare sector.json --output Saved/CliffImportAudit/source.json.gz
Run THIS.py in UnrealEditor-Cmd -run=pythonscript -nullrhi
  -EnablePlugins=GeometryScripting, with DC_CLIFF_AUDIT_SOURCE naming that gzip.
Optional DC_CLIFF_AUDIT_ASSET and DC_CLIFF_AUDIT_REPORT override the preview asset
and Saved report. Reads SourceModel LOD0, never Nanite fallback, and creates only
a transient DynamicMesh. Does not save/modify an asset or change import settings.

All source positions and triangle connectivity are compared, and ALL triangle
corner RGBA values are checked at their physical positions (no mask averaging).
Signed-axis inference uses asymmetric full geometry, not sorted bounding sizes.
GeometryScript reverses MeshDescription's sRGB storage conversion; its output is
expected to match the author's numeric masks truncated to8bit. Raw source-storage
decode is reported only as an alternative diagnostic, not the expected result.
"""
import argparse
from collections import Counter
import gzip
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import sys


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def prepare(sidecar_path, output):
    import numpy as np
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from review_eroded_cliff_sector import load_pair, triangles
    sidecar_path = Path(sidecar_path).resolve()
    meta = json.loads(sidecar_path.read_text(encoding="utf-8"))
    if meta["schema"] != "directorscut.cliff_preview_fbx.v1":
        raise RuntimeError("Unsupported FBX sidecar")
    for kind in ("seed", "result", "fbx"):
        if meta.get(kind + "_path") and sha(meta[kind + "_path"]) != meta[kind + "_sha256"]:
            raise RuntimeError(kind + " hash no longer matches the exported FBX")
    seed, result = load_pair(Path(meta["seed_path"]), Path(meta["result_path"]) if meta.get("result_path") else None)
    rows, cols = seed["height_m"].shape
    cell = float(seed["cell_size_m"])
    x, y = np.meshgrid(np.arange(cols)*cell-(cols-1)*cell*.5,
                       np.arange(rows)*cell-(rows-1)*cell*.5)
    valid = seed["heightfield_valid_mask"] > 0
    h = result["height_m"]
    z_origin = float(np.median(h[valid]))
    vertices = np.stack((x, y, h-z_origin), axis=-1).reshape(-1, 3)
    faces = triangles(valid)
    used = np.unique(faces)
    remap = np.full(len(vertices), -1, dtype=np.int64)
    remap[used] = np.arange(len(used))
    # Blender's authoring positions and FLOAT_COLOR are float32 before FBX.
    vertices = vertices[used].astype(np.float32)
    faces = remap[faces]
    channels = []
    for key in ("flow_accumulation", "deposited_m", "eroded_m"):
        raw = np.maximum(result[key].astype(np.float64), 0)
        field = np.log1p(raw) if key == "flow_accumulation" else raw
        divisor = max(float(field[valid].max()), 1.e-12)
        if not math.isclose(divisor, max(meta["color_normalization"][key]["divisor"], 1.e-12), rel_tol=1.e-9):
            raise RuntimeError("Export color normalization no longer matches " + key)
        channels.append((field/divisor).ravel()[used])
    rgba = np.stack((*channels, np.ones(len(used))), axis=-1).astype(np.float32)
    if len(vertices) != meta["vertices"] or len(faces) != meta["triangles"]:
        raise RuntimeError("Reference topology does not match FBX sidecar")
    payload = {"schema": "directorscut.cliff_import_audit_source.v1", "sidecar_path": str(sidecar_path),
               "sidecar_sha256": sha(sidecar_path), "meta": meta,
               "positions_m": vertices.tolist(), "triangles": faces.tolist(), "rgba": rgba.tolist()}
    output = Path(output).resolve()
    if "saved" not in [part.lower() for part in output.parts]:
        raise RuntimeError("Reference payload belongs in ignored Saved")
    output.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(output, "wt", encoding="utf-8") as stream:
        json.dump(payload, stream, separators=(",", ":"))
    print(json.dumps({"reference": str(output), "bytes": output.stat().st_size,
                      "vertices": len(vertices), "triangles": len(faces)}))


def quantized_srgb_decode(rgba):
    """Exact legacy importer intent: truncate to FColor then FLinearColor(FColor)."""
    byte_values = [int(255 * max(0., min(1., value))) / 255. for value in rgba]
    return [v/12.92 if v <= .04045 else ((v+.055)/1.055)**2.4
            for v in byte_values[:3]] + byte_values[3:]


def key_cm(point):
    # 0.1cm buckets; source XY spacing is200cm. Full residual is checked later.
    return tuple(round(value*10) for value in point)


def nearby_key(point, keys):
    base = key_cm(point)
    if base in keys:
        return base
    # Float32 cm conversion can cross a bucket boundary. The final residual
    # check remains0.02cm, so this never accepts a shifted authored vertex.
    for offset in itertools.product((-1, 0, 1), repeat=3):
        candidate = tuple(base[i]+offset[i] for i in range(3))
        if candidate in keys:
            return candidate
    return None


def apply_axis(point, permutation, signs):
    return [100.*signs[axis]*point[permutation[axis]] for axis in range(3)]


def infer_axes(source_positions, actual_positions):
    actual_keys = {key_cm(v) for v in actual_positions}
    indices = sorted(set([0, len(source_positions)-1] +
                         [i*(len(source_positions)-1)//96 for i in range(97)]))
    choices = []
    for permutation in itertools.permutations(range(3)):
        for signs in itertools.product((-1, 1), repeat=3):
            hits = sum(nearby_key(apply_axis(source_positions[i], permutation, signs), actual_keys) is not None for i in indices)
            choices.append((hits, permutation, signs))
    choices.sort(reverse=True)
    if choices[0][0] != len(indices) or choices[1][0] == choices[0][0]:
        raise RuntimeError("No unique axis/unit match; best signed-axis probes=" + str(choices[:3]))
    return choices[0][1:]


def audit_geometry(source, actual_positions, actual_triangles):
    permutation, signs = infer_axes(source["positions_m"], actual_positions)
    expected = [apply_axis(p, permutation, signs) for p in source["positions_m"]]
    lookup = {key_cm(point): i for i, point in enumerate(expected)}
    if len(lookup) != len(expected):
        raise RuntimeError("Ambiguous reference position buckets")
    actual_to_source = []
    residual = 0.
    first_actual = {}
    for point in actual_positions:
        source_index = lookup.get(nearby_key(point, lookup))
        if source_index is None:
            raise RuntimeError("Actual source-mesh position is not in the authored geometry: " + str(point))
        actual_to_source.append(source_index)
        first_actual.setdefault(source_index, point)
        residual = max(residual, max(abs(a-b) for a, b in zip(point, expected[source_index])))
    if residual > .02 or len(first_actual) != len(expected):
        raise RuntimeError("Source geometry coverage/residual failed: " + str((len(first_actual), residual)))
    expected_faces = Counter(tuple(sorted(tri)) for tri in source["triangles"])
    actual_faces = Counter(tuple(sorted(actual_to_source[i] for i in tri)) for tri in actual_triangles)
    if expected_faces != actual_faces:
        raise RuntimeError("Source triangle connectivity differs from authored mesh")
    # Three noncollinear identity anchors, plus an asymmetric highest-relief point.
    positions = source["positions_m"]
    anchor_ids = [min(range(len(positions)), key=lambda i: positions[i][0]),
                  max(range(len(positions)), key=lambda i: positions[i][0]),
                  min(range(len(positions)), key=lambda i: positions[i][1]),
                  min(range(len(positions)), key=lambda i: positions[i][2])]
    matrix = [[0, 0, 0] for _ in range(3)]
    for axis in range(3):
        matrix[axis][permutation[axis]] = signs[axis]
    meta = source["meta"]
    basis = [meta["basis_u_arena"], meta["basis_v_arena"], meta["basis_z_arena"]]
    # Source column basis multiplied by inverse signed permutation.
    imported_basis = [[sum(basis[k][r]*matrix[c][k] for k in range(3)) for r in range(3)] for c in range(3)]
    return actual_to_source, {"source_vertices": len(expected), "actual_vertices": len(actual_positions),
        "triangles": len(actual_triangles), "position_max_error_cm": residual,
        "source_metres_to_imported_cm": {"scale": 100, "axis_matrix": matrix},
        "anchors": [{"source_vertex": i, "author_m": positions[i], "import_cm": first_actual[i]} for i in anchor_ids],
        "arena_reconstruction": {"origin_cm": [100*v for v in meta["local_origin_arena_m"]],
                                  "basis_for_imported_xyz": imported_basis,
                                  "equation": "arena_cm=origin_cm+Xcm*basis[0]+Ycm*basis[1]+Zcm*basis[2]"}}


def unreal_main():
    import unreal
    payload_path = Path(os.environ["DC_CLIFF_AUDIT_SOURCE"]).resolve()
    with gzip.open(payload_path, "rt", encoding="utf-8") as stream:
        source = json.load(stream)
    if source["schema"] != "directorscut.cliff_import_audit_source.v1":
        raise RuntimeError("Unsupported audit payload")
    if sha(source["sidecar_path"]) != source["sidecar_sha256"]:
        raise RuntimeError("Sidecar changed after reference preparation")
    for kind in ("seed", "result", "fbx"):
        path = source["meta"].get(kind + "_path")
        if path and sha(path) != source["meta"][kind + "_sha256"]:
            raise RuntimeError(kind + " changed after reference preparation")
    asset_path = os.environ.get("DC_CLIFF_AUDIT_ASSET")
    if not asset_path:
        imported = json.loads((Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) /
                               "CliffLandformPreviewImport.json").read_text(encoding="utf-8"))
        if imported["source_sha256"] != source["meta"]["fbx_sha256"]:
            raise RuntimeError("Latest imported preview does not match audit reference")
        asset_path = imported["asset"]
    report_path = Path(os.environ.get("DC_CLIFF_AUDIT_REPORT", str(Path(unreal.Paths.convert_relative_path_to_full(
        unreal.Paths.project_saved_dir())) / "CliffPreviewSourceAudit.json")))
    report = {"schema": "directorscut.cliff_import_audit.v1", "asset": asset_path,
              "source_fbx_sha256": source["meta"]["fbx_sha256"], "asset_modified": False,
              "source_route": "SourceModel LOD0→transient DynamicMesh, build settings/scale disabled",
              "nanite_render_colors_verified": False}
    try:
        if not hasattr(unreal, "GeometryScript_AssetUtils"):
            raise RuntimeError("Enable installed GeometryScripting plugin for this commandlet with -EnablePlugins=GeometryScripting")
        mesh = unreal.load_asset(asset_path)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Audit asset is not a StaticMesh")
        description = mesh.get_static_mesh_description(0)
        if description is None:
            raise RuntimeError("No source MeshDescription LOD0; refusing render fallback")
        report["mesh_description_counts"] = {"vertices": description.get_vertex_count(),
                                            "triangles": description.get_triangle_count(),
                                            "vertex_instances": description.get_vertex_instance_count()}
        options = unreal.GeometryScriptCopyMeshFromAssetOptions()
        options.apply_build_settings = False
        options.request_tangents = False
        options.use_build_scale = False
        lod = unreal.GeometryScriptMeshReadLOD()
        lod.lod_type = unreal.GeometryScriptLODType.SOURCE_MODEL
        lod.lod_index = 0
        dynamic = unreal.DynamicMesh()
        dynamic, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh_v2(mesh, dynamic, options, lod)
        if outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
            raise RuntimeError("SourceModel copy failed: " + str(outcome))
        query = unreal.GeometryScript_MeshQueries
        lists = unreal.GeometryScript_List
        _, position_list, position_gaps = query.get_all_vertex_positions(dynamic, False)
        _, triangle_list, triangle_gaps = query.get_all_triangle_indices(dynamic, False)
        if position_gaps or triangle_gaps:
            raise RuntimeError("Unexpected source DynamicMesh ID gaps; refuse silently reindexing")
        positions = [[float(p.x), float(p.y), float(p.z)] for p in lists.convert_vector_list_to_array(position_list)]
        triangles = [[int(t.x), int(t.y), int(t.z)] for t in lists.convert_triangle_list_to_array(triangle_list)]
        mapping, geometry_report = audit_geometry(source, positions, triangles)
        report["geometry"] = geometry_report
        report["geometry_passed"] = True
        expected_legacy = [quantized_srgb_decode(color) for color in source["rgba"]]
        raw_max = [0.]*4
        legacy_max = [0.]*4
        raw_sum = [0.]*4
        legacy_sum = [0.]*4
        byte_max = [0.]*4
        count = 0
        for triangle_id, indices in enumerate(triangles):
            _, c0, c1, c2, valid = query.get_triangle_vertex_colors(dynamic, triangle_id)
            if not valid:
                raise RuntimeError("Triangle has no color attribute: " + str(triangle_id))
            for vertex_id, color in zip(indices, (c0, c1, c2)):
                source_id = mapping[vertex_id]
                for channel, value in enumerate((color.r, color.g, color.b, color.a)):
                    raw_error = abs(value-source["rgba"][source_id][channel])
                    legacy_error = abs(value-expected_legacy[source_id][channel])
                    raw_max[channel] = max(raw_max[channel], raw_error)
                    legacy_max[channel] = max(legacy_max[channel], legacy_error)
                    raw_sum[channel] += raw_error
                    legacy_sum[channel] += legacy_error
                    expected_byte = int(255*max(0., min(1., source["rgba"][source_id][channel]))) / 255.
                    byte_max[channel] = max(byte_max[channel], abs(value-expected_byte))
                count += 1
        report["masks"] = {"corners_checked": count, "channels": "RGBA", "averaged": False,
                           "raw_numeric_max_error": raw_max, "raw_numeric_mean_error": [v/count for v in raw_sum],
                           "legacy_truncated_srgb_decode_max_error": legacy_max,
                           "legacy_truncated_srgb_decode_mean_error": [v/count for v in legacy_sum],
                           "numeric_fidelity_within_8bit": max(raw_max) <= 1./255.+1.e-6,
                           "matches_legacy_fbx_transform": max(legacy_max) <= 1.e-6,
                           "expected_truncated_byte_max_error": byte_max,
                           "matches_expected_geometryscript_transform": max(byte_max) <= 1.e-6,
                           "storage_note": "FBX importer truncates numeric channels to bytes then sRGB-decodes RGB for source storage. GeometryScript's MeshDescriptionToDynamicMesh converter defaults LinearToSRGB=true, reversing that storage conversion. Its returned RGBA should therefore match original numeric masks truncated to8bits. StaticMeshBuilder also re-encodes via ToFColor(true); Nanite reads normalized bytes without another gamma conversion. Do not change exporter toSRGB to compensate source storage.",
                           "local_engine_evidence": ["FbxStaticMeshImport.cpp:944-952", "MeshDescriptionToDynamicMesh.h:43", "MeshDescriptionToDynamicMesh.cpp:1113-1127", "StaticMeshBuilder.cpp:1708", "NaniteBuilder/Cluster.cpp:90", "NaniteAttributeDecode.ush:848", "NaniteVertexFactory.ush:841"],
                           "limitation": "SourceModel attributes via documented DynamicMesh color conversion only; actual Nanite render/LOD interpolation and material sampling not measured."}
        report["audit_completed"] = True
        report["source_mask_contract_passed"] = report["masks"]["matches_expected_geometryscript_transform"]
        report["mask_fidelity_passed"] = report["masks"]["numeric_fidelity_within_8bit"]
        if not report["source_mask_contract_passed"] or not report["mask_fidelity_passed"]:
            report["finding"] = "SourceModel→DynamicMesh mask contract differs from the traced import/conversion path; inspect before material calibration."
    except Exception as exc:
        report["error"] = type(exc).__name__ + ": " + str(exc)
        raise
    finally:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
        unreal.log_warning("DC_CLIFF_SOURCE_AUDIT " + json.dumps(report))


if __name__ == "__main__":
    if "--prepare" in sys.argv:
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--prepare", type=Path, required=True)
        parser.add_argument("--output", type=Path, required=True)
        args = parser.parse_args()
        prepare(args.prepare, args.output)
    else:
        unreal_main()
