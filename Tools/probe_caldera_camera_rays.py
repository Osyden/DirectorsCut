"""Attribute screenshot pixels to source terrain and statically flattened water.

CPU only; run Blender --background --factory-startup --threads 1 --python THIS
-- --native-rays Saved/frame.json. Native deprojected rays are preferred. Camera
fallback takes --camera-location-cm X Y Z --camera-rotation-deg P Y R --fov-deg F
--encounter-time T. Pixel coordinates denote screen positions (centres end .5).
No scene is imported, rendered, or exported. The accepted terrain is the default;
an experimental --candidate must be explicitly named for a separate comparison.
"""
import argparse
import json
import math
from pathlib import Path
import sys
import time

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import probe_caldera_navigation_clearance as navigation

ROOT = Path(__file__).resolve().parents[1]


def rotation(pitch_yaw_roll):
    """UE FRotator acting on column vectors; camera +X forward,+Y right,+Z up."""
    pitch, yaw, roll = np.deg2rad(pitch_yaw_roll)
    cp, sp, cy, sy, cr, sr = np.cos(pitch), np.sin(pitch), np.cos(yaw), np.sin(yaw), np.cos(roll), np.sin(roll)
    return np.array([[cy, -sy, 0.], [sy, cy, 0.], [0., 0., 1.]]) @ np.array(
        [[cp, sp*sr, -sp*cr], [0., cr, sr], [sp, -cp*sr, cp*cr]])


def unit(vector):
    vector = np.asarray(vector, dtype=float)
    if vector.shape != (3,) or not np.isfinite(vector).all() or np.linalg.norm(vector) < 1.e-12:
        raise ValueError('Expected a finite nonzero three-component direction')
    return vector/np.linalg.norm(vector)


def cast(vertices, faces, origin, direction, max_distance=10000., max_hits=16):
    """Two-sided Moller-Trumbore on actual triangles, in bounded NumPy chunks."""
    hits = []
    direction = unit(direction)
    for start in range(0, len(faces), 40000):
        tri = vertices[faces[start:start+40000]]
        e1, e2 = tri[:, 1]-tri[:, 0], tri[:, 2]-tri[:, 0]
        pv = np.cross(direction, e2)
        det = np.einsum('ij,ij->i', e1, pv)
        valid = np.abs(det) > 1.e-10
        inv = np.divide(1., det, out=np.zeros_like(det), where=valid)
        tv = origin-tri[:, 0]
        u = np.einsum('ij,ij->i', tv, pv)*inv
        qv = np.cross(tv, e1)
        v = np.einsum('j,ij->i', direction, qv)*inv
        distance = np.einsum('ij,ij->i', e2, qv)*inv
        good = valid & (u >= -1.e-8) & (v >= -1.e-8) & (u+v <= 1.+1.e-8) & (distance >= 0.) & (distance <= max_distance)
        for index in np.flatnonzero(good):
            hits.append({'distance_m': float(distance[index]), 'triangle_index': int(start+index),
                         'barycentric': [float(1.-u[index]-v[index]), float(u[index]), float(v[index])],
                         'position_world_m': (origin+direction*distance[index]).tolist()})
    hits.sort(key=lambda h: h['distance_m'])
    # An exact shared-edge hit may have two equal-depth triangles. Keep those
    # IDs rather than silently selecting one; depth ordering uses the first.
    return hits[:max_hits]


def previous_coverage(world_position):
    p = np.asarray(world_position)
    return {'inside_previous_xy_rectangle': bool(-72. <= p[0] <= 72. and -20. <= p[1] <= 20.),
            'inside_recorded_hull_xy': bool(np.all(p[:2] >= navigation.HMIN[:2]) and np.all(p[:2] <= navigation.HMAX[:2])),
            'hull_ellipse_radius_m': float(np.linalg.norm(p[:2]*[.5, 2.])),
            'note': 'Rectangle membership is not an assertion this exact XY was previously sampled.'}


