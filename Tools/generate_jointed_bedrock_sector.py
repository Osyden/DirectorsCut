"""One connected source bedrock solid with sparse shared-cell releases.

Offline Blender CPU proof. An anisotropic, oblique Voronoi partition defines
shared internal interfaces; most stay closed. Only selected surface-crossing
cells detach. There are no separately shrunken hulls, lattice-width cracks,
central exposure ellipse, imported noise or projected replacement surface.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import sys
import time
import numpy as np

sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
from generate_weathered_cliff_mesh import original_mesh
from generate_fractured_cliff_outcrop import mesh_object,wedge,arrays,mesh_metrics,sample_height,clean_boolean_mesh,apply_boolean
from assemble_cliff_skin_blocks import collapse_submillimetre_slivers


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cell_vertices(sites,index):
    """Exact halfspace vertices, then reject any omitted competing site."""
    center=sites[index]
    neighbours=np.argsort(np.sum((sites-center)**2,axis=1))[1:29]
    normals=sites[neighbours]-center
    limits=(np.sum(sites[neighbours]**2,axis=1)-center@center)*.5
    triples=np.asarray(list(itertools.combinations(range(len(neighbours)),3)))
    matrices=normals[triples];det=np.linalg.det(matrices)
    good=np.abs(det)>1.e-9
    vertices=np.linalg.solve(matrices[good],limits[triples[good]][...,None])[...,0]
    vertices=vertices[np.all(vertices@normals.T<=limits+1.e-8,axis=1)]
    if len(vertices)<4:return None
    vertices=np.unique(np.round(vertices,10),axis=0)
    own=np.sum((vertices-center)**2,axis=1)
    all_dist=np.sum((vertices[:,None]-sites[None])**2,axis=2)
    if np.any(all_dist.min(axis=1)<own-1.e-7):return None
    return vertices


def source_sample(source,xy):
    return sample_height(source,xy[:,0],xy[:,1],0,0)


def cancel_opposite_face_pairs(obj):
    """Cancel exact zero-thickness Boolean flaps, never area-based real faces."""
    import bmesh
    _,faces=arrays(obj)
    _,inverse,counts=np.unique(np.sort(faces,axis=1),axis=0,return_inverse=True,return_counts=True)
    pairs=[];polygons=[]
    for group in np.where(counts==2)[0]:
        ids=np.flatnonzero(inverse==group);a,b=faces[ids]
        if any(np.array_equal(np.roll(a[::-1],i),b) for i in range(3)):
            pairs.append(dict(loop_triangle_indices=ids.tolist(),vertices=a.tolist(),opposite_vertices=b.tolist()))
            polygons.extend(obj.data.loop_triangles[int(i)].polygon_index for i in ids)
    if polygons:
        bm=bmesh.new();bm.from_mesh(obj.data);bm.faces.ensure_lookup_table()
        bmesh.ops.delete(bm,geom=[bm.faces[i] for i in sorted(set(polygons))],context='FACES')
        orphan=[v for v in bm.verts if not v.link_faces]
        if orphan:bmesh.ops.delete(bm,geom=orphan,context='VERTS')
        bm.normal_update();bm.to_mesh(obj.data);bm.free();obj.data.update()
    return pairs


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--max-releases',type=int,default=26)
    parser.add_argument('--release-mode',choices=('gully','shoulder'),default='gully')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Choose a new ignored Saved directory')
    if not 1<=args.max_releases<=40:parser.error('Bound this proof to at most40 releases')
    start=time.perf_counter()
    with np.load(args.source,allow_pickle=False) as a:source={k:a[k].copy() for k in a.files}
    world,faces,grid_faces,grid_world=original_mesh(source)
    origin=source['frame_origin_world_m'];basis=np.column_stack((source['basis_u_world'],source['basis_v_world'],source['basis_up_world']))
    if origin[2]!=0. or not np.array_equal(source['basis_up_world'],np.array([0.,0.,1.])):
        raise ValueError('This bounded proof expects the measured arena datumZ=0 and originalZ-up frame')
    local=(world-origin)@basis
    # Original chart coordinates are exact multiples of the measured2m grid.
    local[:,:2]=np.rint(local[:,:2]/float(source['cell_size_m']))*float(source['cell_size_m'])
    h=source['height_m'];cell=float(source['cell_size_m'])
    protected=source['protected_mask'].reshape(-1)>=1
    frozen_triangles=protected[grid_faces].any(axis=1)
    frozen_grid=np.unique(grid_faces[frozen_triangles]);guard=np.zeros(h.size,bool);guard[frozen_grid]=True;guard=guard.reshape(h.shape)
    gy,gx=np.gradient(h.astype(float),cell);slope=np.hypot(gx,gy)
    lap=np.gradient(gx,cell,axis=1)+np.gradient(gy,cell,axis=0)
    # Fixed shared structural metric, with irregular sites rather than repeated
    # parallelepiped pockets. Physical dimensions are not sample-cloud spacing.
    strike=np.array([1.,.16,.08]);strike/=np.linalg.norm(strike)
    dip=np.array([-.12,.77,.63]);dip-=strike*(dip@strike);dip/=np.linalg.norm(dip)
    rotation=np.column_stack((strike,dip,np.cross(strike,dip)))
    scales=np.array([38.,27.,24.])
    q=local@rotation/scales
    lo=np.floor(q.min(axis=0))-2;hi=np.ceil(q.max(axis=0))+2
    grid=np.meshgrid(*(np.arange(a,b+1) for a,b in zip(lo,hi)),indexing='ij')
    sites=np.stack(grid,axis=-1).reshape(-1,3)
    sites=sites+np.random.default_rng(20261115).uniform(-.29,.29,sites.shape)
    centers=(sites*scales)@rotation.T
    rr=np.floor(centers[:,1]/cell).astype(int);cc=np.floor(centers[:,0]/cell).astype(int)
    inside=(rr>=1)&(cc>=1)&(rr<h.shape[0]-2)&(cc<h.shape[1]-2)
    ids=np.where(inside)[0];r,c=rr[ids],cc[ids]
    actual=source_sample(source,centers[ids,:2]);vertical=actual-centers[ids,2]
    approximate_depth=vertical/np.sqrt(1+slope[r,c]**2)
    pick=(source['heightfield_valid_mask'][r,c]>0)&(~guard[r,c])&(slope[r,c]>.65)&(approximate_depth>-7)&(approximate_depth<5)
    candidates=ids[pick]
    # Prefer existing incised gullies and side faces, with unequal global
    # ordering. The whole safe escarpment is eligible, not a central ellipse.
    score=(1. if args.release_mode=='gully' else -1.)*lap[rr[candidates],cc[candidates]]+np.random.default_rng(71421).uniform(-.06,.06,len(candidates))
    candidates=candidates[np.argsort(score)[::-1]]
    releases=[];reasons={}
    for identity in candidates:
        poly=cell_vertices(sites,int(identity))
        if poly is None:reasons['unbounded_or_incomplete_cell']=reasons.get('unbounded_or_incomplete_cell',0)+1;continue
        p=(poly*scales)@rotation.T
        minimum=np.floor(p[:,:2].min(axis=0)/cell).astype(int)-4
        maximum=np.ceil(p[:,:2].max(axis=0)/cell).astype(int)+4
        c0,r0=minimum;c1,r1=maximum
        if r0<1 or c0<1 or r1>=h.shape[0]-1 or c1>=h.shape[1]-1:
            reasons['domain_edge']=reasons.get('domain_edge',0)+1;continue
        box=np.s_[r0:r1+1,c0:c1+1]
        if guard[box].any() or not np.all(source['heightfield_valid_mask'][box]>0):
            reasons['protected_triangle_collar']=reasons.get('protected_triangle_collar',0)+1;continue
        heights=source_sample(source,p[:,:2]);d=heights-p[:,2]
        pr=np.rint(p[:,1]/cell).astype(int);pc=np.rint(p[:,0]/cell).astype(int)
        normal_depth=d/np.sqrt(1+slope[pr,pc]**2)
        if not (d.min()<-.5 and d.max()>.5) or normal_depth.max()>23. or d.max()>120.:
            reasons['not_bounded_surface_release']=reasons.get('not_bounded_surface_release',0)+1;continue
        opening_angle=0.
        if args.release_mode=='shoulder':
            outside=d<-.5
            if outside.sum()<3:
                reasons['not_open_corner']=reasons.get('not_open_corner',0)+1;continue
            normals=np.c_[-gx[pr[outside],pc[outside]],-gy[pr[outside],pc[outside]],np.ones(outside.sum())]
            normals/=np.linalg.norm(normals,axis=1)[:,None]
            opening_angle=float(np.degrees(np.arccos(np.clip((normals@normals.T).min(),-1.,1.))))
            offsets=p[outside,:2]-centers[identity,:2]
            offsets/=np.maximum(np.linalg.norm(offsets,axis=1)[:,None],1.e-9)
            radial_angle=float(np.degrees(np.arccos(np.clip((offsets@offsets.T).min(),-1.,1.))))
            if opening_angle<35. or radial_angle<100.:
                reasons['not_open_corner']=reasons.get('not_open_corner',0)+1;continue
        # A surface-crossing convex cell cannot be a wholly internal void.
        releases.append(dict(site_index=int(identity),points_local_m=p.tolist(),
            bbox_source_grid_with_collar=[int(r0),int(r1),int(c0),int(c1)],
            vertex_normal_depth_range_m=[float(normal_depth.min()),float(normal_depth.max())],
            outward_source_normal_spread_degrees=opening_angle,
            vertex_vertical_depth_range_m=[float(d.min()),float(d.max())]))
        if len(releases)>=args.max_releases:break
    print(json.dumps(dict(candidate_sites=len(candidates),selected_releases=len(releases),rejections=reasons)),flush=True)
    if len(releases)<3:raise RuntimeError('Too few safe surface cells for meaningful whole-face proof')
    import bpy
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    n=len(local);back=local.copy();back[:,2]-=160.
    directed=np.concatenate((faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]]))
    undirected=np.sort(directed,axis=1);_,inv,counts=np.unique(undirected,axis=0,return_inverse=True,return_counts=True)
    boundary=directed[counts[inv]==1]
    shell_faces=np.concatenate((faces,faces[:,::-1]+n,np.array([(a,a+n,b+n) for a,b in boundary]),np.array([(a,b+n,b) for a,b in boundary])))
    base=mesh_object('Original continuous bedrock',np.concatenate((local,back)).tolist(),shell_faces.tolist())
    # Closure tags survive exact booleans; cuts cannot reach the160m underside.
    mats=[bpy.data.materials.new(x) for x in ('Source','AuthoringBack','AuthoringSide','Fracture')]
    for mat in mats:base.data.materials.append(mat)
    for i,p in enumerate(base.data.polygons):p.material_index=0 if i<len(faces) else(1 if i<2*len(faces) else 2)
    candidate=base.copy();candidate.data=base.data.copy();bpy.context.collection.objects.link(candidate)
    base.hide_render=True;base.hide_set(True)
    cutters=bpy.data.collections.new('Selected shared-cell releases');bpy.context.scene.collection.children.link(cutters)
    for release in releases:
        obj=wedge('Release_'+str(release['site_index']),np.asarray(release['points_local_m']))
        for mat in mats:obj.data.materials.append(mat)
        for p in obj.data.polygons:p.material_index=3
        for parent in list(obj.users_collection):parent.objects.unlink(obj)
        cutters.objects.link(obj)
    if args.release_mode=='shoulder':
        for i,obj in enumerate(list(cutters.objects)):
            apply_boolean(candidate,obj,'DIFFERENCE')
            print(f'Applied corner release {i+1}/{len(releases)}',flush=True)
    else:
        bpy.context.view_layer.objects.active=candidate;candidate.select_set(True)
        modifier=candidate.modifiers.new('Finite structural releases','BOOLEAN');modifier.operation='DIFFERENCE';modifier.solver='EXACT'
        modifier.operand_type='COLLECTION';modifier.collection=cutters;modifier.use_self=True
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    clean_boolean_mesh(candidate)
    numerical_collapses=collapse_submillimetre_slivers(candidate)
    opposite_pairs=cancel_opposite_face_pairs(candidate)
    local_result,all_triangles=arrays(candidate)
    tags=np.array([candidate.data.polygons[t.polygon_index].material_index for t in candidate.data.loop_triangles])
    exposed=all_triangles[(tags!=1)&(tags!=2)]
    used=np.unique(exposed);mapping=np.full(len(local_result),-1,int);mapping[used]=np.arange(len(used))
    exposed=mapping[exposed];p=local_result[used]
    # Snap only vertices already coincident with ORIGINAL source samples;
    # new intersections and overhang vertices retain all three coordinates.
    r=np.rint(p[:,1]/cell).astype(int);c=np.rint(p[:,0]/cell).astype(int)
    valid=(r>=0)&(c>=0)&(r<h.shape[0])&(c<h.shape[1])
    # Intersections cannot occur outside all cutter XY bounds. Restrict source
    # restoration there; never snap a near-coincident new Boolean vertex.
    for release in releases:
        bounds=np.asarray(release['points_local_m'])[:,:2]
        low=bounds.min(axis=0)-1.e-4;high=bounds.max(axis=0)+1.e-4
        valid&=~np.all((p[:,:2]>=low)&(p[:,:2]<=high),axis=1)
    snap=np.zeros(len(p),bool)
    k=np.where(valid)[0];reference=np.c_[c[k]*cell,r[k]*cell,h[r[k],c[k]]-origin[2]]
    match=np.max(np.abs(p[k]-reference),axis=1)<3.e-5;snap[k[match]]=True;p[k[match]]=reference[match]
    result_world=origin+p@basis.T
    lookup=np.flatnonzero(snap);result_world[lookup]=grid_world[r[lookup]*h.shape[1]+c[lookup]]
    meta=dict(schema='directorscut.jointed_continuous_bedrock.v1',source_kind='original_jointed_bedrock_surface',
        source_path=str(args.source.resolve()),source_sha256=sha(args.source),generator_sha256=sha(__file__),
        algorithm='Anisotropic oblique shared Voronoi cells; sparse exact surface-crossing releases from one continuous original solid',
        physical_partition_scales_m=scales.tolist(),structural_rotation_columns=rotation.tolist(),
        site_jitter_fraction=.29,seed=20261115,total_sites=len(sites),selected_releases=releases,
        release_mode=args.release_mode,boolean_mode='sequential exact' if args.release_mode=='shoulder' else 'collection exact',
        rejected_cells=reasons,frozen_source_triangle_count=int(frozen_triangles.sum()),
        conservative_cell_bbox_collar_grid_cells=4,source_cell_size_m=cell,
        original_authoring_backing_m=160.,closure_tags_removed=[1,2],snapped_original_samples=int(snap.sum()),
        collapsed_numerical_sliver_edges_m=numerical_collapses,
        cancelled_exact_opposite_face_pairs=opposite_pairs,
        elapsed_seconds=time.perf_counter()-start,masks_transferred=False,arena_integrated=False,visual_accepted=False,
        limitations='Offline shared-cell release experiment, not calibrated fracture/erosion. Heights of uncut source remain original. New3D surfaces have no scientific transport masks. No final visual acceptance.')
    metrics=mesh_metrics(p,exposed)
    meta['exposed_metrics']=metrics
    meta['basic_geometry_gate_passed']=bool(metrics['finite'] and metrics['degenerate_triangles']==0
        and metrics['boundary_edges']==len(boundary) and metrics['nonmanifold_edges']==len(boundary))
    args.output.mkdir(parents=True)
    frame=dict(frame_origin_world_m=origin,basis_u_world=basis[:,0],basis_v_world=basis[:,1],basis_z_world=basis[:,2])
    for name,verts,tris in [('baseline',world,faces),('candidate',result_world,exposed)]:
        np.savez_compressed(args.output/(name+'.npz'),vertices_m=verts,triangles=tris,**frame,
            protected_source_triangle_indices=np.flatnonzero(frozen_triangles),
            metadata_json=np.asarray(json.dumps(dict(meta,variant=name))))
    np.savez_compressed(args.output/'closed_diagnostic.npz',vertices_m=origin+local_result@basis.T,triangles=all_triangles,**frame,metadata_json=np.asarray(json.dumps(meta)))
    (args.output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
    bpy.ops.wm.save_as_mainfile(filepath=str((args.output/'bedrock.blend').resolve()))
    print(json.dumps(dict(output=str(args.output.resolve()),metrics=metrics,seconds=time.perf_counter()-start)),flush=True)
    if not meta['basic_geometry_gate_passed']:
        raise RuntimeError('Retained failed geometry for review; basic original-surface topology gate failed')


if __name__=='__main__':main()
