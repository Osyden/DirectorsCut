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
Optional --rock-source uses co-registered marble_cliff_01 surface maps only on
explicit face_kind=1 triangles, at source physical U/Z tile size. It never adds
renderer displacement; neutral clay remains the default and geometry control.
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
            "metadata": json.loads(str(data.get("metadata_json", "{}"))),
            "face_kind": data.get("face_kind")}


def topology_audit(mesh, surface_mode=False):
    vertices, triangles = mesh["chart"], mesh["triangles"]
    xyz = vertices[triangles]
    cross = np.cross(xyz[:, 1]-xyz[:, 0], xyz[:, 2]-xyz[:, 0])
    twice_area = np.linalg.norm(cross, axis=1)
    normals = cross/np.maximum(twice_area[:, None], 1.e-30)
    directed = np.concatenate((triangles[:, [0, 1]], triangles[:, [1, 2]], triangles[:, [2, 0]]))
    edges, inverse, counts = np.unique(np.sort(directed, axis=1), axis=0, return_inverse=True, return_counts=True)
    mesh["boundary_edges"] = edges[counts == 1]
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
              "signed_volume_m3": None if surface_mode else volume, "surface_area_m2": float(twice_area.sum()*.5),
              "expected_topology": "open_surface" if surface_mode else "closed_solid",
              "topology_limitations": ("Open edge-manifold/winding checks only; no enclosed-volume or exhaustive self-intersection test." if surface_mode else
                                      "Edge-manifold/winding and signed-volume checks only; no exhaustive self-intersection or nested-shell containment test."),
              "front_downward_area_m2": float(twice_area[downward_front].sum()*.5),
              "front_area_m2": float(twice_area[front].sum()*.5),
              "front_downward_area_limitation": "Normal-selected diagnostic; closure geometry and internal cavity surfaces are not proof of visible overhangs.",
              "bounds_chart_m": [vertices.min(axis=0).tolist(), vertices.max(axis=0).tolist()],
              "dimensions_m": np.ptp(vertices, axis=0).tolist(),
              "edge_length_m_p0_p5_p50_p95_p100": np.percentile(edge_lengths, [0, 5, 50, 95, 100]).tolist()}
    report["closed_consistently_oriented"] = not any(report[k] for k in (
        "boundary_edges", "nonmanifold_edges", "inconsistent_winding_edges", "degenerate_triangles", "duplicate_triangles")) and volume > 0
    boundary_degrees = np.bincount(mesh["boundary_edges"].ravel(), minlength=len(vertices))
    report["boundary_vertices_not_degree_two"] = int(np.count_nonzero((boundary_degrees != 0) & (boundary_degrees != 2)))
    report["valid_open_manifold_surface"] = report["boundary_edges"] > 0 and components == 1 and not any(report[k] for k in (
        "nonmanifold_edges", "inconsistent_winding_edges", "degenerate_triangles", "duplicate_triangles", "unused_vertices", "boundary_vertices_not_degree_two"))
    report["orientation_limitation"] = "Consistent adjacent winding is tested; open-surface outward orientation and game support require separate source checks."
    return report


def matched_views(baseline, candidate):
    points = np.concatenate((baseline["chart"], candidate["chart"]))
    low, high = points.min(axis=0), points.max(axis=0)
    center = (low+high)*.5
    span = float(max(high-low))
    return {
        "wall": {"location": (center+[.08*span, -2*span, .10*span]).tolist(), "target": center.tolist(), "width": span*1.90},
        "oblique": {"location": (center+[1.35*span, -1.65*span, .95*span]).tolist(), "target": center.tolist(), "width": span*2.00},
        "side": {"location": (center+[2*span, -.12*span, .05*span]).tolist(), "target": center.tolist(), "width": span*1.90},
    }


def crop_boundary_audit(baseline, candidate, tolerance=1.e-5):
    """Compare crop-plane vertices, independent of the generator's own metrics.

    This checks rectangular authoring-patch joins. It does not claim that an
    arbitrary new cutout already fits the game's full cliff mesh.
    """
    low, high = baseline["chart"].min(axis=0), baseline["chart"].max(axis=0)
    sets = []
    counts = []
    for mesh in (baseline, candidate):
        points = mesh["chart"]
        edge = np.any((np.abs(points[:, :2]-low[:2]) <= tolerance) |
                      (np.abs(points[:, :2]-high[:2]) <= tolerance), axis=1)
        border = points[edge]
        counts.append(len(border))
        sets.append({tuple(row) for row in np.rint(border/tolerance).astype(np.int64)})
    return {"tolerance_m": tolerance, "baseline_vertices": counts[0], "candidate_vertices": counts[1],
            "missing_quantized_vertices": len(sets[0]-sets[1]), "added_quantized_vertices": len(sets[1]-sets[0]),
            "boundary_vertex_set_preserved": sets[0] == sets[1],
            "limitation": "Rectangular XY crop-plane vertex sets only; not a proof of full game-sector seam or temple support."}


