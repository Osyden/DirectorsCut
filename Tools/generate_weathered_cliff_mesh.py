"""Refine an accepted massif into an original, physically weathered cliff mesh.

Run in Blender CPU: --background --factory-startup --threads 8 --python-exit-code 1
--python THIS_FILE -- --input accepted_open256.npz --output-dir Saved/Weathered/first

Conforming adaptive midpoint refinement preserves the original ABD/ADC surface.
Existing Blender 3D Manhattan Voronoi distance fields add physically scaled
intermediate facets; the retained ridge mode is the first soft-noise diagnostic.
Stable source normals are globally biased upward to avoid folds. Guards preserve shore,
seams and temple supports exactly. Actual transported masks are barycentrically
carried from the source; they are NOT new erosion simulated on the weathered mesh.
No booleans, polygon pockets, regular bedding bands or heightfield reprojection.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import time

import numpy as np

sys.dont_write_bytecode = True


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def smoothstep(x):
    q=np.clip(x,0,1);return q*q*(3-2*q)


def original_mesh(data):
    sys.path.insert(0,str(Path(__file__).resolve().parent))
    from review_eroded_cliff_sector import triangles
    h=data['height_m'];grid_triangles=triangles(data['heightfield_valid_mask']>0).astype(np.int32)
    world=np.concatenate((data['world_xy_m'],h[...,None]),axis=-1).reshape(-1,3).astype(np.float64)
    used=np.unique(grid_triangles);remap=np.full(len(world),-1,np.int32);remap[used]=np.arange(len(used))
    return world[used],remap[grid_triangles],grid_triangles,world


def refine(vertices,faces,target_m=1.):
    """Conforming split of marked shared edges, retaining source triangle IDs."""
    lineage=np.arange(len(faces),dtype=np.int32);history=[]
    for iteration in range(12):
        edges=np.stack((faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]]),axis=1).reshape(-1,2)
        edges=np.sort(edges,axis=1)
        unique,inverse=np.unique(edges,axis=0,return_inverse=True)
        lengths=np.linalg.norm(vertices[unique[:,0]]-vertices[unique[:,1]],axis=1)
        marked=lengths>target_m*(1+1.e-9)
        history.append(dict(iteration=iteration,vertices=len(vertices),triangles=len(faces),marked_edges=int(marked.sum()),maximum_edge_m=float(lengths.max())))
        if not marked.any():return vertices,faces,lineage,history
        if len(vertices)+int(marked.sum())>3000000:raise ValueError('Refinement exceeds3 million vertex bound')
        mid=np.full(len(unique),-1,np.int32);mid[marked]=np.arange(marked.sum(),dtype=np.int32)+len(vertices)
        vertices=np.concatenate((vertices,.5*(vertices[unique[marked,0]]+vertices[unique[marked,1]])))
        mids=mid[inverse].reshape(-1,3)
        masks=(mids[:,0]>=0).astype(np.uint8)+2*(mids[:,1]>=0)+4*(mids[:,2]>=0)
        new=[];ids=[]
        # Each case respects source winding; both incident faces share the exact
        # midpoint index, so mixed refinement levels introduce no T junctions.
        for pattern in range(8):
            select=np.where(masks==pattern)[0]
            if not len(select):continue
            a,b,c=faces[select].T;ab,bc,ca=mids[select].T
            if pattern==0:parts=[(a,b,c)]
            elif pattern==1:parts=[(a,ab,c),(ab,b,c)]
            elif pattern==2:parts=[(b,bc,a),(bc,c,a)]
            elif pattern==4:parts=[(c,ca,b),(ca,a,b)]
            elif pattern==3:parts=[(b,bc,ab),(a,ab,c),(ab,bc,c)]
            elif pattern==6:parts=[(c,ca,bc),(b,bc,a),(bc,ca,a)]
            elif pattern==5:parts=[(a,ab,ca),(c,ca,b),(ca,ab,b)]
            else:parts=[(a,ab,ca),(ab,b,bc),(ca,bc,c),(ab,bc,ca)]
            for part in parts:new.append(np.stack(part,axis=1));ids.append(lineage[select])
        faces=np.concatenate(new).astype(np.int32);lineage=np.concatenate(ids)
        print(f'REFINE {iteration}: {len(vertices)} vertices / {len(faces)} triangles',flush=True)
    raise ValueError('Refinement failed to reach requested surface-edge size')


def correspondence(vertices,faces,lineage,grid_triangles,grid_world):
    flat=faces.reshape(-1);_,first=np.unique(flat,return_index=True)
    source_index=lineage[first//3]
    abc=grid_world[grid_triangles[source_index]]
    a=abc[:,0,:2];b=abc[:,1,:2]-a;c=abc[:,2,:2]-a;p=vertices[:,:2]-a
    determinant=b[:,0]*c[:,1]-b[:,1]*c[:,0]
    u=(p[:,0]*c[:,1]-p[:,1]*c[:,0])/determinant
    v=(b[:,0]*p[:,1]-b[:,1]*p[:,0])/determinant
    weights=np.stack((1-u-v,u,v),axis=1)
    if weights.min()<-1.e-7 or weights.max()>1+1.e-7:raise ValueError('Refined vertex left its original source triangle')
    return source_index.astype(np.int32),weights


def sample_grid(field,source_index,weights,grid_triangles):
    values=np.asarray(field).reshape((-1,)+np.asarray(field).shape[2:])
    selected=values[grid_triangles[source_index]]
    return np.sum(selected*weights[(...,)+(None,)*(selected.ndim-2)],axis=1)


def source_normals(data):
    """Two metric2m-grid lowpasses: stable normals, not mesh smoothing."""
    height=data['height_m'].astype(float);valid=data['heightfield_valid_mask'].astype(float)
    def blur(a):
        for axis in (0,1):
            pad=[(0,0),(0,0)];pad[axis]=(1,1);p=np.pad(a,pad,mode='edge')
            if axis==0:a=(p[:-2]+2*p[1:-1]+p[2:])/4
            else:a=(p[:,:-2]+2*p[:,1:-1]+p[:,2:])/4
        return a
    for _ in range(2):
        weight=blur(valid);height=blur(height*valid)/np.maximum(weight,1.e-9)
    gy,gx=np.gradient(height,float(data['cell_size_m']))
    local=np.stack((-gx,-gy,np.ones_like(gx)),axis=-1)
    local/=np.linalg.norm(local,axis=-1)[...,None]
    basis=np.column_stack((data['basis_u_world'],data['basis_v_world'],data['basis_up_world']))
    return local@basis.T


def guard_field(data,vertices,source_index,weights,grid_triangles):
    from mathutils.kdtree import KDTree
    kinds=data['constraint_type'].reshape(-1)
    guard_kind=np.bitwise_or.reduce(kinds[grid_triangles[source_index]],axis=1).astype(np.uint8)
    points=data['world_xy_m'].reshape(-1,2)[data['protected_mask'].reshape(-1)>=1]
    tree=KDTree(len(points))
    for i,p in enumerate(points):tree.insert((float(p[0]),float(p[1]),0.),i)
    tree.balance()
    distances=np.empty(len(vertices))
    for i,p in enumerate(vertices):distances[i]=tree.find((float(p[0]),float(p[1]),0.))[2]
    guard=smoothstep((distances-3.)/12.)
    guard[guard_kind!=0]=0.
    return guard,guard_kind,distances


def rock_relief(vertices,normals,guard,erodibility,data,seed,strength,mode='faceted'):
    from mathutils import Vector,noise
    basis=np.column_stack((data['basis_u_world'],data['basis_v_world'],data['basis_up_world']))
    local=(vertices-data['frame_origin_world_m'])@basis
    # A single inclined geological frame across the whole tile. The field is
    # genuinely3D and warped; no sine strata or repeated elevation thresholds.
    strike=np.array([.982,.112,.151]);strike/=np.linalg.norm(strike)
    bedding=np.array([-.09,.32,.943]);bedding-=strike*np.dot(strike,bedding);bedding/=np.linalg.norm(bedding)
    inward=np.cross(strike,bedding)
    geology=np.column_stack((strike,bedding,inward))
    coordinates=local@geology
    rng=np.random.default_rng(seed);offsets=rng.uniform(-150,150,(6,3))
    scalar=np.zeros(len(vertices));middle=np.zeros(len(vertices));chips=np.zeros(len(vertices))
    slope=np.degrees(np.arccos(np.clip(normals[:,2],-1,1)))
    steep=smoothstep((slope-24.)/36.)
    hardness=np.clip(1-erodibility,.25,1.)
    for i in np.where((guard>0)&(steep>0))[0]:
        p=coordinates[i]
        warp_bound=2.4 if mode=='ridge' else .7
        warp=np.array([noise.noise(Vector(tuple(p/42.+offsets[j])),noise_basis='PERLIN_ORIGINAL') for j in range(3)])*warp_bound
        p=p+warp
        fine=noise.noise(Vector(tuple(p/np.array([3.6,2.8,3.2])+offsets[5])),noise_basis='PERLIN_ORIGINAL')
        if mode=='ridge':
            ridged=noise.ridged_multi_fractal(Vector(tuple(p/np.array([18.,11.,14.])+offsets[3])),1.25,2.,3.,1.,2.,noise_basis='PERLIN_ORIGINAL')
            hetero=noise.hetero_terrain(Vector(tuple(p/np.array([30.,20.,24.])+offsets[4])),1.1,2.,2.,.65,noise_basis='PERLIN_ORIGINAL')
            mid=np.clip(1.65*(ridged-.95)+.55*(hetero-.85),-1.8,1.7)
            chip=.20*fine
        else:
            # Manhattan Voronoi distance is piecewise planar in this common
            # geological frame. Random feature sites give irregular extents;
            # a smaller, weaker distance field breaks pristine plane edges.
            # This is direct3D surface relief, not polygonal boolean cavities.
            coarse=noise.voronoi(Vector(tuple(p/np.array([14.,8.,11.])+offsets[3])),distance_metric='MANHATTAN')[0][0]
            small=noise.voronoi(Vector(tuple(p/np.array([5.5,3.5,4.5])+offsets[4])),distance_metric='MANHATTAN')[0][0]
            mid=np.clip(2.2*(.85-coarse)+.55*(.85-small),-2.6,2.2)
            chip=.12*fine
        amount=guard[i]*steep[i]*(.65+.35*hardness[i])*strength
        middle[i]=mid*amount;chips[i]=chip*amount;scalar[i]=middle[i]+chips[i]
    settings=dict(seed=seed,strength=strength,relief_mode=mode,geology_axes_local=geology.tolist(),
                  ridge_scales_by_octave_m=[[18.,11.,14.],[9.,5.5,7.],[4.5,2.75,3.5]],
                  heterogeneous_scales_m=[[30.,20.,24.],[15.,10.,12.]],chip_scales_m=[3.6,2.8,3.2],
                  ridge_fractal=dict(H=1.25,lacunarity=2.,octaves=3,offset=1.,gain=2.),
                  heterogeneous_fractal=dict(H=1.1,lacunarity=2.,octaves=2,offset=.65),
                  domain_warp_scale_m=42.,domain_warp_component_bound_m=2.4,
                  intermediate_clamp_m=[-1.8,1.7],chip_coefficient_m=.20,
                  slope_fade_degrees=[24.,60.],hardness_definition='clamp(1-source_erodibility,.25,1)',
                  displacement_direction='stable smoothed original-surface unit normal, never recomputed from weathered mesh')
    if mode=='faceted':
        for key in ('ridge_scales_by_octave_m','heterogeneous_scales_m','ridge_fractal','heterogeneous_fractal'):settings.pop(key)
        settings.update(distance_field='Blender mathutils.noise.voronoi nearest-feature Manhattan distance',
                        coarse_distance_scales_m=[14.,8.,11.],smaller_distance_scales_m=[5.5,3.5,4.5],
                        coarse_distance_coefficient_m=2.2,smaller_distance_coefficient_m=.55,
                        intermediate_clamp_m=[-2.6,2.2],chip_coefficient_m=.12,domain_warp_component_bound_m=.7)
    return scalar,middle,chips,settings


def restrict_folds(baseline,faces,normals,scalar):
    """Choose one smooth upward bias; per-vertex clipping creates new creases.

    Reducing the normal's horizontal part is global, then directions are
    renormalized. Physical relief magnitude stays unchanged. The0 fallback is
    vertical displacement, which leaves the original XY mapping exact.
    """
    def signed_xy(v):
        a,b,c=[v[faces[:,i],:2] for i in range(3)]
        return (b[:,0]-a[:,0])*(c[:,1]-a[:,1])-(b[:,1]-a[:,1])*(c[:,0]-a[:,0])
    source_area=signed_xy(baseline)
    if np.any(source_area<=0):raise ValueError('Original source winding is not positive in arena XY')
    history=[]
    for factor in (1.,.75,.5,.25,.125,0.):
        direction=normals.copy();direction[:,:2]*=factor
        direction/=np.linalg.norm(direction,axis=1)[:,None]
        result=baseline+direction*scalar[:,None]
        ratio=signed_xy(result)/source_area;bad=ratio<.20
        history.append(dict(horizontal_normal_factor=factor,violating_triangles=int(bad.sum())))
        if not bad.any():return result,direction,dict(attempts=history,horizontal_normal_factor=factor,minimum_xy_area_ratio=float(ratio.min()),amplitude_attenuation=False)
    raise ValueError('Bounded geometry displacement did not preserve positive projected triangles')


def normalized_masks(data):
    valid=data['heightfield_valid_mask']>0;channels=[];report={}
    for key,logarithmic in (('flow_accumulation',True),('deposited_m',False),('eroded_m',False)):
        raw=data[key].astype(float);field=np.log1p(np.maximum(raw,0)) if logarithmic else np.maximum(raw,0)
        maximum=float(field[valid].max());channels.append(field/max(maximum,1.e-12))
        report[key]=dict(transform='log1p' if logarithmic else 'identity',divisor=maximum,raw_max=float(raw[valid].max()))
    return np.stack((*channels,np.ones(valid.shape)),axis=-1),report


def save_pair(folder,name,fields,metadata,selected_faces=None):
    faces=fields['triangles'] if selected_faces is None else fields['triangles'][selected_faces]
    used=np.unique(faces);remap=np.full(len(fields['vertices_m']),-1,np.int32);remap[used]=np.arange(len(used))
    sliced={key:(value[used] if key in ('vertices_m','baseline_vertices_m','source_triangle_index','source_barycentric','guard_weight','guard_kind','guard_distance_m','vertex_rgba','stable_source_normal','normal_displacement_m','fold_attenuation') else value) for key,value in fields.items()}
    sliced['triangles']=remap[faces]
    for variant in ('baseline','weathered'):
        output=dict(sliced)
        if variant=='baseline':output['vertices_m']=output['baseline_vertices_m']
        meta=dict(metadata,variant=variant,subset=name,vertices=len(used),triangles=len(faces),closed_volume=False,open_boundary=True)
        output['metadata_json']=np.asarray(json.dumps(meta))
        np.savez_compressed(folder/(name+'_'+variant+'.npz'),**output)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True);parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--edge-m',type=float,default=1.);parser.add_argument('--seed',type=int,default=81517)
    parser.add_argument('--strength',type=float,default=1.)
    parser.add_argument('--relief-mode',choices=('ridge','faceted'),default='faceted',help='Ridge reproduces the first rejected soft-noise candidate')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.output_dir.exists() or 'saved' not in [p.lower() for p in args.output_dir.resolve().parts]:parser.error('Use a NEW ignored Saved directory')
    if not .75<=args.edge_m<=1. or not 0.<args.strength<=1.5:parser.error('Use0.75..1m edge target and strength within0..1.5')
    with np.load(args.input,allow_pickle=False) as archive:data={k:archive[k].copy() for k in archive.files}
    start=time.monotonic();baseline,faces,grid_triangles,grid_world=original_mesh(data)
    baseline,faces,lineage,refinement=refine(baseline,faces,args.edge_m)
    source_index,weights=correspondence(baseline,faces,lineage,grid_triangles,grid_world)
    reconstructed=np.sum(grid_world[grid_triangles[source_index]]*weights[...,None],axis=1)
    error=float(np.max(np.abs(reconstructed-baseline)))
    if error>1.e-6:raise ValueError('Source correspondence does not reconstruct original positions')
    normals=sample_grid(source_normals(data),source_index,weights,grid_triangles);normals/=np.linalg.norm(normals,axis=1)[:,None]
    erodibility=sample_grid(data['erodibility'],source_index,weights,grid_triangles)
    guard,guard_kind,distance=guard_field(data,baseline,source_index,weights,grid_triangles)
    print(f'NOISE: {len(baseline)} original3D vertices, guards ready',flush=True)
    scalar,middle,chips,settings=rock_relief(baseline,normals,guard,erodibility,data,args.seed,args.strength,args.relief_mode)
    candidate,effective_normals,fold=restrict_folds(baseline,faces,normals,scalar)
    settings['displacement_direction']='Stable original normal with one global horizontal-component factor, then renormalized; see normal_horizontal_factor. No per-vertex amplitude clipping.'
    settings['normal_horizontal_factor']=fold['horizontal_normal_factor']
    mask_grid,normalization=normalized_masks(data);masks=sample_grid(mask_grid,source_index,weights,grid_triangles).astype(np.float32)
    centre=data['frame_origin_world_m'].astype(float).copy()
    centre+=data['basis_u_world']*(data['height_m'].shape[1]-1)*float(data['cell_size_m'])*.5+data['basis_v_world']*(data['height_m'].shape[0]-1)*float(data['cell_size_m'])*.5
    centre[2]=float(np.median(data['height_m'][data['heightfield_valid_mask']>0]))
    displacement=np.linalg.norm(candidate-baseline,axis=1);fixed=guard==0
    metrics=dict(vertices=len(candidate),triangles=len(faces),source_reconstruction_max_error_m=error,
                 fixed_vertices=int(fixed.sum()),fixed_positions_bitwise_equal=bool(np.array_equal(candidate[fixed],baseline[fixed])),
                 displacement_max_m=float(displacement.max()),displacement_nonzero_p50_p95_p99_m=np.percentile(displacement[displacement>0],[50,95,99]).tolist(),
                 scalar_range_m=[float(scalar.min()),float(scalar.max())],
                 refinement=refinement,fold_guard=fold,elapsed_seconds=time.monotonic()-start,
                 finite=bool(np.isfinite(candidate).all()),overhangs_proven=False)
    if not metrics['finite'] or not metrics['fixed_positions_bitwise_equal']:raise ValueError('Nonfinite geometry or altered protected point')
    import bpy
    metadata=dict(schema='directorscut.weathered_cliff_mesh.v1',source_kind='weathered_original_triangle_cliff',
                  source_npz=str(args.input.resolve()),source_sha256=sha(args.input),source_metadata=json.loads(str(data['metadata_json'])),
                  generator_source_sha256=sha(__file__),blender_version=bpy.app.version_string,physical_units='absolute arena/design-world XYZ metres; no exaggeration',
                  frame='same tangent/outward/up frame as accepted source',surface_edge_target_m=args.edge_m,settings=settings,metrics=metrics,
                  correspondence='source_triangle_index indexes source_grid_triangles, whose entries are original NPZ grid flat vertex IDs; source_barycentric weights sum1',
                  guard_kind_bits={'invalid':1,'seam':2,'coast':4,'temple':8},guard_description='OR of source triangle corners; any kind fixed. Additional Euclidean XY distance fade from3m fixed to15m free.',
                  color_normalization=normalization,color_provenance='Old actual v3 hydraulic transport, normalized before barycentric interpolation; not recomputed erosion after displacement',
                  close_focus_source_row_col=[89,64],close_extent_uv_m=[72.,40.],
                  limitations='Original refined open triangle mesh. Positive XY triangle Jacobians are enforced: no overhang claim. Contact uses these original3D triangles, not source-height interpolation. New shape relief is authored geological noise, not an erosion solver or measured rock scan.')
    fields=dict(vertices_m=candidate,baseline_vertices_m=baseline,triangles=faces,source_triangle_index=source_index,source_barycentric=weights,
                source_grid_triangles=grid_triangles,guard_weight=guard.astype(np.float32),guard_kind=guard_kind,guard_distance_m=distance.astype(np.float32),
                vertex_rgba=masks,stable_source_normal=effective_normals.astype(np.float32),normal_displacement_m=scalar.astype(np.float32),fold_attenuation=np.ones(len(scalar),np.float32),
                frame_origin_world_m=centre,basis_u_world=data['basis_u_world'],basis_v_world=data['basis_v_world'],basis_z_world=data['basis_up_world'])
    args.output_dir.mkdir(parents=True)
    save_pair(args.output_dir,'sector',fields,metadata)
    uv=(baseline[:,:2]-data['world_xy_m'][89,64])@np.column_stack((data['basis_u_world'][:2],data['basis_v_world'][:2]))
    inside=(np.abs(uv[:,0])<=36.+1.e-7)&(np.abs(uv[:,1])<=20.+1.e-7)
    save_pair(args.output_dir,'crop',fields,metadata,np.all(inside[faces],axis=1))
    (args.output_dir/'weathered.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(output=str(args.output_dir.resolve()),**metrics),indent=2))


if __name__=='__main__':main()
