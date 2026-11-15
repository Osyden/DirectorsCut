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
from collections import deque

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


def convex_footprint(points):
    """Original cluster XY hull; its edges come from the actual source mass."""
    p = sorted(set(map(tuple, np.asarray(points)[:, :2])))
    if len(p) < 3:
        return np.empty((0, 2))
    def cross(a, b, c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    lower, upper = [], []
    for q in p:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], q) <= 0:
            lower.pop()
        lower.append(q)
    for q in reversed(p):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], q) <= 0:
            upper.pop()
        upper.append(q)
    return np.asarray(lower[:-1]+upper[:-1])


def joint_exposure_field(footprints, half_u=32., half_v=16.):
    """Distance to the exterior UNION boundary, never internal block edges.

    A0.75m inward-halfspace margin seals sub-lattice projection gaps (this is a
    polygon-plane offset, not a Euclidean corner-radius dilation). Flood-filling
    exterior air prevents a hole between projected blocks becoming an artificial
    pillar of unfractured terrain. This changes only the exposure mask, not the
    retained block geometry. Distances are exact Euclidean on this0.25m grid.
    """
    step = .25
    x, y = np.meshgrid(np.arange(-half_u, half_u+step/2, step), np.arange(-half_v, half_v+step/2, step))
    inside = np.zeros(x.shape, dtype=bool)
    for polygon in footprints:
        polygon = np.asarray(polygon, dtype=np.float64)
        edge = np.roll(polygon, -1, axis=0)-polygon
        normal = np.column_stack((-edge[:, 1], edge[:, 0]))
        normal /= np.linalg.norm(normal, axis=1)[:, None]
        # Halfspace offset is tested only in its conservative expanded bbox.
        # Keep physical .25m resolution; do not test every regional grid cell
        # against every small cluster polygon.
        expanded=[]
        limits=(polygon*normal).sum(axis=1)-.75
        for i in range(len(polygon)):
            j=(i+1)%len(polygon)
            matrix=np.stack((normal[i],normal[j]))
            if abs(np.linalg.det(matrix))>1.e-10:
                expanded.append(np.linalg.solve(matrix,limits[[i,j]]))
        bounds=np.asarray(expanded) if expanded else polygon
        low=np.floor((bounds.min(axis=0)+[half_u,half_v])/step).astype(int)-1
        high=np.ceil((bounds.max(axis=0)+[half_u,half_v])/step).astype(int)+2
        c0,c1=max(0,low[0]),min(x.shape[1],high[0]);r0,r1=max(0,low[1]),min(x.shape[0],high[1])
        if c0>=c1 or r0>=r1:continue
        xy=np.column_stack((x[r0:r1,c0:c1].ravel(),y[r0:r1,c0:c1].ravel()))
        distance = ((xy[:, None]-polygon) * normal).sum(axis=2).min(axis=1)
        inside[r0:r1,c0:c1] |= (distance>=-.75).reshape(r1-r0,c1-c0)
    inside[[0, -1], :] = False
    inside[:, [0, -1]] = False
    exterior = np.zeros_like(inside)
    queue = deque((int(r), int(c)) for r, c in zip(*np.where(~inside & ((np.indices(inside.shape)[0] == 0) | (np.indices(inside.shape)[0] == inside.shape[0]-1) | (np.indices(inside.shape)[1] == 0) | (np.indices(inside.shape)[1] == inside.shape[1]-1)))))
    for r, c in queue:
        exterior[r, c] = True
    while queue:
        r, c = queue.popleft()
        for dr, dc in ((-1,0),(1,0),(0,-1),(0,1)):
            nr, nc = r+dr, c+dc
            if 0 <= nr < len(inside) and 0 <= nc < inside.shape[1] and not inside[nr,nc] and not exterior[nr,nc]:
                exterior[nr,nc] = True
                queue.append((nr,nc))
    filled = ~exterior
    def edt_line(f):
        n=len(f);v=np.zeros(n,dtype=np.int32);z=np.zeros(n+1);d=np.zeros(n)
        k=0;z[0]=-1.e30;z[1]=1.e30
        for q in range(1,n):
            s=((f[q]+q*q)-(f[v[k]]+v[k]*v[k]))/(2.*(q-v[k]))
            while s <= z[k]:
                k-=1
                s=((f[q]+q*q)-(f[v[k]]+v[k]*v[k]))/(2.*(q-v[k]))
            k+=1;v[k]=q;z[k]=s;z[k+1]=1.e30
        k=0
        for q in range(n):
            while z[k+1] < q:k+=1
            d[q]=(q-v[k])**2+f[v[k]]
        return d
    distances=np.where(filled,1.e9,0.)
    for r in range(len(distances)):distances[r]=edt_line(distances[r])
    for c in range(distances.shape[1]):distances[:,c]=edt_line(distances[:,c])
    return dict(distance_m=np.sqrt(distances)*step, step_m=step,
                half_extents_m=[half_u,half_v],
                filled_projection_hole_cells=int((filled & ~inside).sum()))