def surface_boundary_audit(baseline, candidate, tolerance=1.e-5):
    sets = []
    for mesh in (baseline, candidate):
        positions = np.rint(mesh["chart"]/tolerance).astype(np.int64)
        sets.append({tuple(sorted((tuple(positions[a]), tuple(positions[b])))) for a, b in mesh["boundary_edges"]})
    return {"tolerance_m": tolerance, "baseline_edges": len(sets[0]), "candidate_edges": len(sets[1]),
            "missing_quantized_edges": len(sets[0]-sets[1]), "added_quantized_edges": len(sets[1]-sets[0]),
            "boundary_edge_geometry_preserved": sets[0] == sets[1],
            "limitation": "Compares every actual open boundary edge, including irregular tile perimeter; does not establish temple support."}


def surface_views(baseline, candidate, close_center=None):
    points = np.concatenate((baseline["chart"], candidate["chart"]))
    low, high = points.min(axis=0), points.max(axis=0)
    center = (low+high)*.5
    width = float(high[0]-low[0])
    focus = np.asarray(close_center) if close_center is not None else center
    far = center+np.array([0., -1500., 0.])
    far[2] = low[2]+12.
    return {
        "wall": {"location": (center+[.08*width, -2*width, .12*width]).tolist(), "target": center.tolist(), "width": width*1.25},
        "deck": {"location": [center[0]+.14*width, low[1]-width, low[2]+12.], "target": center.tolist(), "width": width*1.30},
        "oblique": {"location": (center+[.9*width, -1.8*width, .85*width]).tolist(), "target": center.tolist(), "width": width*1.80},
        "close_wall": {"location": (focus+[0., -180., 5.]).tolist(), "target": focus.tolist(), "width": 110.},
        "close_oblique": {"location": (focus+[110., -150., 85.]).tolist(), "target": focus.tolist(), "width": 135.},
        "far_crater": {"location": far.tolist(), "target": center.tolist(), "width": width*1.8,
                       "perspective_lens_mm": 35., "sensor_width_mm": 36., "horizontal_range_m": 1500.,
                       "context": "Isolated sector, 1500m horizontal range. Reference bar100m and posts10m; no surrounding crater or atmosphere implied."},
    }


def rock_review_material(source_dir, tile_metres):
    """Optional co-registered Poly Haven surface shading; never displace geometry."""
    import bpy
    material = bpy.data.materials.new("Poly Haven marble_cliff_01; native physical tile")
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    shader = nodes.get("Principled BSDF")
    uv = nodes.new("ShaderNodeUVMap")
    uv.uv_map = "CliffSourceUZ"
    report = {"source_dir": str(source_dir.resolve()), "tile_metres": tile_metres,
              "coordinates": "Original source-chart U/Z metres, shared UV for every map",
              "face_selection": "face_kind == 1 (exposed cut); other faces retain neutral clay",
              "geometry_displacement": False, "normal_strength": 1., "maps": {}}
    for role, filename, space in (
            ("diffuse", "marble_cliff_01_diff_2k.jpg", "sRGB"),
            ("normal", "marble_cliff_01_nor_gl_2k.exr", "Non-Color"),
            ("roughness", "marble_cliff_01_rough_2k.exr", "Non-Color")):
        path = source_dir/filename
        image = bpy.data.images.load(str(path.resolve()), check_existing=True)
        image.colorspace_settings.name = space
        texture = nodes.new("ShaderNodeTexImage")
        texture.image, texture.extension = image, "REPEAT"
        links.new(uv.outputs["UV"], texture.inputs["Vector"])
        if role == "normal":
            normal = nodes.new("ShaderNodeNormalMap")
            normal.space, normal.uv_map = "TANGENT", "CliffSourceUZ"
            normal.inputs["Strength"].default_value = 1.
            links.new(texture.outputs["Color"], normal.inputs["Color"])
            links.new(normal.outputs["Normal"], shader.inputs["Normal"])
        else:
            links.new(texture.outputs["Color"], shader.inputs["Base Color" if role == "diffuse" else "Roughness"])
        report["maps"][role] = {"path": str(path.resolve()), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                                "colorspace": space, "size_pixels": list(image.size)}
    return material, report


