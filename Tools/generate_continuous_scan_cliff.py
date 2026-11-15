"""One connected crater cliff with measured, physically scaled scan relief.

No rock instances, stacked scan aprons, erosion solver, or Boolean panel field.
Keeps the canonical ring perimeter/toe; old waterfall attachment is invalid.
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
from generate_jointed_headland import sample_ring
from generate_rock_buttress import splice_full_ring


def smooth(x):
    t = np.clip(x, 0, 1)
    return t*t*(3-2*t)


def curve(u, knots, values):
    k = np.clip(np.searchsorted(knots, u)-1, 0, len(knots)-2)
    t = smooth((u-knots[k])/(knots[k+1]-knots[k]))
    return values[k]*(1-t)+values[k+1]*t


def macro_profile(u, t, toe_radius, span):
    knots = np.array([0,.10,.22,.37,.48,.63,.77,.88,1])*span
    crest = curve(u,knots,np.array([38.,52.,44.,59.,47.,54.,39.,46.,35.]))
    setback = curve(u,knots,np.array([88.,59.,94.,52.,84.,58.,96.,66.,84.]))
    # Broad masses remain rooted: width increases toward the receiving toe,
    # and shoulder relief starts gradually instead of ending as a hanging fin.
    radius = toe_radius+setback*(.65*t+.35*smooth(t))
    for centre,width,depth,lean in [(86,66,10,-12),(249,89,16,16),(408,59,12,-9)]:
        varying_width=width*(1.40-.40*t)
        shoulder=smooth(1-abs(u-centre-lean*(t-.5))/varying_width)
        radius-=depth*shoulder*smooth(t/.65)
    # Open gullies cut through the crest; none is capped into an arch/slot.
    for centre,width,depth,lean in [(143,17,10,12),(326,24,15,-17),(454,13,8,8)]:
        gully=smooth(1-abs(u-centre-lean*(t-.3))/width)
        radius+=depth*gully*smooth(t/.38)
        crest-=5*smooth(1-abs(u-centre-lean*.7)/width)
    # Two unequal finite oblique shelves. They recess into connected rock and
    # terminate gradually across U, rather than becoming repeated wall bands.
    for centre,width,depth,level,lean in [(178,61,5,.56,.0009),(370,47,7,.36,-.0011)]:
        extent=smooth(1-abs(u-centre)/width)
        radius+=depth*extent*smooth((t-level-lean*(u-centre))/.055)
    return radius,crest


def uv_at(world, start, width, height, unique=False, crest_radius=None):
    angle = np.mod(np.arctan2(world[:, 1], world[:, 0]), 2*np.pi)
    arc = (angle-start*2*np.pi/1536)*1600.
    z = world[:, 2]
    if unique:
        landward=0. if crest_radius is None else np.maximum(0.,np.linalg.norm(world[:,:2],axis=1)-crest_radius)
        return np.c_[arc/width,(z+.35*landward+200.)/height]
    return np.c_[(arc+1.6*np.sin(z/37.))/width, (z+1.1*np.sin(arc/61.))/height]


def lookup(tile, uv, clamp=False):
    # Samples at pixel/grid centres, periodic on both axes. UV+V is source+Z.
    x = (np.clip(uv[:,0],0,1) if clamp else np.mod(uv[:, 0], 1))*tile.shape[1]-.5
    y = (np.clip(uv[:,1],0,1) if clamp else np.mod(uv[:, 1], 1))*tile.shape[0]-.5
    if clamp:
        x=np.clip(x,0,tile.shape[1]-1);y=np.clip(y,0,tile.shape[0]-1)
    ix, iy = np.floor(x).astype(int), np.floor(y).astype(int)
    fx, fy = x-ix, y-iy
    jx=np.minimum(ix+1,tile.shape[1]-1) if clamp else (ix+1)%tile.shape[1]
    jy=np.minimum(iy+1,tile.shape[0]-1) if clamp else (iy+1)%tile.shape[0]
    return (tile[iy % tile.shape[0], ix % tile.shape[1]]*(1-fx)*(1-fy)
        +tile[iy % tile.shape[0], jx]*fx*(1-fy)
        +tile[jy, ix % tile.shape[1]]*(1-fx)*fy
        +tile[jy, jx]*fx*fy)


def intermediate_relief(tile, u, z):
    """Authored 15–50 m rock forms from three nonrepeating scan windows.

    This explicitly enlarges measured geometry, not the fine PBR textures.
    Positive values project toward the crater; the underlying U/Z chart stays
    single-valued so radial setbacks can form overhangs without folded strips.
    """
    result = np.zeros_like(u, dtype=float)
    for scale, degrees, ox, oz, weight in [(3.2,17,82,-58,.72),
            (5.8,-23,241,-45,.40),(2.1,8,115,-82,.18)]:
        angle = np.deg2rad(degrees)
        sx = ox+(u*np.cos(angle)+z*np.sin(angle))/scale
        sz = oz+(-u*np.sin(angle)+z*np.cos(angle))/scale
        result += weight*lookup(tile,np.c_[sx/497.418836818384,(sz+200)/280],True)
    return np.clip(result*22.,-12.,12.)


def faceted_relief(tile, u, z):
    """Fit raw scanned shape to unequal planar faces, with narrow joints."""
    gx,gz=np.meshgrid(np.arange(-2,25),np.arange(-12,7))
    rng=np.random.default_rng(721983)
    sites=np.c_[gx.ravel()*24+12,gz.ravel()*24+12]+rng.uniform(-10.,10.,(gx.size,2))
    dx,dz=np.meshgrid([-8.,0.,8.],[-8.,0.,8.])
    offsets=np.c_[dx.ravel(),dz.ravel()]
    samples=sites[:,None,:]+offsets[None,:,:]
    h=intermediate_relief(tile,samples[:,:,0].ravel(),samples[:,:,1].ravel()).reshape(-1,9)
    fit=np.linalg.lstsq(np.c_[np.ones(9),offsets],h.T,rcond=None)[0].T
    fit[:,1:]=np.clip(fit[:,1:],-1.,1.)
    # Only the pilot needs this field. Clamp remote unchanged ring queries.
    qu=np.clip(u,-24.,552.);qz=np.clip(z,-264.,120.)
    ix=np.clip(np.floor(qu/24).astype(int),-1,23)
    iz=np.clip(np.floor(qz/24).astype(int),-11,5)
    candidates=[];distances=[];values=[]
    for oz in [-1,0,1]:
        for ox in [-1,0,1]:
            ids=(iz+oz+12)*27+(ix+ox+2)
            du=qu-sites[ids,0];dv=qz-sites[ids,1]
            distances.append(du*du+dv*dv)
            values.append(fit[ids,0]+fit[ids,1]*du+fit[ids,2]*dv)
    ds=np.asarray(distances);vs=np.asarray(values)
    # Equal-distance boundaries blend over about one physical metre, not a
    # Gaussian spanning each face. Stable soft weights keep triple joints continuous.
    w=np.exp(np.maximum(-60.,-(ds-ds.min(axis=0))/(2*24.*1.)))
    return np.clip(np.sum(w*vs,axis=0)/np.sum(w,axis=0),-12.,12.)


def midscale_profile(tile, u, t, toe_radius, toe_z, span, faceted=False):
    field=faceted_relief if faceted else intermediate_relief
    radius, crest = macro_profile(u,t,toe_radius,span)
    crest_radius, _ = macro_profile(u,np.ones_like(t),toe_radius,span)
    # A small scan-derived crest variation participates in the real silhouette.
    crest += .30*field(tile,u,crest)
    z = toe_z+(crest-toe_z)*t
    relief = field(tile,u,z)*smooth(t/.16)
    radius -= relief
    crest_radius -= field(tile,u,crest)
    return radius,crest,crest_radius,relief


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ring', type=Path, required=True)
    p.add_argument('--tile', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--tile-width-m', type=float, default=7.9875)
    p.add_argument('--tile-height-m', type=float, default=5.5125)
    p.add_argument('--edge-m', type=float, default=.85)
    p.add_argument('--unique-atlas', action='store_true', help='One full-pilot U/Z atlas, Z range−200..80m')
    p.add_argument('--midscale-rock', action='store_true', help='Add nonperiodic scan-derived intermediate rock forms')
    p.add_argument('--faceted-rock', action='store_true', help='Fit raw scan medium forms to irregular planar faces')
    a = p.parse_args()
    if a.output.exists() or 'saved' not in [x.lower() for x in a.output.resolve().parts]:
        p.error('Use a new ignored Saved directory')
    ring_data = np.load(a.ring, allow_pickle=False)
    ring = ring_data['vertices_m'].reshape(161, 1537, 3)
    tile_data = np.load(a.tile, allow_pickle=False)
    tile = np.asarray(tile_data['depth_m'], float)
    if tile.ndim != 2 or not np.isfinite(tile).all():
        raise ValueError('Scan tile must be finite and fully covered')
    # Mesh sampling cannot represent the texture's sub-metre frequencies.
    # Source normal textures supply fine detail; this discarded geometric
    # residual is not automatically reconstructed by those normal textures.
    fy = np.fft.fftfreq(tile.shape[0], a.tile_height_m/tile.shape[0])
    fx = np.fft.fftfreq(tile.shape[1], a.tile_width_m/tile.shape[1])
    kernel = np.exp(-2*np.pi*np.pi*.35**2*(fy[:, None]**2+fx[None, :]**2))
    depth = np.fft.ifft2(np.fft.fft2(tile)*kernel).real
    depth -= depth.mean()
    c0, c1 = 529, 605
    count = c1-c0+1
    rr, cc = np.meshgrid(np.arange(161), np.arange(c0, c1+1), indexing='ij')
    arc_step = 1600*2*np.pi/1536
    chart = np.c_[(cc.ravel()-c0)*arc_step, rr.ravel()*1.55, np.zeros(rr.size)]
    ids = (np.arange(160)[:, None]*count+np.arange(count-1)).ravel()
    triangles = np.concatenate((np.c_[ids, ids+count, ids+1], np.c_[ids+1, ids+count, ids+count+1])).astype(np.int32)
    rows, cols = rr.ravel()[triangles], cc.ravel()[triangles]
    selected = ((rows.min(1)>8)&(rows.max(1)<122)&(cols.min(1)>c0)&(cols.max(1)<c1)).astype(np.int8)
    chart, triangles, lineage, passes = refine(chart, triangles, selected, a.edge_m)
    u, row = chart[:, 0], chart[:, 1]/1.55
    col = c0+u/arc_step
    old = sample_ring(ring, row, col)
    toe = sample_ring(ring, np.full(len(row), 8.), col)
    end = sample_ring(ring, np.full(len(row), 160.), col)
    toe_radius = np.linalg.norm(toe[:, :2], axis=1)
    span = (c1-c0)*arc_step
    knots = np.array([0, .16, .35, .55, .76, 1])*span
    crest = curve(u, knots, np.array([37., 43., 39., 47., 42., 36.]))
    setback = curve(u, knots, np.array([79., 62., 76., 67., 82., 75.]))
    t = np.clip((row-8)/112., 0, 1)
    z = toe[:, 2]+(crest-toe[:, 2])*t
    radius = toe_radius+20*smooth(t/.18)+(setback-20)*t
    # Three unequal connected shoulders. Their finite slanting faces terminate
    # at different elevations instead of becoming full-height parallel fins.
    envelope = np.interp(t, [0,.18,.42,.83,1], [0,0,1,1,.15])
    for centre, half_width, amount, lean in [(95,53,13,-17),(258,76,19,12),(411,48,15,-9)]:
        ridge = np.maximum(0., 1-abs(u-centre-lean*(t-.5))/half_width)
        radius -= amount*ridge*envelope
    if a.unique_atlas:
        radius,crest=macro_profile(u,t,toe_radius,span)
        crest_radius,_=macro_profile(u,np.ones_like(t),toe_radius,span)
        if a.midscale_rock:
            radius,crest,crest_radius,mid_relief=midscale_profile(tile if a.faceted_rock else depth,u,t,toe_radius,toe[:,2],span,a.faceted_rock)
        z=toe[:,2]+(crest-toe[:,2])*t
    else:
        crest_radius=toe_radius+setback
    angle = col*2*np.pi/1536
    macro = np.c_[np.cos(angle)*radius, np.sin(angle)*radius, z]
    back = row>120
    b = np.clip((row-120)/40., 0, 1)
    top = np.c_[np.cos(angle)*crest_radius, np.sin(angle)*crest_radius, crest]
    macro[back] = (top+(end-top)*b[:, None])[back]
    macro[back, 2] = (crest+(end[:, 2]-crest)*smooth((b-.72)/.28))[back]
    side = smooth(np.minimum(u, span-u)/28.)
    shape = old+(macro-old)*side[:, None]
    fixed = (row<=8+1e-10)|(row>=160-1e-10)|(col<=c0+1e-10)|(col>=c1-1e-10)
    shape[fixed] = old[fixed]
    uv = uv_at(shape, c0, a.tile_width_m, a.tile_height_m,a.unique_atlas,crest_radius)
    mask = smooth(t/.13)*smooth((1-t)/.08)*side
    mask[(row>=120)|fixed] = 0.
    scan = lookup(depth, uv,a.unique_atlas)*mask
    scan[(row>=120)|fixed] = 0.
    # +Y scan depth is an outward rock protrusion, hence decreases arena radius.
    final = shape.copy()
    final[:, :2] -= np.c_[np.cos(angle), np.sin(angle)]*scan[:, None]
    final[fixed] = old[fixed]
    if not np.array_equal(final[fixed], old[fixed]):
        raise ValueError('Original perimeter/toe moved')
    cross = np.cross(final[triangles[:, 1]]-final[triangles[:, 0]], final[triangles[:, 2]]-final[triangles[:, 0]])
    if not np.isfinite(final).all() or np.any(np.linalg.norm(cross, axis=1)<1e-9):
        raise ValueError('Nonfinite or degenerate continuous surface')
    origin = np.round(ring[4, (c0+c1)//2]).copy(); origin[2] = -25.
    frame = dict(frame_origin_world_m=origin, basis_u_world=np.array([1.,0,0]),
                 basis_v_world=np.array([0.,1,0]), basis_z_world=np.array([0.,0,1]))
    meta = dict(schema='directorscut.continuous_scan_cliff.v1', source_ring=str(a.ring.resolve()),
        ring_sha256=hashlib.sha256(a.ring.read_bytes()).hexdigest(), scan_tile=str(a.tile.resolve()),
        tile_sha256=hashlib.sha256(a.tile.read_bytes()).hexdigest(),
        canonical_crop=dict(row_start=0,row_end=160,column_start=c0,column_end=c1),
        physical_arc_width_m=span, mesh_edge_target_m=a.edge_m, refinement_passes=passes,
        tile_width_m=a.tile_width_m,tile_height_m=a.tile_height_m, geometry_lowpass_sigma_m=.35,
        tile_depth_range_m=[float(depth.min()),float(depth.max())], scan_applied_range_m=[float(scan.min()),float(scan.max())],
        exact_fixed_vertices=int(fixed.sum()), uv_mapping='U=(arc+1.6sin(Z/37))/width,V=(Z+1.1sin(arc/61))/height; +V=source+Z; periodic pixel centres',
        existing_waterfall_attachment_valid=False, visual_accepted=False,
        limitations='New authored continuous wall with measured scan relief, not simulated geology. Existing falls require rebinding. Original outer/toe coordinates retained; no full dynamic-route/collision certification.')
    if a.unique_atlas:
        meta['uv_mapping']='U=arc/497.418836818384; V=(Z+.35*max(0,R−localCrestR)+200)/280; +V=source+Z; no phase or repeated face UV'
        meta['unique_atlas']=True
    if a.midscale_rock:
        meta['midscale_rock']=dict(mapping='intermediate_relief on three unique scan windows; fine PBR unchanged',
            scales=[3.2,5.8,2.1],limit_m=12.,applied_range_m=[float(mid_relief.min()),float(mid_relief.max())],
            crest_height_factor=.30,authoring='Deliberate physical enlargement of scanned geometry, not erosion simulation')
        meta['midscale_rock']['faceted_raw_scan']=a.faceted_rock
    a.output.mkdir(parents=True)
    for name, v in [('baseline',old),('candidate',final)]:
        np.savez_compressed(a.output/(name+'.npz'),vertices_m=v,triangles=triangles,uv0=uv,
            metadata_json=np.asarray(json.dumps(meta)),**frame)
    splice_full_ring(ring_data, final, triangles, meta, a.output/'full_ring_candidate.npz')
    full_path = a.output/'full_ring_candidate.npz'
    with np.load(full_path, allow_pickle=False) as f:
        full = {k:f[k].copy() for k in f.files}
    fv = full['vertices_m']
    fc = np.mod(np.arctan2(fv[:,1],fv[:,0]),2*np.pi)*1536/(2*np.pi)
    fu = (fc-c0)*arc_step
    ftoe = sample_ring(ring,np.full(len(fv),8.),fc)
    ftz = ftoe[:,2]
    fcrest = curve(fu,knots,np.array([37.,43.,39.,47.,42.,36.]))
    ft = np.clip((fv[:,2]-ftz)/np.maximum(fcrest-ftz,1.),0,1)
    fm = smooth(ft/.13)*smooth((1-ft)/.08)*smooth(np.minimum(fu,span-fu)/28.)
    fcr=np.linalg.norm(ftoe[:,:2],axis=1)+curve(fu,knots,np.array([79.,62.,76.,67.,82.,75.]))
    if a.unique_atlas:
        fcr,_=macro_profile(fu,np.ones_like(fu),np.linalg.norm(ftoe[:,:2],axis=1),span)
        if a.midscale_rock:
            _,_,fcr,_=midscale_profile(tile if a.faceted_rock else depth,fu,np.ones_like(fu),np.linalg.norm(ftoe[:,:2],axis=1),ftz,span,a.faceted_rock)
        fm=smooth(np.minimum(fu,span-fu)/28.)
    full['uv0'] = uv_at(fv,c0,a.tile_width_m,a.tile_height_m,a.unique_atlas,fcr).astype(np.float32)
    rgba = np.zeros((len(fv),4),np.float32);rgba[:,0]=fm;rgba[:,3]=1.
    full['vertex_rgba'] = rgba
    np.savez_compressed(full_path, **full)
    (a.output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(json.dumps(dict(output=str(a.output),vertices=len(final),triangles=len(triangles),**meta),indent=2))


if __name__ == '__main__':main()
