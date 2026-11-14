"""Build an original clean macro cliff seed from measured shore/plateau bounds.

The rejected production wall's interior heightfield is deliberately never read.
Only the first supported radial shore branch (at most 35 m dry apron) and the
V>=.84 plateau are constraints. The replacement is a single-valued XY surface,
with connected promontories/catchments formed by changing the escarpment setback.
This is an offline shape proposal, NOT a runtime mesh or accepted visual asset.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True


@dataclass(frozen=True)
class Settings:
    seed: int = 71421
    protected_apron_m: float = 35.0
    fixed_edge_m: float = 12.0
    plateau_v: float = .84
    preview_catchments: bool = False


def read_npz(path):
    with np.load(path, allow_pickle=False) as archive:
        return {k: archive[k].copy() for k in archive.files}


def _first_branch(radius, height, stop):
    """Stop at first crossing; never select a folded face's upper envelope."""
    crossing = np.flatnonzero(radius >= stop)
    if len(crossing) == 0:
        raise ValueError("Shore apron does not reach its requested outer bound")
    end = int(crossing[0])
    if end == 0 or np.any(np.diff(radius[:end+1]) <= 0):
        raise ValueError("Measured lower shore branch folds; constraints need repair")
    fraction = (stop-radius[end-1])/(radius[end]-radius[end-1])
    return (np.r_[radius[:end], stop],
            np.r_[height[:end], height[end-1]+fraction*(height[end]-height[end-1])])


def _cut_drainage(arc_grid, radius, upper_grid, low_grid, arc, spans, seed):
    """Finite connected trunk/tributary reaches in physical arc/radial metres.

    This is explicit macro drainage, not rainfall masquerading as rock detail.
    D is distance behind the measured rim (negative on the face). Width/depth
    taper independently along reaches; tributaries join the trunk's actual nodes.
    """
    rng = np.random.default_rng(seed+101)
    d = radius-upper_grid
    cut = np.zeros_like(radius)
    networks = []
    for index, centre in enumerate((-162., 3., 174.)):
        centre += float(rng.uniform(-12, 12))
        span = float(np.interp(centre, arc, spans))
        drift = float(rng.uniform(-30, 32))
        depth = (33., 47., 28.)[index]
        upper = [centre, -min(19., span*.18), 21., depth*.85]
        junction = [centre+drift, 5., 11., 9.]
        trunk = [[centre-drift*.25, -span+6., 23., 0.],
                 [centre-drift*.15, -span*.65, 25., depth*.40],
                 [centre+drift*.15, -span*.37, 22., depth],
                 upper, junction,
                 [centre+drift*1.35, 38., 5., 0.]]
        branches = [
            [upper, [centre-32., 9., 12., 10.], [centre-55., 27., 4., 0.]],
            [junction, [centre+drift+25., 17., 8., 5.], [centre+drift+47., 34., 3., 0.]],
        ]
        paths = [trunk]+branches
        for path in paths:
            for start, end in zip(path[:-1], path[1:]):
                a, b = np.asarray(start), np.asarray(end)
                delta = b[:2]-a[:2]
                t = np.clip(((arc_grid-a[0])*delta[0]+(d-a[1])*delta[1])/np.dot(delta, delta), 0, 1)
                distance = np.hypot(arc_grid-(a[0]+t*delta[0]), d-(a[1]+t*delta[1]))
                width = a[2]+t*(b[2]-a[2])
                incision = a[3]+t*(b[3]-a[3])
                cross_section = np.maximum(1-distance/width, 0)**1.15
                cut = np.maximum(cut, incision*cross_section)
        networks.append({"arc_centre_m": centre, "paths_arc_rimdistance_halfwidth_depth_m": paths})
    # No movement of the measured receiving shore, even where a wide trunk's
    # support overlaps it. A short fade keeps a continuous channel mouth.
    fade = np.clip((radius-low_grid)/8., 0, 1)
    cut *= fade*fade*(3-2*fade)
    return cut, networks


