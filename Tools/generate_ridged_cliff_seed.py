"""Blender's built-in ridged/heterogeneous terrain as an offline cliff proposal.

Run using installed Blender; CPU field evaluation only, no render or GPU:
  blender --background --factory-startup --threads 8 --python THIS_FILE -- \
    --input Saved/.../source08_clean_branched_seed.npz \
    --output Saved/CliffLandform/source08_ridged_seed.npz

Only measured shore/plateau/frame constraints and explicitly protected heights
are reused. The input's interior profile is discarded. Noise is evaluated in
physical world XY, with 34-96 m default ridge/terrain scales; --octaves 4 adds
24/12m ridge and17/8.5m terrain bands without increasing the relief envelope. No column-wise
gullies, forced monotonic profiles or micro-noise are added. This is a shape
preview requiring clay review, erosion, rock fracture and temple footing checks;
it is not a runtime asset or a calibrated geological simulation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np


def smoothstep(x):
    q = np.clip(x, 0, 1)
    return q*q*(3-2*q)


def masked_lowpass(field, mask, sigma_cells):
    """Separable Gaussian mean; padding/protected samples do not bias detail."""
    radius = int(np.ceil(3*sigma_cells))
    offsets = np.arange(-radius, radius+1, dtype=np.float64)
    weights = np.exp(-.5*(offsets/sigma_cells)**2)
    weights /= weights.sum()
    def blur(values):
        for axis in (0, 1):
            pad = [(0,0), (0,0)]
            pad[axis] = (radius, radius)
            values = np.apply_along_axis(lambda line: np.convolve(line, weights, mode="valid"), axis,
                                         np.pad(values, pad, mode="edge"))
        return values
    divisor = blur(mask.astype(np.float64))
    return np.divide(blur(field*mask), divisor, out=np.zeros_like(field), where=divisor>1.e-10)


def generate(data, seed=71421, octaves=2):
    from mathutils import Vector, noise
    metadata = json.loads(str(data["metadata_json"]))
    if octaves not in (2, 3, 4):
        raise ValueError("Use 2..4 octaves; finer noise is unresolved on the2m grid")
    frame = metadata["frame"]
    xy = data["world_xy_m"].astype(np.float64)
    original = data["height_m"]
    shape = original.shape
    cell = float(data["cell_size_m"])
    fixed = data["protected_mask"] == 1
    valid = data["heightfield_valid_mask"] > 0
    if xy.shape != (*shape, 2) or not np.isfinite(xy).all() or cell <= 0:
        raise ValueError("Invalid physical XY grid")
    if np.any(~valid & ~fixed):
        raise ValueError("Nonphysical padding must be protected")
    center = float(frame["source_angle_rad"])
    theta = center+np.angle(np.exp(1j*(np.arctan2(xy[..., 1], xy[..., 0])-center)))
    angles = np.asarray(data.get("constraint_angles_rad", data.get("clean_profile_angles_rad")))
    if angles.ndim != 1 or len(angles) < 2 or np.any(np.diff(angles) <= 0):
        raise ValueError("Expected increasing measured constraint angles")
    low_r, high_r, low_z, high_z = [np.interp(theta, angles, data[key]) for key in (
        "constraint_lower_radius_m", "constraint_upper_radius_m", "constraint_lower_height_m", "constraint_upper_height_m")]
    radius = np.linalg.norm(xy, axis=-1)
    span, rise = high_r-low_r, high_z-low_z
    if np.any(span[valid] <= 0) or np.any(rise[valid] <= 0):
        raise ValueError("Invalid shore-to-plateau constraints")
    q = np.clip((radius-low_r)/span, 0, 1)

    # A planar escarpment is only the boundary interpolation. All masses,
    # saddles and recesses below are genuine 2D fields, not per-angle profiles.
    base = low_z+rise*q
    rng = np.random.default_rng(seed)
    offsets = rng.uniform(-200, 200, (4, 3))
    ridge = np.zeros(shape, np.float64)
    heterogeneous = np.zeros_like(ridge)
    base_ridge = np.zeros_like(ridge)
    base_heterogeneous = np.zeros_like(ridge)
    coarse_noise = np.zeros_like(ridge)
    # Anchor coordinates to the supplied metric frame, preserving physical
    # scale and allowing exact regeneration on a finer XY grid later.
    dxy = xy-data["frame_origin_world_m"][:2]
    tangent = dxy@data["basis_u_world"][:2]
    outward = dxy@data["basis_v_world"][:2]
    for y, x in zip(*np.where(valid & ~fixed)):
        u, v = float(tangent[y, x]), float(outward[y, x])
        # Smooth 180m domain warp bends the ridge network, rather than laying
        # independent grooves or random per-cell displacements over a wall.
        warp_u = 18*noise.noise(Vector((u/180+offsets[0, 0], v/180+offsets[0, 1], offsets[0, 2])), noise_basis="PERLIN_ORIGINAL")
        warp_v = 18*noise.noise(Vector((u/180+offsets[1, 0], v/180+offsets[1, 1], offsets[1, 2])), noise_basis="PERLIN_ORIGINAL")
        u, v = u+warp_u, v+warp_v
        # Default2 octaves:96/48m and68/34m. Optional4 adds24/12m and17/8.5m;
        # even the shortest wavelength spans4.25 cells on the2m grid. The
        # physical relief cap below remains identical to the baseline.
        ridge[y, x] = noise.ridged_multi_fractal(Vector((u/96+offsets[2, 0], v/96+offsets[2, 1], offsets[2, 2])),
                                                  1.1, 2.0, float(octaves), 1.0, 2.0, noise_basis="PERLIN_ORIGINAL")
        heterogeneous[y, x] = noise.hetero_terrain(Vector((u/68+offsets[3, 0], v/68+offsets[3, 1], offsets[3, 2])),
                                                  1.0, 2.0, float(octaves), .65, noise_basis="PERLIN_ORIGINAL")
        if octaves > 2:
            base_ridge[y, x] = noise.ridged_multi_fractal(Vector((u/96+offsets[2, 0], v/96+offsets[2, 1], offsets[2, 2])),
                                                        1.1, 2.0, 2., 1., 2., noise_basis="PERLIN_ORIGINAL")
            base_heterogeneous[y, x] = noise.hetero_terrain(Vector((u/68+offsets[3, 0], v/68+offsets[3, 1], offsets[3, 2])),
                                                          1., 2., 2., .65, noise_basis="PERLIN_ORIGINAL")
        coarse_noise[y, x] = noise.noise(Vector((u/160+offsets[2, 0], v/160+offsets[2, 1], offsets[3, 2])), noise_basis="PERLIN_ORIGINAL")
    # Ridged multifractal supplies connected sharp crests; heterogeneous terrain
    # breaks their symmetry. Fixed analytic centering avoids per-tile histogram
    # normalization (which would change the shape on a larger export).
    signal = 1.7*(ridge-.85)+.75*(heterogeneous-.85)+.45*coarse_noise
    if octaves > 2:
        # Both fractal functions have positive added-octave means. Feeding raw4
        # directly into the height field raises the whole wall by5m median.
        # Retain the baseline macro signal and add only the extra-octave detail,
        # with its broad12m-sigma mean removed (no new procedural noise).
        extra = 1.7*(ridge-base_ridge)+.75*(heterogeneous-base_heterogeneous)
        extra -= masked_lowpass(extra, valid & ~fixed, 12./cell)
        signal = 1.7*(base_ridge-.85)+.75*(base_heterogeneous-.85)+.45*coarse_noise+extra
    mass = np.tanh(signal)
    amplitude = np.minimum(48., .24*rise)
    radial_window = 4*q*(1-q)
    # Tangential tile edges must meet the exact retained constraints. Smoothly
    # blend only near those boundaries, leaving the broad interior free to form
    # oblique ridges, local saddles and nonmonotone radial sections.
    rows, cols = np.indices(shape)
    edge = np.minimum.reduce((rows, cols, shape[0]-1-rows, shape[1]-1-cols))*cell
    tile_blend = smoothstep((edge-12)/24)
    boundary_blend = smoothstep((radius-low_r)/12)*smoothstep((high_r-radius)/18)
    influence = tile_blend*boundary_blend
    proposal = base+amplitude*radial_window*mass
    height = original.astype(np.float64)*(1-influence)+proposal*influence
    # Copy every protected sample bit-for-bit. No original interior detail is
    # used away from the explicit 12-36m tile /12-18m support blend margins.
    h32 = height.astype(np.float32)
    h32[fixed] = original[fixed]

    # No baked shelf rings. Hardness variation is a broad material property for
    # later erosion, independent of forced elevation contours.
    erodibility = np.clip(.56+.12*coarse_noise, .32, .72)
    erodibility *= influence
    erodibility[fixed] = 0
    depth = 5.+5*erodibility
    bedrock = (h32.astype(np.float64)-depth).astype(np.float32)
    bedrock[fixed] = h32[fixed]
    loose = (.08*erodibility).astype(np.float32)
    active = valid & ~fixed
    slope_y, slope_x = np.gradient(h32.astype(np.float64), cell)
    # The slope measured along outward radial direction may turn locally down;
    # this is allowed and recorded, unlike the previous monotone wall model.
    radial_x = np.sum(xy*data["basis_u_world"][:2], axis=-1)/radius
    radial_y = np.sum(xy*data["basis_v_world"][:2], axis=-1)/radius
    radial_slope = slope_x*radial_x+slope_y*radial_y
    metrics = dict(grid_shape=list(shape), active_cells=int(active.sum()), fixed_cells=int(fixed.sum()),
                   valid_cells=int(valid.sum()), height_min_m=float(h32[valid].min()), height_max_m=float(h32[valid].max()),
                   octaves=octaves, nominal_ridge_scales_m=[96/2**i for i in range(octaves)],
                   nominal_heterogeneous_scales_m=[68/2**i for i in range(octaves)], domain_warp_scale_m=180,
                   extra_octave_mean_removal_sigma_m=12 if octaves>2 else 0,
                   domain_warp_max_m=18, maximum_relief_envelope_m=float(amplitude[active].max()),
                   actual_relief_range_m=[float((h32-base)[active].min()), float((h32-base)[active].max())],
                   ridge_raw_range=[float(ridge[active].min()), float(ridge[active].max())],
                   hetero_raw_range=[float(heterogeneous[active].min()), float(heterogeneous[active].max())],
                   slope_degrees_p50_p90_p99=list(map(float, np.percentile(np.degrees(np.arctan(np.hypot(slope_x,slope_y)))[active], [50,90,99]))),
                   local_outward_downslope_cells=int(((radial_slope<0)&active).sum()),
                   fixed_heights_bitwise_preserved=bool(np.array_equal(h32[fixed].view(np.uint32),original[fixed].view(np.uint32))),
                   exceeds_measured_rim_cells=int(((h32>high_z+1e-4)&active).sum()),
                   not_runtime_geometry=True, temple_footing_compatibility_verified=False)
    if not np.isfinite(h32).all() or np.any(bedrock > h32) or not metrics["fixed_heights_bitwise_preserved"]:
        raise ValueError("Invalid proposal or changed protected support")
    result = {key: data[key].copy() for key in ("cell_size_m", "frame_origin_world_m", "basis_u_world", "basis_v_world", "basis_up_world",
              "world_xy_m", "u_m", "v_m", "shore_clearance_m", "heightfield_valid_mask", "protected_mask", "outflow_mask",
              "constraint_lower_radius_m", "constraint_upper_radius_m", "constraint_lower_height_m", "constraint_upper_height_m")}
    result.update(height_m=h32, bedrock_m=bedrock, erodibility=erodibility.astype(np.float32), loose_m=loose,
                  constraint_angles_rad=angles.copy(), terrain_ridge_field=ridge.astype(np.float32),
                  terrain_heterogeneous_field=heterogeneous.astype(np.float32), source_boundary_blend=influence.astype(np.float32))
    new_meta = dict(schema="directorscut.blender_ridged_landform.v1", source_kind="blender_ridged_macro_landform",
                    frame=frame, coordinate_frame=metadata["coordinate_frame"], array_layout=metadata["array_layout"],
                    provenance={"geometry_reference": metadata["provenance"].get("geometry_reference"),
                                "use": "Measured shore/plateau bounds and fixed supports only; old interior discarded outside explicit boundary blend"},
                    generator="Blender mathutils.noise ridged_multi_fractal + hetero_terrain; PERLIN_ORIGINAL",
                    seed=seed, settings={"seed":seed,"octaves":octaves}, physical_units="metres; no vertical exaggeration", metrics=metrics,
                    limitations="Offline 2D heightfield alternative, not runtime. Protected input samples unchanged; authoritative individual temple footprints unavailable, so footing approval remains required. No 3D fracture/overhangs; no calibrated erosion yet.")
    result["metadata_json"] = np.asarray(json.dumps(new_meta))
    result["seed_metrics_json"] = np.asarray(json.dumps(metrics))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=71421)
    parser.add_argument("--octaves", type=int, choices=(2,3,4), default=2)
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else None)
    if args.input.resolve() == args.output.resolve() or "saved" not in [s.lower() for s in args.output.resolve().parts]:
        parser.error("Choose a new output file under ignored Saved")
    with np.load(args.input, allow_pickle=False) as archive:
        data = {key: archive[key].copy() for key in archive.files}
    result = generate(data, args.seed, args.octaves)
    metadata = json.loads(str(result["metadata_json"]))
    import bpy
    metadata["provenance"].update(input_npz=str(args.input.resolve()), input_sha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),
                                   blender_version=bpy.app.version_string,
                                   generator_source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    result["metadata_json"] = np.asarray(json.dumps(metadata))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(args.output, **result)
    args.output.with_suffix(".json").write_text(json.dumps(metadata, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(dict(output=str(args.output.resolve()), **metadata["metrics"]), indent=2))


if __name__ == "__main__":
    main()
