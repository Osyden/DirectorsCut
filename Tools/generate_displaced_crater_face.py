"""Refine the measured crater cut and recess its exposed face behind the shore.

Uses an externally verified, linear, physical-scale rock height map. Macro
recess locations come from low points of the actual parent/cut intersection.
No parent erosion, domain scaling, synthetic rock noise or runtime mutation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import numpy as np

sys.dont_write_bytecode = True


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    with np.load(path, allow_pickle=False) as z:
        return {k: z[k].copy() for k in z.files}


def edges_of(t):
    return np.concatenate((t[:, [0, 1]], t[:, [1, 2]], t[:, [2, 0]]))


def refine(vertices, triangles, kinds, target):
    """Conforming red/green edge splits; top faces stay exactly coplanar."""
    v = vertices.copy()
    t = triangles.copy()
    lineage = np.arange(len(t), dtype=np.int32)
    patterns = {
        0: [[0, 1, 2]], 1: [[0, 3, 2], [3, 1, 2]],
        2: [[1, 4, 0], [4, 2, 0]], 4: [[2, 5, 1], [5, 0, 1]],
        3: [[1, 4, 3], [0, 3, 2], [3, 4, 2]],
        6: [[2, 5, 4], [1, 4, 0], [4, 5, 0]],
        5: [[0, 3, 5], [2, 5, 1], [5, 3, 1]],
        7: [[0, 3, 5], [3, 1, 4], [5, 4, 2], [3, 4, 5]],
    }
    for iteration in range(12):
        e, inv = np.unique(np.sort(edges_of(t), axis=1), axis=0, return_inverse=True)
        ids = inv.reshape(3, len(t)).T
        length = np.linalg.norm(v[e[:, 1]] - v[e[:, 0]], axis=1)
        mark = np.zeros(len(e), bool)
        selected = ids[kinds[lineage] == 1].ravel()
        mark[selected[length[selected] > target * (1. + 1e-10)]] = True
        if not mark.any():
            return v, t, lineage, iteration
        chosen = np.flatnonzero(mark)
        mid = np.full(len(e), -1, np.int32)
        mid[chosen] = np.arange(len(v), len(v) + len(chosen))
        v = np.concatenate((v, v[e[chosen]].mean(axis=1)))
        code = (mark[ids] * np.array([1, 2, 4])).sum(axis=1)
        table = np.concatenate((t, mid[ids]), axis=1)
        nt, nl = [], []
        for mask, pattern in patterns.items():
            rows = np.flatnonzero(code == mask)
            if not len(rows):
                continue
            p = np.asarray(pattern)
            nt.append(table[rows][:, p].reshape(-1, 3))
            nl.append(np.repeat(lineage[rows], len(pattern)))
        t = np.concatenate(nt).astype(np.int32)
        lineage = np.concatenate(nl)
        if len(t) > 4000000:
            raise ValueError('Refinement exceeds bounded four-million-face proof')
    raise ValueError('Refinement did not reach target')


def segment_distance(points, segments, limit):
    """Exact clipped segment distance using bounded physical spatial buckets."""
    result = np.full(len(points), limit)
    buckets = {}
    cell = limit
    for i, segment in enumerate(segments):
        lo = np.floor((segment.min(axis=0) - limit) / cell).astype(int)
        hi = np.floor((segment.max(axis=0) + limit) / cell).astype(int)
        for u in range(lo[0], hi[0] + 1):
            for z in range(lo[1], hi[1] + 1):
                buckets.setdefault((u, z), []).append(i)
    cells, inv = np.unique(np.floor(points / cell).astype(np.int32), axis=0, return_inverse=True)
    order = np.argsort(inv)
    starts = np.r_[0, np.cumsum(np.bincount(inv, minlength=len(cells)))]
    for k, key in enumerate(cells):
        selection = buckets.get(tuple(key))
        if selection is None:
            continue
        rows = order[starts[k]:starts[k + 1]]
        seg = segments[selection]
        a, direction = seg[:, 0], seg[:, 1] - seg[:, 0]
        length2 = (direction * direction).sum(axis=1)
        for offset in range(0, len(rows), 2048):
            r = rows[offset:offset + 2048]
            delta = points[r, None, :] - a
            fraction = np.clip((delta * direction).sum(axis=2) / np.maximum(length2, 1e-20), 0., 1.)
            distance = np.linalg.norm(delta - fraction[:, :, None] * direction, axis=2)
            result[r] = np.minimum(limit, distance.min(axis=1))
    return result


def smoothstep(x):
    x = np.clip(x, 0., 1.)
    return x * x * (3. - 2. * x)


def sample_parent(parent, uv):
    """Exact original ABD/ADC triangles after HeadlandA's rigid translation."""
    h = parent['height_m']
    col = uv[:, 0] * .5
    row = (uv[:, 1] + 600.) * .5
    c, r = np.floor(col).astype(int), np.floor(row).astype(int)
    if np.any(c < 0) or np.any(c >= h.shape[1] - 1) or np.any(r < 0) or np.any(r >= h.shape[0] - 1):
        raise ValueError('Recession escapes original parent coverage')
    x, y = col - c, row - r
    a, b, d, e = h[r, c], h[r, c + 1], h[r + 1, c + 1], h[r + 1, c]
    return np.where(x >= y, a * (1. - x) + b * (x - y) + d * y,
                    a * (1. - y) + d * x + e * (y - x)) - 55.


