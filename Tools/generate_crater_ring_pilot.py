"""Build one joined compressed-parent pilot in the unchanged production crater ring.

CPU-only: extracts current production positions through the existing source probe,
then authors one bounded arc. No Unreal launch, import, gameplay or shader edits.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import numpy as np
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
from export_cliff_erosion_seed import source_snapshot

EXPORT = r'''
int main(){
 ADCMaelstrom m;
 FILE* f=std::fopen("production_ring.bin","wb");if(!f)return 2;
 for(int row=0;row<=DCCliffGenerator::MeshRows;++row)for(int side=0;side<=DCCliffGenerator::MeshSectors;++side){
  const float A=side*UE_TWO_PI/DCCliffGenerator::MeshSectors;
  const FVector p=m.CliffSurfacePoint(A,float(row)/DCCliffGenerator::MeshRows);
  const double xyz[]={p.X*.01,p.Y*.01,p.Z*.01};
  if(std::fwrite(xyz,sizeof(double),3,f)!=3)return 3;
 }
 std::fclose(f);f=std::fopen("production_shore.bin","wb");if(!f)return 4;
 for(int side=0;side<=DCCliffGenerator::MeshSectors;++side){
  const float A=side*UE_TWO_PI/DCCliffGenerator::MeshSectors,E=m.WaterEdgeRadius(A);
  const FVector2D D(FMath::Cos(A),FMath::Sin(A));
  const double data[]={A,E*.01,m.HeightAt(D*E,false)*.01,m.HeightAt(D*(E-400.f),false)*.01};
  if(std::fwrite(data,sizeof(double),4,f)!=4)return 5;
 }
 std::fclose(f);return 0;
}
'''


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def smooth(x):
    t=np.clip(x,0,1);return t*t*(3-2*t)
def load(p):
    with np.load(p,allow_pickle=False) as z:return {k:z[k].copy() for k in z.files}


def polygon_distance(xy,poly):
    """Unsigned distance outside a convex footprint, zero inside."""
    sign=[];dist=np.full(len(xy),np.inf)
    for a,b in zip(poly,np.roll(poly,-1,axis=0)):
        e=b-a;p=xy-a;den=float(e@e)
        sign.append(e[0]*p[:,1]-e[1]*p[:,0])
        t=np.clip((p@e)/max(den,1.e-20),0,1)
        dist=np.minimum(dist,np.linalg.norm(p-t[:,None]*e,axis=1))
    cross=np.stack(sign,axis=1);inside=(cross.min(axis=1)>=-1.e-8)|(cross.max(axis=1)<=1.e-8)
    return np.where(inside,0.,dist)


def query_heights(grid,points):
    """Topmost exact rendered BC-triangle query at requested arena XY."""
    angles=np.unwrap(np.arctan2(grid[0,:,1],grid[0,:,0])); n=grid.shape[1]-1
    qangle=np.arctan2(points[:,1],points[:,0])%(2*np.pi)
    sectors=np.clip(np.searchsorted(angles,qangle,side='right')-1,0,n-1)
    results=[]
    for p,s in zip(points,sectors):
        a=grid[:-1,s];b=grid[:-1,s+1];c=grid[1:,s];d=grid[1:,s+1]
        tri=np.stack((np.stack((a,c,b),1),np.stack((b,c,d),1)),1).reshape(-1,3,3)
        e=tri[:,1,:2]-tri[:,0,:2];f=tri[:,2,:2]-tri[:,0,:2];v=p-tri[:,0,:2]
        det=e[:,0]*f[:,1]-e[:,1]*f[:,0];safe=np.where(abs(det)>1.e-12,det,1.)
        u=(v[:,0]*f[:,1]-v[:,1]*f[:,0])/safe;w=(e[:,0]*v[:,1]-e[:,1]*v[:,0])/safe
        ok=(abs(det)>1.e-12)&(u>=-1.e-7)&(w>=-1.e-7)&(u+w<=1+1.e-7)
        z=tri[:,0,2]+u*(tri[:,1,2]-tri[:,0,2])+w*(tri[:,2,2]-tri[:,0,2])
        results.append(float(z[ok].max()) if ok.any() else float('nan'))
    return np.array(results)


def ring_triangles(rows,cols):
    a=(np.arange(rows-1)[:,None]*cols+np.arange(cols-1)[None,:]).ravel()
    return np.stack((np.stack((a,a+cols,a+1),1),np.stack((a+1,a+cols,a+cols+1),1)),1).reshape(-1,3).astype(np.int32)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project-root',type=Path,required=True)
    p.add_argument('--compressed',type=Path,required=True)
    p.add_argument('--anchors',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--engine',type=Path,default=Path('E:/Utilities/UE_5.8'))
    p.add_argument('--vcvars',type=Path,default=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat'))
    p.add_argument('--center-angle',type=float,default=2.35)
    p.add_argument('--width-m',type=float,default=500.)
    args=p.parse_args();out=args.output.resolve()
    if out.exists() or 'saved' not in [v.lower() for v in out.parts]:p.error('Use a new ignored Saved output directory')
    out.mkdir(parents=True)
    code,provenance=source_snapshot(args.project_root.resolve(),None,args.engine)
    cpp=out/'export_ring.cpp';exe=out/'export_ring.exe';cpp.write_text(code+EXPORT,encoding='utf-8')
    batch=out/'compile.cmd';batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "export_ring.obj"}"\n',encoding='utf-8')
    subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
    subprocess.run([str(exe)],cwd=out,check=True)
    old=np.fromfile(out/'production_ring.bin',dtype='<f8').reshape(161,1537,3)
    shore=np.fromfile(out/'production_shore.bin',dtype='<f8').reshape(1537,4)
    angles=shore[:,0];edge=shore[:,1];edgez=shore[:,2];r0=np.linalg.norm(old[:,:,:2],axis=2)
    delta=(args.center_angle-angles+np.pi)%(2*np.pi)-np.pi
    arc=1600.*delta;blend=smooth((args.width_m*.5-abs(arc))/60.)
    compressed=load(args.compressed);meta0=json.loads(str(compressed['metadata_json']))
    origin=compressed['frame_origin_world_m'];basis=np.stack([compressed[k] for k in ('basis_u_world','basis_v_world','basis_z_world')],axis=1)
    local=(compressed['vertices_m']-origin)@basis
    cu=compressed['contact_uv_m'][:,0];nc=len(cu);source=local.reshape(-1,nc,3)
    chart_u=(cu[0]+cu[-1])*.5+arc
    sampled=np.empty((len(source),len(angles),3))
    for row in range(len(source)):
        for axis in range(3):sampled[row,:,axis]=np.interp(chart_u,cu,source[row,:,axis])
    refshore=np.interp(chart_u,cu,compressed['contact_uv_m'][:,1])
    refwater=np.interp(chart_u,cu,compressed['contact_water_height_m'])
    profiles=sampled[4:]
    # Allocate actual shoreline as an explicit row; broad upper back-join remains
    # connected to the untouched legacy outer plateau. Row topology is unchanged.
    target=old.copy();chart=np.zeros_like(old);target_r=r0.copy();face=np.zeros((161,1537))
    contact_row=4;main_start=8;main_end=120
    source_s=np.linspace(0,1,len(profiles))
    for row in range(161):
        if row<contact_row:
            t=row/contact_row
            target_r[row]=(1-t)*r0[0]+t*edge
            target[row,:,2]=(1-t)*old[0,:,2]+t*edgez
            chart[row]=sampled[0]
        elif row<main_start:
            t=(row-contact_row)/(main_start-contact_row)
            target_r[row]=edge+6.*t
            target[row,:,2]=edgez+t*(profiles[0,:,2]-refwater)
            chart[row]=sampled[1]+t*(profiles[0]-sampled[1])
        elif row<=main_end:
            s=(row-main_start)/(main_end-main_start)
            q=s*(len(profiles)-1);j=min(int(q),len(profiles)-2);w=q-j
            point=profiles[j]*(1-w)+profiles[j+1]*w
            target_r[row]=edge+point[:,1]-refshore
            target[row,:,2]=point[:,2]+edgez-refwater
            chart[row]=point
            face[row]=smooth((row-main_start)/6.)*(1-smooth((row-main_end+6.)/6.))
        else:
            t=(row-main_end)/(160-main_end)
            crest_r=edge+profiles[-1,:,1]-refshore
            crest_z=profiles[-1,:,2]+edgez-refwater
            target_r[row]=crest_r+(r0[-1]-crest_r)*t
            target[row,:,2]=crest_z*(1-smooth(t))+old[row,:,2]*smooth(t)
            chart[row]=profiles[-1];chart[row,:,1]+=target_r[row]-crest_r
    target[:,:,:2]=np.stack((np.cos(angles),np.sin(angles)),axis=1)[None,:,:]*target_r[:,:,None]
    # Preserve temples crossing the side boundary by narrowing the connected side
    # collar. Interior temples may move vertically; no old-height islands.
    anchors=json.loads(args.anchors.read_text())['anchors'];boundary_anchors=[]
    for a in anchors:
        poly=np.asarray(a['footprint_xy_m']);aa=np.arctan2(poly[:,1],poly[:,0])
        ar=1600.*((args.center_angle-aa+np.pi)%(2*np.pi)-np.pi)
        if ar.min()<args.width_m*.5 and ar.max()>-args.width_m*.5 and (ar.min()<-args.width_m*.5 or ar.max()>args.width_m*.5):
            margin=float(a.get('margin_m',10))+float(a.get('blend_m',30))
            if ar.max()>0: blend*=smooth((ar.min()-margin-arc)/35.)
            else: blend*=smooth((arc-ar.max()-margin)/35.)
            boundary_anchors.append(a['id'])
    result=old+(target-old)*blend[None,:,None]
    result[0]=old[0];result[-1]=old[-1]
    influence=face*blend[None,:]
    # Broad support pads follow NEW plateau height. Only affected footprints
    # wholly inside the arc are moved; flatten a generously supported foundation,
    # then blend30m into neighboring new plateau. Record the actual relocation.
    xy=result[:,:,:2].reshape(-1,2);relocations=[];pad_weight=np.zeros(len(xy))
    pads=[]
    for anchor in anchors:
        poly=np.asarray(anchor['footprint_xy_m']);points=np.array([[x['x_m'],x['y_m']] for x in anchor['ground_samples']])
        center=poly.mean(0);ca=np.arctan2(center[1],center[0]);cb=float(np.interp(ca%(2*np.pi),angles,blend))
        if cb<.05 or anchor['id'] in boundary_anchors:continue
        distance=polygon_distance(xy,poly)
        if not (distance<55.).any():continue
        before=query_heights(result,points)
        if not np.isfinite(before).all():raise ValueError('Affected temple footprint escapes pilot: '+anchor['id'])
        pads.append(dict(anchor=anchor,poly=poly,points=points,before=before,distance=distance))
    # Overlapping inner support bands become one broad platform. Soft neighboring
    # blends must NEVER modify an already established foundation plane.
    group=list(range(len(pads)))
    def root(i):
        while group[i]!=i:i=group[i]
        return i
    for i in range(len(pads)):
        for j in range(i):
            separation=min(polygon_distance(pads[i]['poly'],pads[j]['poly']).min(),
                           polygon_distance(pads[j]['poly'],pads[i]['poly']).min())
            if separation<40.:group[root(i)]=root(j)
    targets={g:float(np.median(np.concatenate([x['before'] for i,x in enumerate(pads) if root(i)==g])))
             for g in {root(i) for i in range(len(pads))}}
    sumw=np.zeros(len(xy));sumz=np.zeros(len(xy));hard=np.zeros(len(xy),bool);hardz=np.zeros(len(xy))
    for i,pad in enumerate(pads):
        w=1.-smooth((pad['distance']-20.)/30.);w*=np.tile(blend,161)
        w[:1537]=0.;w[-1537:]=0.
        targetz=targets[root(i)];sumw+=w;sumz+=w*targetz;pad_weight=np.maximum(pad_weight,w)
        fixed=w>=1.-1.e-12;hardz[fixed]=targetz;hard|=fixed
        pad['target']=targetz
    flat=result.reshape(-1,3);weighted=np.divide(sumz,np.maximum(sumw,1.e-30))
    flat[:,2]=flat[:,2]*(1-pad_weight)+weighted*pad_weight
    flat[hard,2]=hardz[hard]
    # Final-after-ALL-pads checks are the only support certification reported.
    for pad in pads:
        anchor=pad['anchor'];support=pad['target'];after=query_heights(result,pad['points'])
        error=float(np.max(abs(after-support)))
        if error>1.e-5:raise ValueError('Final temple pad is not planar: '+anchor['id']+' '+str(error))
        relocations.append(dict(id=anchor['id'],original_support_height_m=anchor['support_height_m'],
            target_support_height_m=support,translation_world_m=[0.,0.,support-anchor['support_height_m']],
            footprint_xy_m=anchor['footprint_xy_m'],support_samples_before_m=pad['before'].tolist(),support_samples_after_m=after.tolist(),
            support_range_m=float(np.ptp(after)),maximum_plane_error_m=error,
            authored_relocation=True,applied_to_live_actors=False))
    influence*=1.-pad_weight.reshape(161,1537)
    triangles=ring_triangles(161,1537)
    changed=np.linalg.norm(result-old,axis=2)>1.e-9
    outside=blend==0
    if not np.array_equal(result[:,outside],old[:,outside]):raise ValueError('Outside pilot changed')
    cross=np.cross(result.reshape(-1,3)[triangles[:,1]]-result.reshape(-1,3)[triangles[:,0]],result.reshape(-1,3)[triangles[:,2]]-result.reshape(-1,3)[triangles[:,0]])
    if not np.isfinite(result).all() or np.any(np.linalg.norm(cross,axis=1)<=1.e-10):raise ValueError('Invalid pilot triangles')
    metadata=dict(schema='directorscut.crater_ring_pilot.v1',source_kind='production_ring_with_authored_compressed_parent_pilot',
        arena_radius_m=1600.,arena_depth_m=900.,units='absolute arena metres',source_provenance=provenance,
        compressed_path=str(args.compressed.resolve()),compressed_sha256=sha(args.compressed),
        anchors_path=str(args.anchors.resolve()),anchors_sha256=sha(args.anchors),generator_sha256=sha(__file__),
        pilot=dict(center_angle_rad=args.center_angle,width_at_radius1600_m=args.width_m,side_collar_m=60.,
                   main_rows=[main_start,main_end],contact_row=contact_row,back_join_rows=[main_end,160],
                   unchanged_side_temple_groups=boundary_anchors),
        coordinates='Identity arena frame. Exporter must retain documented metres-to-cm and FBX Y reflection; runtime actor inherits arena transform.',
        metrics=dict(vertices=result.size//3,triangles=len(triangles),changed_vertices=int(changed.sum()),
                     changed_columns=int(changed.any(axis=0).sum()),outside_columns_bitwise_unchanged=True,
                     minimum_twice_area_m2=float(np.linalg.norm(cross,axis=1).min()),
                     maximum_displacement_m=float(np.linalg.norm(result-old,axis=2).max())),
        landmark_relocations=relocations,colors='No vertex colors exported: do not mix flow masks with runtime wetness.',
        limitations='Pilot only; outside source unchanged, no duplicated sectors. Side/back blends require native visual judgement. Existing duplicate polar seam retained. Relocations are proposals until actor transforms are applied. No final art or whole-arena collision approval.',runtime_integrated=False)
    common=dict(frame_origin_world_m=np.zeros(3),basis_u_world=np.array([1.,0,0]),basis_v_world=np.array([0.,1,0]),basis_z_world=np.array([0.,0,1]))
    for name,verts in [('baseline',old),('candidate',result)]:
        np.savez_compressed(out/(name+'.npz'),vertices_m=verts.reshape(-1,3),triangles=triangles,
            pilot_weight=np.broadcast_to(blend,(161,1537)).reshape(-1),fracture_weight=influence.reshape(-1),
            support_weight=pad_weight,compressed_chart_uvz_m=chart.reshape(-1,3),
            source_ring_vertex_index=np.arange(result.size//3,dtype=np.int32),
            metadata_json=np.asarray(json.dumps(metadata)),**common)
    (out/'manifest.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
    (out/'landmark_relocations.json').write_text(json.dumps(dict(schema='directorscut.cliff_landmark_relocations.v1',coordinate_frame='arena_local_metres',relocations=relocations,unchanged_boundary_groups=boundary_anchors),indent=2),encoding='utf-8')
    moved={a['id']:a['translation_world_m'] for a in relocations}
    support_actors=[dict(id=a['id'],footing_bottom_m=a['footing_bottom_m'],
        centre_xy_m=np.asarray(a['footprint_xy_m']).mean(axis=0).tolist(),
        offset_arena_m=moved.get(a['id'],[0.,0.,0.])) for a in anchors]
    (out/'preview_supports.json').write_text(json.dumps(dict(schema='directorscut.cliff_preview_supports.v1',
        source_fbx_sha256='',source_anchor_sha256=sha(args.anchors),actors=support_actors,
        runtime_integration_approved=False,limitations='Original placements must be verified before optional offsets. Preview only; restore original transforms on exit.'),indent=2),encoding='utf-8')
    print(json.dumps(metadata['metrics'],indent=2),flush=True)
    print('LANDMARKS',json.dumps(relocations),flush=True)


if __name__=='__main__':main()
