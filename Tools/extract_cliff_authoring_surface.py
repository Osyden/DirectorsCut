"""Remove only the explicit crop sides and 24m authoring back cap.

Keeps all real original 3D face positions and triangles, including overhangs.
This creates the open terrain patch for a truthful surface review. It does not
hide exposure transitions, repair shape, or reconstruct an XY heightfield.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_fractured_cliff_outcrop import sample_height


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a new ignored Saved output path')
    with np.load(args.input, allow_pickle=False) as archive:
        data = {k: archive[k].copy() for k in archive.files}
    meta = json.loads(str(data['metadata_json']))
    path = Path(meta['source_path'])
    if hashlib.sha256(path.read_bytes()).hexdigest() != meta['source_sha256']:
        raise ValueError('Changed original source')
    with np.load(path, allow_pickle=False) as archive:
        source = {k: archive[k].copy() for k in archive.files}
    basis = np.column_stack((data['basis_u_world'], data['basis_v_world'], data['basis_z_world']))
    vertices, triangles = data['vertices_m'], data['triangles']
    local = (vertices-data['frame_origin_world_m'])@basis
    if (meta.get('original_backing_depth_m') != 24.
        or not np.allclose([local[:, 0].min(), local[:, 0].max(), local[:, 1].min(), local[:, 1].max()],
                           [-36., 36., -20., 20.], atol=.0001, rtol=0)):
        raise ValueError('This explicit closure contract is only for the 72x40m, 24m-backed authoring crop')
    expected_bottom = sample_height(source, local[:, 0], local[:, 1], 89, 64)-data['frame_origin_world_m'][2]-24.
    on_bottom = np.abs(local[:, 2]-expected_bottom) <= .0001
    remove_bottom = on_bottom[triangles].all(axis=1)
    remove_side = np.zeros(len(triangles), dtype=bool)
    for axis, boundary in ((0, -36.), (0, 36.), (1, -20.), (1, 20.)):
        remove_side |= (np.abs(local[:, axis]-boundary) <= .0001)[triangles].all(axis=1)
    kept = triangles[~(remove_bottom | remove_side)]
    ids = np.unique(kept)
    remap = np.full(len(vertices), -1, dtype=np.int64)
    remap[ids] = np.arange(len(ids))
    data['vertices_m'] = vertices[ids]
    data['triangles'] = remap[kept]
    meta.update(authoring_closure_removed=dict(input_path=str(args.input.resolve()),
                input_sha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),
                bottom_triangles=int(remove_bottom.sum()), side_triangles=int(remove_side.sum()),
                position_tolerance_m=.0001, retained_triangles=len(kept),
                rule='Only faces wholly on explicit +/-36m X, +/-20m Y sides or source minus24m authoring underside',
                positions_changed=False, overhangs_projected=False),
                source_kind='original_cliff_exposure_surface', visual_accepted=False)
    data['metadata_json'] = np.asarray(json.dumps(meta))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(args.output, **data)
    print(json.dumps(meta['authoring_closure_removed'], indent=2))


if __name__ == '__main__':
    main()
