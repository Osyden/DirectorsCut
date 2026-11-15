"""Fresh coastal parent terrain: broad divides, uplift and branching incision.

Offline proposal in metres. Existing source08 contributes only exact XY/domain
and measured coastal targets. Added inland terrain is authored, not surveyed.
No old outer seam/pad locks, imposed cliff step, mesh rescaling or pixel cap.
External Fastscapelib0.3.0 is GPL3; never embedded in Unreal.
"""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import shutil
import sys
import time

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_fastscape_crater_sector import masked_diffuse
from generate_massif_cliff_seed import screened_fit, polygon_distance
from generate_weathered_cliff_mesh import original_mesh


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def polyline_distance(uv, points):
    points = np.asarray(points, dtype=float)
    best = np.full(uv.shape[:-1], np.inf)
    for a, b in zip(points[:-1], points[1:]):
        edge = b-a
        t = np.clip(np.sum((uv-a)*edge, axis=-1)/np.dot(edge, edge), 0., 1.)
        best = np.minimum(best, np.linalg.norm(uv-a-t[..., None]*edge, axis=-1))
    return best


def make_parent(source, measured, seed):
    cell = float(source['cell_size_m'])
    if cell != 2. or source['height_m'].shape != (253,251):
        raise ValueError('This bounded proof requires the physical source08 2m grid')
    u, v = np.meshgrid(np.arange(251)*cell, np.arange(551)*cell)
    xy = source['frame_origin_world_m'][:2]+u[...,None]*source['basis_u_world'][:2]+v[...,None]*source['basis_v_world'][:2]
    xy[:253] = source['world_xy_m']
    valid = np.ones(u.shape, dtype=bool)
    valid[:253] = source['heightfield_valid_mask'] > 0
    authored = np.zeros(u.shape, dtype=bool)
    authored[253:] = True
    outlets = np.zeros(u.shape, dtype=bool)
    outlets[:253] = source['outflow_mask'] > 0
    coast = np.zeros(u.shape, dtype=bool)
    coast[:253] = (source['constraint_type'].astype(np.uint8)&4) != 0
    # Include every original triangle touching measured coastal locks. This
    # one-cell collar preserves actual source contact geometry, not old pads.
    padded = np.pad(coast,1)
    collar = np.zeros_like(coast)
    for dy in range(3):
        for dx in range(3):
            collar |= padded[dy:dy+len(coast),dx:dx+coast.shape[1]]
    coast_lock = collar & valid
    fixed = coast_lock | ~valid
    rows = np.argmax(outlets,axis=0)
    cols = np.arange(u.shape[1])
    actual_z = measured['height_m'][rows,cols]
    knots = np.linspace(0.,500.,11)
    levels = [np.median(actual_z[np.abs(u[0]-k)<=30.]) for k in knots]
    coast_level = np.interp(u[0],knots,levels)[None,:]
    coast_v = source['v_m'][rows].astype(float)
    inland = np.maximum(0.,v-coast_v[None,:])
    rng = np.random.Generator(np.random.PCG64(seed))
    raw = coast_level+.015*inland+rng.uniform(0.,.8,u.shape)
    target = raw.astype(np.float32)
    target[:253][fixed[:253]] = measured['height_m'][fixed[:253]]
    initial, correction, fit = screened_fit(raw,target,valid,fixed,cell,
        fit_length_m=12.,max_iterations=1600)
    uv = np.stack((u,v),axis=-1)
    main = [[92.,205.],[146.,440.],[232.,695.],[302.,1040.]]
    branch = [[432.,165.],[391.,335.],[312.,512.],[232.,695.]]
    dmain = polyline_distance(uv,main)
    dbranch = polyline_distance(uv,branch)
    # Unequal broad tectonic/authoring forcing, not ridges stamped into height.
    # The secondary divide merges into the primary in the same terrain field.
    forcing = np.maximum(np.exp(-(dmain/105.)**2), .72*np.exp(-(dbranch/76.)**2))
    uplift = .002+.010*forcing
    uplift[fixed] = 0.
    return dict(u=u,v=v,xy=xy,valid=valid,authored=authored,outlets=outlets,
        coast_lock=coast_lock,fixed=fixed,target=target,raw=raw,initial=initial.astype(float),
        uplift=uplift,correction=correction,fit=fit,divides=[main,branch])


