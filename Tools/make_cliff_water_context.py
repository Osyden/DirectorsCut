"""Convert existing measured mean-water queries to an optional clay context mesh.

Uses HeightAt(P,false) and WaterEdgeRadius coverage from the existing exporter.
No water extrapolation beyond queried coverage, no terrain changes, no waves.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from review_eroded_cliff_sector import triangles


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--heightfield', type=Path, required=True)
    parser.add_argument('--queries', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Choose a new ignored Saved output')
    with np.load(args.heightfield, allow_pickle=False) as archive:
        data = {key: archive[key].copy() for key in archive.files}
    q = np.fromfile(args.queries, dtype='<f8').reshape(*data['height_m'].shape, 4)
    if not np.isfinite(q).all() or not np.allclose(q[..., :2], data['world_xy_m'], atol=1.e-9, rtol=0):
        raise ValueError('Query XY does not match the physical source grid')
    faces = triangles(q[..., 3] >= 0)
    used = np.unique(faces)
    remap = np.full(q.shape[0]*q.shape[1], -1, dtype=np.int64)
    remap[used] = np.arange(len(used))
    vertices = q[..., :3].reshape(-1, 3)[used]
    if not len(faces):
        raise ValueError('No measured receiving water in this sector')
    metadata = dict(source_kind='measured_analytic_mean_water_context',
                    source_queries=str(args.queries.resolve()),
                    query_sha256=hashlib.sha256(args.queries.read_bytes()).hexdigest(),
                    heightfield=str(args.heightfield.resolve()),
                    heightfield_sha256=hashlib.sha256(args.heightfield.read_bytes()).hexdigest(),
                    vertices=len(vertices), triangles=len(faces),
                    coverage='All triangle vertices inside measured WaterEdgeRadius; conservative at grid boundary',
                    limitation='Static analytic mean water only; no current wave/WPO, no inner-cutoff extrapolation outside this rim sector')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(args.output, vertices_m=vertices, triangles=remap[faces],
                        frame_origin_world_m=data['frame_origin_world_m'],
                        basis_u_world=data['basis_u_world'], basis_v_world=data['basis_v_world'],
                        basis_z_world=data['basis_up_world'], metadata_json=np.asarray(json.dumps(metadata)))
    print(json.dumps(metadata))


if __name__ == '__main__':
    main()
