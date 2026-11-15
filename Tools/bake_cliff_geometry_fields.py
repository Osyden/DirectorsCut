"""Rasterize actual accepted cliff triangles into their existing unique UV atlas.

CPU/NumPy only. Source positions, topology and UVs are read without modification.
Rows increase with V. Curvature is signed cotangent mean curvature (positive
concavity), not simulated erosion, water accumulation or ambient occlusion.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np


def vertex_fields(v, t):
    normals=np.zeros_like(v);lap=np.zeros_like(v);area=np.zeros(len(v))
    for start in range(0,len(t),100000):
        q=t[start:start+100000];p=v[q]
        cross=np.cross(p[:,1]-p[:,0],p[:,2]-p[:,0])
        twice=np.linalg.norm(cross,axis=1)
        for k in range(3):
            np.add.at(normals,q[:,k],cross)
            np.add.at(area,q[:,k],twice/6)
        for i,j,k in [(0,1,2),(1,2,0),(2,0,1)]:
            cot=np.sum((p[:,i]-p[:,k])*(p[:,j]-p[:,k]),axis=1)/np.maximum(twice,1e-20)
            d=(p[:,j]-p[:,i])*np.clip(cot,-1e4,1e4)[:,None]
            np.add.at(lap,q[:,i],d);np.add.at(lap,q[:,j],-d)
    normals/=np.maximum(np.linalg.norm(normals,axis=1),1e-20)[:,None]
    curvature=np.sum(lap*normals,axis=1)/np.maximum(4*area,1e-20)
    return normals,curvature


def bake(source, output, width, height):
    with np.load(source,allow_pickle=False) as f:
        v=f['vertices_m'].astype(float);t=f['triangles'].astype(np.int32)
        uv=f['uv0'].astype(float);color=f['vertex_rgba']
    vn,vc=vertex_fields(v,t)
    n=width*height
    position=np.zeros((n,3),np.float32);normal=np.zeros((n,3),np.float32)
    curvature=np.zeros(n,np.float32);triangle=np.full(n,-1,np.int32)
    bary=np.zeros((n,3),np.float32);radius_min=np.full(n,np.inf);radius_max=np.full(n,-np.inf)
    pixeluv=uv*np.array([width,height])
    selected=np.flatnonzero((color[t,0].max(axis=1)>0)&
        (uv[t,0].max(axis=1)>=0)&(uv[t,0].min(axis=1)<=1)&
        (uv[t,1].max(axis=1)>=0)&(uv[t,1].min(axis=1)<=1))
    for begin in range(0,len(selected),40000):
        ids=selected[begin:begin+40000];p=pixeluv[t[ids]]
        lo=np.maximum(0,np.ceil(p.min(axis=1)-.5)).astype(np.int64)
        hi=np.minimum([width-1,height-1],np.floor(p.max(axis=1)-.5)).astype(np.int64)
        size=np.maximum(0,hi-lo+1);count=size[:,0]*size[:,1]
        ok=count>0;ids=ids[ok];p=p[ok];lo=lo[ok];size=size[ok];count=count[ok]
        if not len(ids):continue
        owner=np.repeat(np.arange(len(ids)),count)
        ordinal=np.arange(count.sum())-np.repeat(np.cumsum(count)-count,count)
        x=lo[owner,0]+ordinal%size[owner,0];y=lo[owner,1]+ordinal//size[owner,0]
        pp=p[owner];a=pp[:,0];b=pp[:,1];c=pp[:,2]
        den=(b[:,1]-c[:,1])*(a[:,0]-c[:,0])+(c[:,0]-b[:,0])*(a[:,1]-c[:,1])
        safe=np.abs(den)>1e-12
        wa=np.zeros(len(x));wb=np.zeros(len(x))
        wa[safe]=((b[:,1]-c[:,1])*(x+.5-c[:,0])+(c[:,0]-b[:,0])*(y+.5-c[:,1]))[safe]/den[safe]
        wb[safe]=((c[:,1]-a[:,1])*(x+.5-c[:,0])+(a[:,0]-c[:,0])*(y+.5-c[:,1]))[safe]/den[safe]
        weights=np.c_[wa,wb,1-wa-wb]
        hit=safe&(weights.min(axis=1)>=-1e-8)&(weights.max(axis=1)<=1+1e-8)
        pix=(y*width+x)[hit];faces=ids[owner[hit]];weights=weights[hit]
        pos=np.einsum('ni,nij->nj',weights,v[t[faces]])
        radius=np.linalg.norm(pos[:,:2],axis=1)
        np.minimum.at(radius_min,pix,radius);np.maximum.at(radius_max,pix,radius)
        # Stable lowest source triangle wins genuine UV overlaps, which are
        # separately identified below instead of silently called one surface.
        unique,first=np.unique(pix,return_index=True)
        fresh=triangle[unique]<0;unique=unique[fresh];first=first[fresh]
        faces=faces[first];w=weights[first]
        position[unique]=pos[first];triangle[unique]=faces;bary[unique]=w
        nn=np.einsum('ni,nij->nj',w,vn[t[faces]])
        nn/=np.maximum(np.linalg.norm(nn,axis=1),1e-20)[:,None]
        normal[unique]=nn
        curvature[unique]=np.einsum('ni,ni->n',w,vc[t[faces]])
    valid=triangle>=0
    overlap=valid&((radius_max-radius_min)>.03)
    slope=np.degrees(np.arccos(np.clip(normal[:,2],-1,1))).astype(np.float32)
    slope[~valid]=0
    roof=valid&(normal[:,2]>.819152)
    cliff=valid&(normal[:,2]<.573576)
    # Crest-start proxy from actual roof-to-cliff adjacency within 4 texels.
    # This is a material guide; it is not a hydrological flow simulation.
    nearroof=roof.reshape(height,width).copy()
    for _ in range(4):
        out=nearroof.copy();out[1:]|=nearroof[:-1];out[:-1]|=nearroof[1:]
        out[:,1:]|=nearroof[:,:-1];out[:,:-1]|=nearroof[:,1:];nearroof=out
    starts=valid&cliff&nearroof.ravel()
    check=np.flatnonzero(valid)[::max(1,int(valid.sum())//10000)]
    reconstructed=np.einsum('ni,nij->nj',bary[check].astype(float),v[t[triangle[check]]])
    error=np.linalg.norm(reconstructed-position[check],axis=1)
    metadata=dict(schema='directorscut.cliff_geometry_atlas.v1',source=str(source.resolve()),
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),width=width,height=height,
        physical_width_m=497.418836818384,physical_height_m=280,z_min_m=-200,
        uv='pixel centres; rows increase +V; exact original uv0 including roof radial continuation',
        position_frame='arena XYZ metres',curvature='signed cotangent mean curvature, positive concavity, 1/metre',
        normal='area-weighted original triangle vertex normals, barycentrically interpolated; not imported split shading',
        valid_pixels=int(valid.sum()),uv_overlap_pixels=int(overlap.sum()),
        max_overlap_radial_span_m=float(np.max((radius_max-radius_min)[valid],initial=0)),
        sampled_barycentric_position_error_m=float(error.max(initial=0)),
        drainage_start='actual roof/cliff adjacency proxy only; no simulated flow or rainfall',
        source_geometry_modified=False)
    def grid(a):return a.reshape((height,width)+a.shape[1:])
    output.mkdir(parents=True,exist_ok=False)
    np.savez_compressed(output/'fields.npz',position_m=grid(position),normal=grid(normal),
        slope_degrees=grid(slope),curvature=grid(curvature),valid=grid(valid.astype(np.uint8)),
        uv_overlap_mask=grid(overlap.astype(np.uint8)),triangle_index=grid(triangle),barycentric=grid(bary),
        roof_mask=grid(roof.astype(np.uint8)),cliff_mask=grid(cliff.astype(np.uint8)),
        drainage_start_mask=grid(starts.astype(np.uint8)),metadata_json=np.asarray(json.dumps(metadata)))
    (output/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(json.dumps(metadata,indent=2))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--width',type=int,default=2048);p.add_argument('--height',type=int,default=1024)
    a=p.parse_args();bake(a.source,a.output,a.width,a.height)
