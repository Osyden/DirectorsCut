"""Combine separately supported hero outlets into one physical native bundle.

Blender background only. No scaling, cliff edits or invented connecting water.
First outlet supplies the SVT frame; every other original trajectory keeps its
own source momentum, support, receiving surface and contact clock.
"""
import argparse,copy,hashlib,json,sys
from pathlib import Path
import bpy,numpy as np
sys.dont_write_bytecode=True

def load(path):
    with np.load(path) as d:return {k:d[k].copy() for k in d.files}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--outlet',type=Path,action='append',required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
    if len(a.outlet)!=3 or a.output.exists() or 'saved' not in [s.lower() for s in a.output.resolve().parts]:
        p.error('Use exactly three measured outlets and a new Saved output')
    reports=[json.loads((d/'attachment.json').read_text()) for d in a.outlet]
    profiles=[json.loads((d/'waterfall_profile.json').read_text()) for d in a.outlet]
    for r in reports[1:]:
        for key in ('source_sha256','water_sha256','frame_origin_arena_m','basis_u_arena','basis_v_arena','basis_z_arena'):
            if r[key]!=reports[0][key]:raise ValueError('Outlets do not share the same exact original geometry/chart')
    core=[];foam=[];supports=[]
    for folder in a.outlet:
        core.append(load(folder/'core.npz'));foam.append(load(folder/'receiving_foam.npz'));supports.append(load(folder/'support.npz'))
    def combine(parts):
        offset=0;vertices=[];faces=[];uv=[];slots=[]
        for d in parts:
            vertices.append(d['vertices_m']);faces.append(d['triangles']+offset);uv.append(d['uv'])
            if 'face_material_index' in d:slots.append(d['face_material_index'])
            offset+=len(d['vertices_m'])
        out={k:parts[0][k] for k in ('frame_origin_world_m','basis_u_world','basis_v_world','basis_z_world')}
        out.update(vertices_m=np.concatenate(vertices),triangles=np.concatenate(faces),uv=np.concatenate(uv))
        if slots:out['face_material_index']=np.concatenate(slots)
        return out
    core=combine(core);foam=combine(foam)
    origin=np.array(reports[0]['frame_origin_arena_m'])
    basis=np.array([reports[0][k]for k in ('basis_u_arena','basis_v_arena','basis_z_arena')]).T
    crest=np.concatenate([np.array(p['ballistic_launch']['cross_sources_local_cm'])*.01@
          np.array([p['basis_x_arena'],p['basis_y_arena'],p['basis_z_arena']])+np.array(p['local_origin_arena_cm'])*.01 for p in profiles])
    crest_chart=(crest-origin)@basis;span=float(np.ptp(crest_chart[:,0]))
    if not 20<span<40:raise ValueError(f'Actual crest span {span:.2f}m outside bounded20–40m')
    profile=copy.deepcopy(profiles[0]);profile['ballistic_outlets']=profiles
    profile['source_half_width_cm']=span*50
    profile['trajectory_model']='three_separately_measured_ballistic_outlets_in_shared_frame'
    profile['profile_sha1']=hashlib.sha1(json.dumps({k:v for k,v in profile.items()if k!='profile_sha1'},sort_keys=True).encode()).hexdigest()
    report=copy.deepcopy(reports[0]);report['profile_sha1']=profile['profile_sha1'];report['crest_span_m']=span
    report['outlets']=[dict(bundle=str(folder.resolve()),canonical_source_vertex=r['source_vertex_index'],
        width_m=max(r['width_range_m']),free_drop_m=r['terminal_jet']['drop_m'],
        source_arena_m=r['source_arena_m'],foot_arena_m=r['foot_arena_m'],jet=r['terminal_jet'])for folder,r in zip(a.outlet,reports)]
    report['drop_m']=float(core['vertices_m'][:,2].max()-foam['vertices_m'][:,2].min())
    report['width_range_m']=[min(min(r['width_range_m'])for r in reports),max(max(r['width_range_m'])for r in reports)]
    report['core_triangles']=len(core['triangles']);report['foam_triangles']=len(foam['triangles'])
    report['limitations']='Three separately authored outlets; no connecting upstream river or hydrological supply solved. Static measured water; no runtime hull guarantee.'
    a.output.mkdir(parents=True)
    for name,data in (('core',core),('receiving_foam',foam)):np.savez_compressed(a.output/(name+'.npz'),**data)
    merged={}
    for key in ('path_chart_m','cross_sections_chart_m','support_triangle','support_barycentric','source_mesh_edge_path','width_m'):
        merged[key]=np.concatenate([d[key] for d in supports])
    lengths=[len(d['path_chart_m'])for d in supports];ends=np.cumsum(lengths);merged['outlet_row_ranges']=np.c_[np.r_[0,ends[:-1]],ends]
    merged['jet_start_index']=np.array([d['jet_start_index']for d in supports])
    merged['impact_triangle_index']=np.array([d['impact_triangle_index']for d in supports])
    merged['impact_barycentric']=np.array([d['impact_barycentric']for d in supports])
    np.savez_compressed(a.output/'support.npz',**merged)
    (a.output/'waterfall_profile.json').write_text(json.dumps(profile,indent=2))
    (a.output/'attachment.json').write_text(json.dumps(report,indent=2))
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    mats=[]
    for name,color in [('WetCore',(.095,.20,.22,1)),('AeratedCore',(.68,.76,.77,1)),('ReceivingFoam',(.75,.80,.80,1))]:
        m=bpy.data.materials.new(name);m.diffuse_color=color;mats.append(m)
    for name,d in [('CrestCore',core),('CrestFoam',foam)]:
        me=bpy.data.meshes.new(name);me.from_pydata(((d['vertices_m']-origin)@basis).tolist(),[],d['triangles'].tolist());me.update()
        ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
        for m in mats:me.materials.append(m)
        for i,f in enumerate(me.polygons):f.use_smooth=True;f.material_index=int(d['face_material_index'][i])if 'face_material_index'in d else 2
        uv=me.uv_layers.new(name='AcrossAndArcMetres')
        for loop in me.loops:uv.data[loop.index].uv=d['uv'][loop.vertex_index]
    bpy.ops.object.select_all(action='SELECT');bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    bpy.ops.export_scene.fbx(filepath=str((a.output/'waterfall_core.foam.fbx').resolve()),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,global_scale=1,bake_space_transform=False,mesh_smooth_type='FACE',use_mesh_modifiers=False,add_leaf_bones=False)
    print(json.dumps(dict(crest_span_m=span,outlets=report['outlets'],drop_m=report['drop_m']),indent=2))

if __name__=='__main__':main()
