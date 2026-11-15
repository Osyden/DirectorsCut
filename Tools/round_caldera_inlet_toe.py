"""Isolated original-source lower inlet toe candidate; no import or installation.

Lower two finite shoulder tips beside the narrow receiving river. Preserve all
triangles touching the wet centre corridor, the upper terrain, XY and topology.
This deliberately changes local exposed shoreline; fog/support data must be
rebuilt against the result before installation. It does not include WetFoot.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np

SOURCE_SHA = '6eac5bba828f84ef2196f3a6598c23829c503722eb056f7ad59269bd6ab444e7'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def smooth(a, b, x):
    u = np.clip((x-a)/(b-a), 0., 1.)
    return u*u*(3.-2.*u)


def chart(xy):
    r = np.linalg.norm(xy, axis=-1)
    angle = np.pi-.4*(1.-smooth(928., 1576., r))
    across = ((np.arctan2(xy[..., 1], xy[..., 0])-angle+np.pi) % (2*np.pi)-np.pi)*r
    width = 160.+25.*smooth(1040., 1105., r)+35.*smooth(1113., 1130., r)
    return r, across, width


def main():
    root = Path(__file__).resolve().parents[1]
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=root.parent/'Crater-Workers/Cliff-Contact/Saved/CompleteCliffRing/IntegratedBanksFractured/candidate.npz')
    p.add_argument('--output', type=Path, default=root/'Saved/CalderaInletToe')
    args = p.parse_args()
    if sha(args.source) != SOURCE_SHA:
        raise ValueError('Expected the immutable original IntegratedBanksFractured source.')
    if (args.output/'candidate.npz').exists():
        raise FileExistsError('Use a fresh output folder; never overwrite a candidate.')
    with np.load(args.source, allow_pickle=False) as data:
        arrays = {k: data[k].copy() for k in data.files}
    old = arrays['vertices_m']; f = arrays['triangles']; r, across, width = chart(old[:, :2])
    meta = json.loads(str(arrays['metadata_json'].item())); shape = tuple(meta['grid_shape'])
    # A finite river-mouth treatment, expressed in the production polar chart.
    # The full-weight tip reaches -405m. The collar joins unchanged terrain.
    # The negative bank's measured inner shoulder is steeper. Its broader
    # 25m collar avoids reversing four original face normals at R1027..1032m;
    # the positive bank's 15m collar reaches the actual exposed tip by180m.
    inner_end = np.where(across < 0., 190., 180.)
    weight = (smooth(940., 960., r)*(1.-smooth(1030., 1060., r))
              *smooth(165., inner_end, abs(across))*(1.-smooth(210., 245., abs(across))))
    protected_face = ((abs(across) <= np.maximum(165., width))[f]).any(axis=1)
    protected_vertex = np.zeros(len(old), dtype=bool)
    protected_vertex[f[protected_face].ravel()] = True
    protected_vertex.reshape(shape)[[0, -1], :] = True
    protected_vertex.reshape(shape)[:, [0, -1]] = True
    weight[protected_vertex] = 0.
    lowering = np.maximum(old[:, 2]+405., 0.)*weight
    new = old.copy(); new[:, 2] -= lowering
    changed = lowering > 1.e-9; cf = changed[f].any(axis=1)
    n0 = np.cross(old[f[:, 1]]-old[f[:, 0]], old[f[:, 2]]-old[f[:, 0]])
    n1 = np.cross(new[f[:, 1]]-new[f[:, 0]], new[f[:, 2]]-new[f[:, 0]])
    area = np.linalg.norm(n1, axis=1)
    native = ((new-np.array([-1072., 1154., -25.]))*100.).astype(np.float32).astype(np.float64)*.01
    na = np.linalg.norm(np.cross(native[f[:, 1]]-native[f[:, 0]], native[f[:, 2]]-native[f[:, 0]]), axis=1)
    oldz = old[f, 2]; newz = new[f, 2]
    shore0 = (oldz.min(axis=1) <= -400.) & (oldz.max(axis=1) >= -400.)
    shore1 = (newz.min(axis=1) <= -400.) & (newz.max(axis=1) >= -400.)
    witnesses = []
    # These are existing original-triangle witnesses, not deformation controls.
    ray_path = root/'Saved/CalderaCameraRays/Native285/report.json'
    if ray_path.exists():
        rays = json.loads(ray_path.read_text())['sources']['source']['rays'][:2]
        for ray in rays:
            hit = ray['terrain_hits'][0]; face = int(hit['triangle_index'])
            bary = np.asarray(hit['barycentric']); point = bary@new[f[face]]
            witnesses.append(dict(triangle=face, barycentric=bary.tolist(), original_z_m=float(bary@old[f[face], 2]),
                                  candidate_position_m=point.tolist(), mean_lake_cover_m=float(-400.-point[2])))
    metrics = dict(vertices=len(new), triangles=len(f), changed_vertices=int(changed.sum()),
                   changed_triangles=int(cf.sum()), maximum_lowering_m=float(lowering.max()),
                   changed_radius_m=[float(r[changed].min()), float(r[changed].max())],
                   changed_abs_across_m=[float(abs(across[changed]).min()), float(abs(across[changed]).max())],
                   changed_original_height_m=[float(old[changed, 2].min()), float(old[changed, 2].max())],
                   minimum_twice_triangle_area_m2=float(area.min()),
                   degenerate_triangles=int(np.count_nonzero(area <= 1.e-9)),
                   source_opposed_triangle_normals=int(np.count_nonzero(np.einsum('ij,ij->i', n0, n1) < 0.)),
                   native_float32_cm_collapsed_triangles=int(np.count_nonzero(na <= 1.e-9)),
                   native_float32_cm_minimum_twice_triangle_area_m2=float(na.min()),
                   seam_position_error_m=float(np.max(np.linalg.norm(new.reshape(*shape, 3)[:, 0]-new.reshape(*shape, 3)[:, -1], axis=1))),
                   xy_exact=bool(np.array_equal(new[:, :2], old[:, :2])), topology_uv_masks_exact=True,
                   protected_wet_triangle_max_change_m=float(np.max(abs(new[f[protected_face]]-old[f[protected_face]]))),
                   original_shore_faces=int(shore0.sum()), candidate_shore_faces=int(shore1.sum()),
                   changed_original_shore_faces=int(np.count_nonzero(shore0 & cf)),
                   removed_shore_faces=int(np.count_nonzero(shore0 & ~shore1)),
                   added_shore_faces=int(np.count_nonzero(shore1 & ~shore0)),
                   upper_radius_1060_plus_exact=bool(np.array_equal(new[r >= 1060.], old[r >= 1060.])),
                   exact_triangle_witnesses=witnesses)
    provenance = dict(schema='directorscut.caldera_inlet_toe.v1', parent_source=str(args.source.resolve()),
                      parent_source_sha256=SOURCE_SHA, generator_sha256=sha(__file__),
                      method='Z-only smooth polar mouth-tip taper beside the finite lower river; target -405m at full weight',
                      radial_mask_m=[940., 960., 1030., 1060.],
                      positive_across_mask_m=[165., 180., 210., 245.], negative_abs_across_mask_m=[165., 190., 210., 245.],
                      target_z_m=-405., metrics=metrics,
                      limitations='Original source, not WetFoot. Local shoreline deliberately changes. Inherited bank anchors and mist contacts are historical and MUST be revalidated/regenerated. No native, wave, optical or whole-route certification. Lowering cannot add a new obstacle, but existing centreline clearance is not improved.')
    args.output.mkdir(parents=True, exist_ok=True)
    if metrics['degenerate_triangles'] or metrics['source_opposed_triangle_normals'] or metrics['native_float32_cm_collapsed_triangles']:
        (args.output/'failed_report.json').write_text(json.dumps(provenance, indent=2)+'\n')
        raise RuntimeError('Bounded geometry failed; see failed_report.json. No candidate written.')
    meta['inlet_toe_rounding'] = provenance
    meta['shoreline_contacts_require_regeneration'] = True
    meta['seam_position_error_m'] = metrics['seam_position_error_m']
    meta['exported_collapsed_triangles'] = metrics['native_float32_cm_collapsed_triangles']
    meta['exported_minimum_twice_triangle_area_m2'] = metrics['native_float32_cm_minimum_twice_triangle_area_m2']
    arrays['vertices_m'] = new; arrays['inlet_toe_lowering_m'] = lowering.astype(np.float32)
    arrays['metadata_json'] = np.array(json.dumps(meta))
    candidate = args.output/'candidate.npz'; np.savez_compressed(candidate, **arrays)
    manifest = json.loads((args.source.parent/'manifest.json').read_text())
    manifest.update(source=str(candidate.resolve()), source_sha256=sha(candidate), vertices=len(new), triangles=len(f),
                    inlet_toe_rounding=provenance, shoreline_contacts_require_regeneration=True,
                    bank_boulder_anchors_require_revalidation=True, installation_approved=False,
                    seam_position_error_m=metrics['seam_position_error_m'],
                    exported_collapsed_triangles=metrics['native_float32_cm_collapsed_triangles'],
                    exported_minimum_twice_triangle_area_m2=metrics['native_float32_cm_minimum_twice_triangle_area_m2'],
                    historical_parent_geometry=dict(source=str(args.source.resolve()), source_sha256=SOURCE_SHA,
                                                    manifest_sha256=sha(args.source.parent/'manifest.json')))
    (args.output/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    report = dict(provenance, candidate=str(candidate.resolve()), candidate_sha256=sha(candidate))
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    np.savez_compressed(args.output/'changed_shoreline.npz',
                        changed_original_shore_faces=np.flatnonzero(shore0 & cf),
                        removed_shore_faces=np.flatnonzero(shore0 & ~shore1),
                        added_shore_faces=np.flatnonzero(shore1 & ~shore0),
                        candidate_local_shore_faces=np.flatnonzero(shore1 & cf))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
