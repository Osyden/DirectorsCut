"""Offline authored radial compression of a real eroded parent, not erosion.

The measured shoreline is joined to the selected parent through a declared
six-metre apron. Optional physical source-height smoothing is explicitly authored,
not erosion. After that filter, U/Z remain fixed during monotone V compression.
No runtime terrain, foundation, material or water assets are changed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import numpy as np


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    with np.load(path, allow_pickle=False) as z:
        return {k: z[k].copy() for k in z.files}


def physical_height_lowpass(height, valid, fixed, cell_m, sigma_m):
    """Normalized Gaussian in source metres, support ceil(3 sigma / cell).

    Invalid padding contributes neither height nor weight. Array edges repeat
    their nearest sample; fixed historical baselevels are restored afterwards.
    This is authored morphology smoothing, not conservative sediment transport.
    """
    if sigma_m == 0:
        return height.copy(), dict(sigma_m=0.,support_radius_m=0.,enabled=False)
    radius=int(np.ceil(3.*sigma_m/cell_m))
    offsets=np.arange(-radius,radius+1)*cell_m
    kernel=np.exp(-.5*(offsets/sigma_m)**2); kernel/=kernel.sum()
    def separable(field):
        result=field.copy()
        for axis in (0,1):
            pads=[(0,0),(0,0)];pads[axis]=(radius,radius)
            padded=np.pad(result,pads,mode='edge')
            result=np.apply_along_axis(lambda x:np.convolve(x,kernel,mode='valid'),axis,padded)
        return result
    weight=separable(valid.astype(float))
    filtered=separable(np.where(valid,height,0.))/np.maximum(weight,1.e-30)
    filtered=np.where(valid & ~fixed,filtered,height)
    if not np.isfinite(filtered).all():raise ValueError('Nonfinite source filter')
    delta=filtered[valid]-height[valid]
    return filtered,dict(enabled=True,sigma_m=float(sigma_m),support_radius_m=float(radius*cell_m),
        kernel_samples=len(kernel),kernel_weights=kernel.tolist(),normalization='valid-domain normalized separable convolution',
        boundary='nearest sample extension; historical fixed baselevels restored',
        authored_height_change_range_m=[float(delta.min()),float(delta.max())],
        authored_absolute_height_change_p50_p90_p99_m=np.percentile(abs(delta),[50,90,99]).tolist(),
        fixed_height_bitwise_preserved=bool(np.array_equal(filtered[fixed],height[fixed])))


def grid_triangles(rows, cols):
    a = (np.arange(rows-1)[:, None]*cols + np.arange(cols-1)[None, :]).ravel()
    return np.stack((np.stack((a,a+1,a+cols+1),axis=1),
                     np.stack((a,a+cols+1,a+cols),axis=1)),axis=1).reshape(-1,3).astype(np.int32)


def source_samples(height, u, v, cell):
    """Exact ABD/ADC barycentrics in the original two-metre source grid."""
    x=u/cell; y=v/cell
    c=np.clip(np.floor(x).astype(int),0,height.shape[1]-2)
    r=np.clip(np.floor(y).astype(int),0,height.shape[0]-2)
    fx=x-c; fy=y-r; upper=fx>=fy
    a=r*height.shape[1]+c
    ids=np.stack((a,np.where(upper,a+1,a+height.shape[1]+1),
                  np.where(upper,a+height.shape[1]+1,a+height.shape[1])),axis=-1)
    weights=np.where(upper[...,None],np.stack((1-fx,fx-fy,fy),axis=-1),
                     np.stack((1-fy,fx,fy-fx),axis=-1))
    return np.sum(height.ravel()[ids]*weights,axis=-1),ids,weights


def audit(vertices,triangles):
    p=vertices[triangles]; cross=np.cross(p[:,1]-p[:,0],p[:,2]-p[:,0])
    edges=np.concatenate((triangles[:,[0,1]],triangles[:,[1,2]],triangles[:,[2,0]]))
    _,inv,counts=np.unique(np.sort(edges,axis=1),axis=0,return_inverse=True,return_counts=True)
    direction=np.bincount(inv,weights=np.where(edges[:,0]<edges[:,1],1,-1))
    result=dict(vertices=len(vertices),triangles=len(triangles),finite=bool(np.isfinite(vertices).all()),
                boundary_edges=int((counts==1).sum()),nonmanifold_edges=int((counts>2).sum()),
                winding_errors=int(((counts==2)&(direction!=0)).sum()),
                degenerate_triangles=int((np.linalg.norm(cross,axis=1)<=1.e-10).sum()),
                minimum_positive_xy_twice_area_m2=float(cross[:,2].min()),
                dimensions_uvz_m=np.ptp(vertices,axis=0).tolist())
    if not result['finite'] or any(result[k] for k in ('nonmanifold_edges','winding_errors','degenerate_triangles')) or cross[:,2].min()<=0:
        raise ValueError(f'Mesh gate failed: {result}')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--parent',type=Path,required=True)
    parser.add_argument('--shore-reference',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--z-offset',type=float,default=-55.)
    parser.add_argument('--toe-compression',type=float,default=.12)
    parser.add_argument('--crest-compression',type=float,default=.35)
    parser.add_argument('--join-width',type=float,default=6.)
    parser.add_argument('--envelope-slope',type=float,default=2.)
    parser.add_argument('--source-smoothing-sigma',type=float,default=0.,help='Authored Gaussian height smoothing in source metres; support 3 sigma. Zero retains original source.')
    args=parser.parse_args()
    if args.output.exists() or 'saved' not in [v.lower() for v in args.output.resolve().parts]:
        parser.error('Use a new ignored Saved directory')
    if min(args.toe_compression,args.crest_compression,args.join_width,args.envelope_slope)<=0:
        parser.error('Compression and join parameters must be positive')
    if not np.isfinite(args.source_smoothing_sigma) or args.source_smoothing_sigma<0:
        parser.error('Source smoothing sigma must be finite and nonnegative')
    parent=load(args.parent); shore=load(args.shore_reference)
    origin=parent['frame_origin_world_m'].astype(float)
    basis=np.stack([parent['basis_u_world'],parent['basis_v_world'],parent['basis_z_world']],axis=1)
    if not np.allclose(basis.T@basis,np.eye(3),atol=1.e-12) or not np.isclose(np.linalg.det(basis),1.):
        raise ValueError('Expected a right-handed rigid arena frame')
    for key in ('frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world'):
        if not np.array_equal(parent[key],shore[key]): raise ValueError(f'Shore frame mismatch: {key}')
    controls=shore['contact_uv_m']; uu=controls[:,0]; sv=controls[:,1]; sz=shore['contact_water_height_m']
    cell=float(parent['cell_size_m']); cols=np.rint(uu/cell).astype(int)
    if not np.allclose(uu/cell,cols,atol=1.e-12) or not np.allclose(np.diff(uu),cell):
        raise ValueError('Source-aligned, equally spaced shore controls required')
    original_height=parent['height_m'].astype(float)
    height,filter_info=physical_height_lowpass(original_height,parent['heightfield_valid_mask']>0,
        parent.get('baselevel_mask',np.zeros_like(original_height))>0,cell,args.source_smoothing_sigma)
    source_top=(height.shape[0]-1)*cell
    gap=height[:,cols]+args.z_offset-sz[None,:]
    roots=[]; counts=[]
    for i in range(len(uu)):
        rr=np.flatnonzero((gap[:-1,i]<0)&(gap[1:,i]>=0))
        if not len(rr): raise ValueError(f'No upward source-water crossing at U={uu[i]}')
        r=rr[-1]; roots.append(cell*(r-gap[r,i]/(gap[r+1,i]-gap[r,i])));counts.append(len(rr))
    roots=np.asarray(roots)
    # Minimal Lipschitz majorant of the last dry crossings. This is an authored
    # contour envelope, NOT a claim that branch endpoints form a natural contour.
    start=np.max(roots[None,:]-args.envelope_slope*abs(uu[:,None]-uu[None,:]),axis=1)
    if np.any(start>=source_top-cell): raise ValueError('No usable main-face source interval')
    rows=int(np.ceil((source_top-start.min())/cell))+1
    s=np.linspace(0.,1.,rows)[:,None]
    source_v=start[None,:]+s*(source_top-start)[None,:]
    U=np.broadcast_to(uu,source_v.shape)
    Z,indices,weights=source_samples(height,U,source_v,cell)
    Z+=args.z_offset
    derivative=args.toe_compression+(args.crest_compression-args.toe_compression)*s*s
    radial=(source_top-start)[None,:]*(args.toe_compression*s+(args.crest_compression-args.toe_compression)*s**3/3.)
    V=sv[None,:]+args.join_width+radial
    toe_gap=Z[0]-sz
    if toe_gap.min()<-1.e-8: raise ValueError('Selected parent toe is below measured shore')
    # U/Z of the explicitly filtered source is retained during compression. The regular
    # parameter-grid retessellation approximates original interior triangle edges;
    # source barycentrics for EVERY main-face vertex are provided, not hidden.
    main=np.stack((U,V,Z),axis=-1)
    baseline_main=np.stack((U,source_v-600.,Z),axis=-1)
    # Lower overlap and waterline copied from the already verified finite-water
    # reference. The 0..join_width strip is a separate authored contact join.
    fractions=np.array([0.,1./3,2./3])[:,None]
    join_v=sv[None,:]+args.join_width*fractions
    join_z=sz[None,:]+toe_gap[None,:]*fractions
    lower_v=shore['lower_overlap_uv_m'][:,1]
    lower_z=shore['lower_overlap_ground_height_m']
    lower=np.stack((uu,lower_v,lower_z),axis=-1)[None,:,:]
    join=np.stack((np.broadcast_to(uu,join_v.shape),join_v,join_z),axis=-1)
    chart=np.concatenate((lower,join,main),axis=0)
    triangles=grid_triangles(len(chart),len(uu)); flat=chart.reshape(-1,3)
    kinds=np.repeat(np.where(np.arange(len(chart)-1)<4,1,0),2*(len(uu)-1)).astype(np.int8)
    baseline_chart=baseline_main.reshape(-1,3); baseline_triangles=grid_triangles(rows,len(uu))
    checks=audit(flat,triangles); baseline_checks=audit(baseline_chart,baseline_triangles)
    checks.update(main_face_uz_max_error_m=float(np.max(abs(main[..., [0,2]]-baseline_main[..., [0,2]]))),
                  main_face_source_barycentric_min=float(weights.min()),
                  toe_join_height_range_m=[float(toe_gap.min()),float(toe_gap.max())],
                  compression_derivative_range=[float(derivative.min()),float(derivative.max())],
                  source_crossings_per_column_range=[min(counts),max(counts)],
                  raw_last_crossing_max_column_jump_m=float(np.max(abs(np.diff(roots)))),
                  envelope_max_column_slope=float(np.max(abs(np.diff(start)/np.diff(uu)))),
                  source_main_radial_span_m=[float((source_top-start).min()),float((source_top-start).max())],
                  authored_main_radial_span_m=[float(radial[-1].min()),float(radial[-1].max())],
                  shoreline_control_error_m=0.,baseline=baseline_checks)
    original_sample=np.sum(original_height.ravel()[indices]*weights,axis=-1)+args.z_offset
    checks['main_face_height_change_from_original_range_m']=[float((Z-original_sample).min()),float((Z-original_sample).max())]
    checks['source_filter']=filter_info
    refmeta=json.loads(str(shore['metadata_json']))
    metadata=dict(schema='directorscut.compressed_escarpment.v1',source_kind='authored_radially_compressed_historical_parent',
        parent_path=str(args.parent.resolve()),parent_sha256=sha(args.parent),
        shore_reference_path=str(args.shore_reference.resolve()),shore_reference_sha256=sha(args.shore_reference),
        generator_sha256=sha(__file__),units='absolute arena metres',physical_tangential_width_m=float(uu[-1]-uu[0]),
        mapping='Vnew=shoreV+joinWidth+(sourceTop-C)*(a*s+(b-a)*s^3/3), s=(Vsource-C)/(sourceTop-C); U/Z fixed AFTER explicitly authored source-height filter',
        contour='Lipschitz majorant of last per-column upward crossings; explicit authored join, not a connected natural contour',
        parameters=dict(z_offset_m=args.z_offset,toe_compression=args.toe_compression,crest_compression=args.crest_compression,
                        join_width_m=args.join_width,envelope_source_slope=args.envelope_slope,source_smoothing_sigma_m=args.source_smoothing_sigma),
        source_height_filter=filter_info,source_height_preserved=args.source_smoothing_sigma==0.,
        source_reconstruction='Original-grid ABD/ADC barycentrics applied to saved authored_parent_height_m after declared filter. Parameter-grid retessellation; interior triangle edges are approximated.',
        source_contour_branches=True,shore_join_authored=True,radial_deformation_authored=True,
        new_erosion_simulated=False,vertical_exaggeration=False,tangential_scaling=False,
        water_mesh_path=refmeta['water_mesh_path'],water_mesh_sha256=refmeta['water_mesh_sha256'],
        water_query_path=refmeta['water_query_path'],water_query_sha256=refmeta['water_query_sha256'],
        metrics=checks,temple_support_preserved=False,old_outer_seams_preserved=False,
        entire_route_verified=False,runtime_integrated=False,visual_accepted=False,
        limitations='Authored source-height smoothing and radial compression alter historical geology; not physical erosion. 6m shore join replaces unavailable continuous source contour. No overhang or exact original-triangle interior preservation claim. Static sampled water reference; no animated-water collision proof.')
    args.output.mkdir(parents=True)
    common=dict(frame_origin_world_m=origin,basis_u_world=basis[:,0],basis_v_world=basis[:,1],basis_z_world=basis[:,2])
    contact={k:shore[k] for k in ('contact_uv_m','contact_water_height_m','lower_overlap_uv_m','lower_overlap_ground_height_m','lower_water_height_m','lower_water_triangle_index','lower_water_barycentric') if k in shore}
    np.savez_compressed(args.output/'candidate.npz',vertices_m=origin+flat@basis.T,triangles=triangles,face_kind=kinds,
        main_vertex_start=np.int64(4*len(uu)),main_source_grid_indices=indices.reshape(-1,3),
        main_source_barycentric=weights.reshape(-1,3),main_source_uv_m=np.stack((U,source_v),axis=-1).reshape(-1,2),
        authored_parent_height_m=height,main_original_source_height_m=original_sample.reshape(-1),
        source_crossing_v_m=roots,authored_source_envelope_v_m=start,metadata_json=np.asarray(json.dumps(metadata)),**common,**contact)
    np.savez_compressed(args.output/'baseline.npz',vertices_m=origin+baseline_chart@basis.T,triangles=baseline_triangles,
        metadata_json=np.asarray(json.dumps(dict(metadata,source_kind='uncompressed_historical_parent_selection',
        comparison_note='Same selected source vertices after declared height filtering, before V compression; rigid V-600/Z-55 placement, no shore apron. Cameras may frame compressed target.'))),**common)
    (args.output/'manifest.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
    shutil.copyfile(__file__,args.output/Path(__file__).name)
    print(json.dumps(checks,indent=2),flush=True)


if __name__=='__main__':main()