def render_pair(baseline, candidate, output, cameras, samples, mode, angle, context_water=None, rock_source=None, rock_tile=4.299351692199707):
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
    if context_water is not None:
        water_mesh = bpy.data.meshes.new('Measured mean receiving water; no waves')
        water_mesh.from_pydata(context_water['chart'].tolist(), [], context_water['triangles'].tolist())
        water_mesh.update()
        water_object = bpy.data.objects.new(water_mesh.name, water_mesh)
        scene.collection.objects.link(water_object)
        water_material = bpy.data.materials.new('Diagnostic mean water; no animated shading')
        water_material.use_nodes = True
        water_shader = water_material.node_tree.nodes.get('Principled BSDF')
        water_shader.inputs['Base Color'].default_value = (.035, .09, .12, 1)
        water_shader.inputs['Roughness'].default_value = .6
        water_mesh.materials.append(water_material)
    rock_material, rock_report = rock_review_material(rock_source, rock_tile) if rock_source else (None, None)
    objects, normal_reports = {}, {}
    for stage, data in (("baseline", baseline), ("fractured", candidate)):
        mesh = bpy.data.meshes.new(stage)
        mesh.from_pydata(data["chart"].tolist(), [], data["triangles"].tolist())
        mesh.update()
        obj = bpy.data.objects.new(stage, mesh)
        scene.collection.objects.link(obj)
        mesh.materials.append(material)
        if rock_material:
            kinds = data.get("face_kind")
            if kinds is None or np.asarray(kinds).shape != (len(mesh.polygons),):
                raise RuntimeError("Rock review requires an explicit face_kind for each source triangle")
            mesh.materials.append(rock_material)
            mesh.polygons.foreach_set("material_index", (np.asarray(kinds) == 1).astype(np.int32))
            uv = mesh.uv_layers.new(name="CliffSourceUZ")
            coordinates = data["chart"][data["triangles"].ravel()][:, [0, 2]]/rock_tile
            uv.data.foreach_set("uv", coordinates.astype(np.float32).ravel())
        normal_reports[stage] = configure_mesh_normals(mesh, mode, angle)
        obj.hide_render = True
        objects[stage] = obj
    for name, view in cameras.items():
        camera = camera_at(scene, view["location"], view["target"], view["width"])
        perspective = "perspective_lens_mm" in view
        markers = []
        if perspective:
            camera.data.type = "PERSP"
            camera.data.lens = view["perspective_lens_mm"]
            camera.data.sensor_width = view["sensor_width_mm"]
            # label_camera draws text 5m from the camera. Match that plane's
            # physical width; changing this does not alter camera FOV.
            camera.data.ortho_scale = 5.*camera.data.sensor_width/camera.data.lens
            low = baseline["chart"].min(axis=0)
            start = np.array([low[0], low[1]-20., low[2]+2.])
            for offset, dimensions in (([50, 0, 0], [100, 1., 1.]), ([0, 0, 5], [1., 1., 10.]), ([100, 0, 5], [1., 1., 10.])):
                bpy.ops.mesh.primitive_cube_add(size=1., location=start+offset)
                marker = bpy.context.object
                marker.name = "Metric reference only"
                marker.dimensions = dimensions
                marker.data.materials.append(material)
                markers.append(marker)
        for stage, data in (("baseline", baseline), ("fractured", candidate)):
            objects[stage].hide_render = False
            dims = np.ptp(data["chart"], axis=0)
            title = "ORIGINAL TRIANGLE SURFACE" if data.get("surface_mode") else "TRUE 3D OUTCROP"
            stage_label = "CANDIDATE" if data.get("surface_mode") and stage == "fractured" else stage.upper()
            label_camera(scene, camera, f"{title} | {stage_label} | {name}",
                         (f"Range {view['horizontal_range_m']:.0f}m | 35mm / 36mm sensor | bar100m, posts10m" if perspective else
                          f"Physical bounds {dims[0]:.1f} x {dims[1]:.1f} x {dims[2]:.1f} m | identical camera and raking light"),
                         f"Source {data['sha256'][:10]} | normals {mode}"+(f" {angle:g}deg" if mode=="angle" else "")+
                         (" | measured mean water, no waves" if context_water is not None else
                          " | renderer adds no displacement or geometry smoothing")+
                         (f" | cut: actual rock maps, {rock_tile:.3f}m tile" if rock_material else ""))
            scene.render.filepath = str(output/f"{name}_{stage}.png")
            bpy.ops.render.render(write_still=True)
            for child in list(camera.children):
                bpy.data.objects.remove(child, do_unlink=True)
            objects[stage].hide_render = True
        bpy.data.objects.remove(camera, do_unlink=True)
        for marker in markers:
            bpy.data.objects.remove(marker, do_unlink=True)
    if rock_report:
        normal_reports["surface_material"] = rock_report
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
    parser.add_argument("--open-surface", action="store_true", help="Audit an open terrain surface without making a closed-solid/volume claim")
    parser.add_argument("--close-center", type=float, nargs=3, metavar=("X", "Y", "Z"), help="Close focus in the shared source chart, metres")
    parser.add_argument("--views", nargs="+", help="Render selected named cameras only")
    parser.add_argument("--audit-only", action="store_true")
    parser.add_argument("--context-water", type=Path, help="Optional original-frame measured mean-water mesh; context only, no terrain edits")
    parser.add_argument("--rock-source", type=Path, help="Optional verified Poly Haven marble_cliff_01 maps; exposed face_kind1 only, no displacement")
    parser.add_argument("--rock-tile-metres", type=float, default=4.299351692199707, help="Physical tile width/height from the source blend; never scene-scale texture magnification")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else None)
    output = args.output.resolve()
    if "saved" not in [p.lower() for p in output.parts]:
        parser.error("Generated output must be under ignored Saved")
    if not 1 <= args.samples <= 128 or not 0 < args.normal_angle_deg < 180:
        parser.error("Invalid samples or normal angle")
    if not np.isfinite(args.rock_tile_metres) or args.rock_tile_metres <= 0:
        parser.error("Physical rock tile size must be positive and finite")
    baseline, candidate = load_mesh(args.baseline), load_mesh(args.candidate)
    context_water = load_mesh(args.context_water) if args.context_water else None
    if context_water is not None:
        if not np.array_equal(baseline['origin'], context_water['origin']) or not np.array_equal(baseline['basis'], context_water['basis']):
            raise RuntimeError('Measured water context must use the exact terrain frame')
    if not np.allclose(baseline["origin"], candidate["origin"], atol=1.e-8) or not np.allclose(baseline["basis"], candidate["basis"], atol=1.e-8):
        raise RuntimeError("Baseline/candidate frames differ")
    output.mkdir(parents=True, exist_ok=True)
    report = {"schema": "directorscut.cliff_outcrop_review.v1", "source": {},
              "frame_origin_world_m": baseline["origin"].tolist(), "chart_basis_columns": baseline["basis"].tolist(),
              "geometry_operation": "Rigid translation/rotation only; no XY projection or repair", "runtime_integrated": False}
    if context_water is not None:
        report['context_water'] = dict(path=context_water['path'], sha256=context_water['sha256'],
                                      metadata=context_water['metadata'],
                                      limitation='Separate measured analytic mean-water surface; no animated waves or terrain edits')
    for stage, data in (("baseline", baseline), ("fractured", candidate)):
        data["surface_mode"] = args.open_surface
        report["source"][stage] = {"path": data["path"], "sha256": data["sha256"], "metadata": data["metadata"], "audit": topology_audit(data, args.open_surface)}
    cameras = json.loads(args.camera_manifest.read_text())["cameras"] if args.camera_manifest else (
        surface_views(baseline, candidate, args.close_center) if args.open_surface else matched_views(baseline, candidate))
    if args.views:
        if set(args.views)-set(cameras):
            parser.error("Unknown camera name")
        cameras = {name: cameras[name] for name in args.views}
    report["cameras"] = cameras
    report["crop_boundary"] = surface_boundary_audit(baseline, candidate) if args.open_surface else crop_boundary_audit(baseline, candidate)
    pass_key = "valid_open_manifold_surface" if args.open_surface else "closed_consistently_oriented"
    report["audit_passed"] = all(s["audit"][pass_key] for s in report["source"].values())
    if args.open_surface:
        report["audit_passed"] &= report["crop_boundary"]["boundary_edge_geometry_preserved"]
    # Invalid candidates may be rendered diagnostically, but never silently
    # repaired or certified. Their exact defects stay visible in the report.
    if not args.audit_only:
        report["normals"] = render_pair(baseline, candidate, output, cameras, args.samples, args.normal_mode, args.normal_angle_deg, context_water, args.rock_source, args.rock_tile_metres)
        report["render"] = {"engine": "Cycles", "device": "CPU", "threads": 8, "samples": args.samples, "normal_mode": args.normal_mode}
    (output/"review.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "audit_passed": report["audit_passed"],
                      "audits": {k: v["audit"] for k, v in report["source"].items()}}, indent=2))


if __name__ == "__main__":
    main()
