"""CPU Blender: measured Massive Nordic face -> registered PBR/depth tile.

Run after project_owned_cliff_faces.py with the same DC_OWNED_CLIFF_WORK.
Uses original owned 8K maps, actual source UVs and an explicit native-size
rectangle; derives pixels only under ignored Saved. No vendor assets in Git.
"""
import bpy, numpy as np, json, struct, hashlib, time, os
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
started=time.time()
work=Path(os.environ.get('DC_OWNED_CLIFF_WORK',str(Path(__file__).resolve().parents[1]/'Saved'/'OwnedScanSources')))
root=work/'OwnedCliffProjection'/'vfmjadufa'
metadata=json.loads((root/'face_field.json').read_text())
out=root/'Tile'; out.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=metadata['projection_mesh'],use_custom_normals=True)
obj=next(o for o in bpy.context.scene.objects if o.type=='MESH'); mesh=obj.data
mesh.transform(obj.matrix_world); obj.matrix_world.identity(); bpy.context.view_layer.update()
p=np.empty(len(mesh.vertices)*3); mesh.vertices.foreach_get('co',p); p=p.reshape(-1,3)
indices=np.empty(len(mesh.loops),np.int32);mesh.loops.foreach_get('vertex_index',indices);indices=indices.reshape(-1,3)
uv=np.empty(len(mesh.loops)*2,np.float32);mesh.uv_layers[0].data.foreach_get('uv',uv);uv=uv.reshape(-1,3,2)
tree=BVHTree.FromObject(obj,bpy.context.evaluated_depsgraph_get())
N=512; x0,z0=-7.2,3.; width,height=10.65,7.35; pw,ph=width*.75,height*.75
# Sample the actual source rectangle at texel centres, not interpolated mesh UVs.
x=x0+(np.arange(N)+.5)*width/N; z=z0+(np.arange(N)+.5)*height/N
hits=np.zeros((N,N,3)); normals=np.zeros_like(hits); faces=np.full((N,N),-1,np.int32)
for iz,zz in enumerate(z):
    for ix,xx in enumerate(x):
        q,n,f,d=tree.ray_cast(Vector((float(xx),-8.,float(zz))),Vector((0,1,0)),20.)
        if f is not None: hits[iz,ix]=q;normals[iz,ix]=n;faces[iz,ix]=f
valid=faces>=0
if not valid.all(): raise RuntimeError('Chosen face rectangle contains holes: '+str((~valid).sum()))
tri=p[indices[faces.ravel()]];q=hits.reshape(-1,3)-tri[:,0];a=tri[:,1]-tri[:,0];b=tri[:,2]-tri[:,0]
aa=(a*a).sum(1);ab=(a*b).sum(1);bb=(b*b).sum(1);aq=(a*q).sum(1);bq=(b*q).sum(1);denom=aa*bb-ab*ab
wa=(bb*aq-ab*bq)/denom;wb=(aa*bq-ab*aq)/denom;bary=np.column_stack((1-wa-wb,wa,wb))
mapped=np.einsum('ij,ijk->ik',bary,uv[faces.ravel()]).reshape(N,N,2)
def image_sample(path):
    im=bpy.data.images.load(path,check_existing=False); im.colorspace_settings.name='Non-Color'
    original=list(im.size)
    if max(im.size)>4096: im.scale(4096,4096)
    w,h=im.size;pixels=np.empty(w*h*4,np.float32);im.pixels.foreach_get(pixels);pixels=pixels.reshape(h,w,4)
    sx=np.clip(mapped[:,:,0]*w-.5,0,w-1);sy=np.clip(mapped[:,:,1]*h-.5,0,h-1)
    ix=np.floor(sx).astype(int);iy=np.floor(sy).astype(int);fx=(sx-ix)[...,None];fy=(sy-iy)[...,None]
    jx=np.minimum(ix+1,w-1);jy=np.minimum(iy+1,h-1)
    result=(pixels[iy,ix]*(1-fx)+pixels[iy,jx]*fx)*(1-fy)+(pixels[jy,ix]*(1-fx)+pixels[jy,jx]*fx)*fy
    bpy.data.images.remove(im)
    return result[:,:,:3],original
