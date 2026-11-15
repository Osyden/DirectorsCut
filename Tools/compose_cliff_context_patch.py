"""Join one original3D outcrop to the unchanged full massif for clay context.

No terrain regeneration/refinement: remove the exact source-grid crop, split
only outside triangles sharing the finer patch boundary, and weld by measured
3D correspondence. Outside original positions/masks stay exact. New patch
interior has diagnostic zero RGB, not invented erosion/flow evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_weathered_cliff_mesh import original_mesh, normalized_masks
from generate_fractured_cliff_outcrop import validate_crop


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def edge_table(faces):
    directed = np.concatenate((faces[:, [0,1]], faces[:, [1,2]], faces[:, [2,0]]))
    edges, inverse, counts = np.unique(np.sort(directed, axis=1), axis=0, return_inverse=True, return_counts=True)
    orientation = np.bincount(inverse, weights=np.where(directed[:,0] < directed[:,1], 1, -1))
    return edges, counts, orientation


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--patch', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a new ignored Saved output directory')
    with np.load(args.source, allow_pickle=False) as z:
        source = {k:z[k].copy() for k in z.files}
    with np.load(args.patch, allow_pickle=False) as z:
        patch = {k:z[k].copy() for k in z.files}
    patch_meta = json.loads(str(patch['metadata_json']))
    if patch_meta['source_sha256'] != sha(args.source):
        raise ValueError('Patch belongs to a different immutable massif')
    crop = patch_meta.get('crop', dict(row=89,col=64,width_m=72.,depth_m=40.,edge_guard_m=4.))
    row,col = int(crop['row']),int(crop['col'])
    width,depth = float(crop['width_m']),float(crop['depth_m'])
    spacing = float(patch_meta.get('authoring_surface_spacing_m', .5))
    cell = float(source['cell_size_m'])
    if not np.isfinite([width,depth,spacing,cell]).all() or min(width,depth,spacing,cell)<=0:
        raise ValueError('Crop and spacing must be finite physical distances')
    if any(abs(value-round(value))>1.e-8 for value in (width/(2*cell),depth/(2*cell),cell/spacing)):
        raise ValueError('Crop must align with source edges and subdivide source cells exactly')
    validate_crop(source,row,col,width,depth,float(crop.get('edge_guard_m',4.)))
    half_u,half_v = width/2.,depth/2.
    expected_boundary = int(round(2*(width+depth)/spacing))
    expected_removed = int(round(2*width*depth/(cell*cell)))
    expected_seam = int(round(2*(width+depth)/cell))
    vertices, triangles, grid_triangles, grid_world = original_mesh(source)
    grid_used = np.unique(grid_triangles)
    mask_grid, normalization = normalized_masks(source)
    rgba = mask_grid.reshape(-1,4)[grid_used].astype(np.float32)
    patch_vertices = patch['vertices_m'].astype(np.float64)
    patch_triangles = patch['triangles'].astype(np.int64)
    origin = patch['frame_origin_world_m']
    basis = np.column_stack((patch['basis_u_world'],patch['basis_v_world'],patch['basis_z_world']))
    expected_origin = np.r_[source['world_xy_m'][row,col],source['height_m'][row,col]]
    expected_basis = np.column_stack((source['basis_u_world'],source['basis_v_world'],source['basis_up_world']))
    if not np.allclose(origin,expected_origin,atol=1.e-8,rtol=0) or not np.array_equal(basis,expected_basis):
        raise ValueError('Patch frame does not match its declared original-source crop')
    local = (vertices-origin)@basis
    patch_local = (patch_vertices-origin)@basis
    tolerance = 1.e-5
    pe, pc, po = edge_table(patch_triangles)
    if np.any(pc > 2) or np.any((pc == 2)&(po != 0)):
        raise ValueError('Patch is not consistently wound edge-manifold')
    patch_boundary = np.unique(pe[pc == 1])
    if len(patch_boundary) != expected_boundary:
        raise ValueError(f'Expected{expected_boundary} vertices on the declared physical crop boundary')
    on_crop = (np.isclose(np.abs(patch_local[:,0]),half_u,atol=tolerance,rtol=0)
               | np.isclose(np.abs(patch_local[:,1]),half_v,atol=tolerance,rtol=0))
    if not on_crop[patch_boundary].all():
        raise ValueError('Patch has an interior hole or an unexpected boundary')
    xyz = local[triangles]
    inside = ((xyz[:,:,0] >= -half_u-1.e-8)&(xyz[:,:,0] <=half_u+1.e-8)
              &(xyz[:,:,1] >= -half_v-1.e-8)&(xyz[:,:,1] <=half_v+1.e-8)).all(axis=1)
    if int(inside.sum()) != expected_removed:
        raise ValueError(f'Expected{expected_removed} exact source crop triangles, got{inside.sum()}')
    outside_ids = np.flatnonzero(~inside)
    outside = triangles[outside_ids]
    seam = {}
    for face_id, face in zip(outside_ids, outside):
        for k in range(3):
            a,b=int(face[k]),int(face[(k+1)%3])
            pa,pb=local[a],local[b]
            for axis, half, other in ((0,half_u,1),(1,half_v,0)):
                if (abs(abs(pa[axis])-half)<1.e-8 and abs(pb[axis]-pa[axis])<1.e-8
                    and max(abs(pa[other]),abs(pb[other])) <= (half_v if other==1 else half_u)+1.e-8):
                    seam[tuple(sorted((a,b)))] = dict(face=int(face_id),points=[])
                    break
    if len(seam) !=expected_seam:
        raise ValueError(f'Expected{expected_seam} source seam edges, got{len(seam)}')
    edge_ids=np.asarray(list(seam),dtype=np.int64)
    a3=vertices[edge_ids[:,0]]; delta=vertices[edge_ids[:,1]]-a3
    length2=np.einsum('ij,ij->i',delta,delta)
    out_vertices=vertices.tolist();out_rgba=rgba.tolist()
    source_vertex=list(range(len(vertices)));patch_vertex=[-1]*len(vertices)
    patch_map=np.full(len(patch_vertices),-1,dtype=np.int64)
    max_snap=0.; seam_added=0
    for pv in patch_boundary:
        p=patch_vertices[pv]
        t=np.clip(np.einsum('ij,ij->i',p-a3,delta)/length2,0.,1.)
        matches=a3+t[:,None]*delta
        error=np.linalg.norm(matches-p,axis=1)
        ei=int(np.argmin(error));value=float(error[ei]);max_snap=max(max_snap,value)
        if value>tolerance:
            raise ValueError(f'Patch boundary point{pv} misses true source edge by{value}m')
        aa,bb=map(int,edge_ids[ei]);fraction=float(t[ei])
        if np.linalg.norm(matches[ei]-vertices[aa])<=tolerance:
            index=aa
        elif np.linalg.norm(matches[ei]-vertices[bb])<=tolerance:
            index=bb
        else:
            index=len(out_vertices);out_vertices.append(matches[ei].tolist())
            out_rgba.append(((1-fraction)*rgba[aa]+fraction*rgba[bb]).tolist())
            source_vertex.append(-1);patch_vertex.append(int(pv));seam_added+=1
            seam[(aa,bb)]['points'].append((fraction,index))
        patch_map[pv]=index;patch_vertex[index]=int(pv)
    if len(np.unique(patch_map[patch_boundary])) != len(patch_boundary):
        raise ValueError('Patch boundary collapses or duplicates source correspondence')
    for pv in np.flatnonzero(patch_map<0):
        patch_map[pv]=len(out_vertices);out_vertices.append(patch_vertices[pv].tolist())
        out_rgba.append([0.,0.,0.,1.]);source_vertex.append(-1);patch_vertex.append(int(pv))
    out_faces=[];face_source=[];split_faces=0
    for face_id,face in zip(outside_ids,outside):
        split=[]
        for k in range(3):
            aa,bb=int(face[k]),int(face[(k+1)%3]);key=tuple(sorted((aa,bb)))
            if key in seam and seam[key]['points']:
                split.append((k,key))
        if len(split)>1:
            raise ValueError('Unexpected multi-edge seam triangle; do not create a T junction')
        if not split:
            out_faces.append(face.tolist());face_source.append(int(face_id));continue
        k,key=split[0];aa,bb,cc=int(face[k]),int(face[(k+1)%3]),int(face[(k+2)%3])
        mids=[index for fraction,index in sorted(seam[key]['points'],reverse=aa>bb)]
        chain=[aa]+mids+[bb]
        for p,q in zip(chain[:-1],chain[1:]):
            out_faces.append([p,q,cc]);face_source.append(int(face_id))
        split_faces+=1
    outside_face_count=len(out_faces)
    out_faces.extend(patch_map[patch_triangles].tolist());face_source.extend([-1]*len(patch_triangles))
    out_vertices=np.asarray(out_vertices);out_faces=np.asarray(out_faces,dtype=np.int64)
    out_rgba=np.asarray(out_rgba,dtype=np.float32)
    source_vertex=np.asarray(source_vertex,dtype=np.int64);patch_vertex=np.asarray(patch_vertex,dtype=np.int64)
    used=np.unique(out_faces);remap=np.full(len(out_vertices),-1,dtype=np.int64);remap[used]=np.arange(len(used))
    out_vertices=out_vertices[used];out_faces=remap[out_faces];out_rgba=out_rgba[used]
    source_vertex=source_vertex[used];patch_vertex=patch_vertex[used]
    edges,counts,orientation=edge_table(out_faces)
    if np.any(counts>2) or np.any((counts==2)&(orientation!=0)):
        raise ValueError('Joined context is nonmanifold or has inconsistent winding')
    seam_result=remap[patch_map[patch_boundary]]
    actual_boundary_snap=float(np.linalg.norm(out_vertices[seam_result]-patch_vertices[patch_boundary],axis=1).max())
    if actual_boundary_snap > tolerance:
        raise ValueError('Final endpoint welding exceeds the declared3D join tolerance')
    if np.isin(edges[counts==1],seam_result).any():
        raise ValueError('Unjoined crop seam/T junction remains')
    old_edges,old_counts,_=edge_table(triangles)
    if (counts==1).sum() != (old_counts==1).sum():
        raise ValueError('Full-sector external boundary changed')
    xyz=out_vertices[out_faces]
    twice_area=np.linalg.norm(np.cross(xyz[:,1]-xyz[:,0],xyz[:,2]-xyz[:,0]),axis=1)
    if (twice_area<=1.e-10).any():
        raise ValueError('Joined context has a degenerate triangle')
    existing=source_vertex>=0
    if not np.array_equal(out_vertices[existing],vertices[source_vertex[existing]]) or not np.array_equal(out_rgba[existing],rgba[source_vertex[existing]]):
        raise ValueError('Outside original positions or masks changed')
    report=dict(schema='directorscut.single_outcrop_context.v1',source_path=str(args.source.resolve()),source_sha256=sha(args.source),
                patch_path=str(args.patch.resolve()),patch_sha256=sha(args.patch),source_kind='single_original3d_outcrop_in_unchanged_massif',
                crop=dict(row=row,col=col,width_m=width,depth_m=depth),removed_source_triangles=int(inside.sum()),
                adjacent_outside_triangles_split=split_faces,new_interpolated_seam_vertices=seam_added,
                maximum_boundary_snap_m=actual_boundary_snap,maximum_edge_projection_residual_m=max_snap,
                join_tolerance_m=tolerance,vertices=len(out_vertices),triangles=len(out_faces),
                external_boundary_edges=int((counts==1).sum()),nonmanifold_edges=0,inconsistent_winding_edges=0,degenerate_triangles=0,
                outside_original_positions_bitwise_preserved=True,outside_original_masks_bitwise_preserved=True,
                masks='Actual normalized source masks outside; source-edge interpolation on seam; diagnostic RGB0/alpha1 inside new3Dpatch, not invented flow',
                color_normalization=normalization,diagnostic_clay_only=True,full_sector_geology_regenerated=False,arena_integrated=False,visual_accepted=False)
    frame=dict(frame_origin_world_m=source['frame_origin_world_m'],basis_u_world=source['basis_u_world'],
               basis_v_world=source['basis_v_world'],basis_z_world=source['basis_up_world'])
    args.output.mkdir(parents=True)
    np.savez_compressed(args.output/'baseline.npz',vertices_m=vertices,triangles=triangles,vertex_rgba=rgba,**frame,
                        metadata_json=np.asarray(json.dumps(dict(report,variant='unchanged original massif'))))
    np.savez_compressed(args.output/'context.npz',vertices_m=out_vertices,triangles=out_faces,vertex_rgba=out_rgba,
                        source_vertex_index=source_vertex,patch_vertex_index=patch_vertex,source_face_index=np.asarray(face_source,dtype=np.int64),
                        seam_vertex_indices=seam_result,**frame,metadata_json=np.asarray(json.dumps(report)))
    (args.output/'context.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