def water_mesh(rot, translation, scale, sea_level):
    vertices, faces, cataract = navigation.water_mesh()
    # Existing navigation probe omits the lake centre fan, which can fall on a
    # general camera ray. Restore the exact production fan here.
    cat_vertices = 241*65
    centre = len(vertices)
    vertices = np.r_[vertices, [[0., 0., -400.]]]
    sides = np.arange(768)
    fan = np.stack([np.full(768, centre), cat_vertices+sides+1, cat_vertices+sides], 1)
    faces = np.r_[faces, fan]
    cataract = np.r_[cataract, False]
    world = (vertices*scale)@rot.T+translation
    ellipse = np.linalg.norm(world[:, :2]*[.5, 2.], axis=1)
    gate = np.where(cataract, navigation.sm(.56, .58, np.linalg.norm(vertices[:, :2], axis=1)/1600.),
                    1.-navigation.sm(3., 8., np.abs(sea_level-world[:, 2])))
    flatten = gate*(1.-navigation.sm(12., 35., ellipse))
    world[:, 2] += (sea_level-world[:, 2])*flatten
    return vertices, world, faces, flatten


def water_triangle_identity(index):
    # navigation.grid_faces groups first and second cell triangles. Production
    # CreateMeshSection interleaves them; report both IDs without ambiguity.
    cataract_cells = 240*64
    basin_cells = 160*768
    if index < 2*cataract_cells:
        cell, half = index % cataract_cells, index//cataract_cells
        return {'component': 'EntryCataract', 'production_triangle_index': int(cell*2+half),
                'row': int(cell//64), 'side': int(cell % 64)}
    index -= 2*cataract_cells
    if index < 2*basin_cells:
        cell, half = index % basin_cells, index//basin_cells
        return {'component': 'Basin', 'production_triangle_index': int(cell*2+half),
                'row': int(cell//768), 'side': int(cell % 768)}
    return {'component': 'Basin', 'production_triangle_index': int(2*basin_cells+index-2*basin_cells), 'centre_fan': True}


def decorate(hit, vertices, world, faces, inverse_pose, water_flatten=None):
    face = faces[hit['triangle_index']]
    point = np.asarray(hit['position_world_m'])
    hit['vertex_indices'] = face.tolist()
    hit['triangle_vertices_arena_m'] = vertices[face].tolist()
    hit['triangle_vertices_world_m'] = world[face].tolist()
    hit['rest_position_arena_m'] = (np.asarray(hit['barycentric'])@vertices[face]).tolist()
    hit['position_arena_after_displacement_m'] = inverse_pose(point).tolist()
    hit['prior_sampling_coverage'] = previous_coverage(point)
    if water_flatten is not None:
        hit.update(water_triangle_identity(hit['triangle_index']))
        hit['interpolated_vertex_flatten'] = float(np.asarray(hit['barycentric'])@water_flatten[face])
    return hit


def arguments():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native-rays', type=Path)
    p.add_argument('--camera-location-cm', nargs=3, type=float)
    p.add_argument('--camera-rotation-deg', nargs=3, type=float, metavar=('PITCH', 'YAW', 'ROLL'))
    p.add_argument('--fov-deg', type=float, help='Effective horizontal FOV of the supplied viewport')
    p.add_argument('--width', type=int, default=1600)
    p.add_argument('--height', type=int, default=900)
    p.add_argument('--pixels', nargs='+', default=['400.5,780.5', '450.5,740.5', '250.5,820.5', '600.5,780.5'])
    p.add_argument('--encounter-time', type=float)
    p.add_argument('--arena-location-cm', nargs=3, type=float)
    p.add_argument('--arena-rotation-deg', nargs=3, type=float)
    p.add_argument('--arena-scale', nargs=3, type=float, default=[1., 1., 1.])
    p.add_argument('--offset-m', type=float, default=0.)
    p.add_argument('--velocity-m-s', type=float, default=0.)
    p.add_argument('--sea-level-cm', type=float, default=230.)
    p.add_argument('--source', type=Path, default=navigation.DEFAULT_SOURCE)
    p.add_argument('--candidate', type=Path, help='Optional explicit alternate mesh; never replaces source')
    p.add_argument('--max-distance-m', type=float, default=10000.)
    p.add_argument('--max-hits', type=int, default=12)
    p.add_argument('--output', type=Path, default=ROOT/'Saved/CalderaCameraRays')
    args = p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.native_rays:
        frame = json.loads(args.native_rays.read_text(encoding='utf-8-sig'))
        for key in ('encounter_time_s', 'arena_location_cm', 'arena_rotation_deg', 'rays'):
            if key not in frame:
                p.error('Native ray input lacks '+key)
        args.encounter_time = float(frame['encounter_time_s'])
        args.arena_location_cm = frame['arena_location_cm']
        args.arena_rotation_deg = frame['arena_rotation_deg']
        args.arena_scale = frame.get('arena_scale', [1., 1., 1.])
        args.sea_level_cm = float(frame.get('sea_level_cm', args.sea_level_cm))
        rays = [{'pixel': r['pixel'], 'origin_m': np.asarray(r['origin_cm'], dtype=float)*.01,
                 'direction': unit(r['direction'])} for r in frame['rays']]
        mode = 'native viewport-deprojected rays; no FOV or camera reconstruction'
    else:
        if any(value is None for value in (args.camera_location_cm, args.camera_rotation_deg, args.fov_deg, args.encounter_time)):
            p.error('Supply --native-rays or explicit camera location/rotation/FOV and encounter time')
        if not 0. < args.fov_deg < 179. or min(args.width, args.height) <= 0:
            p.error('Invalid FOV or viewport dimensions')
        frame = {'camera_location_cm': args.camera_location_cm, 'camera_rotation_deg': args.camera_rotation_deg,
                 'horizontal_fov_deg': args.fov_deg, 'viewport': [args.width, args.height]}
        cam_rot = rotation(args.camera_rotation_deg)
        focal = .5*args.width/math.tan(math.radians(args.fov_deg)*.5)
        rays = []
        for value in args.pixels:
            pixel = [float(x) for x in value.split(',')]
            if len(pixel) != 2:
                p.error('Pixels use X,Y screen coordinates')
            direction = unit(cam_rot@np.array([1., (pixel[0]-.5*args.width)/focal, (.5*args.height-pixel[1])/focal]))
            rays.append({'pixel': pixel, 'origin_m': np.asarray(args.camera_location_cm)*.01, 'direction': direction})
        mode = 'pinhole camera fallback; assumes effective horizontal FOV, no crop/letterbox/jitter'
    if not rays or len(rays) > 32:
        p.error('Supply between1 and32 rays for this bounded CPU diagnostic')
    if (args.arena_location_cm is None) != (args.arena_rotation_deg is None):
        p.error('Exact arena location and rotation must be supplied together')
    return args, frame, rays, mode


def main():
    args, frame, rays, ray_mode = arguments()
    started = time.monotonic()
    production = navigation.source_contract()
    sea_level = args.sea_level_cm*.01
    if args.arena_location_cm is not None:
        rot = rotation(args.arena_rotation_deg)
        translation = np.asarray(args.arena_location_cm, dtype=float)*.01
        pose_mode = 'exact supplied native arena transform'
    else:
        rot, translation = navigation.pose(args.encounter_time, args.offset_m, args.velocity_m_s)
        translation[2] += sea_level-2.3
        pose_mode = 'analytical route pose; RideTime=270+encounter_time, requested steering state'
    scale = np.asarray(args.arena_scale, dtype=float)
    if not np.isfinite(scale).all() or np.any(np.abs(scale) < 1.e-9):
        raise ValueError('Arena scale must be finite and nonsingular')
    inverse_pose = lambda point: ((point-translation)@rot)/scale
    wv, ww, wf, flatten = water_mesh(rot, translation, scale, sea_level)
    water_hits = []
    for ray in rays:
        hh = cast(ww, wf, ray['origin_m'], ray['direction'], args.max_distance_m, args.max_hits)
        water_hits.append([decorate(h, wv, ww, wf, inverse_pose, flatten) for h in hh])
    sources = {}
    for name, path in [('source', args.source), ('candidate', args.candidate)]:
        if path is None:
            continue
        before = navigation.sha(path)
        with np.load(path, allow_pickle=False) as data:
            vertices, faces = data['vertices_m'].copy(), data['triangles'].copy()
        world = (vertices*scale)@rot.T+translation
        results = []
        for index, ray in enumerate(rays):
            hits = cast(world, faces, ray['origin_m'], ray['direction'], args.max_distance_m, args.max_hits)
            for hit in hits:
                decorate(hit, vertices, world, faces, inverse_pose)
                at = np.asarray(hit['position_world_m'])
                origin = at.copy(); origin[2] = max(float(ww[:, 2].max()), at[2])+100.
                vertical = cast(ww, wf, origin, np.array([0., 0., -1.]), 20000., 1)
                hit['composite_water_vertical_clearance_m'] = None if not vertical else vertical[0]['position_world_m'][2]-at[2]
                hit['composite_water_vertical_triangle'] = None if not vertical else water_triangle_identity(vertical[0]['triangle_index'])
            wh = water_hits[index]
            order = 'neither_hit' if not hits and not wh else 'water_only' if not hits else 'terrain_only' if not wh else ('water_first' if wh[0]['distance_m'] < hits[0]['distance_m'] else 'terrain_first')
            results.append({'pixel': ray['pixel'], 'ray_origin_world_m': ray['origin_m'].tolist(),
                            'ray_direction_world': ray['direction'].tolist(), 'first_surface_order': order,
                            'terrain_minus_water_ray_distance_m': None if not hits or not wh else hits[0]['distance_m']-wh[0]['distance_m'],
                            'terrain_hits': hits, 'flattened_water_hits': wh})
            print(f'{name} pixel{ray["pixel"]}: {order}; terrain {hits[0]["distance_m"] if hits else None}m, water {wh[0]["distance_m"] if wh else None}m', flush=True)
        sources[name] = {'path': str(path.resolve()), 'sha256': before, 'file_stable': before == navigation.sha(path),
                         'vertices': len(vertices), 'triangles': len(faces), 'rays': results}
    report = {'schema': 'directorscut.caldera_camera_rays.v1', 'probe_sha256': navigation.sha(__file__),
              'shared_navigation_probe_sha256': navigation.sha(navigation.__file__),
              'production_source_sha256': production, 'input_frame': frame, 'ray_mode': ray_mode,
              'encounter_time_s': args.encounter_time, 'pose_mode': pose_mode,
              'arena_location_world_m': translation.tolist(), 'arena_rotation_matrix': rot.tolist(),
              'arena_scale': scale.tolist(), 'sea_level_m': sea_level, 'sources': sources,
              'limits': ['Source triangles, not selected Nanite clusters or fallback render geometry.',
                         'Water contains exact reconstructed grid connectivity and per-vertex static hull flatten; live FFT/swell displacement is omitted.',
                         'Two-sided geometric rays do not model opacity, transmission, depth pass, temporal reconstruction, or GPU visibility.',
                         'Other scene objects, ship, bank instances and fog are not included.',
                         'Camera fallback assumes an effective horizontal FOV and full viewport. Native deprojected rays avoid that assumption.',
                         'Previous sampling membership refers only to its rectangle, not continuous clearance certification.'],
              'elapsed_s': time.monotonic()-started}
    args.output.mkdir(parents=True, exist_ok=True)
    path = args.output/'report.json'; path.write_text(json.dumps(report, indent=2))
    print(f'Report {path}; {report["elapsed_s"]:.2f}s', flush=True)


if __name__ == '__main__':
    main()
