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
                vertices.append(points[i,j]);uv.append((j/(cols-1),i/(rows-1) if role==3 else arc[i]));colors.append((0,min(width/64,1),min(arc[-1]/128,1) if role==3 else 1,alpha))
        for i in range(rows-1):
            for j in range(cols-1):
                q=base+i*cols+j
                for face in ((q,q+cols,q+1),(q+1,q+cols,q+cols+1)):
                    if role==1:
                        middle=np.asarray([vertices[k] for k in face]).mean(axis=0);hits=[h for h in (rock.hit(middle[:2]),sea.hit(middle[:2])) if h is not None]
                        if hits and middle[2]<max(h[0][2] for h in hits)+.01:continue
                    faces.append(face);slots.append(role);face_source.append(-1)
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
            faces.append((base,base+1,base+2));slots.append(0);face_source.append(int(triangle))
        return len(chosen)
    def flight(source,radial,speed):
        start=source.copy();start[2]+=.30;velocity=-radial*speed+np.array((0.,0.,-1.))
        def at(t):return start+velocity*t+np.array((0.,0.,-4.905*t*t))
        clearance=0.;last=0.
        for t in np.arange(.04,9.,.04):
            point=at(t);hits=[h for h in (rock.hit(point[:2]),sea.hit(point[:2])) if h is not None]
            if not hits:last=t;continue
            height=max(h[0][2] for h in hits);gap=point[2]-height;clearance=max(clearance,gap)
            if gap<=.08:
                lo=last;hi=t
                for _ in range(12):
                    mid=(lo+hi)/2;point=at(mid);hits=[h for h in (rock.hit(point[:2]),sea.hit(point[:2])) if h is not None]
                    if hits and point[2]<=max(h[0][2] for h in hits)+.08:hi=mid
                    else:lo=mid
                if start[2]-at(hi)[2]<12 or clearance<2:return None
                return start,velocity,hi,clearance
            last=t
        return None
    def detached_curtains(radial,cross,width,seq):
        accepted=[];last_radius=1.e9
        for k in range(3,len(seq)-4,9):
            radius=seq[k][0]
            if last_radius-radius<24:continue
            center=rock.hit((radial*radius)[:2])
            if center is None or flight(center[0],radial,22.) is None:continue
            group=[]
            for offset in np.linspace(-.32,.32,13)*width:
                hit=rock.hit((radial*radius+cross*offset)[:2]);f=None if hit is None else flight(hit[0],radial,22.)
                if f is None:
                    if len(group)>=3:break
                    group=[];continue
                group.append(f)
            if len(group)<3:continue
            points=[]
            for fraction in np.linspace(0,1,49):
                row=[]
                for start,velocity,duration,clearance in group:
                    t=duration*fraction;row.append(start+velocity*t+np.array((0,0,-4.905*t*t)))
                points.append(row)
            before=len(faces);strip(points,1);count=len(faces)-before
            if count:
                accepted.append(dict(launch_radius_m=radius,columns=len(group),triangles=count,speed_m_s=22.,duration_s=[min(g[2] for g in group),max(g[2] for g in group)],maximum_detachment_m=max(g[3] for g in group)))
                last_radius=radius
            if len(accepted)>=8:break
        return accepted
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
        curtains=detached_curtains(radial,cross,width,seq)
        support_ids.extend(local_ids);support_weights.extend(local_weights)
        # Each filament starts at its own measured receiving hit and follows
        # the production current direction with repeated water queries.
        contacts=[]
        for j in (2,5,8,11,14):
            start=track[-1,j].copy();receiving=None
            # The widened outlet crosses an uneven shoreline. Each filament
            # gets its own first exposed water contact, not the centre track's
            # radius projected under neighbouring rock.
            for inset in np.arange(0,120,.5):
                point=start-radial*inset;hit=sea.hit(point[:2]);solid=rock.hit(point[:2])
                if hit is not None and (solid is None or hit[0][2]>=solid[0][2]-.05):
                    receiving=hit[0].copy();break
            if receiving is None:continue
            start=receiving;contacts.append(start.copy())
            direction=current_at(start);direction/=np.linalg.norm(direction);across=np.array((-direction[1],direction[0],0))
            length=48+number*5+(j%4)*9;foam=[]
            for distance in np.linspace(0,length,129):
                # Carry the impact's inward momentum into the receiving
                # current, then broaden after leaving the irregular shore.
                center=start+direction*distance-radial*(1.+.35*min(distance,35.))+across*math.sin(distance*.055+j)*1.8*min(distance/15,1)
                half=(.4+(2.6+width*.04)*min(distance/12,1))*(1-.75*distance/length);row=[]
                for sign in np.linspace(-1,1,5):
                    point=center+across*sign*half;h=sea.hit(point[:2]);r=rock.hit(point[:2])
                    if h is None or (r is not None and r[0][2]>h[0][2]+.1):break
                    point[2]=h[0][2]+.18;row.append(point)
                if len(row)!=5:break
                foam.append(row)
            if len(foam)>2:strip(foam,2)
            # Crossed, bowed cards retain apparent mist depth from the basin
            # and ship. The old radial cards were edge-on from inside the bowl.
            for plume in range(3):
                spray=[];height=22.+width*.20+plume*5;angle=plume*math.pi/3
                axis=cross*math.cos(angle)+radial*math.sin(angle)
                for s in np.linspace(0,1,13):
                    center=start-radial*(3+15*s)+direction*(plume-1)*4;center[2]+=height*s
                    half=(9.+width*.06)*(1+.65*s)
                    spray.append([center+axis*x*half for x in np.linspace(-1,1,9)])
                strip(spray,3,alpha=.72)
        proofs.append(dict(angle_degrees=degrees,width_m=width,crest_arena_m=track[0,8].tolist(),contact_arena_m=foot.tolist(),drop_m=float(track[0,8,2]-foot[2]),contacts_arena_m=[c.tolist() for c in contacts],rock_samples=len(local_ids)-covered,water_samples=covered,supported_core_triangles=patch_triangles,detached_curtains=curtains))
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
