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


def uv_at(world, start, width, height):
    angle = np.mod(np.arctan2(world[:, 1], world[:, 0]), 2*np.pi)
    arc = (angle-start*2*np.pi/1536)*1600.
    z = world[:, 2]
    return np.c_[(arc+1.6*np.sin(z/37.))/width, (z+1.1*np.sin(arc/61.))/height]


def lookup(tile, uv):
    # Samples at pixel/grid centres, periodic on both axes. UV+V is source+Z.
    x = np.mod(uv[:, 0], 1)*tile.shape[1]-.5
    y = np.mod(uv[:, 1], 1)*tile.shape[0]-.5
    ix, iy = np.floor(x).astype(int), np.floor(y).astype(int)
    fx, fy = x-ix, y-iy
    return (tile[iy % tile.shape[0], ix % tile.shape[1]]*(1-fx)*(1-fy)
        +tile[iy % tile.shape[0], (ix+1) % tile.shape[1]]*fx*(1-fy)
        +tile[(iy+1) % tile.shape[0], ix % tile.shape[1]]*(1-fx)*fy
        +tile[(iy+1) % tile.shape[0], (ix+1) % tile.shape[1]]*fx*fy)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ring', type=Path, required=True)
    p.add_argument('--tile', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--tile-width-m', type=float, default=7.9875)
    p.add_argument('--tile-height-m', type=float, default=5.5125)
    p.add_argument('--edge-m', type=float, default=.85)
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
    angle = col*2*np.pi/1536
    macro = np.c_[np.cos(angle)*radius, np.sin(angle)*radius, z]
    back = row>120
    b = np.clip((row-120)/40., 0, 1)
    top = np.c_[np.cos(angle)*(toe_radius+setback), np.sin(angle)*(toe_radius+setback), crest]
    macro[back] = (top+(end-top)*b[:, None])[back]
    macro[back, 2] = (crest+(end[:, 2]-crest)*smooth((b-.72)/.28))[back]
    side = smooth(np.minimum(u, span-u)/28.)
    shape = old+(macro-old)*side[:, None]
    fixed = (row<=8+1e-10)|(row>=160-1e-10)|(col<=c0+1e-10)|(col>=c1-1e-10)
    shape[fixed] = old[fixed]
    uv = uv_at(shape, c0, a.tile_width_m, a.tile_height_m)
    mask = smooth(t/.13)*smooth((1-t)/.08)*side
    mask[(row>=120)|fixed] = 0.
    scan = lookup(depth, uv)*mask
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
    a.output.mkdir(parents=True)
    for name, v in [('baseline',old),('candidate',final)]:
        np.savez_compressed(a.output/(name+'.npz'),vertices_m=v,triangles=triangles,uv0=uv,
            metadata_json=np.asarray(json.dumps(meta)),**frame)
    splice_full_ring(ring_data, final, triangles, meta, a.output/'full_ring_candidate.npz')
    full_path = a.output/'full_ring_candidate.npz'
    with np.load(full_path, allow_pickle=False) as f:
        full = {k:f[k].copy() for k in f.files}
    full['uv0'] = uv_at(full['vertices_m'],c0,a.tile_width_m,a.tile_height_m).astype(np.float32)
    fv = full['vertices_m']
    fc = np.mod(np.arctan2(fv[:,1],fv[:,0]),2*np.pi)*1536/(2*np.pi)
    fu = (fc-c0)*arc_step
    ftz = sample_ring(ring,np.full(len(fv),8.),fc)[:,2]
    fcrest = curve(fu,knots,np.array([37.,43.,39.,47.,42.,36.]))
    ft = np.clip((fv[:,2]-ftz)/np.maximum(fcrest-ftz,1.),0,1)
    fm = smooth(ft/.13)*smooth((1-ft)/.08)*smooth(np.minimum(fu,span-fu)/28.)
    rgba = np.zeros((len(fv),4),np.float32);rgba[:,0]=fm;rgba[:,3]=1.
    full['vertex_rgba'] = rgba
    np.savez_compressed(full_path, **full)
    (a.output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(json.dumps(dict(output=str(a.output),vertices=len(final),triangles=len(triangles),**meta),indent=2))


if __name__ == '__main__':main()
