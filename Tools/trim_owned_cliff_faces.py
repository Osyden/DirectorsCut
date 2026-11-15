"""CPU Blender recipe: owned Fab raw scans -> bottom-apron-free derivatives.

Source pixels/meshes are not redistributed. DC_OWNED_CLIFF_CACHE locates the
owned FabLibrary; DC_OWNED_CLIFF_WORK selects an ignored workspace output root.
The fixed source identities and exact 22% cut reproduce the original proof.
"""
import bpy, json, time, os
import numpy as np
from pathlib import Path
cache=Path(os.environ.get('DC_OWNED_CLIFF_CACHE','C:/ProgramData/Epic/EpicGamesLauncher/VaultCache/FabLibrary'))
work=Path(os.environ.get('DC_OWNED_CLIFF_WORK',str(Path(__file__).resolve().parents[1]/'Saved'/'OwnedScanSources')))
base=work/'OwnedCliffTrim'
sources=[('vefkehbva',cache/'Huge_Nordic_Coastal_Cliff-85318909/fbx/raw/huge_nordic_coastal_clif_extracted'),('vfmjadufa',cache/'Massive_Nordic_Coastal_Cliff-93b50c63/fbx/raw/massive_nordic_coastal_c_extracted')]
start=time.time()
for asset_id,folder in sources:
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 for mesh in list(bpy.data.meshes):
  if mesh.users==0:bpy.data.meshes.remove(mesh)
 source=next(folder.glob('*.fbx'));t=time.time()
 bpy.ops.import_scene.fbx(filepath=str(source),use_custom_normals=True)
 objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
 all_min=[];all_max=[];counts=[]
 for obj in objects:
  co=np.empty(len(obj.data.vertices)*3,dtype=np.float32);obj.data.vertices.foreach_get('co',co);co=co.reshape(-1,3)
  mat=np.asarray(obj.matrix_world,dtype=np.float64);world=co@mat[:3,:3].T+mat[:3,3]
  all_min.append(world.min(0));all_max.append(world.max(0));counts.append(len(obj.data.polygons))
 minimum=np.min(all_min,0);maximum=np.max(all_max,0);cut=float(minimum[2]+.22*(maximum[2]-minimum[2]))
 records=[];new_min=[];new_max=[]
 for obj in objects:
  bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
  mesh=obj.data;co=np.empty(len(mesh.vertices)*3,dtype=np.float32);mesh.vertices.foreach_get('co',co);co=co.reshape(-1,3)
  mat=np.asarray(obj.matrix_world,dtype=np.float64);world=co@mat[:3,:3].T+mat[:3,3]
  selected=np.ascontiguousarray(world[:,2]<cut,dtype=np.bool_)
  before=dict(vertices=len(mesh.vertices),triangles=len(mesh.polygons),uv_layers=[l.name for l in mesh.uv_layers],has_custom_normals=bool(mesh.has_custom_normals),material_slots=len(obj.material_slots))
  mesh.edges.foreach_set('select',np.zeros(len(mesh.edges),dtype=np.bool_));mesh.polygons.foreach_set('select',np.zeros(len(mesh.polygons),dtype=np.bool_));mesh.vertices.foreach_set('select',selected);bpy.context.tool_settings.mesh_select_mode=(True,False,False)
  bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.delete(type='VERT');bpy.ops.object.mode_set(mode='OBJECT')
  mesh=obj.data;after=dict(vertices=len(mesh.vertices),triangles=len(mesh.polygons),uv_layers=[l.name for l in mesh.uv_layers],has_custom_normals=bool(mesh.has_custom_normals),material_slots=len(obj.material_slots))
  if after['uv_layers']!=before['uv_layers'] or after['material_slots']!=before['material_slots']:
   raise RuntimeError('UV/material layer changed during deletion')
  if before['has_custom_normals'] and not after['has_custom_normals']:raise RuntimeError('Custom normals lost')
  co=np.empty(len(mesh.vertices)*3,dtype=np.float32);mesh.vertices.foreach_get('co',co);world=co.reshape(-1,3)@mat[:3,:3].T+mat[:3,3]
  new_min.append(world.min(0));new_max.append(world.max(0));records.append(dict(object=obj.name,before=before,after=after))
 bpy.ops.object.select_all(action='DESELECT')
 for obj in objects:obj.select_set(True)
 out=base/asset_id;out.mkdir(parents=True,exist_ok=True);fbx=out/(source.stem+'_face.fbx')
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},use_mesh_modifiers=False,
   apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Z',axis_up='Y',global_scale=1.,
   bake_anim=False,add_leaf_bones=False,mesh_smooth_type='OFF',use_tspace=False,path_mode='AUTO',embed_textures=False)
 lo=np.min(new_min,0);hi=np.max(new_max,0)
 report=dict(asset_id=asset_id,source_fbx=str(source),derivative_fbx=str(fbx),crop='delete vertices below world minZ+22%height, including incident triangles; no cap',cut_world_z_m=cut,original_bounds_min_m=minimum.tolist(),original_bounds_max_m=maximum.tolist(),bounds_min_m=lo.tolist(),bounds_max_m=hi.tolist(),size_m=(hi-lo).tolist(),objects=records,remaining_triangles=sum(r['after']['triangles'] for r in records),seconds=time.time()-t,source_unchanged=True,uvs_and_custom_normal_layers_retained=True,rigid_scale=1.)
 (out/'trim_report.json').write_text(json.dumps(report,indent=2));print('CLIFF_TRIM '+json.dumps(report),flush=True)
print('TOTAL_SECONDS',time.time()-start,flush=True)

