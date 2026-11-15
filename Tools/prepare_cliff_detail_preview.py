"""Place installed, native-size cliff outcrops on an audited preview surface.

Offline NumPy only. This writes a hash-bound opt-in JSON, not Unreal assets.
Asset geometry/materials stay installed and unchanged. Burial is measured
against each sampled triangle plane using the native bounding-box projection;
it is not a claim about exact buried solid volume or final visual quality.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def read(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def segment_distance(point, starts, ends):
    edge = ends-starts
    u = np.clip(np.sum((point-starts)*edge, axis=1)/np.maximum(np.sum(edge*edge, axis=1), 1.e-20), 0, 1)
    return float(np.linalg.norm(point-starts-u[:, None]*edge, axis=1).min())


def polygon_distance(point, polygon):
    edge = np.roll(polygon, -1, axis=0)-polygon
    delta = point-polygon
    cross = edge[:, 0]*delta[:, 1]-edge[:, 1]*delta[:, 0]
    if np.all(cross >= -1.e-8) or np.all(cross <= 1.e-8):
        return 0.
    return segment_distance(point, polygon, np.roll(polygon, -1, axis=0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project-root', type=Path, required=True)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--maximum', type=int, default=80)
    parser.add_argument('--seed', type=int, default=151106)
    args = parser.parse_args()
    if not 1 <= args.maximum <= 100:
        raise ValueError('Maximum must remain in1..100 for this bounded preview')
    root = args.project_root.resolve()
    output = args.output.resolve()
    if 'saved' not in [p.lower() for p in output.parts]:
        raise ValueError('Generated placement contracts belong under Saved')
    active = read(root/'Saved/CliffLandformPreviewImport.json')
    sidecar = read(active['export_sidecar'])
    source_hash = sha(args.input)
    if sidecar.get('input_sha256') != source_hash or sha(active['source_fbx']) != active['source_sha256']:
        raise ValueError('Input surface/activeFBX identity mismatch')
    source = np.load(args.input, allow_pickle=False)
    vertices, triangles = source['vertices_m'], source['triangles']
    xyz = vertices[triangles]
    centers = xyz.mean(axis=1)
    cross = np.cross(xyz[:, 1]-xyz[:, 0], xyz[:, 2]-xyz[:, 0])
    twice_area = np.linalg.norm(cross, axis=1)
    normals = cross/np.maximum(twice_area[:, None], 1.e-20)
    angles = np.mod(np.arctan2(centers[:, 1], centers[:, 0]), 2*np.pi)
    inward = -centers[:, :2]/np.maximum(np.linalg.norm(centers[:, :2], axis=1)[:, None], 1.e-20)
    angle_min, angle_max = np.array([568, 591])*2*np.pi/1536
    eligible = np.flatnonzero((angles >= angle_min)&(angles <= angle_max)
        &(centers[:, 2] > -75)&(centers[:, 2] < 25)&(np.abs(normals[:, 2]) < .55)
        &(np.sum(normals[:, :2]*inward, axis=1) > .4)&(twice_area > .05))
    if not len(eligible):
        raise ValueError('No eligible inward-facing steep surface')

    # Actual measured finite water domain; reject even the projected bounding
    # footprint, with5m extra separation. This is local mean-water protection,
    # not a claim of full dynamic route/collision validation.
    water_path = root/'Saved/StructuralCliff/mean_water.npz'
    water = np.load(water_path, allow_pickle=False)
    water_xy = water['vertices_m'][water['triangles'], :2]
    water_lo, water_hi = water_xy.min(axis=1), water_xy.max(axis=1)
    directed = np.concatenate([water['triangles'][:, [0, 1]], water['triangles'][:, [1, 2]], water['triangles'][:, [2, 0]]])
    edges, count = np.unique(np.sort(directed, axis=1), axis=0, return_counts=True)
    boundary = water['vertices_m'][edges[count == 1], :2]

    def water_distance(p):
        hit = np.all(water_hi >= p, axis=1)&np.all(water_lo <= p, axis=1)
        near = water_xy[hit]
        if len(near):
            e = np.roll(near, -1, axis=1)-near
            d = p-near
            c = e[:, :, 0]*d[:, :, 1]-e[:, :, 1]*d[:, :, 0]
            if np.any(np.all(c >= -1.e-7, axis=1)|np.all(c <= 1.e-7, axis=1)):
                return 0.
        return segment_distance(p, boundary[:, 0], boundary[:, 1])

    anchor_path = root/'Saved/CliffSupportAnchors.json'
    anchors = [(a['id'], np.asarray(a['footprint_xy_m'])) for a in read(anchor_path)['anchors']]
    dimensions_path = root/'Saved/InstalledCliffDimensions.json'
    assets = read(dimensions_path)
    if len(assets) != 3 or any(not a['asset'].startswith('/Game/AncientRuins/Meshes/Cliffs/SM_Cliff_0') for a in assets):
        raise ValueError('Unexpected installed cliff asset list')
    # Guard the fitted sheets as well as the sea. Each centreline is expanded
    # by its actual widest cross-section and2m. The rock's circumscribed sphere
    # is deliberately conservative; this is not a mist/particle collision test.
    fall_descriptor = root/'Saved/CliffPreviewWaterfall.json'
    fall_binding = read(fall_descriptor)
    if fall_binding['source_candidate_sha256'] != source_hash:
        raise ValueError('Active waterfall is bound to a different cliff')
    profile_path = Path(fall_binding['profile_path'])
    fall_profile = read(profile_path)
    fall_paths = []
    for outlet in fall_profile.get('ballistic_outlets', [fall_profile]):
        if outlet['source_mesh_sha256'] != source_hash:
            raise ValueError('Waterfall outlet source differs from cliff')
        centerline = np.asarray(outlet['samples_local_cm'])*.01
        sections = np.asarray(outlet['cross_sections_local_cm'])*.01
        if sections.shape[0] != len(centerline) or sections.shape[-1] != 3:
            raise ValueError('Unexpected waterfall cross-section layout')
        radius = float(np.linalg.norm(sections-centerline[:, None], axis=-1).max())
        basis = np.asarray([outlet['basis_x_arena'], outlet['basis_y_arena'], outlet['basis_z_arena']])
        arena_path = centerline@basis+np.asarray(outlet['local_origin_arena_cm'])*.01
        fall_paths.append((arena_path, radius))
    rng = np.random.default_rng(args.seed)
    weights = twice_area[eligible]/twice_area[eligible].sum()
    picks = rng.choice(eligible, 18000, p=weights)
    placements = []
    rejects = dict(spacing=0, water=0, supports=0, waterfall=0, dimensions=0, band=0)
    for face_id in picks:
        uv = rng.random(2)
        if uv.sum() > 1:
            uv = 1-uv
        bary = np.array([1-uv.sum(), uv[0], uv[1]])
        point = bary@xyz[face_id]
        normal = normals[face_id]
        angle = np.mod(np.arctan2(point[1], point[0]), 2*np.pi)
        if not angle_min <= angle <= angle_max or not -75 < point[2] < 25:
            rejects['band'] += 1
            continue
        spacing = float(rng.uniform(10, 14))
        if any(np.linalg.norm(point-np.asarray(x['surface_arena_m'])) < max(spacing, x['spacing_m']) for x in placements):
            rejects['spacing'] += 1
            continue
        asset_id = int(rng.choice(3, p=[.15, .60, .25]))
        asset = assets[asset_id]
        scale = float(rng.uniform(.85, min(1.05, 15./asset['size_m'][1])))
        yaw = float(np.arctan2(normal[1], normal[0])-np.pi/2)
        rotation = np.array([[np.cos(yaw), -np.sin(yaw), 0], [np.sin(yaw), np.cos(yaw), 0], [0, 0, 1.]])
        size = np.asarray(asset['size_m'])*scale
        normal_depth = float(np.abs(rotation.T@normal)@size)
        low, high = max(1., normal_depth*.20), min(3., normal_depth*.40)
        if low > high:
            rejects['dimensions'] += 1
            continue
        relief = float(rng.uniform(low, high))
        box_center = point-normal*(normal_depth*.5-relief)
        half_world = np.abs(rotation)@(size*.5)
        if box_center[2]-half_world[2] < -75 or box_center[2]+half_world[2] > 25:
            rejects['band'] += 1
            continue
        footprint_radius = float(np.linalg.norm(half_world[:2]))
        shore_distance = water_distance(box_center[:2])
        if shore_distance < footprint_radius+5:
            rejects['water'] += 1
            continue
        support_clearance = min(polygon_distance(box_center[:2], polygon) for _, polygon in anchors)-footprint_radius
        if support_clearance < 8:
            rejects['supports'] += 1
            continue
        fall_clearance = min(segment_distance(box_center, path[:-1], path[1:])-radius
            for path, radius in fall_paths)-float(np.linalg.norm(half_world))
        if fall_clearance < 2:
            rejects['waterfall'] += 1
            continue
        # Reject bounding footprints straying beyond this angular preview sector.
        corners = box_center[:2]+np.array([[-1,-1],[-1,1],[1,-1],[1,1]])*half_world[:2]
        corner_angles = np.mod(np.arctan2(corners[:, 1], corners[:, 0]), 2*np.pi)
        if corner_angles.min() < angle_min or corner_angles.max() > angle_max:
            rejects['band'] += 1
            continue
        placements.append(dict(asset_index=asset_id, surface_arena_m=point.tolist(), normal_arena=normal.tolist(),
            yaw_degrees=float(np.degrees(yaw)), uniform_scale=scale, relief_m=relief,
            plane_depth_m=normal_depth, buried_depth_fraction=1-relief/normal_depth,
            box_center_arena_m=box_center.tolist(), box_half_extent_arena_m=half_world.tolist(),
            spacing_m=spacing, source_triangle=int(face_id), source_barycentric=bary.tolist(),
            water_footprint_clearance_m=shore_distance-footprint_radius, support_footprint_clearance_m=support_clearance,
            waterfall_envelope_clearance_m=fall_clearance))
        if len(placements) >= args.maximum:
            break
    if not placements:
        raise ValueError('All placements rejected by physical guards')
    report = dict(schema='directorscut.cliff_detail_preview.v1', cliff_fbx_sha256=active['source_sha256'],
        source_path=str(args.input.resolve()), source_candidate_sha256=source_hash,
        generator_sha256=sha(__file__), seed=args.seed, units='arena metres; installed mesh coordinates are native Unreal centimetres',
        source_water_path=str(water_path), source_water_sha256=sha(water_path),
        source_anchors_path=str(anchor_path), source_anchors_sha256=sha(anchor_path),
        source_fall_profile_path=str(profile_path), source_fall_profile_sha256=sha(profile_path),
        assets=assets, instances=placements, eligible_triangle_count=len(eligible),
        eligible_surface_area_m2=float(twice_area[eligible].sum()*.5), rejects=rejects,
        metrics=dict(instances=len(placements), total_source_mesh_triangles=sum(assets[x['asset_index']]['triangles'] for x in placements),
            minimum_water_footprint_clearance_m=min(x['water_footprint_clearance_m'] for x in placements),
            minimum_support_footprint_clearance_m=min(x['support_footprint_clearance_m'] for x in placements),
            minimum_waterfall_envelope_clearance_m=min(x['waterfall_envelope_clearance_m'] for x in placements),
            uniform_scale_range=[min(x['uniform_scale'] for x in placements), max(x['uniform_scale'] for x in placements)],
            relief_range_m=[min(x['relief_m'] for x in placements), max(x['relief_m'] for x in placements)]),
        limitations='Opt-in native-size visual comparison only. Native UVs/materials untouched. Bounding-box plane burial, not exact solid volume. Local static water guards; no full route or visual acceptance.',
        visual_accepted=False, runtime_integrated=False)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(output=str(output), **report['metrics']), indent=2))


if __name__ == '__main__':
    main()
