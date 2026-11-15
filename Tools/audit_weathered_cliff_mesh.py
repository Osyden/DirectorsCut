"""Independent original-mesh provenance, guard and mask checks; no projection.

Requires candidate NPZ correspondence to an immutable source heightfield, but
never replaces the candidate's original 3D vertices/triangles with heights.
Reports fold/normal diagnostics separately from topology and source fidelity.
Arbitrary ray comparisons are in test_cliff_triangle_surface.py --mesh.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np

sys.dont_write_bytecode = True


def load(path):
    with np.load(path, allow_pickle=False) as data:
        return {key: data[key].copy() for key in data.files}


def ray_hit(faces, bounds, origin, direction, maximum):
    """Original 3D triangle ray test after segment-AABB rejection, two sided."""
    direction = np.asarray(direction,float)
    direction /= np.linalg.norm(direction)
    end = origin+direction*maximum
    lo,hi = np.minimum(origin,end)-1.e-7,np.maximum(origin,end)+1.e-7
    indices = np.flatnonzero(np.all(bounds[1]>=lo,axis=1)&np.all(bounds[0]<=hi,axis=1))
    if len(indices) == 0:
        return None
    xyz = faces[indices]
    e1,e2 = xyz[:,1]-xyz[:,0],xyz[:,2]-xyz[:,0]
    n = np.cross(e1,e2)
    p = np.cross(direction,e2)
    det = np.einsum('ij,ij->i',e1,p)
    usable = np.abs(det)>np.linalg.norm(n,axis=1)*1.e-14
    inv = np.divide(1.,det,out=np.zeros_like(det),where=usable)
    ao = origin-xyz[:,0]
    q = np.cross(ao,e1)
    u = np.einsum('ij,ij->i',ao,p)*inv
    v = (q@direction)*inv
    distance = np.einsum('ij,ij->i',e2,q)*inv
    usable &= (u>=-1.e-9)&(v>=-1.e-9)&(u+v<=1+1.e-9)&(distance>=0)&(distance<=maximum)
    if not usable.any():
        return None
    j = int(np.argmin(np.where(usable,distance,np.inf)))
    return dict(triangle=int(indices[j]),distance_m=float(distance[j]),position=origin+direction*distance[j],
                normal=n[j]/np.linalg.norm(n[j]))


def contact_probes(faces, baseline_faces, source, normals):
    bounds = (faces.min(axis=1),faces.max(axis=1))
    old_bounds = (baseline_faces.min(axis=1),baseline_faces.max(axis=1))
    global_min = np.minimum(bounds[0].min(axis=0),old_bounds[0].min(axis=0))
    global_max = np.maximum(bounds[1].max(axis=0),old_bounds[1].max(axis=0))
    # Short rays normal to real steep faces do not reinterpret them as heights.
    choices = np.flatnonzero(np.abs(normals[:,2])<.45)
    selected = choices[np.linspace(0,len(choices)-1,min(64,len(choices))).astype(int)] if len(choices) else []
    missed,blocked,errors = 0,0,[]
    for i in selected:
        center = faces[i].mean(axis=0)
        hit = ray_hit(faces,bounds,center+normals[i]*.25,-normals[i],.250001)
        if hit is None:
            missed += 1
        else:
            errors.append(abs(hit['distance_m']-.25))
            blocked += hit['distance_m'] < .24999
    report = dict(steep_face_normal_rays=len(selected),missing_steep_hits=missed,
                  steep_hits_occluded_before_target=blocked,
                  steep_distance_error_max_m=max(errors,default=0),temples=[])
    anchors = json.loads(str(source.get('anchors_json','{"anchors":[]}')))
    if isinstance(anchors,list):
        anchors = dict(anchors=anchors)
    high = float(max(faces[:,:,2].max(),baseline_faces[:,:,2].max())+10)
    low = float(min(faces[:,:,2].min(),baseline_faces[:,:,2].min())-10)
    for anchor in anchors.get('anchors',[]):
        samples=[]
        for sample in anchor.get('ground_samples',[]):
            origin=np.array([sample['x_m'],sample['y_m'],high])
            if np.any(origin[:2]<global_min[:2]-1.e-7) or np.any(origin[:2]>global_max[:2]+1.e-7):
                continue
            old=ray_hit(baseline_faces,old_bounds,origin,[0,0,-1],high-low)
            new=ray_hit(faces,bounds,origin,[0,0,-1],high-low)
            if old is None and new is None:
                continue
            samples.append(dict(x_m=sample['x_m'],y_m=sample['y_m'],baseline_covered=old is not None,candidate_covered=new is not None,
                                delta_from_baseline_m=float(new['position'][2]-old['position'][2]) if new and old else None,
                                candidate_height_m=float(new['position'][2]) if new else None,
                                delta_from_actual_legacy_ground_m=float(new['position'][2]-sample['ground_m']) if new else None,
                                clearance_above_footing_m=float(new['position'][2]-anchor['footing_bottom_m']) if new and 'footing_bottom_m' in anchor else None))
        if samples:
            report['temples'].append(dict(id=anchor['id'],samples=samples,
                                          input_sample_count=len(anchor.get('ground_samples',[])),
                                          covered_sample_count=sum(s['candidate_covered'] for s in samples)))
    report['limit']='Original triangle rays at finite sample points; not complete footprint or self-intersection proof. Near-normal rays can be occluded by legitimate overhangs.'
    return report


def audit(candidate_path, source_path, allow_projected_reversal=False, allow_crop_boundary_motion=False):
    data, source = load(candidate_path), load(source_path)
    p = np.asarray(data['vertices_m'], np.float64)
    base = np.asarray(data['baseline_vertices_m'], np.float64)
    t = np.asarray(data['triangles'])
    st = np.asarray(data['source_grid_triangles'])
    ids = np.asarray(data['source_triangle_index'])
    weights = np.asarray(data['source_barycentric'], np.float64)
    guard = np.asarray(data['guard_weight'])
    kind = np.asarray(data['guard_kind'])
    rgba = np.asarray(data['vertex_rgba'])
    failures = []

    def require(ok, reason):
        if not ok:
            failures.append(reason)

    if p.shape != base.shape or p.ndim != 2 or p.shape[1] != 3 or not np.isfinite(p).all() or not np.isfinite(base).all():
        raise ValueError('Original/baseline vertices must be finite matching N x 3 arrays')
    if t.ndim != 2 or t.shape[1] != 3 or t.dtype.kind not in 'iu' or np.any(t < 0) or np.any(t >= len(p)):
        raise ValueError('Original triangles contain invalid indices')
    if weights.shape != p.shape or ids.shape != (len(p),) or ids.dtype.kind not in 'iu' or np.any(ids < 0) or np.any(ids >= len(st)):
        raise ValueError('Invalid source triangle/barycentric correspondence')
    h = source['height_m']
    rows, cols = h.shape
    valid = source['heightfield_valid_mask'] != 0
    # Independently enumerate original ABD then ADC halves in canonical order.
    expected = []
    for half in (0,1):
        for r in range(rows-1):
            for c in range(cols-1):
                a = r*cols+c
                tri = (a, a+1, a+cols+1) if half == 0 else (a, a+cols+1, a+cols)
                if valid.ravel()[list(tri)].all():
                    expected.append(tri)
    expected = np.asarray(expected, np.int64)
    require(np.array_equal(st, expected), 'source triangle ordering/domain differs from canonical valid ABD/ADC source')
    if st.ndim != 2 or st.shape[1] != 3 or np.any(st < 0) or np.any(st >= h.size):
        raise ValueError('Invalid source_grid_triangles')
    require(np.isfinite(weights).all() and weights.min() >= -1.e-8 and np.max(np.abs(weights.sum(axis=1)-1)) < 1.e-7, 'invalid barycentric weights')
    xx, yy = np.meshgrid(np.arange(cols)*float(source['cell_size_m']), np.arange(rows)*float(source['cell_size_m']))
    source_xyz = (source['frame_origin_world_m']+xx[...,None]*source['basis_u_world']+yy[...,None]*source['basis_v_world']).reshape(-1,3)
    source_xyz[:,2] = h.ravel()
    corners = st[ids]
    reconstructed = np.einsum('ni,nij->nj', weights, source_xyz[corners])
    position_error = float(np.max(np.linalg.norm(reconstructed-base, axis=1)))
    require(position_error < 1.e-5, 'baseline no longer reconstructs original source triangles')
    require(guard.shape == (len(p),) and np.isfinite(guard).all() and guard.min() >= 0 and guard.max() <= 1, 'guard weight outside [0,1]')
    source_kind = np.bitwise_or.reduce(source['constraint_type'].ravel()[corners], axis=1)
    require(np.array_equal(kind, source_kind), 'guard kinds no longer match original source constraint bits')
    require(np.all(guard[source_kind != 0] == 0), 'original constrained triangle may move')
    fixed = guard == 0
    delta = p-base
    distance = np.linalg.norm(delta, axis=1)
    require(np.array_equal(p[fixed], base[fixed]), 'fixed guard vertices changed')
    direction_report = None
    if 'stable_source_normal' in data and 'normal_displacement_m' in data:
        direction = np.asarray(data['stable_source_normal'],float)
        scalar = np.asarray(data['normal_displacement_m'],float)
        if direction.shape != p.shape or scalar.shape != (len(p),):
            raise ValueError('Stored displacement direction/scalar shape mismatch')
        direction_error = float(np.max(np.linalg.norm(delta-direction*scalar[:,None],axis=1)))
        unit_error = float(np.max(np.abs(np.linalg.norm(direction,axis=1)-1)))
        require(np.isfinite(direction).all() and np.isfinite(scalar).all() and direction_error < 1.e-5 and unit_error < 1.e-6,
                'stored displacement direction does not reproduce original mesh positions')
        metadata = json.loads(str(data.get('metadata_json','{}')))
        declared_hash = metadata.get('source_sha256')
        require(declared_hash is None or declared_hash == hashlib.sha256(Path(source_path).read_bytes()).hexdigest(),
                'declared original source hash mismatch')
        direction_report = dict(reconstruction_max_m=direction_error,unit_length_error_max=unit_error,
                                declared_horizontal_factor=metadata.get('metrics',{}).get('fold_guard',{}).get('horizontal_normal_factor'),
                                declared_description=metadata.get('settings',{}).get('displacement_direction'),
                                note='The stored direction is effective, potentially upward biased; its name alone does not imply an untouched source normal.')
    channels = []
    for key, logarithmic in (('flow_accumulation', True), ('deposited_m', False), ('eroded_m', False)):
        field = np.maximum(source[key].astype(np.float64), 0)
        if logarithmic:
            field = np.log1p(field)
        field /= max(float(field[valid].max()), 1.e-12)
        channels.append(np.sum(field.ravel()[corners]*weights, axis=1))
    expected_rgba = np.column_stack((*channels, np.ones(len(p))))
    if rgba.shape != expected_rgba.shape:
        raise ValueError('vertex_rgba shape mismatch')
    mask_error = np.max(np.abs(expected_rgba-rgba), axis=0)
    require(np.isfinite(rgba).all() and mask_error.max() < 2.e-6, 'material masks disagree with original triangle barycentric sampling')

    faces, old_faces = p[t], base[t]
    cross = np.cross(faces[:,1]-faces[:,0], faces[:,2]-faces[:,0])
    old_cross = np.cross(old_faces[:,1]-old_faces[:,0], old_faces[:,2]-old_faces[:,0])
    area2, old_area2 = np.linalg.norm(cross,axis=1), np.linalg.norm(old_cross,axis=1)
    normal = cross/np.maximum(area2[:,None], 1.e-30)
    require(np.all(area2 > 1.e-10), 'degenerate original triangles')
    xy_ratio = cross[:,2]/old_cross[:,2]
    if not allow_projected_reversal:
        require(np.isfinite(xy_ratio).all() and np.all(xy_ratio>0), 'weathered mapping violates its positive projected triangle contract')
    directed = np.concatenate((t[:,[0,1]],t[:,[1,2]],t[:,[2,0]]))
    edge, inverse, counts = np.unique(np.sort(directed,axis=1),axis=0,return_inverse=True,return_counts=True)
    winding = np.bincount(inverse,weights=np.where(directed[:,0] < directed[:,1],1,-1))
    require(np.all(counts <= 2), 'nonmanifold edge')
    require(np.all(winding[counts == 2] == 0), 'inconsistent shared-edge winding')
    require(len(np.unique(np.sort(t,axis=1),axis=0)) == len(t), 'duplicate triangles')
    boundary = np.unique(edge[counts == 1])
    if not allow_crop_boundary_motion:
        require(np.array_equal(p[boundary],base[boundary]), 'an original open boundary moved')
    lengths = np.linalg.norm(p[edge[:,0]]-p[edge[:,1]],axis=1)
    old_lengths = np.linalg.norm(base[edge[:,0]]-base[edge[:,1]],axis=1)
    cos = np.sum(cross*old_cross,axis=1)/np.maximum(area2*old_area2,1.e-30)
    angle = np.degrees(np.arccos(np.clip(cos,-1,1)))
    report = dict(schema='directorscut.weathered_mesh_audit.v1',candidate=str(Path(candidate_path).resolve()),
                  source=str(Path(source_path).resolve()),candidate_sha256=hashlib.sha256(Path(candidate_path).read_bytes()).hexdigest(),
                  source_sha256=hashlib.sha256(Path(source_path).read_bytes()).hexdigest(),
                  vertices=len(p),triangles=len(t),fixed_vertices=int(fixed.sum()),open_boundary_vertices=len(boundary),
                  source_position_error_max_m=position_error,mask_error_max_rgba=mask_error.tolist(),
                  displacement_m_p0_p50_p95_p99_p100=np.percentile(distance,[0,50,95,99,100]).tolist(),
                  edge_length_m_p0_p50_p95_p99_p100=np.percentile(lengths,[0,50,95,99,100]).tolist(),
                  edge_stretch_p50_p95_p99_p100=np.percentile(lengths/np.maximum(old_lengths,1.e-20),[50,95,99,100]).tolist(),
                  normal_change_deg_p50_p95_p99_p100=np.percentile(angle,[50,95,99,100]).tolist(),
                  normal_reversals_over90_deg=int((cos < 0).sum()),downward_triangles=int((normal[:,2]<0).sum()),
                  nearvertical_triangles=int((np.abs(normal[:,2])<.25).sum()),
                  projected_xy_area_ratio_min=float(xy_ratio.min()),
                  projected_nonpositive_triangle_count=int((xy_ratio<=0).sum()),
                  crop_boundary_motion_max_m=float(distance[boundary].max()) if len(boundary) else 0.,
                  explicit_audit_allowances=dict(projected_reversal=allow_projected_reversal,crop_boundary_motion=allow_crop_boundary_motion),
                  stored_displacement_direction=direction_report,
                  failures=failures,passed=not failures,
                  limitations='No exhaustive self-intersection test or visual approval. Normal reversals are diagnostics, not automatically invalid. Temple support outside this tile remains unproved.')
    report['contact_probes'] = contact_probes(faces,old_faces,source,normal)
    if report['contact_probes']['missing_steep_hits']:
        failures.append('short normal ray misses its actual original steep triangle')
    for temple in report['contact_probes']['temples']:
        if any(s['baseline_covered'] != s['candidate_covered'] or
               abs(s['delta_from_baseline_m'] or 0) > 1.e-4 for s in temple['samples']):
            failures.append('temple guard sample changed: '+temple['id'])
    report['passed'] = not failures
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mesh', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--allow-projected-reversal',action='store_true',help='Only for a mapping whose real3D overhangs are intentional; does not relax triangle or winding checks')
    parser.add_argument('--allow-crop-boundary-motion',action='store_true',help='Review crop cuts may move; actual constrained source points must still be exact')
    args = parser.parse_args()
    report = audit(args.mesh,args.source,args.allow_projected_reversal,args.allow_crop_boundary_motion)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({key:report[key] for key in ('passed','vertices','triangles','fixed_vertices','source_position_error_max_m','mask_error_max_rgba','normal_reversals_over90_deg','projected_xy_area_ratio_min','failures')},indent=2))
    raise SystemExit(0 if report['passed'] else 1)


if __name__ == '__main__':
    main()
