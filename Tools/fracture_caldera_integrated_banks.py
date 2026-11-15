"""Restore measured source-cliff fracture relief on connected caldera shoulders."""
import argparse,hashlib,json
from pathlib import Path
import numpy as np


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def ss(a,b,x):
    t=np.clip((x-a)/(b-a),0,1);return t*t*(3-2*t)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--rock-source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    if args.output.exists() or 'saved' not in [x.lower() for x in args.output.resolve().parts]:p.error('Use a fresh Saved output')
    with np.load(args.source,allow_pickle=False) as a:data={k:a[k].copy() for k in a.files}
    meta=json.loads(str(data['metadata_json']));shape=meta['grid_shape'];base=data['vertices_m'].reshape(*shape,3);rows,cols,_=base.shape
    with np.load(args.rock_source,allow_pickle=False) as a:rock=a['vertices_m'].reshape(base.shape)
    parent=json.loads((args.source.parent/'manifest.json').read_text(encoding='utf-8'))
    row=np.arange(rows)[:,None];t=np.clip(row/248,0,1);radius=np.linalg.norm(base[:,:,:2],axis=2);theta=np.arctan2(base[:,:,1],base[:,:,0]);angle=np.arctan2(np.sin(theta-np.pi),np.cos(theta-np.pi))
    q=ss(.58,.985,radius/1600);rv=np.pi-.4*(1-q);across=np.arctan2(np.sin(theta-rv),np.cos(theta-rv))*radius
    region=ss(-1.15,-.75,angle)*(1-ss(.27,.64,angle))
    guard=ss(235,290,abs(across))*(1-ss(205,248,row))*ss(-410,-380,base[:,:,2])*ss(1050,1130,radius)*region
    rr=np.linalg.norm(rock[:,:,:2],axis=2);rprofile=rr[:1]+(rr[248:249]-rr[:1])*t
    zprofile=rock[:1,:,2]+(rock[248:249,:,2]-rock[:1,:,2])*t
    dr=np.clip(.45*(rr-rprofile)*np.sin(np.pi*t),-18,18)
    dz=np.clip(.85*(rock[:,:,2]-zprofile),-12,20)
    # One shallow, shared-vertex bevel on the added relief only. Large original
    # faces remain; no global smoothing, independent stones, new noise or tiling.
    def bevel(a):
        n=(np.roll(a,1,axis=1)+np.roll(a,-1,axis=1)+np.concatenate([a[:1],a[:-1]])+np.concatenate([a[1:],a[-1:]]))*.25
        return .85*a+.15*n
    dr=bevel(dr)*guard;dz=bevel(dz)*guard
    offset=np.zeros_like(base);offset[:,:,:2]=base[:,:,:2]/radius[:,:,None]*dr[:,:,None];offset[:,:,2]=dz
    # Bound only the few source-fracture offsets which invert a neighbouring
    # triangle. Gaussian attenuation has no hard edge or one-vertex pin.
    faces=data['triangles'];before=base.reshape(-1,3);flat_offset=offset.reshape(-1,3)
    reference=np.cross(before[faces[:,1]]-before[faces[:,0]],before[faces[:,2]]-before[faces[:,0]])
    attenuation=np.ones(len(before));limited_passes=[]
    for iteration in range(12):
        trial=before+flat_offset*attenuation[:,None]
        cross=np.cross(trial[faces[:,1]]-trial[faces[:,0]],trial[faces[:,2]]-trial[faces[:,0]])
        bad=np.flatnonzero(np.einsum('ij,ij->i',cross,reference)<=0)
        if not len(bad):break
        limited_passes.append(int(len(bad)))
        local=np.zeros(len(before))
        for f in faces[bad]:
            centre=before[f].mean(axis=0);distance2=np.sum((before-centre)**2,axis=1)
            local=np.maximum(local,.65*np.exp(-distance2/(2*10.**2)))
        attenuation*=1-local
    offset=(flat_offset*attenuation[:,None]).reshape(base.shape)
    grid=base+offset;grid[:,-1]=grid[:,0]
    # Inspect actual triangles at native precision; no topology repair or
    # silent weak-radius fallback is allowed in this bounded source pass.
    faces=data['triangles'];vertices=grid.reshape(-1,3);origin=data['frame_origin_world_m'];native=(vertices-origin).astype(np.float32).astype(np.float64)
    minimum=float('inf');collapsed=0;reverse=0
    before=base.reshape(-1,3)
    for start in range(0,len(faces),100000):
        f=faces[start:start+100000];pts=native[f];cross=np.cross(pts[:,1]-pts[:,0],pts[:,2]-pts[:,0]);area=np.linalg.norm(cross,axis=1)
        original=before[f];oldcross=np.cross(original[:,1]-original[:,0],original[:,2]-original[:,0])
        reverse+=int(np.count_nonzero(np.einsum('ij,ij->i',cross,oldcross)<0));minimum=min(minimum,float(area.min()));collapsed+=int(np.count_nonzero(area<1e-10))
    if collapsed:raise RuntimeError(f'{collapsed} collapsed triangles')
    if reverse:raise RuntimeError(f'{reverse} opposite source triangle normals; inspect before export')
    wet=abs(across)<=220
    if not np.array_equal(grid[wet],base[wet]):raise RuntimeError('Wet corridor changed')
    if not np.array_equal(grid[248:],base[248:]):raise RuntimeError('Upper geometry changed')
    uv=data['uv0'].reshape(rows,cols,2).copy();changed=np.any(grid!=base,axis=2)
    rnew=np.linalg.norm(grid[:,:,:2],axis=2);crest_r=rnew[248:249]
    newv=(grid[:,:,2]+.35*np.maximum(0,rnew-crest_r)+200)/280
    uv[:,:,1][changed]=newv[changed]
    shores=[]
    for c in range(0,cols-1,(cols-1)//128):
        z=grid[:,c,2];hits=np.flatnonzero((z[:-1]<=-400)&(z[1:]>-400))
        if not len(hits):raise RuntimeError('Missing shoreline')
        k=int(hits[0]);u=(-400-z[k])/(z[k+1]-z[k]);shores.append((grid[k,c]*(1-u)+grid[k+1,c]*u).tolist())
    # Reattach the existing accents to the closest corresponding authored grid
    # vertex; positions are taken from final triangles, never the old wedge.
    anchors=[]
    for xyz in parent['bank_boulder_anchors_m']:
        idx=int(np.argmin(np.sum((before-np.asarray(xyz))**2,axis=1)));anchors.append(vertices[idx].tolist())
    correction=dict(parent=str(args.source.resolve()),parent_sha256=sha(args.source),rock_source=str(args.rock_source.resolve()),rock_source_sha256=sha(args.rock_source),
        added_radial_limit_m=18,added_vertical_limit_m=[-12,20],bevel_neighbour_fraction=.15,
        protected_wet_halfwidth_m=220,blend_start_m=235,blend_end_m=290,upper_rows_unchanged_from=248,
        changed_vertices=int(changed.sum()),maximum_added_offset_m=float(np.linalg.norm(offset,axis=2).max()),opposite_source_normals=reverse,
        local_fold_limiter_passes=limited_passes,local_fold_limiter_sigma_m=10,vertices_attenuated_over_one_percent=int(np.count_nonzero(attenuation<.99)),
        method='Restore pre-cut source radial/vertical fracture residuals; retain supported macro apron')
    meta['shoulder_fractures']=correction;meta['exported_minimum_twice_triangle_area_m2']=minimum;meta['exported_collapsed_triangles']=collapsed
    data['vertices_m']=vertices;data['uv0']=uv.reshape(-1,2);data['metadata_json']=np.asarray(json.dumps(meta))
    args.output.mkdir(parents=True);out=args.output/'candidate.npz';np.savez_compressed(out,**data)
    report=dict(parent,source=str(out.resolve()),source_sha256=sha(out),shoulder_fractures=correction,shore_mist_centers_m=shores,bank_boulder_anchors_m=anchors,exported_minimum_twice_triangle_area_m2=minimum,exported_collapsed_triangles=collapsed)
    (args.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(source=str(out),sha256=sha(out),correction=correction,min_area=minimum),indent=2))

if __name__=='__main__':main()
