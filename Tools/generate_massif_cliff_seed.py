"""Author connected 2D cliff massifs fitted only to semantic constraints.

Run with installed Blender --background --factory-startup --threads 8 --python
this_file -- --input historical_sector.npz --output Saved/Massif/seed.npz
[--anchors authoritative_arena_footprints.json]
[--boundary-source measured_sector.npz]. No scene, GPU or game changes.

The old wall interior, blanket plateau/apron locks, and radial relief window are
discarded. A mountain field continues across the face and hinterland. A screened
Poisson correction fits narrow physical coast, seams and optional support pads.
This remains an authored heightfield preview: no overhang/fracture or runtime
integration is implied, and the inherited coast is historical, not current UE.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def distance_to_points(xy, points):
    """Exact Euclidean arena-XY distance to explicit outlet grid vertices."""
    if len(points) == 0:
        raise ValueError("At least one explicit physical coastline outlet is required")
    flat = np.asarray(xy, np.float64).reshape(-1, 2)
    result = np.empty(len(flat), np.float64)
    for start in range(0, len(flat), 1024):
        delta = flat[start:start+1024, None, :] - points[None, :, :]
        result[start:start+1024] = np.sqrt(np.min(np.sum(delta*delta, axis=-1), axis=1))
    return result.reshape(xy.shape[:-1])


def polygon_distance(xy, polygon):
    """Unsigned outside distance (zero inside) for a validated convex polygon."""
    p = np.asarray(polygon, np.float64)
    if p.ndim != 2 or p.shape[1] != 2 or len(p) < 3 or not np.isfinite(p).all():
        raise ValueError("Anchor footprint must contain at least three finite XY points")
    edge = np.roll(p, -1, axis=0)-p
    length2 = np.sum(edge*edge, axis=1)
    cross = edge[:, 0]*np.roll(edge[:, 1], -1)-edge[:, 1]*np.roll(edge[:, 0], -1)
    if np.min(length2) <= 1.e-8 or not (np.all(cross > 1.e-8) or np.all(cross < -1.e-8)):
        raise ValueError("Anchor footprint must be strictly convex and consistently ordered")
    d = xy[..., None, :]-p
    side = edge[:, 0]*d[..., 1]-edge[:, 1]*d[..., 0]
    inside = np.all(side >= -1.e-8, axis=-1) | np.all(side <= 1.e-8, axis=-1)
    t = np.clip(np.sum(d*edge, axis=-1)/length2, 0, 1)
    closest = d-t[..., None]*edge
    distance = np.sqrt(np.min(np.sum(closest*closest, axis=-1), axis=-1))
    return np.where(inside, 0., distance)


def validate_boundary_source(data, boundary):
    """Clean macro seeds may have changed seams; accept only matched raw grids."""
    for key in ("world_xy_m", "heightfield_valid_mask", "cell_size_m", "frame_origin_world_m",
                "basis_u_world", "basis_v_world", "basis_up_world"):
        if key not in boundary or not np.array_equal(data[key],boundary[key]):
            raise ValueError(f"Measured boundary source has incompatible {key}")
    height = np.asarray(boundary["height_m"])
    if height.shape != data["height_m"].shape or not np.isfinite(height).all():
        raise ValueError("Measured boundary source has invalid heights")
    return height.astype(np.float32)


def constraints(data, anchors=None, coast_width_m=6., seam_width_m=12., boundary_height=None):
    xy = data["world_xy_m"].astype(np.float64)
    original = data["height_m"].astype(np.float32)
    valid = data["heightfield_valid_mask"] > 0
    outlets = data["outflow_mask"] > 0
    cell = float(data["cell_size_m"])
    if xy.shape != (*original.shape, 2) or not np.isfinite(xy).all() or not np.isfinite(original).all() or cell <= 0:
        raise ValueError("Invalid finite metric XY grid")
    if np.any(outlets & ~valid):
        raise ValueError("Outlets must be physical valid land cells")
    if not 4. <= coast_width_m <= 6. or seam_width_m < cell:
        raise ValueError("Preserve 4..6m coast and at least one cell of tile seams")
    coast_distance = distance_to_points(xy, xy[outlets])
    rows, cols = np.indices(original.shape)
    edge_distance = np.minimum.reduce((rows, cols, original.shape[0]-1-rows, original.shape[1]-1-cols))*cell
    kinds = (~valid).astype(np.uint8)
    kinds[(edge_distance <= seam_width_m) & valid] |= 2
    kinds[(coast_distance <= coast_width_m+1.e-6) & valid] |= 4
    target = original.copy() if boundary_height is None else np.asarray(boundary_height,np.float32).copy()
    reports = []
    anchor_json = anchors or {"schema":"directorscut.cliff_support_anchors.v1", "coordinate_frame":"arena_local_metres", "anchors":[]}
    if anchor_json.get("schema") != "directorscut.cliff_support_anchors.v1" or anchor_json.get("coordinate_frame") != "arena_local_metres":
        raise ValueError("Unsupported anchor schema or coordinate frame")
    origin = data["frame_origin_world_m"][:2]
    basis_u, basis_v = data["basis_u_world"][:2], data["basis_v_world"][:2]
    for anchor in anchor_json["anchors"]:
        z = float(anchor["support_height_m"])
        margin = float(anchor.get("margin_m", 10.))
        blend = float(anchor.get("blend_m", 30.))
        if not np.isfinite([z, margin, blend]).all() or margin < 0 or blend <= 0:
            raise ValueError("Support height/margins must be finite, with nonnegative margin and positive blend")
        polygon = np.asarray(anchor["footprint_xy_m"], np.float64)
        distance = polygon_distance(xy, polygon)
        # Include a cell diagonal beyond semantic support. This guarantees all
        # vertices of every covered footprint triangle are fixed to support Z.
        raster_margin = margin + np.sqrt(2.)*cell
        pad = (distance <= raster_margin) & valid
        conflict = pad & (kinds != 0) & (np.abs(target-z) > .05)
        if np.any(conflict & ((kinds&8)>0)):
            raise ValueError(f"Anchor {anchor.get('id')} overlaps another incompatible support height")
        # A partial landmark can cross an immutable tile seam. Preserve that
        # seam and record the unsupported portion; never silently claim the
        # tile can satisfy mutually inconsistent historical/current targets.
        conflict_count = int(conflict.sum())
        conflict_error = float(np.max(np.abs(target[conflict]-z),initial=0.))
        fitted_pad = pad & ((kinds&6)==0)
        local = np.column_stack(((polygon-origin)@basis_u, (polygon-origin)@basis_v))
        rectangle_covered = bool(np.all(local[:,0] >= raster_margin) and np.all(local[:,0] <= (original.shape[1]-1)*cell-raster_margin)
                                 and np.all(local[:,1] >= raster_margin) and np.all(local[:,1] <= (original.shape[0]-1)*cell-raster_margin))
        touches_invalid = bool(np.any((distance <= raster_margin) & ~valid))
        coverage = "outside" if not pad.any() else "full" if rectangle_covered and not touches_invalid else "partial"
        kinds[fitted_pad] |= 8
        target[fitted_pad] = np.float32(z)
        reports.append(dict(id=anchor.get("id"), coverage=coverage, fixed_samples=int(fitted_pad.sum()),
                            support_height_m=z, semantic_margin_m=margin, raster_margin_m=raster_margin,
                            preserved_seam_coast_conflict_samples=conflict_count, maximum_conflict_m=conflict_error,
                            requested_blend_m=blend, footprint_xy_m=polygon.tolist(),
                            full_footing_support_verified=False))
    return kinds, target, coast_distance, reports, anchor_json


def screened_fit(raw, target, valid, fixed, cell_m, fit_length_m=60., tolerance_m=2.e-4, max_iterations=1600):
    """Metric screened Laplace fit; missing land is a no-flux boundary.

    Red/black SOR solves (degree + cell²/length²)*correction = neighbor sum.
    Hard semantic targets remain exact; free terrain tends to the original 2D
    mountain field away from them. No radial interpolation or relief envelope.
    """
    raw = np.asarray(raw, np.float64)
    correction = np.zeros_like(raw)
    correction[fixed] = target[fixed]-raw[fixed]
    degree = np.zeros_like(raw)
    degree[1:] += valid[:-1]
    degree[:-1] += valid[1:]
    degree[:,1:] += valid[:,:-1]
    degree[:,:-1] += valid[:,1:]
    diagonal = degree + (cell_m/fit_length_m)**2
    rows, cols = np.indices(raw.shape)
    free = valid & ~fixed
    colors = [free & ((rows+cols)%2 == c) for c in (0,1)]
    delta_max = 0.
    for iteration in range(max_iterations):
        delta_max = 0.
        for color in colors:
            total = np.zeros_like(raw)
            total[1:] += correction[:-1]*valid[:-1]
            total[:-1] += correction[1:]*valid[1:]
            total[:,1:] += correction[:,:-1]*valid[:,:-1]
            total[:,:-1] += correction[:,1:]*valid[:,1:]
            step = 1.85*(total[color]/diagonal[color]-correction[color])
            correction[color] += step
            delta_max = max(delta_max, float(np.max(np.abs(step), initial=0.)))
        if delta_max < tolerance_m:
            break
    if delta_max >= tolerance_m:
        raise ValueError(f"Constraint fit did not converge: {delta_max:.6g}m step")
    height = (raw+correction).astype(np.float32)
    height[fixed] = target[fixed]
    return height, correction, dict(iterations=iteration+1, final_maximum_update_m=delta_max,
                                    fit_length_m=fit_length_m, tolerance_m=tolerance_m)


def mountain_field(data, valid, seed):
    from mathutils import Vector, noise
    xy = data["world_xy_m"].astype(np.float64)
    delta = xy-data["frame_origin_world_m"][:2]
    u, v = delta@data["basis_u_world"][:2], delta@data["basis_v_world"][:2]
    regional_z = float(np.median(data["constraint_upper_height_m"]))
    offsets = np.random.default_rng(seed).uniform(-200., 200., (5,3))
    raw = data["height_m"].astype(np.float64).copy()
    broad = np.zeros_like(raw)
    ridge = np.zeros_like(raw)
    for row, col in zip(*np.where(valid)):
        x, y = float(u[row,col]), float(v[row,col])
        wx = 22*noise.noise(Vector((x/240+offsets[0,0],y/240+offsets[0,1],offsets[0,2])), noise_basis="PERLIN_ORIGINAL")
        wy = 22*noise.noise(Vector((x/240+offsets[1,0],y/240+offsets[1,1],offsets[1,2])), noise_basis="PERLIN_ORIGINAL")
        x, y = x+wx, y+wy
        # This field spans ALL physical land, including the former plateau and
        # apron. The mountain network is never multiplied by a radial window.
        h = noise.hetero_terrain(Vector((x/220+offsets[2,0],y/220+offsets[2,1],offsets[2,2])),
                                 1.0, 2.0, 2.0, .65, noise_basis="PERLIN_ORIGINAL")
        r = noise.ridged_multi_fractal(Vector((x/96+offsets[3,0],y/96+offsets[3,1],offsets[3,2])),
                                        1.1, 2.0, 4.0, 1.0, 2.0, noise_basis="PERLIN_ORIGINAL")
        broad[row,col] = 35*np.tanh(1.6*(h-.85))
        ridge[row,col] = 22*np.tanh(1.5*(r-.95))
        raw[row,col] = regional_z + broad[row,col] + ridge[row,col]
    return raw, broad, ridge


def generate(data, seed=71421, anchors=None, coast_width_m=6., seam_width_m=12., fit_length_m=60., boundary=None):
    old_meta = json.loads(str(data["metadata_json"]))
    valid = data["heightfield_valid_mask"] > 0
    boundary_height = validate_boundary_source(data,boundary) if boundary is not None else None
    kinds, target, coast_distance, anchor_reports, anchor_json = constraints(data, anchors, coast_width_m, seam_width_m,boundary_height)
    fixed = kinds > 0
    raw, broad, ridge = mountain_field(data, valid, seed)
    height, correction, solver = screened_fit(raw, target, valid, fixed, float(data["cell_size_m"]), fit_length_m)
    active = valid & ~fixed
    erodibility = np.clip(.52+.10*broad/35, .35, .7)
    erodibility[fixed] = 0
    bedrock = (height.astype(np.float64)-(5.+5*erodibility)).astype(np.float32)
    bedrock[fixed] = height[fixed]
    result = {key:data[key].copy() for key in ("cell_size_m", "frame_origin_world_m", "basis_u_world", "basis_v_world", "basis_up_world",
              "world_xy_m", "u_m", "v_m", "shore_clearance_m", "heightfield_valid_mask", "outflow_mask")}
    result.update(height_m=height, bedrock_m=bedrock, erodibility=erodibility.astype(np.float32),
                  protected_mask=fixed.astype(np.float32), loose_m=(.06*erodibility).astype(np.float32),
                  constraint_type=kinds, constraint_target_height_m=target, coast_distance_m=coast_distance.astype(np.float32),
                  unconstrained_massif_height_m=raw.astype(np.float32), constraint_correction_m=correction.astype(np.float32),
                  massif_broad_relief_m=broad.astype(np.float32), massif_ridge_relief_m=ridge.astype(np.float32),
                  anchors_json=np.asarray(json.dumps(anchor_json)))
    gy, gx = np.gradient(height.astype(np.float64), float(data["cell_size_m"]))
    metrics = dict(grid_shape=list(height.shape), valid_cells=int(valid.sum()), active_cells=int(active.sum()),
                   fixed_physical_cells=int((valid&fixed).sum()), fixed_physical_fraction=float((valid&fixed).sum()/valid.sum()),
                   old_fixed_physical_fraction=float(((data["protected_mask"]>0)&valid).sum()/valid.sum()),
                   constraint_counts={name:int(((kinds&bit)>0).sum()) for name,bit in (("invalid",1),("seam",2),("coast",4),("temple",8))},
                   height_range_m=[float(height[valid].min()),float(height[valid].max())],
                   fixed_targets_bitwise_preserved=bool(np.array_equal(height[fixed].view(np.uint32),target[fixed].view(np.uint32))),
                   old_fixed_land_released_cells=int((valid & (data["protected_mask"]>0) & ~fixed).sum()),
                   slope_degrees_p50_p90_p99=list(map(float,np.percentile(np.degrees(np.arctan(np.hypot(gx,gy)))[active],[50,90,99]))),
                   anchors=anchor_reports, constraint_solver=solver, measured_boundary_source_supplied=boundary is not None, not_runtime_geometry=True,
                   temple_footing_compatibility_verified=False)
    if not np.isfinite(height).all() or np.any(bedrock>height) or not metrics["fixed_targets_bitwise_preserved"]:
        raise ValueError("Nonfinite terrain, crossed bedrock, or altered hard targets")
    settings = dict(seed=seed, coast_width_m=coast_width_m, seam_width_m=seam_width_m, fit_length_m=fit_length_m,
                    broad_scales_m=[220,110], broad_relief_bound_m=35, ridge_scales_m=[96,48,24,12], ridge_relief_bound_m=22,
                    domain_warp_scale_m=240, domain_warp_bound_m=22, coastline_distance_metric="Exact Euclidean XY distance to explicit outlet vertices",
                    anchor_blend_policy="Uniform screened-fit length; per-anchor blend_m recorded as authoring request, not an independent local falloff")
    boundary_provenance = json.loads(str(boundary["metadata_json"])).get("provenance",{}) if boundary is not None else {}
    meta = dict(schema="directorscut.massif_cliff_seed.v1", source_kind="massif_macro_landform",
                frame=old_meta["frame"], coordinate_frame=old_meta["coordinate_frame"], array_layout=old_meta["array_layout"],
                physical_units="metres; no vertical exaggeration", seed=seed, settings=settings, metrics=metrics,
                generator="Blender ridged/heterogeneous mountain field + screened 2D Dirichlet constraint fit",
                provenance=dict(input_seed_provenance=old_meta.get("provenance",{}), measured_boundary_provenance=boundary_provenance,
                                source_head=boundary_provenance.get("source_head"), geometry_reference=boundary_provenance.get("geometry_reference"),
                                position_functions_sha256=boundary_provenance.get("position_functions_sha256"),
                                anchors_geometry_sha1=anchor_json.get("source_geometry_sha1"),
                                use="Only frame/domain/outlets, narrow coast and seam heights reused; historical wall interior and blanket cap/apron locks discarded"),
                limitations="Authored XY heightfield preview, no overhangs or fracture, no runtime/mesh/waterfall integration. Boundary provenance is explicit; omitting measured boundary source retains potentially altered clean-seed targets. Anchor samples are fixed but full triangle footprint coverage requires independent audit. Old raw/profile grids intentionally omitted.")
    result["metadata_json"] = np.asarray(json.dumps(meta))
    result["seed_metrics_json"] = np.asarray(json.dumps(metrics))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--anchors",type=Path)
    parser.add_argument("--boundary-source",type=Path,help="Matched measured XY grid supplying exact seam/coast heights; strongly recommended")
    parser.add_argument("--seed",type=int,default=71421)
    parser.add_argument("--coast-width-m",type=float,default=6.)
    parser.add_argument("--seam-width-m",type=float,default=12.)
    parser.add_argument("--fit-length-m",type=float,default=60.)
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else None)
    if args.output.exists() or "saved" not in [p.lower() for p in args.output.resolve().parts]:
        parser.error("Choose a NEW output file under ignored Saved; earlier proposals remain immutable")
    if not np.isfinite(args.fit_length_m) or not 10 <= args.fit_length_m <= 150:
        parser.error("Constraint fit length must be finite and within 10..150m")
    with np.load(args.input,allow_pickle=False) as archive:
        data = {key:archive[key].copy() for key in archive.files}
    boundary = None
    if args.boundary_source:
        with np.load(args.boundary_source,allow_pickle=False) as archive:
            boundary = {key:archive[key].copy() for key in archive.files}
    anchor_json = json.loads(args.anchors.read_text(encoding="utf-8-sig")) if args.anchors else None
    result = generate(data,args.seed,anchor_json,args.coast_width_m,args.seam_width_m,args.fit_length_m,boundary)
    meta = json.loads(str(result["metadata_json"]))
    import bpy
    meta["provenance"].update(input_npz=str(args.input.resolve()),input_sha256=sha256(args.input),
                              anchors_path=str(args.anchors.resolve()) if args.anchors else None,
                              anchors_sha256=sha256(args.anchors) if args.anchors else None,
                              boundary_source_path=str(args.boundary_source.resolve()) if args.boundary_source else None,
                              boundary_source_sha256=sha256(args.boundary_source) if args.boundary_source else None,
                              generator_source_sha256=sha256(__file__),blender_version=bpy.app.version_string)
    result["metadata_json"] = np.asarray(json.dumps(meta))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    np.savez_compressed(args.output,**result)
    args.output.with_suffix(".json").write_text(json.dumps(meta,indent=2)+"\n",encoding="utf-8")
    summary = dict(meta["metrics"])
    summary["anchors"] = [a for a in summary["anchors"] if a["coverage"] != "outside"]
    print(json.dumps(dict(output=str(args.output.resolve()),**summary),indent=2))


if __name__ == "__main__":
    main()
