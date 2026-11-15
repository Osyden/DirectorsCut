"""CPU Blender: exact frontmost depth/UV fields from the trimmed owned scans.

Run after trim_owned_cliff_faces.py with the same DC_OWNED_CLIFF_WORK directory.
These are actual scan rays in metres, not artist-created rock shapes.
"""
import bpy, numpy as np, json, time, os
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from pathlib import Path

WORK=Path(os.environ.get('DC_OWNED_CLIFF_WORK',str(Path(__file__).resolve().parents[1]/'Saved'/'OwnedScanSources')))
ROOT=WORK/'OwnedCliffProjection'
TRIM=WORK/'OwnedCliffTrim'
for asset_id in ('vfmjadufa','vefkehbva'):
    started=time.time(); out=ROOT/asset_id; out.mkdir(parents=True,exist_ok=True)
    report=json.loads((TRIM/asset_id/'trim_report.json').read_text())
    source=Path(report['derivative_fbx'])
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(source),use_custom_normals=True)
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert len(meshes)==1
    obj=meshes[0]; mesh=obj.data; mesh.transform(obj.matrix_world); obj.matrix_world.identity(); bpy.context.view_layer.update()
    if any(len(p.vertices)!=3 for p in mesh.polygons):
        bpy.context.view_layer.objects.active=obj
        mod=obj.modifiers.new('ProjectionTriangulation','TRIANGULATE')
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mesh=obj.data
    points=np.empty(len(mesh.vertices)*3,np.float64); mesh.vertices.foreach_get('co',points); points=points.reshape(-1,3)
    indices=np.empty(len(mesh.loops),np.int32); mesh.loops.foreach_get('vertex_index',indices); indices=indices.reshape(-1,3)
    uv=np.empty(len(mesh.loops)*2,np.float32); mesh.uv_layers[0].data.foreach_get('uv',uv); uv=uv.reshape(-1,3,2)
    lo=points.min(0); hi=points.max(0); step=.15
    x=np.arange(np.ceil(lo[0]/step)*step,hi[0]+1e-6,step); z=np.arange(np.ceil(lo[2]/step)*step,hi[2]+1e-6,step)
    hits=np.zeros((len(z),len(x),3)); normals=np.zeros_like(hits); faces=np.full((len(z),len(x)),-1,np.int32)
    tree=BVHTree.FromObject(obj,bpy.context.evaluated_depsgraph_get()); direction=Vector((0,1,0))
    for iz,zz in enumerate(z):
        for ix,xx in enumerate(x):
            p,n,f,d=tree.ray_cast(Vector((float(xx),float(lo[1]-2),float(zz))),direction,float(hi[1]-lo[1]+4))
            if f is not None:
                hits[iz,ix]=p; normals[iz,ix]=n; faces[iz,ix]=f
    valid=faces>=0
    tri=points[indices[faces[valid]]]; p=hits[valid]; a=tri[:,1]-tri[:,0]; b=tri[:,2]-tri[:,0]; q=p-tri[:,0]
    aa=np.einsum('ij,ij->i',a,a); ab=np.einsum('ij,ij->i',a,b); bb=np.einsum('ij,ij->i',b,b); aq=np.einsum('ij,ij->i',a,q); bq=np.einsum('ij,ij->i',b,q)
    denom=aa*bb-ab*ab; wa=(bb*aq-ab*bq)/denom; wb=(aa*bq-ab*aq)/denom
    bary=np.column_stack((1-wa-wb,wa,wb)); mapped=np.zeros((len(z),len(x),2),np.float32); mapped[valid]=np.einsum('ij,ijk->ik',bary,uv[faces[valid]])
    depth=-hits[:,:,1]; depth[~valid]=0.; normals[:,:,1]*=-1
    source_raw=Path(report['source_fbx']); prefix=source_raw.stem+'_8K_'
    textures={k:str(source_raw.parent/(prefix+suffix+'.jpg')) for k,suffix in [('albedo','BaseColor'),('normal','Normal'),('roughness','Roughness')]}
    metadata=dict(asset_id=asset_id,source_fbx=str(source_raw),projection_mesh=str(source),units='metres',axes='UE X,+Y front,Z up',step_m=step,bottom22percent_removed=True,frontmost_ray=True,textures=textures,normal_format='DirectX source texture; normal_ue stores actual mesh geometric normals',valid_samples=int(valid.sum()),all_samples=int(valid.size),source_bounds_min_blender_m=lo.tolist(),source_bounds_max_blender_m=hi.tolist())
    np.savez_compressed(out/'face_field.npz',x_m=x,z_m=z,depth_m=depth,valid=valid,uv0=mapped,normal_ue=normals,source_face=faces,metadata_json=json.dumps(metadata))
    (out/'face_field.json').write_text(json.dumps(metadata,indent=2)); print('FIELD_READY '+str(out/'face_field.npz')+' '+str(time.time()-started),flush=True)