def evolve(fs, fields, steps, dt, diffusivity, padding_offset=0., routing='multi'):
    z=fields['initial'].copy()
    valid,fixed=fields['valid'],fields['fixed']
    z[~valid]+=padding_offset
    target=z.copy()
    nodes={tuple(map(int,rc)):fs.NodeStatus.FIXED_VALUE for rc in np.argwhere(fixed)}
    grid=fs.RasterGrid(list(z.shape),[2.,2.],fs.NodeStatus.CORE,nodes)
    operators=[fs.SingleFlowRouter(),fs.MSTSinkResolver()]
    if routing=='multi':operators.append(fs.MultiFlowRouter(1.1))
    graph=fs.FlowGraph(grid,operators)
    graph.mask=~valid
    graph.base_levels=np.flatnonzero(fields['outlets']).tolist()
    spl=fs.SPLEroder(graph,k_coef=np.where(valid&~fixed,1.e-4,0.),area_exp=.4,slope_exp=1.,tolerance=1.e-5)
    area=np.zeros_like(z)
    cut_total=np.zeros_like(z)
    diffusion_total=np.zeros_like(z)
    ledger=dict(boundary_exchange_m3=0.,ledger_residual_m3=0.,max_outgoing_weight=0.,substeps=0)
    for _ in range(steps):
        lifted=z+dt*fields['uplift']
        graph.update_routes(lifted)
        graph.accumulate(area,valid.astype(float))
        cut=spl.erode(lifted,area,dt)
        eroded=lifted-cut
        z,entry=masked_diffuse(eroded,valid,fixed,2.,dt,diffusivity)
        cut_total+=cut
        diffusion_total+=eroded-z
        for key in ('boundary_exchange_m3','ledger_residual_m3','substeps'):
            ledger[key]+=entry[key]
        ledger['max_outgoing_weight']=max(ledger['max_outgoing_weight'],entry['max_outgoing_weight'])
        if not np.array_equal(z[fixed],target[fixed]) or not np.isfinite(z).all():
            raise ValueError('Model violated fixed coastal values or finite terrain')
    graph.update_routes(z)
    graph.accumulate(area,valid.astype(float))
    return z,area,cut_total,diffusion_total,ledger


