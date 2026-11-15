"""CPU clay review of original OBJ rock/bedrock meshes at declared physical scale.

Blender --background --factory-startup --threads 8 --python THIS.py --
  --obj mesh.obj --metres-per-unit 1 --up-axis Y --output Saved/RockReview

No export, remesh, displacement, texture, or game integration. Optional baseline
and saved cameras use the same physical transform. A separate flat oblique view
exposes geometric artifacts hidden by imported source normals. The 250m deck
distance is an explicit review assumption, not a measured gameplay camera.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
import numpy as np
import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from review_eroded_cliff_sector import reset_scene, camera_at, label_camera


def clay(name, value):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (value, value, value, 1)
    shader.inputs["Roughness"].default_value = .85
    shader.inputs["Specular IOR Level"].default_value = .2
    return material


def load_obj(path, up, metres_per_unit, material):
    before = set(bpy.data.objects)
    bpy.ops.wm.obj_import(filepath=str(path), up_axis=up,
                         forward_axis="NEGATIVE_Z" if up == "Y" else "NEGATIVE_Y",
                         global_scale=metres_per_unit)
    objects = [o for o in bpy.data.objects if o not in before and o.type == "MESH"]
    if not objects:
        raise RuntimeError(f"No mesh in {path}")
    points, counts = [], {"vertices": 0, "triangles": 0}
    for obj in objects:
        mesh = obj.data
        xyz = np.empty((len(mesh.vertices), 3), dtype=np.float32)
        mesh.vertices.foreach_get("co", xyz.ravel())
        matrix = np.asarray(obj.matrix_world)
        points.append(xyz @ matrix[:3, :3].T + matrix[:3, 3])
        mesh.calc_loop_triangles()
        counts["vertices"] += len(mesh.vertices)
        counts["triangles"] += len(mesh.loop_triangles)
        mesh.materials.clear()
        mesh.materials.append(material)
        for face in mesh.polygons:
            face.material_index = 0
        obj.hide_render = True
    points = np.concatenate(points).astype(np.float64)
    if not np.isfinite(points).all():
        raise RuntimeError("Non-finite imported positions")
    return {"objects": objects, "points": points, "counts": counts,
            "path": str(path.resolve()), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def reference_person(scene, base, material):
    """Simple scale marker, feet at0/head at1.8m; not a game character asset."""
    parts = []
    for x in (-.11, .11):
        bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=.075, depth=.8,
                                          location=Vector(base)+Vector((x, 0, .4)))
        parts.append(bpy.context.object)
    for location, size in (((0, 0, 1.13), (.38, .22, .66)),
                           ((-.27, 0, 1.10), (.13, .16, .58)),
                           ((.27, 0, 1.10), (.13, .16, .58)),
                           ((0, 0, 1.51), (.10, .10, .12))):
        bpy.ops.mesh.primitive_cube_add(size=1, location=Vector(base)+Vector(location))
        obj = bpy.context.object
        obj.dimensions = size
        parts.append(obj)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=.12,
                                       location=Vector(base)+Vector((0, 0, 1.68)))
    parts.append(bpy.context.object)
    for obj in parts:
        obj.name = "1.8m reference figure"
        obj.data.materials.append(material)


def flat_copy(obj):
    """Remove custom shading by rebuilding only an exact diagnostic mesh copy."""
    source = obj.data
    coordinates = np.empty((len(source.vertices), 3), np.float32)
    source.vertices.foreach_get("co", coordinates.ravel())
    original_loops = np.empty(len(source.loops), np.int32)
    source.loops.foreach_get("vertex_index", original_loops)
    result = bpy.data.meshes.new(source.name+"_flat_diagnostic")
    result.from_pydata(coordinates.tolist(), [], [list(p.vertices) for p in source.polygons])
    result.update()
    for material in source.materials:
        result.materials.append(material)
    check = np.empty_like(coordinates)
    result.vertices.foreach_get("co", check.ravel())
    loops = np.empty(len(result.loops), np.int32)
    result.loops.foreach_get("vertex_index", loops)
    if not np.array_equal(check, coordinates) or not np.array_equal(loops, original_loops):
        raise RuntimeError("Flat diagnostic changed original geometry")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--obj", type=Path, required=True)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--metadata", type=Path)
    parser.add_argument("--name", default="Implicit bedrock proof")
    parser.add_argument("--metres-per-unit", type=float, required=True)
    parser.add_argument("--up-axis", choices=("Y", "Z"), required=True)
    parser.add_argument("--deck-distance", type=float, default=250.)
    parser.add_argument("--samples", type=int, default=24)
    parser.add_argument("--camera-manifest", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else None)
    if args.metres_per_unit <= 0 or args.deck_distance <= 0 or not 1 <= args.samples <= 128:
        parser.error("Scale, deck distance and samples must be positive and bounded")
    output = args.output.resolve()
    if "saved" not in [p.lower() for p in output.parts]:
        parser.error("Generated output must stay in ignored Saved")
    output.mkdir(parents=True, exist_ok=True)
    scene = reset_scene(1440, 960, args.samples)
    material = clay("Neutral rough clay, no added detail", .33)
    inputs = {}
    if args.baseline:
        inputs["baseline"] = load_obj(args.baseline, args.up_axis, args.metres_per_unit, material)
    inputs["candidate"] = load_obj(args.obj, args.up_axis, args.metres_per_unit, material)
    reference = inputs.get("baseline", inputs["candidate"])["points"]
    low, high = reference.min(axis=0), reference.max(axis=0)
    translation = np.array([-(low[0]+high[0])*.5, -(low[1]+high[1])*.5, -low[2]])
    previous = json.loads(args.camera_manifest.read_text()) if args.camera_manifest else None
    if previous:
        translation = np.asarray(previous["authoring_translation_m"])
    for data in inputs.values():
        for obj in data["objects"]:
            obj.location += Vector(translation)
        data["points"] += translation
    low, high = reference.min(axis=0), reference.max(axis=0)
    # reference aliases the translated array above.
    center = (low+high)*.5
    width, depth, height = high-low
    span = float(max(width, depth, height))
    figure_base = np.asarray(previous["figure_base_m"]) if previous else np.array([low[0]-2., low[1]-2., 0.])
    reference_person(scene, figure_base, clay("Scale reference", .12))
    sun = bpy.data.objects.new("Fixed raking light", bpy.data.lights.new("Fixed raking light", "SUN"))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.angle = 2., .08
    sun.rotation_euler = Vector((.7, .45, -.6)).to_track_quat("-Z", "Y").to_euler()
    cameras = {
        "close": {"position": (center+[.06*span, -3*span, .08*span]).tolist(), "target": center.tolist(), "width": span*1.95},
        "oblique": {"position": [center[0]+1.9*span, center[1]-2.7*span, max(1.8, height*.20)],
                    "target": center.tolist(), "width": span*2.3},
        "deck": {"position": [center[0], low[1]-args.deck_distance, 8.], "target": center.tolist(),
                 "width": span*2., "lens_mm": 35., "sensor_mm": 36., "distance_to_front_m": args.deck_distance},
    }
    if previous:
        cameras = previous["cameras"]
    report = {"schema": "directorscut.implicit_cliff_clay.v1", "name": args.name, "metres_per_source_unit": args.metres_per_unit,
              "source_up_axis": args.up_axis,
              "source_exposed_face": "+Y mapped toward camera-local -Y" if args.up_axis == "Z" else "+Z mapped toward camera-local -Y",
              "scale_authority": "Explicit authoring declaration; must match generator metadata",
              "authoring_translation_m": translation.tolist(), "cameras": cameras, "reference_figure_height_m": 1.8,
              "figure_base_m": figure_base.tolist(), "deck_eye_height_m": 8., "deck_distance_is_assumption": True,
              "renderer": "Cycles CPU", "threads": 8, "samples": args.samples, "runtime_integrated": False,
              "geometry_changes": "Declared unit conversion and rigid axis rotation/translation only", "inputs": {}}
    if args.metadata:
        report["generator_metadata"] = {"path": str(args.metadata.resolve()), "sha256": hashlib.sha256(args.metadata.read_bytes()).hexdigest(),
                                        "content": json.loads(args.metadata.read_text())}
    for stage, data in inputs.items():
        report["inputs"][stage] = {k: data[k] for k in ("path", "sha256", "counts")}
        report["inputs"][stage]["dimensions_m"] = np.ptp(data["points"], axis=0).tolist()
        for view, camera_data in cameras.items():
            camera = camera_at(scene, camera_data["position"], camera_data["target"], camera_data["width"])
            if "lens_mm" in camera_data:
                camera.data.type = "PERSP"
                camera.data.lens = camera_data["lens_mm"]
                camera.data.sensor_width = camera_data["sensor_mm"]
                camera.data.ortho_scale = 5.*camera.data.sensor_width/camera.data.lens
            for obj in data["objects"]:
                obj.hide_render = False
            variants = ("source_normals", "flat_diagnostic") if view == "oblique" else ("source_normals",)
            for normal_mode in variants:
                originals = []
                if normal_mode == "flat_diagnostic":
                    for obj in data["objects"]:
                        originals.append((obj, obj.data))
                        obj.data = flat_copy(obj)
                dims = np.ptp(data["points"], axis=0)
                subtitle = f"Actual {dims[0]:.2f} x {dims[1]:.2f} x {dims[2]:.2f}m | figure1.8m | {normal_mode}"
                front_distance = float(data["points"][:, 1].min()-camera_data["position"][1])
                if view == "deck":
                    report["inputs"][stage]["deck_distance_to_current_front_m"] = front_distance
                footer = (f"Deck review: {front_distance:.1f}m from this front, eye8m, 35mm/36mm sensor" if view == "deck" else
                          "Original mesh | neutral clay | no added texture, displacement or geometry smoothing")
                label_camera(scene, camera, f"{args.name} | {stage.upper()} | {view}", subtitle, footer)
                scene.render.filepath = str(output/f"{stage}_{view}_{normal_mode}.png")
                bpy.ops.render.render(write_still=True)
                for child in list(camera.children):
                    bpy.data.objects.remove(child, do_unlink=True)
                for obj, original in originals:
                    temporary = obj.data
                    obj.data = original
                    bpy.data.meshes.remove(temporary)
            for obj in data["objects"]:
                obj.hide_render = True
            bpy.data.objects.remove(camera, do_unlink=True)
    (output/"review.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "inputs": report["inputs"]}, indent=2))


if __name__ == "__main__":
    main()
