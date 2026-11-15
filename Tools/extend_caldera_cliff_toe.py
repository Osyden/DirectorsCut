"""Extend the existing complete caldera toe below its lake, preserving its grid."""
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
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--toe-height-m', type=float, default=-445.)
    # The accepted source contains overhangs above row0. Moving six rows was
    # measured to reverse 1392 faces; lowering row0 alone has no reversals.
    parser.add_argument('--feather-rows', type=int, default=1)
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a fresh output directory under Saved')
    with np.load(args.source, allow_pickle=False) as archive:
        data = {key: archive[key].copy() for key in archive.files}
    metadata = json.loads(str(data['metadata_json']))
    rows, columns = metadata['grid_shape']
    if not metadata.get('entry_cut', {}).get('original_topology_and_uvs_retained'):
        raise RuntimeError('Source must be the complete ring with its audited inlet cut')
    if not 1 <= args.feather_rows < metadata['crest_row_index'] or args.toe_height_m >= -440.:
        raise RuntimeError('Extension must finish below -440m and below the crest')
    original = data['vertices_m'].copy()
    vertices = original.copy()
    grid = vertices.reshape(rows, columns, 3)
    delta = np.maximum(grid[0, :, 2] - args.toe_height_m, 0.)
    for row in range(args.feather_rows):
        t = row / args.feather_rows
        weight = 1. - t*t*(3.-2.*t)
        grid[row, :, 2] -= delta * weight
    grid[:, -1, 2] = grid[:, 0, 2]
    if not np.array_equal(vertices[:, :2], original[:, :2]):
        raise RuntimeError('Planar geometry changed')
    if not np.array_equal(grid[args.feather_rows:], original.reshape(rows, columns, 3)[args.feather_rows:]):
        raise RuntimeError('Extension changed the upper cliff')
    if float(grid[0, :, 2].max()) > args.toe_height_m + 1.e-8:
        raise RuntimeError('A terminal vertex remains above the submerged toe')
    faces = data['triangles']
    origin = data['frame_origin_world_m']
    native = (vertices-origin).astype(np.float32).astype(np.float64)
    native_old = (original-origin).astype(np.float32).astype(np.float64)
    min_area = float('inf'); collapsed = 0; reversed_normals = 0; changed_faces = 0
    for start in range(0, len(faces), 100000):
        f = faces[start:start+100000]
        p, old = native[f], native_old[f]
        cross = np.cross(p[:, 1]-p[:, 0], p[:, 2]-p[:, 0])
        old_cross = np.cross(old[:, 1]-old[:, 0], old[:, 2]-old[:, 0])
        area = np.linalg.norm(cross, axis=1)
        min_area = min(min_area, float(area.min()))
        collapsed += int(np.count_nonzero(area < 1.e-10))
        changed = np.any(p != old, axis=(1, 2))
        changed_faces += int(changed.sum())
        reversed_normals += int(np.count_nonzero(np.einsum('ij,ij->i', cross[changed], old_cross[changed]) < 0))
        # XY is unchanged: the signed projected triangle area must be exact.
        if not np.array_equal(cross[:, 2], old_cross[:, 2]):
            raise RuntimeError('Extension changed projected face winding')
    if collapsed or reversed_normals:
        raise RuntimeError(f'Native triangles: {collapsed} collapsed, {reversed_normals} normal reversals')
    toe = grid[0, :-1]
    radius = np.linalg.norm(toe[:, :2], axis=1)
    centers = [[float(toe[i, 0]), float(toe[i, 1]), -410.] for i in range(0, columns-1, (columns-1)//32)]
    if len(centers) != 32:
        raise RuntimeError('Expected 32 evenly spaced source toe columns')
    extension = dict(parent_source_path=str(args.source.resolve()), parent_source_sha256=sha(args.source),
        terminal_row=0, feather_rows=args.feather_rows, toe_height_max_m=args.toe_height_m,
        maximum_lowering_m=float(delta.max()), changed_vertices=int(np.count_nonzero(vertices[:, 2] != original[:, 2])),
        changed_faces=changed_faces, native_normal_reversals=reversed_normals,
        upper_faces_unchanged=True, planar_coordinates_unchanged=True, topology_and_uvs_unchanged=True,
        entry_cut_preserved=True, material_ancestor_sha256=metadata['entry_cut']['source_sha256'])
    metadata['toe_extension'] = extension
    metadata['shore_mist_centers_m'] = centers
    metadata['seam_position_error_m'] = float(np.linalg.norm(grid[:, 0]-grid[:, -1], axis=1).max())
    metadata['exported_collapsed_triangles'] = collapsed
    metadata['exported_minimum_twice_triangle_area_m2'] = min_area
    data['vertices_m'] = vertices
    data['metadata_json'] = np.asarray(json.dumps(metadata))
    args.output.mkdir(parents=True)
    target = args.output/'candidate.npz'
    np.savez_compressed(target, **data)
    report = dict(schema=metadata['schema'], source=str(target.resolve()), source_sha256=sha(target),
        vertices=len(vertices), triangles=len(faces), grid_shape=[rows, columns],
        entry_cut=metadata['entry_cut'], toe_extension=extension, shore_mist_centers_m=centers,
        toe_radius_min_m=float(radius.min()), toe_radius_median_m=float(np.median(radius)), toe_radius_max_m=float(radius.max()),
        toe_z_min_m=float(toe[:, 2].min()), toe_z_max_m=float(toe[:, 2].max()),
        seam_position_error_m=metadata['seam_position_error_m'], exported_collapsed_triangles=collapsed,
        exported_minimum_twice_triangle_area_m2=min_area)
    (args.output/'manifest.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k != 'shore_mist_centers_m'}, indent=2))


if __name__ == '__main__':
    main()
