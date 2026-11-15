"""Bounded macro-erosion response of the measured cliff; no new noise field.

Uses the existing conservative D8 solver with a deeper authored bedrock bound.
Every original triangle touching a protected sample stays exact. This probes
whether stronger transport can change the soft macro form; it is not a physical
geological-timescale simulation or a finished cliff generator.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_eroded_cliff_sector import erode, Settings
from generate_weathered_cliff_mesh import original_mesh


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Choose a new ignored Saved directory')
    with np.load(args.source, allow_pickle=False) as archive:
        source = {k: archive[k].copy() for k in archive.files}
    baseline_vertices, triangles, grid_faces, _ = original_mesh(source)
    originally_fixed = source['protected_mask'].reshape(-1) >= 1
    frozen_faces = originally_fixed[grid_faces].any(axis=1)
    fixed = originally_fixed.copy()
    fixed[np.unique(grid_faces[frozen_faces])] = True
    fixed = fixed.reshape(source['height_m'].shape)
    valid = source['heightfield_valid_mask'] > 0
    active = valid & ~fixed
    # Retain original hardness variations. This is an authored erodible-depth
    # budget, not a measured underground geological survey.
    hardness_response = source['erodibility'].astype(np.float64)
    seed = {k: v.copy() for k, v in source.items()}
    seed['protected_mask'] = fixed.astype(np.float32)
    seed['erodibility'] = np.where(fixed, 0., hardness_response).astype(np.float32)
    available_depth = 35. + 30. * hardness_response
    seed['bedrock_m'] = np.where(fixed, source['height_m'],
        source['height_m'].astype(np.float64) - available_depth).astype(np.float32)
    seed['loose_m'] = np.zeros_like(source['height_m'])
    settings = Settings(steps=640, seed=71421, rainfall_m=.025,
        sediment_capacity=4., erosion_rate=.25, deposition_rate=.20,
        thermal_rate=.05, max_erosion_cell_fraction=.06)
    result, metrics = erode(seed, settings)
    vertices, output_triangles, _, _ = original_mesh(result)
    if not np.array_equal(output_triangles, triangles):
        raise ValueError('Macro response unexpectedly changed source topology')
    protected_vertex_ids = np.unique(triangles[frozen_faces])
    if not np.array_equal(vertices[protected_vertex_ids], baseline_vertices[protected_vertex_ids]):
        raise ValueError('Protected source triangle positions changed')
    displacement = result['height_m'].astype(np.float64) - source['height_m']
    changed = np.abs(displacement[active])
    metadata = dict(schema='directorscut.macro_erosion_response.v1',
        source_kind='measured_cliff_deeper_erosion_probe',
        source_path=str(args.source.resolve()),
        source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        generator='Existing D8 conservative solver; no new procedural mountain/noise',
        source_erosion_metrics=json.loads(str(source['metrics_json'])),
        source_surface_is_baseline=True,
        authored_available_depth_m=[float(available_depth[active].min()), float(available_depth[active].max())],
        retained_source_triangle_count=int(frozen_faces.sum()),
        retained_source_vertex_count=len(protected_vertex_ids),
        frozen_triangle_positions_bitwise_exact=True,
        absolute_height_change_p50_p90_p99_max_m=np.percentile(changed,[50,90,99,100]).tolist(),
        free_land_fraction_changed_over_5m=float((changed>5.).mean()),
        free_land_fraction_changed_over_20m=float((changed>20.).mean()),
        current_solver_metrics=metrics, shape_accepted=False, arena_integrated=False,
        limitations='Accelerated heightfield morphology response; D8 grid bias, no overhangs, rock fractures or calibrated years. Coast and whole protected source triangles fixed; remaining transport masks describe this solver run, not present-day wetness.')
    result['metadata_json'] = np.asarray(json.dumps(metadata))
    frame = dict(frame_origin_world_m=source['frame_origin_world_m'],
        basis_u_world=source['basis_u_world'], basis_v_world=source['basis_v_world'],
        basis_z_world=source['basis_up_world'])
    args.output.mkdir(parents=True)
    np.savez_compressed(args.output/'eroded_heightfield.npz', **result)
    for label, positions in (('baseline', baseline_vertices), ('candidate', vertices)):
        np.savez_compressed(args.output/(label+'.npz'), vertices_m=positions,
            triangles=triangles, **frame,
            metadata_json=np.asarray(json.dumps(dict(metadata, variant=label))))
    (args.output/'response.json').write_text(json.dumps(metadata, indent=2)+'\n')
    print(json.dumps(dict(output=str(args.output.resolve()),
        absolute_height_change_p50_p90_p99_max_m=metadata['absolute_height_change_p50_p90_p99_max_m'],
        fraction_changed_over_5m=metadata['free_land_fraction_changed_over_5m'],
        retained_triangles=metadata['retained_source_triangle_count'],
        max_erosion_m=metrics['max_erosion_depth_m'],
        max_deposition_m=metrics['max_deposition_height_m'],
        solver_solid_error_m3=metrics['solver_solid_mass_error_m3'],
        water_error_m3=metrics['water_budget_error_m3']), indent=2))


if __name__ == '__main__':
    main()