def joint_exposure_weight(xy, field):
    xy = np.asarray(xy, dtype=np.float64).reshape(-1, 2)
    half_u,half_v=field.get('half_extents_m',[32.,16.])
    u = np.clip((xy[:,0]+half_u)/field['step_m'], 0., field['distance_m'].shape[1]-1.)
    v = np.clip((xy[:,1]+half_v)/field['step_m'], 0., field['distance_m'].shape[0]-1.)
    c=np.minimum(np.floor(u).astype(int),field['distance_m'].shape[1]-2)
    r=np.minimum(np.floor(v).astype(int),field['distance_m'].shape[0]-2)
    tx,ty=u-c,v-r;d=field['distance_m']
    clearance=(1-ty)*((1-tx)*d[r,c]+tx*d[r,c+1])+ty*((1-tx)*d[r+1,c]+tx*d[r+1,c+1])
    guard = np.minimum(half_u-np.abs(xy[:, 0]), half_v-np.abs(xy[:, 1]))
    t = np.clip(np.minimum(clearance, guard)/4., 0., 1.)
    return t*t*(3.-2.*t)


def original_mass_shape(cluster):
    centered = cluster-cluster.mean(axis=0)
    _, _, axes = np.linalg.svd(centered, full_matrices=False)
    spans = np.sort(np.ptp(centered@axes.T, axis=0))
    return dict(thickness_m=float(spans[0]), longest_span_m=float(spans[-1]),
                thickness_ratio=float(spans[0]/max(spans[-1], .001)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--points', type=Path, required=True)
    parser.add_argument('--blocks', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--core-depth-m', type=float, default=14.)
    parser.add_argument('--rear-depth-m', type=float, default=20.)
    parser.add_argument('--release-patches', action='store_true', help='Detach shallow clusters only within two connected downhill release regions')
    parser.add_argument('--joint-transition', action='store_true', help='Source-footprint exposure boundary and contact-aware bounded anchors')
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
    descriptor=point_data['metadata']['crop']
    row,col=int(descriptor['row']),int(descriptor['col'])
    width,depth=float(descriptor['width_m']),float(descriptor['depth_m'])
    crop=validate_crop(source,row,col,width,depth,4.)
    point_half_u,point_half_v=point_data['metadata'].get('point_inner_half_extents_m',[width/2.-6.,depth/2.-6.])
    cavity_half_u,cavity_half_v=width/2.-4.,depth/2.-4.
    z_origin = float(source['height_m'][row, col])
    origin = np.r_[source['world_xy_m'][row, col], z_origin]
    basis = np.column_stack((source['basis_u_world'], source['basis_v_world'], source['basis_up_world']))
    frame = point_data['frame']
    rotation = np.asarray(frame['canonical_to_crop_rotation'])
    anchor = np.asarray(frame['canonical_origin_crop_m'])
    points = np.asarray(point_data['points_m'])@rotation.T+anchor
    depths = np.asarray(point_data['depth_below_face_m'])
    footprints, footprint_ids = [], []
    if args.joint_transition:
        for block in block_data['blocks']:
            cluster = points[np.asarray(block['point_indices'], dtype=np.int64)]
            center = cluster.mean(axis=0)
            # Select a connected central exposure, then let whole geological
            # footprints define its actual irregular boundary.
            if ((center[0]+point_half_u/30.)/(point_half_u*23./30.))**2+((center[1]-point_half_v*.5/14.)/(point_half_v*9.5/14.))**2 < 1.:
                polygon = convex_footprint(cluster)
                if len(polygon) >= 3:
                    footprints.append(polygon)
                    footprint_ids.append(int(block['cluster_index']))
        if not footprints:
            raise ValueError('No original fracture footprints selected')
    exposure_field = joint_exposure_field(footprints,cavity_half_u,cavity_half_v) if args.joint_transition else None
    base, top, perimeter = closed_shell(source, row, col, width, depth, .5, z_origin, 24., name='Baseline')
    candidate = base.copy()
    candidate.data = base.data.copy()
    candidate.name = 'Curved fracture exposure'
    bpy.context.collection.objects.link(candidate)
    base.hide_render = True
    base.hide_set(True)
    cavity_width,cavity_depth=(2*cavity_half_u,2*cavity_half_v) if args.joint_transition else (2*point_half_u,2*point_half_v)
    cavity, cavity_top, _ = closed_shell(source, row, col, cavity_width, cavity_depth, .5, z_origin, args.core_depth_m, 1., name='Graded skin opening')
    count = len(cavity_top)
    # Grade the removal depth to zero at the crop interior; original outer
    # guard stays intact. This is an exposure mask, not a terrain noise field.
    joint_weights = joint_exposure_weight(cavity_top[:, :2], exposure_field) if args.joint_transition else None
    for i in range(count):
        x, y, _ = cavity_top[i]
        weight = np.clip(min((point_half_u-abs(x))/5., (point_half_v-abs(y))/4.), 0., 1.)
        weight = joint_weights[i] if args.joint_transition else weight*weight*(3.-2.*weight)
        h = float(sample_height(source, x, y, row, col))-z_origin
        cavity.data.vertices[i+count].co.z = h-args.core_depth_m*weight
    apply_boolean(candidate, cavity, 'DIFFERENCE')
    bpy.data.objects.remove(cavity, do_unlink=True)
    collection = bpy.data.collections.new('Physical fractured bedrock blocks')
    bpy.context.scene.collection.children.link(collection)
    retained, removed, support_records = [], [], []
    for block in block_data['blocks']:
        identity = int(block['cluster_index'])
        ids = np.asarray(block['point_indices'], dtype=np.int64)
        if ids.min() < 0 or ids.max() >= len(points) or len(ids) < 5:
            raise ValueError('Invalid original point IDs')
        depth = depths[ids]
        cluster = points[ids]
        center = cluster.mean(axis=0)
        ru,rv=point_half_u/30.,point_half_v/14.
        release_region = (((center[0]+13.*ru)/(8.*ru))**2+((center[1]+rv)/(13.*rv))**2 < 1.
                          or ((center[0]-11.*ru)/(7.*ru))**2+((center[1]-3.*rv)/(10.*rv))**2 < 1.)
        # Whole shallow fragments detach; no rectangular face is cut out.
        if np.percentile(depth, 90) < 3.5 and (release_region if args.release_patches else identity % 3 != 0):
            removed.append(identity)
            continue
        if args.joint_transition:
            shape = original_mass_shape(cluster)
            weight = joint_exposure_weight(cluster[:, :2], exposure_field)
            core_z = sample_height(source, cluster[:, 0], cluster[:, 1], row, col)-z_origin-args.core_depth_m*weight
            gap = cluster[:, 2]-core_z
            contact = cluster[gap < -.5]
            contact_spread = 0.
            if len(contact) >= 4:
                _, contact_axes = np.linalg.eigh(np.cov(contact[:, :2].T))
                contact_spread = float(np.min(np.ptp((contact[:, :2]-contact[:, :2].mean(axis=0))@contact_axes, axis=0)))
            support = dict(cluster_id=identity, original_shape=shape,
                           core_contact_count=int(len(contact)), contact_spread_m=contact_spread)
            if len(contact) >= 4 and contact_spread >= .4:
                # Already connected original hull: never manufacture new side
                # planes by moving deep source points UP to a fixed12m target.
                hull_points = cluster
                support.update(action='original supported hull', maximum_anchor_extension_m=0.)
            else:
                thin = shape['thickness_m'] < 1. and shape['thickness_ratio'] < .12 and shape['longest_span_m'] > 3.
                chosen = np.argsort(gap)[:4]
                extension = np.maximum(gap[chosen]+.65, 0.)
                if (thin and np.any(gap > .02)) or float(extension.max()) > 2.5:
                    removed.append(identity)
                    support.update(action='detached unsupported thin fragment' if thin else 'detached unsupported long anchor',
                                   avoided_anchor_extension_m=float(extension.max()))
                    support_records.append(support)
                    continue
                back = cluster[chosen].copy()
                back[:, 2] -= extension
                hull_points = np.concatenate((cluster, back))
                support.update(action='short downward local-core anchor', maximum_anchor_extension_m=float(extension.max()))
            support_records.append(support)
        else:
            back = cluster[np.argsort(depth)[-4:]].copy()
            back[:, 2] = sample_height(source, back[:, 0], back[:, 1], row, col)-z_origin-args.rear_depth_m
            hull_points = np.concatenate((cluster, back))
        obj = wedge('PhysicalCluster_'+str(identity), hull_points)
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
                    crop=crop,authoring_surface_spacing_m=.5,
                    exposure_field_half_extents_m=[cavity_half_u,cavity_half_v],
                    kept_cluster_ids=retained, detached_cluster_ids=removed, metrics=metrics,
                    maximum_crop_boundary_error_m=float(boundary_error), physical_rescaling=False,
                    core_depth_m=args.core_depth_m,
                    rear_anchor_vertical_depth_m=None if args.joint_transition else args.rear_depth_m,
                    legacy_rear_depth_argument_unused_m=args.rear_depth_m if args.joint_transition else None,
                    collapsed_numerical_sliver_edge_lengths_m=collapsed_edges,
                    release_mode='two downhill release patches' if args.release_patches else 'scattered shallow fragments',
                    exposure_transition='source cluster footprint exterior union;0.75m halfspace gap margin;0.25m distance grid;4m inward grading;strict4m guard' if args.joint_transition else 'rectangular graded window',
                    filled_projection_hole_cells=exposure_field['filled_projection_hole_cells'] if args.joint_transition else 0,
                    exposure_footprint_cluster_ids=footprint_ids,
                    exposure_footprints_crop_xy=[p.tolist() for p in footprints],
                    original_hull_support_records=support_records,
                    maximum_new_anchor_downward_m=2.5 if args.joint_transition else None,
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