def choose_gullies(parent, contacts):
    """Unequal troughs are measured from the retained parent, not random slots."""
    u = contacts[:, 0]
    shore_v, shore_z = contacts[:, 1], contacts[:, 2]
    slope = np.tan(np.deg2rad(70.))
    v = shore_v[:, None] + np.arange(0., 180.01, .5)[None, :]
    top = sample_parent(parent, np.c_[np.repeat(u, v.shape[1]), v.ravel()]).reshape(v.shape)
    cut = shore_z[:, None] + slope * (v - shore_v[:, None])
    crossed = top <= cut
    if not crossed.any(axis=1).all():
        raise ValueError('Retained top intersection not found')
    j = np.argmax(crossed, axis=1)
    before = np.maximum(j - 1, 0)
    i = np.arange(len(u))
    d0, d1 = (top - cut)[i, before], (top - cut)[i, j]
    f = d0 / np.maximum(d0 - d1, 1e-20)
    crest_v = v[i, before] + .5 * f
    crest_z = shore_z + slope * (crest_v - shore_v)
    # Six-metre average suppresses only grid cadence; feature selection is30–80m.
    filtered = np.convolve(np.pad(crest_z, (2, 2), mode='edge'), np.ones(5) / 5., mode='valid')
    # Include actual drainage crossings, not absolute crest altitude alone.
    flow = parent['flow_accumulation']
    flow_r = np.rint((crest_v + 600.) / 2.).astype(int)
    flow_c = np.rint(u / 2.).astype(int)
    drainage = np.zeros(len(u))
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            drainage = np.maximum(drainage, flow[flow_r + dr, flow_c + dc])
    trend = np.convolve(np.pad(filtered, (25, 25), mode='edge'), np.ones(51) / 51., mode='valid')
    choices = []
    for k in range(12, len(u) - 12):
        low = filtered[k] == filtered[k - 4:k + 5].min()
        stream = drainage[k] == drainage[k - 4:k + 5].max() and drainage[k] >= 512. and filtered[k] < trend[k] - 3.
        if not (low or stream):
            continue
        left = max(0, k - 22) + np.argmax(filtered[max(0, k - 22):k])
        right = k + 1 + np.argmax(filtered[k + 1:min(len(u), k + 23)])
        prominence = min(filtered[left], filtered[right]) - filtered[k]
        if prominence < 5. and not stream:
            continue
        importance = float(prominence + 3. * np.log2(1. + drainage[k] / 512.))
        choices.append((importance, float(prominence), k, left, right))
    chosen = []
    for importance, prominence, k, left, right in sorted(choices, reverse=True):
        if any(abs(u[k] - q['u_m']) < 30. for q in chosen):
            continue
        # Actual valley direction just behind the cut provides the lateral drift.
        offsets = np.arange(-14., 14.01, 2.)
        back_v = crest_v[k] + 16.
        local_h = sample_parent(parent, np.c_[u[k] + offsets, np.full(len(offsets), back_v)])
        upstream_u = u[k] + offsets[np.argmin(local_h)]
        drift = float(np.clip((u[k] - upstream_u) / 40., -.22, .22))
        # The explicit guard already tapers this field. Do not budget it twice.
        available = float(crest_z[k] - shore_z[k])
        stream_strength = float(np.log2(1. + drainage[k] / 256.))
        length = min(float(np.clip(48. + 8. * stream_strength, 60., 100.)), available - 4.)
        if length < 12.:
            continue
        chosen.append(dict(u_m=float(u[k]), crest_v_m=float(crest_v[k]), crest_z_m=float(crest_z[k]),
                           prominence_m=prominence,
                           drainage_area_m2=float(drainage[k]), selection_importance=importance,
                           half_width_left_m=float(np.clip((u[k] - u[left]) * .52, 8., 30.)),
                           half_width_right_m=float(np.clip((u[right] - u[k]) * .52, 8., 30.)),
                           available_face_height_m=available, length_m=length,
                           recession_m=float(min(.2 * available, np.clip(5. + 2.5 * stream_strength, 10., 20.))), drift_u_per_z=drift,
                           left_spur_u_m=float(u[left]), right_spur_u_m=float(u[right])))
        if len(chosen) == 8:
            break
    return sorted(chosen, key=lambda a: a['u_m']), np.c_[u, crest_v, crest_z]


