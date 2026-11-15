"""CPU-only actual-triangle clearance audit; run with Blender --background --threads 1.

Example: blender --background --factory-startup --threads 1 --python-exit-code 1
 --python Tools/probe_caldera_navigation_clearance.py -- --candidate <candidate.npz>
No scene objects, exports, renders, or production assets are created. Output is
Saved/CalderaNavigationClearance. A sampled hull AABB is not exact hull collision.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
import time

import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = Path('E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CompleteCliffRing/IntegratedBanksFractured/candidate.npz')
HMIN = np.array([-20.4851, -5.6688, -.3113])
HMAX = np.array([26.8875, 5.6688, 10.3945])
RADIUS = 1600.


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def sm(a, b, x):
    t = np.clip((x-a)/(b-a), 0., 1.)
    return t*t*(3.-2.*t)


def wrap(x):
    return np.arctan2(np.sin(x), np.cos(x))


def height(p):
    r = np.linalg.norm(p[..., :2], axis=-1)
    q = sm(.58, .985, r/RADIUS)
    a = np.pi-.4*(1.-q)
    across = np.abs(wrap(np.arctan2(p[..., 1], p[..., 0])-a))*r
    return np.where((r >= 928.) & (r <= 1728.) & (across <= 220.), -400.+400.*q, -400.)


def point(t, offset=0.):
    r = RADIUS*(.985+(.58-.985)*sm(-8., 32., t))
    u = np.clip((t-24.)/8., 0., 1.)
    a = np.pi-.4*(1.-sm(.58, .985, r/RADIUS))-.009*(8.*(u**3-.5*u**4)+max(0., t-32.))
    return (r+offset)*np.array([np.cos(a), np.sin(a)])


def pose(t, offset, velocity):
    riding = sm(-4., 12., t)
    tt = max(-7.9, t)
    centre = point(tt+.05)-point(tt-.05)
    travel = point(tt+.05, offset+velocity*riding*.05)-point(tt-.05, offset-velocity*riding*.05)
    yaw_delta = np.clip(wrap(np.arctan2(travel[1], travel[0])-np.arctan2(centre[1], centre[0])), -np.deg2rad(12.), np.deg2rad(12.))
    yaw = -np.arctan2(centre[1], centre[0])-yaw_delta
    pitch = np.deg2rad(.35*np.sin(t*.65)*riding)
    roll = np.deg2rad(np.clip(velocity*.14+offset*.018, -3.5, 3.5)*riding)
    cy, sy, cp, sp, cr, sr = np.cos(yaw), np.sin(yaw), np.cos(pitch), np.sin(pitch), np.cos(roll), np.sin(roll)
    # UE FRotator: +pitch lifts +X; +roll lowers +Y. Pose=Over*Yaw.
    rot = np.array([[cp, sp*sr, -sp*cr], [0., cr, sr], [sp, -cp*sr, cp*cr]]) @ np.array([[cy, -sy, 0.], [sy, cy, 0.], [0., 0., 1.]])
    p = point(t, offset)
    local = np.r_[p, height(p)]
    translation = np.array([0., 0., 2.3+.25*np.sin((270.+t)*.85)])-rot@local
    return rot, translation


def source_contract():
    """Fail closed when the production formulas this bounded probe mirrors change."""
    files = {'runtime': ROOT/'Source/DirectorsCut/DCMaelstrom.cpp',
             'route': ROOT/'Source/DirectorsCut/DCMaelstromRoute.h',
             'material': ROOT/'Tools/make_maelstrom_materials.py'}
    required = {
        'runtime': ['Rows=240,Sides=64', 'Rings = 160', 'Sectors = 768',
                    'FMath::Lerp(1.08f,.52f,F)', '160.f+25.f*FMath::SmoothStep(1040.f,1105.f,RM)',
                    '35.f*FMath::SmoothStep(1113.f,1130.f,RM)', 'SideAngle=A+Across/R',
                    '(1.f-FMath::SmoothStep(.56f,.58f,static_cast<float>(P.Size())/Radius))*1000.f',
                    'const float Pitch=0.f', 'RouteVelocity*.0014f+RouteOffset*.00018f',
                    '.35f*FMath::Sin(Elapsed*.65f)*Riding', 'Director->RideTime*.85f)*25.f',
                    '0.065f * FMath::Sin(Angle * 3.f + 0.7f)',
                    '0.032f * FMath::Sin(Angle * 7.f - 1.3f)',
                    '0.014f * FMath::Sin(Angle * 17.f + 2.1f)'],
        'route': ['EntryStartSeconds=-8.f', 'LakeEntrySeconds=32.f', 'LakeRadiusFraction=.58f',
                  'OrbitAngularSpeed=.009f', 'Offset-RadialVelocity*.05f', '-12.f, 12.f',
                  '-1300.f,1300.f', '-11000.f,11000.f'],
        'material': ['smoothstep(.56,.58,length(UV*100.0)/max(RadiusM,1.0))',
                     '1-smoothstep(300.0,800.0,abs(Level-P.z))',
                     'smoothstep(1200.0,3500.0,length(P.xy*float2(0.5,2.0)))',
                     'smoothstep(1200.0,4000.0,length(P.xy*float2(.5,2)))',
                     'return wave+float3(0,0,(Level-P.z)*flatten)',
                     'd.xy=d.xy/(1+abs(d.xy)/150.0)', 'd.z=d.z/(1+abs(d.z)/400.0)']}
    for key, path in files.items():
        text = path.read_text()
        missing = [s for s in required[key] if s not in text]
        if missing:
            raise RuntimeError(f'Production {key} changed; review probe before running: {missing}')
    return {str(p.relative_to(ROOT)): sha(p) for p in files.values()}


def grid_faces(rows, sides, cataract):
    k = (np.arange(rows)[:, None]*(sides+1)+np.arange(sides)[None, :]).ravel()
    j = k+sides+1
    return np.concatenate([np.stack([k, j, k+1] if cataract else [k, k+1, j], 1),
                           np.stack([k+1, j, j+1] if cataract else [k+1, j+1, j], 1)])


def water_mesh():
    r = np.linspace(1728., 832., 241)
    q = sm(.58, .985, r/RADIUS)
    width = 160.+25.*sm(1040., 1105., r)+35.*sm(1113., 1130., r)
    a = np.pi-.4*(1.-q[:, None])+width[:, None]*np.linspace(-1., 1., 65)[None, :]/r[:, None]
    z = -400.+400.*q-10.*(1.-sm(.56, .58, r/RADIUS))
    v = np.stack([r[:, None]*np.cos(a), r[:, None]*np.sin(a), np.broadcast_to(z[:, None], a.shape)], -1).reshape(-1, 3)
    f = grid_faces(240, 64, True)
    kind = np.ones(len(v), dtype=bool)
    a = np.linspace(0., 2.*np.pi, 769); a[-1] = 0.
    bays = .065*np.sin(a*3.+.7)+.032*np.sin(a*7.-1.3)+.014*np.sin(a*17.+2.1)
    rim = RADIUS*(1.+bays*sm(.35, .85, np.abs(wrap(a-np.pi))))
    r = 1.2+(rim[None, :]*1.1-1.2)*np.linspace(0., 1., 161)[:, None]
    lake = np.stack([r*np.cos(a), r*np.sin(a), np.full(r.shape, -400.)], -1).reshape(-1, 3)
    # Centre fan is outside the audited descent/entry region.
    return np.concatenate([v, lake]), np.concatenate([f, grid_faces(160, 768, False)+len(v)]), np.r_[kind, np.zeros(len(lake), dtype=bool)]


def compact(vertices, faces):
    ids, inverse = np.unique(faces, return_inverse=True)
    return vertices[ids], inverse.reshape(-1, 3), ids


def bvh(vertices, faces):
    return BVHTree.FromPolygons(vertices.tolist(), faces.tolist(), all_triangles=True)


def geometry_audit(old, new):
    ov, nv, faces = old['vertices_m'], new['vertices_m'], old['triangles']
    delta = nv-ov
    changed = np.any(delta != 0., axis=1)
    unchanged = {k: bool(np.array_equal(old[k], new[k])) for k in old.files
                 if k not in ('vertices_m', 'metadata_json') and k in new.files}
    if not unchanged.get('triangles', False):
        raise RuntimeError('Candidate connectivity changed; this probe requires matching topology')
    old_dry = ov[:, 2] >= height(ov)-.01
    cross_lake = 0; changed_dry_faces = 0; collapsed = 0; flipped = 0
    min_area = float('inf'); min_ratio = float('inf'); shoreline_faces = 0
    lake_faces = []; changed_lake_flags = []
    origin = old['frame_origin_world_m']
    for start in range(0, len(faces), 50000):
        ff = faces[start:start+50000]
        before = ((ov[ff]-origin)*100.).astype(np.float32).astype(np.float64)*.01
        after = ((nv[ff]-origin)*100.).astype(np.float32).astype(np.float64)*.01
        on = np.cross(before[:, 1]-before[:, 0], before[:, 2]-before[:, 0])
        nn = np.cross(after[:, 1]-after[:, 0], after[:, 2]-after[:, 0])
        oa, na = np.linalg.norm(on, axis=1), np.linalg.norm(nn, axis=1)
        collapsed += int(np.count_nonzero(na < 1.e-8))
        flipped += int(np.count_nonzero(np.einsum('ij,ij->i', on, nn) < 0.))
        min_area = min(min_area, float(na.min()))
        min_ratio = min(min_ratio, float((na/np.maximum(oa, 1.e-20)).min()))
        change_face = changed[ff].any(1)
        touch_lake = (ov[ff, 2].min(1) <= -400.) & (ov[ff, 2].max(1) >= -400.)
        shoreline_faces += int(touch_lake.sum())
        cross_lake += int(np.count_nonzero(touch_lake & change_face))
        lake_faces.append(ff[touch_lake]); changed_lake_flags.append(change_face[touch_lake])
        changed_dry_faces += int(np.count_nonzero(old_dry[ff].any(1) & change_face))
    # Z=-400 contours below the visible river are not exposed lake shoreline.
    # Classify with actual original cataract triangles and finite sheet width,
    # not HeightAt's wider220m analytic corridor. Include the face centroid.
    lake_faces = np.concatenate(lake_faces)
    changed_lake_flags = np.concatenate(changed_lake_flags)
    wv, wf, kind = water_mesh()
    river_faces = wf[kind[wf].all(1)]
    rv, rf, _ = compact(wv, river_faces)
    river_tree = bvh(rv, rf)
    queries = np.concatenate([ov[lake_faces], ov[lake_faces].mean(1)[:, None, :]], axis=1)
    clearances = np.full(queries.shape[:2], -np.inf)
    for i, face in enumerate(queries):
        for j, p in enumerate(face):
            hit = river_tree.ray_cast(Vector((p[0], p[1], 100.)), Vector((0., 0., -1.)), 700.)[0]
            if hit is not None:
                clearances[i, j] = hit.z-p[2]
    wet_river = (clearances >= 1.5).all(1)
    metadata = json.loads(str(new['metadata_json']))
    shape = metadata.get('grid_shape', [281, 3073])
    grid = nv.reshape(*shape, 3)
    return {'vertices': len(ov), 'triangles': len(faces), 'unchanged_arrays': unchanged,
            'changed_vertices': int(changed.sum()), 'max_abs_xy_change_m': float(np.abs(delta[:, :2]).max()),
            'z_change_range_m': [float(delta[:, 2].min()), float(delta[:, 2].max())],
            'changed_originally_dry_vertices': int(np.count_nonzero(changed & old_dry)),
            'changed_faces_touching_originally_dry_vertices': changed_dry_faces,
            'lake_shoreline_faces': shoreline_faces, 'changed_lake_shoreline_faces': cross_lake,
            'original_lake_plane_faces_fully_under_actual_cataract_1_5m': int(wet_river.sum()),
            'changed_lake_plane_faces_fully_under_actual_cataract_1_5m': int(np.count_nonzero(changed_lake_flags & wet_river)),
            'changed_true_exposed_or_lake_only_shore_faces': int(np.count_nonzero(changed_lake_flags & ~wet_river)),
            'min_original_river_clearance_on_changed_lake_plane_faces_m': (float(clearances[changed_lake_flags].min()) if changed_lake_flags.any() else None),
            'collapsed_native_float_triangles': collapsed, 'opposite_source_normals': flipped,
            'min_native_doubled_area_m2': min_area, 'min_area_ratio': min_ratio,
            'angular_seam_error_m': float(np.linalg.norm(grid[:, 0]-grid[:, -1], axis=1).max())}


def sample_xy():
    x = np.unique(np.r_[np.arange(-20., 27., 2.), HMIN[0], HMAX[0]])
    y = np.unique(np.r_[np.arange(-5., 6., 1.), HMIN[1], HMAX[1]])
    hull = np.stack(np.meshgrid(x, y), -1).reshape(-1, 2)
    outer = np.stack(np.meshgrid(np.arange(-72., 73., 4.), np.arange(-20., 21., 2.)), -1).reshape(-1, 2)
    return np.unique(np.r_[hull, outer], axis=0)


def region_stats(mask, bed, water, margin, xy, flatten):
    valid = mask & np.isfinite(bed) & np.isfinite(water)
    indices = np.flatnonzero(valid)
    if not len(indices):
        return {'samples': 0, 'bed_missing': int(np.count_nonzero(mask & ~np.isfinite(bed)))}
    gap = water-bed
    worst = indices[np.argmin(gap[indices])]
    hull_clearance = HMIN[2]-bed[indices]
    worst_hull = indices[np.argmin(hull_clearance)]
    return {'samples': len(indices), 'bed_missing': int(np.count_nonzero(mask & ~np.isfinite(bed))),
            'water_missing': int(np.count_nonzero(mask & ~np.isfinite(water))),
            'min_water_above_bed_m': float(gap[indices].min()),
            'exposed_bed_samples': int(np.count_nonzero(gap[indices] < -.02)),
            'min_water_minus_wave_bound_above_bed_m': float((gap-margin)[indices].min()),
            'min_hull_aabb_bottom_above_bed_m': float(hull_clearance.min()),
            'bed_above_hull_aabb_bottom_samples': int(np.count_nonzero(hull_clearance < -.02)),
            'bed_above_entire_hull_samples': int(np.count_nonzero(bed[indices] > HMAX[2])),
            'worst_hull_xy_m': xy[worst_hull].tolist(),
            'bed_z_at_hull_worst_m': float(bed[worst_hull]),
            'worst_xy_m': xy[worst].tolist(), 'bed_z_at_worst_m': float(bed[worst]),
            'water_z_at_worst_m': float(water[worst]), 'water_face_wave_bound_at_worst_m': float(margin[worst]),
            'water_face_flatten_at_worst': float(flatten[worst])}


def summarize(records):
    summary = {}
    for name in ('original', 'candidate'):
        summary[name] = {}
        for region in ('hull_AABB', 'flatten_affected_outside_hull'):
            all_results = [r[name][region] for r in records]
            candidates = [(r, r[name][region]) for r in records if r[name][region]['samples']]
            worst = min(candidates, key=lambda it: it[1]['min_water_above_bed_m'])
            hull_worst = min(candidates, key=lambda it: it[1]['min_hull_aabb_bottom_above_bed_m'])
            summary[name][region] = {
                'states': len(candidates),
                'states_tested': len(records),
                'terrain_missing_rays': sum(s['bed_missing'] for s in all_results),
                'states_with_any_terrain_miss': sum(s['bed_missing'] > 0 for s in all_results),
                'states_with_only_terrain_misses': sum(s['samples'] == 0 and s['bed_missing'] > 0 for s in all_results),
                'water_missing_rays': sum(s.get('water_missing', 0) for s in all_results),
                'states_with_exposed_bed': sum(s['exposed_bed_samples'] > 0 for _, s in candidates),
                'states_with_bed_above_hull_bottom': sum(s['bed_above_hull_aabb_bottom_samples'] > 0 for _, s in candidates),
                'min_water_above_bed_m': worst[1]['min_water_above_bed_m'],
                'min_water_minus_wave_bound_above_bed_m': min(s['min_water_minus_wave_bound_above_bed_m'] for _, s in candidates),
                'min_hull_aabb_bottom_above_bed_m': hull_worst[1]['min_hull_aabb_bottom_above_bed_m'],
                'worst_water_state': {k: worst[0][k] for k in ('encounter_s', 'offset_m', 'radial_velocity_m_s')},
                'worst_hull_state': {k: hull_worst[0][k] for k in ('encounter_s', 'offset_m', 'radial_velocity_m_s')}}
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=DEFAULT_SOURCE)
    parser.add_argument('--candidate', type=Path, required=True, help='Explicit candidate bundle; never assume a previously accepted trial')
    parser.add_argument('--output', type=Path, default=ROOT/'Saved/CalderaNavigationClearance')
    parser.add_argument('--time-step', type=float, default=1., help='Descent time sampling in seconds; default 1')
    parser.add_argument('--offset-step', type=float, default=10., help='Steering offset sampling in metres; default 10')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    started = time.monotonic()
    source_hashes = source_contract()
    source_sha, candidate_sha = sha(args.source), sha(args.candidate)
    out = args.output; out.mkdir(parents=True, exist_ok=True)
    old, new = np.load(args.source, allow_pickle=False), np.load(args.candidate, allow_pickle=False)
    geom = geometry_audit(old, new)
    # Includes both ends and intermediate offsets, extreme velocities and the
    # centreline regressions. Cartesian combinations overbound reachable states.
    if args.time_step <= 0. or args.offset_step <= 0.:
        raise ValueError('Sampling steps must be positive')
    times = sorted(set(np.r_[np.arange(-8., 34.001, args.time_step), -7., 5., 10., 14., 15., 27.5, 32., 34., 40., 50., 60.].tolist()))
    offsets = np.unique(np.r_[np.arange(-110., 110.001, args.offset_step), -110., 0., 110.])
    states = [(t, o, v) for t in times for o in offsets for v in (-13., 0., 13.)]
    centres = np.array([point(t, o) for t, o, _ in states])
    lo, hi = centres.min(0)-120., centres.max(0)+120.
    faces = old['triangles']; ov = old['vertices_m']; nv = new['vertices_m']
    selection = []
    for start in range(0, len(faces), 50000):
        ff = faces[start:start+50000]; bb = ov[ff, :2]
        mask = (bb.min(1) <= hi).all(1) & (bb.max(1) >= lo).all(1)
        selection.append(ff[mask])
    bed_faces = np.concatenate(selection)
    bv, bf, ids = compact(ov, bed_faces)
    beds = {'original': bvh(bv, bf), 'candidate': bvh(nv[ids], bf)}
    wv, wf, kinds = water_mesh()
    bb = wv[wf, :2]
    selected = (bb.min(1) <= hi).all(1) & (bb.max(1) >= lo).all(1)
    wv, wf, ids = compact(wv, wf[selected]); kinds = kinds[ids]
    xy = sample_xy()
    inside = ((xy >= HMIN[:2]) & (xy <= HMAX[:2])).all(1)
    origins_world = np.c_[xy, np.full(len(xy), 1000.)]
    records = []
    snapshots = {}
    for index, (t, offset, velocity) in enumerate(states):
        rot, translation = pose(t, offset, velocity)
        world = wv@rot.T+translation
        ellipse = np.linalg.norm(world[:, :2]*[.5, 2.], axis=1)
        gate = np.where(kinds, sm(.56, .58, np.linalg.norm(wv[:, :2], axis=1)/RADIUS),
                        1.-sm(3., 8., np.abs(2.3-world[:, 2])))
        flatten = gate*(1.-sm(12., 35., ellipse))
        world[:, 2] += (2.3-world[:, 2])*flatten
        bb = world[wf, :2]
        local = (bb.min(1) <= xy.max(0)+1.).all(1) & (bb.max(1) >= xy.min(0)-1.).all(1)
        water_faces = wf[local]
        vv, ff, _ = compact(world, water_faces)
        water_bvh = bvh(vv, ff)
        wz = np.full(len(xy), np.nan); bound = np.zeros(len(xy)); fg = np.zeros(len(xy))
        # |Live| <= sqrt(1.5^2+1.5^2+4^2)m after soft clipping; curved
        # normal transport is a rotation. Bounds include interpolated face vertices.
        vertex_bound = math.sqrt(20.5)*sm(12., 40., ellipse)
        face_bound = vertex_bound[water_faces].max(1)
        face_flatten = flatten[water_faces].max(1)
        for j, origin in enumerate(origins_world):
            hit, _, face, _ = water_bvh.ray_cast(Vector(origin), Vector((0., 0., -1.)), 2500.)
            if hit is not None:
                wz[j] = hit.z; bound[j] = face_bound[face]; fg[j] = face_flatten[face]
        origin_local = (origins_world-translation)@rot
        direction_local = Vector(rot.T@np.array([0., 0., -1.]))
        record = {'encounter_s': t, 'ride_s': 270.+t, 'offset_m': float(offset), 'radial_velocity_m_s': velocity}
        for name, tree in beds.items():
            bz = np.full(len(xy), np.nan)
            for j, origin in enumerate(origin_local):
                hit = tree.ray_cast(Vector(origin), direction_local, 2500.)[0]
                if hit is not None:
                    bz[j] = (rot@np.array(hit)+translation)[2]
            record[name] = {
                'hull_AABB': region_stats(inside, bz, wz, bound, xy, fg),
                'flatten_affected_outside_hull': region_stats(~inside & (fg > .01), bz, wz, bound, xy, fg)}
            if offset == 0. and velocity == 0. and t in (5., 10., 14., 15., 32.):
                snapshots[f'{name}_t{int(t)}_bed_z_m'] = bz
                snapshots[f't{int(t)}_water_z_m'] = wz
        records.append(record)
        if index % 100 == 0:
            print(f'clearance {index}/{len(states)} states; elapsed {time.monotonic()-started:.1f}s', flush=True)
    summary = summarize([r for r in records if r['encounter_s'] <= 34.])
    late_orbit_summary = summarize([r for r in records if r['encounter_s'] > 34.])
    topology_pass = (geom['max_abs_xy_change_m'] == 0. and all(geom['unchanged_arrays'].values())
                     and geom['collapsed_native_float_triangles'] == 0 and geom['opposite_source_normals'] == 0
                     and geom['changed_originally_dry_vertices'] == 0 and geom['changed_true_exposed_or_lake_only_shore_faces'] == 0
                     and geom['changed_faces_touching_originally_dry_vertices'] == 0)
    sampled_static_pass = (summary['candidate']['hull_AABB']['states_with_bed_above_hull_bottom'] == 0
                           and summary['candidate']['hull_AABB']['states_with_exposed_bed'] == 0
                           and summary['candidate']['flatten_affected_outside_hull']['states_with_exposed_bed'] == 0)
    stable = source_sha == sha(args.source) and candidate_sha == sha(args.candidate) and source_hashes == source_contract()
    report = {'source': str(args.source.resolve()), 'source_sha256': source_sha,
              'candidate': str(args.candidate.resolve()), 'candidate_sha256': candidate_sha,
              'probe_sha256': sha(__file__), 'input_files_stable_during_probe': stable,
              'production_source_sha256': source_hashes, 'units': 'metres',
              'hull_recorded_world_AABB_m': [HMIN.tolist(), HMAX.tolist()],
              'geometry': geom, 'topology_and_visible_contacts_pass': topology_pass,
              'sampled_static_clearance_pass': sampled_static_pass,
              'conservative_wave_bound_clearance_pass': all(summary['candidate'][reg]['min_water_minus_wave_bound_above_bed_m'] >= 0. for reg in ('hull_AABB', 'flatten_affected_outside_hull')),
              'sampling': {'states': len(states), 'times_s': times, 'offsets_m': offsets.tolist(),
                           'velocities_m_s': [-13., 0., 13.], 'xy_samples_per_state': len(xy),
                           'hull_xy_samples': int(inside.sum()), 'terrain_faces_in_BVH': len(bed_faces)},
              'method': 'Actual source/candidate triangles ray cast after inverse arena pose; current production cataract and basin grids, polar chart and diagonals reconstructed with fail-closed source guards. Hull flatten and protected/lake gates evaluated at every water vertex before triangle ray queries. Full Cartesian steering limits overbound reachable states; repeated seeks have identical zero-state formula.',
              'limits': ['Finite spatial/time/offset samples do not prove continuous collision clearance.',
                         'Recorded resting HullShip AABB is a conservative footprint, not exact hull/deck mesh or collision; live ship jolts and animated components are not modelled.',
                         'Wave field is not sampled. Report subtracts a conservative per-face 4.528m maximum displacement norm, reduced by near-hull suppression. It is a vertical sensitivity check, not a proof against displaced XY vertices or changing wave topology.',
                         'Native float area checks approximate exported centimetre precision. This probe does not import, render or inspect the selected Nanite/fallback geometry; native validation is separate.',
                         'Lake centre fan is omitted because it is outside this descent/arrival envelope. Extra 40/50/60s orbit diagnostics are separated: known HeightAt corridor220m versus lower ribbon160m mismatch is outside this bed candidate scope. No full combat orbit or GPU material certification.',
                         'Hull-bottom comparisons outside the hull footprint are informational only: an uphill river legitimately rises above the hull elsewhere.'],
              'summary': summary, 'late_orbit_diagnostics': late_orbit_summary,
              'records': records, 'elapsed_s': time.monotonic()-started}
    report_text = json.dumps(report, indent=2)
    (out/'report.json').write_text(report_text)
    (out/f'report_{candidate_sha[:12]}_{report["probe_sha256"][:12]}.json').write_text(report_text)
    np.savez_compressed(out/'contacts.npz', xy_m=xy, inside_hull_aabb=inside, **snapshots)
    print(json.dumps({k: report[k] for k in ('topology_and_visible_contacts_pass', 'sampled_static_clearance_pass', 'geometry', 'summary', 'elapsed_s')}, indent=2), flush=True)
    if not (stable and topology_pass and sampled_static_pass):
        raise SystemExit(2)


if __name__ == '__main__':
    main()
