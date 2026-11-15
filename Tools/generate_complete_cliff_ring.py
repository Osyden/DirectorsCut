"""Continuous full-circumference cliff, preserving canonical toe/outer edges.

Authored rock architecture and scan-derived planar relief; not erosion physics.
No cloned sectors, boulder instances, or changes to water/sky/runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from generate_jointed_headland import sample_ring
from generate_continuous_scan_cliff import smooth,lookup

CIRCUMFERENCE=2*np.pi*1600.


def cyclic_profile(u):
    rng=np.random.default_rng(15731)
    widths=rng.uniform(125.,340.,48);widths*=CIRCUMFERENCE/widths.sum()
    knots=np.r_[0.,np.cumsum(widths)]
    heights=rng.uniform(34.,65.,48);radii=rng.uniform(1550.,1665.,48)
    q=np.mod(u,CIRCUMFERENCE)
    k=np.clip(np.searchsorted(knots,q,side='right')-1,0,47)
    f=(q-knots[k])/widths[k]
    # Flat-topped unequal supports, with finite straight shoulders between.
    blend=smooth((f-.25)/.50)
    return heights[k]*(1-blend)+heights[(k+1)%48]*blend,radii[k]*(1-blend)+radii[(k+1)%48]*blend


def scan_forms(tile,u,z,feature_scale=1.,amplitude=1.):
    angle=u/1600.;x=1600*np.cos(angle);y=1600*np.sin(angle)
    result=np.zeros_like(u)
    for aa,bb,cc,dd,phase,weight in [(1.,.43,-.37,.86,.2,.72),(.62,-.79,.83,.56,2.1,.38)]:
        su=.5+.44*np.sin((aa*x+bb*y+.61*z)/(450*feature_scale)+phase)
        sv=.5+.44*np.sin((cc*x+dd*y-.9*z)/(320*feature_scale)-phase)
        result+=weight*lookup(tile,np.c_[su,sv],True)
    return np.clip(24*result,-14,14)*amplitude


def planar_forms(tile,u,z,cell_scale=1.,amplitude=1.,surface_coordinate=False):
    count=round(420/cell_scale);step=CIRCUMFERENCE/count;vertical_step=24*cell_scale
    zlo=-26 if not surface_coordinate else int(np.floor(-650/vertical_step))-1
    zhi=7 if not surface_coordinate else int(np.ceil(850/vertical_step))+1
    gx,gz=np.meshgrid(np.arange(count),np.arange(zlo,zhi+1))
    rng=np.random.default_rng(591736)
    sites=np.c_[gx.ravel()*step+step*.5,gz.ravel()*vertical_step+vertical_step*.5]
    sites+=rng.uniform(-.40,.40,sites.shape)*[step,vertical_step]
    dx,dz=np.meshgrid(np.array([-8.,0.,8.])*cell_scale,np.array([-8.,0.,8.])*cell_scale)
    offsets=np.c_[dx.ravel(),dz.ravel()]
    samples=sites[:,None,:]+offsets
    h=scan_forms(tile,samples[:,:,0].ravel(),samples[:,:,1].ravel(),cell_scale,amplitude).reshape(-1,9)
    fit=np.linalg.lstsq(np.c_[np.ones(9),offsets],h.T,rcond=None)[0].T
    fit[:,1:]=np.clip(fit[:,1:],-1.,1.)
    result=np.empty(len(u))
    for begin in range(0,len(u),150000):
        qu=np.mod(u[begin:begin+150000],CIRCUMFERENCE)
        qz=np.clip(z[begin:begin+150000],(zlo+1)*vertical_step,(zhi-1)*vertical_step)
        ix=np.floor(qu/step).astype(int);iz=np.clip(np.floor(qz/vertical_step).astype(int),zlo+1,zhi-1)
        ds=[];values=[]
        for oz in [-1,0,1]:
            for ox in [-1,0,1]:
                ids=(iz+oz-zlo)*count+(ix+ox)%count
                du=(qu-sites[ids,0]+CIRCUMFERENCE*.5)%CIRCUMFERENCE-CIRCUMFERENCE*.5
                dv=qz-sites[ids,1]
                ds.append(du*du+dv*dv)
                values.append(fit[ids,0]+fit[ids,1]*du+fit[ids,2]*dv)
        ds=np.asarray(ds);weights=np.exp(np.maximum(-60.,-(ds-ds.min(0))/(2*step*1.2)))
        result[begin:begin+len(qu)]=np.clip(np.sum(weights*np.asarray(values),axis=0)/weights.sum(0),-14*amplitude,14*amplitude)
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True);p.add_argument('--tile',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--angular-segments',type=int,default=3072)
    p.add_argument('--face-rows',type=int,default=240)
    p.add_argument('--surface-rock',action='store_true',help='Physical face-distance facets displaced along the base wall normal')
    a=p.parse_args()
    if a.output.exists():p.error('Choose a new Saved output')
    with np.load(a.source,allow_pickle=False) as f:
        ring=f['original_ring_vertices_m'].reshape(161,1537,3).copy()
        original_tri=f['original_ring_triangles'].copy()
        origin=f['frame_origin_world_m'].copy()
    with np.load(a.tile,allow_pickle=False) as f:tile=f['depth_m'].astype(float)
    rows=np.r_[np.arange(9),np.linspace(8,120,a.face_rows+1)[1:],np.linspace(120,160,33)[1:]]
    cols=np.linspace(0,1536,a.angular_segments+1)
    rr,cc=np.meshgrid(rows,cols,indexing='ij');r=rr.ravel();c=cc.ravel()
    angle=c*2*np.pi/1536.;u=angle*1600
    old=sample_ring(ring,r,c)
    toe=sample_ring(ring,np.full(len(r),8.),c)
    outer=sample_ring(ring,np.full(len(r),160.),c)
    toe_r=np.linalg.norm(toe[:,:2],axis=1);outer_r=np.linalg.norm(outer[:,:2],axis=1)
    crest,crest_r=cyclic_profile(u)
    crest=np.maximum(crest,toe[:,2]+36)
    crest_r=np.minimum(np.maximum(crest_r,toe_r+62),outer_r-90)
    crest+=.25*planar_forms(tile,u,crest)
    # Evaluate at final crest height so the face and plateau share one radius.
    top_relief=planar_forms(tile,u,crest)
    crest_r-=top_relief
    t=np.clip((r-8)/112.,0,1);z=toe[:,2]+(crest-toe[:,2])*t
    # A finite lower erosion apron supports the tall, steeper upper wall.
    radial=toe_r+(crest_r-toe_r)*(1-(1-t)**2.35)
    if a.surface_rock:
        # Use true meridian distance on the base wall. Z-only coordinates
        # stretched small rock faces across the several-hundred-metre apron.
        base_r=radial.reshape(len(rows),len(cols));base_z=z.reshape(base_r.shape)
        distances=np.r_[np.zeros((1,len(cols))),np.cumsum(np.hypot(np.diff(base_r,axis=0),np.diff(base_z,axis=0)),axis=0)]
        distances-=distances[8]
        face_distance=distances.ravel();face_length=np.broadcast_to(distances[8+a.face_rows],distances.shape).ravel()
        field_z=face_distance-550.
        secondary=planar_forms(tile,u,field_z,surface_coordinate=True)
        primary=planar_forms(tile,u,field_z,cell_scale=3.3,amplitude=2.4,surface_coordinate=True)
        collar=smooth(face_distance/15)*smooth((face_length-face_distance)/15)
        relief=(primary+secondary*.8)*collar
        dr=(crest_r-toe_r)*2.35*np.maximum(0,1-t)**1.35
        dz=crest-toe[:,2];length=np.maximum(np.hypot(dr,dz),1e-6)
        # Keep protrusions landward of the existing receiving-water toe,
        # including shallow-height entry sectors where a normal tilts sideways.
        outward_limit=.8*np.maximum(0,radial-toe_r)*length/np.maximum(dz,1e-6)
        relief=np.minimum(relief,outward_limit)
        radial-=relief*dz/length
        z+=relief*dr/length
    else:
        relief=planar_forms(tile,u,z)*smooth(t/.17)
        # Keep the crest identical to the back join after adding face facets.
        radial-=relief-top_relief*t
    vertices=np.c_[np.cos(angle)*radial,np.sin(angle)*radial,z]
    back=r>120;b=np.clip((r-120)/40,0,1)
    back_r=crest_r+(outer_r-crest_r)*b
    back_z=crest+(outer[:,2]-crest)*smooth((b-.60)/.40)
    vertices[back]=np.c_[np.cos(angle)*back_r,np.sin(angle)*back_r,back_z][back]
    fixed=(r<=8)|(r>=160)
    vertices[fixed]=old[fixed]
    nr,nc=len(rows),len(cols)
    # Exactly coincident seam geometry; UVs remain split to avoid interpolation
    # across the whole atlas on the closing angular strip.
    vertices=vertices.reshape(nr,nc,3);vertices[:,-1]=vertices[:,0];vertices=vertices.reshape(-1,3)
    ids=(np.arange(nr-1)[:,None]*nc+np.arange(nc-1)).ravel()
    triangles=np.concatenate([np.c_[ids,ids+nc,ids+1],np.c_[ids+1,ids+nc,ids+nc+1]]).astype(np.int32)
    radius=np.linalg.norm(vertices[:,:2],axis=1)
    uv=np.c_[(u-529*1600*2*np.pi/1536)/497.418836818384,
        (vertices[:,2]+.35*np.maximum(0,radius-crest_r)+200)/280].astype(np.float32)
    rgba=np.zeros((len(vertices),4),np.float32);rgba[:,3]=1
    minimum=np.inf;collapsed=0
    local32=(vertices-origin).astype(np.float32).astype(float)
    for start in range(0,len(triangles),200000):
        q=local32[triangles[start:start+200000]]
        area=np.linalg.norm(np.cross(q[:,1]-q[:,0],q[:,2]-q[:,0]),axis=1)
        minimum=min(minimum,float(area.min()));collapsed+=int((area<1e-8).sum())
    if collapsed or not np.isfinite(vertices).all():raise ValueError('Invalid exported geometry')
    canonical_c=np.arange(1537)*(a.angular_segments//1536)
    if a.angular_segments%1536:raise ValueError('Use an integer multiple of canonical angular segments')
    grid=vertices.reshape(nr,nc,3)
    toe_error=float(abs(grid[:9,canonical_c]-ring[:9]).max())
    outer_error=float(abs(grid[-1,canonical_c]-ring[-1]).max())
    metadata=dict(schema='directorscut.complete_cliff_ring.v1',source=str(a.source.resolve()),
        source_sha256=hashlib.sha256(a.source.read_bytes()).hexdigest(),scan_source=str(a.tile.resolve()),
        scan_sha256=hashlib.sha256(a.tile.read_bytes()).hexdigest(),
        arena_frame='XYZ metres; identity basis',grid_shape=[nr,nc],crest_row_index=8+a.face_rows,
        angular_segments=a.angular_segments,angular_step_metres=CIRCUMFERENCE/a.angular_segments,
        circumference_metres=CIRCUMFERENCE,canonical_toe_max_error_m=toe_error,
        canonical_outer_edge_max_error_m=outer_error,seam_position_error_m=float(abs(grid[:,0]-grid[:,-1]).max()),
        exported_minimum_twice_triangle_area_m2=minimum,exported_collapsed_triangles=collapsed,
        uv='U=(arc−529*1600*2pi/1536)/497.418836818384; V=(Z+.35max(0,R−crestR)+200)/280; seam split',
        vertex_color='R=0 everywhere: full context material, no former pilot atlas',
        form='48 unequal cyclic support transitions; nonrepeated scan-derived planar faces on420 jittered angular cells',
        scale='nominal1.6km crest radius, actual source toe/outer boundaries preserved',
        existing_waterfalls_valid=False,temples_require_actual_surface_query=True,
        surface_rock=a.surface_rock,
        relief_contract='actual meridian metres; ~24m subordinate and~79m primary fitted planes; base normal displacement;15m collars' if a.surface_rock else 'pilot-scale radial displacement',
        limitations='Authored whole-ring prototype; no physical erosion claim or full dynamic route validation')
    a.output.mkdir(parents=True)
    np.savez_compressed(a.output/'candidate.npz',vertices_m=vertices,triangles=triangles,uv0=uv,vertex_rgba=rgba,
        original_ring_vertices_m=ring.reshape(-1,3),original_ring_triangles=original_tri,
        ring_row_parameter=rows,ring_column_parameter=cols,metadata_json=np.asarray(json.dumps(metadata)),
        frame_origin_world_m=origin,basis_u_world=np.array([1.,0,0]),basis_v_world=np.array([0.,1,0]),basis_z_world=np.array([0.,0,1]))
    (a.output/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(json.dumps(dict(path=str(a.output/'candidate.npz'),vertices=len(vertices),triangles=len(triangles),**metadata),indent=2))


if __name__=='__main__':main()
