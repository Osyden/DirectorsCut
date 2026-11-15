"""Independent triangle-interior clearance audit for a generated fall bundle."""
import json,sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent))
from attach_compressed_cliff_waterfall import Surface,load,sha
p=Path(sys.argv[sys.argv.index('--')+1]);report=json.loads((p/'attachment.json').read_text())
assert sha(report['source_path'])==report['source_sha256']
rock_data=load(report['source_path']);water_data=load(report['water_path']);core=load(p/'core.npz')
rock=Surface(rock_data['vertices_m'],rock_data['triangles']);water=Surface(water_data['vertices_m'],water_data['triangles'])
result={}
source_ids=core.get('source_rock_triangle')
if source_ids is not None:
    supported=np.flatnonzero(source_ids>=0)
    expected=rock_data['vertices_m'][rock_data['triangles'][source_ids[supported]]].copy();expected[:,:,2]+=.28
    actual=core['vertices_m'][core['triangles'][supported]]
    error=float(np.abs(actual-expected).max())
    if error>1.e-8:raise RuntimeError('Supported core differs from its actual source triangles')
    result['exact_source_triangle_support']=dict(triangles=len(supported),max_position_error_m=error,vertical_offset_m=.28,interior_support='Every point is the same barycentric interpolation of the unchanged actual source triangle, plus the recorded coating offset')
for role,name in ((0,'wet_core'),(1,'aerated_core'),(2,'foam')):
    if role in (0,1) and source_ids is not None:continue
    ids=np.flatnonzero(core['face_material_index']==role)[::max(1,int(np.sum(core['face_material_index']==role)/10000))]
    deltas=[]
    for point in core['vertices_m'][core['triangles'][ids]].mean(axis=1):
        r=rock.hit(point[:2]);w=water.hit(point[:2]);surface=w if role==2 else max((h for h in (r,w) if h is not None),key=lambda h:h[0][2],default=None)
        if surface is not None:deltas.append(float(point[2]-surface[0][2]))
    result[name]=dict(sampled_triangles=len(deltas),clearance_min_m=min(deltas),clearance_max_m=max(deltas),p95_abs_m=float(np.percentile(np.abs(deltas),95)),buried_over_1m=sum(x<-1 for x in deltas),floating_over_2m=sum(x>2 for x in deltas))
(p/'contact_audit.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