albedo,source_res=image_sample(metadata['textures']['albedo'])
rough,_=image_sample(metadata['textures']['roughness']);rough=rough[:,:,0]
tn,_=image_sample(metadata['textures']['normal']);tn=tn*2-1;tn[:,:,1]*=-1 # source DX to mathematical UV basis
duv1=uv[faces.ravel(),1]-uv[faces.ravel(),0];duv2=uv[faces.ravel(),2]-uv[faces.ravel(),0]
det=duv1[:,0]*duv2[:,1]-duv1[:,1]*duv2[:,0];safe=np.where(np.abs(det)>1e-12,det,1.)
T=(a*duv2[:,1,None]-b*duv1[:,1,None])/safe[:,None];B=(b*duv1[:,0,None]-a*duv2[:,0,None])/safe[:,None]
gn=normals.reshape(-1,3);T-=gn*(T*gn).sum(1)[:,None];T/=np.maximum(np.linalg.norm(T,axis=1)[:,None],1e-12)
hand=np.sign((np.cross(gn,T)*B).sum(1));B=np.cross(gn,T)*hand[:,None]
world=(T*tn[:,:,0].reshape(-1,1)+B*tn[:,:,1].reshape(-1,1)+gn*tn[:,:,2].reshape(-1,1));world/=np.maximum(np.linalg.norm(world,axis=1)[:,None],1e-12)
detail=world-gn*(world*gn).sum(1)[:,None]
detail=detail.reshape(N,N,3)
depth=-hits[:,:,1]
linear=np.where(albedo<=.04045,albedo/12.92,((albedo+.055)/1.055)**2.4)
def fetch(field,sx,sz):
    xx=np.clip(sx/width*N-.5,0,N-1);zz=np.clip(sz/height*N-.5,0,N-1)
    ix=np.floor(xx).astype(int);iz=np.floor(zz).astype(int);fx=xx-ix;fz=zz-iz
    if field.ndim==3:fx=fx[...,None];fz=fz[...,None]
    jx=np.minimum(ix+1,N-1);jz=np.minimum(iz+1,N-1)
    return (field[iz,ix]*(1-fx)+field[iz,jx]*fx)*(1-fz)+(field[jz,ix]*(1-fx)+field[jz,jx]*fx)*fz
u,v=np.meshgrid((np.arange(N)+.5)*pw/N,(np.arange(N)+.5)*ph/N)
def smooth(t):t=np.clip(t,0,1);return t*t*(3-2*t)
fields={'depth':depth,'albedo':linear,'roughness':rough,'detail':detail};acc={k:np.zeros_like(q) for k,q in fields.items()};total=np.zeros((N,N))
for ox in (0,pw):
    for oz in (0,ph):
        sx=u+ox;sz=v+oz
        w=smooth(sx/(width-pw))*smooth((width-sx)/(width-pw))*smooth(sz/(height-ph))*smooth((height-sz)/(height-ph))
        total+=w
        for k,q in fields.items():acc[k]+=fetch(q,sx,sz)*(w[...,None] if q.ndim==3 else w)
for k in acc:acc[k]/=total[...,None] if acc[k].ndim==3 else total
offset=float(acc['depth'].mean());d=acc['depth']-offset
# Fine normal-map perturbation only; the full projected scan depth is geometry.
dn=acc['detail'];normal=np.stack((dn[:,:,0],dn[:,:,2],np.sqrt(np.maximum(1-dn[:,:,0]**2-dn[:,:,2]**2,.01))),axis=-1);normal/=np.linalg.norm(normal,axis=-1)[...,None]
rgb=np.where(acc['albedo']<=.0031308,acc['albedo']*12.92,1.055*np.maximum(acc['albedo'],0)**(1/2.4)-.055)
def tga(name,array):
    if array.ndim==2:array=np.repeat(array[:,:,None],3,axis=2)
    data=np.clip(np.rint(array*255),0,255).astype(np.uint8)[:,:,::-1]
    # Bottom-left origin; source physical +Z corresponds to increasing image V.
    (out/name).write_bytes(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,N,N,24,0)+data.tobytes())
tga('T_NordicFace_BaseColor.tga',rgb);tga('T_NordicFace_Roughness.tga',acc['roughness']);tga('T_NordicFace_NormalGL.tga',normal*.5+.5)
manifest=dict(schema='directorscut.projected_owned_face.v1',source=metadata,physical_width_m=pw,physical_height_m=ph,source_rectangle_m=[x0,z0,width,height],source_overlap_fraction=.25,resolution=N,source_images_original_resolution=source_res,source_images_sample_resolution=4096,uv='u=physical wall arc / width; v=world Z / height; bottom-left atlas; all maps and depth share exact periodic four-projection overlap',depth_mean_removed_m=offset,depth_min_m=float(d.min()),depth_max_m=float(d.max()),normal_format='OpenGL tangent normal; fine source-map perturbation only, full scan shape in depth field',normal_uv_axes='U=sourceX, V=sourceZ, outward=source-BlenderY (UE+Y)',source_pixels_modified=False,derived_projection_pixels=True,geometry_scale=1.,roughness='original Nordic Raw roughness R; no invented values',seconds=time.time()-started)
np.savez_compressed(out/'tile.npz',depth_m=d.astype(np.float32),physical_width_m=pw,physical_height_m=ph,valid=np.ones((N,N),bool),metadata_json=json.dumps(manifest))
manifest['files']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file()}
(out/'tile_manifest.json').write_text(json.dumps(manifest,indent=2)); print('TILE_READY '+json.dumps(manifest),flush=True)
