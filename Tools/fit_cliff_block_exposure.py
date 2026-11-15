"""Fit one metre-scale implicit rock face into the measured offline cliff crop.

Blender CPU only. This is a bounded geometry experiment, not a runtime asset.
The input is Z-up with its exposed side +Y. A rigid transform aligns it with
the crop's measured slope; no scaling or sample tiling is performed. A shallow
elliptic exposure removes the old soft skin, then the new solid is united with
the retained bedrock. Actual original triangles, not a heightfield, are saved.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_fractured_cliff_outcrop import (
    apply_boolean, arrays, clean_boolean_mesh, closed_shell, mesh_metrics,
    mesh_object, sample_height, validate_crop, write_npz,
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--face', type=Path, required=True)
    parser.add_argument('--metadata', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--front-offset-m', type=float, default=0.)
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a NEW directory under ignored Saved')
    meta = json.loads(args.metadata.read_text(encoding='utf-8'))
    if (meta['units'] != 'metres' or meta['up_axis'] != 'Z'
        or meta['front_direction'] != [0, 1, 0] or meta['mesh_sha256'] != sha(args.face)):
        raise ValueError('Need the verified original Z-up metre-scale +Y face')
    if abs(args.front_offset_m) > 3:
        raise ValueError('This proof permits at most 3m normal placement offset')
    import bpy
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    with np.load(args.source, allow_pickle=False) as archive:
        source = {k: archive[k].copy() for k in archive.files}
    row, col, width, depth, guard = 89, 64, 72., 40., 4.
    crop = validate_crop(source, row, col, width, depth, guard)
    z_origin = float(source['height_m'][row, col])
    origin = np.r_[source['world_xy_m'][row, col], z_origin]
    basis = np.column_stack((source['basis_u_world'], source['basis_v_world'], source['basis_up_world']))
    xx, yy = np.meshgrid(np.linspace(-15., 15., 31), np.linspace(-9., 9., 19))
    zz = sample_height(source, xx, yy, row, col)-z_origin
    design = np.column_stack((xx.ravel(), yy.ravel(), np.ones(xx.size)))
    slope = np.linalg.lstsq(design, zz.ravel(), rcond=None)[0]
    normal = np.array([-slope[0], -slope[1], 1.])
    normal /= np.linalg.norm(normal)
    up = np.array([0., 0., 1.])-normal*normal[2]
    up /= np.linalg.norm(up)
    tangent = np.cross(normal, up)
    rotation = np.column_stack((tangent, normal, up))
    if not np.allclose(rotation.T@rotation, np.eye(3)) or np.linalg.det(rotation) < .999999:
        raise ValueError('Invalid rigid placement frame')
    anchor = np.array([0., 0., slope[2]]) + normal*args.front_offset_m
    base, top, perimeter = closed_shell(source, row, col, width, depth, .5, z_origin, 24., name='Baseline')
    candidate = base.copy()
    candidate.data = base.data.copy()
    candidate.name = 'Fitted exposure'
    bpy.context.collection.objects.link(candidate)
    base.hide_render = True
    base.hide_set(True)

    # Read explicit OBJ coordinates without the importer's forward/up heuristics.
    vertices, triangles = [], []
    for line in args.face.read_text(encoding='utf-8').splitlines():
        if line.startswith('v '):
            vertices.append([float(v) for v in line.split()[1:4]])
        elif line.startswith('f '):
            ids = [int(v.split('/')[0])-1 for v in line.split()[1:]]
            if len(ids) != 3:
                raise ValueError('Expected original triangle OBJ')
            triangles.append(ids)
    points = np.asarray(vertices, dtype=float)
    bounds = np.array([points.min(axis=0), points.max(axis=0)])
    if not np.allclose(bounds, [meta['bounds_m_min'], meta['bounds_m_max']], atol=1.e-5):
        raise ValueError('Actual face bounds differ from source contract')
    transformed = anchor+points@rotation.T
    if np.any(np.abs(transformed[:, 0]) >= width/2-guard) or np.any(np.abs(transformed[:, 1]) >= depth/2-guard):
        raise ValueError('Rigid face leaves guarded crop: do not rescale it to fit')
    rock = mesh_object('Original physical block face', transformed.tolist(), triangles)
    # Elliptic opening tapers its recess to zero at the exposure perimeter.
    # Its rear depth is 2m; it does not cut through the 24m vertical backing.
    bpy.ops.mesh.primitive_uv_sphere_add(segments=64, ring_count=32)
    cavity = bpy.context.object
    cavity.name = 'Finite skin exposure'
    radius_x = .42*(bounds[1, 0]-bounds[0, 0])
    radius_z = .42*(bounds[1, 2]-bounds[0, 2])
    for vertex in cavity.data.vertices:
        p = np.asarray(vertex.co)*np.array([radius_x, 8., radius_z])
        p[1] += 6.
        vertex.co = anchor+rotation@p
    apply_boolean(candidate, cavity, 'DIFFERENCE')
    bpy.data.objects.remove(cavity, do_unlink=True)
    apply_boolean(candidate, rock, 'UNION')
    bpy.data.objects.remove(rock, do_unlink=True)
    clean_boolean_mesh(candidate)
    out_vertices, out_triangles = arrays(candidate)
    metrics = mesh_metrics(out_vertices, out_triangles)
    tree = BVHTree.FromPolygons([Vector(p) for p in out_vertices], out_triangles.tolist(), all_triangles=True)
    boundary_error = max(tree.find_nearest(Vector(p))[3] for p in top[perimeter])
    report = dict(schema='directorscut.fitted_implicit_exposure.v1', source_sha256=sha(args.source),
                  source_path=str(args.source.resolve()), block_sha256=sha(args.face),
                  block_metadata=meta, source_kind='bounded_3d_implicit_block_splice', crop=crop,
                  placement_rotation=rotation.tolist(), placement_anchor=anchor.tolist(),
                  scale=1., fitted_local_plane=slope.tolist(), front_offset_m=args.front_offset_m,
                  cavity='Finite ellipsoid: radii(X,normal,Z), centre6m ahead, rear reaches2m behind fitted plane',
                  cavity_radii_m=[radius_x, 8., radius_z], metrics=metrics,
                  maximum_crop_boundary_error_m=float(boundary_error),
                  arena_integrated=False, visual_accepted=False,
                  limitations='Offline source crop only. No complete sector, material masks, terrain-support or runtime signoff. Boolean topology and visual seam need independent review.')
    args.output.mkdir(parents=True)
    write_npz(args.output/'baseline.npz', base, origin, basis, report)
    write_npz(args.output/'candidate.npz', candidate, origin, basis, report)
    (args.output/'placement.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    bpy.ops.wm.save_as_mainfile(filepath=str((args.output/'placement.blend').resolve()))
    print(json.dumps(dict(metrics=metrics, boundary_error_m=boundary_error)), flush=True)
    if not metrics['finite'] or metrics['nonmanifold_edges'] or metrics['degenerate_triangles'] or boundary_error > .001:
        raise RuntimeError('Retained failed splice: topology/boundary did not pass')


if __name__ == '__main__':
    main()
