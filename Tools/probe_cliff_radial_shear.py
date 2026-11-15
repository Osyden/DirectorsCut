"""One small-crop proof of invertible radial shear, distinct from normal relief.

V'=V+G(U,V)*F(U,Z); U'=U and Z'=Z. F is independent of V and clamped to2m.
G is smoothstep of exact Euclidean XY distance to original protected grid points
over3..15m, hence |dG/dV|<=.125 and continuous mapping determinant>=.75.
This continuous bound is not a discrete triangle/self-intersection certificate.
Run with Blender CPU; original arbitrary triangles remain the contact source.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True,help='Existing refined72m crop baseline NPZ')
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--seed',type=int,default=81517)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.output_dir.exists() or 'saved' not in [p.lower() for p in args.output_dir.resolve().parts]:parser.error('Choose a NEW Saved output directory')
    from mathutils import Vector,noise
    from mathutils.kdtree import KDTree
    import bpy
    with np.load(args.input,allow_pickle=False) as archive:old={k:archive[k].copy() for k in archive.files}
    old_meta=json.loads(str(old['metadata_json']));source_path=Path(old_meta['source_npz'])
    if sha(source_path)!=old_meta['source_sha256']:raise ValueError('Original massif source hash changed')
    with np.load(source_path,allow_pickle=False) as archive:source={k:archive[k].copy() for k in archive.files}
    baseline=old['baseline_vertices_m'].astype(np.float64);faces=old['triangles']
    if len(baseline)>150000:raise ValueError('This proof is bounded to the small crop, not full sector')
    u=source['basis_u_world'].astype(float);v=source['basis_v_world'].astype(float);up=source['basis_up_world'].astype(float)
    origin=source['frame_origin_world_m'].astype(float)
    local=(baseline-origin)@np.column_stack((u,v,up))
    # Constant geological strike/dip in U/Z only; there is no V-dependent
    # slope, hardness, erodibility or normal component hiding in this field.
    s=.988*local[:,0]+.154*local[:,2]
    t=-.154*local[:,0]+.988*local[:,2]
    offsets=np.random.default_rng(args.seed).uniform(-150,150,(6,3))
    field=np.empty(len(baseline))
    for i in range(len(baseline)):
        p=np.array([s[i],t[i],0.])
        warp=np.array([noise.noise(Vector(tuple(p/42.+offsets[j])),noise_basis='PERLIN_ORIGINAL') for j in range(2)])*.7
        q=p.copy();q[:2]+=warp
        coarse=noise.voronoi(Vector(tuple(q/np.array([14.,8.,11.])+offsets[3])),distance_metric='MANHATTAN')[0][0]
        small=noise.voronoi(Vector(tuple(q/np.array([5.5,3.5,4.5])+offsets[4])),distance_metric='MANHATTAN')[0][0]
        detail=noise.noise(Vector(tuple(q/np.array([3.6,2.8,3.2])+offsets[5])),noise_basis='PERLIN_ORIGINAL')
        field[i]=np.clip(2.2*(.85-coarse)+.55*(.85-small)+.12*detail,-2.,2.)
    points=source['world_xy_m'].reshape(-1,2)[source['protected_mask'].reshape(-1)>=1]
    tree=KDTree(len(points))
    for i,p in enumerate(points):tree.insert((float(p[0]),float(p[1]),0.),i)
    tree.balance()
    distance=np.array([tree.find((float(p[0]),float(p[1]),0.))[2] for p in baseline])
    q=np.clip((distance-3.)/12.,0,1);guard=q*q*(3-2*q)
    if np.any((old['guard_kind']!=0)&(guard!=0)):raise ValueError('Semantic corner guard extends outside natural zero distance region')
    offset=field*guard;candidate=baseline+offset[:,None]*v
    a,b,c=[baseline[faces[:,i]] for i in range(3)];source_normals=np.cross(b-a,c-a)
    aa,bb,cc=[candidate[faces[:,i]] for i in range(3)];new_normals=np.cross(bb-aa,cc-aa)
    source_area=np.linalg.norm(source_normals,axis=1);new_area=np.linalg.norm(new_normals,axis=1)
    grid=np.concatenate((source['world_xy_m'],source['height_m'][...,None]),axis=-1).reshape(-1,3)
    p=grid[old['source_grid_triangles'][old['source_triangle_index']]]
    normals=np.cross(p[:,1]-p[:,0],p[:,2]-p[:,0]);normals/=np.linalg.norm(normals,axis=1)[:,None]
    normal_relief=np.abs(np.sum((candidate-baseline)*normals,axis=1))
    protected=guard==0
    delta_local=(candidate-baseline)@np.column_stack((u,v,up))
    d1=b[:,:2]-a[:,:2];d2=c[:,:2]-a[:,:2]
    xy_source=d1[:,0]*d2[:,1]-d1[:,1]*d2[:,0]
    d1=bb[:,:2]-aa[:,:2];d2=cc[:,:2]-aa[:,:2]
    xy_new=d1[:,0]*d2[:,1]-d1[:,1]*d2[:,0]
    metrics=dict(vertices=len(candidate),triangles=len(faces),continuous_determinant_lower_bound=.75,
                 actual_function_range_m=[float(field.min()),float(field.max())],actual_offset_range_m=[float(offset.min()),float(offset.max())],
                 source_normal_relief_p50_p95_p99_max_m=np.percentile(normal_relief,[50,95,99,100]).tolist(),
                 fixed_vertices=int(protected.sum()),fixed_positions_bitwise_equal=bool(np.array_equal(candidate[protected],baseline[protected])),
                 maximum_U_or_Z_change_m=float(np.abs(delta_local[:,[0,2]]).max()),
                 degenerate_triangles=int(np.count_nonzero(new_area<1.e-10)),minimum_area_ratio=float((new_area/source_area).min()),
                 projected_XY_reversed_triangles=int(np.count_nonzero(xy_new*xy_source<0)),
                 relative_normal_reversed_triangles=int(np.count_nonzero(np.sum(source_normals*new_normals,axis=1)<0)),
                 self_intersections_verified=False,overhangs_verified=False)
    if not np.isfinite(candidate).all() or metrics['degenerate_triangles'] or not metrics['fixed_positions_bitwise_equal']:raise ValueError('Invalid discrete geometry or changed fixed guard')
    keep=('baseline_vertices_m','triangles','source_triangle_index','source_barycentric','source_grid_triangles','guard_kind','vertex_rgba',
          'frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world')
    fields={k:old[k] for k in keep}
    fields.update(vertices_m=candidate,baseline_vertices_m=baseline,guard_weight=guard,guard_distance_m=distance,
                  shear_function_m=field,radial_offset_m=offset)
    metadata=dict(schema='directorscut.radial_shear_cliff_probe.v1',source_kind='radial_shear_original_cliff_mesh',
                  source_npz=str(source_path.resolve()),source_sha256=sha(source_path),input_mesh_npz=str(args.input.resolve()),input_mesh_sha256=sha(args.input),
                  input_metadata=old_meta,generator_source_sha256=sha(__file__),blender_version=bpy.app.version_string,seed=args.seed,
                  closed_volume=False,open_boundary=True,physical_units='absolute arena/design-world XYZ metres; no vertical exaggeration',
                  mapping="V'=V+G(U,V)*F(U,Z), U'=U, Z'=Z",radial_basis_world=v.tolist(),
                  F_definition='Two rotated U/Z coordinates, V-independent Manhattan Voronoi scales14/8m and5.5/3.5m,0.7m warp at42m,0.12m small noise, clipped[-2,2]m; no slope/hardness/erodibility multiplier',
                  G_definition='smoothstep((Euclidean XY distance to original protected vertices-3m)/12m); no crop-edge override or extra factors',
                  continuous_proof='Distance is1-Lipschitz, smoothstep derivative<=1.5/12=.125; |F|<=2 and dF/dV=0, so det=1+F*dG/dV>=.75. This is not a discrete triangle certificate.',
                  color_normalization=old_meta['color_normalization'],color_provenance=old_meta['color_provenance'],metrics=metrics,
                  limitations='Single72m crop proof only. Original discrete mesh must be independently checked; no overhang/self-intersection claims from the continuous bound. No asset import or fullsector approval.')
    args.output_dir.mkdir(parents=True)
    for variant in ('baseline','candidate'):
        output=dict(fields)
        if variant=='baseline':output['vertices_m']=baseline
        output['metadata_json']=np.asarray(json.dumps(dict(metadata,variant=variant)))
        np.savez_compressed(args.output_dir/(variant+'.npz'),**output)
    (args.output_dir/'radial_shear.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(output=str(args.output_dir.resolve()),**metrics),indent=2))


if __name__=='__main__':main()
