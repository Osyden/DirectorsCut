"""An authored connected 500m headland built from interlocking rock volumes.

CPU Blender. Actual unequal finite joint planes define each volume. A common
bedrock mass connects all exposures; no cell pockets, copied meshes, height
noise, or repeated full-height columns are used. Final triangles are the
exact Boolean exterior, with the original ring shoreline and perimeter.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np

sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
from generate_rock_buttress import splice_full_ring
from generate_fractured_cliff_outcrop import mesh_object, wedge, arrays, mesh_metrics


def sample_ring(ring,row,col):
    """Sample the original canonical BC-diagonal triangles in arena metres."""
    r=np.clip(np.floor(row).astype(int),0,159);c=np.clip(np.floor(col).astype(int),0,1535)
    v=np.clip(row-r,0,1);u=np.clip(col-c,0,1)
    a,b,cc,d=ring[r,c],ring[r,c+1],ring[r+1,c],ring[r+1,c+1]
    return np.where((u+v<=1)[:,None],a+(b-a)*u[:,None]+(cc-a)*v[:,None],
                    d+(cc-d)*(1-u[:,None])+(b-d)*(1-v[:,None]))


def internal_shells(vertices,triangles):
    """Omit only small closed components demonstrably inside the outer solid.

    Every vertex must see an outward-facing hit in all six directions against
    the original largest closed component. This does not discard loose rocks
    by size, or cull legitimate visible faces by area.
    """
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    parent=np.arange(len(vertices))
    def root(i):
        while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
        return i
    for a,b,c in triangles:
        q=root(a);parent[root(b)]=q;parent[root(c)]=q
    labels=np.asarray([root(i) for i in range(len(vertices))]);face_labels=labels[triangles[:,0]]
    ids,counts=np.unique(face_labels,return_counts=True);main=ids[np.argmax(counts)]
    tree=BVHTree.FromPolygons([Vector(tuple(p)) for p in vertices],triangles[face_labels==main].tolist(),all_triangles=True)
    omit=np.zeros(len(triangles),bool);records=[]
    for component,nfaces in zip(ids,counts):
        if component==main:continue
        fi=np.flatnonzero(face_labels==component);f=triangles[fi];vi=np.unique(f);p=vertices[vi]
        edges=np.sort(np.concatenate((f[:,[0,1]],f[:,[1,2]],f[:,[2,0]])),axis=1)
        _,ec=np.unique(edges,axis=0,return_counts=True)
        a,b,c=[vertices[f[:,i]]-p.mean(0) for i in range(3)]
        volume=float(np.sum(np.einsum('ij,ij->i',a,np.cross(b,c)))/6)
        if nfaces>100 or abs(volume)>1 or np.any(ec!=2):raise ValueError('Unapproved disconnected visible rock component')
        minimum_dot=1.;minimum_distance=1.e9
        for point in p:
            for direction in np.vstack((np.eye(3),-np.eye(3))):
                hit,normal,index,distance=tree.ray_cast(Vector(tuple(point)),Vector(tuple(direction)),1.e6)
                dot=-1 if hit is None else float(np.dot(normal,direction))
                if hit is None or dot<=1.e-6 or distance<=1.e-5:raise ValueError('Disconnected component is not proven buried')
                minimum_dot=min(minimum_dot,dot);minimum_distance=min(minimum_distance,distance)
        omit[fi]=True;records.append(dict(triangles=int(nfaces),vertices=len(vi),signed_volume_m3=volume,
            minimum_outward_exit_dot=minimum_dot,minimum_exit_distance_m=minimum_distance,
            proof='Every component vertex has outward exits in six axis directions against the original main closed solid'))
    return omit,records


FALL_RECT=(-1055.,-920.,1125.,1270.)


def fall_clearance(xy):
    xmin,xmax,ymin,ymax=FALL_RECT
    return np.maximum.reduce((xmin-xy[:,0],xy[:,0]-xmax,ymin-xy[:,1],xy[:,1]-ymax))


def check_protected_falls(reference,vertices,triangles,origin,basis,tolerance=.001,provisional=False):
    """Bidirectional original-surface samples in the agreed full-height box."""
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    old=np.load(reference,allow_pickle=False);ov=old['vertices_m'];ot=old['triangles']
    def samples(v,t):
        p=np.vstack((v,v[t].mean(1)));return p[fall_clearance(p[:,:2])<=0]
    def tree(v,t):return BVHTree.FromPolygons([Vector(tuple(p)) for p in (v-origin)@basis],t.tolist(),all_triangles=True)
    old_tree=tree(ov,ot);new_tree=tree(vertices,triangles)
    result={}
    for name,points,target in [('old_to_new',samples(ov,ot),new_tree),('new_to_old',samples(vertices,triangles),old_tree)]:
        if len(points)<20:raise ValueError('Protected waterfall region unexpectedly empty')
        distances=[target.find_nearest(Vector(tuple(p)))[3] for p in (points-origin)@basis]
        maximum=max(distances)
        if maximum>tolerance and not provisional:raise ValueError(f'Waterfall corridor changed: {name} {maximum}m')
        result[name]=dict(samples=len(points),maximum_distance_m=maximum)
    result['reference_sha256']=hashlib.sha256(reference.read_bytes()).hexdigest()
    result['allowed_sampling_tolerance_m']=tolerance
    result['trajectory_revalidation_required']=any(result[name]['maximum_distance_m']>.001 for name in ['old_to_new','new_to_old'])
    result['passes_sampling_tolerance']=all(result[name]['maximum_distance_m']<=tolerance for name in ['old_to_new','new_to_old'])
    result['provisional_only']=provisional
    result['scope']='Original triangle surface vertices and centroids in the full-height agreed XY box; no continuous all-point or waterfall simulation claim'
    return result


def base_surface(ring,row,col,hierarchy=False):
    original=sample_ring(ring,row,col);u=(col-537)/64*500
    lower=sample_ring(ring,np.full(len(col),8.),col)
    foot_r=np.linalg.norm(lower[:,:2],axis=1);foot_z=lower[:,2]
    knots=[0,44,91,151,208,272,333,387,449,500]
    setback=np.interp(u,knots,[85,67,48,73,39,57,82,43,69,85])
    crest=np.interp(u,knots,[37,47,39,51,44,40,55,42,47,37])
    t=np.clip((row-8)/112,0,1);z=foot_z+(crest-foot_z)*t
    radius=foot_r+25*t+(setback-25)*t*t
    angle=col*2*np.pi/1536
    position=np.column_stack((np.cos(angle)*radius,np.sin(angle)*radius,z))
    back=row>120
    if back.any():
        end=sample_ring(ring,np.full(len(col),160.),col);b=(row-120)/40
        cr=np.column_stack((np.cos(angle)*(foot_r+setback),np.sin(angle)*(foot_r+setback),crest))
        position[back]=(cr+(end-cr)*b[:,None])[back]
        # Most of the hinterland stays a low connected plateau. Its final
        # return to the immutable ring edge is explicit, not a hidden skirt.
        q=np.clip((b-.72)/.28,0,1);q=q*q*(3-2*q)
        position[back,2]=(crest+(end[:,2]-crest)*q)[back]
    side=np.clip(np.minimum(u/35,(500-u)/35),0,1)
    side=side*side*(3-2*side)
    result=original+(position-original)*side[:,None]
    if hierarchy:
        # Finite oblique exposed shoulders vary the treatment margin with
        # height. The original outer edge remains fixed; no extra field of
        # small blocks is spread across the collar.
        left=np.interp(z,[-180,-105,-65,-22,14,60],[35,12,29,10,25,35])
        right=np.interp(z,[-180,-100,-53,-8,31,65],[35,18,9,27,13,35])
        q=np.clip(np.minimum(u/left,(500-u)/right),0,1);q=q*q*(3-2*q)
        alternate=original+(position-original)*q[:,None]
        clearance=np.minimum(fall_clearance(result[:,:2]),fall_clearance(alternate[:,:2]))
        guard=np.clip((clearance-10)/15,0,1);guard=guard*guard*(3-2*guard)
        result+=(alternate-result)*guard[:,None]
    fixed=(row<=8)|(row==160)|(col==537)|(col==601)
    result[fixed]=original[fixed]
    return result


def main():
    import bpy,bmesh
    from mathutils import Vector,geometry
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--hierarchy',action='store_true',help='Add three unequal connected buttresses and irregular side shoulders')
    p.add_argument('--protected-surface',type=Path,help='Frozen F whole-mesh source for the agreed waterfall corridor check')
    p.add_argument('--corridor-tolerance-m',type=float,default=.001,help='At most.02m for provisional comparison; any result above.001m requires actual trajectory revalidation')
    p.add_argument('--provisional-corridor-review',action='store_true',help='Write a non-certified diagnostic mesh for actual pinned trajectory checks even if the sampled corridor gate fails')
    a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);out=a.output.resolve()
    if out.exists() or 'saved' not in [v.lower() for v in out.parts]:p.error('New Saved output directory required')
    if a.hierarchy and not a.protected_surface:p.error('Hierarchy requires the agreed frozen waterfall source')
    if not 0<a.corridor_tolerance_m<=.02:p.error('Corridor tolerance must be positive and at most.02m')
    out.mkdir(parents=True)
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    data=np.load(a.input,allow_pickle=False);ring=data['vertices_m'].reshape(161,1537,3)
    rr,cc=np.meshgrid(np.arange(161),np.arange(537,602),indexing='ij')
    row=rr.ravel().astype(float);col=cc.ravel().astype(float)
    old=sample_ring(ring,row,col);front=base_surface(ring,row,col,a.hierarchy)
    origin=ring[4,569].copy();origin[2]=0
    ang=569*2*np.pi/1536
    basis=np.column_stack(([np.sin(ang),-np.cos(ang),0],[np.cos(ang),np.sin(ang),0],[0,0,1]))
    # The reviewed ring has increasing angular columns opposite local U.
    # All geometry is made in this rigid metre chart, then restored to arena.
    local=(front-origin)@basis;old_local=(old-origin)@basis
    ids=(np.arange(160)[:,None]*65+np.arange(64)).ravel()
    whole_top=np.concatenate((np.stack((ids,ids+65,ids+1),1),np.stack((ids+1,ids+65,ids+66),1)))
    protected_base_triangles=0
    if a.hierarchy:
        # Vertex-only distance fades can still alter a long triangle crossing
        # the corridor. Freeze every corner of each old OR proposed triangle
        # whose XY bounds overlap the agreed rectangle plus10m.
        original_f=base_surface(ring,row,col,False)
        lo=np.minimum(front[whole_top][:,:,:2].min(1),original_f[whole_top][:,:,:2].min(1))
        hi=np.maximum(front[whole_top][:,:,:2].max(1),original_f[whole_top][:,:,:2].max(1))
        xmin,xmax,ymin,ymax=FALL_RECT
        locked=(hi[:,0]>=xmin-10)&(lo[:,0]<=xmax+10)&(hi[:,1]>=ymin-10)&(lo[:,1]<=ymax+10)
        points=np.unique(whole_top[locked]);front[points]=original_f[points]
        local=(front-origin)@basis;protected_base_triangles=int(locked.sum())
    whole_front=front.copy();whole_local=local.copy()
    # Keep the historical overhanging collar OUTSIDE the Boolean operand.
    # It is retained exactly as a visible source surface. Only the fresh,
    # monotone bedrock core needs a solid closure for the rock union.
    core_ids=np.arange(len(local)).reshape(161,65)[8:151,5:60].ravel()
    front=front[core_ids];local=local[core_ids];core_rows,core_cols=143,55
    count=len(local);ids=(np.arange(core_rows-1)[:,None]*core_cols+np.arange(core_cols-1)).ravel()
    top=np.concatenate((np.stack((ids,ids+core_cols,ids+1),1),np.stack((ids+1,ids+core_cols,ids+core_cols+1),1)))
    # The source collar contains real radial overhangs. Projecting its backing
    # vertically to a flat bottom creates overlapping XY side strips. A deep
    # outward/downward backing follows the same 3D section instead; it is
    # authoring closure only and is never exported as visible terrain.
    backing=front.copy();radial=backing[:,:2]/np.linalg.norm(backing[:,:2],axis=1)[:,None]
    backing[:,:2]+=radial*500.;backing[:,2]-=100.
    bottom=(backing-origin)@basis
    boundary=list(range(core_cols))+[r*core_cols+core_cols-1 for r in range(1,core_rows)]+list(range(core_rows*core_cols-2,(core_rows-1)*core_cols-1,-1))+[r*core_cols for r in range(core_rows-2,0,-1)]
    verts=np.vstack((local,bottom));faces=top.tolist()+top[:,::-1].__add__(count).tolist()
    for x,y in zip(boundary,boundary[1:]+boundary[:1]):faces.append((x,y,y+count,x+count))
    host=mesh_object('Connected bedrock',verts.tolist(),faces)
    for name in ['Exposed rock','Hidden backing']:
        host.data.materials.append(bpy.data.materials.new(name))
    for poly in host.data.polygons:poly.material_index=0 if poly.index<len(top) else 1
    bm=bmesh.new();bm.from_mesh(host.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(host.data);bm.free()
    inherited_specifications=[];base_closed_path=None
    if a.hierarchy:
        # Work from the already accepted ORIGINAL TRIANGLES, not a fresh
        # replay of the old Boolean polygons. Replaying those polygons can
        # change interior diagonals by centimetres even far from a new cut.
        # This also avoids rebuilding hundreds of unchanged rock volumes.
        base_closed_path=a.protected_surface.parent/'closed_diagnostic.npz'
        fixed=np.load(base_closed_path,allow_pickle=False)
        fv=(fixed['vertices_m']-origin)@basis;ft=fixed['triangles'];fk=fixed['face_kind']
        remove_internal,_=internal_shells(fv,ft);ft=ft[~remove_internal];fk=fk[~remove_internal]
        materials=list(host.data.materials);bpy.data.objects.remove(host,do_unlink=True)
        host=mesh_object('Frozen F original triangle bedrock',fv.tolist(),ft.tolist())
        for material in materials:host.data.materials.append(material)
        for poly,kind in zip(host.data.polygons,fk):poly.material_index=int(kind)
        ref=np.load(a.protected_surface,allow_pickle=False)
        inherited_specifications=json.loads(str(ref['metadata_json']))['volumes']
    blocks=bpy.data.collections.new('Connected finite fracture volumes');bpy.context.scene.collection.children.link(blocks)
    rng=np.random.default_rng(71463);sites=[];radii=[]
    for q in rng.uniform([36,-140],[464,46],(9000,2)):
        # Three physical size classes coexist; the larger masses interrupt
        # clusters of subordinate faces rather than forming a regular wall.
        radius=11+5*np.sin(q[0]/83+.7)*np.cos(q[1]/57-.4)
        if sites:
            delta=np.asarray(sites)-q;distance=np.sum(delta*delta,1)
            if np.any(distance<(.61*(np.asarray(radii)+radius))**2):continue
        column=537+q[0]/500*64
        foot=sample_ring(ring,np.array([8.]),np.array([column]))[0,2]
        if q[1]<foot+24:continue
        crest=base_surface(ring,np.array([120.]),np.array([column]),a.hierarchy)[0,2]
        if q[1]>crest-3:continue
        sites.append(q);radii.append(radius)
    specifications=list(inherited_specifications)
    def add_block(u,z,width,height,depth,level):
        column=537+u/500*64
        foot=sample_ring(ring,np.array([8.]),np.array([column]))[0,2]
        crest=base_surface(ring,np.array([120.]),np.array([column]),a.hierarchy)[0,2]
        rr=8+112*(z-foot)/(crest-foot)
        center_world=base_surface(ring,np.array([rr]),np.array([column]),a.hierarchy)[0]
        angle=column*2*np.pi/1536
        tangent=np.array([np.sin(angle),-np.cos(angle),0.]);radial=np.array([np.cos(angle),np.sin(angle),0.])
        axes=np.column_stack((tangent,radial,[0,0,1.]))
        if level==1:center_world-=radial*3.0
        if level==2:center_world-=radial*8.0
        # Unequal oblique joint families, with a quiet broad front face.
        # These are true solid planes, not normals painted on a slab.
        normals=np.asarray([[1,rng.uniform(-.25,.3),rng.uniform(-.36,.38)],
            [-1,rng.uniform(-.3,.35),rng.uniform(-.35,.35)],
            [rng.uniform(-.18,.18),-1,rng.uniform(-.22,.22)],
            [0,1,0],[rng.uniform(-.43,.42),rng.uniform(-.35,.34),1],
            [rng.uniform(-.41,.4),rng.uniform(-.35,.32),-1]],float)
        distances=np.array([width*.52,width*.48,depth*.24,depth*1.55,height*.53,height*.47])
        if level==2:
            # A projecting buttress has two unequal inclined front exposures
            # meeting at a finite ridge, rather than one oversized flat plate.
            normals[2]=[.67,-1,.13]
            normals=np.vstack((normals,[-.49,-1,-.11]))
            distances=np.r_[distances,depth*.25]
        # A few bounded edge planes break corners; they do not make rounded
        # boulders or a blanket of small chipped bumps.
        if level in (0,2):
            sign=-1 if rng.random()<.5 else 1
            normals=np.vstack((normals,[sign,-.55,.72],[-sign,-.35,-.77],[sign,-.4,-.48]))
            distances=np.r_[distances,width*.36+height*.28,width*.32+height*.3,width*.34+height*.29]
        dip=np.deg2rad(19+rng.uniform(-16,17));cs,sn=np.cos(dip),np.sin(dip)
        normal_u=normals[:,0].copy();normal_z=normals[:,2].copy()
        normals[:,0]=normal_u*cs-normal_z*sn;normals[:,2]=normal_u*sn+normal_z*cs
        length=np.linalg.norm(normals,axis=1);normals/=length[:,None];distances/=length
        if level in (0,2):
            # Selective 0.6–1.8m arris truncation, plus an unequal finite corner
            # fracture. Large face interiors stay planar and unchanged. This
            # is actual extra solid geometry, not a global bevel modifier.
            side_id=int(rng.choice([0,1,4,5]));n=normals[2]+normals[side_id]
            d=distances[2]+distances[side_id]-rng.uniform(.6,1.8)*np.linalg.norm(n)
            normals=np.vstack((normals,n/np.linalg.norm(n)));distances=np.r_[distances,d/np.linalg.norm(n)]
            if rng.random()<.62:
                ids=[2,int(rng.choice([0,1])),int(rng.choice([4,5]))]
                n=normals[ids].sum(0);d=distances[ids].sum()-rng.uniform(1.4,3.5)*np.linalg.norm(n)
                normals=np.vstack((normals,n/np.linalg.norm(n)));distances=np.r_[distances,d/np.linalg.norm(n)]
        points,_=geometry.points_in_planes([Vector((*n,-d)) for n,d in zip(normals,distances)],1.e-6,1.e-6)
        if len(points)<4:raise ValueError('Degenerate authored rock block')
        pv=np.asarray([tuple(v) for v in points]);world=center_world+pv@axes.T
        columns=np.mod(np.arctan2(world[:,1],world[:,0]),2*np.pi)/(2*np.pi)*1536
        if columns.min()<542.3 or columns.max()>595.7:return
        lower_z=sample_ring(ring,np.full(len(columns),8.),columns)[:,2]
        if np.any(world[:,2]<lower_z+5):return
        if level==2:
            lo=world[:,:2].min(0);hi=world[:,:2].max(0);xmin,xmax,ymin,ymax=FALL_RECT
            if not (hi[0]<xmin-10 or lo[0]>xmax+10 or hi[1]<ymin-10 or lo[1]>ymax+10):return
        if a.hierarchy and level!=2:return
        block=wedge('Primary' if level==0 else 'Shoulder',((world-origin)@basis).tolist())
        for collection in list(block.users_collection):collection.objects.unlink(block)
        blocks.objects.link(block);block.data.materials.append(host.data.materials[0])
        specifications.append(dict(u_m=u,z_m=z,width_m=width,height_m=height,depth_m=depth,level=level))
    for (u,z),radius in zip(sites,radii):
        add_block(float(u),float(z),float(radius*rng.uniform(1.9,2.65)),float(radius*rng.uniform(1.5,2.3)),float(rng.uniform(8,16)),0)
    # Sparse unequal 2–8m shoulder pieces only on the selected primary joint
    # terminations. Their rear faces embed into the connected primary body.
    for (u,z),radius in list(zip(sites,radii))[::4]:
        add_block(float(u+radius*.45),float(z-radius*.45),float(rng.uniform(3,8)),float(rng.uniform(3,7)),float(rng.uniform(7,11)),1)
    if a.hierarchy:
        # Three finite interlocking masses, each with unequal lower face,
        # leaning upper face and one offset shoulder. The existing small
        # planes remain only where these broader quiet exposures do not bury
        # them. No extra small-block blanket is introduced.
        buttresses=[(258,-61,78,85,34),(251,-15,56,62,31),(281,-35,43,57,27),
                    (341,-62,66,99,36),(327,-4,61,72,30),(354,-21,45,56,29),
                    (414,-43,75,101,33),(426,8,53,70,31),(391,-12,43,61,26)]
        for spec in buttresses:add_block(*spec,2)
    print(json.dumps(dict(stage='Boolean union',primary=sum(v['level']==0 for v in specifications),all_volumes=len(specifications))),flush=True)
    bpy.context.view_layer.objects.active=host;host.select_set(True)
    mod=host.modifiers.new('Finite shared bedrock union','BOOLEAN');mod.operation='UNION';mod.solver='EXACT';mod.operand_type='COLLECTION';mod.collection=blocks
    bpy.ops.object.modifier_apply(modifier=mod.name)
    # Only weld coincident Boolean points, at ten micrometres. No decimation,
    # broad area culling, or arbitrary source-triangle removal is performed.
    bm=bmesh.new();bm.from_mesh(host.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1.e-5)
    bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1.e-5)
    numerical_boundary_repairs=[]
    boundary_edges=[edge for edge in bm.edges if edge.is_boundary]
    if boundary_edges:
        corner_vertices=list({v for edge in boundary_edges for v in edge.verts})
        if len(boundary_edges)!=3 or len(corner_vertices)!=3:raise ValueError('Unexpected open Boolean core')
        points=np.asarray([tuple(v.co) for v in corner_vertices]);edge_max=max(edge.calc_length() for edge in boundary_edges)
        area=float(np.linalg.norm(np.cross(points[1]-points[0],points[2]-points[0]))*.5)
        if edge_max>.5 or area>1.e-6:raise ValueError('Non-microscopic Boolean opening must not be patched automatically')
        material=boundary_edges[0].link_faces[0].material_index
        created=bmesh.ops.holes_fill(bm,edges=boundary_edges,sides=3)['faces']
        for face in created:face.material_index=material
        numerical_boundary_repairs.append(dict(operation='Close one existing three-edge numerical slit without moving vertices',
            area_m2=area,maximum_edge_m=edge_max,vertices_chart_m=points.tolist()))
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(host.data);bm.free();host.data.update()
    host.data.calc_loop_triangles()
    v=np.asarray([tuple(x.co) for x in host.data.vertices],float)
    tri=np.asarray([tuple(t.vertices) for t in host.data.loop_triangles],int)
    kind=np.asarray([host.data.polygons[t.polygon_index].material_index for t in host.data.loop_triangles])
    closed_metrics=mesh_metrics(v,tri)
    np.savez_compressed(out/'closed_diagnostic.npz',vertices_m=origin+v@basis.T,triangles=tri,face_kind=kind)
    omitted,internal_records=internal_shells(v,tri)
    tri=tri[(kind==0)&~omitted]
    # Restore the original retained boundary after float32 Blender authoring.
    # No new Boolean-intersection vertex is snapped by this perimeter-only map.
    for idx in boundary:
        d=np.linalg.norm(v-local[idx],axis=1);near=np.flatnonzero(d<.00012)
        if not len(near):raise ValueError('Lost original perimeter vertex')
        v[near]=local[idx]
    used=np.unique(tri);remap=np.full(len(v),-1,int);remap[used]=np.arange(len(used));v=v[used];tri=remap[tri]
    core_world=origin+v@basis.T
    lookup={tuple(np.round(point,6)):i for i,point in enumerate(whole_front)}
    mapping=[];added=[]
    for point in core_world:
        existing=lookup.get(tuple(np.round(point,6)))
        if existing is not None and np.linalg.norm(whole_front[existing]-point)<2.e-6:mapping.append(existing)
        else:mapping.append(len(whole_front)+len(added));added.append(point)
    remove=np.all(np.isin(whole_top,core_ids),axis=1)
    world=np.concatenate((whole_front,np.asarray(added)))
    tri=np.concatenate((whole_top[~remove],np.asarray(mapping)[tri]))
    used=np.unique(tri);remap=np.full(len(world),-1,int);remap[used]=np.arange(len(used));world=world[used];tri=remap[tri]
    waterfall_check=check_protected_falls(a.protected_surface,world,tri,origin,basis,a.corridor_tolerance_m,a.provisional_corridor_review) if a.hierarchy else None
    meta=dict(schema='directorscut.jointed_headland.v1',units='absolute arena metres',
        source_sha256=hashlib.sha256(a.input.read_bytes()).hexdigest(),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        canonical_crop=dict(row_start=0,row_end=160,column_start=537,column_end=601),
        construction='Whole face rebuilt as connected bedrock and unequal finite plane-defined overlapping rock volumes; exact Boolean exterior',
        closed_metrics=closed_metrics,buried_shells_omitted=internal_records,
        numerical_boundary_repairs=numerical_boundary_repairs,
        primary_sites_considered=len(sites),primary_volumes=sum(v['level']==0 for v in specifications),
        hierarchy_enabled=a.hierarchy,buttress_volumes=sum(v['level']==2 for v in specifications),
        protected_fall_rectangle_xy_m=list(FALL_RECT) if a.hierarchy else None,
        protected_base_triangles=protected_base_triangles,
        protected_fall_surface_check=waterfall_check,
        frozen_base_closed_source_sha256=hashlib.sha256(base_closed_path.read_bytes()).hexdigest() if base_closed_path else None,
        total_volumes=len(specifications),volumes=specifications,
        limitations='Authored rock construction, not physical erosion. Old interior temple supports and fall crests must rebind to actual final triangles.')
    frame=dict(frame_origin_world_m=origin,basis_u_world=basis[:,0],basis_v_world=basis[:,1],basis_z_world=basis[:,2])
    for name,pv,pt in [('baseline',old,whole_top),('candidate',world,tri)]:
        np.savez_compressed(out/(name+'.npz'),vertices_m=pv,triangles=pt,metadata_json=np.asarray(json.dumps(meta)),**frame)
    (out/'manifest.json').write_text(json.dumps(meta,indent=2))
    # Export only after the actual CPU render is judged; the original ring
    # splice runs here so its geometry/float precision gates already apply.
    cameras={'wall':dict(location=[0,-560,25],target=[0,70,-25],width=650),
        'oblique':dict(location=[330,-500,180],target=[0,70,-25],width=660),
        'close_wall':dict(location=[-20,-200,-5],target=[-20,60,-20],width=155)}
    (out/'cameras.json').write_text(json.dumps(dict(cameras=cameras),indent=2))
    splice_full_ring(data,world,tri,meta,out/'full_ring_candidate.npz')
    print(json.dumps(dict(output=str(out),closed=closed_metrics,visible=mesh_metrics(world,tri)),indent=2),flush=True)


if __name__=='__main__':main()
