"""Compose installed cliff scans over the actual pilot face; writes JSON only.

Bounds come from Unreal importer reports. Horizontal radial rays query original
backing triangles at each elevation, including across every scan's footprint.
This is an authored overlapping assembly, not new terrain or contact geometry.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rotation(pitch, yaw, roll):
    """Column-vector form of Unreal FRotator's native Pitch/Yaw/Roll basis."""
    p, y, r = np.deg2rad([pitch, yaw, roll])
    cp, sp, cy, sy, cr, sr = np.cos(p), np.sin(p), np.cos(y), np.sin(y), np.cos(r), np.sin(r)
    return np.array([[cp*cy, cp*sy, sp],
                     [sr*sp*cy-cr*sy, sr*sp*sy+cr*cy, -sr*cp],
                     [-cr*sp*cy-sr*sy, cy*sr-cr*sp*sy, cr*cp]]).T


class Face:
    def __init__(self, path, low, high):
        data = np.load(path, allow_pickle=False)
        vertices, triangles = data['vertices_m'], data['triangles']
        xyz = vertices[triangles]
        angles = np.mod(np.arctan2(xyz[:, :, 1], xyz[:, :, 0]), 2*np.pi)
        keep = (angles.max(1) >= low-.08) & (angles.min(1) <= high+.08)
        self.ids = np.flatnonzero(keep)
        self.xyz = xyz[keep]
        self.low, self.high = angles[keep].min(1), angles[keep].max(1)
        self.zlow, self.zhigh = self.xyz[:, :, 2].min(1), self.xyz[:, :, 2].max(1)

    def hit(self, angle, height):
        ids = np.flatnonzero((self.low <= angle+1e-9)&(self.high >= angle-1e-9)
                            &(self.zlow <= height+1e-8)&(self.zhigh >= height-1e-8))
        if not len(ids):
            return None
        xyz = self.xyz[ids]
        direction = np.array([np.cos(angle), np.sin(angle), 0.])
        origin = np.array([0., 0., height])
        e1, e2 = xyz[:, 1]-xyz[:, 0], xyz[:, 2]-xyz[:, 0]
        h = np.cross(direction, e2)
        determinant = np.einsum('ij,ij->i', e1, h)
        valid = abs(determinant) > 1e-10
        inv = np.divide(1., determinant, out=np.zeros_like(determinant), where=valid)
        s = origin-xyz[:, 0]
        u = inv*np.einsum('ij,ij->i', s, h)
        q = np.cross(s, e1)
        v = inv*(q@direction)
        distance = inv*np.einsum('ij,ij->i', e2, q)
        valid &= (u >= -1e-7)&(v >= -1e-7)&(u+v <= 1+1e-7)&(distance > 1100)&(distance < 2100)
        if not valid.any():
            return None
        k = np.argmin(np.where(valid, distance, np.inf))
        return float(distance[k]), int(self.ids[ids[k]]), [float(1-u[k]-v[k]), float(u[k]), float(v[k])]


