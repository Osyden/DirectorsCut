"""Bake original mountain-scale geology using the actual cliff UV surface fields.

No image tiles or vendor pixels. Geological bands, mineral regions and runoff
are authored once over the full surface. Geometry is read-only. Inputs are the
actual rasterized UV0 position/normal/slope/curvature masks, rows +V up.
NumPy and the standard library only; outputs retain the existing importer names.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import time
import zlib

import numpy as np


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def resize(a, shape):
    """Centre-aligned bilinear sampling with edge clamping, no periodic wrap."""
    h, w = shape
    x = np.clip((np.arange(w, dtype=np.float32)+.5)*a.shape[1]/w-.5, 0, a.shape[1]-1)
    y = np.clip((np.arange(h, dtype=np.float32)+.5)*a.shape[0]/h-.5, 0, a.shape[0]-1)
    ix = x.astype(np.int32); iy = y.astype(np.int32)
    jx = np.minimum(ix+1, a.shape[1]-1); jy = np.minimum(iy+1, a.shape[0]-1)
    fx = x-ix; fy = y-iy
    if a.ndim == 3:
        fx = fx[:, None]; fy = fy[:, None, None]
    else:
        fy = fy[:, None]
    return (((1-fx)*a[iy[:, None], ix]+fx*a[iy[:, None], jx])*(1-fy)
            + ((1-fx)*a[jy[:, None], ix]+fx*a[jy[:, None], jx])*fy).astype(np.float32)


def hash01(x, y, seed):
    with np.errstate(over='ignore'):
        q = x.astype(np.uint32)*np.uint32(374761393)+y.astype(np.uint32)*np.uint32(668265263)+np.uint32(seed*123457)
        q = (q^(q >> np.uint32(13)))*np.uint32(1274126177)
        return ((q^(q >> np.uint32(16))) & np.uint32(0xffffff)).astype(np.float32)/16777216.


def noise(x, y, sx, sy, seed):
    """Aperiodic smooth gradient noise in metres; not reused texture samples."""
    x=x/sx; y=y/sy; ix=np.floor(x).astype(np.int32); iy=np.floor(y).astype(np.int32)
    fx=x-ix; fy=y-iy; tx=fx*fx*fx*(fx*(fx*6-15)+10); ty=fy*fy*fy*(fy*(fy*6-15)+10)
    result=np.zeros_like(x, dtype=np.float32)
    for dx, dy in ((0,0),(1,0),(0,1),(1,1)):
        angle=hash01(ix+dx,iy+dy,seed)*np.float32(2*np.pi)
        dot=np.cos(angle)*(fx-dx)+np.sin(angle)*(fy-dy)
        result += dot*(tx if dx else 1-tx)*(ty if dy else 1-ty)
    return result*1.6


def smoothstep(a,b,x):
    q=np.clip((x-a)/(b-a),0,1)
    return q*q*(3-2*q)


def fractured_planes(x,y,sx,sy,seed):
    """Continuous angular fractured relief, with planar triangular interiors."""
    x=x/sx;y=y/sy;ix=np.floor(x).astype(np.int32);iy=np.floor(y).astype(np.int32)
    fx=x-ix;fy=y-iy
    a=hash01(ix,iy,seed)*2-1;b=hash01(ix+1,iy,seed)*2-1
    c=hash01(ix,iy+1,seed)*2-1;d=hash01(ix+1,iy+1,seed)*2-1
    return np.where(fx+fy<1,a*(1-fx-fy)+b*fx+c*fy,d*(fx+fy-1)+c*(1-fx)+b*(1-fy))


def write_tga(path,a):
    if a.ndim==2:a=np.repeat(a[:,:,None],3,axis=2)
    data=np.clip(np.rint(a*255),0,255).astype(np.uint8)[:,:,::-1]
    h,w=a.shape[:2]
    # Origin bit zero: first row is the bottom of the +V-up atlas.
    Path(path).write_bytes(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,w,h,24,0)+data.tobytes())


def write_png(path,a):
    a=np.clip(np.rint(a*255),0,255).astype(np.uint8)
    if a.ndim==2:a=np.repeat(a[:,:,None],3,axis=2)
    h,w=a.shape[:2]
    def chunk(tag, payload):return struct.pack('>I',len(payload))+tag+payload+struct.pack('>I',zlib.crc32(tag+payload)&0xffffffff)
    raw=b''.join(b'\0'+row.tobytes() for row in a)
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))


def runoff(fields):
    """Stable one-pass UV drainage with gravity projected onto actual triangles.

    Art-directed weathering coverage, not a calibrated hydrology simulation.
    The chart is monotone up the main face; roof receives diffuse rainfall.
    """
    position=fields['position_m'].astype(np.float32)
    valid=fields['valid'].astype(bool)
    normal=fields['normal'].astype(np.float32)
    size=(max(64,position.shape[0]//2), max(128,position.shape[1]//2))
    p=resize(position,size); n=resize(normal,size)
    n/=np.maximum(np.linalg.norm(n,axis=2)[...,None],1.e-6)
    keep=resize(valid.astype(np.float32),size)>.2
    # Solve for chart components of the gravity vector projected to the surface.
    dv,du=np.gradient(p,axis=(0,1)); gravity=n*n[:,:,2,None]; gravity[:,:,2]-=1
    uu=np.sum(du*du,axis=2); vv=np.sum(dv*dv,axis=2); uv=np.sum(du*dv,axis=2)
    gu=np.sum(gravity*du,axis=2); gv=np.sum(gravity*dv,axis=2)
    det=np.maximum(uu*vv-uv*uv,1.e-6)
    velocity_u=(gu*vv-gv*uv)/det; velocity_v=(gv*uu-gu*uv)/det
    drift=np.clip(velocity_u/np.maximum(-velocity_v,.05),-.7,.7)
    overlap=resize(fields.get('uv_overlap_mask',np.zeros(valid.shape,dtype=np.float32)),size)>.1
    drift[overlap]=0
    h,w=size; x=np.arange(w,dtype=np.float32); z=np.arange(h,dtype=np.float32)[:,None]
    # Nonperiodic catchments differ in spacing and strength; their paths then
    # follow the actual gravity/tangent field rather than straight UV stripes.
    rng=np.random.default_rng(571991)
    crown=np.zeros(w,dtype=np.float32)
    for c, width, amount in zip(rng.uniform(0,w,37),rng.uniform(.6,4.0,37),rng.uniform(.25,1.,37)):
        crown+=amount*np.exp(-.5*((x-c)/width)**2)
    starts=resize(fields.get('drainage_start_mask',np.zeros(valid.shape,dtype=np.float32)),size)
    starts[overlap]=0
    slope=resize(fields.get('slope_degrees',np.degrees(np.arccos(np.clip(normal[:,:,2],-1,1)))),size)
    flow=np.zeros(size,dtype=np.float32); previous=crown.copy()
    for row in range(h-1,-1,-1):
        sample=x-drift[row]; left=np.clip(np.floor(sample).astype(np.int32),0,w-1); right=np.minimum(left+1,w-1)
        f=np.clip(sample-left,0,1)
        previous=(previous[left]*(1-f)+previous[right]*f)*.996
        # Ledges collect and release a small amount; no painted cavity shadows.
        previous+=crown*(.003+starts[row]*.025)+np.maximum(0,1-slope[row]/75)*.001
        previous*=keep[row]
        flow[row]=previous
    scale=max(float(np.percentile(flow[keep],97)),1.e-4)
    return np.clip(flow/scale,0,1),drift


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fields',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--width',type=int,default=4096)
    ap.add_argument('--height',type=int,default=2048)
    args=ap.parse_args(); started=time.time(); out=args.output.resolve(); out.mkdir(parents=True,exist_ok=True)
    with np.load(args.fields,allow_pickle=False) as source:
        fields={k:source[k] for k in source.files}
    for k in ('position_m','normal','valid'):
        if k not in fields:raise ValueError('Missing actual geometry field: '+k)
    for k in ('position_m','normal'):
        fields[k]=np.nan_to_num(fields[k],nan=0,posinf=0,neginf=0)
    W,H=args.width,args.height; physical_width=497.418836818384; physical_height=280.; zmin=-200.
    flow_small,drift=runoff(fields)
    print('GEOMETRY_ROUTED',time.time()-started,flush=True)
    position=resize(fields['position_m'],(H,W))
    valid=resize(fields['valid'].astype(np.float32),(H,W))>.2
    geometry_normal=resize(fields['normal'],(H,W)); geometry_normal/=np.maximum(np.linalg.norm(geometry_normal,axis=2)[...,None],1.e-6)
    u,chart_z=np.meshgrid((np.arange(W,dtype=np.float32)+.5)*physical_width/W,zmin+(np.arange(H,dtype=np.float32)+.5)*physical_height/H)
    actual_z=np.where(valid,position[:,:,2],chart_z)
    # Input positions are metres in the authored cliff frame. Geology uses true
    # elevation on roof/ledges, not the chart's radial height continuation.
    formation_z=actual_z+.052*u+5.0*noise(u,actual_z,180,210,81)+1.4*noise(u,actual_z,55,120,84)
    # A single ordered sedimentary sequence, with irregular bed widths and
    # strengths. No periodic sin bands or Voronoi rock-cell coloration.
    rng=np.random.default_rng(711531)
    breaks=np.r_[-500.,-500.+np.cumsum(rng.uniform(1.2,6.8,250))]
    bed=np.clip(np.searchsorted(breaks,formation_z,side='right')-1,0,len(breaks)-2)
    fraction=(formation_z-breaks[bed])/(breaks[bed+1]-breaks[bed])
    strengths=rng.normal(0,.013,len(breaks)).astype(np.float32)
    lithology=strengths[bed]
    interbed=np.exp(-(fraction/.055)**2)*(.55+.45*noise(u,actual_z,17,6,21))
    broad=noise(u,actual_z,160,125,193)
    meso=noise(u,actual_z,18,11,37)+.38*noise(u,actual_z,6,3.5,51)
    fine=noise(u,actual_z,1.4,.85,931)+.42*noise(u,actual_z,.38,.28,435)
    runoff_coverage=resize(flow_small,(H,W))
    broken=smoothstep(-.42,.40,noise(u,actual_z,7,28,714))
    stains=smoothstep(.20,.85,runoff_coverage)*(.55+.45*broken)
    concavity=resize(np.tanh(np.maximum(fields.get('curvature',np.zeros(fields['valid'].shape)),0)*2),(H,W))
    wet=smoothstep(.50,.95,runoff_coverage)
    wet*=.95+.05*concavity
    stains*=.88+.12*concavity
    # Actual face slope controls exposure/sediment; not a facet ID random color.
    shelf=smoothstep(.3,.8,geometry_normal[:,:,2])
    exposed=smoothstep(.03,.7,noise(u,actual_z,65,90,191))
    mineral=smoothstep(.10,.62,noise(u+7*noise(u,actual_z,60,90,9),formation_z,90,28,411))
    # Sparse joints with nonuniform spacing and interrupted traces.
    # These are shallow discoloured fractures, never dark painted cavities.
    joint_coordinate=u+.115*actual_z+.7*noise(u,actual_z,24,16,53)
    joint_breaks=np.r_[-200.,-200.+np.cumsum(rng.uniform(9,27,90))]
    joint_bed=np.clip(np.searchsorted(joint_breaks,joint_coordinate)-1,0,len(joint_breaks)-2)
    joint_distance=np.minimum(joint_coordinate-joint_breaks[joint_bed],joint_breaks[joint_bed+1]-joint_coordinate)
    joints=np.exp(-(joint_distance/.22)**2)*smoothstep(-.17,.35,noise(u,actual_z,13,8,516))
    fracture_exposure=smoothstep(-.32,.40,noise(u,actual_z,23,14,735))
    # Only locally exposed strata retain visible bedding. Both their material
    # contrast and physical ledge relief terminate together; the underlying
    # formation is coherent without drawing lines across the whole mountain.
    bedding_exposure=smoothstep(-.04,.32,noise(u,actual_z,46,31,202))
    bedding_exposure*=smoothstep(-.27,.18,noise(u,actual_z,16,9,819))
    interbed*=bedding_exposure
    value=.165+.045*broad+.85*lithology*bedding_exposure+.023*meso+.008*fine-.016*interbed
    value+=.016*fracture_exposure
    value-=.013*joints
    value+=.010*shelf+.010*exposed
    value*=1-.30*stains-.22*wet
    color=np.stack((value*.981,value*1.008,value*1.027),axis=2).astype(np.float32)
    # Restrained warm mineral oxidation and a cool waterborne patina, both
    # material reflectance rather than baked directional lighting/AO.
    color+=mineral[:,:,None]*np.array([.012,.004,-.006],np.float32)
    color+=stains[:,:,None]*np.array([-.002,.000,-.003],np.float32)
    color=np.clip(color,.075,.39)
    rough=np.clip(.79+.055*meso+.035*broad-.025*shelf, .65,.9)
    rough=rough*(1-wet)+(.39+.06*broken)*wet
    # Between the large mesh faces and fine grain, angular bedrock planes and
    # broken shallow ledges restore metre-scale surface structure. These are
    # original normal-only local relief, not repeated scan or macro-face normals.
    relief=.012*noise(u,actual_z,.95,.65,101)+.008*noise(u,actual_z,2.9,1.3,144)-.010*interbed-.012*joints
    plane_u=u+.58*noise(u,actual_z,8,5,552)
    plane_v=formation_z+.38*noise(u,actual_z,7,4,116)
    angular=fractured_planes(plane_u,plane_v,2.7,1.8,871)
    small_angular=fractured_planes(plane_u+.35*plane_v,plane_v,.95,.68,563)
    relief+=(.11+.30*fracture_exposure)*angular
    relief+=.060*fracture_exposure*small_angular
    ledge_profile=fraction-smoothstep(.84,.98,fraction)
    bed_strength=.2+.32*hash01(bed,np.zeros_like(bed),471)
    ledge_continuity=bedding_exposure*smoothstep(-.12,.30,noise(u,actual_z,11,7,819))
    relief+=bed_strength*ledge_profile*ledge_continuity
    dv,du=np.gradient(relief,physical_height/H,physical_width/W)
    # UV overlap/steep roof chart transitions must not create black normal spikes.
    gradient_length=np.hypot(du,dv)
    limit=np.minimum(1,1.25/np.maximum(gradient_length,1.e-6));du*=limit;dv*=limit
    normal=np.stack((-du,-dv,np.ones((H,W),np.float32)),axis=2)
    normal/=np.maximum(np.linalg.norm(normal,axis=2)[...,None],1.e-6)
    srgb=np.where(color<=.0031308,color*12.92,1.055*color**(1/2.4)-.055)
    print('GEOLOGY_AUTHORED',time.time()-started,flush=True)
    files={'albedo':'T_NordicFace_BaseColor.tga','normal':'T_NordicFace_NormalGL.tga','roughness':'T_NordicFace_Roughness.tga'}
    write_tga(out/files['albedo'],srgb); write_tga(out/files['normal'],normal*.5+.5); write_tga(out/files['roughness'],rough)
    preview=resize(srgb,(750,1500))[::-1]
    write_png(out/'geology_albedo_preview.png',preview)
    write_png(out/'geology_runoff_preview.png',resize(stains,(750,1500))[::-1])
    write_png(out/'geology_normal_preview.png',resize(normal*.5+.5,(750,1500))[::-1])
    np.savez_compressed(out/'geology_masks.npz',runoff=resize(stains,(512,1024)),wetness=resize(wet,(512,1024)),mineral=resize(mineral,(512,1024)),shelf=resize(shelf,(512,1024)),valid=resize(valid.astype(np.float32),(512,1024)))
    stats={'albedo_linear_min':color.min(axis=(0,1)).tolist(),'albedo_linear_max':color.max(axis=(0,1)).tolist(),'albedo_linear_mean':color.mean(axis=(0,1)).tolist(),'roughness_min':float(rough.min()),'roughness_max':float(rough.max()),'wet_fraction_above_025':float(np.mean(wet>.25)),'valid_fraction':float(valid.mean())}
    manifest={'schema':'directorscut.unique_geology.v1','resolution':[W,H],'physical_width_m':physical_width,'physical_height_m':physical_height,'z_min_m':zmin,'z_max_m':80.,'unique_pilot':True,'source_kind':'original geometry-conditioned procedural geology; no repeated scan albedo','geometry_fields':str(args.fields.resolve()),'geometry_fields_sha256':sha(args.fields),'generator_source_sha256':sha(__file__),'uv':'UV0 U=arc/497.418836818384; face V=(Z+200)/280; rows +V up; roof radial continuation in source UV. Use in-domain mask and pilot vertexR; do not wrap unique atlas beyond pilot.','normal_format':'OpenGL tangent-space fine relief only; positive V up; importer flips green once','albedo_encoding':'sRGB from linear reflectance; no AO/cavity/directional shading','roughness_encoding':'linear dry .65-.90; wet target .39-.45 blended by local flow coverage','geology':'One ordered irregular stratigraphic sequence; whole-surface dip/fold; broad mineral regions; projected gravity runoff; restrained sub-metre unique relief','limitations':'Art-directed geology and approximate UV drainage, not a calibrated erosion solver; atlas covers pilot only, not whole 4.3km ring','statistics':stats,'seconds':time.time()-started,'files':{n:sha(out/n) for n in files.values()}}
    manifest.update(schema='directorscut.unique_geology.v3',normal_format='OpenGL tangent-space original micro and meso fracture/ledge relief; positive V up; importer flips green once',geology='One irregular geological sequence exposed only in terminating regional patches; broad mineral regions; projected gravity runoff; angular .6-4m fractured bedrock and interrupted shallow ledges; subdued sub-metre relief')
    manifest['statistics']['bedding_exposure_fraction_above_025']=float(np.mean(bedding_exposure[valid]>.25))
    manifest['statistics']['normal_angle_p50_p90_p99_degrees']=np.percentile(np.degrees(np.arccos(np.clip(normal[:,:,2],0,1)))[valid],[50,90,99]).tolist()
    (out/'tile_manifest.json').write_text(json.dumps(manifest,indent=2))
    print('GEOLOGY_READY '+json.dumps({'output':str(out),'seconds':manifest['seconds'],'statistics':stats}),flush=True)


if __name__=='__main__':main()
