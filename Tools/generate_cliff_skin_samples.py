"""Sample a bounded curved bedrock skin for structural fracture clustering.

No mesh deformation or material noise. Points occupy the actual measured crop
in metres, with stable IDs and depth below the source surface. This authoring
cloud is not the final visible geometry and does not certify a full arena join.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_fractured_cliff_outcrop import sample_height, validate_crop


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--spacing-m', type=float, default=1.15)
    parser.add_argument('--row', type=int, default=89)
    parser.add_argument('--col', type=int, default=64)
    parser.add_argument('--width-m', type=float, default=72.)
    parser.add_argument('--depth-m', type=float, default=40.)
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a new ignored Saved output path')
    if not .8 <= args.spacing_m <= 2.5:
        parser.error('Use 0.8–2.5m physical point spacing')
    if not 40. <= args.width_m <= 180. or not 24. <= args.depth_m <= 110.:
        parser.error('Keep this bounded proof within40–180m width and24–110m depth')
    with np.load(args.source, allow_pickle=False) as archive:
        source = {k: archive[k].copy() for k in archive.files}
    row, col = args.row, args.col
    cell=float(source['cell_size_m'])
    if any(abs(size/(2*cell)-round(size/(2*cell)))>1.e-9 for size in (args.width_m,args.depth_m)):
        parser.error('Crop sides must align exactly to the original source grid')
    crop = validate_crop(source, row, col, args.width_m, args.depth_m, 4.)
    z_origin = float(source['height_m'][row, col])
    origin = np.r_[source['world_xy_m'][row, col], z_origin]
    basis = np.column_stack((source['basis_u_world'], source['basis_v_world'], source['basis_up_world']))
    fit_u,fit_v=min(30.,args.width_m*15./72.),min(24.,args.depth_m*9./40.)
    u, v = np.meshgrid(np.linspace(-fit_u,fit_u,31), np.linspace(-fit_v,fit_v,19))
    h = sample_height(source, u, v, row, col)-z_origin
    plane = np.linalg.lstsq(np.c_[u.ravel(), v.ravel(), np.ones(u.size)], h.ravel(), rcond=None)[0]
    front = np.r_[-plane[:2], 1.]
    front /= np.linalg.norm(front)
    up = np.array([0., 0., 1.])-front*front[2]
    up /= np.linalg.norm(up)
    rotation = np.column_stack((np.cross(front, up), front, up))
    anchor = np.array([0., 0., plane[2]])
    # A jittered physical 3D lattice samples volume, not a displaced 2D surface.
    # The conservative min-axis separation is 0.7*spacing; it is not Poisson.
    step = args.spacing_m
    half_u,half_v=args.width_m/2.-6.,args.depth_m/2.-6.
    x, y, z = np.meshgrid(np.arange(-half_u,half_u+step,step), np.arange(-half_v,half_v+step,step),
                         np.arange(crop['source_height_range_m'][0]-z_origin-14.,
                                   crop['source_height_range_m'][1]-z_origin+step, step), indexing='ij')
    points = np.c_[x.ravel(), y.ravel(), z.ravel()]
    points += np.random.default_rng(20261115).uniform(-.15*step, .15*step, points.shape)
    points = points[(np.abs(points[:, 0]) < half_u) & (np.abs(points[:, 1]) < half_v)]
    surface = sample_height(source, points[:, 0], points[:, 1], row, col)-z_origin
    vertical_depth = surface-points[:, 2]
    keep = (vertical_depth >= .15) & (vertical_depth <= 14.)
    points, vertical_depth = points[keep], vertical_depth[keep]
    if len(points)>24000:
        raise ValueError('Point cloud exceeds bounded24000-point clustering proof; increase physical spacing')
    px, py = points[:, 0], points[:, 1]
    hx = (sample_height(source, px+.5, py, row, col)-sample_height(source, px-.5, py, row, col))
    hy = (sample_height(source, px, py+.5, row, col)-sample_height(source, px, py-.5, row, col))
    approximate_normal_depth = vertical_depth/np.sqrt(1.+hx*hx+hy*hy)
    canonical = (points-anchor)@rotation
    report = dict(schema='cliff_skin_points_v1', points_m=canonical.tolist(),
                  depth_below_face_m=approximate_normal_depth.tolist(),
                  source_mesh_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),
                  frame=dict(origin_arena_m=(origin+basis@anchor).tolist(),
                             basis_columns_arena=(basis@rotation).tolist(),
                             canonical_to_crop_rotation=rotation.tolist(), canonical_origin_crop_m=anchor.tolist()),
                  metadata=dict(source_path=str(args.source.resolve()), crop=crop, units='metres',
                                up_axis='Z', front_direction=[0, 1, 0], point_ids='stable zero-based array index',
                                spacing_m=step, seed=20261115, jitter_fraction=.15,
                                conservative_minimum_separation_m=.7*step,
                                count=len(canonical), bounds_m=[canonical.min(axis=0).tolist(), canonical.max(axis=0).tolist()],
                                depth_kind='Approximate source-normal depth from exact source height and local one-metre slope estimate',
                                vertical_depth_interval_m=[.15, 14.],
                                point_inner_half_extents_m=[half_u,half_v],point_guard_inset_m=6.,
                                frame_fit_half_extents_m=[fit_u,fit_v],
                                original_vertical_backing_m=24., new_runtime_geometry=False,
                                limitations='Bounded inner crop; depths guide cluster removal only, not final collision or support queries.'))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, separators=(',', ':'))+'\n', encoding='utf-8')
    print(json.dumps(report['metadata'], indent=2))


if __name__ == '__main__':
    main()
