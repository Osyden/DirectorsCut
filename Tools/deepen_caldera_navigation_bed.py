"""Make an isolated, underwater-only clearance candidate; never import or install it.

Run with the bundled Blender Python (NumPy only; Blender itself is unnecessary).
The envelope follows the current radial steering and inverse arena pose. It does
not move the ship, water, shoreline, mesh XY, topology, UVs or visible rock faces.
Independent triangle-ray validation is required before this candidate is used.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import time
from pathlib import Path

import numpy as np


ACCEPTED_SHA = "6eac5bba828f84ef2196f3a6598c23829c503722eb056f7ad59269bd6ab444e7"
HULL_MIN = np.array([-20.4851, -5.6688, -.3113])
HULL_MAX = np.array([26.8875, 5.6688, 10.3945])


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_install_manifest(source, candidate):
    """Retain the authoritative parent install fields, not stale NPZ labels."""
    parent_path = Path(source).parent/'manifest.json'
    parent = json.loads(parent_path.read_text(encoding='utf-8-sig'))
    with np.load(candidate, allow_pickle=False) as archive:
        meta = json.loads(str(archive['metadata_json'].item()))
        vertex_count, face_count = len(archive['vertices_m']), len(archive['triangles'])
    nav = meta['navigation_bed']; metrics = nav['metrics']
    parent['historical_parent_geometry'] = {
        'source': str(Path(source).resolve()), 'source_sha256': ACCEPTED_SHA,
        'manifest_sha256': sha(parent_path),
        'note': 'All inherited entry-cut, toe-extension, bank-integration and shoulder-fracture measurements describe their historical ancestor, not current clearance certification.',
    }
    parent.update(source=str(Path(candidate).resolve()), source_sha256=sha(candidate),
                  vertices=vertex_count, triangles=face_count,
                  navigation_bed=nav,
                  seam_position_error_m=metrics['seam_position_error_m'],
                  exported_collapsed_triangles=metrics['native_float32_cm_collapsed_triangles'],
                  exported_minimum_twice_triangle_area_m2=metrics['native_float32_cm_minimum_twice_triangle_area_m2'],
                  current_geometry_metrics=metrics,
                  independent_clearance_audit_required=True)
    path = Path(candidate).parent/'manifest.json'
    path.write_text(json.dumps(parent, indent=2)+'\n')
    return path


def smooth(lo, hi, x):
    t = np.clip((x-lo)/(hi-lo), 0., 1.)
    return t*t*(3.-2.*t)


def chart(xy):
    radius = np.linalg.norm(xy, axis=-1)
    q = smooth(.58, .985, radius/1600.)
    angle = np.pi-.4*(1.-q)
    across = ((np.arctan2(xy[..., 1], xy[..., 0])-angle+np.pi) % (2*np.pi)-np.pi)*radius
    half_width = 160.+25.*smooth(1040., 1105., radius)+35.*smooth(1113., 1130., radius)
    covered = (radius >= 832.) & (radius <= 1728.) & (abs(across) <= half_width)
    surface = -400.+400.*q-10.*(1.-smooth(.56, .58, radius/1600.))
    return radius, across, surface, covered


def route(t, offset=0.):
    r0 = 1600.*(.985+(.58-.985)*smooth(-8., 32., t))
    u = np.clip((t-24.)/8., 0., 1.)
    angle = np.pi-.4*(1.-smooth(.58, .985, r0/1600.))
    angle -= .009*(8.*(u**3-.5*u**4)+max(0., t-32.))
    r = r0+offset
    xy = r*np.array([np.cos(angle), np.sin(angle)])
    q = smooth(.58, .985, r/1600.)
    across = abs(((angle-(np.pi-.4*(1.-q))+np.pi) % (2*np.pi)-np.pi)*r)
    z = -400.+400.*q if r >= 928. and r <= 1728. and across <= 220. else -400.
    return np.array([xy[0], xy[1], z])


def pose(t, offset, velocity):
    # SteeringYaw uses the offset route plus radial velocity, not a cross-river
    # translation. Its -12..12 degree limit and physical bank are retained.
    tt = max(-7.9, t)
    centre = route(tt+.05)[:2]-route(tt-.05)[:2]
    riding = smooth(-4., 12., t)
    travel = route(tt+.05, offset+velocity*riding*.05)[:2]-route(tt-.05, offset-velocity*riding*.05)[:2]
    ca = np.arctan2(centre[1], centre[0]); ta = np.arctan2(travel[1], travel[0])
    steer = np.clip((ta-ca+np.pi) % (2*np.pi)-np.pi, -np.deg2rad(12.), np.deg2rad(12.))
    yaw = -ca-steer
    pitch = np.deg2rad(.35*np.sin(t*.65)*riding)
    roll = np.deg2rad(np.clip(velocity*.14+offset*.018, -3.5, 3.5)*riding)
    cy, sy = np.cos(yaw), np.sin(yaw)
    cp, sp = np.cos(pitch), np.sin(pitch)
    cr, sr = np.cos(roll), np.sin(roll)
    # UE FRotator: positive pitch lifts +X; positive roll lowers +Y.
    rot = np.array([[cp, sp*sr, -sp*cr], [0., cr, sr], [sp, -cp*sr, cp*cr]]) @ np.array([[cy, -sy, 0.], [sy, cy, 0.], [0., 0., 1.]])
    origin = np.array([0., 0., 2.3+.25*np.sin((270.+t)*.85)])-rot@route(t, offset)
    return rot, origin, float(np.rad2deg(steer)), float(np.rad2deg(roll))


def expand_grid(mask, shape, steps):
    a = mask.reshape(shape).copy()
    for _ in range(steps):
        b = a.copy()
        b[1:] |= a[:-1]; b[:-1] |= a[1:]
        b[:, 1:] |= a[:, :-1]; b[:, :-1] |= a[:, 1:]
        a = b
    return a.ravel()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[1]
    parser.add_argument('--source', type=Path, default=root.parent/'Crater-Workers/Cliff-Contact/Saved/CompleteCliffRing/IntegratedBanksFractured/candidate.npz')
    parser.add_argument('--output', type=Path, default=root/'Saved/CalderaNavigationBed')
    parser.add_argument('--max-lowering-m', type=float, default=24.)
    parser.add_argument('--allow-submerged-contours', action='store_true',
                        help='Release z=-400 crossings only where the entire original triangle is underwater in the cataract')
    args = parser.parse_args()
    if not 20. <= args.max_lowering_m <= 30.:
        raise ValueError('Use a bounded 20..30 metre extra-depth limit.')
    if sha(args.source) != ACCEPTED_SHA:
        raise ValueError('Input is not the immutable accepted IntegratedBanksFractured source.')
    if (args.output/'candidate.npz').exists():
        raise FileExistsError('Use a fresh output directory; candidates remain immutable.')
    started = time.perf_counter()
    source_files = {p: sha(root/p) for p in ['Source/DirectorsCut/DCMaelstromRoute.h', 'Source/DirectorsCut/DCMaelstrom.cpp', 'Tools/make_maelstrom_materials.py']}
    route_text = (root/'Source/DirectorsCut/DCMaelstromRoute.h').read_text()
    for token in ['LakeRadiusFraction=.58f', 'EntryStartSeconds=-8.f', 'LakeEntrySeconds=32.f', '-1300.f,1300.f', '-11000.f,11000.f', '-12.f, 12.f']:
        if token not in route_text:
            raise ValueError('Route source changed; inspect this port before running: '+token)
    with np.load(args.source, allow_pickle=False) as source:
        arrays = {k: source[k].copy() for k in source.files}
    v = arrays['vertices_m']; faces = arrays['triangles'].astype(np.int64, copy=False)
    metadata = json.loads(str(arrays['metadata_json'].item()))
    shape = tuple(metadata['grid_shape'])
    r, across, water, covered = chart(v[:, :2])

    # Freeze complete original dry/shoreline triangles, not just shoreline
    # vertices. This retains the exact contours supporting the installed fog.
    dry_vertex = ~covered | (water-v[:, 2] < 1.5)
    dry_face = dry_vertex[faces].any(axis=1)
    zz = v[faces, 2]
    shore_face = (zz.min(axis=1) <= -400.) & (zz.max(axis=1) >= -400.)
    centre = v[faces].mean(axis=1)
    _, _, centre_water, centre_covered = chart(centre[:, :2])
    submerged_face = ~dry_face & centre_covered & (centre_water-centre[:, 2] >= 1.5)
    preserved_shore_face = shore_face & (~submerged_face if args.allow_submerged_contours else True)
    protected = np.zeros(len(v), bool)
    protected[faces[dry_face | preserved_shore_face].ravel()] = True
    protected.reshape(shape)[[0, -1], :] = True
    protected.reshape(shape)[:, [0, -1]] = True
    # Three rings form a fixed collar beside the already fixed triangles.
    collar = expand_grid(protected, shape, 3)
    free = ~collar & covered
    local = np.flatnonzero(free)
    points = v[local]; target = np.zeros(len(local)); witnesses = np.zeros((len(local), 3))
    state_count = 0; max_across = 0.; max_yaw = 0.; max_roll = 0.
    for t in np.arange(-8., 34.001, .5):
        for offset in np.arange(-110., 110.001, 10.):
            # The complete legal control box is conservative: some extreme
            # offset/velocity combinations are not reachable at early times.
            rp = route(t, offset)
            ix = np.flatnonzero(np.linalg.norm(points[:, :2]-rp[:2], axis=1) < 95.)
            if not len(ix):
                continue
            _, ac, _, _ = chart(rp[None, :2]); max_across = max(max_across, abs(float(ac[0])))
            for velocity in [-13., 0., 13.]:
                rot, origin, yaw, roll = pose(t, offset, velocity)
                max_yaw = max(max_yaw, abs(yaw)); max_roll = max(max_roll, abs(roll))
                bed_world = points[ix]@rot.T+origin
                water_points = points[ix].copy(); water_points[:, 2] = water[local[ix]]
                surface_world = water_points@rot.T+origin
                ellipse = np.linalg.norm(surface_world[:, :2]*[.5, 2.], axis=1)
                flatten = (1.-smooth(12., 35., ellipse))*smooth(.56, .58, r[local[ix]]/1600.)
                flat_z = surface_world[:, 2]+(2.3-surface_world[:, 2])*flatten
                # Reserve 2.5 m for water triangulation, pose-grid interpolation
                # and the small XY shift caused by lowering under bank/pitch.
                water_need = np.maximum(0., bed_world[:, 2]-flat_z+2.5)
                hull_weight = (1.-smooth(24.1863, 32.1863, abs(bed_world[:, 0]-3.2012)))*(1.-smooth(6.1688, 14.1688, abs(bed_world[:, 1])))
                hull_need = np.maximum(0., bed_world[:, 2]-(HULL_MIN[2]-1.))*hull_weight
                needed = np.maximum(water_need, hull_need)/rot[2, 2]
                improve = needed > target[ix]
                target[ix[improve]] = needed[improve]
                witnesses[ix[improve]] = [t, offset, velocity]
                state_count += 1

    required = np.zeros(len(v)); required[local] = target
    lowering = required.reshape(shape).copy()
    # A smooth majorant fills shallow dips in the envelope without weakening
    # any measured requirement. It can only deepen other submerged free cells.
    allow = free.reshape(shape)
    for _ in range(10):
        average = lowering*.4
        average[1:] += lowering[:-1]*.15; average[:-1] += lowering[1:]*.15
        average[:, 1:] += lowering[:, :-1]*.15; average[:, :-1] += lowering[:, 1:]*.15
        lowering = np.where(allow, np.maximum(lowering, average), 0.)
    lowering = np.minimum(lowering.ravel(), args.max_lowering_m)
    # A z=-400 intersection underneath the higher cataract is submerged bed,
    # not exposed lakeshore. The opt-in correction may move only those
    # intersections; genuine lake-only and exposed shore triangles stay fixed.
    if not args.allow_submerged_contours:
        above = v[:, 2] > -400.
        lowering[above] = np.minimum(lowering[above], np.maximum(0., v[above, 2]+399.75))
    lowering[collar] = 0.
    new = v.copy(); new[:, 2] -= lowering
    changed = lowering > 1.e-9
    n0 = np.cross(v[faces[:, 1]]-v[faces[:, 0]], v[faces[:, 2]]-v[faces[:, 0]])
    n1 = np.cross(new[faces[:, 1]]-new[faces[:, 0]], new[faces[:, 2]]-new[faces[:, 0]])
    area = np.linalg.norm(n1, axis=1)
    native = ((new-np.array([-1072., 1154., -25.]))*100.).astype(np.float32).astype(np.float64)*.01
    nn = np.cross(native[faces[:, 1]]-native[faces[:, 0]], native[faces[:, 2]]-native[faces[:, 0]])
    nnarea = np.linalg.norm(nn, axis=1)
    reversed_faces = np.einsum('ij,ij->i', n0, n1) < 0.
    changed_faces = changed[faces].any(axis=1)
    worst = int(np.argmax(required-lowering))
    metrics = {
        'vertices': len(v), 'triangles': len(faces), 'changed_vertices': int(changed.sum()),
        'changed_triangles': int(changed_faces.sum()), 'maximum_lowering_m': float(lowering.max()),
        'lowering_positive_quantiles_m': np.quantile(lowering[changed], [0, .25, .5, .75, 1]).tolist(),
        'changed_bounds_m': [new[changed].min(axis=0).tolist(), new[changed].max(axis=0).tolist()],
        'minimum_original_water_above_changed_vertex_m': float((water-v[:, 2])[changed].min()),
        'original_shore_triangles': int(shore_face.sum()),
        'shore_triangle_position_difference_m': float(np.max(abs(new[faces[shore_face]]-v[faces[shore_face]]))),
        'preserved_visible_or_lake_only_shore_triangles': int(preserved_shore_face.sum()),
        'preserved_visible_or_lake_only_shore_difference_m': float(np.max(abs(new[faces[preserved_shore_face]]-v[faces[preserved_shore_face]]))),
        'released_submerged_lake_plane_triangles': int(np.count_nonzero(shore_face & ~preserved_shore_face)),
        'original_dry_triangle_position_difference_m': float(np.max(abs(new[faces[dry_face]]-v[faces[dry_face]]))),
        'xy_exact': bool(np.array_equal(new[:, :2], v[:, :2])),
        'topology_and_uvs_exact': True, 'new_lake_plane_vertex_crossings': int(np.count_nonzero((new[:, 2] < -400.) != (v[:, 2] < -400.))),
        'minimum_twice_triangle_area_m2': float(area.min()), 'degenerate_triangles': int(np.count_nonzero(area <= 1.e-9)),
        'source_opposed_triangle_normals': int(reversed_faces.sum()),
        'native_float32_cm_minimum_twice_triangle_area_m2': float(nnarea.min()),
        'native_float32_cm_collapsed_triangles': int(np.count_nonzero(nnarea <= 1.e-9)),
        'seam_position_error_m': float(np.max(np.linalg.norm(new.reshape(*shape, 3)[:, 0]-new.reshape(*shape, 3)[:, -1], axis=1))),
        'pose_count': state_count, 'route_maximum_across_m': max_across,
        'route_maximum_steering_yaw_degrees': max_yaw, 'route_maximum_bank_degrees': max_roll,
        'envelope_required_maximum_m': float(required.max()),
        'envelope_unfulfilled_vertices': int(np.count_nonzero(required-lowering > .01)),
        'envelope_unfulfilled_maximum_m': float((required-lowering).max()),
        'worst_unfulfilled_vertex': worst, 'worst_unfulfilled_pose_t_offset_velocity': witnesses[np.searchsorted(local, worst)].tolist() if worst in local else None,
        'elapsed_seconds': time.perf_counter()-started,
    }
    if metrics['degenerate_triangles'] or metrics['native_float32_cm_collapsed_triangles'] or metrics['source_opposed_triangle_normals']:
        raise RuntimeError('Geometry gate failed: '+json.dumps(metrics))
    metadata['navigation_bed'] = {
        'schema': 'directorscut.caldera_navigation_bed.v1', 'parent_source': str(args.source.resolve()),
        'parent_sha256': ACCEPTED_SHA, 'generator_sha256': sha(__file__), 'production_source_sha256': source_files,
        'method': 'maximum sampled current route/steering inverse-pose underwater clearance envelope; Z lowering only',
        'pose_grid': {'time_s': [-8., 34., .5], 'radial_offset_m': [-110., 110., 10.], 'radial_velocity_mps': [-13., 0., 13.]},
        'maximum_extra_depth_m': args.max_lowering_m, 'water_margin_m': 2.5, 'hull_margin_m': 1.,
        'allow_submerged_lake_plane_contours': args.allow_submerged_contours,
        'hull_bounds_world_m': [HULL_MIN.tolist(), HULL_MAX.tolist()], 'metrics': metrics,
        'limitations': 'Sampled conservative control box, not a continuous route proof. Independent original-triangle probe required. No dynamic FFT waves, transient hull jolts or native validation. Source visible banks and actual exposed/lake-only shoreline triangles are bitwise fixed. Submerged z=-400 contours may move only with the explicit option; fog/source reports must be regenerated, never relabelled.',
    }
    metadata['seam_position_error_m'] = metrics['seam_position_error_m']
    metadata['exported_minimum_twice_triangle_area_m2'] = metrics['native_float32_cm_minimum_twice_triangle_area_m2']
    metadata['exported_collapsed_triangles'] = metrics['native_float32_cm_collapsed_triangles']
    arrays['vertices_m'] = new
    arrays['navigation_lowering_m'] = lowering.astype(np.float32)
    arrays['metadata_json'] = np.array(json.dumps(metadata))
    args.output.mkdir(parents=True, exist_ok=True)
    target_path = args.output/'candidate.npz'; np.savez_compressed(target_path, **arrays)
    report = dict(metadata['navigation_bed'], source_npz=str(target_path.resolve()), source_sha256=sha(target_path),
                  inherited_metadata_preserved_except_current_geometry_metrics=True,
                  source_evidence=str(root/'Saved/CalderaHullBedProbe/report.json'))
    (args.output/'report.json').write_text(json.dumps(report, indent=2))
    write_install_manifest(args.source, target_path)
    np.savez_compressed(args.output/'envelope.npz', vertex_index=local, required_lowering_m=target, witness_pose=witnesses,
                        protected_vertex=protected, fixed_collar=collar, original_shore_triangle_indices=np.flatnonzero(shore_face),
                        preserved_visible_shore_triangle_indices=np.flatnonzero(preserved_shore_face))
    print(json.dumps({'candidate': str(target_path), 'sha256': report['source_sha256'], 'metrics': metrics}, indent=2))


if __name__ == '__main__':
    main()
