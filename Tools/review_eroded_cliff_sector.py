"""Render measured erosion seeds/results with matched CPU-only clay cameras.

Run with installed Blender, no packages or runtime assets are modified:
  blender --background --factory-startup --threads 8 --python THIS_FILE -- \
    --seed Saved/.../cliff_erosion_seed.npz --eroded Saved/.../eroded.npz \
    --output Saved/CliffErosion/Review --obj

All heights/coordinates are metres. Clean macro landforms are labelled separately
from measured uppermost-XY projections. Original 3D profiles are optional, never
invented for a clean seed. Omit --eroded for a seed-only review. No vertical
exaggeration, displacement material, normal texture or GPU is used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np


def load_pair(seed_path, result_path):
    def load(path):
        with np.load(path, allow_pickle=False) as data:
            return {key: data[key].copy() for key in data.files}
    seed = load(seed_path)
    result = load(result_path) if result_path else dict(seed)
    if not result_path:
        for name in ("eroded_m", "deposited_m", "flow_accumulation"):
            result[name] = np.zeros_like(seed["height_m"])
    shape = seed["height_m"].shape
    if len(shape) != 2 or min(shape) < 3:
        raise ValueError("Expected a 2D heightfield at least 3x3")
    for key in ("height_m", "eroded_m", "deposited_m", "flow_accumulation", "protected_mask"):
        if result[key].shape != shape or not np.isfinite(result[key]).all():
            raise ValueError("Invalid result array: " + key)
    for key in ("cell_size_m", "world_xy_m", "frame_origin_world_m", "basis_u_world", "basis_v_world", "heightfield_valid_mask", "protected_mask"):
        if not np.array_equal(seed[key], result[key]):
            raise ValueError("Seed/result frame or protection mismatch: " + key)
    fixed = seed["protected_mask"] == 1
    if not np.array_equal(seed["height_m"][fixed], result["height_m"][fixed]):
        raise ValueError("Protected heights changed")
    if np.any(result["height_m"] < seed["bedrock_m"] - 1.e-5):
        raise ValueError("Result penetrates bedrock")
    return seed, result


def source_info(seed):
    metadata = json.loads(str(seed.get("metadata_json", "{}")))
    provenance = metadata.get("provenance", {})
    kind = metadata.get("source_kind", provenance.get("source_kind", "measured_uppermost_xy"))
    clean = kind == "clean_macro_landform"
    reference = provenance.get("geometry_reference") or provenance.get("source_head", "unspecified")[:8]
    return dict(kind=kind, clean=clean, reference=reference,
                title="CLEAN MACRO LANDFORM" if clean else "MEASURED CLIFF HEIGHTFIELD",
                limitation=("Authored heightfield; no overhangs. Physical 1:1 scale; no surface-detail shader." if clean else
                            "Uppermost XY projection; lower overhang surfaces omitted. Physical 1:1 scale."))


def triangles(valid):
    """Both fixed cell diagonals; never bridge invalid padding."""
    rows, cols = valid.shape
    index = np.arange(rows*cols).reshape(rows, cols)
    a, b = index[:-1, :-1], index[:-1, 1:]
    c, d = index[1:, :-1], index[1:, 1:]
    one = np.stack((a, b, d), axis=-1)[valid[:-1, :-1] & valid[:-1, 1:] & valid[1:, 1:]]
    two = np.stack((a, d, c), axis=-1)[valid[:-1, :-1] & valid[1:, 1:] & valid[1:, :-1]]
    return np.concatenate((one, two))


def raw_profile_geometry(seed):
    """Same source profile vertices; crop whole triangles to the exported XY tile."""
    xyz = seed["raw_profile_xyz_world_m"]
    delta = xyz-seed["frame_origin_world_m"]
    cell = float(seed["cell_size_m"])
    height, width = seed["height_m"].shape
    size_x, size_y = (width-1)*cell, (height-1)*cell
    chart = np.stack((delta@seed["basis_u_world"]-size_x*.5,
                      delta@seed["basis_v_world"]-size_y*.5, xyz[..., 2]), axis=-1).reshape(-1, 3)
    index = np.arange(xyz.shape[0]*xyz.shape[1]).reshape(xyz.shape[:2])
    a, b, c, d = index[:-1, :-1], index[1:, :-1], index[:-1, 1:], index[1:, 1:]
    # Production uses the b-c diagonal; reverse UE winding for Blender normals.
    faces = np.concatenate((np.stack((a, c, b), axis=-1).reshape(-1, 3),
                            np.stack((b, c, d), axis=-1).reshape(-1, 3)))
    centers = chart[faces].mean(axis=1)
    faces = faces[(np.abs(centers[:, 0]) <= size_x*.5) & (np.abs(centers[:, 1]) <= size_y*.5)]
    return chart, faces


def export_obj(path, seed, height, faces):
    xyz = np.dstack((seed["world_xy_m"], height)).reshape(-1, 3)
    with path.open("w", encoding="utf-8", newline="\n") as out:
        out.write("# "+source_info(seed)["title"]+"; absolute design-world metres.\n")
        out.write("# Invalid padding faces omitted. See review.json for provenance and frame.\n")
        for x, y, z in xyz:
            out.write(f"v {x:.8f} {y:.8f} {z:.8f}\n")
        for a, b, c in faces+1:
            out.write(f"f {a} {b} {c}\n")


def emission(name, color):
    import bpy
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    shader = tree.nodes.new("ShaderNodeEmission")
    shader.inputs["Color"].default_value = (*color, 1)
    tree.links.new(shader.outputs[0], tree.nodes.new("ShaderNodeOutputMaterial").inputs["Surface"])
    return material


def reset_scene(width, height, samples):
    import bpy
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = samples
    scene.cycles.use_denoising = False
    scene.cycles.max_bounces = 2
    scene.cycles.diffuse_bounces = 1
    scene.render.threads_mode = "FIXED"
    scene.render.threads = 8
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    scene.view_settings.exposure = 0
    scene.view_settings.gamma = 1
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs[0].default_value = (.07, .07, .07, 1)
    scene.world.node_tree.nodes["Background"].inputs[1].default_value = .5
    return scene


def camera_at(scene, location, target, scale):
    import bpy
    from mathutils import Vector
    camera = bpy.data.objects.new("Matched camera", bpy.data.cameras.new("Matched camera"))
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.location = location
    camera.rotation_euler = (Vector(target)-camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.type = "ORTHO"
    camera.data.sensor_fit = "HORIZONTAL"
    camera.data.ortho_scale = scale
    camera.data.clip_end = 10000
    return camera


def text_object(scene, value, location, size, material, parent=None):
    import bpy
    curve = bpy.data.curves.new("Diagnostic label", type="FONT")
    curve.body, curve.size = value, size
    obj = bpy.data.objects.new("Diagnostic label", curve)
    scene.collection.objects.link(obj)
    obj.parent = parent
    obj.location = location
    curve.materials.append(material)
    return obj


def label_camera(scene, camera, title, subtitle, footer):
    width = camera.data.ortho_scale
    height = width*scene.render.resolution_y/scene.render.resolution_x
    ink = emission("Labels", (.85, .85, .85))
    for value, y, size in ((title, .45, .032), (subtitle, .405, .021), (footer, -.46, .020)):
        text_object(scene, value, (-width*.47, height*y, -5), height*size, ink, camera)


def render_clay(seed, result, faces, out, samples, has_result=True, requested_views=("wall", "oblique")):
    import bpy
    from mathutils import Vector
    scene = reset_scene(1440, 960, samples)
    cell = float(seed["cell_size_m"])
    rows, cols = seed["height_m"].shape
    width, depth = (cols-1)*cell, (rows-1)*cell
    x, y = np.meshgrid(np.arange(cols)*cell-width*.5, np.arange(rows)*cell-depth*.5)
    info = source_info(seed)
    mesh = bpy.data.meshes.new(f"Physical {cell:g}m heightfield")
    mesh.from_pydata(np.stack((x, y, seed["height_m"]), axis=-1).reshape(-1, 3).tolist(), [], faces.tolist())
    mesh.update()
    obj = bpy.data.objects.new(info["title"], mesh)
    scene.collection.objects.link(obj)
    clay = bpy.data.materials.new("Neutral rough clay, no detail shader")
    clay.use_nodes = True
    shader = clay.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (.33, .33, .33, 1)
    shader.inputs["Roughness"].default_value = .85
    shader.inputs["Specular IOR Level"].default_value = .2
    obj.data.materials.append(clay)
    # Keep triangulated geometric facets visible; no smoothing can hide grid artifacts.
    for polygon in mesh.polygons:
        polygon.use_smooth = False
    raw_mesh = None
    if "raw_profile_xyz_world_m" in seed:
        raw_vertices, raw_faces = raw_profile_geometry(seed)
        raw_mesh = bpy.data.meshes.new("Original measured profile triangles with overhangs")
        raw_mesh.from_pydata(raw_vertices.tolist(), [], raw_faces.tolist())
        raw_mesh.materials.append(clay)
        raw_mesh.update()
    sun = bpy.data.objects.new("Fixed raking light", bpy.data.lights.new("Fixed raking light", "SUN"))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.angle = 2.0, .12
    sun.rotation_euler = Vector((.7, .45, -.6)).to_track_quat("-Z", "Y").to_euler()
    active_y, active_x = np.where(seed["erodibility"] > 0)
    if not len(active_y):
        active_y, active_x = np.where(seed["heightfield_valid_mask"] > 0)
    target_y = float((active_y.min()+active_y.max())*.5*cell-depth*.5)
    target_z = float(np.median(seed["height_m"][seed["erodibility"] > 0]))
    views = {
        "wall": ((width*.16, target_y-width*1.8, target_z+width*.25), (0, target_y, target_z), width*1.18),
        "oblique": ((width*.80, -depth*1.35, target_z+width*.85), (0, -depth*.10, target_z), width*2.00),
        "deck": ((width*.14, -depth*1.3, float(np.percentile(seed["height_m"][seed["heightfield_valid_mask"] > 0], 8))+12),
                 (0, target_y, target_z), width*1.25),
        "plan": ((0, 0, float(seed["height_m"].max())+1000), (0, 0, target_z), max(width, depth)*1.60),
    }
    manifest = {}
    for view, (location, target, scale) in views.items():
        if view not in requested_views:
            continue
        camera = camera_at(scene, location, target, scale)
        manifest[view] = dict(location_chart_m=list(location), target_chart_m=list(target), orthographic_width_m=scale)
        stages = [("before", seed["height_m"])]
        if has_result:
            stages.append(("after", result["height_m"]))
        if raw_mesh is not None:
            stages.append(("raw_profiles", None))
        for stage, height in stages:
            obj.data = raw_mesh if height is None else mesh
            if height is not None:
                xyz = np.stack((x, y, height), axis=-1).reshape(-1, 3).astype(np.float32)
                mesh.vertices.foreach_set("co", xyz.ravel())
                mesh.update()
            footer = ("Original measured profile triangles: retains overhangs. Same physical scale, camera and clay."
                      if height is None else info["limitation"])
            label_camera(scene, camera, f"DIAGNOSTIC {stage.upper()} | {view} | {info['title']}",
                         f"{width:.0f} x {depth:.0f} m / source {info['reference']} / identical camera and raking light",
                         footer)
            scene.render.filepath = str(out / f"{view}_{stage}.png")
            bpy.ops.render.render(write_still=True)
            for child in list(camera.children):
                bpy.data.objects.remove(child, do_unlink=True)
        bpy.data.objects.remove(camera, do_unlink=True)
    return manifest


def colorize(field, maximum, palette):
    value = np.clip(np.asarray(field)/max(maximum, 1.e-12), 0, 1)
    stops = np.asarray(palette)
    position = value*(len(stops)-1)
    lower = np.minimum(position.astype(int), len(stops)-2)
    return stops[lower]*(1-(position-lower))[..., None]+stops[lower+1]*(position-lower)[..., None]


def map_plane(scene, name, rgb, center, size):
    import bpy
    height, width = rgb.shape[:2]
    image = bpy.data.images.new(name, width=width, height=height, alpha=True)
    image.colorspace_settings.name = "Non-Color"
    pixels = np.dstack((rgb, np.ones((height, width)))).astype(np.float32)
    image.pixels.foreach_set(pixels.ravel())
    image.update()
    material = emission(name, (1, 1, 1))
    shader = next(n for n in material.node_tree.nodes if n.type == "EMISSION")
    texture = material.node_tree.nodes.new("ShaderNodeTexImage")
    texture.image, texture.interpolation = image, "Closest"
    material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Color"])
    bpy.ops.mesh.primitive_plane_add(size=2, location=(*center, 0))
    plane = bpy.context.object
    plane.scale = (size[0]*.5, size[1]*.5, 1)
    plane.data.materials.append(material)


def render_maps(seed, result, out, has_result=True):
    import bpy
    scene = reset_scene(1800, 1800, 1)
    camera_at(scene, (0, 0, 10), (0, 0, 0), 3.35)
    ink = emission("Map labels", (.9, .9, .9))
    valid = seed["heightfield_valid_mask"] > 0
    warm = ((.015, .02, .025), (.05, .12, .3), (.8, .19, .04), (1, .85, .45))
    cool = ((.015, .02, .025), (.025, .18, .3), (.03, .65, .8), (.8, 1, 1))
    flow = np.log1p(result["flow_accumulation"])
    cuts, deposits = result["eroded_m"], result["deposited_m"]
    delta = result["height_m"]-seed["height_m"]
    delta_max = float(np.abs(delta[valid]).max())
    delta_colors = colorize((delta+delta_max)*.5, delta_max, ((.02, .25, 1), (.14, .14, .14), (1, .3, .03)))
    protection = np.zeros((*valid.shape, 3)) + (.04, .35, .12)
    protection[seed["protected_mask"] == 1] = (.65, .26, .025)
    maps = [
        ("FLOW / log(1 + cumulative outgoing m3)", colorize(flow, float(flow[valid].max()), cool), f"black 0  ->  white {float(result['flow_accumulation'][valid].max()):.2f} m3 (log scale)"),
        ("GROSS EROSION / transported thickness", colorize(cuts, float(cuts[valid].max()), warm), f"black 0  ->  yellow {float(cuts[valid].max()):.3f} m"),
        ("GROSS DEPOSITION / transported thickness", colorize(deposits, float(deposits[valid].max()), cool), f"black 0  ->  white {float(deposits[valid].max()):.3f} m"),
        ("PROTECTION / physical domain only", protection, "green erodible / orange fixed / grey invalid"),
        ("NET HEIGHT CHANGE / after - before", delta_colors, f"blue {-delta_max:.3f} m  /  grey 0  /  orange +{delta_max:.3f} m"),
    ]
    if "discarded_vertical_span_m" in seed:
        span=seed["discarded_vertical_span_m"]
        maps.append(("OVERHANG LOSS / highest - lowest hit", colorize(span, float(span.max()), warm), f"black 0 -> yellow {float(span.max()):.2f} m"))
    else:
        outlets=result.get("outflow_sediment_m3", seed.get("outflow_mask", np.zeros_like(cuts)))
        maps.append(("OUTFLOW / sediment m3" if "outflow_sediment_m3" in result else "OUTFLOW / declared receiving cells",
                     colorize(outlets, max(float(outlets.max()), 1.e-12), warm), f"black 0 -> yellow {float(outlets.max()):.3g}"))
    if not has_result:
        h=seed["height_m"]
        floor=float(h[valid].min()); relief=float(h[valid].max()-floor)
        capacity=seed["height_m"]-seed["bedrock_m"]
        maps[0]=("SEED HEIGHT / absolute metres", colorize(h-floor, relief, cool), f"{floor:.1f} -> {floor+relief:.1f} m")
        maps[1]=("ERODIBILITY / input only", colorize(seed["erodibility"], 1, warm), "0 fixed rock -> 1 erodible")
        maps[2]=("AVAILABLE CUT / above immutable bedrock", colorize(capacity, float(capacity.max()), cool), f"0 -> {float(capacity.max()):.2f} m")
        maps[4]=("EROSION NOT RUN", np.zeros_like(protection)+(.10, .10, .10), "No erosion result supplied")
    for i, (title, colors, legend) in enumerate(maps):
        center = ((i % 2-.5)*1.65, 1.05-(i//2)*1.04)
        colors[~valid] = (.085, .085, .085)
        aspect = (valid.shape[1]-1)/(valid.shape[0]-1)
        map_plane(scene, title, colors, center, (.82*aspect, .82))
        text_object(scene, title, (center[0]-.67, center[1]+.45, .01), .034, ink)
        text_object(scene, legend, (center[0]-.67, center[1]-.46, .01), .027, ink)
    info=source_info(seed)
    text_object(scene, f"{info['title']} / {info['reference']} | radial outward up / tangent right", (-1.56, 1.57, .01), .034, ink)
    cell = float(seed["cell_size_m"])
    width, depth = (valid.shape[1]-1)*cell, (valid.shape[0]-1)*cell
    text_object(scene, f"Each panel: {width:g} x {depth:g} m. Grey is omitted padding. "+("Clean authored heightfield; no projected folds." if info["clean"] else "Uppermost surface only; original overhangs are lost."), (-1.56, -1.59, .01), .030, ink)
    scene.render.filepath = str(out / "diagnostic_maps.png")
    bpy.ops.render.render(write_still=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=Path, required=True)
    parser.add_argument("--eroded", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--samples", type=int, default=16)
    parser.add_argument("--obj", action="store_true")
    parser.add_argument("--views", default="wall,oblique", help="Comma-separated wall,oblique,deck,plan")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else None)
    out = args.output.resolve()
    if "saved" not in [part.lower() for part in out.parts]:
        parser.error("Generated diagnostics must be under ignored Saved")
    if not 1 <= args.samples <= 128:
        parser.error("Samples must be 1..128")
    requested_views=args.views.split(",")
    if any(view not in ("wall", "oblique", "deck", "plan") for view in requested_views):
        parser.error("Unknown view")
    seed, result = load_pair(args.seed, args.eroded)
    faces = triangles(seed["heightfield_valid_mask"] > 0)
    out.mkdir(parents=True, exist_ok=True)
    if args.obj:
        export_obj(out / "before_heightfield.obj", seed, seed["height_m"], faces)
        if args.eroded:
            export_obj(out / "after_heightfield.obj", seed, result["height_m"], faces)
    cameras = render_clay(seed, result, faces, out, args.samples, bool(args.eroded), requested_views)
    render_maps(seed, result, out, bool(args.eroded))
    delta = result["height_m"].astype(np.float64)-seed["height_m"]
    review = dict(approximation=source_info(seed)["limitation"], source_kind=source_info(seed)["kind"], erosion_result_supplied=bool(args.eroded),
                  seed_path=str(args.seed.resolve()), result_path=str(args.eroded.resolve()) if args.eroded else None,
                  seed_sha256=hashlib.sha256(args.seed.read_bytes()).hexdigest(), result_sha256=hashlib.sha256(args.eroded.read_bytes()).hexdigest() if args.eroded else None,
                  coordinate_frame=json.loads(str(seed["metadata_json"])), cameras=cameras,
                  render_chart="x=clockwise tangent minus half width, y=outward radial minus half depth, z=absolute design-world metres",
                  obj_frame="Absolute design-world XY and Z in metres; same origin/basis as seed; invalid faces omitted",
                  triangle_count=len(faces), max_net_cut_m=float(-delta.min()), max_net_deposit_m=float(delta.max()),
                  eroded_metrics=json.loads(str(result["metrics_json"])) if "metrics_json" in result else {})
    import bpy
    review["render_backend"] = dict(blender=bpy.app.version_string, engine="Cycles", device="CPU", threads=8,
                                    clay_samples=args.samples, flat_shading=True, world_strength=.5, sun_energy=2.0,
                                    sun_angle_rad=.12, clay_roughness=.85, clay_base_color_linear=[.33, .33, .33],
                                    view_transform="Standard", vertical_exaggeration=1, imported_into_game=False)
    (out / "review.json").write_text(json.dumps(review, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(dict(output=str(out), triangles=len(faces), max_cut_m=review["max_net_cut_m"], max_deposit_m=review["max_net_deposit_m"]), indent=2))


if __name__ == "__main__":
    main()
