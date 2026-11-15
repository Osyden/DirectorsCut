"""Lower one wide entry river into the frozen complete cliff, retaining topology/UVs."""
import argparse, hashlib, json
from pathlib import Path
import numpy as np


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def smooth(a,b,x):
    u=np.clip((x-a)/(b-a),0.,1.)
    return u*u*(3.-2.*u)


def river(points):
    radius=np.linalg.norm(points[:,:2],axis=1)
    phase=smooth(.58,.985,radius/1600.)
    center=np.pi-.40*(1.-phase)
    delta=np.arctan2(np.sin(np.arctan2(points[:,1],points[:,0])-center),
                     np.cos(np.arctan2(points[:,1],points[:,0])-center))
    across=np.abs(delta)*radius
    height=-400.+400.*phase
    return height,across


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a fresh output directory under Saved')
    with np.load(args.source,allow_pickle=False) as archive:
        data={key:archive[key].copy() for key in archive.files}
    original=data['vertices_m'].copy();vertices=original.copy();faces=data['triangles']
    height,across=river(vertices)
    weight=1.-smooth(220.,250.,across)
    # Retain 0.2% of the source relief below the bed cap: completely
    # flattening nearly vertical source faces collapses four import triangles.
    target=height-3.-.002*(float(original[:,2].max())-original[:,2])
    vertices[:,2]-=np.maximum(0.,vertices[:,2]-target)*weight
    rows,columns=json.loads(str(data['metadata_json']))['grid_shape']
    grid=vertices.reshape(rows,columns,3)
    # Close the existing duplicate endpoint exactly; no XY or UV changes.
    grid[:,-1,2]=grid[:,0,2]
    if not np.array_equal(vertices[:,:2],original[:,:2]):
        raise RuntimeError('Entry cut changed planar cliff coordinates')
    outside=across>=250.
    if not np.array_equal(vertices[outside],original[outside]):
        raise RuntimeError('Entry cut escaped its physical width')
    origin=data['frame_origin_world_m']
    native=(vertices-origin).astype(np.float32).astype(np.float64)
    min_area=float('inf');collapsed=0;centroid_clearance=[]
    for start in range(0,len(faces),100000):
        f=faces[start:start+100000]
        p=native[f]
        area=np.linalg.norm(np.cross(p[:,1]-p[:,0],p[:,2]-p[:,0]),axis=1)
        min_area=min(min_area,float(area.min()));collapsed+=int(np.count_nonzero(area<1.e-10))
        c=vertices[f].mean(axis=1); h,d=river(c)
        # This is a sampled face-interior check, not continuous collision proof.
        inside=d<=215.
        if inside.any():centroid_clearance.extend((h[inside]-c[inside,2]).tolist())
    if collapsed:raise RuntimeError(f'{collapsed} collapsed native float32 triangles')
    metadata=json.loads(str(data['metadata_json']))
    metadata['entry_cut']=dict(source_path=str(args.source.resolve()),source_sha256=sha(args.source),
        lake_height_m=-400.,radius_m=1600.,radius_fraction=[.58,.985],river_angle_radians='.pi-.40*(1-smoothstep(.58,.985,R/1600))',
        full_half_width_m=220.,edge_feather_m=30.,bed_below_ramp_m=3.,retained_relief_fraction=.002,
        changed_vertices=int(np.count_nonzero(vertices[:,2]!=original[:,2])),maximum_cut_m=float((original[:,2]-vertices[:,2]).max()),
        old_material_mapping_retained=True,original_topology_and_uvs_retained=True,
        old_waterfalls_valid=False,temple_support_requires_new_actual_surface_query=True,
        vertex_river_clearance_min_m=float((height[across<=220]-vertices[across<=220,2]).min()),
        sampled_triangle_centroid_clearance_min_m=min(centroid_clearance),
        limitations='Authored single cataract excavation. Centroid sampling is not continuous collision proof; root owns lake and game validation.')
    metadata['canonical_toe_max_error_m_before_entry_cut']=metadata.pop('canonical_toe_max_error_m',None)
    metadata['canonical_outer_edge_max_error_m_before_entry_cut']=metadata.pop('canonical_outer_edge_max_error_m',None)
    metadata['seam_position_error_m']=float(np.linalg.norm(grid[:,0]-grid[:,-1],axis=1).max())
    metadata['exported_collapsed_triangles']=collapsed
    metadata['exported_minimum_twice_triangle_area_m2']=min_area
    data['vertices_m']=vertices;data['metadata_json']=np.asarray(json.dumps(metadata))
    args.output.mkdir(parents=True)
    out=args.output/'candidate.npz';np.savez_compressed(out,**data)
    report=dict(source=str(out.resolve()),source_sha256=sha(out),vertices=len(vertices),triangles=len(faces),
        entry_cut=metadata['entry_cut'],seam_position_error_m=metadata['seam_position_error_m'],
        exported_collapsed_triangles=collapsed,exported_minimum_twice_triangle_area_m2=min_area)
    (args.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
