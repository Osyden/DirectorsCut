"""Bake one unique full-pilot PBR/depth atlas from a measured owned scan tile.

CPU NumPy only. Inputs and outputs must remain outside public Git; this file
contains no vendor pixels or meshes. The source tile is an explicit measured
dependency with its physical width/height in tile_manifest.json. No enlargement
of source features: only overlapping phase/rotation and bounded coordinate warp.
"""
from pathlib import Path
import argparse
import numpy as np,json,struct,hashlib,time
started=time.time()
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,required=True,help='Measured tile directory containing manifest, depth NPZ and three TGA maps')
parser.add_argument('--output',type=Path,required=True,help='Ignored output directory')
args=parser.parse_args()
source=args.source.resolve()
out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
old=json.loads((source/'tile_manifest.json').read_text())
W,H=4096,2048;physical_width=497.418836818384;physical_height=280.;zmin=-200.
tile=np.load(source/'tile.npz')['depth_m']; tw=float(old['physical_width_m']);th=float(old['physical_height_m'])
def read(name):
 b=(source/name).read_bytes();w,h=struct.unpack_from('<HH',b,12)
 return np.frombuffer(b,np.uint8,offset=18).reshape(h,w,3)[:,:,::-1].astype(np.float32)/255.
albedo=read('T_NordicFace_BaseColor.tga');albedo=np.where(albedo<=.04045,albedo/12.92,((albedo+.055)/1.055)**2.4)
rough=read('T_NordicFace_Roughness.tga')[:,:,0];normal=read('T_NordicFace_NormalGL.tga')*2-1
outcolor=np.zeros((H,W,3),np.float32);outnormal=np.zeros_like(outcolor);outrough=np.zeros((H,W),np.float32);outdepth=np.zeros((H,W),np.float32)
def hashed(i,j,k):
 q=(i.astype(np.uint64)*np.uint64(374761393)+j.astype(np.uint64)*np.uint64(668265263)+np.uint64(k)*np.uint64(2246822519))&np.uint64(0xffffffff)
 q=(q^(q>>np.uint64(13)))*np.uint64(1274126177)&np.uint64(0xffffffff)
 return ((q^(q>>np.uint64(16)))&np.uint64(0xffffff)).astype(np.float32)/16777216.
def sample(a,u,v):
 x=np.mod(u,1)*a.shape[1]-.5;y=np.mod(v,1)*a.shape[0]-.5
 ix=np.floor(x).astype(np.int32);iy=np.floor(y).astype(np.int32);fx=x-ix;fy=y-iy
 if a.ndim==3:fx=fx[...,None];fy=fy[...,None]
 return (a[iy%a.shape[0],ix%a.shape[1]]*(1-fx)+a[iy%a.shape[0],(ix+1)%a.shape[1]]*fx)*(1-fy)+(a[(iy+1)%a.shape[0],ix%a.shape[1]]*(1-fx)+a[(iy+1)%a.shape[0],(ix+1)%a.shape[1]]*fx)*fy
for row in range(0,H,64):
 last=min(row+64,H)
 u,z=np.meshgrid((np.arange(W)+.5)*physical_width/W,zmin+(np.arange(row,last)+.5)*physical_height/H)
 # Smooth bounded authoring warp changes scan lookup, never source tile size.
 pu=u+1.7*np.sin(2*np.pi*(z/67+u/133))+1.1*np.sin(2*np.pi*(u/51-z/97))
 pz=z+1.2*np.sin(2*np.pi*(u/83+z/149))
 qx=pu/19.7;qz=pz/19.7;ix=np.floor(qx).astype(np.int64);iz=np.floor(qz).astype(np.int64);fx=qx-ix;fz=qz-iz;upper=fx+fz>1
 cells=[(ix+upper,iz+upper,np.where(upper,fx+fz-1,1-fx-fz)),(ix+1,iz,np.where(upper,1-fz,fx)),(ix,iz+1,np.where(upper,1-fx,fz))]
 weights=[np.maximum(c[2],0)**4 for c in cells];total=sum(weights)
 for (cx,cz,_),weight in zip(cells,weights):
  weight/=total
  theta=(hashed(cx,cz,3)-.5)*.52;c=np.cos(theta);s=np.sin(theta)
  su=(c*pu-s*pz)/tw+hashed(cx,cz,7);sv=(s*pu+c*pz)/th+hashed(cx,cz,11)
  outcolor[row:last]+=sample(albedo,su,sv)*weight[...,None]
  outrough[row:last]+=sample(rough,su,sv)*weight
  outdepth[row:last]+=sample(tile,su,sv)*weight*.19
  n=sample(normal,su,sv);nx=n[:,:,0].copy();nz=n[:,:,1].copy()
  n[:,:,0]=c*nx+s*nz;n[:,:,1]=-s*nx+c*nz
  outnormal[row:last]+=n*weight[...,None]
outdepth-=outdepth.mean()
outnormal/=np.maximum(np.linalg.norm(outnormal,axis=2)[...,None],1e-12)
outcolor=np.where(outcolor<=.0031308,outcolor*12.92,1.055*np.maximum(outcolor,0)**(1/2.4)-.055)
def tga(name,a):
 if a.ndim==2:a=np.repeat(a[:,:,None],3,axis=2)
 data=np.clip(np.rint(a*255),0,255).astype(np.uint8)[:,:,::-1]
 (out/name).write_bytes(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,W,H,24,0)+data.tobytes())
tga('T_NordicFace_BaseColor.tga',outcolor);tga('T_NordicFace_Roughness.tga',outrough);tga('T_NordicFace_NormalGL.tga',outnormal*.5+.5)
manifest=dict(old)
manifest.update(schema='directorscut.unique_owned_face.v1',physical_width_m=physical_width,physical_height_m=physical_height,z_min_m=zmin,z_max_m=80.,resolution=[W,H],geometry_scale=.19,depth_min_m=float(outdepth.min()),depth_max_m=float(outdepth.max()),depth_mean_removed_m=0.,source_tile=str(source/'tile.npz'),source_tile_sha256=hashlib.sha256((source/'tile.npz').read_bytes()).hexdigest(),unique_pilot=True,sampler=dict(physical_scan_size=[tw,th],stochastic_grid_m=19.7,three_way_weights_power=4,rotation_degrees_max=14.8969,source_relief_multiplier=.19,warp_amplitudes_m=[1.7,1.1,1.2],warp_definition='exact source script; applied to maps/depth together'),uv='Unique U/497.418836818384, (Z+200)/280; positiveVup. Depth texel centres; clamp unique field, no periodic8m lookup',normal_format='OpenGL projected fine-source normal rotated with each sample; geometry relief separately reduced',seconds=time.time()-started)
np.savez_compressed(out/'tile.npz',depth_m=outdepth,physical_width_m=physical_width,physical_height_m=physical_height,z_min_m=zmin,valid=np.ones((H,W),bool),metadata_json=json.dumps(manifest))
manifest['generator_source_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
manifest['files']={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ['tile.npz','T_NordicFace_BaseColor.tga','T_NordicFace_Roughness.tga','T_NordicFace_NormalGL.tga']}
(out/'tile_manifest.json').write_text(json.dumps(manifest,indent=2))
print('UNIQUE_READY '+json.dumps({k:manifest[k] for k in ['physical_width_m','physical_height_m','depth_min_m','depth_max_m','seconds']}),flush=True)
