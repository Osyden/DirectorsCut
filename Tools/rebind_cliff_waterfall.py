"""Rebind an unchanged waterfall/cache only after exact local triangle proof.

Offline CPU. The target must carry its original-ring vertex correspondence.
Every triangle overlapping the full fixed VDB bounds or core/foam bounds must
match the original ring exactly, including any newly introduced geometry.
This is not permission to fit an old cache to a changed local cliff.
"""
import argparse,hashlib,itertools,json,shutil
from pathlib import Path
import numpy as np

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--attachment',type=Path,required=True)
    parser.add_argument('--cliff',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists() or 'saved' not in [v.lower() for v in args.output.resolve().parts]:
        parser.error('Choose a new Saved output; old bundles remain immutable')
    old=json.loads((args.attachment/'attachment.json').read_text())
    profile=json.loads((args.attachment/'waterfall_profile.json').read_text())
    manifest=json.loads((args.attachment/'Volumes/manifest.json').read_text())
    if sha(Path(old['source_path']))!=old['source_sha256']:
        raise ValueError('Original source changed')
    source=np.load(old['source_path']);target=np.load(args.cliff)
    if not np.array_equal(source['vertices_m'],target['original_ring_vertices_m']):
        raise ValueError('Target original-ring positions differ from this attachment source')
    B=np.array([profile['basis_x_arena'],profile['basis_y_arena'],profile['basis_z_arena']]).T
    origin=np.array(profile['local_origin_arena_cm'])/100
    world=[]
    for seq in manifest['sequences'].values():
        lo,hi=np.array(seq['index_bounds_inclusive'],float)
        corners=np.array(list(itertools.product(*zip(lo,hi+1))))*manifest['config']['voxel_m']
        world.append(corners@B.T+origin)
    for name in ('core.npz','receiving_foam.npz'):
        world.append(np.load(args.attachment/name)['vertices_m'])
    points=np.concatenate(world);lo=points.min(axis=0)-.05;hi=points.max(axis=0)+.05
    def faces_in_bounds(mesh):
        tri=mesh['triangles'];p=mesh['vertices_m'][tri]
        ids=np.flatnonzero(np.all(p.max(axis=1)>=lo,axis=1)&np.all(p.min(axis=1)<=hi,axis=1))
        return tri[ids],ids
    old_faces,old_face_ids=faces_in_bounds(source);new_faces,new_face_ids=faces_in_bounds(target)
    mapping=target['source_ring_vertex_index'];new_ids=mapping[new_faces]
    if (new_ids<0).any():
        raise ValueError('New target geometry enters the waterfall/cache bounds; refit required')
    vertices=np.unique(new_faces)
    if not np.array_equal(target['vertices_m'][vertices],source['vertices_m'][mapping[vertices]]):
        raise ValueError('Local cliff vertices moved; refit required')
    old_keys={tuple(row) for row in np.sort(old_faces,axis=1)}
    new_keys={tuple(row) for row in np.sort(new_ids,axis=1)}
    if old_keys!=new_keys or len(old_faces)!=len(new_faces):
        raise ValueError('Local cliff triangles changed; refit required')
    old_order={tuple(sorted(face)):face for face in old_faces}
    for face in new_ids:
        original=old_order[tuple(sorted(face))]
        if not any(np.array_equal(face,np.roll(original,k)) for k in range(3)):
            raise ValueError('Local triangle winding changed; refit required')
    # Keep recorded support IDs truthful in the new mesh; barycentric weights
    # must also follow the target triangle's actual corner order.
    target_faces={tuple(sorted(mapping[face])):int(i) for face,i in zip(new_faces,new_face_ids)}
    def vertex_id(i):
        ids=np.flatnonzero(mapping==i)
        ids=ids[np.all(target['vertices_m'][ids]==source['vertices_m'][i],axis=1)]
        if not len(ids):raise ValueError('Required original support vertex missing')
        return int(ids[0])
    support=dict(np.load(args.attachment/'support.npz'))
    support_ids=support['support_triangle'].copy();weights=support['support_barycentric'].copy()
    for at in zip(*np.nonzero(support_ids>=0)):
        original=source['triangles'][support_ids[at]]
        new_id=target_faces[tuple(sorted(original))]
        target_order=mapping[target['triangles'][new_id]]
        weights[at]=weights[at][[int(np.flatnonzero(original==i)[0]) for i in target_order]]
        support_ids[at]=new_id
    support['support_triangle']=support_ids;support['support_barycentric']=weights
    support['source_mesh_edge_path']=np.array([vertex_id(int(i)) for i in support['source_mesh_edge_path']])
    old['canonical_source_vertex_index']=old['source_vertex_index']
    old['source_vertex_index']=vertex_id(old['source_vertex_index'])
    for jet in (old['terminal_jet'],profile['terminal_ballistic_jet']):
        if 'source_support_vertex' in jet:
            jet['canonical_source_support_vertex']=jet['source_support_vertex']
            jet['source_support_vertex']=vertex_id(jet['source_support_vertex'])
    proof=dict(method='exact original vertex positions and triangle incidence throughout full fixed volume/mesh AABB',
               original_source_sha256=old['source_sha256'],target_sha256=sha(args.cliff),
               compared_triangles=len(old_faces),aabb_arena_m=[lo.tolist(),hi.tolist()],
               cache_regenerated=False,physical_trajectory_fields_identical=True,
               caveat='Static local geometry equivalence only; no animated-water or runtime hull guarantee')
    args.output.mkdir(parents=True)
    for name in ('core.npz','receiving_foam.npz','waterfall_core.foam.fbx'):
        shutil.copy2(args.attachment/name,args.output/name)
    np.savez_compressed(args.output/'support.npz',**support)
    profile['source_mesh_path']=str(args.cliff.resolve());profile['source_mesh_sha256']=proof['target_sha256']
    profile['profile_sha1']=hashlib.sha1(json.dumps({k:v for k,v in profile.items() if k!='profile_sha1'},sort_keys=True).encode()).hexdigest()
    profile_path=args.output/'waterfall_profile.json';profile_path.write_text(json.dumps(profile,indent=2))
    old.update(source_path=str(args.cliff.resolve()),source_sha256=proof['target_sha256'],profile_sha1=profile['profile_sha1'],
               geometry_rebind=proof)
    (args.output/'attachment.json').write_text(json.dumps(old,indent=2))
    volumes=args.output/'Volumes';volumes.mkdir()
    for name in ('Falling','Impact'):
        shutil.copytree(args.attachment/'Volumes'/name,volumes/name)
    manifest['config']['output']=str(volumes.resolve());manifest['config']['profile']=str(profile_path.resolve())
    manifest['fitted_profile'].update(file=str(profile_path.resolve()),file_sha256=sha(profile_path),profile_sha1=profile['profile_sha1'])
    manifest['geometry_rebind']=proof
    (volumes/'manifest.json').write_text(json.dumps(manifest,indent=2))
    (args.output/'geometry_rebind.json').write_text(json.dumps(proof,indent=2))
    print(json.dumps(proof,indent=2))

if __name__=='__main__':main()
