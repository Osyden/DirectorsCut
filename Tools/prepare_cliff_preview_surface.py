"""Prepare actual imported arena triangles for current-footprint ruin placement.

The full authored surface includes fractured overhangs; an old polar grid must
not stand in for newly sculpted rock.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--sidecar',type=Path,required=True)
p.add_argument('--supports',type=Path,required=True)
p.add_argument('--saved',type=Path,required=True)
a=p.parse_args()
meta=json.loads(a.sidecar.read_text(encoding='utf-8'))
support=json.loads(a.supports.read_text(encoding='utf-8'))
source=Path(meta['input_path'])
if hashlib.sha256(source.read_bytes()).hexdigest()!=meta['input_sha256']:
    raise ValueError('Source changed after FBX export')
if support['source_fbx_sha256']!=meta['fbx_sha256']:
    raise ValueError('Support/mesh identity differs')
with np.load(source,allow_pickle=False) as z:
    v=z['vertices_m'];t=z['triangles']
if (v.ndim!=2 or v.shape[1]!=3 or not np.isfinite(v).all()
    or t.ndim!=2 or t.shape[1]!=3 or t.min()<0 or t.max()>=len(v)):
    raise ValueError('Expected finite arena vertices and valid triangles')
out=a.saved.resolve()
if out.name.lower()!='saved':
    raise ValueError('Output must be the project Saved folder')
payload=np.asarray(v*100.,dtype='<f4').tobytes()+np.asarray(t,dtype='<u4').tobytes()
(out/'CliffPreviewSurface.bin').write_bytes(payload)
support.pop('surface_grid',None)
support['surface_mesh']=dict(vertex_count=len(v),triangle_count=len(t),
    md5=hashlib.md5(payload).hexdigest(),
    units='arena centimetres; xyz float32 vertices then uint32 triangle indices, little-endian',
    source_sha256=meta['input_sha256'])
(out/'CliffPreviewSupports.json').write_text(json.dumps(support,indent=2)+'\n',encoding='utf-8')
print(json.dumps(support['surface_mesh']))
