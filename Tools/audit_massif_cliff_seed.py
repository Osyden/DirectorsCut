"""Independent numeric constraint/footprint audit for a proposed massif NPZ.

No Blender, Unreal, geometry edits or erosion rerun. Footprints are clipped
against the original a-d grid triangles, including absent triangles, rather
than inferred safe from a protected-mask count. Heights/frame are metres.
Historical boundary preservation does not prove compatibility with live water.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True


def load(path):
    with np.load(path, allow_pickle=False) as data:
        return {key: data[key].copy() for key in data.files}


def cross(a, b):
    return a[0]*b[1]-a[1]*b[0]


def area(poly):
    if len(poly) < 3:
        return 0.
    p = np.asarray(poly)
    return abs(float(np.sum(p[:, 0]*np.roll(p[:, 1], -1)-p[:, 1]*np.roll(p[:, 0], -1))*.5))


def convex_ccw(poly):
    p = np.asarray(poly, np.float64)
    if p.ndim != 2 or p.shape[1] != 2 or len(p) < 3 or not np.isfinite(p).all():
        raise ValueError("Footprint must contain at least three finite XY points")
    turns = np.array([cross(p[(i+1)%len(p)]-p[i], p[(i+2)%len(p)]-p[(i+1)%len(p)]) for i in range(len(p))])
    if not (np.all(turns >= -1.e-9) or np.all(turns <= 1.e-9)) or area(p) <= 1.e-8:
        raise ValueError("Footprint must be a nondegenerate convex polygon in perimeter order")
    return p if np.sum(turns) > 0 else p[::-1].copy()


def clip(subject, boundary):
    """Sutherland-Hodgman clipping; boundary is a convex CCW polygon."""
    output = [np.asarray(point, np.float64) for point in subject]
    for a, b in zip(boundary, np.roll(boundary, -1, axis=0)):
        previous, output = output, []
        if not previous:
            break
        p = previous[-1]
        dp = cross(b-a, p-a)
        for q in previous:
            dq = cross(b-a, q-a)
            if (dp >= 0) != (dq >= 0):
                output.append(p+(q-p)*(dp/(dp-dq)))
            if dq >= 0:
                output.append(q)
            p, dp = q, dq
    return output


def sample_point(data, x_m, y_m):
    """Height on source ABD/ADC triangles, or None outside physical coverage."""
    h, valid = data["height_m"], data["heightfield_valid_mask"] != 0
    rows, cols = h.shape
    delta = np.array([x_m, y_m])-data["frame_origin_world_m"][:2]
    u = float(delta@data["basis_u_world"][:2]/float(data["cell_size_m"]))
    v = float(delta@data["basis_v_world"][:2]/float(data["cell_size_m"]))
    if u < -1.e-8 or v < -1.e-8 or u > cols-1+1.e-8 or v > rows-1+1.e-8:
        return None
    if abs(u-round(u)) < 1.e-8:
        u = float(round(u))
    if abs(v-round(v)) < 1.e-8:
        v = float(round(v))
    col, row = min(int(np.floor(u)), cols-2), min(int(np.floor(v)), rows-2)
    for y in (row, row-1):
        for x in (col, col-1):
            if not (0 <= x < cols-1 and 0 <= y < rows-1):
                continue
            a, b = u-x, v-y
            for vertices, weights in ((((y, x), (y, x+1), (y+1, x+1)), (1-a, a-b, b)),
                                      (((y, x), (y+1, x+1), (y+1, x)), (1-b, a, b-a))):
                if min(weights) >= -1.e-8 and all(valid[yy, xx] for yy, xx in vertices):
                    return float(sum(h[yy, xx]*w for (yy, xx), w in zip(vertices, weights)))
    return None


def footprint_report(data, anchor):
    h = np.asarray(data["height_m"], np.float64)
    valid = np.asarray(data["heightfield_valid_mask"]) != 0
    rows, cols = h.shape
    cell = float(data["cell_size_m"])
    origin = np.asarray(data["frame_origin_world_m"], np.float64)[:2]
    u, v = (np.asarray(data[k], np.float64)[:2] for k in ("basis_u_world", "basis_v_world"))
    world = convex_ccw(anchor["footprint_xy_m"])
    delta = world-origin
    local = convex_ccw(np.stack((delta@u, delta@v), axis=-1))
    bounds = np.array([[0, 0], [(cols-1)*cell, 0], [(cols-1)*cell, (rows-1)*cell], [0, (rows-1)*cell]])
    clipped_to_tile = clip(local, bounds)
    tile_area, total_area = area(clipped_to_tile), area(local)
    report = {"id": anchor.get("id", "unnamed"), "footprint_area_m2": total_area,
              "inside_tile_area_m2": tile_area, "outside_tile_area_m2": max(0., total_area-tile_area),
              "covered_area_m2": 0., "missing_triangle_area_m2": 0., "height_min_m": None,
              "height_max_m": None, "target_height_m": float(anchor["support_height_m"])}
    if tile_area < 1.e-8:
        report["status"] = "outside_tile"
        return report
    lo, hi = np.min(local, axis=0), np.max(local, axis=0)
    x0, y0 = np.maximum(np.floor(lo/cell).astype(int), 0)
    x1, y1 = np.minimum(np.floor(hi/cell).astype(int), [cols-2, rows-2])
    extrema = []
    for row in range(y0, y1+1):
        for col in range(x0, x1+1):
            a, b, c, d = (row, col), (row, col+1), (row+1, col), (row+1, col+1)
            for tri in ((a, b, d), (a, d, c)):
                xy = np.array([[x*cell, y*cell] for y, x in tri])
                overlap = clip(xy, local)
                overlap_area = area(overlap)
                if overlap_area <= 1.e-10:
                    continue
                if not all(valid[y, x] for y, x in tri):
                    report["missing_triangle_area_m2"] += overlap_area
                    continue
                report["covered_area_m2"] += overlap_area
                # Linear triangle heights attain extrema at clipped vertices.
                aa, bb, cc = xy
                ab, ac = bb-aa, cc-aa
                determinant = cross(ab, ac)
                z = [h[y, x] for y, x in tri]
                for p in overlap:
                    ap = p-aa
                    wb, wc = cross(ap, ac)/determinant, cross(ab, ap)/determinant
                    extrema.append(z[0]*(1-wb-wc)+z[1]*wb+z[2]*wc)
    if extrema:
        report["height_min_m"], report["height_max_m"] = float(min(extrema)), float(max(extrema))
        report["target_error_max_m"] = max(abs(report["height_min_m"]-report["target_height_m"]), abs(report["height_max_m"]-report["target_height_m"]))
        bottom = anchor.get("footing_bottom_m", anchor.get("actor_bottom_m"))
        if bottom is not None:
            report["minimum_ground_above_actor_bottom_m"] = report["height_min_m"]-float(bottom)
    tolerance = max(1.e-5, total_area*1.e-8)
    if report["missing_triangle_area_m2"] > tolerance:
        report["status"] = "hole_in_footprint"
    elif report["outside_tile_area_m2"] > tolerance:
        report["status"] = "partial_tile_coverage_requires_other_provider"
    elif report.get("target_error_max_m", float("inf")) > .01:
        report["status"] = "support_height_mismatch"
    else:
        report["status"] = "covered_at_target"
    report["area_accounting_error_m2"] = report["covered_area_m2"]+report["missing_triangle_area_m2"]-tile_area
    return report


def audit(baseline, candidate, anchors=None, boundary_source=None):
    failures, warnings = [], []
    def check(condition, label):
        if not bool(condition):
            failures.append(label)
    shape = baseline["height_m"].shape
    boundary = boundary_source if boundary_source is not None else baseline
    check(candidate["height_m"].shape == shape, "grid dimensions changed")
    if candidate["height_m"].shape != shape:
        return {"failures": failures, "passed": False}
    for key in ("cell_size_m", "world_xy_m", "frame_origin_world_m", "basis_u_world", "basis_v_world", "heightfield_valid_mask", "outflow_mask"):
        check(np.array_equal(baseline[key], candidate[key]), "changed frame/domain/outlets: "+key)
    for key in ("cell_size_m", "world_xy_m", "frame_origin_world_m", "basis_u_world", "basis_v_world", "heightfield_valid_mask"):
        check(np.array_equal(boundary[key], candidate[key]), "boundary source frame/domain differs: "+key)
    for key in ("height_m", "bedrock_m", "erodibility", "protected_mask", "loose_m"):
        check(candidate[key].shape == shape and np.isfinite(candidate[key]).all(), "invalid/nonfinite "+key)
    h = candidate["height_m"]
    fixed = candidate["protected_mask"] == 1
    valid = candidate["heightfield_valid_mask"] != 0
    outlets = candidate["outflow_mask"] != 0
    check(np.all(h >= candidate["bedrock_m"]), "height below bedrock")
    check(np.all(fixed[~valid]), "invalid padding not protected")
    check(np.all(fixed[outlets] & valid[outlets]), "outlet is not fixed valid terrain")
    check(np.array_equal(h[outlets], boundary["height_m"][outlets]), "outlet heights moved")
    check(np.all(candidate["erodibility"][fixed] == 0), "protected terrain is erodible")
    check(np.all(candidate["loose_m"][fixed] == 0), "protected terrain has loose sediment")
    cell = float(candidate["cell_size_m"])
    rows, cols = np.indices(shape)
    origin = candidate["frame_origin_world_m"]
    reconstructed = origin[:2]+cols[..., None]*cell*candidate["basis_u_world"][:2]+rows[..., None]*cell*candidate["basis_v_world"][:2]
    frame_error = float(np.max(np.abs(reconstructed-candidate["world_xy_m"])))
    check(frame_error < 1.e-7, "grid frame does not reconstruct actual world XY in metres")
    types = np.asarray(candidate.get("constraint_type", np.zeros(shape)), dtype=np.uint8)
    if "constraint_type" not in candidate:
        failures.append("missing independently auditable constraint_type")
    check(np.array_equal(types != 0, fixed), "constraint flags and fixed mask disagree")
    if "constraint_target_height_m" in candidate:
        target = candidate["constraint_target_height_m"].astype(np.float32)
        check(np.array_equal(h[fixed].view(np.uint32), target[fixed].view(np.uint32)), "fixed heights differ from constraint target")
    else:
        failures.append("missing independently auditable constraint_target_height_m")
    for flag, label in ((2, "seam"), (4, "coast")):
        selected = (types & flag) != 0
        check(np.array_equal(h[selected], boundary["height_m"][selected]), label+" source heights moved")
    coast = candidate.get("coast_distance_m")
    coast_comparison = None
    if coast is not None:
        points = candidate["world_xy_m"].reshape(-1, 2)
        sources = candidate["world_xy_m"][outlets]
        nearest = np.empty(len(points))
        for first in range(0, len(points), 2048):
            square = np.sum((points[first:first+2048, None, :]-sources[None, :, :])**2, axis=-1)
            nearest[first:first+2048] = np.sqrt(np.min(square, axis=1))
        nearest = nearest.reshape(shape)
        diff = np.asarray(coast)-nearest
        coast_comparison = {"reference_metric": "exact Euclidean XY distance to explicit outlet vertices",
                            "max_abs_difference_m": float(np.max(np.abs(diff))),
                            "p95_abs_difference_m": float(np.percentile(np.abs(diff), 95))}
        check(np.all(fixed[valid & (nearest <= 6+1.e-7)]), "a physical sample within6m of receiving outlets is not fixed")
    else:
        warnings.append("coast distance field unavailable")
    support = []
    if anchors:
        if anchors.get("coordinate_frame") != "arena_local_metres":
            failures.append("anchor units/frame is not arena_local_metres")
        for anchor in anchors.get("anchors", []):
            current = footprint_report(candidate, anchor)
            prior = footprint_report(boundary, anchor)
            current["boundary_source_status"] = prior["status"]
            current["boundary_source_target_error_max_m"] = prior.get("target_error_max_m")
            if current["status"] != "outside_tile":
                evidence = []
                for sample in anchor.get("ground_samples", []):
                    if not sample.get("covered", False):
                        continue
                    previous_h = sample_point(boundary, sample["x_m"], sample["y_m"])
                    candidate_h = sample_point(candidate, sample["x_m"], sample["y_m"])
                    if previous_h is not None:
                        evidence.append({"x_m": sample["x_m"], "y_m": sample["y_m"],
                                         "current_legacy_ground_m": sample["ground_m"],
                                         "boundary_grid_ground_m": previous_h,
                                         "boundary_minus_runtime_m": previous_h-sample["ground_m"],
                                         "candidate_ground_m": candidate_h})
                current["boundary_vs_runtime_samples"] = evidence
                if evidence:
                    current["boundary_runtime_datum_error_max_m"] = max(abs(e["boundary_minus_runtime_m"]) for e in evidence)
            support.append(current)
            if current["status"] in ("hole_in_footprint", "support_height_mismatch"):
                failures.append("temple "+str(current["id"])+": "+current["status"])
            if current["status"] == "partial_tile_coverage_requires_other_provider":
                warnings.append("temple "+str(current["id"])+" extends outside tile; full support not established")
    else:
        warnings.append("actual temple footprints unavailable: support not verified")
    warnings.append("Preserving an XY sampled boundary is not proof of an exact seam with the original polar triangles/current water")
    return {"passed": not failures, "failures": failures, "warnings": warnings,
            "grid_shape": list(shape), "valid_cells": int(valid.sum()), "fixed_cells": int(fixed.sum()),
            "active_cells": int((valid & ~fixed).sum()), "outlet_cells": int(outlets.sum()),
            "world_xy_reconstruction_max_error_m": frame_error,
            "coast_distance_comparison": coast_comparison, "temples": support,
            "runtime_integration_approved": False, "visual_quality_approved": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--boundary-source", type=Path, help="Actual measured source for immutable coast/seam heights; replaces baseline heights only")
    parser.add_argument("--anchors", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if "saved" not in [p.lower() for p in args.output.resolve().parts]:
        parser.error("Write audit artifacts under ignored Saved")
    anchors = json.loads(args.anchors.read_text(encoding="utf-8")) if args.anchors else None
    report = audit(load(args.baseline), load(args.candidate), anchors,
                   load(args.boundary_source) if args.boundary_source else None)
    report["provenance"] = {key: {"path": str(path.resolve()), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                            for key, path in (("baseline", args.baseline), ("candidate", args.candidate), ("anchors", args.anchors), ("boundary_source", args.boundary_source)) if path}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False)+"\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("passed", "failures", "warnings", "grid_shape", "valid_cells", "fixed_cells", "active_cells", "outlet_cells", "world_xy_reconstruction_max_error_m", "coast_distance_comparison")}, indent=2))
    print("Footprint results:", [(a["id"], a["status"], a.get("target_error_max_m")) for a in report["temples"] if a["status"] != "outside_tile"])
    print("Full report:", args.output.resolve())
    raise SystemExit(0 if report["passed"] else 1)


if __name__ == "__main__":
    main()
