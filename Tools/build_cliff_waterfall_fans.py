"""Build a translucent lower aerated fan on the exact fitted hero trajectories.

Blender CPU authoring. Preserves crest cores; replaces old receiving ribbons.
The VDB must then be regenerated with --hero-breakup --aerated-fan.
"""
import argparse,copy,hashlib,json,math,shutil,sys
from pathlib import Path
import bpy,numpy as np
sys.dont_write_bytecode=True;sys.path.insert(0,str(Path(__file__).resolve().parent))
from attach_compressed_cliff_waterfall import Surface,load,sha,current_at
from generate_waterfall_vdb import FittedWaterfall,fan_motion_parameters,smoothstep

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--attachment',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    a=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    if a.output.exists() or 'saved'not in[s.lower()for s in a.output.resolve().parts]:parser.error('Choose new Saved output')
    report=json.loads((a.attachment/'attachment.json').read_text());profile=json.loads((a.attachment/'waterfall_profile.json').read_text())
    if sha(report['source_path'])!=report['source_sha256']:raise ValueError('Actual cliff source changed')
    cliff=load(report['source_path']);water=load(report['water_path']);rock=Surface(cliff['vertices_m'],cliff['triangles']);sea=Surface(water['vertices_m'],water['triangles'])
    core=load(a.attachment/'core.npz');old_n=len(core['vertices_m'])
    vertices=list(core['vertices_m']);faces=list(core['triangles']);uv=list(core['uv']);slots=list(core['face_material_index']);rgba=[(0,0,0,0)]*old_n
    foam_v=[];foam_f=[];foam_uv=[];stats=[]
    outlets=profile.get('ballistic_outlets',[profile]);reference=copy.deepcopy(outlets[0])
    for oi,p in enumerate(outlets):
        width=p['source_half_width_cm']*.02;B=np.array([p['basis_x_arena'],p['basis_y_arena'],p['basis_z_arena']]).T;origin=np.array(p['local_origin_arena_cm'])*.01
        count=49;x=np.linspace(-width*.5,width*.5,count);phase=np.zeros(count)
        model=object.__new__(FittedWaterfall);model.x=x;model.ballistic_offsets=np.array(p['ballistic_launch']['cross_offsets_cm'])*.01;model.ballistic_sources=np.array(p['ballistic_launch']['cross_sources_local_cm'])*.01;model.ballistic_velocity=np.array(p['ballistic_launch']['initial_velocity_local_cm_s'])*.01
        model.fan_release_s,model.fan_kick,model.fan_tau=fan_motion_parameters(x,width,phase,model.ballistic_sources,model.ballistic_velocity)
        def positions(age):return model.fan_kinematics(np.asarray(age))[0]@B.T+origin
        times=np.full(count,np.nan);contact=np.zeros((count,3));previous=0.
        for time in np.arange(0,12,.025):
            pts=positions(np.full(count,time))
            for j in np.flatnonzero(np.isnan(times)):
                point=pts[j];wet=sea.hit(point[:2]);solid=rock.hit(point[:2])
                if wet is not None and point[2]<=wet[0][2]:
                    lo=previous;hi=time
                    for _ in range(22):
                        mid=(lo+hi)*.5;one=positions(np.full(count,mid))[j];h=sea.hit(one[:2])
                        if h is not None and one[2]<=h[0][2]:hi=mid
                        else:lo=mid
                    times[j]=hi;contact[j]=positions(np.full(count,hi))[j]
                elif solid is not None and point[2]<solid[0][2]-.04:
                    raise ValueError(f'Fan {oi} track {j} intersects actual rock before receiving water')
            if np.isfinite(times).all():break
            previous=time
        if not np.isfinite(times).all():raise ValueError('Fan misses finite water')
        rows=160;start=model.fan_release_s-.10;rows_pos=[];rows_age=[]
        for k in range(rows):
            age=start+(times-start)*(k/(rows-1));pts=positions(age)
            if k==rows-1:pts[:,2]+=.035
            rows_pos.append(pts);rows_age.append(age)
        points=np.array(rows_pos);ages=np.array(rows_age);arcs=np.zeros((rows,count));arcs[1:]=np.cumsum(np.linalg.norm(np.diff(points,axis=0),axis=2),axis=0)
        base=len(vertices)
        for k in range(rows):
            local_width=float(np.linalg.norm(points[k,-1]-points[k,0]));elapsed=np.maximum(ages[k]-model.fan_release_s,0)
            progress=smoothstep(0,3.5,elapsed)
            # Extinction per remaining sheet area increases as large fragments
            # aerate into smaller droplets; this is an authored thin proxy.
            droplet_mm=3.-2.2*progress
            tau=.35*(3./droplet_mm)*(width/max(local_width,width))
            tau*=smoothstep(0,.45,elapsed)*(1-smoothstep(.70,1,np.abs(x)/(width*.5)))
            tau*=.72+.28*np.sin(x*.65+ages[k]*1.1)**2
            for j in range(count):
                vertices.append(points[k,j]);uv.append((j/(count-1),float(arcs[k,j])));rgba.append((0,min(local_width/64,1),float(progress[j]),float(min(tau[j],.85))))
        for k in range(rows-1):
            for j in range(count-1):
                idx=base+k*count+j;faces.extend(((idx,idx+count,idx+count+1),(idx,idx+count+1,idx+1)));slots.extend((3,3))
        # Receiving filaments originate at the new drag/contact solution, then
        # follow the measured sloping sea and prescribed local current.
        for number,j in enumerate((5,13,22,34,43)):
            direction=current_at(contact[j]);direction/=np.linalg.norm(direction);across=np.array((-direction[1],direction[0],0))
            strip=[];length=9+3*number
            for k in range(41):
                distance=length*k/40;center=contact[j]+direction*distance+across*(.35*np.sin(distance*.7+number))
                half=(.3+.35*np.sin(distance*.31+number)**2)*(1-.65*k/40)
                pair=[]
                for sign in (-1,1):
                    pt=center+across*sign*half;hit=sea.hit(pt[:2])
                    if hit is None:raise ValueError('New impact foam leaves finite receiving water')
                    pt[2]=hit[0][2]+.055;pair.append(len(foam_v));foam_v.append(pt);foam_uv.append(((sign+1)*.5,distance))
                strip.append(pair)
            for left,right in zip(strip,strip[1:]):foam_f.extend(((left[0],right[0],right[1]),(left[0],right[1],left[1])))
        # Exact source triangle samples on the existing water grid also stop
        # impact spray at rock, rather than allowing rare edge droplets inside.
        w=p['receiving_water'];n=w['count'];grid_origin=np.array(w['origin_local_cm'])*.01;step=w['step_cm']*.01;heights=[];coverage=[]
        for row in range(n):
            for col in range(n):
                local=grid_origin+np.array((col*step,row*step,0));arena=local@B.T+origin;hit=rock.hit(arena[:2]);coverage.append(hit is not None);heights.append(0 if hit is None else float((hit[0]-origin)[2]*100))
        p['occluding_rock']=dict(count=n,step_cm=w['step_cm'],origin_local_cm=w['origin_local_cm'],heights_local_cm_y_major=heights,coverage_y_major=coverage,source_sha256=report['source_sha256'])
        p['aerated_fan']=dict(drag_seconds=3.,release_drop_m=[25,37],native_veil_optical_model='3mm to0.8mm equivalent fragments, inverse-area optical-depth proxy; not measured fluid calibration')
        stats.append(dict(outlet=oi,source_width_m=width,base_width_m=float(np.linalg.norm(contact[-1]-contact[0])),contact_times_seconds=[float(times.min()),float(times.max())],release_drop_m=[25,37],water_contact_max_abs_m=max(abs(pt[2]-sea.hit(pt[:2])[0][2])for pt in contact)))
    # Reference rock query covers the entire crest group in its common frame.
    profile['occluding_rock']=copy.deepcopy(outlets[0]['occluding_rock'])
    profile['aerated_fan']=dict(outlets=stats,lower_dense_shaft_retained=False)
    for p in reversed([profile]+outlets):p['profile_sha1']=hashlib.sha1(json.dumps({k:v for k,v in p.items()if k!='profile_sha1'},sort_keys=True).encode()).hexdigest()
    report['profile_sha1']=profile['profile_sha1'];report['aerated_veil']=dict(vertex_alpha='reference optical depth tau',vertex_green='physical fan width_m/64',vertex_blue='breakup progression',vertex_red='unused0',uv='normalized across; physical arc metres',advection_speed_m_s=29.4,advection_basis='linear-drag terminal vertical speed9.81*3, representative lower-flow speed, not exact at every pixel',outlets=stats)
    report['native_material_slots'].append('AeratedVeil');report['core_triangles']=len(faces);report['foam_triangles']=len(foam_f)
    report['drop_m']=float(np.max(np.array(vertices)[:,2])-np.min(np.array(foam_v)[:,2]))
    a.output.mkdir(parents=True);common={k:core[k]for k in ('frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world')}
    np.savez_compressed(a.output/'core.npz',vertices_m=np.array(vertices),triangles=np.array(faces),uv=np.array(uv),face_material_index=np.array(slots),vertex_rgba=np.array(rgba),**common)
    np.savez_compressed(a.output/'receiving_foam.npz',vertices_m=np.array(foam_v),triangles=np.array(foam_f),uv=np.array(foam_uv),**common)
    shutil.copy2(a.attachment/'support.npz',a.output/'support.npz')
    (a.output/'waterfall_profile.json').write_text(json.dumps(profile,indent=2));(a.output/'attachment.json').write_text(json.dumps(report,indent=2))
    np.savez_compressed(a.output/'fan_contact.npz',last_outlet_contact_arena_m=contact)
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    chart_origin=np.array(report['frame_origin_arena_m']);chart_basis=np.array([report[k]for k in ('basis_u_arena','basis_v_arena','basis_z_arena')]).T
    mats=[bpy.data.materials.new(name)for name in report['native_material_slots']]
    for name,vs,fs,us,ms,cs in [('CrestCoreAndFans',vertices,faces,uv,slots,rgba),('NewContactFoam',foam_v,foam_f,foam_uv,[2]*len(foam_f),[(0,0,0,0)]*len(foam_v))]:
        me=bpy.data.meshes.new(name);me.from_pydata(((np.asarray(vs)-chart_origin)@chart_basis).tolist(),[],np.asarray(fs).tolist());me.update();ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
        for mat in mats:me.materials.append(mat)
        for f,slot in zip(me.polygons,ms):f.material_index=int(slot);f.use_smooth=True
        corners=np.array([loop.vertex_index for loop in me.loops]);layer=me.uv_layers.new(name='AcrossAndArcMetres');layer.data.foreach_set('uv',np.asarray(us)[corners].astype(np.float32).ravel())
        attr=me.color_attributes.new(name='AeratedOpticalDepth',type='FLOAT_COLOR',domain='CORNER');attr.data.foreach_set('color',np.asarray(cs)[corners].astype(np.float32).ravel());me.color_attributes.active_color=attr
    bpy.ops.object.select_all(action='SELECT');bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    bpy.ops.export_scene.fbx(filepath=str((a.output/'waterfall_core.foam.fbx').resolve()),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,global_scale=1,bake_space_transform=False,mesh_smooth_type='FACE',use_mesh_modifiers=False,add_leaf_bones=False)
    print(json.dumps(report['aerated_veil'],indent=2))

if __name__=='__main__':main()