def sample_texture(height, uv):
    """Periodic bilinear data sampling; array row0 is documented by source bundle."""
    ny, nx = height.shape
    x, y = np.mod(uv[:, 0], 1.) * nx, np.mod(uv[:, 1], 1.) * ny
    i, j = np.floor(x).astype(int), np.floor(y).astype(int)
    a, b = x - i, y - j
    return ((1. - a) * (1. - b) * height[j % ny, i % nx]
            + a * (1. - b) * height[j % ny, (i + 1) % nx]
            + (1. - a) * b * height[(j + 1) % ny, i % nx]
            + a * b * height[(j + 1) % ny, (i + 1) % nx])


def filter_height_to_mesh(height, width, height_m, edge):
    """Periodic raised-cosine lowpass in physical cycles/metre, not pixel blur."""
    ny, nx = height.shape
    fx = np.fft.rfftfreq(nx, width / nx)
    fy = np.fft.fftfreq(ny, height_m / ny)
    frequency = np.sqrt(fy[:, None] ** 2 + fx[None, :] ** 2)
    cutoff = 1. / (2. * edge)
    t = np.clip((frequency - .5 * cutoff) / (.5 * cutoff), 0., 1.)
    response = .5 + .5 * np.cos(np.pi * t)
    filtered = np.fft.irfft2(np.fft.rfft2(height) * response, s=height.shape).real
    return filtered, dict(kind='periodic_radial_raised_cosine', passband_cycles_per_m=.5 * cutoff,
                          stopband_cycles_per_m=cutoff, target_max_edge_m=edge,
                          original_range=[float(height.min()), float(height.max())],
                          filtered_range=[float(filtered.min()), float(filtered.max())])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, required=True)
    p.add_argument('--heightmap', type=Path, required=True, help='Verified linear scalar height NPY or NPZ:height')
    p.add_argument('--provenance', type=Path, required=True)
    p.add_argument('--width-m', type=float, required=True)
    p.add_argument('--height-m', type=float, required=True)
    p.add_argument('--relief-m', type=float, required=True, help='Physical relief per scalar0..1, no scan upscaling')
    p.add_argument('--edge-m', type=float, default=.5)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    if a.output.exists() or 'saved' not in [part.lower() for part in a.output.resolve().parts]:
        p.error('Use a new ignored Saved output')
    if min(a.width_m, a.height_m, a.relief_m, a.edge_m) <= 0. or a.relief_m > 1.:
        p.error('Positive physical dimensions and bounded <=1m scan relief required')
    src = load(a.input)
    src_meta = json.loads(str(src['metadata_json']))
    if src_meta.get('schema') != 'directorscut.historical_crater_cut.v1' or src_meta['variant'] != 'visible_cut_surface':
        p.error('This proof accepts the immutable HeadlandA visible cut schema')
    parent_path = Path(src_meta['parent_path'])
    if sha(parent_path) != src_meta['parent_sha256']:
        raise ValueError('Original parent changed')
    parent = load(parent_path)
    texture = (np.load(a.heightmap, allow_pickle=False) if a.heightmap.suffix.lower() == '.npy'
               else load(a.heightmap)['height']).astype(float)
    if texture.ndim != 2 or not np.isfinite(texture).all() or texture.min() < 0. or texture.max() > 1.:
        raise ValueError('Expected finite verified linear0..1 height without renormalization')
    texture, texture_filter = filter_height_to_mesh(texture, a.width_m, a.height_m, a.edge_m)
    provenance = json.loads(a.provenance.read_text(encoding='utf-8-sig'))
    origin = src['frame_origin_world_m']
    basis = np.stack([src['basis_u_world'], src['basis_v_world'], src['basis_z_world']], axis=1)
    local_source = (src['vertices_m'] - origin) @ basis
    original_tri = src['triangles']
    kind = src['face_kind']
    baseline_world, triangles, lineage, passes = refine(src['vertices_m'], original_tri, kind, a.edge_m)
    local = (baseline_world - origin) @ basis
    e, inv, counts = np.unique(np.sort(edges_of(original_tri), axis=1), axis=0, return_inverse=True, return_counts=True)
    ids = inv.reshape(3, len(original_tri)).T
    cut_use, top_use = np.zeros(len(e), bool), np.zeros(len(e), bool)
    cut_use[ids[kind == 1].ravel()] = True
    top_use[ids[kind == 0].ravel()] = True
    guard_edges = e[cut_use & (top_use | (counts == 1))]
    segments = local_source[guard_edges][:, :, [0, 2]]
    contact = np.c_[src['contact_uv_m'][:, 0], src['contact_water_height_m']]
    segments = np.concatenate((segments, np.stack((contact[:-1], contact[1:]), axis=1)))
    distance = segment_distance(local[:, [0, 2]], segments, 8.)
    guard = smoothstep((distance - 2.) / 6.)
    top_vertices = np.unique(triangles[kind[lineage] == 0])
    guard[top_vertices] = 0.
    gullies, crest = choose_gullies(parent, np.c_[src['contact_uv_m'], src['contact_water_height_m']])
    macro = np.zeros(len(local))
    for g in gullies:
        down = g['crest_z_m'] - local[:, 2]
        t = down / g['length_m']
        center = g['u_m'] + g['drift_u_per_z'] * np.clip(down, 0., g['length_m'])
        offset = local[:, 0] - center
        width = np.where(offset < 0., g['half_width_left_m'], g['half_width_right_m'])
        # Unequal planar shoulders, only a narrow edge blend; no U-shaped slots.
        linear_cross = np.clip(1. - np.abs(offset) / width, 0., 1.)
        cross = linear_cross * smoothstep(linear_cross / .15)
        linear_down = np.clip(1. - t, 0., 1.)
        vertical = smoothstep(down / 8.) * linear_down * smoothstep(linear_down / .15)
        macro = np.maximum(macro, g['recession_m'] * cross * vertical)
    tex_uv = np.c_[local[:, 0] / a.width_m, local[:, 2] / a.height_m]
    # Higher scan samples protrude relative to its valleys while ALL offsets
    # remain landward of the original cut. No arbitrary range stretching.
    scan = a.relief_m * (float(texture.max()) - sample_texture(texture, tex_uv))
    desired = guard * (macro + scan)
    # Parent is a real measured triangle surface. Restrict recession before the
    # first landward crossing, rather than letting the new face pierce a ravine.
    allowed = desired.copy()
    active = np.flatnonzero(desired > 0.)
    collided = np.zeros(len(local), bool)
    for fraction in np.linspace(.125, 1., 8):
        rows = active[~collided[active]]
        q = local[rows, :2].copy()
        q[:, 1] += desired[rows] * fraction
        bad = sample_parent(parent, q) < local[rows, 2] + .05
        hit = rows[bad]
        if not len(hit):
            continue
        lo, hi = desired[hit] * (fraction - .125), desired[hit] * fraction
        for _ in range(24):
            mid = (lo + hi) * .5
            q = local[hit, :2].copy()
            q[:, 1] += mid
            below = sample_parent(parent, q) >= local[hit, 2] + .05
            lo = np.where(below, mid, lo)
            hi = np.where(below, hi, mid)
        allowed[hit] = lo
        collided[hit] = True
    moved = baseline_world + allowed[:, None] * basis[:, 1]
    # One exact input-triangle/barycentric correspondence per refined vertex.
    _, first = np.unique(triangles.ravel(), return_index=True)
    owner = lineage[(first // 3)]
    source_tri = src['vertices_m'][original_tri[owner]]
    ab, ac = source_tri[:, 1] - source_tri[:, 0], source_tri[:, 2] - source_tri[:, 0]
    delta = baseline_world - source_tri[:, 0]
    aa, bb, cc = (ab * ab).sum(1), (ab * ac).sum(1), (ac * ac).sum(1)
    ad, cd = (ab * delta).sum(1), (ac * delta).sum(1)
    det = aa * cc - bb * bb
    b1, b2 = (cc * ad - bb * cd) / det, (aa * cd - bb * ad) / det
    bary = np.c_[1. - b1 - b2, b1, b2]
    meta = dict(schema='directorscut.displaced_crater_face.v1', source_kind='scan_displaced_historical_crater_face',
                source_path=str(a.input.resolve()), source_sha256=sha(a.input), parent_path=str(parent_path),
                parent_sha256=sha(parent_path), height_path=str(a.heightmap.resolve()), height_sha256=sha(a.heightmap),
                provenance_path=str(a.provenance.resolve()), provenance_sha256=sha(a.provenance), source_provenance=provenance,
                generator_sha256=sha(__file__), scan_size_m=[a.width_m, a.height_m], scan_relief_m=a.relief_m,
                revision='B_drainage_guides_and_physical_lowpass', height_filter=texture_filter,
                scan_sampling='Periodic bilinear linear data bottom-to-top rows, U/Z physical axes, max-height landward translation; no range normalization or physical upscaling. Relief scale is upstream authored material setting, not survey calibration.',
                macro_gullies=gullies, macro_source='Parent/cut seam troughs plus original drainage crossings; original upstream trough direction; explicit guard applied once',
                displacement='Nonnegative landward localV; U/Z fixed; bounded before first sampled parent intersection',
                guard='Exact UZ segment distance to shared top seam, open crop edge and201 measured shoreline controls; zero<=2m, smooth2..8m',
                target_edge_m=a.edge_m, refinement_passes=passes, original_top_unchanged=True,
                metrics=dict(vertices=len(moved), triangles=len(triangles), moved_vertices=int((allowed > 0.).sum()),
                             parent_limited_vertices=int(collided.sum()), max_recession_m=float(allowed.max()),
                             gully_count=len(gullies), exact_fixed_vertices=int((allowed == 0.).sum())),
                modern_contact_pending_independent_check=True, temple_support_approved=False, runtime_integrated=False,
                visual_accepted=False, limitations='Finite scanned relief cannot establish fullcliff geology. Periodic repeat remains reviewable. No sediment simulation or whole-route validation.')
    a.output.mkdir(parents=True)
    np.save(a.output / 'filtered_height.npy', texture.astype(np.float32), allow_pickle=False)
    common = dict(triangles=triangles, face_kind=kind[lineage], source_face_index=lineage,
                  source_triangle_index=owner, source_barycentric=bary, baseline_vertices_m=baseline_world,
                  guard_weight=guard, guard_distance_m=distance, landward_displacement_m=allowed,
                  requested_displacement_m=desired, macro_recession_m=macro, scan_recession_m=scan,
                  parent_limited_mask=collided, crest_uvz_m=crest,
                  frame_origin_world_m=origin, basis_u_world=basis[:, 0], basis_v_world=basis[:, 1], basis_z_world=basis[:, 2])
    np.savez_compressed(a.output / 'candidate.npz', vertices_m=moved, metadata_json=np.asarray(json.dumps(meta)), **common)
    np.savez_compressed(a.output / 'baseline.npz', vertices_m=baseline_world,
                        metadata_json=np.asarray(json.dumps(dict(meta, variant='same_topology_undisplaced'))), **common)
    (a.output / 'manifest.json').write_text(json.dumps(meta, indent=2) + '\n', encoding='utf-8')
    shutil.copyfile(__file__, a.output / 'generator_source.py')
    print(json.dumps(dict(output=str(a.output.resolve()), metrics=meta['metrics']), indent=2))


if __name__ == '__main__':
    main()
