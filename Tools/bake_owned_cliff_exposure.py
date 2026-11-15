"""Derive restrained stone-color exposure from the actual shared relief field.

No new geometry, AO, normal or roughness noise. Selects the low-chroma palette
present in the owned scan and uses the geometry worker's exact intermediate
relief function to vary exposure. Original fine luminance detail is retained.
"""
import argparse,ast,hashlib,json,shutil,struct,time
from pathlib import Path
import numpy as np

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--geometry-source',type=Path,required=True)
args=parser.parse_args();started=time.time()
src=args.source.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
manifest=json.loads((src/'tile_manifest.json').read_text())
def read(path):
    b=path.read_bytes();w,h=struct.unpack_from('<HH',b,12)
    return np.frombuffer(b,np.uint8,offset=18).reshape(h,w,3)[:,:,::-1].astype(np.float32)/255.
def linear(a):return np.where(a<=.04045,a/12.92,((a+.055)/1.055)**2.4)
palette_path=Path(manifest['source_tile']).parent/'T_NordicFace_BaseColor.tga'
palette=read(palette_path);y=(palette*np.array([.2126,.7152,.0722])).sum(2)
chroma=(palette.max(2)-palette.min(2))/np.maximum(palette.max(2),.001)
selected=(chroma<.14)&(y>.3)&(y<.8)
if selected.sum()<100:raise RuntimeError('Insufficient measured neutral stone population')
fresh_rgb=np.median(palette[selected],axis=0);fresh=linear(fresh_rgb)
fresh/=float(fresh@np.array([.2126,.7152,.0722]))
# Evaluate only these reviewed pure helpers, not the other worker's generator.
tree=ast.parse(args.geometry_source.read_text())
helpers=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in ('lookup','intermediate_relief')]
if len(helpers)!=2:raise RuntimeError('Expected exact shared lookup/intermediate_relief helpers')
scope={'np':np};exec(compile(ast.Module(body=helpers,type_ignores=[]),str(args.geometry_source),'exec'),scope)
depth=np.load(src/'tile.npz')['depth_m'].astype(np.float64);H,W=depth.shape
width=float(manifest['physical_width_m']);height=float(manifest['physical_height_m']);zmin=float(manifest['z_min_m'])
fy=np.fft.fftfreq(H,height/H);fx=np.fft.fftfreq(W,width/W)
kernel=np.exp(-2*np.pi*np.pi*.35**2*(fy[:,None]**2+fx[None,:]**2))
tile=np.fft.ifft2(np.fft.fft2(depth)*kernel).real;tile-=tile.mean()
color=linear(read(src/'T_NordicFace_BaseColor.tga'));ranges=[]
for row in range(0,H,64):
    last=min(row+64,H);u,z=np.meshgrid((np.arange(W)+.5)*width/W,zmin+(np.arange(row,last)+.5)*height/H)
    relief=scope['intermediate_relief'](tile,u.ravel(),z.ravel()).reshape(last-row,W)
    exposure=np.clip((relief+1.)/6.,0,1);exposure=exposure*exposure*(3-2*exposure)
    # Edge restraint follows the pilot's angular collar, without painting AO.
    collar=np.clip(np.minimum(u,width-u)/28.,0,1);collar=collar*collar*(3-2*collar)
    exposure*=collar
    a=.78*exposure[...,None]
    original=color[row:last];luma=(original*np.array([.2126,.7152,.0722])).sum(2)[...,None]
    color[row:last]=(original*(1-a)+luma*fresh*a)*(1+.15*exposure[...,None])
    ranges.append((float(relief.min()),float(relief.max()),float(exposure.mean())))
color=np.where(color<=.0031308,color*12.92,1.055*np.maximum(color,0)**(1/2.4)-.055)
data=np.clip(np.rint(color*255),0,255).astype(np.uint8)[:,:,::-1]
(out/'T_NordicFace_BaseColor.tga').write_bytes(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,W,H,24,0)+data.tobytes())
for name in ['tile.npz','T_NordicFace_NormalGL.tga','T_NordicFace_Roughness.tga']:shutil.copyfile(src/name,out/name)
manifest['exposure_color']=dict(source_palette_path=str(palette_path),selected_pixels=int(selected.sum()),measured_neutral_srgb=fresh_rgb.tolist(),geometry_source=str(args.geometry_source.resolve()),geometry_source_sha256=hashlib.sha256(args.geometry_source.read_bytes()).hexdigest(),expression='intermediate_relief of exact .35m-filtered source depth; smoothstep((-1..5)m), angular28m collar',max_neutral_palette_blend=.78,max_linear_luminance_multiplier=1.15,ambient_occlusion_added=False,normal_roughness_geometry_unchanged=True,relief_range=[min(r[0] for r in ranges),max(r[1] for r in ranges)],mean_exposure=float(np.mean([r[2] for r in ranges])),generator_source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
manifest['files']={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ['tile.npz','T_NordicFace_BaseColor.tga','T_NordicFace_Roughness.tga','T_NordicFace_NormalGL.tga']}
(out/'tile_manifest.json').write_text(json.dumps(manifest,indent=2))
print(json.dumps({'output':str(out),'seconds':time.time()-started,'exposure':manifest['exposure_color']}))
