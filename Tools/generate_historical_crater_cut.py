"""Rigid historical headland placement and a measured-water crater excavation.

No erosion, scale, bending, noise, rock pockets or modern-foundation claims.
Original parent triangles are clipped against a columnwise affine70deg cut;
the resulting open visible surface and closed authoring bedrock are exported.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import numpy as np

sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
from generate_weathered_cliff_mesh import original_mesh

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def clip(poly,fn,sign=1.):
    if not poly:return []
    output=[];a=poly[-1];da=sign*fn(a);ina=da>=-1.e-10
    for b in poly:
        db=sign*fn(b);inb=db>=-1.e-10
        if inb!=ina:output.append(a+(b-a)*(da/(da-db)))
        if inb:output.append(b)
        a,da,ina=b,db,inb
    return output

class Mesh:
    def __init__(self):self.vertices=[];self.triangles=[];self.kind=[];self.parent=[];self.index={}
    def vertex(self,p):
        key=tuple(np.round(p,8))
        if key not in self.index:
            self.index[key]=len(self.vertices);self.vertices.append(np.asarray(p,float))
        return self.index[key]
    def polygon(self,points,kind,parent):
        if len(points)<3:return
        ids=[self.vertex(p) for p in points]
        for i in range(1,len(ids)-1):
            t=[ids[0],ids[i],ids[i+1]]
            if len(set(t))<3:continue
            a,b,c=[self.vertices[j] for j in t]
            if np.linalg.norm(np.cross(b-a,c-a))<1.e-9:continue
            self.triangles.append(t);self.kind.append(kind);self.parent.append(parent)
    def arrays(self):
        vertices=np.asarray(self.vertices);triangles=np.asarray(self.triangles,np.int32)
        used=np.unique(triangles);remap=np.full(len(vertices),-1,np.int32);remap[used]=np.arange(len(used))
        return vertices[used],remap[triangles],np.asarray(self.kind,np.int8),np.asarray(self.parent,np.int32)

def water_queries(water,origin,basis,points):
    local=(water['vertices_m']-origin)@basis
    t=water['triangles'];uv=local[:,:2]
    bins={}
    for i,center in enumerate(uv[t].mean(axis=1)):
        key=tuple(np.floor(center/2.).astype(int));bins.setdefault(key,[]).append(i)
    heights=[];ids=[];weights=[]
    for point in points:
        cell=np.floor(point/2.).astype(int);found=None
        for du in (-1,0,1):
            for dv in (-1,0,1):
                for i in bins.get((cell[0]+du,cell[1]+dv),[]):
                    a,b,c=uv[t[i]];x=b-a;y=c-a;q=point-a
                    det=x[0]*y[1]-x[1]*y[0]
                    if abs(det)<1.e-12:continue
                    s=(q[0]*y[1]-q[1]*y[0])/det;r=(x[0]*q[1]-x[1]*q[0])/det
                    w=np.array([1.-s-r,s,r])
                    if w.min()>=-1.e-8:
                        found=(i,w);break
                if found:break
            if found:break
        if found is None:raise ValueError(f'Lower overlap escapes finite receiving-water triangles:{point}')
        i,w=found;heights.append(float(w@water['vertices_m'][t[i],2]));ids.append(i);weights.append(w)
    return np.asarray(heights),np.asarray(ids,np.int32),np.asarray(weights)

def save(path,local,triangles,kinds,parent_ids,origin,basis,meta,extra=None):
    world=origin+local@basis.T
    np.savez_compressed(path,vertices_m=world,triangles=triangles,face_kind=kinds,parent_triangle_index=parent_ids,
        frame_origin_world_m=origin,basis_u_world=basis[:,0],basis_v_world=basis[:,1],basis_z_world=basis[:,2],
        metadata_json=np.asarray(json.dumps(meta)),**(extra or {}))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--parent',type=Path,required=True)
    p.add_argument('--source-coast',type=Path,required=True)
    p.add_argument('--water',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if args.output.exists() or 'saved' not in [x.lower() for x in args.output.resolve().parts]:p.error('Use new ignored Saved output')
    def load(path):
        with np.load(path,allow_pickle=False) as z:return {k:z[k].copy() for k in z.files}
    parent=load(args.parent);source=load(args.source_coast);water=load(args.water)
    origin=parent['frame_origin_world_m'];basis=np.stack([parent['basis_u_world'],parent['basis_v_world'],parent['basis_z_world']],axis=1)
    if not np.allclose(basis.T@basis,np.eye(3),atol=1.e-12) or not np.isclose(np.linalg.det(basis),1.):raise ValueError('Parent frame must be rigid')
    wm=json.loads(str(water['metadata_json']));query_path=Path(wm['source_queries'])
    if sha(query_path)!=wm['query_sha256']:raise ValueError('Water source query bytes changed')
    q=np.fromfile(query_path,dtype='<f8').reshape(253,251,4)
    if not np.allclose(q[:,:,:2],source['world_xy_m'],atol=1.e-9,rtol=0.):raise ValueError('Coast query frame mismatch')
    columns=np.arange(20,221);uu=2.*columns
    crossings=(q[:-1,:,3]>=0)&(q[1:,:,3]<0)
    if not np.all(crossings[:,columns].sum(axis=0)==1):raise ValueError('Expected one real outer water edge')
    rows=np.argmax(crossings[:,columns],axis=0)
    w=q[rows,columns,3]/(q[rows,columns,3]-q[rows+1,columns,3])
    shore_v=2.*(rows+w)
    shore_z=q[rows,columns,2]*(1.-w)+q[rows+1,columns,2]*w
    slope=float(np.tan(np.deg2rad(70.)))
    lower_v=shore_v-4.
    lower_water,water_ids,water_weights=water_queries(water,origin,basis,np.c_[uu,lower_v])
    lower_z=shore_z-4.*slope
    if np.any(lower_z>=lower_water):raise ValueError('Authored overlap creates dry rock inside finite water')
    transformed_parent_vertices,pt,grid_triangles,_=original_mesh(parent)
    parent_index={tuple(t):i for i,t in enumerate(grid_triangles)}
    translation=-600.*basis[:,1]+np.array([0.,0.,-55.])
    matrix=np.eye(4);matrix[:3,3]=translation
    terrain=parent['height_m'];base=Mesh();visible=Mesh()
    def interpolated(point,values):return float(np.interp(point[0],uu,values))
    def shorefn(point):return point[1]-interpolated(point,lower_v)
    def cutheight(point):return interpolated(point,shore_z)+slope*(point[1]-interpolated(point,shore_v))
    # Source rows300..510 become localV0..420 under the rigid600m shift.
    # All selected original parent cells are valid. No XY deformation occurs.
    for row in range(300,510):
        for col in range(20,220):
            ids=[row*251+col,row*251+col+1,(row+1)*251+col,(row+1)*251+col+1]
            points=[np.array([2.*col,2.*row-600.,terrain[row,col]-55.]),
                    np.array([2.*(col+1),2.*row-600.,terrain[row,col+1]-55.]),
                    np.array([2.*col,2.*(row+1)-600.,terrain[row+1,col]-55.]),
                    np.array([2.*(col+1),2.*(row+1)-600.,terrain[row+1,col+1]-55.])]
            for corners in ((0,1,3),(0,3,2)):
                source_id=parent_index[tuple(ids[i] for i in corners)]
                poly=clip([points[i] for i in corners],shorefn)
                if not poly:continue
                base.polygon(poly,0,source_id)
                distance=lambda point:point[2]-cutheight(point)
                kept=clip(poly,distance,-1.)
                cut=clip(poly,distance,1.)
                visible.polygon(kept,0,source_id)
                visible.polygon([np.array([a[0],a[1],cutheight(a)]) for a in cut],1,source_id)
    vertices,triangles,kinds,parent_ids=visible.arrays()
    bv,bt,bk,bi=base.arrays()
    edges=np.concatenate([triangles[:,[0,1]],triangles[:,[1,2]],triangles[:,[2,0]]])
    _,first,count=np.unique(np.sort(edges,axis=1),axis=0,return_index=True,return_counts=True)
    if np.any(count>2):raise ValueError('Visible mesh is nonmanifold')
    boundary=edges[first[count==1]]
    bottom=float(min(vertices[:,2].min(),lower_z.min())-30.)
    nv=len(vertices);closed_vertices=np.concatenate([vertices,np.c_[vertices[:,:2],np.full(nv,bottom)]])
    sides=[]
    for a,b in boundary:sides.extend([[b,a,a+nv],[b,a+nv,b+nv]])
    closed_faces=np.concatenate([triangles,triangles[:,::-1]+nv,np.asarray(sides,np.int32)])
    closed_kind=np.r_[kinds,np.full(len(triangles),2,np.int8),np.full(len(sides),3,np.int8)]
    closed_parent=np.r_[parent_ids,np.full(len(triangles)+len(sides),-1,np.int32)]
    # Parent-water clearance at the exact interpolated shoreline; rigidH only.
    h=[]
    for col,v in zip(columns,shore_v):
        r=(v+600.)/2.;lo=int(np.floor(r));t=r-lo
        h.append(terrain[lo,col]*(1.-t)+terrain[lo+1,col]*t-55.)
    gap=np.asarray(h)-shore_z
    if gap.min()<=0.:raise ValueError('A parent ravine lies below modern water; do not conceal it')
    meta=dict(schema='directorscut.historical_crater_cut.v1',source_kind='historical_headland_crater_cut',
        parent_path=str(args.parent.resolve()),parent_sha256=sha(args.parent),source_coast_path=str(args.source_coast.resolve()),
        source_coast_sha256=sha(args.source_coast),water_mesh_path=str(args.water.resolve()),water_mesh_sha256=sha(args.water),
        water_query_path=str(query_path),water_query_sha256=sha(query_path),generator_sha256=sha(__file__),
        rigid_parent_to_arena_matrix=matrix.tolist(),scale=1.,rotation_degrees=0.,chart_translation_uvz_m=[0.,-600.,-55.],
        crop_chart_uv_m=[[40.,440.],[0.,420.]],parent_rows_inclusive=[300,510],parent_cols_inclusive=[20,220],
        units='absolute arena metres, original physical parent triangles clipped only; no bending/scaling',
        cut=dict(angle_degrees=70.,equation='Z=interpolatedshoreHeight+tan70*(V−interpolatedshoreV)',
            lower_overlap_waterward_m=4.,shore_interpolation='clearancezero and HeightAt jointly interpolated between actual2m query rows',
            lower_water_check='barycentric exact finite meanwater triangle',parent_top_unchanged_except_clipped_region=True),
        metrics=dict(visible_vertices=len(vertices),visible_triangles=len(triangles),natural_top_triangles=int((kinds==0).sum()),
            excavation_face_triangles=int((kinds==1).sum()),boundary_edges=len(boundary),
            skyline_range_m=[float(vertices[:,2].min()),float(vertices[:,2].max())],
            shore_parent_above_water_range_m=[float(gap.min()),float(gap.max())],
            lower_overlap_submergence_range_m=[float((lower_water-lower_z).min()),float((lower_water-lower_z).max())]),
        face_kinds={'0':'rigidoriginalparenttop','1':'newcurvedexcavationface','2':'artificialbottom','3':'artificialcropclosure'},
        artificial_bottom_z_m=bottom,visible_surface_excludes_all_authoring_caps=True,
        modern_local_contact_approximation=True,entire_route_verified=False,old_outer_seams_preserved=False,
        temple_support_preserved=False,runtime_integrated=False,visual_accepted=False,
        limitations='2m sampled shore interpolation, static meanwater without waves.70degree excavation is macroshape, not finishedrock. Historicalparent not converged. Arbitrary crop closure is authoring-only.')
    args.output.mkdir(parents=True)
    extra=dict(contact_uv_m=np.c_[uu,shore_v],contact_water_height_m=shore_z,
        lower_overlap_uv_m=np.c_[uu,lower_v],lower_overlap_ground_height_m=lower_z,
        lower_water_height_m=lower_water,lower_water_triangle_index=water_ids,lower_water_barycentric=water_weights)
    save(args.output/'baseline.npz',bv,bt,bk,bi,origin,basis,dict(meta,variant='placed_uncut_parent'))
    save(args.output/'candidate.npz',vertices,triangles,kinds,parent_ids,origin,basis,dict(meta,variant='visible_cut_surface'),extra)
    save(args.output/'closed_bedrock.npz',closed_vertices,closed_faces,closed_kind,closed_parent,origin,basis,dict(meta,variant='closed_authoring_bedrock'),extra)
    (args.output/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    shutil.copyfile(__file__,args.output/'generator_source.py')
    print(json.dumps(dict(output=str(args.output.resolve()),metrics=meta['metrics']),indent=2))

if __name__=='__main__':main()
