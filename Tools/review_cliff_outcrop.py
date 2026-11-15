"""Audit and render paired true-3D cliff meshes at physical scale, CPU only.

Blender --background --factory-startup --threads 8 --python THIS.py --
  --baseline original.npz --candidate fractured.npz --output Saved/OutcropReview

NPZ: vertices_m[N,3] absolute arena XYZ metres, triangles[M,3] integers,
frame_origin_world_m[3], basis_u_world/basis_v_world/basis_z_world[3],
metadata_json scalar string. Optional vertex_rgba is preserved as input data but
ignored by neutral clay. Both meshes must use the same orthonormal frame.
Only a rigid frame transform is applied. No heightfield projection, repair,
decimation, displacement or geometry smoothing is performed. Render labels and
report distinguish flat versus optional angle-based shading. No Unreal launch.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from review_eroded_cliff_sector import reset_scene, camera_at, label_camera, configure_mesh_normals


def load_mesh(path):
    with np.load(path, allow_pickle=False) as source:
        data = {key: source[key].copy() for key in source.files}
    vertices = data["vertices_m"].astype(np.float64)
    original_triangles = data["triangles"]
    if not np.issubdtype(original_triangles.dtype, np.integer):
        raise ValueError("Triangle indices must be integer")
    triangles = original_triangles.astype(np.int64)
    if vertices.ndim != 2 or vertices.shape[1] != 3 or not np.isfinite(vertices).all():
        raise ValueError("Expected finite vertices_m[N,3]")
    if triangles.ndim != 2 or triangles.shape[1] != 3 or len(triangles) < 4:
        raise ValueError("Expected triangles[M,3]")
    if triangles.min() < 0 or triangles.max() >= len(vertices):
        raise ValueError("Triangle references missing vertices")
    origin = np.asarray(data["frame_origin_world_m"], dtype=float)
    basis = np.stack([data["basis_u_world"], data["basis_v_world"], data.get("basis_z_world", [0, 0, 1])], axis=1).astype(float)
    if origin.shape != (3,) or not np.isfinite(origin).all() or not np.allclose(basis.T@basis, np.eye(3), atol=1.e-6) or not np.isclose(np.linalg.det(basis), 1., atol=1.e-6):
        raise ValueError("Frame must be finite, orthonormal and right handed")
    chart = (vertices-origin)@basis
    return {"path": str(Path(path).resolve()), "sha256": hashlib.sha256(Path(path).read_bytes()).hexdigest(),
            "vertices": vertices, "triangles": triangles, "chart": chart, "origin": origin, "basis": basis,
            "metadata": json.loads(str(data.get("metadata_json", "{}")))}


def topology_audit(mesh):
    vertices, triangles = mesh["chart"], mesh["triangles"]
    xyz = vertices[triangles]
    cross = np.cross(xyz[:, 1]-xyz[:, 0], xyz[:, 2]-xyz[:, 0])
    twice_area = np.linalg.norm(cross, axis=1)
    normals = cross/np.maximum(twice_area[:, None], 1.e-30)
    directed = np.concatenate((triangles[:, [0, 1]], triangles[:, [1, 2]], triangles[:, [2, 0]]))
    edges, inverse, counts = np.unique(np.sort(directed, axis=1), axis=0, return_inverse=True, return_counts=True)
    orientations = np.bincount(inverse, weights=np.where(directed[:, 0] < directed[:, 1], 1, -1))
    degenerate = int(np.count_nonzero(twice_area <= 1.e-10))
    duplicates = len(triangles)-len(np.unique(np.sort(triangles, axis=1), axis=0))
    volume = float(np.sum(np.einsum("ij,ij->i", xyz[:, 0], np.cross(xyz[:, 1], xyz[:, 2])))/6)
    parent = np.arange(len(vertices))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for a, b in edges:
        aa, bb = find(a), find(b)
        if aa != bb:
            parent[aa] = bb
    used = np.unique(triangles)
    components = len({find(i) for i in used})
    # Excludes most artificial rear/bottom closure faces. This metric is only
    # a diagnostic; the matched side view must establish visible undercuts.
    front = normals[:, 1] < -.15
    downward_front = front & (normals[:, 2] < -.15)
    edge_lengths = np.linalg.norm(vertices[edges[:, 0]]-vertices[edges[:, 1]], axis=1)
    report = {"vertices": len(vertices), "triangles": len(triangles), "connected_components": components,
              "unused_vertices": len(vertices)-len(used), "boundary_edges": int(np.count_nonzero(counts == 1)),
              "nonmanifold_edges": int(np.count_nonzero(counts > 2)),
              "inconsistent_winding_edges": int(np.count_nonzero((counts == 2) & (orientations != 0))),
              "degenerate_triangles": degenerate, "duplicate_triangles": duplicates,
              "signed_volume_m3": volume, "surface_area_m2": float(twice_area.sum()*.5),
              "front_downward_area_m2": float(twice_area[downward_front].sum()*.5),
              "front_area_m2": float(twice_area[front].sum()*.5),
              "front_downward_area_limitation": "Normal-selected diagnostic; closure geometry and internal cavity surfaces are not proof of visible overhangs.",
              "bounds_chart_m": [vertices.min(axis=0).tolist(), vertices.max(axis=0).tolist()],
              "dimensions_m": np.ptp(vertices, axis=0).tolist(),
              "edge_length_m_p0_p5_p50_p95_p100": np.percentile(edge_lengths, [0, 5, 50, 95, 100]).tolist()}
    report["closed_consistently_oriented"] = not any(report[k] for k in (
        "boundary_edges", "nonmanifold_edges", "inconsistent_winding_edges", "degenerate_triangles", "duplicate_triangles")) and volume > 0
    return report


def matched_views(baseline, candidate):
    points = np.concatenate((baseline["chart"], candidate["chart"]))
    low, high = points.min(axis=0), points.max(axis=0)
    center = (low+high)*.5
    span = float(max(high-low))
    return {
        "wall": {"location": (center+[.08*span, -2*span, .10*span]).tolist(), "target": center.tolist(), "width": span*1.35},
        "oblique": {"location": (center+[1.35*span, -1.65*span, .95*span]).tolist(), "target": center.tolist(), "width": span*1.70},
        "side": {"location": (center+[2*span, -.12*span, .05*span]).tolist(), "target": center.tolist(), "width": span*1.35},
    }


def render_pair(baseline, candidate, output, cameras, samples, mode, angle):
    import bpy
    from mathutils import Vector
    scene = reset_scene(1440, 960, samples)
    material = bpy.data.materials.new("Neutral rough clay; no detail")
    material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (.33, .33, .33, 1)
    shader.inputs["Roughness"].default_value = .85
    shader.inputs["Specular IOR Level"].default_value = .2
    sun = bpy.data.objects.new("Fixed raking light", bpy.data.lights.new("Fixed raking light", "SUN"))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.angle = 2., .08
    sun.rotation_euler = Vector((.7, .45, -.6)).to_track_quat("-Z", "Y").to_euler()
    objects, normal_reports = {}, {}
    for stage, data in (("baseline", baseline), ("fractured", candidate)):
        mesh = bpy.data.meshes.new(stage)
        mesh.from_pydata(data["chart"].tolist(), [], data["triangles"].tolist())
        mesh.update()
        obj = bpy.data.objects.new(stage, mesh)
        scene.collection.objects.link(obj)
        mesh.materials.append(material)
        normal_reports[stage] = configure_mesh_normals(mesh, mode, angle)
        obj.hide_render = True
        objects[stage] = obj
    for name, view in cameras.items():
        camera = camera_at(scene, view["location"], view["target"], view["width"])
        for stage, data in (("baseline", baseline), ("fractured", candidate)):
            objects[stage].hide_render = False
            dims = np.ptp(data["chart"], axis=0)
            label_camera(scene, camera, f"TRUE 3D OUTCROP | {stage.upper()} | {name}",
                         f"Physical bounds {dims[0]:.1f} x {dims[1]:.1f} x {dims[2]:.1f} m | identical camera and raking light",
                         f"Source {data['sha256'][:10]} | normals {mode}"+(f" {angle:g}deg" if mode=="angle" else "")+
                         " | no projection, displacement or geometry smoothing")
            scene.render.filepath = str(output/f"{name}_{stage}.png")
            bpy.ops.render.render(write_still=True)
            for child in list(camera.children):
                bpy.data.objects.remove(child, do_unlink=True)
            objects[stage].hide_render = True
        bpy.data.objects.remove(camera, do_unlink=True)
    return normal_reports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--samples", type=int, default=16)
    parser.add_argument("--normal-mode", choices=("flat", "angle"), default="flat")
    parser.add_argument("--normal-angle-deg", type=float, default=35.)
    parser.add_argument("--camera-manifest", type=Path)
    parser.add_argument("--audit-only", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else None)
    output = args.output.resolve()
    if "saved" not in [p.lower() for p in output.parts]:
        parser.error("Generated output must be under ignored Saved")
    if not 1 <= args.samples <= 128 or not 0 < args.normal_angle_deg < 180:
        parser.error("Invalid samples or normal angle")
    baseline, candidate = load_mesh(args.baseline), load_mesh(args.candidate)
    if not np.allclose(baseline["origin"], candidate["origin"], atol=1.e-8) or not np.allclose(baseline["basis"], candidate["basis"], atol=1.e-8):
        raise RuntimeError("Baseline/candidate frames differ")
    output.mkdir(parents=True, exist_ok=True)
    report = {"schema": "directorscut.cliff_outcrop_review.v1", "source": {},
              "frame_origin_world_m": baseline["origin"].tolist(), "chart_basis_columns": baseline["basis"].tolist(),
              "geometry_operation": "Rigid translation/rotation only; no XY projection or repair", "runtime_integrated": False}
    for stage, data in (("baseline", baseline), ("fractured", candidate)):
        report["source"][stage] = {"path": data["path"], "sha256": data["sha256"], "metadata": data["metadata"], "audit": topology_audit(data)}
    cameras = json.loads(args.camera_manifest.read_text())["cameras"] if args.camera_manifest else matched_views(baseline, candidate)
    report["cameras"] = cameras
    report["audit_passed"] = all(s["audit"]["closed_consistently_oriented"] for s in report["source"].values())
    # Invalid candidates may be rendered diagnostically, but never silently
    # repaired or certified. Their exact defects stay visible in the report.
    if not args.audit_only:
        report["normals"] = render_pair(baseline, candidate, output, cameras, args.samples, args.normal_mode, args.normal_angle_deg)
        report["render"] = {"engine": "Cycles", "device": "CPU", "threads": 8, "samples": args.samples, "normal_mode": args.normal_mode}
    (output/"review.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "audit_passed": report["audit_passed"],
                      "audits": {k: v["audit"] for k, v in report["source"].items()}}, indent=2))


if __name__ == "__main__":
    main()
