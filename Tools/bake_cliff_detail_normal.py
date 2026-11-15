"""Bake native-scale measured scan microrelief plus its fine OpenGL normal.

No geometry or basecolor output. NumPy and the standard library only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def read_tga(path):
    data=path.read_bytes();h=struct.unpack('<BBBHHBHHHHBB',data[:18])
    if h[1]!=0 or h[2]!=2 or h[10] not in (24,32):
        raise ValueError('Expected uncompressed RGB/RGBA TGA')
    pixels=np.frombuffer(data,dtype=np.uint8,offset=18+h[0]).reshape(h[9],h[8],h[10]//8)
    rgb=pixels[:,:,:3][:,:,::-1].copy()
    if h[11]&32:rgb=rgb[::-1]
    if h[11]&16:rgb=rgb[:,::-1]
    return rgb


def normalize(n):return n/np.maximum(np.linalg.norm(n,axis=-1,keepdims=True),1e-12)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--blur-sigma-m',type=float,default=.8)
    p.add_argument('--relief-scale',type=float,default=.18)
    p.add_argument('--anti-alias-sigma-m',type=float,default=.06)
    a=p.parse_args()
    if a.output.exists():p.error('Use a new output folder; completed bakes are immutable')
    manifest=json.loads((a.source/'tile_manifest.json').read_text())
    with np.load(a.source/'tile.npz',allow_pickle=False) as f:
        depth=f['depth_m'].astype(float)
        width=float(f['physical_width_m']);height=float(f['physical_height_m'])
    fine=read_tga(a.source/'T_NordicFace_NormalGL.tga').astype(float)/127.5-1
    if fine.shape!=depth.shape+(3,):raise ValueError('Depth and source normal must share the exact tile grid')
    fine=normalize(fine)
    fy=np.fft.fftfreq(depth.shape[0],height/depth.shape[0])[:,None]
    fx=np.fft.fftfreq(depth.shape[1],width/depth.shape[1])[None,:]
    frequency2=fx*fx+fy*fy
    spectrum=np.fft.fft2(depth)
    broad=np.exp(-2*np.pi*np.pi*a.blur_sigma_m**2*frequency2)
    antialias=np.exp(-2*np.pi*np.pi*a.anti_alias_sigma_m**2*frequency2)
    residual_spectrum=spectrum*(1-broad)*antialias*a.relief_scale
    residual=np.fft.ifft2(residual_spectrum).real
    du=np.fft.ifft2(residual_spectrum*(2j*np.pi*fx)).real
    dv=np.fft.ifft2(residual_spectrum*(2j*np.pi*fy)).real
    measured=normalize(np.stack([-du,-dv,np.ones_like(du)],axis=-1))
    # Reoriented normal mapping: flat measured relief reproduces the fine map.
    t=measured+np.array([0.,0.,1.]);u=fine*np.array([-1.,-1.,1.])
    result=normalize(t*(np.sum(t*u,axis=-1)/np.maximum(t[:,:,2],1e-8))[:,:,None]-u)
    rgb=np.clip(np.rint((result*.5+.5)*255),0,255).astype(np.uint8)
    a.output.mkdir(parents=True)
    path=a.output/'T_NordicFace_NormalGL.tga'
    header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,depth.shape[1],depth.shape[0],24,0)
    path.write_bytes(header+rgb[:,:,::-1].tobytes())
    meta=dict(schema='directorscut.measured_cliff_detail_normal.v1',
        source_directory=str(a.source.resolve()),source_manifest_sha256=sha(a.source/'tile_manifest.json'),
        source_depth_sha256=sha(a.source/'tile.npz'),source_normal_sha256=sha(a.source/'T_NordicFace_NormalGL.tga'),
        physical_width_m=width,physical_height_m=height,resolution=[depth.shape[1],depth.shape[0]],
        blur_sigma_m=a.blur_sigma_m,anti_alias_sigma_m=a.anti_alias_sigma_m,relief_scale=a.relief_scale,
        filter='periodic Gaussian high-pass (1-lowpass), slight sampling filter, then physical spectral derivatives',
        normal_format='OpenGL tangent normal; RGB maps XYZ; U=scanX,V=scanZ; bottom-left origin, rows+V',
        blend='reoriented measured-height normal plus original fine source normal; no basecolor',
        residual_height_range_m=[float(residual.min()),float(residual.max())],
        measured_normal_angle_degrees=np.percentile(np.degrees(np.arccos(measured[:,:,2])),[50,90,99]).tolist(),
        output_normal_angle_degrees=np.percentile(np.degrees(np.arccos(np.clip(result[:,:,2],-1,1))),[50,90,99]).tolist(),
        source_physical_scale=1.,geometry_modified=False,basecolor_included=False,
        source=manifest.get('source'),files={path.name:sha(path)})
    (a.output/'tile_manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(json.dumps(meta,indent=2))


if __name__=='__main__':main()
