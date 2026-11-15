"""Add metre-scale broken bedding and finite joints to an authored cliff.

This is an artistic rock exposure treatment, not a geology simulation. The
parent supplies the large ravines; this tool only recesses its smaller beds.
It never changes the original coastline or outer edges. Recesses follow the
geometric surface normal, so fine ledges can change elevation by their depth.
Outputs an offline candidate under Saved for visual review, not runtime assets.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_displaced_crater_face import refine


def smooth(a, b, x):
    t = np.clip((x-a)/(b-a), 0., 1.)
    return t*t*(3.-2.*t)


def exposure(u, v, z):
    """Unequal dipping beds, interrupted ledges and non-throughgoing joints."""
    rng = np.random.default_rng(74019)
    # Bed elevations are aperiodic. A small coherent dip crosses the parent
    # ravines rather than painting contour lines along each hillside.
    q = z+.045*u+.012*v+.18*np.sin(u/83.)+.12*np.sin(v/41.)
    bounds = np.r_[-600., -600.+np.cumsum(rng.uniform(1.8, 6.5, 400))]
    layer = np.clip(np.searchsorted(bounds, q)-1, 0, len(bounds)-2)
    bed_depth = rng.uniform(.22, .85, len(bounds))
    # Long but incomplete exposed bedding: cuts pinch out gradually, not as
    # closed rectangular blocks. Changes in phase occur on separate beds.
    phase = rng.uniform(0., 2*np.pi, len(bounds))
    span = rng.uniform(13., 31., len(bounds))
    # Evaluate each boundary as ONE continuous field on both sides. The old
    # fractional-layer reset jumped from zero to full depth at every boundary,
    # aliasing a sub-metre notch into triangle-sized scratches. Full profiles
    # are now3.7..5.2m wide, giving several samples on the .85m source mesh.
    lower_width = rng.uniform(1.5, 2.1, len(bounds))
    upper_width = rng.uniform(2.2, 3.1, len(bounds))
    beds = np.zeros_like(q)
    # Include two adjacent beds each side so overlapping wide profiles remain
    # continuous when layer membership changes. MAX keeps depth bounded.
    for offset in (-1, 0, 1, 2):
        b = np.clip(layer+offset, 0, len(bounds)-1)
        d = q-bounds[b]
        width = np.where(d<0.,lower_width[b],upper_width[b])
        notch = 1.-smooth(0.,1.,abs(d)/width)
        continuity = smooth(-.65,.25,np.sin(u/span[b]+phase[b]))
        beds = np.maximum(beds,bed_depth[b]*notch*(.15+.85*continuity))
    joints = np.zeros_like(q)
    positions = np.r_[-80., -80.+np.cumsum(rng.uniform(11., 32., 40))]
    for centre in positions:
        for _ in range(3):
            mid = rng.uniform(-160., 65.)
            half = rng.uniform(9., 31.)
            tilt = rng.uniform(-.20, .20)
            # Half-width1.8..3.3m: the previous .4m cut could fit between
            # adjacent mesh samples and produced disconnected serrated marks.
            width = rng.uniform(1.8, 3.3)
            distance = np.abs(u-(centre+tilt*(z-mid)+.25*np.sin(z/17.+centre)))
            end = 1.-smooth(half*.65, half, np.abs(z-mid))
            cut = (1.-smooth(0.,1.,distance/width))*end
            joints = np.maximum(joints, cut*rng.uniform(.30, .80))
    return beds+joints


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--edge-metres', type=float, default=.85)
    a = p.parse_args()
    if not .75 <= a.edge_metres <= .95:
        p.error('Resolved proof requires .75..0.95m edge target; wider targets undersample the physical cuts')
    if a.output.exists() or 'saved' not in [x.lower() for x in a.output.resolve().parts]:
        p.error('Use a new output directory under Saved')
    with np.load(a.input, allow_pickle=False) as z:
        data = {k:z[k].copy() for k in z.files}
    origin = data['frame_origin_world_m']
    basis = np.stack([data['basis_u_world'],data['basis_v_world'],data['basis_z_world']],axis=1)
    chart = (data['vertices_m']-origin)@basis
    triangles = data['triangles'].astype(np.int32)
    # Preserve all contact/join triangles. Conforming refinement may add
    # coplanar edge vertices there, but the protected surface does not move.
    start = int(data['main_vertex_start'])
    selected = np.all(triangles>=start, axis=1).astype(np.int8)
    v,t,lineage,iterations = refine(chart,triangles,selected,a.edge_metres)
    contact = data['contact_uv_m']
    shore = np.interp(v[:,0],contact[:,0],contact[:,1])
    shore_z = np.interp(v[:,0],contact[:,0],data['contact_water_height_m'])
    weights = smooth(8.,20.,v[:,1]-shore)*smooth(3.,12.,v[:,2]-shore_z)
    weights *= smooth(0.,8.,v[:,0]-chart[:,0].min())
    weights *= smooth(0.,8.,chart[:,0].max()-v[:,0])
    # The top edge is curved in source U/V, not the global maximumV. The
    # original parametric grid has one final row per contact column.
    top = chart[-len(contact):]
    if not np.all(np.diff(top[:,0])>0.):
        raise ValueError('Expected monotonic source-column top boundary')
    top_v = np.interp(v[:,0],top[:,0],top[:,1])
    weights *= smooth(0.,9.,top_v-v[:,1])
    edges = np.sort(np.concatenate((t[:,[0,1]],t[:,[1,2]],t[:,[2,0]])),axis=1)
    edges,edge_counts = np.unique(edges,axis=0,return_counts=True)
    boundary = np.unique(edges[edge_counts==1])
    protected = np.unique(t[selected[lineage]==0])
    protected = np.union1d(protected,boundary)
    weights[protected] = 0.
    requested = exposure(v[:,0],v[:,1],v[:,2])*weights
    recess = requested.copy()
    cross_before = np.cross(v[t[:,1]]-v[t[:,0]],v[t[:,2]]-v[t[:,0]])
    normal = np.zeros_like(v)
    for k in range(3):
        np.add.at(normal,t[:,k],cross_before)
    normal /= np.maximum(np.linalg.norm(normal,axis=1)[:,None],1.e-20)
    candidate = v-normal*recess[:,None]
    cross_after = np.cross(candidate[t[:,1]]-candidate[t[:,0]],candidate[t[:,2]]-candidate[t[:,0]])
    area = np.linalg.norm(cross_after,axis=1)
    turns = np.einsum('ij,ij->i',cross_before,cross_after)
    # Smoothly attenuate an actual local fold over6m along mesh edges. Merely
    # halving its three vertices creates a new sharp displacement boundary.
    damped = np.zeros(len(v),dtype=bool)
    limiter = np.ones(len(v))
    edge_step = np.linalg.norm(v[edges[:,1]]-v[edges[:,0]],axis=1)/6.
    for iteration in range(24):
        if np.all(turns>0.) and np.all(area>1.e-10):
            break
        affected = np.unique(t[(turns<=0.)|(area<=1.e-10)])
        damped[affected] = True
        limiter[affected] *= .5
        if iteration>=18:
            limiter[affected] = 0.
        # Multi-source graph distance envelope. A finite physical slope avoids
        # damping unrelated cliff regions or introducing single-vertex seams.
        for propagation in range(48):
            before = limiter.copy()
            np.minimum.at(limiter,edges[:,0],before[edges[:,1]]+edge_step)
            np.minimum.at(limiter,edges[:,1],before[edges[:,0]]+edge_step)
            if np.max(before-limiter)<1.e-10:
                break
        recess = requested*smooth(0.,1.,limiter)
        candidate = v-normal*recess[:,None]
        cross_after = np.cross(candidate[t[:,1]]-candidate[t[:,0]],candidate[t[:,2]]-candidate[t[:,0]])
        area = np.linalg.norm(cross_after,axis=1)
        turns = np.einsum('ij,ij->i',cross_before,cross_after)
    if np.any(turns<=0.) or np.any(area<=1.e-10):
        raise ValueError('Bedding creates unresolved folds; candidate refused: '+json.dumps(dict(
            folds=int(np.count_nonzero(turns<=0.)),min_area=float(area.min()),
            min_source_area=float(np.linalg.norm(cross_before,axis=1).min()),max_recess=float(recess.max()))))
    metadata = dict(schema='directorscut.cliff_bedding.v1',
        source=str(a.input.resolve()),source_sha256=hashlib.sha256(a.input.read_bytes()).hexdigest(),
        units='metres',artistic=True,simulated_geology=False,
        description='Continuous aperiodic dipping beds1.8–6.5m; notch support3.7–5.2m; finite joint support3.6–6.6m; surface-normal recess',
        edge_target_m=a.edge_metres,refinement_iterations=iterations,
        recess_range_m=[float(recess.min()),float(recess.max())],
        protected_surface_displacement_m=float(np.max(np.abs(candidate[protected]-v[protected]))),
        locally_limited_vertices=int(damped.sum()),
        limiter_affected_vertices=int(np.count_nonzero(limiter<1.)),
        limiter_geodesic_radius_m=6.,
        top_guard='Actual per-column curved topV,0..9m inward fade; all open boundary vertices fixed',
        refined_open_boundary_vertices=int(len(boundary)),
        boundary_displacement_m=float(np.max(abs(candidate[boundary]-v[boundary]))),
        min_twice_area_m2=float(area.min()),opposing_source_normals=int(np.count_nonzero(turns<=0.)),
        vertices=len(v),triangles=len(t),visual_accepted=False,
        limitations='Local triangle orientation check, not exhaustive global self-intersection proof. No physical erosion or structural fracture simulation.')
    a.output.mkdir(parents=True)
    common={k:data[k] for k in ('frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world',
        'contact_uv_m','contact_water_height_m','lower_overlap_uv_m','lower_overlap_ground_height_m') if k in data}
    for name,points in (('baseline',v),('candidate',candidate)):
        np.savez_compressed(a.output/(name+'.npz'),vertices_m=origin+points@basis.T,
            triangles=t,face_kind=selected[lineage],recess_m=recess if name=='candidate' else np.zeros(len(v)),
            source_triangle_index=lineage,guard_weight=weights,limiter=limiter,
            metadata_json=np.asarray(json.dumps(metadata)),**common)
    (a.output/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(metadata,indent=2))


if __name__=='__main__':
    main()
