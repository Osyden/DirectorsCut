"""Fit one isolated waterfall to an original compressed cliff triangle mesh.

Blender background CPU only. The original cliff/water files are immutable.
Output is Saved-only: exact-support profile compatible with generate_waterfall_vdb,
an original core/foam preview mesh, FBX and a placement manifest. This is an
authored cascade, not a water solver or a complete drainage catchment simulation.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
sys.dont_write_bytecode=True

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0,str(Path(__file__).resolve().parent))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    with np.load(path, allow_pickle=False) as data:
        return {k: data[k].copy() for k in data.files}


def smooth(x):
    x=np.clip(x,0,1)
    return x*x*(3-2*x)


class Surface:
    def __init__(self, vertices, triangles):
        self.v=vertices; self.t=triangles
        self.bvh=BVHTree.FromPolygons(vertices.tolist(),triangles.tolist(),all_triangles=True)
        self.top=float(vertices[:,2].max()+1000)

    def hit(self, xy):
        p,n,i,_=self.bvh.ray_cast(Vector((float(xy[0]),float(xy[1]),self.top)),Vector((0,0,-1)))
        if i is None:return None
        p=np.asarray(p,dtype=float); n=np.asarray(n,dtype=float)
        a,b,c=self.v[self.t[i]]
        uv=np.linalg.lstsq(np.stack((b-a,c-a),axis=1),p-a,rcond=None)[0]
        weights=np.array((1-uv.sum(),uv[0],uv[1]))
        # BVH is float precision; reconstruct the exact original triangle.
        p=weights@self.v[self.t[i]]
        return p,n,int(i),weights

    def nearest(self, xyz):
        p,n,i,_=self.bvh.find_nearest(Vector(tuple(float(x) for x in xyz)))
        if i is None:return None
        p=np.asarray(p,dtype=float);n=np.asarray(n,dtype=float)
        aa,bb,cc=self.v[self.t[i]]
        uv=np.linalg.lstsq(np.stack((bb-aa,cc-aa),axis=1),p-aa,rcond=None)[0]
        weights=np.array((1-uv.sum(),uv[0],uv[1]))
        return weights@self.v[self.t[i]],n,int(i),weights


def steepest_path(q,t,contact,source_index):
    edges=np.unique(np.sort(np.concatenate((t[:,[0,1]],t[:,[1,2]],t[:,[2,0]])),axis=1),axis=0)
    dest=np.full(len(q),-1,dtype=int); best=np.zeros(len(q));acc=np.ones(len(q))
    for a,b in np.concatenate((edges,edges[:,::-1])):
        slope=(q[a,2]-q[b,2])/max(np.linalg.norm(q[a,:2]-q[b,:2]),1e-6)
        if slope>best[a]:best[a]=slope;dest[a]=b
    for i in np.argsort(q[:,2])[::-1]:
        if dest[i]>=0:acc[dest[i]]+=acc[i]
    end=np.full(len(q),-1,dtype=int)
    for i in np.argsort(q[:,2]):
        if q[i,1]<=np.interp(q[i,0],contact[:,0],contact[:,1])+2.01:end[i]=i
        elif dest[i]>=0:end[i]=end[dest[i]]
    if source_index is None:
        drop=q[:,2]-q[np.maximum(end,0),2]
        valid=(end>=0)&(drop>55)&(drop<100)&(q[:,0]>contact[0,0]+30)&(q[:,0]<contact[-1,0]-30)
        candidates=np.flatnonzero(valid)
        if not len(candidates):raise ValueError('No descending mesh-edge ravine reaches the shore')
        source_index=int(candidates[np.argmax(acc[candidates])])
    ids=[];i=source_index
    while i>=0 and i not in ids:
        ids.append(i)
        if q[i,1]<np.interp(q[i,0],contact[:,0],contact[:,1])-2:break
        i=dest[i]
    if len(ids)<3:raise ValueError('Source has no useful descending path')
    return np.asarray(ids),float(acc[source_index])


def current_at(arena):
    # Same prescribed 18 m/s current direction as DCMaelstromWaterfalls.cpp.
    angle=math.atan2(arena[1],arena[0]);seconds=((math.pi-angle)%(2*math.pi))/.018-8
    seconds=float(np.clip(seconds,-7,179))
    def point(s):
        a=math.pi-.018*max(0,s+8)
        r=.985-.035*smooth((s+8)/16)-.700*smooth((s-8)/157)-.130*smooth((s-165)/15)
        return np.array((math.cos(a),math.sin(a),0))*r
    d=point(seconds+.05)-point(seconds-.05)
    return d/np.linalg.norm(d)*18


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cliff',type=Path,required=True);p.add_argument('--water',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--source-index',type=int)
    p.add_argument('--template-source-index',type=int,default=939,help='Ring pilot: use template tag only to choose a seed; all fitting uses actual ring triangles')
    p.add_argument('--hero-source-index',type=int,help='Explicit actual ring vertex for a supported high-rim ballistic source')
    p.add_argument('--hero-speed',type=float,default=12.5,help='Physical horizontal feeder speed in metres/second')
    p.add_argument('--width-m',type=float,default=6);p.add_argument('--render',action='store_true')
    p.add_argument('--exposed-jet',action='store_true',help='Try one longer supported ravine jet; authored launch velocity, no invented ledge')
    a=p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    if a.output.exists() or 'saved' not in [s.lower() for s in a.output.resolve().parts]:p.error('Choose a new ignored Saved output')
    if not 1<=a.width_m<=12:p.error('Prototype width must be between 1 and 12 physical metres')
    cliff=load(a.cliff);water=load(a.water)
    meta=json.loads(str(cliff['metadata_json']));ring='compressed_chart_uvz_m' in cliff
    # Arena XYZ is authoritative. A ring's identity export frame and a sector's
    # analysis frame may differ; apply only this documented rigid transform.
    origin=water['frame_origin_world_m'];B=np.stack([water['basis_u_world'],water['basis_v_world'],water['basis_z_world']],axis=1)
    if not np.allclose(B.T@B,np.eye(3),atol=1e-10) or not np.isclose(np.linalg.det(B),1):raise ValueError('Invalid physical analysis frame')
    if not ring:
        for k in ('frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world'):
            if not np.array_equal(cliff[k],water[k]):raise ValueError('Measured water and cliff frames differ')
    q=(cliff['vertices_m']-origin)@B;t=cliff['triangles'];wq=(water['vertices_m']-origin)@B
    rock=Surface(q,t);sea=Surface(wq,water['triangles'])
    if ring:
        template=load(meta['compressed_path']);template_q=(template['vertices_m']-origin)@B
        target=template_q[a.template_source_index]
        tag=cliff['compressed_chart_uvz_m'];valid=np.isfinite(tag).all(axis=1)&(cliff['pilot_weight']>.95)
        d=np.sum((tag-target)**2,axis=1);d[~valid]=np.inf
        a.source_index=int(np.argmin(d)) if a.source_index is None else a.source_index
        # Reconstruct the conservative measured-water edge in this analysis
        # frame. This uses actual water triangles, not template shoreline tags.
        contact=[];contact_height=[]
        for u in template['contact_uv_m'][:,0]:
            found=[v for v in np.arange(-50,201,2) if sea.hit((u,v)) is not None]
            if not found:continue
            low=max(found);high=low+2
            for _ in range(18):
                mid=(low+high)*.5
                if sea.hit((u,mid)) is not None:low=mid
                else:high=mid
            h=sea.hit((u,low));contact.append((u,low));contact_height.append(h[0][2])
        contact=np.asarray(contact);contact_height=np.asarray(contact_height)
        active=(q[:,0]>contact[0,0]-5)&(q[:,0]<contact[-1,0]+5)&(q[:,1]>-30)&(q[:,1]<400)
        graph_triangles=t[np.all(active[t],axis=1)]
    else:
        contact=cliff['contact_uv_m'];contact_height=cliff['contact_water_height_m'];graph_triangles=t
    ids,upstream=steepest_path(q,graph_triangles,contact,a.source_index)
    raw=q[ids];distance=np.r_[0,np.cumsum(np.linalg.norm(np.diff(raw,axis=0),axis=1))]
    stations=np.arange(0,distance[-1],.4)
    path=np.stack([np.interp(stations,distance,raw[:,j]) for j in range(3)],axis=1)
    # Try a bounded terminal jet. Its launch speed is authored explicitly; do
    # not invent a lip, lift a submerged end, or clip a jet through the cliff.
    jet=None
    launch_candidates=range(45,len(path)-2) if a.exposed_jet else range(len(path)-2,4,-1)
    for i in launch_candidates:
        expected=np.interp(path[i,0],contact[:,0],contact_height)
        lo,hi=(18,45) if a.exposed_jet else (4,12)
        if not lo<path[i,2]-expected<hi:continue
        for horizontal in ((6.,10.,14.,18.,24.) if a.exposed_jet else (6.,10.,14.)):
            start=path[i].copy();start[2]+=.18
            vel=np.array((0.,-horizontal,-4.))
            samples=[];valid=True;hit=None
            for time in np.arange(0,5,.015):
                pt=start+vel*time+np.array((0,0,-4.905*time*time))
                rockhit=rock.hit(pt[:2]);waterhit=sea.hit(pt[:2])
                if waterhit is not None and pt[2]<=waterhit[0][2]:hit=(time,pt,waterhit);break
                if rockhit is not None and pt[2]<rockhit[0][2]+.035:valid=False;break
                samples.append(pt)
            if valid and hit is not None:
                low=hit[0]-.015;high=hit[0]
                for _ in range(25):
                    mid=(low+high)/2;pt=start+vel*mid+np.array((0,0,-4.905*mid*mid));wh=sea.hit(pt[:2])
                    if wh is not None and pt[2]<=wh[0][2]:high=mid
                    else:low=mid
                tm=high;end=start+vel*tm+np.array((0,0,-4.905*tm*tm))
                jet=dict(launch_index=i,initial_velocity_chart_m_s=vel.tolist(),duration_s=tm,
                         gravity_m_s2=9.81,drop_m=float(start[2]-end[2]))
                path=np.vstack((path[:i],np.asarray(samples),end));break
        if jet is not None:break
    if a.hero_source_index is not None:
        if not ring or not 10<=a.hero_speed<=20:raise ValueError('Hero source requires actual ring and10–20m/s horizontal momentum')
        source=q[a.hero_source_index];world=cliff['vertices_m'][a.hero_source_index]
        direction=np.array((-world[0],-world[1],0));direction/=np.linalg.norm(direction);direction=direction@B
        vel=direction*a.hero_speed;vel[2]=-1.
        feeder=[]
        for dist in np.linspace(-4,-.25,16):
            h=rock.hit(source[:2]+direction[:2]*dist)
            if h is None:raise ValueError('Hero feeder has no actual rock support')
            feeder.append(h[0])
        start=source.copy();start[2]+=.30;samples=[];hit=None;minimum_clearance=1e9
        for time in np.arange(0,9,.015):
            pt=start+vel*time+np.array((0,0,-4.905*time*time));rh=rock.hit(pt[:2]);wh=sea.hit(pt[:2])
            if wh is not None and pt[2]<=wh[0][2]:hit=(time,pt,wh);break
            if rh is not None:
                clearance=pt[2]-rh[0][2];minimum_clearance=min(minimum_clearance,clearance)
                if clearance<.08:raise ValueError('High-rim ballistic centre intersects actual ring')
            samples.append(pt)
        if hit is None:raise ValueError('Hero misses finite receiving water')
        low=hit[0]-.015;high=hit[0]
        for _ in range(26):
            tm=(low+high)*.5;pt=start+vel*tm+np.array((0,0,-4.905*tm*tm));wh=sea.hit(pt[:2])
            if wh is not None and pt[2]<=wh[0][2]:high=tm
            else:low=tm
        end=start+vel*high+np.array((0,0,-4.905*high*high))
        if not 100<start[2]-end[2]<180:raise ValueError('Hero physical drop is outside100–180m')
        path=np.vstack((feeder,samples,end));ids=np.array([a.hero_source_index]);upstream=0.
        jet=dict(launch_index=len(feeder),initial_velocity_chart_m_s=vel.tolist(),duration_s=high,gravity_m_s2=9.81,
                 drop_m=float(start[2]-end[2]),minimum_centre_clearance_m=minimum_clearance,
                 horizontal_projection_m=float(np.linalg.norm(end[:2]-start[:2])),high_rim_source=True,
                 feeder_depth_m=.4,feeder_momentum_authored=True,source_support_vertex=a.hero_source_index)
    if jet is None:raise ValueError('No short terminal ballistic jet clears rock and reaches measured finite water')
    launch_index=jet['launch_index'];s=np.r_[0,np.cumsum(np.linalg.norm(np.diff(path,axis=0),axis=1))]
    support_ids=[];support_weights=[];sections=[];widths=[];positions=[];faces=[];materials=[];uvs=[]
    normalized_offsets=np.linspace(-1,1,13)
    launch=path[launch_index]; tangent=path[launch_index]-path[max(0,launch_index-6)]
    launch_hit=rock.hit(launch[:2]);jet_side=np.cross(launch_hit[1],tangent);jet_side/=np.linalg.norm(jet_side)
    jet_starts=[];jet_times=[];jet_vel=np.asarray(jet['initial_velocity_chart_m_s'])
    jet_width=a.width_m if a.hero_source_index is not None else a.width_m*.65
    for f in normalized_offsets:
        h=rock.nearest(launch+jet_side*f*jet_width*.5)
        if h is None:raise ValueError('Jet launch leaves supported cliff')
        start=h[0].copy();start[2]+=.30 if a.hero_source_index is not None else .18
        end_time=None
        for time in np.arange(.005,9,.005):
            pt=start+jet_vel*time+np.array((0,0,-4.905*time*time));rh=rock.hit(pt[:2]);wh=sea.hit(pt[:2])
            if wh is not None and pt[2]<=wh[0][2]:
                low=time-.005;high=time
                for _ in range(22):
                    tm=(low+high)*.5;pp=start+jet_vel*tm+np.array((0,0,-4.905*tm*tm));ww=sea.hit(pp[:2])
                    if ww is not None and pp[2]<=ww[0][2]:high=tm
                    else:low=tm
                end_time=high;break
            if rh is not None and pt[2]<rh[0][2]+.01:raise ValueError('A separately supported jet strand intersects rock')
        if end_time is None:raise ValueError('A jet strand misses finite receiving water')
        jet_starts.append(start);jet_times.append(end_time)
    jet['cross_track_duration_s']=jet_times;jet['launch_width_m']=jet_width
    for i,pt in enumerate(path):
        tangent=path[min(i+6,launch_index)]-path[max(min(i,launch_index)-6,0)]
        center_hit=rock.hit(pt[:2]) if i<launch_index else launch_hit
        side=np.cross(center_hit[1],tangent) if i<launch_index else jet_side
        side/=max(np.linalg.norm(side),1e-6)
        width=a.width_m*(.78+.13*math.sin(s[i]*.11)+.09*math.sin(s[i]*.29+1))
        blend=smooth((s[i]-s[launch_index]+5)/5)
        width=width*(1-blend)+jet_width*blend
        widths.append(width)
        row=[];ri=[];rw=[]
        for k,f in enumerate(normalized_offsets):
            h=rock.nearest(pt+side*f*width*.5) if i<launch_index else None
            if i<launch_index:
                if h is None:raise ValueError('Cross-section leaves the actual cliff')
                v=h[0].copy();v[2]+=.16+.035*math.sin(s[i]*2.7+k*.8)
                ri.append(h[2]);rw.append(h[3])
            else:
                progress=(i-launch_index)/max(len(path)-1-launch_index,1)
                jt=jet_times[k]*progress
                v=jet_starts[k]+jet_vel*jt+np.array((0,0,-4.905*jt*jt));ri.append(-1);rw.append(np.zeros(3))
            row.append(v);positions.append(v);uvs.append((f*.5+.5,s[i]))
        sections.append(row);support_ids.append(ri);support_weights.append(rw)
    sections=np.asarray(sections);positions=list(positions)
    for i in range(len(path)-1):
        for j in range(12):
            x=i*13+j;faces.extend(((x,x+13,x+14),(x,x+14,x+1)))
            # A continuous wet core with three broken aerated strands. Geometry
            # remains physical; shader animation is a separate native task.
            strand=j in (2,3,6,7,10)
            if a.hero_source_index is not None and i>=launch_index:
                drop_from_lip=path[launch_index,2]-path[i,2]
                # Native review rejected the regularly spaced long-tail pieces
                # as road dashes. Keep only the first25m of sheet/ligaments;
                # the unchanged physical volume carries the remaining fall.
                keep=drop_from_lip<4. or (j in (2,3,6,9,10) and drop_from_lip<25.)
                if not keep:
                    del faces[-2:]
                    continue
            materials.extend((int(strand),int(strand)))
    # Follow original triangles through sharp creases instead of bridging a
    # ridge with a coarse quad. Only offending attached faces are subdivided.
    refined=[];refined_slots=[]
    def refine(face,slot,depth=0):
        vs=np.asarray([positions[j] for j in face]);probes=np.vstack((vs.mean(axis=0),(vs+np.roll(vs,-1,axis=0))*.5))
        clear=[]
        for pp in probes:
            hh=rock.hit(pp[:2]);clear.append(999. if hh is None else pp[2]-hh[0][2])
        if min(clear)>=.025:
            refined.append(face);refined_slots.append(slot);return
        if depth>=7:raise ValueError('Attached core still cuts rock after bounded subdivision')
        mids=[]
        for j,k in ((0,1),(1,2),(2,0)):
            pp=(vs[j]+vs[k])*.5;hh=rock.hit(pp[:2])
            if hh is not None:pp[2]=hh[0][2]+.16
            mids.append(len(positions));positions.append(pp);uvs.append(tuple((np.asarray(uvs[face[j]])+uvs[face[k]])*.5))
        aa,bb,cc=face;ab,bc,ca=mids
        for f in ((aa,ab,ca),(ab,bb,bc),(ca,bc,cc),(ab,bc,ca)):refine(f,slot,depth+1)
    for face,slot in zip(faces,materials):
        if max(face)<launch_index*13:refine(face,slot)
        else:refined.append(face);refined_slots.append(slot)
    faces,materials=refined,refined_slots
    foot=path[-1].copy();waterhit=sea.hit(foot[:2])
    if waterhit is None:raise ValueError('Jet ends outside receiving water')
    endpoint_error=float(foot[2]-waterhit[0][2]);foot[2]=waterhit[0][2]
    arena_foot=origin+foot@B.T;flow_arena=current_at(arena_foot);flow_chart=flow_arena@B
    # Five narrow diverging foam filaments on the actual water surface, never a
    # flat fan hovering above the sloping receiving sea.
    foam_vertices=[];foam_faces=[];foam_uv=[]
    along=flow_chart.copy();along[2]=0;along/=np.linalg.norm(along)
    across=np.array((-along[1],along[0],0))
    for strand in range(5):
        start=len(foam_vertices);previous=False
        for j,dist in enumerate(np.linspace(0,42,85)):
            center=foot+along*dist+across*((strand-2)*.5+(strand-2)*.06*dist+.35*math.sin(dist*.4+strand))
            width=(.32+.09*(strand%3))*(1-.6*dist/42)
            pair=[]
            for sign in (-1,1):
                xy=center[:2]+across[:2]*sign*width;h=sea.hit(xy)
                if h is None:break
                pos=h[0].copy();pos[2]+=.045;pair.append(pos)
            if len(pair)!=2:previous=False;continue
            base=len(foam_vertices);foam_vertices.extend(pair);foam_uv.extend(((0,dist),(1,dist)))
            if previous:foam_faces.extend(((base-2,base,base+1),(base-2,base+1,base-1)))
            previous=True
    # Local VDB axes: -chart U, -chart V, up. Proper right handed yaw frame.
    FB=np.stack((-B[:,0],-B[:,1],B[:,2]),axis=1)
    FO=arena_foot-FB[:,1]*10
    def local(points):return ((origin+np.asarray(points)@B.T-FO)@FB)*100
    count=129;step=1.;grid_origin=np.array((-64.,10.-64.,0.));heights=[];coverage=[]
    for y in range(count):
        for x in range(count):
            arena=FO+FB@np.array((grid_origin[0]+x*step,grid_origin[1]+y*step,0.));chart=(arena-origin)@B
            h=sea.hit(chart[:2]);coverage.append(h is not None)
            heights.append(0. if h is None else (origin+h[0]@B.T-FO)[2]*100)
    # Outside heights must not create a ramp to zero beside covered cells.
    # Extend the nearest valid sample for interpolation, retaining explicit
    # coverage=false. This extrapolates height ONLY, never water presence.
    hg=np.asarray(heights).reshape(count,count);cg=np.asarray(coverage).reshape(count,count);filled=cg.copy()
    while not filled.all():
        old=filled.copy()
        for dr,dc in ((1,0),(-1,0),(0,1),(0,-1)):
            shifted=np.roll(old,(dr,dc),(0,1));val=np.roll(hg,(dr,dc),(0,1))
            if dr==1:shifted[0]=False
            if dr==-1:shifted[-1]=False
            if dc==1:shifted[:,0]=False
            if dc==-1:shifted[:,-1]=False
            take=~filled&shifted;hg[take]=val[take];filled[take]=True
        if np.array_equal(old,filled):raise ValueError('No receiving surface inside impact grid')
    profile=dict(schema='directorscut.waterfall_profile.v1',units='centimeters',feed_index=800,
        source_mesh_path=str(a.cliff.resolve()),source_mesh_sha256=sha(a.cliff),water_mesh_sha256=sha(a.water),
        trajectory_model='original_triangle_ravine_with_terminal_ballistic_jet',
        local_origin_arena_cm=(FO*100).tolist(),local_yaw_degrees=math.degrees(math.atan2(FB[1,0],FB[0,0])),
        basis_x_arena=FB[:,0].tolist(),basis_y_arena=FB[:,1].tolist(),basis_z_arena=FB[:,2].tolist(),
        source_half_width_cm=a.width_m*50,source_local_cm=local(path[:1])[0].tolist(),
        foot_local_cm=local(foot[None])[0].tolist(),samples_local_cm=local(path).tolist(),
        path_length_cm=float(s[-1]*100),cross_offsets_cm=(normalized_offsets*a.width_m*50).tolist(),
        cross_sections_local_cm=local(sections.reshape(-1,3)).reshape(sections.shape).tolist(),
        current_local_cm_s=(flow_arena@FB*100).tolist(),
        receiving_water=dict(count=count,step_cm=step*100,origin_local_cm=(grid_origin*100).tolist(),
            heights_local_cm_y_major=hg.ravel().tolist(),coverage_y_major=cg.ravel().tolist(),mean_surface=True),
        terminal_ballistic_jet=jet,
        limitations='Static finite measured water; no animated-wave collision. Author-selected feed; no upstream source water yet. Sampled VDB water coverage at1m. Preview source800 is not runtime feed8.')
    profile['profile_sha1']=hashlib.sha1(json.dumps(profile,sort_keys=True).encode()).hexdigest()
    if a.hero_source_index is not None:
        profile['ballistic_launch']=dict(initial_velocity_local_cm_s=(jet_vel@B.T@FB*100).tolist(),
            cross_sources_local_cm=local(jet_starts).tolist(),cross_offsets_cm=(normalized_offsets*a.width_m*50).tolist(),
            gravity_cm_s2=981.,source_thickness_cm=40.,source_launch_local_cm=local(np.asarray(jet_starts)[6:7])[0].tolist(),
            model='physical ballistic core; secondary droplets acquire drag after release',sampled_centre_flight_seconds=jet['duration_s'])
        profile['source_local_cm']=profile['ballistic_launch']['source_launch_local_cm']
        profile['profile_sha1']=hashlib.sha1(json.dumps({k:v for k,v in profile.items() if k!='profile_sha1'},sort_keys=True).encode()).hexdigest()
    a.output.mkdir(parents=True)
    (a.output/'waterfall_profile.json').write_text(json.dumps(profile,indent=2),encoding='utf-8')
    np.savez_compressed(a.output/'support.npz',path_chart_m=path,cross_sections_chart_m=sections,
        support_triangle=np.asarray(support_ids),support_barycentric=np.asarray(support_weights),
        source_mesh_edge_path=ids,width_m=np.asarray(widths),jet_start_index=launch_index,
        impact_triangle_index=waterhit[2],impact_barycentric=waterhit[3])
    common={k:water[k] for k in ('frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world')}
    np.savez_compressed(a.output/'core.npz',vertices_m=origin+np.asarray(positions)@B.T,triangles=np.asarray(faces,dtype=np.int32),
        uv=np.asarray(uvs),face_material_index=np.asarray(materials,dtype=np.int8),**common)
    np.savez_compressed(a.output/'receiving_foam.npz',vertices_m=origin+np.asarray(foam_vertices)@B.T,
        triangles=np.asarray(foam_faces,dtype=np.int32),uv=np.asarray(foam_uv),**common)
    # FBX uses the SAME chart/metre frame as the cliff isolated preview, no
    # recentering and no scale bake. Native import must apply its verified axis
    # transform, not assume arena XYZ from a bounding box.
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    def material(name,color,rough):
        m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
        bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough
        return m
    mats=[material('WetCore',(0.095,.20,.22),.22),material('AeratedCore',(.68,.76,.77),.55),material('ReceivingFoam',(.75,.80,.80),.75)]
    def obj(name,verts,tris,uv,slots):
        me=bpy.data.meshes.new(name);me.from_pydata(np.asarray(verts).tolist(),[],np.asarray(tris).tolist());me.update();ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
        for m in mats:me.materials.append(m)
        for f,slot in zip(me.polygons,slots):f.material_index=int(slot);f.use_smooth=True
        layer=me.uv_layers.new(name='AcrossAndArcMetres')
        for loop in me.loops:layer.data[loop.index].uv=uv[loop.vertex_index]
        return ob
    core=obj('CompressedRavineCore',positions,faces,np.asarray(uvs),materials)
    foam=obj('CompressedRavineFoam',foam_vertices,foam_faces,np.asarray(foam_uv),[2]*len(foam_faces))
    bpy.ops.object.select_all(action='DESELECT');core.select_set(True);foam.select_set(True)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    bpy.ops.export_scene.fbx(filepath=str((a.output/'waterfall_core.foam.fbx').resolve()),use_selection=True,object_types={'MESH'},
        axis_forward='-Y',axis_up='Z',apply_unit_scale=True,global_scale=1,bake_space_transform=False,mesh_smooth_type='FACE',use_mesh_modifiers=False,add_leaf_bones=False)
    report=dict(schema='directorscut.compressed_waterfall_attachment.v1',source_path=str(a.cliff.resolve()),source_sha256=sha(a.cliff),
        water_path=str(a.water.resolve()),water_sha256=sha(a.water),profile_sha1=profile['profile_sha1'],
        source_vertex_index=int(ids[0]),template_seed_only=ring,upstream_vertices_proxy=upstream,source_chart_m=path[0].tolist(),foot_chart_m=foot.tolist(),
        source_arena_m=(origin+path[0]@B.T).tolist(),foot_arena_m=arena_foot.tolist(),
        frame_origin_arena_m=origin.tolist(),basis_u_arena=B[:,0].tolist(),basis_v_arena=B[:,1].tolist(),basis_z_arena=B[:,2].tolist(),
        drop_m=float(path[0,2]-foot[2]),length_m=float(s[-1]),width_range_m=[min(widths),max(widths)],
        terminal_jet=jet,mean_water_endpoint_residual_m=endpoint_error,core_triangles=len(faces),foam_triangles=len(foam_faces),
        opaque_core_max_drop_m=25. if a.hero_source_index is not None else None,
        native_material_slots=['WetCore','AeratedCore','ReceivingFoam'],fbx_chart_units='metres',fbx_axes=['-Y','Z'],
        runtime_integrated=False,photorealistic_accepted=False,
        limitations=profile['limitations'])
    (a.output/'attachment.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2),flush=True)
    if a.render:
        from review_eroded_cliff_sector import reset_scene,camera_at,label_camera
        # Reconstruct after reset; this is actual core geometry, not a concept.
        scene=reset_scene(1440,960,24)
        core=obj('CompressedRavineCore',positions,faces,np.asarray(uvs),materials)
        foam=obj('CompressedRavineFoam',foam_vertices,foam_faces,np.asarray(foam_uv),[2]*len(foam_faces))
        for name,verts,tris,col in [('ActualSigma6',q,t,(.33,.33,.33)),('MeasuredWater',wq,water['triangles'],(.075,.12,.14))]:
            me=bpy.data.meshes.new(name);me.from_pydata(verts.tolist(),[],tris.tolist());me.update();ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);me.materials.append(material(name,col,.8))
            for f in me.polygons:f.use_smooth=True
        sun=bpy.data.objects.new('Fixed review light',bpy.data.lights.new('Fixed review light','SUN'))
        scene.collection.objects.link(sun);sun.data.energy=2.;sun.data.angle=.08
        sun.rotation_euler=Vector((.7,.45,-.6)).to_track_quat('-Z','Y').to_euler()
        center=(path[0]+foot)*.5
        for name,loc,target,scale in [('close',center+np.array((-110,-160,35)),center,150),('opposite',center+np.array((95,-160,50)),center,170),('sector',np.array((700,-642,370.754114151)),np.array((250,258,-54.245885849)),900)]:
            title='ACTUAL RING PILOT + AUTHORED CORE' if ring else 'ACTUAL SIGMA6 RAVINE + AUTHORED CORE'
            cam=camera_at(scene,loc,target,scale);label_camera(scene,cam,title,f'{report["drop_m"]:.1f} m drop | {report["width_range_m"][0]:.1f}–{report["width_range_m"][1]:.1f} m width | measured water','Geometry placement proof | spray VDB separate | no runtime/photorealistic claim')
            scene.render.filepath=str((a.output/(name+'.png')).resolve());bpy.ops.render.render(write_still=True)
            for child in list(cam.children):bpy.data.objects.remove(child,do_unlink=True)
            bpy.data.objects.remove(cam,do_unlink=True)


if __name__=='__main__':main()
