"""True 3D, closed, finite-joint outcrop from the immutable eroded massif.

Blender CPU only. Example: blender --background --factory-startup --threads 8
--python THIS_FILE -- --input Saved/Massif-Cliff/seed_open256.npz
--output-dir Saved/FracturedOutcrop/first

The source crop is sampled on its original ABD/ADC triangles, closed with a
24m backing, and cut by finite geologically aligned volumes. Cutter intersection
with a shallow skin preserves the crop edge and connected back. Exact booleans
produce actual cavity roofs/overhangs, not height noise or a projected heightfield.
The outputs are original arbitrary triangle meshes in arena metres. No runtime
integration, Nanite, material displacement, or physical fracture claim is made.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import time

import numpy as np


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def sample_height(data, x, y, center_row, center_col):
    """Exact source-grid ABD/ADC height, x/y in crop-centred metric basis."""
    h = data['height_m']
    cell = float(data['cell_size_m'])
    row = center_row+np.asarray(y)/cell
    col = center_col+np.asarray(x)/cell
    r = np.floor(row).astype(int); c = np.floor(col).astype(int)
    if np.any(r<0) or np.any(c<0) or np.any(r>=h.shape[0]-1) or np.any(c>=h.shape[1]-1):
        raise ValueError('Crop sample outside measured source grid')
    v = row-r; u = col-c
    a,b,cc,d = h[r,c],h[r,c+1],h[r+1,c],h[r+1,c+1]
    return np.where(u>=v,a+(b-a)*u+(d-b)*v,a+(d-cc)*u+(cc-a)*v)


def validate_crop(data,row,col,width,depth,guard):
    cell = float(data['cell_size_m'])
    rx,cx = int(np.ceil(depth/(2*cell))),int(np.ceil(width/(2*cell)))
    if row-rx<1 or col-cx<1 or row+rx>=data['height_m'].shape[0]-1 or col+cx>=data['height_m'].shape[1]-1:
        raise ValueError('Crop extends outside source domain')
    sl=np.s_[row-rx:row+rx+1,col-cx:col+cx+1]
    if not np.all(data['heightfield_valid_mask'][sl]>0) or np.any(data['protected_mask'][sl]>=1):
        raise ValueError('Crop intersects invalid/protected coast, seam or temple support')
    if np.min(data['coast_distance_m'][sl])<30:
        raise ValueError('Keep this isolated experiment at least30m from shoreline')
    pad=(data['constraint_type']&8)>0
    extra=int(np.ceil(12/cell))
    if np.any(pad[max(0,row-rx-extra):row+rx+extra+1,max(0,col-cx-extra):col+cx+extra+1]):
        raise ValueError('Crop is within12m of a protected temple pad')
    if guard<3 or guard>=min(width,depth)/4:
        raise ValueError('Preserve a3m+ crop guard smaller than a quarter of crop depth')
    return dict(row=row,col=col,width_m=width,depth_m=depth,edge_guard_m=guard,
                min_coast_distance_m=float(data['coast_distance_m'][sl].min()),
                source_height_range_m=[float(data['height_m'][sl].min()),float(data['height_m'][sl].max())],
                protected_samples_in_crop=0,temple_pad_clearance_minimum_m=12.)


def mesh_object(name,vertices,faces):
    import bpy
    mesh=bpy.data.meshes.new(name+'Mesh')
    mesh.from_pydata(vertices,[],faces); mesh.update()
    obj=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(obj)
    return obj


def closed_shell(data,row,col,width,depth,spacing,z_origin,lower_m,upper_m=0.,name='Baseline'):
    xs=np.linspace(-width/2,width/2,int(round(width/spacing))+1)
    ys=np.linspace(-depth/2,depth/2,int(round(depth/spacing))+1)
    xx,yy=np.meshgrid(xs,ys)
    zz=sample_height(data,xx,yy,row,col)-z_origin
    top=np.stack((xx,yy,zz+upper_m),axis=-1).reshape(-1,3)
    bottom=top.copy();bottom[:,2]=zz.reshape(-1)-lower_m
    count=len(top); vertices=np.concatenate((top,bottom))
    rows,cols=xx.shape; faces=[]
    for r in range(rows-1):
        for c in range(cols-1):
            a=r*cols+c;b=a+1;cc=a+cols;d=cc+1
            faces.extend(((a,b,d),(a,d,cc),(d+count,b+count,a+count),(cc+count,d+count,a+count)))
    perimeter=list(range(cols))+[r*cols+cols-1 for r in range(1,rows)]+list(range(rows*cols-2,(rows-1)*cols-1,-1))+[r*cols for r in range(rows-2,0,-1)]
    for a,b in zip(perimeter,perimeter[1:]+perimeter[:1]):
        faces.append((a,a+count,b+count,b))
    return mesh_object(name,vertices.tolist(),faces),top,perimeter


def normalize(v):
    return np.asarray(v,float)/np.linalg.norm(v)


def box(name,center,axis_a,axis_b,dimensions):
    a=normalize(axis_a); b=normalize(np.asarray(axis_b)-a*np.dot(axis_b,a)); c=np.cross(a,b)
    axes=np.column_stack((a,b,c)); size=np.asarray(dimensions)/2
    signs=np.array([[-1,-1,-1],[1,-1,-1],[1,1,-1],[-1,1,-1],[-1,-1,1],[1,-1,1],[1,1,1],[-1,1,1]])
    vertices=np.asarray(center)+(signs*size)@axes.T
    faces=[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
    return mesh_object(name,vertices.tolist(),faces),axes


def wedge(name,points):
    """A finite convex detached block; unequal planes replace long saw kerfs."""
    import bmesh
    import bpy
    bm=bmesh.new()
    for point in points:bm.verts.new(point)
    hull=bmesh.ops.convex_hull(bm,input=list(bm.verts),use_existing_faces=False)
    unused=list(set(hull.get('geom_interior',[])+hull.get('geom_unused',[])))
    if unused:bmesh.ops.delete(bm,geom=unused,context='VERTS')
    bm.normal_update()
    mesh=bpy.data.meshes.new(name+'Mesh');bm.to_mesh(mesh);bm.free()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
    return obj


def detachment_specs(data,args,z_origin,rng):
    """Overlapping unequal scars expose rock faces, leaving projecting blocks.

    The shared strike/dip gives geological alignment. Finite convex wedges use
    unequal, tapered corners and several rear planes, not Voronoi noise dents or
    height-displacement. Their intersections form4..12m visible face fragments.
    """
    cuts=[]
    for layer,y0 in enumerate((-11.,0.,11.)):
        for index,x0 in enumerate((-25.,-14.,-3.,8.,20.,28.)):
            # Irregular stagger breaks horizontal brick-course alignments.
            x=float(x0+rng.uniform(-2,2)+(1.5 if layer==1 else 0))
            y=float(y0+rng.uniform(-3.2,3.2))
            p=np.array([x,y,float(sample_height(data,x,y,args.row,args.col))-z_origin])
            strike=normalize((1.,rng.uniform(-.13,.13),rng.uniform(-.17,.13)))
            up=normalize((rng.uniform(-.13,.13),.55+rng.uniform(-.10,.10),.84))
            up=normalize(up-strike*np.dot(strike,up));outward=normalize(np.cross(strike,up))
            width=float(rng.uniform(15,21));height=float(rng.uniform(14,20));depth=float(rng.uniform(2.5,5.5))
            polygon=np.array([[-.52,-.22],[-.33,.42],[.17,.53],[.51,.19],[.37,-.43],[-.20,-.51]])
            polygon+=rng.uniform(-.07,.07,polygon.shape)
            uv=polygon*np.array([width,height])
            near=p+uv[:,0,None]*strike+uv[:,1,None]*up+outward*8.
            # Back corners differ in depth and shoulder reach. Convex hull
            # creates real intersecting fracture planes and tapered endings.
            back_uv=uv*rng.uniform(.80,1.12,(len(uv),1))
            back=p+back_uv[:,0,None]*strike+back_uv[:,1,None]*up-outward*(depth+rng.uniform(-.7,.7,(len(uv),1)))
            cuts.append(dict(id=f'block_{layer}_{index}',family='finite_wedge_detachment',
                             points=np.concatenate((near,back)).tolist(),center=p.tolist(),
                             nominal_face_extent_m=[width,height],normal_recession_m=depth,
                             strike=strike.tolist(),up_dip=up.tolist()))
    return cuts


def apply_boolean(target,cutter,operation):
    import bpy
    bpy.context.view_layer.objects.active=target
    target.select_set(True)
    mod=target.modifiers.new('Exact'+operation,'BOOLEAN')
    mod.operation=operation;mod.solver='EXACT';mod.object=cutter
    bpy.ops.object.modifier_apply(modifier=mod.name)
    target.select_set(False)


def arrays(obj):
    obj.data.calc_loop_triangles()
    vertices=np.array([tuple(v.co) for v in obj.data.vertices],np.float64)
    triangles=np.array([tuple(t.vertices) for t in obj.data.loop_triangles],np.int32)
    return vertices,triangles


def clean_boolean_mesh(obj):
    """Remove exact-boolean coincident/collinear slivers, not visible rock detail."""
    import bmesh
    bm=bmesh.new();bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1.e-5)
    bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1.e-5)
    bmesh.ops.triangulate(bm,faces=list(bm.faces),quad_method='BEAUTY',ngon_method='BEAUTY')
    bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1.e-5)
    bmesh.ops.triangulate(bm,faces=list(bm.faces),quad_method='BEAUTY',ngon_method='BEAUTY')
    bm.normal_update();bm.to_mesh(obj.data);bm.free();obj.data.update()


def mesh_metrics(vertices,triangles):
    a,b,c=[vertices[triangles[:,i]] for i in range(3)]
    cross=np.cross(b-a,c-a)
    area=.5*np.linalg.norm(cross,axis=1)
    edges=np.sort(np.concatenate((triangles[:,[0,1]],triangles[:,[1,2]],triangles[:,[2,0]])),axis=1)
    _,counts=np.unique(edges,axis=0,return_counts=True)
    return dict(vertices=len(vertices),triangles=len(triangles),finite=bool(np.isfinite(vertices).all()),
                boundary_edges=int(np.count_nonzero(counts==1)),nonmanifold_edges=int(np.count_nonzero(counts!=2)),
                degenerate_triangles=int(np.count_nonzero(area<1.e-10)),minimum_triangle_area_m2=float(area.min()),
                signed_volume_m3=float(np.sum(np.einsum('ij,ij->i',a,np.cross(b,c)))/6.),
                bounds_min_m=vertices.min(axis=0).tolist(),bounds_max_m=vertices.max(axis=0).tolist())


def ray_and_boundary_checks(obj,baseline_top,perimeter,data,row,col,z_origin,width,depth,guard):
    import bpy
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    depsgraph=bpy.context.evaluated_depsgraph_get();tree=BVHTree.FromObject(obj,depsgraph)
    vertices,triangles=arrays(obj)
    zmax=float(vertices[:,2].max()+2);zmin=float(vertices[:,2].min()-2)
    examples=[];counts={};overhang_rays=0
    for y in np.arange(-depth/2+guard+1,depth/2-guard,1.):
        for x in np.arange(-width/2+guard+1,width/2-guard,1.):
            hits=[];origin=Vector((float(x),float(y),zmax))
            for _ in range(20):
                point,normal,index,distance=tree.ray_cast(origin,Vector((0,0,-1)),zmax-zmin)
                if point is None or point.z<zmin:break
                if not hits or abs(point.z-hits[-1][0])>.001:hits.append((float(point.z),tuple(normal)))
                origin=point+Vector((0,0,-.0002))
            counts[len(hits)]=counts.get(len(hits),0)+1
            if len(hits)>=4:
                overhang_rays+=1
                if len(examples)<12:examples.append(dict(local_xy_m=[float(x),float(y)],hit_z_m=[p[0] for p in hits],normals=[p[1] for p in hits]))
    errors=[tree.find_nearest(Vector(tuple(baseline_top[i])))[3] for i in perimeter]
    a,b,c=[vertices[triangles[:,i]] for i in range(3)]
    n=np.cross(b-a,c-a); n/=np.maximum(np.linalg.norm(n,axis=1)[:,None],1.e-15)
    centers=(a+b+c)/3
    inside=(np.abs(centers[:,0])<width/2-guard*.5)&(np.abs(centers[:,1])<depth/2-guard*.5)
    # Exclude the deliberate back cap, whose down normals are not overhangs.
    source_z=sample_height(data,np.clip(centers[:,0],-width/2,width/2),np.clip(centers[:,1],-depth/2,depth/2),row,col)-z_origin
    roofs=inside&(n[:,2]<-.2)&(centers[:,2]>source_z-18.)
    return dict(vertical_ray_hit_histogram=counts,multiple_surface_vertical_rays=overhang_rays,
                multiple_surface_examples=examples,interior_downward_roof_triangles=int(roofs.sum()),
                maximum_crop_boundary_distance_m=float(max(errors)),ray_step_m=1.)


def write_npz(path,obj,origin,basis,metadata):
    local,triangles=arrays(obj)
    world=origin+local@basis.T
    np.savez_compressed(path,vertices_m=world,triangles=triangles,
                        frame_origin_world_m=origin,basis_u_world=basis[:,0],basis_v_world=basis[:,1],basis_z_world=basis[:,2],
                        metadata_json=np.asarray(json.dumps(metadata)))


def write_ply(path,obj,origin,basis):
    local,triangles=arrays(obj);world=origin+local@basis.T
    with path.open('w',encoding='ascii') as out:
        out.write('ply\nformat ascii 1.0\ncomment arena XYZ metres; original triangle mesh, not heightfield\n')
        out.write(f'element vertex {len(world)}\nproperty double x\nproperty double y\nproperty double z\nelement face {len(triangles)}\nproperty list uchar int vertex_indices\nend_header\n')
        for p in world:out.write(f'{p[0]:.9f} {p[1]:.9f} {p[2]:.9f}\n')
        for t in triangles:out.write(f'3 {t[0]} {t[1]} {t[2]}\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--row',type=int,default=89);parser.add_argument('--col',type=int,default=64)
    parser.add_argument('--width-m',type=float,default=72.);parser.add_argument('--depth-m',type=float,default=40.)
    parser.add_argument('--spacing-m',type=float,default=.5)
    parser.add_argument('--seed',type=int,default=81515)
    parser.add_argument('--mode',choices=('wedges','slots'),default='wedges',help='Slots retained only to reproduce the rejected saw-kerf diagnostic')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.output_dir.exists() or 'saved' not in [p.lower() for p in args.output_dir.resolve().parts]:
        parser.error('Choose a new directory under ignored Saved; earlier attempts stay immutable')
    if args.spacing_m<.25 or args.spacing_m>1. or args.width_m<40 or args.depth_m<24:
        parser.error('Use0.25..1m sampling,40m+ width and24m+ depth')
    import bpy
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    args.output_dir.mkdir(parents=True)
    with np.load(args.input,allow_pickle=False) as archive:data={k:archive[k].copy() for k in archive.files}
    guard=4.; backing=24.; skin=12.
    crop=validate_crop(data,args.row,args.col,args.width_m,args.depth_m,guard)
    xy=data['world_xy_m'][args.row,args.col];z_origin=float(data['height_m'][args.row,args.col])
    origin=np.array([xy[0],xy[1],z_origin]);basis=np.column_stack((data['basis_u_world'],data['basis_v_world'],data['basis_up_world']))
    baseline,top,perimeter=closed_shell(data,args.row,args.col,args.width_m,args.depth_m,args.spacing_m,z_origin,backing)
    candidate=baseline.copy();candidate.data=baseline.data.copy();candidate.name='Fractured';bpy.context.collection.objects.link(candidate)
    baseline.hide_render=True;baseline.hide_set(True)
    clipper,_,_=closed_shell(data,args.row,args.col,args.width_m-2*guard,args.depth_m-2*guard,2.,z_origin,skin,4.,'CutGuard')
    clipper.hide_render=True
    rng=np.random.default_rng(args.seed); cuts=[]
    # Joint set1: six unequal, finite, steeply dipping fractures. Spacing is
    # 8..12m, not an endless Cartesian masonry grid. Each has a different reach.
    for i,u in enumerate((-25.,-14.5,-5.,5.5,17.,26.)):
        x=u+rng.uniform(-.7,.7); y=float(rng.uniform(-7,8))
        tilt=float(rng.uniform(-.20,.20))
        down=normalize((tilt,.10,-1.))
        into=normalize((0.,1.,-.15));into=normalize(into-down*np.dot(into,down))
        point=np.array([x,y,float(sample_height(data,x,y,args.row,args.col))-z_origin])
        length=float(rng.uniform(25,39));reach=float(rng.uniform(14,19));thickness=float(rng.uniform(.35,.85))
        center=point+into*3.5+down*rng.uniform(-2,3)
        cuts.append(dict(id=f'joint_{i}',family='steep_joint',center=center.tolist(),axis_a=down.tolist(),axis_b=into.tolist(),dimensions=[length,reach,thickness]))
    # Joint set2: short oblique bedding recesses. Entering beneath the face
    # creates downward-facing roofs and multiple vertical ray intersections.
    for i,(x,y) in enumerate(((-22.,-9.),(-12.,5.),(4.,-5.),(21.,8.),(22.,-8.),(-26.,9.))):
        x+=float(rng.uniform(-1,1));y+=float(rng.uniform(-1,1))
        strike=normalize((1.,rng.uniform(-.12,.12),rng.uniform(-.18,.18)))
        into=normalize((0.,1.,rng.uniform(-.28,-.08)));into=normalize(into-strike*np.dot(into,strike))
        point=np.array([x,y,float(sample_height(data,x,y,args.row,args.col))-z_origin])
        cuts.append(dict(id=f'bedding_{i}',family='oblique_undercut',center=(point+into*3.).tolist(),axis_a=strike.tolist(),axis_b=into.tolist(),
                         dimensions=[float(rng.uniform(13,23)),float(rng.uniform(13,18)),float(rng.uniform(.55,1.15))]))
    if args.mode=='wedges':
        cuts=detachment_specs(data,args,z_origin,np.random.default_rng(args.seed))
    timings=[]
    for spec in cuts:
        start=time.monotonic()
        if 'points' in spec:cutter=wedge(spec['id'],spec['points'])
        else:cutter,_=box(spec['id'],spec['center'],spec['axis_a'],spec['axis_b'],spec['dimensions'])
        apply_boolean(cutter,clipper,'INTERSECT')
        if len(cutter.data.polygons):apply_boolean(candidate,cutter,'DIFFERENCE')
        elapsed=time.monotonic()-start;timings.append(elapsed)
        bpy.data.objects.remove(cutter,do_unlink=True)
        print(f"CUT {spec['id']}: {len(candidate.data.polygons)} polygons, {elapsed:.2f}s",flush=True)
        if elapsed>45:
            bpy.ops.wm.save_as_mainfile(filepath=str((args.output_dir/'failed_slow_boolean.blend').resolve()))
            raise RuntimeError('Boolean exceeded bounded45s per-cut budget; retained diagnostic blend')
    bpy.data.objects.remove(clipper,do_unlink=True)
    clean_boolean_mesh(candidate)
    before=mesh_metrics(*arrays(baseline));after=mesh_metrics(*arrays(candidate))
    checks=ray_and_boundary_checks(candidate,top,perimeter,data,args.row,args.col,z_origin,args.width_m,args.depth_m,guard)
    metadata=dict(schema='directorscut.fractured_cliff_outcrop.v1',source_kind='true_3d_fractured_outcrop',
                  physical_units='absolute arena/design-world XYZ metres; no vertical exaggeration',coordinate_frame='arena_local_metres',
                  source_npz=str(args.input.resolve()),source_sha256=digest(args.input),generator_source_sha256=digest(__file__),
                  source_metadata=json.loads(str(data['metadata_json'])),seed=args.seed,crop=crop,
                  fracture_mode=args.mode,
                  front_sample_spacing_m=args.spacing_m,backing_depth_m=backing,maximum_cut_skin_depth_m=skin,
                  boolean_cleanup_tolerance_m=1.e-5,
                  closed_volume=True,cutters=cuts,cut_seconds=timings,baseline_metrics=before,candidate_metrics=after,
                  geometry_checks=checks,blender_version=bpy.app.version_string,
                  limitations='Isolated rock prototype. Artificial closed crop/backing retained for booleans; not live terrain or a physically calibrated fracture simulation. Do not project to heightfield. No UV/material/erosion masks claimed.')
    for label,obj in [('baseline',baseline),('fractured',candidate)]:
        meta=dict(metadata,variant=label)
        write_npz(args.output_dir/(label+'.npz'),obj,origin,basis,meta)
        write_ply(args.output_dir/(label+'.ply'),obj,origin,basis)
    baseline.hide_set(True);candidate.hide_set(False)
    bpy.context.view_layer.objects.active=candidate;candidate.select_set(True)
    bpy.ops.wm.save_as_mainfile(filepath=str((args.output_dir/'outcrop.blend').resolve()))
    (args.output_dir/'outcrop.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(output=str(args.output_dir.resolve()),before=before,after=after,checks=checks),indent=2))
    if after['nonmanifold_edges'] or after['degenerate_triangles'] or not after['finite'] or after['signed_volume_m3']<=0:
        raise RuntimeError('Retained failed candidate: closed original triangle mesh checks failed')
    if checks['maximum_crop_boundary_distance_m']>.001 or checks['multiple_surface_vertical_rays']==0:
        raise RuntimeError('Retained failed candidate: crop continuity or actual overhang proof failed')


if __name__=='__main__':main()
