"""Join the caldera entry banks into the original full-ring surface grid."""
import argparse,hashlib,json
from pathlib import Path
import numpy as np


def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def ss(a,b,x):
    u=np.clip((x-a)/(b-a),0.,1.);return u*u*(3.-2.*u)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    if args.output.exists() or 'saved' not in [s.lower() for s in args.output.resolve().parts]:p.error('Use a new output under Saved')
    with np.load(args.source,allow_pickle=False) as a:data={k:a[k].copy() for k in a.files}
    meta=json.loads(str(data['metadata_json']));parent=Path(meta['toe_extension']['parent_source_path'])
    with np.load(parent,allow_pickle=False) as a:old=a['vertices_m'].reshape(*meta['grid_shape'],3).copy()
    original=data['vertices_m'].copy();grid=old.copy();rows,cols,_=grid.shape;crest=meta['crest_row_index'];row=np.arange(rows)[:,None];t=np.clip(row/crest,0,1)
    theta=np.arctan2(old[crest,:,1],old[crest,:,0]);angle=np.arctan2(np.sin(theta-np.pi),np.cos(theta-np.pi))[None,:]
    radius=np.linalg.norm(old[:,:,:2],axis=2);crest_r=radius[crest:crest+1];toe_r=radius[:1]
    # The rejected one-row closure is spread through already existing lower
    # faces. This removes the untextured four-hundred-metre triangular skirt.
    lowering=np.maximum(old[0:1,:,2]+445.,0.)
    zbase=old[:,:,2]-lowering*(1-ss(0.,220.,row))
    weight=ss(-1.15,-.75,angle)*(1-ss(.27,.64,angle))
    toe=np.minimum(toe_r,850.+330.*ss(.20,.70,np.abs(angle+.40)))
    linear=toe+(crest_r-toe)*t
    old_linear=toe_r+(crest_r-toe_r)*t
    radial=linear+.20*(radius-old_linear)*np.sin(np.pi*t)
    radial=np.where(row<=crest,radial,radius)
    new_r=radius+(radial-radius)*weight
    grid[:,:,:2]=old[:,:,:2]*(new_r/np.maximum(radius,1.e-9))[:,:,None]
    q=ss(.58,.985,new_r/1600.)
    river_angle=np.pi-.40*(1-q)
    across=np.arctan2(np.sin(theta[None,:]-river_angle),np.cos(theta[None,:]-river_angle))*new_r
    ramp=-400.+400.*q
    crest_z=old[crest:crest+1,:,2]
    detail=old[:,:,2]-(old[0:1,:,2]+(crest_z-old[0:1,:,2])*t)
    mass=-445.+(crest_z+445.)*ss(0.,1.,t)+.35*detail*4*t*(1-t)
    # Unequal shoulders emerge from the broad cliff mass. Neither side drops
    # along the old standalone embankment's artificial parallel perimeter.
    width=np.where(across<0.,480.,650.)
    near=1-ss(width-100.,width+120.,np.abs(across))
    rise=np.where(across<0.,32.,58.)*ss(1020.,1250.,new_r)
    wanted=np.maximum(mass,ramp-4.+rise)
    mass+=(wanted-mass)*near*ss(0.,32.,row)
    channel=1-ss(195.,235.,np.abs(across))
    channel_bed=ramp-4.-41.*(1-ss(850.,928.,new_r))
    mass+=(channel_bed-mass)*channel
    # Keep the level lake steering envelope clear outside the inlet itself.
    clear=(1-ss(1040.,1130.,new_r))*ss(185.,235.,np.abs(across))
    mass+=(np.minimum(mass,-405.)-mass)*clear
    top=ss(220.,248.,row)
    mass=mass*(1-top)+old[:,:,2]*top
    grid[:,:,2]=zbase+(mass-zbase)*weight
    grid[crest:]=old[crest:]
    grid[:,-1]=grid[:,0]
    # Physical mapping for altered lower terrain; the upper atlas coordinates
    # and full mesh connectivity remain untouched.
    uv=data['uv0'].reshape(rows,cols,2).copy();phi=np.arange(cols)/ (cols-1)*2*np.pi
    updated=np.stack(np.broadcast_arrays((phi[None,:]*1600.-529*1600*2*np.pi/1536)/497.418836818384,
        (grid[:,:,2]+.35*np.maximum(0.,np.linalg.norm(grid[:,:,:2],axis=2)-crest_r)+200.)/280.),axis=-1)
    changed=np.any(grid!=original.reshape(rows,cols,3),axis=2)
    uv[changed]=updated[changed]
    vertices=grid.reshape(-1,3);faces=data['triangles'];native=(vertices-data['frame_origin_world_m']).astype(np.float32).astype(np.float64)
    minimum=float('inf');collapsed=0
    for start in range(0,len(faces),100000):
        pts=native[faces[start:start+100000]];areas=np.linalg.norm(np.cross(pts[:,1]-pts[:,0],pts[:,2]-pts[:,0]),axis=1)
        minimum=min(minimum,float(areas.min()));collapsed+=int(np.count_nonzero(areas<1.e-10))
    if collapsed:raise RuntimeError(f'{collapsed} collapsed native triangles')
    shores=[]
    for col in range(0,cols-1,(cols-1)//128):
        heights=grid[:,col,2];cross=np.flatnonzero((heights[:-1]<=-400.)&(heights[1:]>-400.))
        if not len(cross):raise RuntimeError(f'No shoreline crossing at column{col}')
        k=int(cross[0]);alpha=(-400.-heights[k])/(heights[k+1]-heights[k]);shores.append((grid[k,col]*(1-alpha)+grid[k+1,col]*alpha).tolist())
    # Actual surface vertices outside the wet channel, spatially separated.
    candidates=np.argwhere((row<220)&(np.abs(across)>238)&(np.abs(across)<390)&(new_r>1120)&(new_r<1710)&(grid[:,:,2]>-385)&(weight>.98))
    rng=np.random.default_rng(151126);rng.shuffle(candidates);anchors=[]
    for r,c in candidates:
        point=grid[r,c]
        if all(np.linalg.norm(point[:2]-np.asarray(a)[:2])>28 for a in anchors):anchors.append(point.tolist())
        if len(anchors)==56:break
    metadata=dict(meta);metadata['integrated_inlet_banks']=True
    metadata['bank_integration']=dict(parent=str(args.source.resolve()),parent_sha256=sha(args.source),pre_toe_parent=str(parent),
        geometry='Original polar surface grid remapped into asymmetric entry apron; no separate embankment',
        upper_rows_from=crest,changed_vertices=int(changed.sum()),source_uvs_retained_on_unchanged_vertices=True,
        lower_uvs_updated=True,route_or_lake_changed=False,original_triangle_indices_retained=True,
        toe_drop_distributed_through_rows=220)
    metadata['seam_position_error_m']=float(np.linalg.norm(grid[:,0]-grid[:,-1],axis=1).max())
    metadata['exported_collapsed_triangles']=collapsed;metadata['exported_minimum_twice_triangle_area_m2']=minimum
    data['vertices_m']=vertices;data['uv0']=uv.reshape(-1,2);data['metadata_json']=np.asarray(json.dumps(metadata))
    args.output.mkdir(parents=True);output=args.output/'candidate.npz';np.savez_compressed(output,**data)
    manifest=dict(schema=meta['schema'],source=str(output.resolve()),source_sha256=sha(output),vertices=len(vertices),triangles=len(faces),grid_shape=meta['grid_shape'],entry_cut=meta['entry_cut'],toe_extension=meta['toe_extension'],bank_integration=metadata['bank_integration'],integrated_inlet_banks=True,
        shore_mist_centers_m=shores,bank_boulder_anchors_m=anchors,seam_position_error_m=metadata['seam_position_error_m'],exported_collapsed_triangles=collapsed,exported_minimum_twice_triangle_area_m2=minimum)
    (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in manifest.items() if k not in ('shore_mist_centers_m','bank_boulder_anchors_m','entry_cut','toe_extension')},indent=2))

if __name__=='__main__':main()
