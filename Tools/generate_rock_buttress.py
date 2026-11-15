"""One authored, connected rock buttress on the original RingPilotB surface.

CPU Blender. Metres throughout. This is explicit geological art direction,
not a physical fracture simulation, copied rock prop, or heightfield replacement.
The lower twenty metres of the receiving cliff and the entire crop perimeter
remain outside the added rock volume. Original arbitrary triangles are retained.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_fractured_cliff_outcrop import mesh_object, wedge, apply_boolean, arrays, clean_boolean_mesh, mesh_metrics


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, vertices, triangles, origin, basis, metadata, **extra):
    np.savez_compressed(path, vertices_m=origin+vertices@basis.T, triangles=triangles,
        frame_origin_world_m=origin, basis_u_world=basis[:, 0], basis_v_world=basis[:, 1],
        basis_z_world=basis[:, 2], metadata_json=np.asarray(json.dumps(metadata)), **extra)


def remove_microscopic_boolean_edges(vertices,triangles,kinds):
    """Collapse only a sub-.1mm edge belonging to a failed-area triangle.

    A general area threshold would silently delete real faces and create holes.
    This maps the two endpoints together and removes only the resulting two
    repeated-index faces; all other triangles and positions remain unchanged.
    """
    v=vertices.copy();t=triangles.copy();k=kinds.copy();events=[]
    for _ in range(8):
        p=v[t];area=.5*np.linalg.norm(np.cross(p[:,1]-p[:,0],p[:,2]-p[:,0]),axis=1)
        bad=np.flatnonzero(area<1.e-10)
        if not len(bad):return v,t,k,events
        face=int(bad[0]);tri=t[face]
        pairs=np.array([[0,1],[1,2],[2,0]])
        lengths=np.linalg.norm(v[tri[pairs[:,0]]]-v[tri[pairs[:,1]]],axis=1)
        edge=tri[pairs[int(np.argmin(lengths))]]
        if lengths.min()>1.e-4:raise ValueError('Degenerate triangle has no microscopic edge; refuse general repair')
        keep,remove=map(int,edge)
        t[t==remove]=keep
        valid=(t[:,0]!=t[:,1])&(t[:,1]!=t[:,2])&(t[:,0]!=t[:,2])
        events.append(dict(source_triangle=face,edge_m=float(lengths.min()),area_m2=float(area[face]),removed_repeated_index_faces=int((~valid).sum())))
        t,k=t[valid],k[valid]
        used=np.unique(t);mapping=np.full(len(v),-1,int);mapping[used]=np.arange(len(used))
        v,t=v[used],mapping[t]
    raise ValueError('Unexpected number of microscopic Boolean defects')


def splice_full_ring(source, patch_vertices_world, patch_triangles, metadata, output):
    """Replace exact canonical crop triangles; retain original grid separately.

    The new surface is arbitrary3D, so the original grid is a valid contact
    provider only OUTSIDE the replacement. In particular it is not a waterfall
    or collision query for the new buttress. Existing distant supports/outlet
    remain on unchanged original triangles.
    """
    original=source['vertices_m'];faces=source['triangles']
    crop=metadata['canonical_crop'];rows=faces//1537;cols=faces%1537
    remove=np.all((rows>=crop['row_start'])&(rows<=crop['row_end'])&
                  (cols>=crop['column_start'])&(cols<=crop['column_end']),axis=1)
    lookup={tuple(np.round(p,6)):i for i,p in enumerate(original)}
    mapping=[];added=[]
    for p in patch_vertices_world:
        existing=lookup.get(tuple(np.round(p,6)))
        if existing is not None and np.linalg.norm(original[existing]-p)<2.e-6:
            mapping.append(existing)
        else:mapping.append(len(original)+len(added));added.append(p)
    vertices=np.concatenate((original,np.asarray(added)))
    triangles=np.concatenate((faces[~remove],np.asarray(mapping)[patch_triangles]))
    used=np.unique(triangles);remap=np.full(len(vertices),-1,int);remap[used]=np.arange(len(used))
    parent_index=np.where(used<len(original),used,-1)
    # Keep small fracture edges near the authoring origin. Arena-centred
    # float32 metres then float32 centimetres collapsed212 valid triangles in
    # the first native import. Translation alone preserves world geometry and
    # connectivity, unlike deleting the faces or weakening the import audit.
    export_origin=np.round(original[4*1537+(crop['column_start']+crop['column_end'])//2]).copy()
    export_origin[2]=-25.
    local_cm=((vertices[used]-export_origin).astype(np.float32).astype(np.float64)*[100.,-100.,100.]).astype(np.float32)
    export_faces=remap[triangles]
    cross_cm=np.cross(local_cm[export_faces[:,1]]-local_cm[export_faces[:,2]],local_cm[export_faces[:,0]]-local_cm[export_faces[:,2]])
    collapsed=int(np.count_nonzero(np.einsum('ij,ij->i',cross_cm,cross_cm)==0))
    if collapsed:raise ValueError(f'{collapsed} source triangles collapse in actual float32 export coordinates')
    frame=dict(frame_origin_world_m=export_origin,basis_u_world=np.array([1.,0,0]),basis_v_world=np.array([0.,1,0]),basis_z_world=np.array([0.,0,1]))
    m=dict(metadata,full_ring_splice=dict(removed_original_triangles=int(remove.sum()),
        added_patch_triangles=len(patch_triangles),new_vertices=len(added),
        original_grid_contact_limit='Valid only outside canonical replacement crop; use actual new triangles for the buttress.',
        unchanged_source_triangles=len(faces)-int(remove.sum())),
        changed_column_range=[crop['column_start'],crop['column_end']],
        export_precision=dict(local_origin_arena_m=export_origin.tolist(),float32_cm_collapsed_triangles=collapsed,
                              operation='Rigid translation only; all world positions and triangle indices unchanged'))
    np.savez_compressed(output,vertices_m=vertices[used],triangles=remap[triangles],
        source_ring_vertex_index=parent_index,original_ring_vertices_m=original,original_ring_triangles=faces,
        metadata_json=np.asarray(json.dumps(m)),**frame)
    return m['full_ring_splice']


def fracture_edges(rock, materials):
    """Detach unequal finite edge wedges, leaving broad intact face interiors.

    The sampled features are actual high-angle edges of the authored solid,
    never internal coplanar triangulation edges or a regular spatial lattice.
    Two nested physical scales break the sharp macro corners without changing
    the full cliff into disconnected blocks or a periodic bedding pattern.
    """
    import bpy
    rng=np.random.default_rng(71426)
    records=[]
    for level, spacing, depth_scale in ((0,10.,1.),(1,5.,.45)):
        verts,tris=arrays(rock)
        xyz=verts[tris]; cross=np.cross(xyz[:,1]-xyz[:,0],xyz[:,2]-xyz[:,0])
        normals=cross/np.maximum(np.linalg.norm(cross,axis=1)[:,None],1.e-20)
        adjacency={}
        for i,t in enumerate(tris):
            for a,b in zip(t,np.roll(t,-1)):
                adjacency.setdefault(tuple(sorted((int(a),int(b)))),[]).append(i)
        options=[]
        for (a,b),face_ids in adjacency.items():
            if len(face_ids)!=2:continue
            n1,n2=normals[face_ids]
            if np.dot(n1,n2)>.93 or min(n1[1],n2[1])>-.25:continue
            p,q=verts[a],verts[b]; length=np.linalg.norm(q-p)
            if length<3.5 or max(p[2],q[2])<-72 or min(p[1],q[1])>110:continue
            n=n1+n2;n/=np.linalg.norm(n)
            if n[1]>.2:continue
            t=(q-p)/length;side=np.cross(t,n);side/=np.linalg.norm(side)
            number=min(5,max(1,int(length/spacing)))
            for k in range(number):
                f=(k+.3+.4*rng.random())/number
                pos=p+(q-p)*f
                if pos[2]<-72 or pos[2]>41 or rng.random()<.18:continue
                options.append((pos,n,t,side,float(rng.uniform(3.5,7)*depth_scale),float(rng.uniform(2.5,5)*depth_scale),float(rng.uniform(1.1,3)*depth_scale)))
        # Limit only computation/count, not the size of retained rock faces.
        rng.shuffle(options)
        for i,(p,n,t,side,half_length,half_width,depth) in enumerate(options[:34 if level==0 else 45]):
            contour=np.array([[-.9,-.35],[-.30,-1],[.65,-.62],[1,.20],[.20,.86],[-.7,.50]])
            front=p+contour[:,0,None]*t*half_length+contour[:,1,None]*side*half_width+n*8
            rear=p+contour[:,0,None]*t*half_length*.62+contour[:,1,None]*side*half_width*.52-n*depth
            cutter=wedge(f'Finite edge fracture {level}_{i}',np.concatenate((front,rear)).tolist())
            for mat in materials:cutter.data.materials.append(mat)
            for poly in cutter.data.polygons:poly.material_index=3
            apply_boolean(rock,cutter,'DIFFERENCE');bpy.data.objects.remove(cutter,do_unlink=True)
            records.append(dict(level=level,center_m=p.tolist(),half_length_m=half_length,half_width_m=half_width,depth_m=depth))
        clean_boolean_mesh(rock)
    return records


def weather_planes(rock, materials, helpers_dir, scan_manifest=None,joined=False):
    """Metre-scale coherent relief on a connected 3D solid, before host union.

    U/Z stay fixed; only V changes by a bounded U/Z-dependent field multiplied
    by a50m back fade. Thus this continuous shear has determinant >= .925.
    This does not certify the sampled triangles; their topology is separately
    checked after Boolean union. Actual macro overhangs remain in the solid.
    """
    import bpy
    from mathutils import noise, Vector
    sys.path.insert(0,str(helpers_dir))
    from generate_displaced_crater_face import refine, filter_height_to_mesh, sample_texture
    old_v,old_t=arrays(rock)
    rock.data.calc_loop_triangles()
    old_kinds=np.array([rock.data.polygons[t.polygon_index].material_index for t in rock.data.loop_triangles])
    edge=.50 if scan_manifest else .85
    xyz=old_v[old_t]
    normals=np.cross(xyz[:,1]-xyz[:,0],xyz[:,2]-xyz[:,0])
    normals/=np.maximum(np.linalg.norm(normals,axis=1)[:,None],1.e-20)
    selected=((xyz[:,:,1].min(axis=1)<140)&(normals[:,1]<.35)).astype(np.int8)
    if joined:selected*=old_kinds==3
    v,t,lineage,iterations=refine(old_v,old_t,selected,edge)
    def smooth(a,b,x):
        x=np.clip((x-a)/(b-a),0,1);return x*x*(3-2*x)
    # Three coherent octaves span14m to~2.4m, deliberately resolved by .85m
    # edges. A rotated second field breaks directional repetition. This is
    # authored surface weathering, not physically simulated erosion.
    u,z=v[:,0],v[:,2]
    f=np.empty(len(v))
    for i,(x,h) in enumerate(zip(u,z)):
        p=Vector(((.93*x+.37*h)/14.,0.731,(-.37*x+.93*h)/14.))
        q=Vector(((.58*x-.815*h)/5.5,3.173,(.815*x+.58*h)/5.5))
        f[i]=1.85*(noise.ridged_multi_fractal(p,.85,2.4,3.,1.,2.,noise_basis='PERLIN_ORIGINAL')-1.)
        f[i]+=.40*(noise.ridged_multi_fractal(q,.9,2.2,2.,1.,2.,noise_basis='VORONOI_F2F1')-1.)
    f=np.clip(f,-2.5,2.5)
    scan_info={}
    if scan_manifest:
        scan_info=json.loads(Path(scan_manifest).read_text(encoding='utf-8'))
        height_path=Path(scan_info['field_path'])
        if digest(height_path)!=scan_info['field_sha256']:raise ValueError('Scan field differs from verified provenance')
        field=np.load(height_path,allow_pickle=False)
        field,filter_info=filter_height_to_mesh(field,scan_info['width_metres'],scan_info['height_metres'],edge)
        sample=sample_texture(field,np.column_stack((u/scan_info['width_metres'],z/scan_info['height_metres'])))
        f=f*.18+(sample-float(field.mean()))*scan_info['authored_displacement_scale']
        scan_info=dict(manifest_path=str(Path(scan_manifest).resolve()),manifest_sha256=digest(scan_manifest),
            field_sha256=scan_info['field_sha256'],width_m=scan_info['width_metres'],height_m=scan_info['height_metres'],
            source_authored_relief_m=scan_info['authored_displacement_scale'],filter=filter_info,
            native_scale_preserved=True,license='Poly Haven CC0; external source files remain under ignored Saved')
    g=(1-smooth(90,140,v[:,1]))*smooth(-83,-67,z)
    if joined:
        # Detail happens after the coarse Boolean; this avoids intersecting a
        # half-million-triangle scan mesh just to establish the rock/host join.
        # Every original host surface vertex remains fixed. Fade across3m of
        # new rock next to that exact join rather than creating a hard step.
        fixed=np.unique(t[old_kinds[lineage]!=3])
        edges=np.unique(np.sort(np.concatenate((t[:,[0,1]],t[:,[1,2]],t[:,[2,0]])),axis=1),axis=0)
        fade=np.ones(len(v));fade[fixed]=0.
        step=np.linalg.norm(v[edges[:,0]]-v[edges[:,1]],axis=1)/3.
        for _ in range(24):
            before=fade.copy()
            np.minimum.at(fade,edges[:,0],before[edges[:,1]]+step)
            np.minimum.at(fade,edges[:,1],before[edges[:,0]]+step)
            if np.max(before-fade)<1.e-8:break
        g*=smooth(0.,1.,fade)
    new=v.copy();new[:,1]+=f*g
    obj=mesh_object('Physically resolved rock planes',new.tolist(),t.tolist())
    for mat in materials:obj.data.materials.append(mat)
    for poly in obj.data.polygons:poly.material_index=int(old_kinds[lineage[poly.index]]) if joined else 3
    bpy.data.objects.remove(rock,do_unlink=True)
    return obj,dict(edge_target_m=edge,refinement_iterations=iterations,
        method='Blender ridged multifractal, metre-scale coherent shear on already fractured solid',
        scale_m=[14.,5.833333333,2.430555556,5.5,2.5],maximum_absolute_shear_m=float(abs(f*g).max()),
        back_fade_m=[90,140],lower_fade_z_m=[-83,-67],
        detail_after_coarse_union=joined,
        scan=scan_info,
        continuous_map_determinant_lower_bound=None if joined else .925,discrete_injectivity_claim=False,
        refine_source_sha256=digest(Path(helpers_dir)/'generate_displaced_crater_face.py'))


def connected_fracture_volume(materials,poisson=False):
    """Boundary of retained irregular cells, with all internal faces omitted.

    Cell sites define a shared finite fracture network. A monotonic exterior
    release exposes many nonparallel interfaces; the rear bedrock stays filled.
    This is authored rock structure, not random holes cut into a soft parent.
    """
    import bpy,bmesh
    from mathutils import geometry,Vector
    rng=np.random.default_rng(71431)
    axes=[]
    for start,stop,lo,hi in [(-52,52,8,15),(-35,95,8,14),(-100,70,9,17)]:
        values=[float(start)]
        while values[-1]<stop:values.append(values[-1]+rng.uniform(lo,hi))
        axes.append(np.asarray(values))
    sites=np.stack(np.meshgrid(*axes,indexing='ij'),axis=-1).reshape(-1,3)
    sites+=rng.uniform(-3.0,3.0,sites.shape)
    if poisson:
        accepted=[];radii=[]
        for point in rng.uniform([-58,-42,-107],[58,102,77],(18000,3)):
            radius=8.5+3.7*np.sin(point[0]/31.+.8)*np.cos(point[2]/43.-.4)
            if accepted:
                delta=np.asarray(accepted)-point
                minimum=.5*(np.asarray(radii)+radius)
                if np.any(np.einsum('ij,ij->i',delta,delta)<minimum*minimum):continue
            accepted.append(point);radii.append(radius)
        sites=np.asarray(accepted)
    threshold=3.5*np.sin(sites[:,0]/23.)+4.5*np.sin(sites[:,2]/37.+sites[:,0]/51.)
    retained=sites[:,1]>threshold
    box_lo=np.array([-38.,-26.,-76.]);box_hi=np.array([38.,84.,43.])
    vertices=[];polygons=[];sources=[]
    for i,center in enumerate(sites):
        if not retained[i]:continue
        if np.any(center<box_lo-25) or np.any(center>box_hi+25):continue
        distance=np.sum((sites-center)**2,axis=1)
        neighbours=np.argsort(distance)[1:49]
        for completion in range(5):
            n=sites[neighbours]-center
            d=(np.sum(sites[neighbours]**2,axis=1)-center@center)*.5
            norm=np.linalg.norm(n,axis=1);n=n/norm[:,None];d=d/norm
            plane_n=np.concatenate((n,np.eye(3),-np.eye(3)))
            plane_d=np.r_[d,box_hi,-box_lo]
            points,used=geometry.points_in_planes([Vector((*nn,-dd)) for nn,dd in zip(plane_n,plane_d)],1.e-6,1.e-6)
            if len(points)<4:break
            p=np.asarray(points,dtype=float)
            # Add every actual omitted competitor rather than trusting a
            # fixed nearest-neighbour count in variable-density geology.
            own=np.sum((p-center)**2,axis=1)
            distances=np.sum((p[:,None]-sites[None])**2,axis=2)
            closer=np.argmin(distances,axis=1)
            violated=distances[np.arange(len(p)),closer]<own-.015
            if not violated.any():break
            added=np.setdiff1d(np.unique(closer[violated]),neighbours)
            if not len(added):raise ValueError('Voronoi intersection accuracy failed')
            neighbours=np.r_[neighbours,added]
        else:raise ValueError('Incomplete Voronoi cell neighbourhood')
        if len(points)<4:continue
        for plane_id in used:
            if plane_id<len(neighbours) and retained[neighbours[plane_id]]:continue
            ids=np.flatnonzero(abs(p@plane_n[plane_id]-plane_d[plane_id])<2.e-4)
            if len(ids)<3:continue
            pts=p[ids];normal=plane_n[plane_id]
            axis=np.array([0.,0.,1.]) if abs(normal[2])<.8 else np.array([1.,0.,0.])
            u=np.cross(axis,normal);u/=np.linalg.norm(u);v=np.cross(normal,u)
            delta=pts-pts.mean(axis=0)
            order=np.argsort(np.arctan2(delta@v,delta@u));pts=pts[order]
            base=len(vertices)
            # Rigid/shear mapping defines the broad scarp dip. Individual
            # fracture planes remain planes under this affine map.
            world=pts.copy();world[:,1]=pts[:,1]+.40*pts[:,2]+36.
            vertices.extend(world.tolist());polygons.append(tuple(range(base,base+len(pts))))
            sources.append(int(i))
    obj=mesh_object('Connected finite fracture bedrock',vertices,polygons)
    for mat in materials:obj.data.materials.append(mat)
    for p in obj.data.polygons:p.material_index=3
    bm=bmesh.new();bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.0008)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(obj.data);bm.free();obj.data.update()
    clean_boolean_mesh(obj)
    return obj,dict(method='Shared unequal finite Voronoi cells; only kept/removed interfaces and external boundaries exported',
        site_count=len(sites),retained_site_count=int(retained.sum()),visible_interface_polygons=len(polygons),
        site_distribution='Variable-radius Poisson,4.8..12.2m separation radii' if poisson else 'Irregular lattice diagnostic',
        physical_axis_spacings_m=[[8,15],[8,14],[9,17]],jitter_m=3.,
        chart_to_local='U=x, V=depth+.4*Z+36, Z=z',domain_m=[box_lo.tolist(),box_hi.tolist()],
        weld_tolerance_m=.0008,source_site_per_boundary_polygon=sources)


def main():
    import bpy
    import bmesh
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--edge-fractures', action='store_true')
    parser.add_argument('--weather-planes', action='store_true')
    parser.add_argument('--helpers-dir',type=Path,default=Path(__file__).resolve().parent)
    parser.add_argument('--scan-manifest',type=Path)
    parser.add_argument('--intermediate-fracture',action='store_true')
    parser.add_argument('--fracture-volume',action='store_true')
    parser.add_argument('--poisson-sites',action='store_true')
    parser.add_argument('--detail-after-union',action='store_true')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    out=args.output.resolve()
    if 'saved' not in [p.lower() for p in out.parts]:
        raise ValueError('Generated geometry belongs under Saved')
    out.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
    source=np.load(args.input, allow_pickle=False)
    ring=source['vertices_m'].reshape(161,1537,3)
    row_end=145; c0,c1=573,585; center=579
    angle=center*2*np.pi/1536
    basis=np.array([[np.sin(angle),np.cos(angle),0],[-np.cos(angle),np.sin(angle),0],[0,0,1.]])
    origin=ring[4,center].copy(); origin[2]=0
    crop=ring[:row_end+1,c0:c1+1].copy()
    top=(crop.reshape(-1,3)-origin)@basis
    rows,cols=crop.shape[:2]; count=len(top)
    a=(np.arange(rows-1)[:,None]*cols+np.arange(cols-1)).ravel()
    faces=np.concatenate((np.stack((a,a+cols,a+1),1),np.stack((a+1,a+cols,a+cols+1),1)))
    directed=np.concatenate((faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]]))
    _,inv,counts=np.unique(np.sort(directed,axis=1),axis=0,return_inverse=True,return_counts=True)
    perimeter=directed[counts[inv]==1]
    bottom=top.copy(); bottom[:,2]=float(top[:,2].min()-100)
    side=np.concatenate((np.stack((perimeter[:,1],perimeter[:,0],perimeter[:,0]+count),1),
                         np.stack((perimeter[:,1],perimeter[:,0]+count,perimeter[:,1]+count),1)))
    closed_faces=np.concatenate((faces,faces[:,::-1]+count,side))
    host=mesh_object('Original connected backing',np.concatenate((top,bottom)).tolist(),closed_faces.tolist())
    materials=[bpy.data.materials.new(name) for name in ['Original surface','Authoring bottom','Authoring side','New fracture face']]
    for mat in materials:host.data.materials.append(mat)
    for p in host.data.polygons:
        p.material_index=0 if p.index<len(faces) else (1 if p.index<2*len(faces) else 2)
    # Unequal connected front planes: a broad upper face, narrower toe, two
    # oblique shoulders and a deep continuous back contained by source bedrock.
    # Added mass begins 21m above mean water; nothing reaches the wet foot.
    sections=[(-83.,[(-18,32),(2,26),(23,35)]),
              (-60.,[(-31,29),(-5,20),(31,35)]),
              (-18.,[(-35,38),(2,27),(29,43)]),
              (28.,[(-27,58),(6,43),(35,71)]),
              (45.,[(-15,104),(5,91),(24,113)])]
    points=[[u,v,z] for z,line in sections for u,v in line]
    points.extend([[-30,182,35],[29,182,37],[-30,210,-90],[29,210,-90]])
    rock=wedge('Single continuous buttress',points)
    for mat in materials:rock.data.materials.append(mat)
    for p in rock.data.polygons:p.material_index=3
    fracture_volume={}
    if args.fracture_volume:
        bpy.data.objects.remove(rock,do_unlink=True)
        rock,fracture_volume=connected_fracture_volume(materials,args.poisson_sites)
        # Only the side/back silhouette is clipped by a broad host-scale
        # wedge. Its front is extended so fracture interfaces define the face.
        envelope_points=np.asarray(points,float)
        envelope_points[:15,1]-=32.
        envelope=wedge('Broad taper only',envelope_points.tolist())
        for mat in materials:envelope.data.materials.append(mat)
        for p in envelope.data.polygons:p.material_index=3
        apply_boolean(rock,envelope,'INTERSECT');bpy.data.objects.remove(envelope,do_unlink=True)
    # Finite open corner removals. No closed cavity cut into the old cliff.
    cutters=[
      ('Left oblique shoulder',[[-50,-30,4],[-50,150,4],[-50,150,75],[-50,-30,75],[-16,54,31],[-11,118,62]]),
      ('Upper right broken shoulder',[[18,-20,30],[58,-20,30],[58,180,30],[58,180,80],[18,-20,80],[17,104,52]]),
      ('Open flank undercut',[[-50,-30,-48],[-50,-30,-17],[-50,35,-26],[-50,35,-37],
                              [4,-30,-41],[4,-30,-22],[4,29,-29],[4,29,-34]]),
      ('Lower unequal corner',[[-48,-20,-81],[-48,75,-81],[-48,75,-43],[-48,-20,-43],[-23,29,-58],[-16,50,-81]])]
    if args.intermediate_fracture:
        # A broad open oblique failure surface joins the side of the mass;
        # the taper ends before the opposite shoulder, avoiding a full course.
        cutters.append(('Dipping shoulder release',[
            [-48,-40,-27],[-48,-40,15],[-48,48,-5],[-48,43,-12],
            [25,-40,-2],[25,-40,35],[25,48,14],[25,43,10]]))
    for name,pts in ([] if args.fracture_volume else cutters):
        cutter=wedge(name,pts)
        for mat in materials:cutter.data.materials.append(mat)
        for p in cutter.data.polygons:p.material_index=3
        apply_boolean(rock,cutter,'DIFFERENCE')
        bpy.data.objects.remove(cutter,do_unlink=True)
    clean_boolean_mesh(rock)
    edge_records=fracture_edges(rock,materials) if args.edge_fractures else []
    weather={}
    if args.weather_planes and not args.detail_after_union:rock,weather=weather_planes(rock,materials,args.helpers_dir,args.scan_manifest)
    rock_v,rock_t=arrays(rock)
    apply_boolean(host,rock,'UNION')
    bpy.data.objects.remove(rock,do_unlink=True)
    clean_boolean_mesh(host)
    if args.weather_planes and args.detail_after_union:
        host,weather=weather_planes(host,materials,args.helpers_dir,args.scan_manifest,joined=True)
    cv,ct=arrays(host)
    # Blender stores mesh positions as float32. Restore only exact surviving
    # source vertices via their float32 identity, not nearest-point snapping of
    # new Boolean intersections. This keeps the original crop edges exact.
    source_positions={tuple(p.astype(np.float32).astype(float)):p for p in np.concatenate((top,bottom))}
    restored=0
    for i,p in enumerate(cv):
        original=source_positions.get(tuple(p))
        if original is not None:cv[i]=original;restored+=1
    # Original side/bottom labels survive the union and are excluded exactly.
    host.data.calc_loop_triangles()
    kinds=np.array([host.data.polygons[t.polygon_index].material_index for t in host.data.loop_triangles])
    cv,ct,kinds,cleanup=remove_microscopic_boolean_edges(cv,ct,kinds)
    visible=ct[(kinds!=1)&(kinds!=2)]
    used=np.unique(visible); remap=np.full(len(cv),-1,int);remap[used]=np.arange(len(used))
    vv,vt=cv[used],remap[visible]
    metadata=dict(schema='directorscut.authored_rock_buttress.v1',source_kind='connected_authored_fracture_solid',
        source_path=str(args.input.resolve()),source_sha256=digest(args.input),generator_sha256=digest(__file__),
        coordinates='absolute arena metres; rigid local chart only; no rescaling',
        canonical_crop=dict(row_start=0,row_end=row_end,column_start=c0,column_end=c1),
        construction=('Connected shared-cell fracture volume, broad tapered envelope, union into original backed ring crop.' if args.fracture_volume else
                      'One convex connected bedrock mass, finite open corner removals, union into original backed ring crop.'),
        finite_edge_fractures=edge_records,exact_source_vertices_restored=restored,
        microscopic_edge_cleanup=cleanup,
        resolved_weathering=weather,
        connected_fracture_volume=fracture_volume,
        wet_foot_rule=('New fracture volume begins at Z=-76m, about29m above central mean water. Existing wet foot unchanged.' if args.fracture_volume else
                      'New solid minimum Z=-90m in buried backing; exposed front begins at -83m, approximately21m above mean water.'),
        limitations='Authored rock prototype, not physical fracture. Sample route clearance and native composition remain separate acceptance gates.',
        closed_metrics=mesh_metrics(cv,ct),visible_metrics=mesh_metrics(vv,vt),rock_metrics=mesh_metrics(rock_v,rock_t))
    save(out/'baseline.npz',top,faces,origin,basis,metadata)
    save(out/'candidate.npz',vv,vt,origin,basis,metadata,face_kind=kinds[(kinds!=1)&(kinds!=2)])
    save(out/'closed_bedrock.npz',cv,ct,origin,basis,metadata)
    save(out/'buttress_solid.npz',rock_v,rock_t,origin,basis,metadata)
    metadata['full_ring_splice']=splice_full_ring(source,origin+vv@basis.T,vt,metadata,out/'full_ring_candidate.npz')
    (out/'manifest.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
    cameras={'wall':dict(location=[15,-280,10],target=[0,60,-25],width=235),
             'oblique':dict(location=[180,-230,100],target=[0,65,-25],width=245),
             'close_wall':dict(location=[20,-175,-10],target=[0,45,-15],width=150)}
    (out/'cameras.json').write_text(json.dumps(dict(cameras=cameras),indent=2),encoding='utf-8')
    print(json.dumps(dict(output=str(out),closed=metadata['closed_metrics'],visible=metadata['visible_metrics'],
                         finite_edge_fractures=len(edge_records),resolved_weathering=weather),indent=2),flush=True)


if __name__=='__main__':main()
