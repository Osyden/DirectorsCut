"""Reproduce the historical uniform-uplift terrain benchmark in physical metres.

Explicit input/dependency/output paths; no original Saved layout is required.
Model constants intentionally remain the reviewed benchmark. This does not
preserve modern arena coast, route, foundations or seams, or certify convergence.
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

TOOLS=Path(__file__).resolve().parent
sys.dont_write_bytecode=True
sys.path.insert(0,str(TOOLS))
from generate_fastscape_crater_sector import masked_diffuse
from generate_weathered_cliff_mesh import original_mesh

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True,
        help='Physical551x251/2m domain template; only frame/XY/valid/coastmask read')
    parser.add_argument('--dependencies',type=Path,required=True,
        help='Isolated fastscapelib0.3.0 --target directory')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    if output.exists() or 'saved' not in [p.lower() for p in output.parts]:
        raise ValueError('Use a new ignored Saved output')
    dependencies=args.dependencies.resolve()
    sys.path.insert(0,str(dependencies))
    import fastscapelib as fs
    if importlib.metadata.version('fastscapelib')!='0.3.0' or not Path(fs.__file__).resolve().is_relative_to(dependencies):
        raise ValueError('Pinned0.3.0 must load from the explicit isolated dependency path')
    source=args.input.resolve()
    with np.load(source,allow_pickle=False) as z:
        data={k:z[k].copy() for k in ('world_xy_m','heightfield_valid_mask','coast_lock_mask','frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world')}
    valid=data['heightfield_valid_mask'].astype(bool)
    band=data['coast_lock_mask'].astype(bool)
    if valid.shape!=(551,251) or band.shape!=valid.shape or np.any(band&~valid) or int(band.sum())!=1311:
        raise ValueError('Exact benchmark requires551x251 domain and1311 valid historical coastal-band nodes')
    row,col=np.indices(valid.shape)
    expected=data['frame_origin_world_m'][:2]+2.*col[...,None]*data['basis_u_world'][:2]+2.*row[...,None]*data['basis_v_world'][:2]
    if not np.isfinite(data['world_xy_m']).all() or not np.allclose(data['world_xy_m'],expected,rtol=0.,atol=1.e-8):
        raise ValueError('Input must retain physical2m XY spacing and its stated rigid frame')
    fixed=band|~valid
    active=valid&~fixed
    rng=np.random.Generator(np.random.PCG64(1234))
    initial=-150.+rng.uniform(0.,1.,valid.shape)
    initial[fixed]=-150.
    z=initial.copy()
    rate=np.where(active,.001,0.)
    nodes={tuple(map(int,p)):fs.NodeStatus.FIXED_VALUE for p in np.argwhere(fixed)}
    grid=fs.RasterGrid(list(z.shape),[2.,2.],fs.NodeStatus.CORE,nodes)
    graph=fs.FlowGraph(grid,[fs.SingleFlowRouter(),fs.MSTSinkResolver()])
    graph.mask=~valid
    graph.base_levels=np.flatnonzero(band).tolist()
    spl=fs.SPLEroder(graph,k_coef=np.where(active,2.e-4,0.),area_exp=.4,slope_exp=1.,tolerance=1.e-5)
    area=np.zeros_like(z)
    cut_total=np.zeros_like(z)
    diff_total=np.zeros_like(z)
    ledger=dict(boundary_exchange_m3=0.,ledger_residual_m3=0.,max_outgoing_weight=0.,substeps=0)
    checkpoints=[]
    prior=z.copy();prior_cut=0.;streak=0;converged=False
    started=time.perf_counter()
    dt=500.
    for step in range(1,501):
        lifted=z+dt*rate
        graph.update_routes(lifted)
        # Rainfall only on dry active land; wet receiving band does not add it.
        graph.accumulate(area,active.astype(float))
        cut=spl.erode(lifted,area,dt)
        eroded=lifted-cut
        z,entry=masked_diffuse(eroded,valid,fixed,2.,dt,.001)
        cut_total+=cut
        diff_total+=eroded-z
        for k in ('boundary_exchange_m3','ledger_residual_m3','substeps'):ledger[k]+=entry[k]
        ledger['max_outgoing_weight']=max(ledger['max_outgoing_weight'],entry['max_outgoing_weight'])
        if not np.array_equal(z[fixed],initial[fixed]) or not np.isfinite(z).all():raise ValueError('Invalid fixed/finite terrain')
        if step%50==0:
            change=np.abs(z-prior)[active]/(50.*dt)
            volume=float(cut_total.sum())*4.
            maxrate=float(change.max())
            record=dict(step=step,model_years=step*dt,height_range_m=[float(z[valid].min()),float(z[valid].max())],
                relief_m=float(np.ptp(z[valid])),max_change_rate_m_per_year=maxrate,
                median_change_rate_m_per_year=float(np.median(change)),
                max_change_over_uplift=maxrate/.001,
                interval_spl_removed_m3=volume-prior_cut,
                interval_uplift_m3=float(rate.sum())*50*dt*4,
                elapsed_seconds=time.perf_counter()-started)
            checkpoints.append(record);print(json.dumps(record),flush=True)
            streak=streak+1 if maxrate<.05*.001 else 0
            prior=z.copy();prior_cut=volume
            if streak>=2:converged=True;break
            if time.perf_counter()-started>240.:break
    graph.update_routes(z);graph.accumulate(area,active.astype(float))
    outarea=area[band]
    total_uplift=float(rate.sum())*step*dt*4.
    removed=float(cut_total.sum())*4.
    net=float((z-initial)[active].sum())*4.
    meta=dict(schema='directorscut.uniform_parent_benchmark.v1',source_kind='historical_uniform_uplift_benchmark',
        source_geometry_template=str(source),source_geometry_template_sha256=sha(source),generator_sha256=sha(__file__),
        dependency_path=str(dependencies),numpy_version=np.__version__,python_version=sys.version,
        model_comparison='Historical datum and uniform uplift both differ from prior authored-parent trials; not an isolated single-variable causal comparison',
        copied_template_fields=list(data),initial_height_reused=False,gaussian_forcing_used=False,
        physical_domain_uv_m=[[0.,500.],[0.,1100.]],cell_size_m=2.,crop_index_range_exclusive=[[0,253],[0,251]],
        historical_datum_m=-150.,historical_receiving_band_nodes=int(band.sum()),
        coastal_baselevel_policy='All1311 flat historical wetshore band nodes; not original251modern external outlets',
        rainfall_policy='Active dryland only; excludes wet receiving band and invalid padding',
        boundary_policy='Historical band fixed; invalidmasked; mobileclosedside/back boundaries',
        physical_valid_domain_area_m2=float(valid.sum()*4),active_dryland_area_m2=float(active.sum()*4),
        settings=dict(seed=1234,initial_perturbation_m=1.,uniform_uplift_m_per_year=.001,k_coef=2.e-4,
            area_exp=.4,slope_exp=1.,diffusivity=.001,router='Single/MST',steps=step,dt_years=dt,
            model_years=step*dt,convergence='Two successive50step windows with maximum height rate<5%ofuniformuplift'),
        converged_by_declared_criterion=converged,checkpoints=checkpoints,diffusion_ledger=ledger,
        metrics=dict(final_height_range_m=[float(z[valid].min()),float(z[valid].max())],
            physical_relief_m=float(np.ptp(z[valid])),uplift_m3=total_uplift,spl_removed_m3=removed,
            combined_ledger_residual_m3=net-total_uplift+removed-ledger['boundary_exchange_m3'],
            largest_baselevel_dryland_area_m2=float(outarea.max()),
            largest_baselevel_fraction_of_dryland=float(outarea.max()/(active.sum()*4)),
            nonzero_receiving_band_nodes=int((outarea>0).sum())),
        upstream_version='0.3.0',upstream_license='GPL-3.0',elapsed_seconds=time.perf_counter()-started,
        current_coast_preserved=False,current_route_preserved=False,temple_support_preserved=False,old_seams_preserved=False,
        authored_historical_geology=True,runtime_integrated=False,visual_accepted=False,
        limitations='No calibrated geologic time or moderncoast/support claim. Heightfield process benchmark, no3D joints/overhangs or PBR sediment. No lateral halo; closed sides can affect drainage.')
    output.mkdir(parents=True)
    arrays=dict(**data,height_m=z,initial_height_m=initial,uplift_rate_m_per_year=rate,cell_size_m=np.asarray(2.),
        fixed_mask=fixed,baselevel_mask=band,flow_accumulation=area,stream_power_lowering_m=cut_total,
        diffusion_signed_lowering_m=diff_total,metadata_json=np.asarray(json.dumps(meta)))
    np.savez_compressed(output/'terrain.npz',**arrays)
    frame={k:data[k] for k in ('frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world')}
    for scope in ('parent','crop'):
        folder=output/scope;folder.mkdir()
        view=arrays if scope=='parent' else {k:(v[:253] if isinstance(v,np.ndarray) and v.shape[:2]==z.shape else v) for k,v in arrays.items()}
        for name,h in [('baseline',view['initial_height_m']),('candidate',view['height_m'])]:
            verts,tris,_,_=original_mesh(dict(view,height_m=h))
            np.savez_compressed(folder/(name+'.npz'),vertices_m=verts,triangles=tris,**frame,
                metadata_json=np.asarray(json.dumps(dict(meta,scope=scope,variant=name))))
    (output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    shutil.copyfile(__file__,output/'generator_source.py')
    # Keep the exact project helpers under their import names so the archived
    # CLI can be replayed from this bundle with new explicit argument paths.
    for name in ('generate_fastscape_crater_sector.py','generate_massif_cliff_seed.py',
                 'generate_weathered_cliff_mesh.py','review_eroded_cliff_sector.py'):
        shutil.copyfile(TOOLS/name,output/name)
    shutil.copyfile(source,output/'domain_template.npz')
    shutil.copyfile(dependencies/'fastscapelib-0.3.0.dist-info/licenses/LICENSE',output/'LICENSE.fastscapelib')
    (output/'ATTRIBUTION.txt').write_text('Offline authored uniformuplift model using external Fastscapelib0.3.0 API, GPL3.\n'
        'https://github.com/fastscape-lem/fastscapelib at707fcc10976b0e729f1415fd16b0a161e1f746c3\n'
        'Not unchanged officialexample; not modernarena geometry; no Unreal dependency.\n',encoding='utf-8')
    print(json.dumps(dict(output=str(output),metrics=meta['metrics'],converged=converged),indent=2),flush=True)

if __name__=='__main__':main()
