"""Fresh physical source08 escarpment using isolated Fastscapelib 0.3.0.

Offline proposal only. The measured source contributes XY, domain, coast and
outer seam values, never its old interior terrain. Temple support is deliberately
NOT frozen or claimed. Fastscapelib is external GPL-3.0; see ThirdParty notice.
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
from generate_massif_cliff_seed import screened_fit, polygon_distance
from generate_weathered_cliff_mesh import original_mesh


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def masked_diffuse(height, valid, fixed, cell_m, dt_years, diffusivity):
    """Conservative valid-edge linear hillslope flux, fixed Dirichlet reservoir.

    Positive boundary_exchange_m3 means material enters the active domain from
    fixed nodes. Invalid edges do not exist. Substep sum of conductance weights
    is at most .8, so every active update is a convex combination of neighbours
    and itself: no new extrema. This is not transported stream sediment.
    """
    if diffusivity <= 0.:
        return height.copy(), dict(boundary_exchange_m3=0., ledger_residual_m3=0.,
            max_outgoing_weight=0., invalid_edge_count=0, substeps=0)
    edge_x = valid[:, :-1] & valid[:, 1:]
    edge_y = valid[:-1, :] & valid[1:, :]
    count = max(1, int(np.ceil(dt_years*diffusivity/(.2*cell_m*cell_m))))
    weight = dt_years/count*diffusivity/(cell_m*cell_m)
    active = valid & ~fixed
    z = height.copy()
    exchange = 0.
    initial_sum = float(np.sum(z[active], dtype=np.float64))*cell_m*cell_m
    for _ in range(count):
        change = np.zeros_like(z)
        fx = np.where(edge_x, weight*(z[:, 1:]-z[:, :-1]), 0.)
        fy = np.where(edge_y, weight*(z[1:, :]-z[:-1, :]), 0.)
        change[:, :-1] += fx
        change[:, 1:] -= fx
        change[:-1, :] += fy
        change[1:, :] -= fy
        exchange -= float(np.sum(change[fixed & valid], dtype=np.float64))*cell_m*cell_m
        z[active] += change[active]
    measured_change = float(np.sum(z[active], dtype=np.float64))*cell_m*cell_m-initial_sum
    degree = np.zeros_like(z, dtype=np.int8)
    degree[:, :-1] += edge_x
    degree[:, 1:] += edge_x
    degree[:-1, :] += edge_y
    degree[1:, :] += edge_y
    return z, dict(boundary_exchange_m3=exchange,
        ledger_residual_m3=measured_change-exchange,
        max_outgoing_weight=float(degree[active].max(initial=0))*weight,
        invalid_edge_count=0, substeps=count)


def fresh_field(source, measured, seed):
    valid = source['heightfield_valid_mask'] > 0
    xy = source['world_xy_m'].astype(float)
    q = xy-source['frame_origin_world_m'][:2]
    u = q @ source['basis_u_world'][:2]
    v = q @ source['basis_v_world'][:2]
    clearance = source['shore_clearance_m'].astype(float)
    crosses = (clearance[:-1] >= 0) & (clearance[1:] < 0)
    if not np.all(crosses.sum(axis=0) == 1):
        raise ValueError('Expected one actual receiving-water crossing per column')
    rows = np.argmax(crosses, axis=0)
    cols = np.arange(xy.shape[1])
    a, b = clearance[rows, cols], clearance[rows+1, cols]
    coast_v = v[rows, cols]+a/(a-b)*(v[rows+1, cols]-v[rows, cols])
    outlet_rows = np.argmax(source['outflow_mask'] > 0, axis=0)
    coast_actual = measured['height_m'][outlet_rows, cols].astype(float)
    knots = np.linspace(0., float(u.max()), 11)
    levels = [np.median(coast_actual[np.abs(u[0]-k) <= 30.]) for k in knots]
    coast = np.interp(u[0], knots, levels)[None, :]
    inland = v-coast_v[None, :]
    # A fresh broad scarp, already spanning the actual low coast to rim datum.
    # These two long lobes only vary where the initial front lies. Erosion,
    # rather than pre-cut slots or the old mountain field, forms its drainage.
    front = 48.+13.*np.cos((u-80.)*2*np.pi/330.)+7.*np.sin(u*2*np.pi/210.)
    t = np.clip((inland-front)/56., 0., 1.)
    rise = t*t*(3.-2.*t)
    plateau_level = float(np.median(measured['height_m'][-1]))
    plateau = plateau_level+.018*(u-250.)+.012*(v-350.)
    rng = np.random.Generator(np.random.PCG64(seed))
    raw = coast+np.maximum(0., inland)*.035+(plateau-coast)*rise
    raw += rng.uniform(0., .8, raw.shape)
    raw[~valid] = measured['height_m'][~valid]
    # Existing boundary bands only: the old temple bit is explicitly removed.
    kinds = source['constraint_type'].astype(np.uint8) & np.uint8(7)
    fixed = (kinds != 0)
    target = measured['height_m'].astype(np.float32)
    initial, correction, fit = screened_fit(raw, target, valid, fixed,
        float(source['cell_size_m']), fit_length_m=18., max_iterations=1800)
    return initial.astype(float), raw, fixed, kinds, correction, dict(
        plateau_datum_m=plateau_level, initial_front_m=[28., 68.],
        scarp_rise_width_m=56., initial_perturbation_m=.8,
        shoreline_coordinate='measured continuous receiving-water zero crossing', fit=fit)


def solve(fs, initial, valid, fixed, outlets, spacing, steps, dt, diffusion, padding_offset=0.):
    z = initial.copy()
    z[~valid] += padding_offset
    target = z.copy()
    nodes = {tuple(map(int, rc)): fs.NodeStatus.FIXED_VALUE for rc in np.argwhere(fixed)}
    grid = fs.RasterGrid(list(z.shape), [spacing, spacing], fs.NodeStatus.FIXED_VALUE, nodes)
    graph = fs.FlowGraph(grid, [fs.SingleFlowRouter(), fs.MSTSinkResolver(), fs.MultiFlowRouter(1.1)])
    graph.mask = ~valid
    graph.base_levels = np.flatnonzero(outlets).tolist()
    erodibility = np.where(valid & ~fixed, 1.e-4, 0.)
    spl = fs.SPLEroder(graph, k_coef=erodibility, area_exp=.4, slope_exp=1., tolerance=1.e-5)
    area = np.zeros_like(z)
    cumulative_spl = np.zeros_like(z)
    cumulative_diff = np.zeros_like(z)
    max_fixed_drift = 0.
    diffusion_ledger = dict(boundary_exchange_m3=0., ledger_residual_m3=0.,
        max_outgoing_weight=0., invalid_edge_count=0, substeps=0)
    for _ in range(steps):
        graph.update_routes(z)
        graph.accumulate(area, valid.astype(float))
        cut = spl.erode(z, area, dt)
        next_z = z-cut
        diffused, ledger = masked_diffuse(next_z, valid, fixed, spacing, dt, diffusion)
        spread = next_z-diffused
        next_z = diffused
        for key in ('boundary_exchange_m3', 'ledger_residual_m3', 'substeps'):
            diffusion_ledger[key] += ledger[key]
        diffusion_ledger['max_outgoing_weight'] = max(diffusion_ledger['max_outgoing_weight'], ledger['max_outgoing_weight'])
        max_fixed_drift = max(max_fixed_drift, float(np.max(np.abs(next_z[fixed]-target[fixed]))))
        if max_fixed_drift > 1.e-9:
            raise ValueError(f'Solver moved fixed boundaries by {max_fixed_drift} m')
        z = next_z
        cumulative_spl += cut
        cumulative_diff += spread
    graph.update_routes(z)
    graph.accumulate(area, valid.astype(float))
    if not np.isfinite(z).all() or not np.array_equal(z[fixed], target[fixed]):
        raise ValueError('Nonfinite terrain or changed boundary')
    return z, area.copy(), cumulative_spl, cumulative_diff, diffusion_ledger


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, required=True)
    p.add_argument('--boundary-source', type=Path, required=True)
    p.add_argument('--dependencies', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--steps', type=int, default=80)
    p.add_argument('--dt-years', type=float, default=100.)
    p.add_argument('--diffusivity', type=float, default=0.)
    p.add_argument('--seed', type=int, default=1234)
    args = p.parse_args()
    if args.output.exists() or 'saved' not in [s.lower() for s in args.output.resolve().parts]:
        p.error('Choose a new ignored Saved output directory')
    if not 1 <= args.steps <= 500 or not 0 < args.dt_years <= 500 or not 0 <= args.diffusivity <= .1:
        p.error('Bounded proof requires steps1..500, dt<=500y, diffusivity0..0.1; ADI is never used')
    sys.path.insert(0, str(args.dependencies.resolve()))
    import fastscapelib as fs
    if importlib.metadata.version('fastscapelib') != '0.3.0' or not Path(fs.__file__).resolve().is_relative_to(args.dependencies.resolve()):
        raise ValueError('Requires isolated Fastscapelib0.3.0')
    with np.load(args.input, allow_pickle=False) as z:
        source = {k: z[k].copy() for k in z.files}
    with np.load(args.boundary_source, allow_pickle=False) as z:
        measured = {k: z[k].copy() for k in z.files}
    for key in ('world_xy_m', 'heightfield_valid_mask', 'cell_size_m', 'frame_origin_world_m', 'basis_u_world', 'basis_v_world', 'basis_up_world'):
        if not np.array_equal(source[key], measured[key]):
            raise ValueError(f'Measured boundary frame mismatch: {key}')
    valid = source['heightfield_valid_mask'] > 0
    outlets = source['outflow_mask'] > 0
    initial, raw, fixed, kinds, correction, settings = fresh_field(source, measured, args.seed)
    cell = float(source['cell_size_m'])
    started = time.perf_counter()
    # Exact differential check on this actual domain before trusting ADI across
    # irregular padding: change only invalid elevations by1000m. Physical land
    # must remain identical. This checks the chosen boundary arrangement.
    probe = solve(fs, initial, valid, fixed, outlets, cell, 3, args.dt_years, args.diffusivity)
    shifted = solve(fs, initial, valid, fixed, outlets, cell, 3, args.dt_years, args.diffusivity, 1000.)
    padding_error = float(np.max(np.abs(probe[0][valid]-shifted[0][valid])))
    if padding_error > 1.e-10:
        raise ValueError(f'Invalid-padding coupling {padding_error}m; do not export this model')
    final, area, spl_cut, diff_change, diffusion_ledger = solve(fs, initial, valid, fixed, outlets, cell, args.steps, args.dt_years, args.diffusivity)
    active = valid & ~fixed
    anchors = json.loads(str(source['anchors_json']))
    support_reports = []
    for anchor in anchors['anchors']:
        inside = (polygon_distance(source['world_xy_m'], anchor['footprint_xy_m']) < 1.e-6) & valid
        if inside.any():
            delta = final[inside]-float(anchor['support_height_m'])
            support_reports.append(dict(id=anchor['id'], sampled_nodes=int(inside.sum()),
                height_relative_support_range_m=[float(delta.min()), float(delta.max())],
                original_support_preserved=False, full_footing_support_verified=False))
    meta = dict(schema='directorscut.fastscape_crater_sector.v1', source_kind='fastscape_crater_sector_proposal',
        input=str(args.input.resolve()), input_sha256=sha(args.input),
        boundary_source=str(args.boundary_source.resolve()), boundary_source_sha256=sha(args.boundary_source),
        generator_sha256=sha(__file__), upstream_version='0.3.0', upstream_license='GPL-3.0',
        upstream_reference_revision='707fcc10976b0e729f1415fd16b0a161e1f746c3',
        units='absolute arena metres, unchanged source XY/frame, no rescaling or vertical exaggeration',
        geometry='2m heightfield exported as original3D triangles; no overhang claim',
        settings=dict(**settings, steps=args.steps, dt_years=args.dt_years, seed=args.seed,
            k_coef=1.e-4, area_exp=.4, slope_exp=1., diffusivity=args.diffusivity, multi_flow_exponent=1.1),
        boundary_policy='Explicit fixed coast/seam and invalid nodes; invalid nodes masked from flow and rainfall. Routing baselevels are only actual coast outlets; outer seams immobile but not baselevels. No periodic boundaries.',
        diffusion_method='Optional explicit valid-edge finite-volume linear diffusion, stable substeps; fixed-node signed exchange accounted; never Fastscapelib ADI',
        diffusion_ledger=diffusion_ledger,
        old_interior_used=False, temple_pads_frozen=False, runtime_integrated=False, visual_accepted=False,
        anchors=support_reports, elapsed_seconds=time.perf_counter()-started,
        metrics=dict(shape=list(final.shape), cell_size_m=cell, physical_extent_m=[500.,504.],
            valid_nodes=int(valid.sum()), fixed_physical_nodes=int((fixed & valid).sum()),
            initial_height_range_m=[float(initial[valid].min()),float(initial[valid].max())],
            final_height_range_m=[float(final[valid].min()),float(final[valid].max())],
            net_change_percentiles_m=np.percentile((final-initial)[active],[0,1,5,50,95,99,100]).tolist(),
            masked_padding_1000m_probe_error_m=padding_error, fixed_bitwise_preserved=bool(np.array_equal(final[fixed],initial[fixed])),
            initial_fit_abs_percentiles_m=np.percentile(np.abs(correction[active]),[50,95,99,100]).tolist()),
        limitations='Offline art proposal, not calibrated geology. ADI omitted because0.3.0 violates interior fixed nodes; optional masked linear hillslope diffusion replaces it. No transported stream sediment accounting. Existing temple placements unsupported and unmodified; no scientific PBR masks.')
    arrays = {k: source[k].copy() for k in ('cell_size_m','frame_origin_world_m','basis_u_world','basis_v_world','basis_up_world','world_xy_m','u_m','v_m','shore_clearance_m','heightfield_valid_mask','outflow_mask','coast_distance_m','anchors_json')}
    arrays.update(height_m=final, initial_height_m=initial, fresh_raw_height_m=raw,
        protected_mask=fixed.astype(np.float32), constraint_type=kinds,
        constraint_target_height_m=measured['height_m'], flow_accumulation=area,
        stream_power_lowering_m=spl_cut, diffusion_signed_lowering_m=diff_change,
        metadata_json=np.asarray(json.dumps(meta)))
    args.output.mkdir(parents=True)
    shutil.copyfile(__file__, args.output/'generator_source.py')
    licence = args.dependencies/'fastscapelib-0.3.0.dist-info'/'licenses'/'LICENSE'
    if not licence.is_file():
        raise ValueError('External package GPL licence must accompany the reproduction bundle')
    shutil.copyfile(licence, args.output/'LICENSE.fastscapelib')
    (args.output/'ATTRIBUTION.txt').write_text('Offline Fastscapelib0.3.0 API adaptation by the DirectorsCut project.\n'
        'External library: Fastscapelib developers, GNU GPL3; full licence accompanies this bundle.\n'
        'https://github.com/fastscape-lem/fastscapelib at707fcc10976b0e729f1415fd16b0a161e1f746c3\n'
        'Fresh physical terrain; not the unchanged official notebook or a scaled mesh. No Unreal runtime dependency.\n', encoding='utf-8')
    np.savez_compressed(args.output/'terrain.npz', **arrays)
    frame={k:source[k] for k in ('frame_origin_world_m','basis_u_world','basis_v_world')}
    frame['basis_z_world']=source['basis_up_world']
    for name, height in [('baseline', initial), ('candidate', final)]:
        mesh_data=dict(arrays,height_m=height)
        vertices,triangles,_,_=original_mesh(mesh_data)
        np.savez_compressed(args.output/(name+'.npz'), vertices_m=vertices, triangles=triangles,
            **frame, metadata_json=np.asarray(json.dumps(dict(meta,variant=name))))
    (args.output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(output=str(args.output.resolve()),metrics=meta['metrics'],anchors=support_reports),indent=2))


if __name__ == '__main__':
    main()
