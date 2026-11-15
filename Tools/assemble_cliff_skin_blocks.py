"""Expose source-informed fracture clusters in a retained bedrock core.

Blender CPU authoring proof. The clusters come from physical points inside the
curved source cliff, not rectangular rock stamps. Selected shallow clusters are
removed entirely; remaining original hull faces form the exposure. Only hidden
rear anchors extend to the retained core. No smooth-front clipping is applied.
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
    sample_height, validate_crop, wedge, write_npz,
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def collapse_submillimetre_slivers(obj):
    """Repair only numerically tiny boolean triangles; never lower the gate."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    collapsed = []
    for _ in range(16):
        bad = []
        for face in bm.faces:
            if len(face.verts) != 3:
                continue
            p = np.array([tuple(vertex.co) for vertex in face.verts], dtype=np.float64)
            area = .5*np.linalg.norm(np.cross(p[1]-p[0], p[2]-p[0]))
            if area < 1.e-10:
                edge = min(face.edges, key=lambda e: e.calc_length())
                if edge.calc_length() <= .001:
                    bad.append(edge)
        if not bad:
            break
        edge = min(bad, key=lambda e: e.calc_length())
        collapsed.append(float(edge.calc_length()))
        bmesh.ops.collapse(bm, edges=[edge])
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    bm.normal_update()
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return collapsed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--points', type=Path, required=True)
    parser.add_argument('--blocks', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--core-depth-m', type=float, default=14.)
    parser.add_argument('--rear-depth-m', type=float, default=20.)
    parser.add_argument('--release-patches', action='store_true', help='Detach shallow clusters only within two connected downhill release regions')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Choose a new ignored Saved output directory')
    if not 4. <= args.core_depth_m < args.rear_depth_m <= 21.:
        parser.error('Require 4m <= core depth < rear anchor depth <=21m')
    point_data = json.loads(args.points.read_text(encoding='utf-8'))
    block_data = json.loads(args.blocks.read_text(encoding='utf-8'))
    if block_data['source_points_sha256'] != sha(args.points):
        raise ValueError('Cluster membership refers to another point cloud')
    source_path = Path(point_data['metadata']['source_path'])
    if sha(source_path) != point_data['source_mesh_sha256']:
        raise ValueError('Original source changed')
    with np.load(source_path, allow_pickle=False) as archive:
        source = {k: archive[k].copy() for k in archive.files}
    import bpy
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    row, col = 89, 64
    validate_crop(source, row, col, 72., 40., 4.)
    z_origin = float(source['height_m'][row, col])
    origin = np.r_[source['world_xy_m'][row, col], z_origin]
    basis = np.column_stack((source['basis_u_world'], source['basis_v_world'], source['basis_up_world']))
    frame = point_data['frame']
    rotation = np.asarray(frame['canonical_to_crop_rotation'])
    anchor = np.asarray(frame['canonical_origin_crop_m'])
    points = np.asarray(point_data['points_m'])@rotation.T+anchor
    depths = np.asarray(point_data['depth_below_face_m'])
    base, top, perimeter = closed_shell(source, row, col, 72., 40., .5, z_origin, 24., name='Baseline')
    candidate = base.copy()
    candidate.data = base.data.copy()
    candidate.name = 'Curved fracture exposure'
    bpy.context.collection.objects.link(candidate)
    base.hide_render = True
    base.hide_set(True)
    cavity, cavity_top, _ = closed_shell(source, row, col, 60., 28., .5, z_origin, args.core_depth_m, 1., name='Graded skin opening')
    count = len(cavity_top)
    # Grade the removal depth to zero at the crop interior; original outer
    # guard stays intact. This is an exposure mask, not a terrain noise field.
    for i in range(count):
        x, y, _ = cavity_top[i]
        weight = np.clip(min((30.-abs(x))/5., (14.-abs(y))/4.), 0., 1.)
        weight = weight*weight*(3.-2.*weight)
        h = float(sample_height(source, x, y, row, col))-z_origin
        cavity.data.vertices[i+count].co.z = h-args.core_depth_m*weight
    apply_boolean(candidate, cavity, 'DIFFERENCE')
    bpy.data.objects.remove(cavity, do_unlink=True)
    collection = bpy.data.collections.new('Physical fractured bedrock blocks')
    bpy.context.scene.collection.children.link(collection)
    retained, removed = [], []
    for block in block_data['blocks']:
        identity = int(block['cluster_index'])
        ids = np.asarray(block['point_indices'], dtype=np.int64)
        if ids.min() < 0 or ids.max() >= len(points) or len(ids) < 5:
            raise ValueError('Invalid original point IDs')
        depth = depths[ids]
        cluster = points[ids]
        center = cluster.mean(axis=0)
        release_region = (((center[0]+13.)/8.)**2+((center[1]+1.)/13.)**2 < 1.
                          or ((center[0]-11.)/7.)**2+((center[1]-3.)/10.)**2 < 1.)
        # Whole shallow fragments detach; no rectangular face is cut out.
        if np.percentile(depth, 90) < 3.5 and (release_region if args.release_patches else identity % 3 != 0):
            removed.append(identity)
            continue
        # Four original deepest members anchor each hull inside the old core.
        # Their explicit depth leaves at least3m of original rear bedrock.
        back = cluster[np.argsort(depth)[-4:]].copy()
        back[:, 2] = sample_height(source, back[:, 0], back[:, 1], row, col)-z_origin-args.rear_depth_m
        obj = wedge('PhysicalCluster_'+str(identity), np.concatenate((cluster, back)))
        for parent in list(obj.users_collection):
            parent.objects.unlink(obj)
        collection.objects.link(obj)
        retained.append(identity)
    print(f'Union {len(retained)} source-defined hulls; removed {len(removed)} shallow clusters', flush=True)
    bpy.context.view_layer.objects.active = candidate
    candidate.select_set(True)
    modifier = candidate.modifiers.new('Union physical blocks with retained core', 'BOOLEAN')
    modifier.operation = 'UNION'
    modifier.solver = 'EXACT'
    modifier.operand_type = 'COLLECTION'
    modifier.collection = collection
    modifier.use_self = True
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    candidate.select_set(False)
    for obj in list(collection.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.collections.remove(collection)
    clean_boolean_mesh(candidate)
    collapsed_edges = collapse_submillimetre_slivers(candidate)
    vertices, triangles = arrays(candidate)
    metrics = mesh_metrics(vertices, triangles)
    tree = BVHTree.FromPolygons([Vector(p) for p in vertices], triangles.tolist(), all_triangles=True)
    boundary_error = max(tree.find_nearest(Vector(p))[3] for p in top[perimeter])
    metadata = dict(schema='directorscut.curved_cliff_blocks.v1', source_path=str(source_path),
                    source_sha256=sha(source_path), points_path=str(args.points.resolve()),
                    points_sha256=sha(args.points), blocks_path=str(args.blocks.resolve()),
                    blocks_sha256=sha(args.blocks), source_kind='physical_curved_skin_fracture_exposure',
                    kept_cluster_ids=retained, detached_cluster_ids=removed, metrics=metrics,
                    maximum_crop_boundary_error_m=float(boundary_error), physical_rescaling=False,
                    core_depth_m=args.core_depth_m, rear_anchor_vertical_depth_m=args.rear_depth_m,
                    collapsed_numerical_sliver_edge_lengths_m=collapsed_edges,
                    release_mode='two downhill release patches' if args.release_patches else 'scattered shallow fragments',
                    original_backing_depth_m=24.,
                    masks_transferred=False, arena_integrated=False, visual_accepted=False,
                    limitations='Original inner-crop cluster hulls, not a calibrated geology simulation. Depth estimates select removal only. Exact supports, contact, shading masks and full-sector joins remain unintegrated.')
    args.output.mkdir(parents=True)
    write_npz(args.output/'baseline.npz', base, origin, basis, metadata)
    write_npz(args.output/'candidate.npz', candidate, origin, basis, metadata)
    (args.output/'assembly.json').write_text(json.dumps(metadata, indent=2)+'\n', encoding='utf-8')
    bpy.ops.wm.save_as_mainfile(filepath=str((args.output/'assembly.blend').resolve()))
    print(json.dumps(dict(metrics=metrics, boundary_error_m=boundary_error)), flush=True)
    if not metrics['finite'] or metrics['nonmanifold_edges'] or metrics['degenerate_triangles'] or boundary_error > .001:
        raise RuntimeError('Retained failed candidate; geometry gate failed')


if __name__ == '__main__':
    main()
