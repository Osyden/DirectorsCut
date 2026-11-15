"""Remove the explicit crop sides and 24m authoring back cap.

Keeps all real original 3D face positions and triangles, including overhangs.
This creates the open terrain patch for a truthful surface review. It does not
hide exposure transitions, repair shape, or reconstruct an XY heightfield.
An optional exact-source audit can omit tiny sealed inward cavity shells;
the independent containment record is retained and exterior faces do not move.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_fractured_cliff_outcrop import sample_height


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--omit-audited-cavities',type=Path,
                        help='Optional exact-source independent audit of tiny enclosed inward shells')
    args = parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a new ignored Saved output path')
    with np.load(args.input, allow_pickle=False) as archive:
        data = {k: archive[k].copy() for k in archive.files}
    meta = json.loads(str(data['metadata_json']))
    path = Path(meta['source_path'])
    if hashlib.sha256(path.read_bytes()).hexdigest() != meta['source_sha256']:
        raise ValueError('Changed original source')
    with np.load(path, allow_pickle=False) as archive:
        source = {k: archive[k].copy() for k in archive.files}
    basis = np.column_stack((data['basis_u_world'], data['basis_v_world'], data['basis_z_world']))
    vertices, triangles = data['vertices_m'], data['triangles']
    local = (vertices-data['frame_origin_world_m'])@basis
    crop=meta.get('crop',dict(row=89,col=64,width_m=72.,depth_m=40.))
    row,col=int(crop['row']),int(crop['col'])
    half_u,half_v=float(crop['width_m'])/2.,float(crop['depth_m'])/2.
    if (meta.get('original_backing_depth_m') != 24.
        or not np.allclose([local[:, 0].min(), local[:, 0].max(), local[:, 1].min(), local[:, 1].max()],
                           [-half_u,half_u,-half_v,half_v], atol=.0001, rtol=0)):
        raise ValueError('Mesh bounds disagree with explicit crop descriptor and24m backing')
    expected_bottom = sample_height(source, local[:, 0], local[:, 1], row, col)-data['frame_origin_world_m'][2]-24.
    on_bottom = np.abs(local[:, 2]-expected_bottom) <= .0001
    remove_bottom = on_bottom[triangles].all(axis=1)
    remove_side = np.zeros(len(triangles), dtype=bool)
    for axis, boundary in ((0,-half_u),(0,half_u),(1,-half_v),(1,half_v)):
        remove_side |= (np.abs(local[:, axis]-boundary) <= .0001)[triangles].all(axis=1)
    remove_cavity=np.zeros(len(triangles),dtype=bool)
    cavity_record=None
    if args.omit_audited_cavities:
        audit=json.loads(args.omit_audited_cavities.read_text())
        if audit['source_closed_sha256']!=hashlib.sha256(args.input.read_bytes()).hexdigest():
            raise ValueError('Cavity audit refers to another original closed mesh')
        proof=audit['containment']
        if (proof['queries']<=0 or len(proof['outside_counts'])<2
            or any(proof['outside_counts']) or any(n!=proof['queries'] for n in proof['odd_inside_counts'])):
            raise ValueError('Require independent two-direction containment evidence for every audited probe')
        volumes=[]
        for shell in audit['cavities']:
            faces=np.asarray(shell['triangle_indices'],dtype=np.int64)
            if len(faces)==0 or faces.min()<0 or faces.max()>=len(triangles) or remove_cavity[faces].any():
                raise ValueError('Invalid or duplicate audited cavity triangle IDs')
            tris=triangles[faces];ids=np.unique(tris)
            if not np.array_equal(ids,np.asarray(shell['vertex_indices'])):
                raise ValueError('Cavity vertex and triangle memberships disagree')
            edges=np.sort(np.concatenate((tris[:,[0,1]],tris[:,[1,2]],tris[:,[2,0]])),axis=1)
            if not np.all(np.unique(edges,axis=0,return_counts=True)[1]==2):
                raise ValueError('Audited cavity is not a closed shell')
            if np.any(np.isin(triangles[np.setdiff1d(np.arange(len(triangles)),faces)],ids)):
                raise ValueError('Audited shell shares vertices with another surface')
            p=vertices[tris]-vertices[ids].mean(axis=0)
            volume=float(np.einsum('ij,ij->i',p[:,0],np.cross(p[:,1],p[:,2])).sum()/6.)
            if not -.1<volume<0. or not np.isclose(volume,shell['signed_volume_m3'],atol=1.e-5,rtol=0.):
                raise ValueError('Only independently measured tiny inward cavity shells may be omitted')
            if np.any(np.abs(local[ids,0])>=half_u-4.) or np.any(np.abs(local[ids,1])>=half_v-4.):
                raise ValueError('Audited shell intersects the immutable physical crop guard')
            volumes.append(volume);remove_cavity[faces]=True
        if -sum(volumes)>.1 or np.any(remove_cavity&(remove_bottom|remove_side)):
            raise ValueError('Cavity omission exceeds bounded0.1m3 or overlaps authoring closure')
        cavity_record=dict(audit_path=str(args.omit_audited_cavities.resolve()),
                           audit_sha256=hashlib.sha256(args.omit_audited_cavities.read_bytes()).hexdigest(),
                           omitted_triangles=int(remove_cavity.sum()),signed_volumes_m3=volumes,
                           containment_evidence=proof,exterior_positions_or_faces_changed=False)
    kept = triangles[~(remove_bottom | remove_side | remove_cavity)]
    ids = np.unique(kept)
    remap = np.full(len(vertices), -1, dtype=np.int64)
    remap[ids] = np.arange(len(ids))
    data['vertices_m'] = vertices[ids]
    data['triangles'] = remap[kept]
    meta.update(authoring_closure_removed=dict(input_path=str(args.input.resolve()),
                input_sha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),
                bottom_triangles=int(remove_bottom.sum()), side_triangles=int(remove_side.sum()),
                position_tolerance_m=.0001, retained_triangles=len(kept),
                rule='Only faces wholly on explicit crop X/Y sides or source minus24m authoring underside',
                crop=crop,
                omitted_audited_cavities=cavity_record,
                positions_changed=False, overhangs_projected=False),
                source_kind='original_cliff_exposure_surface', visual_accepted=False)
    data['metadata_json'] = np.asarray(json.dumps(meta))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(args.output, **data)
    print(json.dumps(meta['authoring_closure_removed'], indent=2))


if __name__ == '__main__':
    main()
