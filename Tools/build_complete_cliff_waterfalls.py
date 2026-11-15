"""Fit unequal full-ring cascades to the actual new cliff and production water.

Blender background, CPU only. Exports an immutable source-bound mesh bundle.
The authored runoff follows topmost rock triangles; the receiving water is a
fresh sampling of the selected project's C++ HeightAt, never an old pilot.
Spray is a scene-lit mesh proxy, not a reused VDB or a fluid simulation.
"""
import argparse, hashlib, json, math, subprocess, sys
from pathlib import Path
import bpy
import numpy as np
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
from attach_compressed_cliff_waterfall import Surface, current_at, load, sha
from export_cliff_erosion_seed import source_snapshot

WATER=r'''
int main(){
 ADCMaelstrom m;m.BuildBreakers(); FILE* f=std::fopen("water.bin","wb"); if(!f)return 2;
 for(int row=0;row<=220;++row)for(int side=0;side<=1536;++side){
  float a=side*UE_TWO_PI/1536,edge=m.WaterEdgeRadius(a);
  float r=FMath::Lerp(40000.f,edge,float(row)/220);
  FVector2D p(FMath::Cos(a)*r,FMath::Sin(a)*r);
  double v[]={p.X*.01,p.Y*.01,m.HeightAt(p)*.01};std::fwrite(v,8,3,f);
 }std::fclose(f);return 0;
}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cliff',type=Path,required=True)
    p.add_argument('--project-root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--prepare-water',action='store_true',help='Prepare receiver while the cliff worker finishes')
    p.add_argument('--finish',action='store_true',help='Finish a previously prepared receiver directory')
    p.add_argument('--engine',type=Path,default=Path('E:/Utilities/UE_5.8'))
    p.add_argument('--vcvars',type=Path,default=Path('C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat'))
    a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
    out=a.output.resolve()
    if (out.exists() and not a.finish) or 'saved' not in [x.lower() for x in out.parts]:p.error('Use a new Saved output directory, or --finish after --prepare-water')
    out.mkdir(parents=True,exist_ok=a.finish)
    code,provenance=source_snapshot(a.project_root.resolve(),None,a.engine)
    breaker_source=(a.project_root/'Source/DirectorsCut/DCMaelstromBreakers.cpp').read_text()
    build=breaker_source[breaker_source.index('void ADCMaelstrom::BuildBreakers()'):]
    code+=build[:build.index('    for(const FBreakingWave& W:BreakingWaves)')]+'}\n'
    provenance['breaker_population_sha256']=hashlib.sha256(build[:build.index('    for(const FBreakingWave& W:BreakingWaves)')].encode()).hexdigest()
    if a.finish:
        prepared=json.loads((out/'water_provenance.json').read_text())
        if prepared!=provenance:raise RuntimeError('Production water changed after preparation; choose a new output')
    else:
        cpp=out/'water.cpp';exe=out/'water.exe';cpp.write_text(code+WATER)
        batch=out/'compile.cmd'
        batch.write_text(f'@echo off\ncall "{a.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "water.obj"}"\n')
        subprocess.run(['cmd','/d','/c',str(batch)],cwd=out,check=True)
        subprocess.run([str(exe)],cwd=out,check=True)
        (out/'water_provenance.json').write_text(json.dumps(provenance,indent=2))
    if a.prepare_water:
        print('RECEIVER_READY '+str(out));return
    w=np.fromfile(out/'water.bin',dtype='<f8').reshape(221,1537,3)
    ix=(np.arange(220)[:,None]*1537+np.arange(1536)).ravel()
    wt=np.stack((np.stack((ix,ix+1537,ix+1),1),np.stack((ix+1,ix+1537,ix+1538),1)),1).reshape(-1,3)
    water_path=out/'receiving_water.npz'
    np.savez_compressed(water_path,vertices_m=w.reshape(-1,3),triangles=wt,metadata_json=json.dumps(provenance))
    cliff=load(a.cliff);rock=Surface(cliff['vertices_m'],cliff['triangles']);sea=Surface(w.reshape(-1,3),wt)
    vertices=[];faces=[];uv=[];slots=[];colors=[];proofs=[];support_ids=[];support_weights=[];face_source=[]
    source_centres=cliff['vertices_m'][cliff['triangles']].mean(axis=1)
    def strip(points,role,alpha=.45):
        points=np.asarray(points);rows,cols=points.shape[:2];base=len(vertices)
        arc=np.r_[0,np.cumsum(np.linalg.norm(np.diff(points[:,cols//2],axis=0),axis=1))]
        width=np.linalg.norm(points[0,-1]-points[0,0])
        for i in range(rows):
            for j in range(cols):
                vertices.append(points[i,j]);uv.append((j/(cols-1),arc[i]));colors.append((0,min(width/64,1),1,alpha))
        for i in range(rows-1):
            for j in range(cols-1):
                q=base+i*cols+j;faces.extend(((q,q+cols,q+1),(q+1,q+cols,q+cols+1)));slots.extend((role,role));face_source.extend((-1,-1))
    def rock_patch(radial,cross,width,start,end,crest_z):
        # A resampled ribbon can bridge fractured ledges even when all of its
        # vertices touch rock. Preserve original triangles instead, so every
        # interior point remains a parallel copy of the actual source face.
        along=source_centres@radial;across=source_centres@cross
        chosen=np.flatnonzero((along<=start)&(along>=end)&(np.abs(across)<width*.5))
        for triangle in chosen:
            points=cliff['vertices_m'][cliff['triangles'][triangle]];base=len(vertices)
            for point in points:
                at=point.copy();at[2]+=.28;vertices.append(at)
                lateral=float(point@cross);distance=math.hypot(start-float(point@radial),crest_z-point[2])
                uv.append((.5+lateral/width,distance));colors.append((0,min(width/64,1),1,.45))
            faces.append((base,base+1,base+2));slots.append(0 if (start-along[triangle])/(start-end)<.14 else 1);face_source.append(int(triangle))
        return len(chosen)
    # Deliberately irregular spacing and widths; keep the entry corridor clear.
    candidates=[(18,64),(52,108),(87,46),(120,125),(147,80),(215,44),(248,96),(286,70),(323,68),(342,38)]
    for number,(degrees,width) in enumerate(candidates):
        angle=math.radians(degrees);radial=np.array((math.cos(angle),math.sin(angle),0.));cross=np.array((-radial[1],radial[0],0.))
        # Scan inward through actual triangles. Start at the highest local crest
        # before its uninterrupted descent; stop where measured water wins.
        probes=[]
        for radius in np.arange(2150,900,-3.):
            xy=(radial*radius)[:2];h=rock.hit(xy);wet=sea.hit(xy)
            if h is not None:probes.append((radius,h,wet))
        if not probes:continue
        crest=max(range(len(probes)),key=lambda i:probes[i][1][0][2])
        seq=probes[crest:];contact=None
        for k,(radius,h,wet) in enumerate(seq):
            if wet is not None and wet[0][2]>=h[0][2]-.15:contact=k;break
        if contact is None:continue
        seq=seq[:contact+1]
        if len(seq)<8 or seq[0][1][0][2]-seq[-1][1][0][2]<20:continue
        # Narrow strips follow the highest exact rock hit, preserving source
        # triangle and barycentric evidence for every vertex before contact.
        track=[];valid=True;local_ids=[];local_weights=[];covered=0
        for k,(radius,_,_) in enumerate(seq):
            row=[];progress=k/(len(seq)-1)
            across_width=width*(.76+.24*math.sin(progress*math.pi))
            for offset in np.linspace(-.5,.5,17)*across_width:
                xy=(radial*radius+cross*offset)[:2];h=rock.hit(xy);wet=sea.hit(xy)
                if h is None and wet is None:valid=False;break
                on_water=wet is not None and (h is None or wet[0][2]>=h[0][2]-.15)
                hit=wet if on_water else h
                point=hit[0].copy();point[2]+=.22 if on_water else .28
                row.append(point)
                local_ids.append(-1 if on_water else hit[2]);local_weights.append(hit[3]);covered+=int(on_water)
            if not valid:break
            track.append(row)
        if not valid or len(track)<8:continue
        track=np.asarray(track);foot=track[-1,8].copy();wet=sea.hit(foot[:2])
        if wet is None:continue
        foot[2]=wet[0][2]
        patch_triangles=rock_patch(radial,cross,width,seq[0][0],seq[-1][0],track[0,8,2]-.28)
        support_ids.extend(local_ids);support_weights.extend(local_weights)
        # Each filament starts at its own measured receiving hit and follows
        # the production current direction with repeated water queries.
        contacts=[]
        for j in (2,5,8,11,14):
            start=track[-1,j].copy();hit=sea.hit(start[:2])
            if hit is None:continue
            start[2]=hit[0][2];contacts.append(start.copy())
            direction=current_at(start);direction/=np.linalg.norm(direction);across=np.array((-direction[1],direction[0],0))
            length=48+number*5+(j%4)*9;foam=[]
            for distance in np.linspace(0,length,65):
                center=start+direction*distance+across*math.sin(distance*.055+j)*1.8
                half=(3.+width*.04)*(1-.75*distance/length);row=[]
                for sign in (-1,1):
                    point=center+across*sign*half;h=sea.hit(point[:2]);r=rock.hit(point[:2])
                    if h is None or (r is not None and r[0][2]>h[0][2]+.1):break
                    point[2]=h[0][2]+.18;row.append(point)
                if len(row)!=2:break
                foam.append(row)
            if len(foam)>2:strip(foam,2)
            # Broken overlapping little spray arcs originate at the contact;
            # their material carries fine moving gaps and scene-depth fade.
            for plume in range(3):
                spray=[];height=15.+width*.13+plume*3.5
                for s in np.linspace(0,1,11):
                    center=start+direction*(8+plume*4)*s;center[2]+=height*math.sin(s*math.pi*.85)
                    half=(3.+plume*1.8)*(1+.8*s)
                    spray.append([center-across*half,center+across*half])
                strip(spray,3,alpha=.58)
        proofs.append(dict(angle_degrees=degrees,width_m=width,crest_arena_m=track[0,8].tolist(),contact_arena_m=foot.tolist(),drop_m=float(track[0,8,2]-foot[2]),contacts_arena_m=[c.tolist() for c in contacts],rock_samples=len(local_ids)-covered,water_samples=covered,supported_core_triangles=patch_triangles))
    if len(proofs)<5:raise RuntimeError(f'Only {len(proofs)} honest full-ring feeds found; inspect actual source geometry')
    np.savez_compressed(out/'support.npz',support_triangle=np.asarray(support_ids),support_barycentric=np.asarray(support_weights))
    np.savez_compressed(out/'core.npz',vertices_m=np.asarray(vertices),triangles=np.asarray(faces),uv=np.asarray(uv),face_material_index=np.asarray(slots),vertex_rgba=np.asarray(colors),source_rock_triangle=np.asarray(face_source))
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    mesh=bpy.data.meshes.new('FullRingContactFalls');mesh.from_pydata(np.asarray(vertices).tolist(),[],faces);mesh.update()
    ob=bpy.data.objects.new('FullRingContactFalls',mesh);bpy.context.collection.objects.link(ob)
    roles=['WetCore','AeratedCore','ReceivingFoam','AeratedVeil']
    for role in roles:mesh.materials.append(bpy.data.materials.new(role))
    for polygon,slot in zip(mesh.polygons,slots):polygon.material_index=int(slot);polygon.use_smooth=True
    corners=np.asarray([x.vertex_index for x in mesh.loops]);layer=mesh.uv_layers.new(name='AcrossAndArcMetres');layer.data.foreach_set('uv',np.asarray(uv)[corners].astype(np.float32).ravel())
    color=mesh.color_attributes.new(name='SprayOpticalDepth',type='FLOAT_COLOR',domain='CORNER');color.data.foreach_set('color',np.asarray(colors)[corners].astype(np.float32).ravel());mesh.color_attributes.active_color=color
    ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    fbx=out/'waterfall_core.foam.fbx'
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,global_scale=1,bake_space_transform=False,mesh_smooth_type='FACE',use_mesh_modifiers=False,add_leaf_bones=False)
    profile=dict(source_mesh_path=str(a.cliff.resolve()),source_mesh_sha256=sha(a.cliff),outlets=proofs,receiving_water_sha256=sha(water_path))
    profile['profile_sha1']=hashlib.sha1(json.dumps(profile,sort_keys=True).encode()).hexdigest()
    report=dict(schema='directorscut.compressed_waterfall_attachment.v1',source_path=str(a.cliff.resolve()),source_sha256=sha(a.cliff),profile_sha1=profile['profile_sha1'],water_path=str(water_path),water_sha256=sha(water_path),frame_origin_arena_m=[0,0,0],basis_u_arena=[1,0,0],basis_v_arena=[0,1,0],basis_z_arena=[0,0,1],width_range_m=[min(x['width_m'] for x in proofs),max(x['width_m'] for x in proofs)],drop_m=float(np.ptp(np.asarray(vertices)[:,2])),native_material_slots=roles,volume_mode='fitted_mesh_spray',outlets=proofs,water_source=provenance,aerated_veil=dict(advection_speed_m_s=9.),limitations='Authored supported runoff and mesh spray; no fluid simulation. Rock uses exact topmost triangles; water uses fresh production HeightAt samples, excluding material WPO.')
    (out/'waterfall_profile.json').write_text(json.dumps(profile,indent=2))
    (out/'attachment.json').write_text(json.dumps(report,indent=2))
    print('COMPLETE_FALLS '+json.dumps(dict(outlets=len(proofs),vertices=len(vertices),triangles=len(faces),source_sha256=report['source_sha256'],bundle=str(out))))

if __name__=='__main__':main()
