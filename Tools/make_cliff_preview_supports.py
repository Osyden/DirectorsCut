"""Select only fully verified existing ruins for the isolated cliff preview.

The independent footprint audit must match the original FBX result hash. This
does not approve runtime terrain stitching, partial footprints or the skyline.
"""
import argparse
import hashlib
import json
from pathlib import Path


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--audit', type=Path, required=True)
    p.add_argument('--fbx-sidecar', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    report = json.loads(args.audit.read_text(encoding='utf-8-sig'))
    fbx = json.loads(args.fbx_sidecar.read_text(encoding='utf-8-sig'))
    if not report['passed'] or report['provenance']['candidate']['sha256'] != fbx['result_sha256']:
        raise ValueError('Footprint proof must match the exported result')
    for value in report['provenance'].values():
        if digest(value['path']) != value['sha256']:
            raise ValueError('Audit source changed: '+value['path'])
    if digest(fbx['fbx_path']) != fbx['fbx_sha256']:
        raise ValueError('Exported FBX changed')
    anchors = json.loads(Path(report['provenance']['anchors']['path']).read_text(encoding='utf-8-sig'))
    by_id = {a['id']: a for a in anchors['anchors']}
    selected = []
    for proof in report['temples']:
        if proof['status'] != 'covered_at_target':
            continue
        if proof['outside_tile_area_m2'] > 1.e-5 or proof['missing_triangle_area_m2'] > 1.e-5:
            raise ValueError('Inconsistent full-footprint proof')
        a = by_id[proof['id']]
        if proof['height_min_m'] < a['footing_bottom_m']:
            raise ValueError('Unsupported ruin')
        points = a['footprint_xy_m']
        selected.append(dict(id=a['id'], footing_bottom_m=a['footing_bottom_m'],
                             centre_xy_m=[sum(v[i] for v in points)/len(points) for i in (0, 1)]))
    if not selected:
        raise ValueError('No fully supported ruins')
    out = args.output.resolve()
    if 'saved' not in [part.lower() for part in out.parts]:
        raise ValueError('Preview manifest belongs under Saved')
    payload = dict(schema='directorscut.cliff_preview_supports.v1', source_fbx_sha256=fbx['fbx_sha256'],
                   source_audit=str(args.audit.resolve()), source_audit_sha256=digest(args.audit),
                   actors=selected, runtime_integration_approved=False)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(payload))


if __name__ == '__main__':
    main()