def box_polygon_overlap(lo, hi, polygon):
    corners = np.array([[lo[0], lo[1]], [hi[0], lo[1]], [hi[0], hi[1]], [lo[0], hi[1]]])
    edges = np.roll(polygon, -1, axis=0)-polygon
    axes = np.concatenate((np.eye(2), np.c_[-edges[:, 1], edges[:, 0]]))
    a, b = corners@axes.T, polygon@axes.T
    return not np.any((a.max(0) < b.min(0))|(b.max(0) < a.min(0)))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reports', type=Path, nargs='+', required=True)
    p.add_argument('--surface', type=Path, required=True)
    p.add_argument('--anchors', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--centre-angle-deg', type=float, default=136.4)
    p.add_argument('--width-m', type=float, default=205.)
    p.add_argument('--front-axis', choices=['+y', '-y', '+x', '-x'], default='+y')
    p.add_argument('--maximum', type=int, default=180)
    a = p.parse_args()
    if not 150 <= a.width_m <= 230 or not 1 <= a.maximum <= 256:
        p.error('This pilot is 150–230 m wide with at most256 instances')
    assets = []
    for path in a.reports:
        report = read(path)
        if report.get('schema') != 'directorscut.owned_cliff_import.v1':
            raise ValueError('Expected authoritative owned-cliff importer report')
        for mesh in report['meshes']:
            lo, hi = np.asarray(mesh['bounds_min_cm'])*.01, np.asarray(mesh['bounds_max_cm'])*.01
            if not np.isfinite([lo, hi]).all() or np.any(hi <= lo):
                raise ValueError('Invalid native Unreal mesh bounds')
            assets.append(dict(asset=mesh['asset'], minimum=lo, maximum=hi,
                               centre=(lo+hi)*.5, size=hi-lo, report=str(path.resolve()), report_sha256=sha(path)))
    if not assets:
        raise ValueError('No imported meshes')
    anchors = [(x['id'], np.asarray(x['footprint_xy_m']), float(x['footing_bottom_m']))
               for x in read(a.anchors)['anchors']]
    centre = np.deg2rad(a.centre_angle_deg)
    low, high = centre-a.width_m/3200., centre+a.width_m/3200.
    face = Face(a.surface, low, high)
    # Overlapping masonry-sized courses are staggered geometrically, not an
    # area-weighted scatter. Original scan forms/UVs provide the exposed rock.
    nominal_width = min(x['size'][0 if a.front_axis[-1] == 'y' else 1] for x in assets)*3.3
    step = min(29., nominal_width*.60)
    # The requested width includes the end scans, not just their centre span.
    span = max(step, a.width_m-nominal_width)
    columns = np.linspace(-span*.5, span*.5, int(np.ceil(span/step))+1)
    placements = []
    rejects = dict(no_surface=0, waterfall=0, temple=0)
    local_face_angle = {'+x': 0., '-x': 180., '+y': 90., '-y': -90.}[a.front_axis]
    for col, offset in enumerate(columns):
        angle = centre+offset/1600.
        heights = np.arange(-190., 78., 4.)
        supported = [z for z in heights if face.hit(angle, z) is not None]
        if not supported:
            continue
        # Preserve a small wet-toe band. Crest rows remain below/around the
        # actual rim, rather than manufacturing a uniformly tall slab.
        bottom, top = min(supported)+18., min(max(supported)-5., 45.)
        rows = np.arange(bottom+5.*np.sin(col*1.73), top+1., 23.)
        for row, z in enumerate(rows):
            if len(placements) >= a.maximum:
                break
            asset = assets[(col+row//2) % len(assets)]
            scale = float(3.35+.34*np.sin(col*2.17+row*1.39))
            theta = angle+((row % 2)-.5)*step*.32/1600.
            hit = face.hit(theta, z)
            if hit is None:
                rejects['no_surface'] += 1
                continue
            radial = np.array([np.cos(theta), np.sin(theta), 0.])
            pitch, roll = float(5.*np.sin(col*1.81+row*.77)), float(6.*np.cos(row*1.67+col*.53))
            yaw = float(np.rad2deg(theta)+180.-local_face_angle+4.*np.sin(row*1.19+col*.67))
            rot = rotation(pitch, yaw, roll)
            signs = np.array([[x, y, zz] for x in (-1, 1) for y in (-1, 1) for zz in (-1, 1)])
            relative_corners = signs*(asset['size']*.5*scale)@rot.T
            tangent = np.array([-radial[1], radial[0], 0.])
            half_width = float(abs(relative_corners@tangent).max())
            half_height = float(abs(relative_corners[:, 2]).max())
            half_depth = float(abs(relative_corners@radial).max())
            samples = [face.hit(theta+du/1600., z+dz)
                       for du in (-half_width*.8, 0., half_width*.8)
                       for dz in (-half_height*.7, 0., half_height*.7)]
            samples = [s for s in samples if s is not None]
            # Fit to the frontmost backing across the whole visible envelope,
            # not the centroid of a deep recess behind a projecting buttress.
            front_radius = min(s[0] for s in samples)
            relief = 4. if z < -90 else 7.5
            box_centre = radial*(front_radius+half_depth-relief)
            box_centre[2] = z
            corners = box_centre+relative_corners
            box_lo, box_hi = corners.min(0), corners.max(0)
            if box_hi[0] >= -1055. and box_lo[0] <= -920. and box_hi[1] >= 1125. and box_lo[1] <= 1270.:
                rejects['waterfall'] += 1
                continue
            if any(box_hi[2] >= bottom_z-2. and box_polygon_overlap(box_lo[:2]-2., box_hi[:2]+2., poly)
                   for _, poly, bottom_z in anchors):
                rejects['temple'] += 1
                continue
            origin = box_centre-rot@(asset['centre']*scale)
            placements.append(dict(mesh=asset['asset'], position_cm=(origin*100).tolist(),
                rotation_degrees=[pitch, yaw, roll], scale=[scale]*3,
                source_triangle=hit[1], source_barycentric=hit[2], surface_radius_m=hit[0],
                footprint_front_radius_m=front_radius, bounds_front_relief_m=relief,
                box_min_arena_m=box_lo.tolist(), box_max_arena_m=box_hi.tolist(),
                source_bounds_min_cm=(asset['minimum']*100).tolist(), source_bounds_max_cm=(asset['maximum']*100).tolist()))
    if not placements:
        raise ValueError('No placements survived the face/corridor/support guards')
    report = dict(schema='directorscut.cliff_scan_demo.v1', instances=placements,
        source_surface=str(a.surface.resolve()), source_sha256=sha(a.surface),
        source_anchors=str(a.anchors.resolve()), anchors_sha256=sha(a.anchors), generator_sha256=sha(__file__),
        imported_reports=[dict(path=str(p.resolve()), sha256=sha(p)) for p in a.reports],
        authored_width_m=a.width_m, centre_angle_deg=a.centre_angle_deg,
        native_front_axis=a.front_axis, waterfall_protected_xy_m=[-1055., -920., 1125., 1270.],
        metrics=dict(instances=len(placements), rejects=rejects,
            scale_range=[min(x['scale'][0] for x in placements), max(x['scale'][0] for x in placements)],
            bounds_min_arena_m=np.min([x['box_min_arena_m'] for x in placements], axis=0).tolist(),
            bounds_max_arena_m=np.max([x['box_max_arena_m'] for x in placements], axis=0).tolist()),
        limitations='Bounds-envelope burial is not scan-surface occlusion proof. Old backing remains authoritative for gameplay and supports. Static local guards are not full dynamic-route collision. Review actual native facing, joints and cover before accepting.',
        visual_accepted=False)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(output=str(a.output), **report['metrics']), indent=2))


if __name__ == '__main__':
    main()
