"""Reproduce the pinned official Fastscapelib escarpment, without adaptation.

This wrapper executes the unmodified computational cells of the upstream
notebook and exports physical arrays/triangles. Plotting cells alone are omitted.
The notebook and Fastscapelib are GPL-3.0; preserve their full downloaded LICENSE
and upstream attribution. Neither solver nor notebook is embedded in the game.
No network, package installation, crater fitting or hidden scale change here.
"""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import shutil
import sys
import time

sys.dont_write_bytecode=True
UPSTREAM_REVISION='707fcc10976b0e729f1415fd16b0a161e1f746c3'
NOTEBOOK_SHA='4df4a91a2560d3c821c1e8841695b0a229a1cd7e367b7c110ba81b58b039da49'
LICENSE_SHA='8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c545eb65b903'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dependencies',type=Path,required=True)
    parser.add_argument('--notebook',type=Path,required=True)
    parser.add_argument('--license',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists() or 'saved' not in [x.lower() for x in args.output.resolve().parts]:
        parser.error('Choose a new ignored Saved output directory')
    if sha(args.notebook)!=NOTEBOOK_SHA or sha(args.license)!=LICENSE_SHA:
        parser.error('Exact reviewed upstream0.3.0 notebook and licence hashes required')
    sys.path.insert(0,str(args.dependencies.resolve()))
    import numpy as np
    import fastscapelib as fs
    if importlib.metadata.version('fastscapelib')!='0.3.0':
        raise ValueError('This reproduction requires pinned fastscapelib0.3.0')
    if not Path(fs.__file__).resolve().is_relative_to(args.dependencies.resolve()):
        raise ValueError('Fastscapelib must load from the isolated dependency folder')
    notebook=json.loads(args.notebook.read_text(encoding='utf-8'))
    # These cells were read at the exact source commit. No model line is
    # rewritten, no timestep/parameter/grid/unit is changed by the wrapper.
    cells=[4,6,8,11,14]
    namespace={'fs':fs,'np':np}
    started=time.perf_counter()
    for index in cells:
        cell=notebook['cells'][index]
        if cell['cell_type']!='code':raise ValueError('Pinned notebook cell layout changed')
        exec(compile(''.join(cell['source']),f'{args.notebook}:cell{index}','exec'),namespace)
    elapsed=time.perf_counter()-started
    initial=namespace['init_elevation'].copy();final=namespace['elevation'].copy()
    area_at_last_step_input=namespace['drainage_area'].copy()
    # Preserve the published final drainage array exactly. Separately compute
    # final-state routing as a labelled diagnostic after the unchanged model.
    graph=namespace['flow_graph'];graph.update_routes(final)
    final_area=np.empty_like(final);graph.accumulate(final_area,1.)
    if not np.isfinite(initial).all() or not np.isfinite(final).all() or final.shape!=(101,201):
        raise ValueError('Reference model returned invalid geometry')
    rows,cols=final.shape
    yy,xx=np.meshgrid(np.arange(rows)*100.,np.arange(cols)*100.,indexing='ij')
    indices=np.arange(rows*cols).reshape(rows,cols)
    a=indices[:-1,:-1].ravel();b=a+1;c=a+cols;d=c+1
    triangles=np.concatenate((np.stack((a,b,d),axis=1),np.stack((a,d,c),axis=1))).astype(np.int32)
    change=final-initial
    metrics=dict(shape=list(final.shape),cell_size_m=100.,extent_xy_m=[20000.,10000.],
        initial_height_range_m=[float(initial.min()),float(initial.max())],
        final_height_range_m=[float(final.min()),float(final.max())],
        net_change_range_m=[float(change.min()),float(change.max())],
        net_change_percentiles_m=np.percentile(change,[1,5,50,95,99]).tolist(),
        cells_lowered_over1m=int((change < -1.).sum()),cells_lowered_over10m=int((change < -10.).sum()),
        left_baselevel_maximum_change_m=float(np.abs(change[:,0]).max()),
        array_sha256={name:hashlib.sha256(np.ascontiguousarray(value).tobytes()).hexdigest()
            for name,value in [('initial_height_m',initial),('final_height_m',final),('upstream_drainage_area',area_at_last_step_input)]},
        model_elapsed_seconds=elapsed)
    meta=dict(schema='directorscut.upstream_fastscape_escarpment.v1',source_kind='upstream_fastscape_reference',
        upstream_repository='https://github.com/fastscape-lem/fastscapelib',upstream_revision=UPSTREAM_REVISION,
        upstream_notebook='doc/source/examples/escarpment_py.ipynb',notebook_sha256=NOTEBOOK_SHA,
        upstream_license='GPL-3.0',license_sha256=LICENSE_SHA,executed_unmodified_cells=cells,
        omitted_cells='Imports are supplied directly; matplotlib setup/plotting/empty cells only are skipped',
        dependency_version='0.3.0',dependency_path=str(Path(fs.__file__).resolve()),numpy_version=np.__version__,
        python_version=sys.version,wrapper_sha256=sha(__file__),metrics=metrics,
        physical_units='metres; native20km by10km upstream domain,100m grid,400m initial step',
        boundary_conditions='left FIXED_VALUE;right CORE;top/bottom LOOPED, exactly upstream code',
        model_settings=dict(seed=1234,steps=50,dt_years=2000,simulation_years=100000,
            k_coef=1.e-4,area_exp=.4,slope_exp=1,tolerance=1.e-5,diffusivity=.01,multiflow_exponent=1.1),
        physical_rescaling=False,vertical_exaggeration=False,crater_adapted=False,arena_integrated=False,
        scientific_transport_masks=False,visual_accepted=False,
        limitations='Independent upstream macro-process reproduction, not game-ready cliff, calibrated real catchment, fracture model, PBR asset or preserved arena supports.')
    args.output.mkdir(parents=True)
    shutil.copyfile(args.notebook,args.output/'upstream_escarpment_py.ipynb')
    shutil.copyfile(args.license,args.output/'LICENSE.fastscapelib')
    (args.output/'ATTRIBUTION.txt').write_text('Fastscapelib developers — official Escarpment [Py] example.\n'
        'Repository: https://github.com/fastscape-lem/fastscapelib\nRevision: '+UPSTREAM_REVISION+'\n'
        'GPL-3.0; full licence accompanies this artifact. Model cells are unmodified.\n',encoding='utf-8')
    np.savez_compressed(args.output/'terrain_arrays.npz',initial_height_m=initial,final_height_m=final,
        upstream_drainage_area_m2=area_at_last_step_input,final_rerouted_drainage_area_m2=final_area,
        cell_size_m=np.asarray(100.),x_m=np.arange(cols)*100.,y_m=np.arange(rows)*100.,metadata_json=np.asarray(json.dumps(meta)))
    frame=dict(frame_origin_world_m=np.zeros(3),basis_u_world=np.array([1.,0.,0.]),
        basis_v_world=np.array([0.,1.,0.]),basis_z_world=np.array([0.,0.,1.]))
    for name,height in [('baseline',initial),('candidate',final)]:
        vertices=np.c_[xx.ravel(),yy.ravel(),height.ravel()]
        np.savez_compressed(args.output/(name+'.npz'),vertices_m=vertices,triangles=triangles,
            **frame,metadata_json=np.asarray(json.dumps(dict(meta,variant=name))))
    (args.output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(output=str(args.output.resolve()),metrics=metrics),indent=2))


if __name__=='__main__':main()