def generate(data, cfg=Settings()):
    meta = json.loads(str(data["metadata_json"]))
    frame = meta["frame"]
    xy = np.asarray(data["world_xy_m"], np.float64)
    shape = xy.shape[:2]
    cell = float(data["cell_size_m"])
    raw = np.asarray(data["raw_profile_xyz_world_m"], np.float64)
    angles = np.asarray(data["raw_profile_angles_rad"], np.float64)
    raw_v = np.asarray(data["raw_profile_v"], np.float64)
    plateau = int(np.searchsorted(raw_v, cfg.plateau_v))
    rr = np.linalg.norm(raw[:, :, :2], axis=-1)
    zz = raw[:, :, 2]
    radius = np.linalg.norm(xy, axis=-1)
    theta = np.arctan2(xy[..., 1], xy[..., 0])
    centre = float(frame["source_angle_rad"])
    theta = centre + np.angle(np.exp(1j*(theta-centre)))
    # Shore is an independently measured water-domain constraint, not old height.
    shore_at_grid = radius+data["shore_clearance_m"]
    order = np.argsort(theta.ravel())
    shore_r = np.interp(angles, theta.ravel()[order], shore_at_grid.ravel()[order])
    # The existing supported talus ends at V=.20. In narrow sectors it is only
    # ~13m wide: extending the "35m apron" there would freeze most of the old
    # folded wall. Preserve the actual lower branch, capped at 35m dry width.
    talus_end = int(np.searchsorted(raw_v, .20))
    low_r = np.minimum(shore_r+cfg.protected_apron_m, rr[:, talus_end])
    upper_r, upper_z = rr[:, plateau], zz[:, plateau]
    if np.any(upper_r-low_r < 8):
        raise ValueError("Less than 8m between the protected shore and plateau")
    shore = [_first_branch(r, z, stop) for r, z, stop in zip(rr, zz, low_r)]
    low_z = np.array([z[-1] for r, z in shore])
    if np.any(upper_z <= low_z):
        raise ValueError("Plateau must be above the lower shore")
    if np.any(np.diff(rr[:, plateau:], axis=1) <= 0):
        raise ValueError("The immutable plateau is not single-valued")

    # Arc coordinate has a physical metre scale. Features are not grid noise.
    arc = float(frame["arena_radius_m"])*(angles-centre)
    rng = np.random.default_rng(cfg.seed)
    centres, widths, recess, drifts, branches = [], [], [], [], []
    position = arc[0]-45.0
    while position < arc[-1]+45:
        position += float(rng.uniform(62, 108))
        centres.append(position)
        widths.append((float(rng.uniform(24, 46)), float(rng.uniform(18, 37))))
        recess.append(float(rng.uniform(29, 58)))
        drifts.append(float(rng.uniform(-31, 32)))
        branches.append(float(rng.uniform(18, 39)))
    q = np.linspace(0, 1, 513)
    gully = np.zeros((len(arc), len(q)))
    for x, (left, right), depth, drift, branch in zip(centres, widths, recess, drifts, branches):
        axis = x+drift*(q-.35)
        # A trunk that splits toward its head: offset is in physical tangent
        # metres and vanishes at the toe. Asymmetric tapered sides yield quiet
        # promontories/oblique planes rather than equally rounded vertical slots.
        split = np.clip((q-.42)/.58, 0, 1)
        def valley(centreline, width_factor):
            dx = arc[:, None]-centreline[None, :]
            width = np.where(dx < 0, left, right)*width_factor[None, :]
            distance = np.maximum(np.abs(dx)-1.5, 0)/width
            return np.clip(1-distance, 0, 1)
        main = valley(axis, np.ones_like(q))
        head = np.maximum(valley(axis-branch*split, 1-.38*split),
                          valley(axis+branch*.66*split, 1-.43*split))
        gully += depth*((1-split)*main+split*head)
    # Recessed gullies run back toward the plateau; quiet foreground shoulders
    # project into the crater. No uplift can exceed the measured plateau.
    span = upper_r-low_r
    setback_m = .32*span[:, None] + gully + 9*np.sin(arc[:, None]/91+1.7)
    setback = np.clip(setback_m/span[:, None], .20, .78)
    face_width_m = 14+7*(.5+.5*np.sin(arc/73+.4))
    face_width = np.clip(face_width_m/span, .10, .26)
    # Integral of a strictly positive slope field is monotone by construction.
    # Quartic peak makes broad near-planar escarpments with rounded shoulders,
    # rather than a sigmoid everywhere or multiple horizontal rock shelves.
    density = .045 + np.exp(-((q[None, :]-setback)/face_width[:, None])**4)
    density += .13*np.exp(-((q[None, :]-(setback+.20))/.24)**2)
    # Keep a gentle connection into the immutable plateau without a terminal step.
    endpoint = np.clip((1-q)/.13, 0, 1)
    endpoint = endpoint*endpoint*(3-2*endpoint)
    density = .025 + density*endpoint[None, :]
    integral = np.cumsum((density[:, 1:]+density[:, :-1])*.5, axis=1)
    normalized = np.c_[np.zeros(len(angles)), integral/integral[:, -1, None]]
    clean_r = low_r[:, None]+span[:, None]*q
    clean_z = low_z[:, None]+(upper_z-low_z)[:, None]*normalized
    clean_z[:, 0], clean_z[:, -1] = low_z, upper_z

    # Interpolate two neighbouring clean polar sections on the actual XY grid.
    # These sections contain no samples of the rejected interior wall.
    side = np.clip(np.searchsorted(angles, theta)-1, 0, len(angles)-2)
    frac = np.clip((theta-angles[side])/(angles[side+1]-angles[side]), 0, 1)
    values = []
    for offset in (0, 1):
        output = np.zeros(shape)
        for i in np.unique(side+offset):
            pick = side+offset == i
            rs = np.r_[shore[i][0][:-1], clean_r[i], rr[i, plateau+1:]]
            zs = np.r_[shore[i][1][:-1], clean_z[i], zz[i, plateau+1:]]
            output[pick] = np.interp(radius[pick], rs, zs)
        values.append(output)
    height = values[0]*(1-frac)+values[1]*frac
    lerp_angle = lambda array: array[side]*(1-frac)+array[side+1]*frac
    toe_grid = lerp_angle(rr[:, 0])
    upper_grid = lerp_angle(upper_r)
    low_grid = lerp_angle(low_r)
    valid = ((radius >= toe_grid) & (radius <= lerp_angle(rr[:, -1]))
             & (theta >= angles[0]) & (theta <= angles[-1]))
    y, x = np.indices(shape)
    edge = np.minimum.reduce([y, x, shape[0]-1-y, shape[1]-1-x])*cell
    fixed = ~valid | (radius <= low_grid) | (radius >= upper_grid) | (edge <= cfg.fixed_edge_m)
    arc_grid = float(frame["arena_radius_m"])*(theta-centre)
    drainage = []
    cut = np.zeros_like(height)
    if cfg.preview_catchments:
        cut, drainage = _cut_drainage(arc_grid, radius, upper_grid, low_grid, arc, span, cfg.seed)
        # Preserve shore/outer boundary; the new contributing catchments may
        # cut the first40m of plateau. Exact temple footprints are unavailable
        # in this seed, so this mode is explicitly NOT live-footing compatible.
        locked = ~valid | (radius <= low_grid) | (edge <= cfg.fixed_edge_m)
        cut[locked] = 0
        height -= cut
        fixed = locked | ((radius >= upper_grid) & (cut == 0))
    # Explicit physical hardness: inclined resistant layers 8–14m thick with
    # irregular 25–40m separation. They affect erosion, not initial shelf heights.
    rock_z = height+.10*arc_grid+3*np.sin(arc_grid/83)
    hard = np.exp(-((rock_z+24)/7)**2)+.7*np.exp(-((rock_z+62)/10)**2)
    erodibility = np.clip(.78-.60*hard, .14, .8)
    erodibility[fixed] = 0
    depth = 3.0+5.0*erodibility
    h32 = height.astype(np.float32)
    bed = (height-depth).astype(np.float32)
    bed[fixed] = h32[fixed]
    loose = (.12*erodibility).astype(np.float32)
    # Fixed valid cells adjacent to nonphysical ocean padding form the open toe.
    neighbour_invalid = np.zeros(shape, bool)
    neighbour_invalid[1:] |= ~valid[:-1]
    neighbour_invalid[:-1] |= ~valid[1:]
    neighbour_invalid[:, 1:] |= ~valid[:, :-1]
    neighbour_invalid[:, :-1] |= ~valid[:, 1:]
    outflow = valid & fixed & neighbour_invalid & (radius <= low_grid)
    metrics = {
        "grid_shape": list(shape), "active_cells": int(np.sum(~fixed)),
        "valid_cells": int(np.sum(valid)), "fixed_cells": int(np.sum(fixed)),
        "outflow_cells": int(np.sum(outflow)),
        "base_profile_min_dz_m": float(np.min(np.diff(clean_z, axis=1))),
        "cliff_rise_min_m": float(np.min(upper_z-low_z)),
        "cliff_rise_max_m": float(np.max(upper_z-low_z)),
        "gully_spacing_m": [62, 108], "gully_halfwidth_m": [18, 46],
        "head_branch_spread_m": [18, 39],
        "face_setback_min_m": float(np.min(setback*span[:, None])),
        "face_setback_max_m": float(np.max(setback*span[:, None])),
        "base_profile_upper_bound_exact": bool(np.array_equal(clean_z[:, -1], upper_z)),
        "protected_apron_width_min_m": float(np.min(low_r-shore_r)),
        "protected_apron_width_max_m": float(np.max(low_r-shore_r)),
        "not_runtime_geometry": True,
        "catchment_max_cut_m": float(np.max(cut)),
        "plateau_cut_cells": int(np.sum((radius >= upper_grid) & (cut > 0))),
        "temple_footing_compatibility_verified": False,
        "plateau_catchments_enabled": cfg.preview_catchments,
    }
    new_meta = {
        "schema": "directorscut.clean_cliff_landform.v1",
        "source_kind": "clean_macro_landform",
        "coordinate_frame": meta["coordinate_frame"], "array_layout": meta["array_layout"],
        "frame": frame, "settings": asdict(cfg), "metrics": metrics,
        "provenance": {"geometry_reference": meta["provenance"]["geometry_reference"],
                       "use": "Only measured first shore branch and V>=.84 plateau constraints; old interior discarded"},
        "protection": "Existing supported lower branch V<=.20, capped at35m dry shore; 12m tile edges and invalid padding. Plateau remains fixed except explicit preview catchment cuts. Fixed valid toe boundary drains water and sediment.",
        "limitations": "Single-valued XY proposal; no rock overhangs, fracture mesh, calibrated geology or runtime replacement. Catchment mode cuts the first40m of plateau: authoritative temple pad constraints are required before integration. Polar boundary interpolation is not identical to old triangle sampling.",
    }
    result = {k: data[k].copy() for k in ("cell_size_m", "frame_origin_world_m", "basis_u_world",
              "basis_v_world", "basis_up_world", "world_xy_m", "u_m", "v_m", "shore_clearance_m")}
    result.update(height_m=h32, bedrock_m=bed, erodibility=erodibility.astype(np.float32),
                  protected_mask=fixed.astype(np.float32), loose_m=loose,
                  heightfield_valid_mask=valid.astype(np.uint8), outflow_mask=outflow.astype(np.uint8),
                  base_profile_angles_rad=angles, base_profile_radius_m=clean_r,
                  base_profile_height_m=clean_z, constraint_lower_radius_m=low_r,
                  constraint_upper_radius_m=upper_r, constraint_lower_height_m=low_z,
                  constraint_upper_height_m=upper_z,
                  drainage_cut_m=cut.astype(np.float32), drainage_paths_json=np.array(json.dumps(drainage)),
                  metadata_json=np.array(json.dumps(new_meta)), seed_metrics_json=np.array(json.dumps(metrics)))
    if not np.isfinite(h32).all() or np.any(bed > h32) or not outflow.any():
        raise ValueError("Invalid generated geometry or missing shoreline outlets")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=71421)
    parser.add_argument("--preview-catchments", action="store_true", help="Cut contributing plateau drainage; requires temple support validation before integration")
    args = parser.parse_args()
    result = generate(read_npz(args.input), Settings(seed=args.seed, preview_catchments=args.preview_catchments))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(args.output, **result)
    print(str(args.output.resolve()))
    print(str(result["seed_metrics_json"]))


if __name__ == "__main__":
    main()
