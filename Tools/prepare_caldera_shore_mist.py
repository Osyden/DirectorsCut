"""Sample actual source-triangle waterline contacts for bounded caldera haze."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--arena', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--count', type=int, default=160)
    parser.add_argument('--radius-m', type=float, default=46.)
    parser.add_argument('--height-offset-m', type=float, default=2.)
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a fresh output under Saved')
    if args.count < 1 or not 0 < args.radius_m <= 50:
        parser.error('Positive count and radius <=50m required')
    arena = json.loads(args.arena.read_text(encoding='utf-8'))
    source_hash = sha(args.source)
    if source_hash != arena['source_candidate_sha256']:
        raise ValueError('Source must match the currently installed arena')
    level = float(arena['lake_height_m'])
    with np.load(args.source, allow_pickle=False) as data:
        vertices = data['vertices_m']
        triangles = data['triangles']
    points = []; barycentrics = []; face_ids = []
    total_length = 0.; segments = 0; maximum_step = 0.
    # Cut every actual original triangle, including secondary shoreline
    # branches; a first-height crossing per angular column misses those.
    for start in range(0, len(triangles), 100000):
        xyz = vertices[triangles[start:start+100000]]
        crossing = np.flatnonzero((xyz[:, :, 2].min(1) < level) & (xyz[:, :, 2].max(1) > level))
        xyz = xyz[crossing]
        endpoints = np.zeros((len(xyz), 2, 3))
        endpoint_bary = np.zeros_like(endpoints)
        counts = np.zeros(len(xyz), dtype=int)
        for i, j in ((0, 1), (1, 2), (2, 0)):
            ids = np.flatnonzero((xyz[:, i, 2] < level) != (xyz[:, j, 2] < level))
            t = (level-xyz[ids, i, 2])/(xyz[ids, j, 2]-xyz[ids, i, 2])
            endpoints[ids, counts[ids]] = xyz[ids, i]+(xyz[ids, j]-xyz[ids, i])*t[:, None]
            endpoint_bary[ids, counts[ids], i] = 1-t
            endpoint_bary[ids, counts[ids], j] = t
            counts[ids] += 1
        if np.any(counts != 2):
            raise ValueError('Unexpected non-segment waterline intersection')
        for index, (p, q) in enumerate(endpoints):
            length = float(np.linalg.norm(q-p))
            if length < 1e-9:
                continue
            n = max(1, int(np.ceil(length)))
            t = np.linspace(0, 1, n+1)[:, None]
            points.append(p+(q-p)*t)
            b0, b1 = endpoint_bary[index]
            barycentrics.append(b0+(b1-b0)*t)
            face_ids.extend([int(start+crossing[index])]*(n+1))
            total_length += length; segments += 1
            maximum_step = max(maximum_step, length/n)
    points = np.concatenate(points)
    barycentrics = np.concatenate(barycentrics)
    face_ids = np.asarray(face_ids, dtype=np.int64)
    order = np.lexsort((face_ids, points[:, 1], points[:, 0]))
    points = points[order]; barycentrics = barycentrics[order]; face_ids = face_ids[order]
    # Farthest-point coverage is deterministic; centres stay on measured
    # triangles rather than being pulled into water by centroid relaxation.
    nearest2 = np.full(len(points), np.inf)
    selected = []; index = 0
    for _ in range(args.count):
        selected.append(index)
        nearest2 = np.minimum(nearest2, np.sum((points-points[index])**2, axis=1))
        index = int(np.argmax(nearest2))
    selected = np.asarray(selected)
    contacts = points[selected]
    bary = barycentrics[selected]
    faces = face_ids[selected]
    # Legacy consumers resample this array by index. Farthest-point selection
    # order would connect distant banks across the lake; retain circular order.
    angles = np.mod(np.arctan2(contacts[:, 1], contacts[:, 0]), 2*np.pi)
    circular_order = np.lexsort((faces, np.linalg.norm(contacts[:, :2], axis=1), angles))
    contacts = contacts[circular_order]
    bary = bary[circular_order]
    faces = faces[circular_order]
    reconstruction = np.sum(vertices[triangles[faces]]*bary[:, :, None], axis=1)
    reconstruction_error = float(np.linalg.norm(reconstruction-contacts, axis=1).max())
    if reconstruction_error > 1e-7:
        raise ValueError('Contact does not reconstruct on original source triangle')
    distances = np.sqrt(nearest2+args.height_offset_m**2)
    # Distance-to-centre is1-Lipschitz. Every continuous contour point is
    # within half a sample interval of a tested point on its original segment.
    conservative_bound = float(distances.max()+maximum_step*.5)
    if conservative_bound > args.radius_m:
        raise ValueError(f'Coverage requires {conservative_bound:.3f}m, above radius budget')
    centres = contacts.copy(); centres[:, 2] += args.height_offset_m
    radius = np.linalg.norm(centres[:, :2], axis=1)
    result = dict(schema='directorscut.caldera_shore_mist.v1',
        source_candidate_sha256=source_hash, cliff_fbx_sha256=arena['cliff_fbx_sha256'],
        source_path=str(args.source.resolve()), arena_path=str(args.arena.resolve()),
        generator_sha256=sha(__file__), coordinate_frame='arena_relative_metres',
        ordering='ascending atan2(y,x) modulo2pi; radius then source triangle break ties',
        lake_height_m=level, count=args.count, radius_m=args.radius_m,
        height_offset_m=args.height_offset_m,
        shore_mist_centers_m=centres.tolist(), waterline_contacts_m=contacts.tolist(),
        source_triangle_indices=faces.tolist(), source_barycentric=bary.tolist(),
        metrics=dict(original_waterline_segments=segments, waterline_length_m=total_length,
            contour_samples=len(points), maximum_sample_step_m=maximum_step,
            maximum_nearest_centre_distance_m=float(distances.max()),
            conservative_continuous_coverage_bound_m=conservative_bound,
            source_reconstruction_error_m=reconstruction_error,
            waterline_height_error_m=float(np.max(np.abs(contacts[:, 2]-level))),
            nominal_orbit928_intersecting_spheres=int(np.count_nonzero(np.abs(radius-928)<args.radius_m)),
            outer_steer1038_intersecting_spheres=int(np.count_nonzero(np.abs(radius-1038)<args.radius_m))),
        limitation='Fog support only; near-inlet mist overlaps some steering water. No terrain, route or installed manifest is changed.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(output=str(args.output),sha256=sha(args.output),metrics=result['metrics']),indent=2))


if __name__ == '__main__':
    main()
