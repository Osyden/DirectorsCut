"""Attach the exact imported polar surface to a preview support manifest.

Preview placement samples the actual current ruin bounds; random ruin pieces
need not match a stale recorded AABB. This does not alter normal gameplay.
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
rows,sectors=160,1536
if len(v)!=(rows+1)*(sectors+1) or not np.isfinite(v).all():
    raise ValueError('Expected complete finite native polar grid')
b=(np.arange(rows)[:,None]*(sectors+1)+np.arange(sectors)[None,:]).ravel()
expected=np.stack((np.stack((b,b+sectors+1,b+1),1),np.stack((b+1,b+sectors+1,b+sectors+2),1)),1).reshape(-1,3)
if not np.array_equal(t,expected):
    raise ValueError('Triangle diagonals differ from support sampler')
out=a.saved.resolve()
if out.name.lower()!='saved':
    raise ValueError('Output must be the project Saved folder')
payload=np.asarray(v*100.,dtype='<f4').tobytes()
(out/'CliffPreviewSurface.bin').write_bytes(payload)
support['surface_grid']=dict(rows=rows,sectors=sectors,vertex_count=len(v),
    md5=hashlib.md5(payload).hexdigest(),units='arena centimetres, xyz float32 little-endian',
    source_sha256=meta['input_sha256'])
(out/'CliffPreviewSupports.json').write_text(json.dumps(support,indent=2)+'\n',encoding='utf-8')
print(json.dumps(support['surface_grid']))