def export_mesh(path,data,height,frame,meta):
    vertices,triangles,_,_=original_mesh(dict(data,height_m=height))
    np.savez_compressed(path,vertices_m=vertices,triangles=triangles,**frame,
        metadata_json=np.asarray(json.dumps(meta)))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path,required=True)
    p.add_argument('--boundary-source',type=Path,required=True)
    p.add_argument('--dependencies',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--steps',type=int,default=100)
    p.add_argument('--dt-years',type=float,default=100.)
    p.add_argument('--seed',type=int,default=1234)
    p.add_argument('--uplift-rate-scale',type=float,default=1.)
    p.add_argument('--routing',choices=('multi','single'),default='multi')
    args=p.parse_args()
    if args.output.exists() or 'saved' not in [x.lower() for x in args.output.resolve().parts]:
        p.error('Use a new ignored Saved output directory')
    if not 1<=args.steps<=400 or not 0<args.dt_years<=200 or not 0<args.uplift_rate_scale<=1.:
        p.error('Bounded proof allows1..400 steps, dt<=200years and uplift-rate-scale in(0,1]')
    sys.path.insert(0,str(args.dependencies.resolve()))
    import fastscapelib as fs
    if importlib.metadata.version('fastscapelib')!='0.3.0' or not Path(fs.__file__).resolve().is_relative_to(args.dependencies.resolve()):
        raise ValueError('Requires isolated Fastscapelib0.3.0')
    with np.load(args.input,allow_pickle=False) as z:source={k:z[k].copy() for k in z.files}
    with np.load(args.boundary_source,allow_pickle=False) as z:measured={k:z[k].copy() for k in z.files}
    for key in ('world_xy_m','heightfield_valid_mask','cell_size_m','frame_origin_world_m','basis_u_world','basis_v_world','basis_up_world'):
        if not np.array_equal(source[key],measured[key]):raise ValueError(f'Frame mismatch:{key}')
    started=time.perf_counter()
    fields=make_parent(source,measured,args.seed)
    fields['uplift']*=args.uplift_rate_scale
    valid,fixed=fields['valid'],fields['fixed']
    probe=evolve(fs,fields,3,args.dt_years,.005,routing=args.routing)[0]
    changed=evolve(fs,fields,3,args.dt_years,.005,1000.,routing=args.routing)[0]
    error=float(np.max(np.abs(probe[valid]-changed[valid])))
    if error>1.e-10:raise ValueError(f'Invalid padding coupling:{error}')
    final,area,cut,diff,ledger=evolve(fs,fields,args.steps,args.dt_years,.005,routing=args.routing)
    active=valid&~fixed
    original_crop=source['height_m'].astype(float)
    crop=final[:253]
    support=[]
    anchors=json.loads(str(source['anchors_json']))
    for anchor in anchors['anchors']:
        inside=(polygon_distance(source['world_xy_m'],anchor['footprint_xy_m'])<1.e-6)&valid[:253]
        if inside.any():
            delta=crop[inside]-anchor['support_height_m']
            support.append(dict(id=anchor['id'],sampled_nodes=int(inside.sum()),
                height_relative_support_range_m=[float(delta.min()),float(delta.max())],full_footing_support_verified=False))
    meta=dict(schema='directorscut.fastscape_parent_landform.v1',source_kind='fastscape_parent_landform_proposal',
        input=str(args.input.resolve()),input_sha256=sha(args.input),boundary_source=str(args.boundary_source.resolve()),
        boundary_source_sha256=sha(args.boundary_source),generator_sha256=sha(__file__),
        helper_sha256=sha(Path(__file__).with_name('generate_fastscape_crater_sector.py')),
        fastscapelib_version='0.3.0',upstream_license='GPL-3.0',
        upstream_reference_revision='707fcc10976b0e729f1415fd16b0a161e1f746c3',
        units='absolute arena metres, physical2m grid, no mesh rescaling/vertical exaggeration',
        parent_domain_uv_m=[[0.,500.],[0.,1100.]],crop_index_range_exclusive=[[0,253],[0,251]],
        boundary_policy='Only actual251 coastal outlets are flow baselevels. Measured coast+onecell triangle collar fixed. Side/back closed mobile boundaries; no inherited tile seams/pads.',
        extra_domain='Rows253..550 are authored inland proposal data, not measured or support-certified',
        settings=dict(steps=args.steps,dt_years=args.dt_years,model_years=args.steps*args.dt_years,seed=args.seed,
            initial_grade=.015,initial_perturbation_m=.8,uplift_range_m_per_year=[.002*args.uplift_rate_scale,.012*args.uplift_rate_scale],
            uplift_rate_scale=args.uplift_rate_scale,integrated_uplift_reference_years=args.steps*args.dt_years*args.uplift_rate_scale,
            divide_polylines_uv_m=fields['divides'],
            divide_widths_m=[105.,76.],secondary_forcing_ratio=.72,k_coef=1.e-4,area_exp=.4,slope_exp=1.,diffusivity=.005,
            flow_router=('Single/MST/Multi1.1' if args.routing=='multi' else 'Single/MST'),fit=fields['fit']),
        metrics=dict(valid_nodes=int(valid.sum()),authored_nodes=int(fields['authored'].sum()),fixed_coast_nodes=int(fields['coast_lock'].sum()),
            initial_height_range_m=[float(fields['initial'][valid].min()),float(fields['initial'][valid].max())],
            parent_height_range_m=[float(final[valid].min()),float(final[valid].max())],
            crop_height_range_m=[float(crop[valid[:253]].min()),float(crop[valid[:253]].max())],
            parent_height_percentiles_m=np.percentile(final[valid],[5,25,50,75,95,99]).tolist(),
            crop_delta_from_old_percentiles_m=np.percentile((crop-original_crop)[valid[:253]],[0,5,50,95,100]).tolist(),
            source_crop_final_toprow_range_m=[float(crop[-1].min()),float(crop[-1].max())],
            padding_1000m_probe_error_m=error,fixed_bitwise_preserved=True),
        diffusion_ledger=ledger,anchors=support,elapsed_seconds=time.perf_counter()-started,
        old_interior_used=False,old_outer_seams_preserved=False,temple_support_preserved=False,
        entire_arena_route_verified=False,visual_accepted=False,runtime_integrated=False,
        limitations='Authored uplift/process experiment, not calibrated tectonic time. Heightfield, no3D fracture/overhang claim. No-flux lateral boundaries constrain catchments. No transported stream sediment/PBR claim.')
    args.output.mkdir(parents=True)
    frame={k:source[k] for k in ('frame_origin_world_m','basis_u_world','basis_v_world')}
    frame['basis_z_world']=source['basis_up_world']
    arrays=dict(world_xy_m=fields['xy'],heightfield_valid_mask=valid,cell_size_m=np.asarray(2.),
        height_m=final,initial_height_m=fields['initial'],fresh_raw_height_m=fields['raw'],uplift_rate_m_per_year=fields['uplift'],
        coast_lock_mask=fields['coast_lock'],fixed_mask=fixed,coast_target_height_m=fields['target'],
        outflow_mask=fields['outlets'],authored_domain_mask=fields['authored'],flow_accumulation=area,
        stream_power_lowering_m=cut,diffusion_signed_lowering_m=diff,**frame,metadata_json=np.asarray(json.dumps(meta)))
    np.savez_compressed(args.output/'parent_terrain.npz',**arrays)
    for scope in ('parent','crop'):
        folder=args.output/scope;folder.mkdir()
        view=arrays if scope=='parent' else {k:(value[:253] if isinstance(value,np.ndarray) and value.shape[:2]==final.shape else value) for k,value in arrays.items()}
        if scope=='crop':np.savez_compressed(args.output/'crop_terrain.npz',**view)
        for name,h in [('baseline',view['initial_height_m']),('candidate',view['height_m'])]:
            export_mesh(folder/(name+'.npz'),view,h,frame,dict(meta,scope=scope,variant=name))
    shutil.copyfile(__file__,args.output/'generator_source.py')
    shutil.copyfile(Path(__file__).with_name('generate_fastscape_crater_sector.py'),args.output/'masked_diffusion_helper.py')
    shutil.copyfile(args.dependencies/'fastscapelib-0.3.0.dist-info/licenses/LICENSE',args.output/'LICENSE.fastscapelib')
    (args.output/'ATTRIBUTION.txt').write_text('Offline API model using Fastscapelib0.3.0 by its developers, GPL3.\n'
        'https://github.com/fastscape-lem/fastscapelib at707fcc10976b0e729f1415fd16b0a161e1f746c3\n'
        'New authored physical parent model; not unchanged upstream notebook. No Unreal dependency.\n',encoding='utf-8')
    (args.output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(output=str(args.output.resolve()),metrics=meta['metrics'],elapsed_seconds=meta['elapsed_seconds']),indent=2))


if __name__=='__main__':main()
