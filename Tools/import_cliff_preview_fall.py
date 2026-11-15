"""Import a fitted preview fall and write its active-cliff runtime contract.

Root runs this in UnrealEditor-Cmd. DC_CLIFF_FALL_DIR names an immutable worker
bundle containing attachment.json, waterfall_profile.json, waterfall_core.foam.fbx
and Volumes/manifest.json. The current cliff import/source hashes must match.
Only generated preview assets are changed. No map, sky or vendor assets change.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import runpy
import unreal


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


source = Path(os.environ['DC_CLIFF_FALL_DIR']).resolve()
saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
attachment = read(source/'attachment.json')
profile = read(source/'waterfall_profile.json')
cache = read(source/'Volumes'/'manifest.json')
active = read(saved/'CliffLandformPreviewImport.json')
sidecar = read(active['export_sidecar'])
if attachment['schema'] != 'directorscut.compressed_waterfall_attachment.v1':
    raise RuntimeError('Unexpected fall attachment schema')
if sha(active['source_fbx']) != active['source_sha256'] or sidecar['fbx_sha256'] != active['source_sha256']:
    raise RuntimeError('Active cliff FBX changed')
candidate_hash = attachment['source_sha256']
if sha(attachment['source_path']) != candidate_hash or sidecar.get('input_sha256') != candidate_hash:
    raise RuntimeError('Fall is not fitted to the active original cliff candidate')
if profile['source_mesh_sha256'] != candidate_hash or profile['profile_sha1'] != attachment['profile_sha1']:
    raise RuntimeError('Profile/attachment mismatch')
if cache.get('fitted_profile', {}).get('profile_sha1') != profile['profile_sha1']:
    raise RuntimeError('VDB cache does not belong to this fitted profile')
if cache['frame_count'] != 48 or cache['fps'] != 12. or cache['duration_seconds'] != 4.:
    raise RuntimeError('Expected original48-frame12fps four-second loop')
for axis in ('frame_origin_arena_m','basis_u_arena','basis_v_arena','basis_z_arena'):
    values=attachment[axis]
    if len(values)!=3 or not all(math.isfinite(float(x)) for x in values):
        raise RuntimeError('Invalid authoring frame')

# Reuse the tested native SVT importer and physical extinction conversion.
old = {k:os.environ.get(k) for k in ('DC_WATERFALL_DIR','DC_WATERFALL_MATERIALS_ONLY')}
try:
    os.environ['DC_WATERFALL_DIR']=str(source/'Volumes')
    os.environ['DC_WATERFALL_MATERIALS_ONLY']='0'
    runpy.run_path(str(Path(__file__).with_name('import_procedural_waterfalls.py')),run_name='__main__')
finally:
    for key,value in old.items():
        if value is None: os.environ.pop(key,None)
        else: os.environ[key]=value
volume_import = read(saved/'ProceduralWaterfallImport.json')
if volume_import['fitted_profile']['profile_sha1'] != profile['profile_sha1']:
    raise RuntimeError('Imported volume report has stale profile')

ROOT='/Game/DC/Maelstrom/GeneratedCliffPreview/Falls'
tools=unreal.AssetToolsHelpers.get_asset_tools()
M=unreal.MaterialEditingLibrary
NOISE=r'''
struct FallField {
 float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
 float3 sample(float2 p) {
  float2 i=floor(p),f=frac(p),u=f*f*(3-2*f),du=6*f*(1-f);
  float a=hash(i),b=hash(i+float2(1,0)),c=hash(i+float2(0,1)),d=hash(i+1);
  return float3(lerp(lerp(a,b,u.x),lerp(c,d,u.x),u.y),
   du.x*lerp(b-a,d-c,u.y),du.y*lerp(c-a,d-b,u.x));
 }
}; FallField field;
'''


def material(role):
    name='M_CliffFall_'+role
    mat=unreal.load_asset(ROOT+'/'+name)
    if mat is None: mat=tools.create_asset(name,ROOT,unreal.Material,unreal.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided',True)
    mat.set_editor_property('opacity_mask_clip_value',.38)
    def node(cls):return M.create_material_expression(mat,cls)
    def custom(code,kind,inputs):
        n=node(unreal.MaterialExpressionCustom)
        n.set_editor_property('code',code);n.set_editor_property('output_type',kind)
        pins=[]
        for key in inputs:
            p=unreal.CustomInput();p.set_editor_property('input_name',key);pins.append(p)
        n.set_editor_property('inputs',pins)
        for key,value in inputs.items():
            if not M.connect_material_expressions(value,'',n,key):raise RuntimeError('Material input failed:'+key)
        return n
    def output(n,prop):
        if not M.connect_material_property(n,'',prop):raise RuntimeError('Material output failed')
    def scalar(value):
        n=node(unreal.MaterialExpressionConstant);n.set_editor_property('r',value);return n
    uv=node(unreal.MaterialExpressionTextureCoordinate)
    time=node(unreal.MaterialExpressionScalarParameter)
    time.set_editor_property('parameter_name','PreviewTime');time.set_editor_property('default_value',0.)
    # Export UV: U is normalized across the sheet, V is physical arc metres.
    # FBX import reverses texture V; 1-UV.y recovers increasing downstream arc.
    foam=role=='ReceivingFoam';aerated=role=='AeratedCore'
    width=1.5 if foam else float(max(attachment['width_range_m']))
    speed=3. if foam else 18.
    code=NOISE+f'''
float across=UV.x*{width:.9g},along=1-UV.y;
float2 p=float2(across,along-{speed:.9g}*T);
float large=field.sample(p/float2(.75,3.7)).x;
float small=field.sample(p/float2(.19,.63)+float2(17.3,4.9)).x;
float stretch=field.sample(p/float2(.38,1.9)+float2(-8.2,13.7)).x;
float bubbles=smoothstep(.36,.73,.50*large+.30*small+.20*stretch);
float edge=smoothstep(0,.10,UV.x)*(1-smoothstep(.90,1,UV.x));
return float4(bubbles,large,small,edge);
'''
    field=custom(code,unreal.CustomMaterialOutputType.CMOT_FLOAT4,{'UV':uv,'T':time})
    if foam:
        color_code='return lerp(float3(.29,.38,.39),float3(.72,.79,.78),Field.x);'
        mask_code='return Field.w*smoothstep(.24,.56,Field.x);'
        rough_code='return lerp(.54,.82,Field.x);'
    elif aerated:
        color_code='return lerp(float3(.035,.09,.105),float3(.64,.73,.73),Field.x);'
        mask_code='return Field.w*lerp(.45,1.,smoothstep(.12,.40,Field.y));'
        rough_code='return lerp(.26,.64,Field.x);'
    else:
        color_code='return lerp(float3(.018,.055,.065),float3(.23,.34,.35),Field.x*.48);'
        mask_code='return smoothstep(0,.055,Field.w);'
        rough_code='return lerp(.16,.35,Field.x);'
    output(custom(color_code,unreal.CustomMaterialOutputType.CMOT_FLOAT3,{'Field':field}),unreal.MaterialProperty.MP_BASE_COLOR)
    output(custom(rough_code,unreal.CustomMaterialOutputType.CMOT_FLOAT1,{'Field':field}),unreal.MaterialProperty.MP_ROUGHNESS)
    output(custom(mask_code,unreal.CustomMaterialOutputType.CMOT_FLOAT1,{'Field':field}),unreal.MaterialProperty.MP_OPACITY_MASK)
    normal=custom(NOISE+f'''
float2 p=float2(UV.x*{width:.9g},1-UV.y-{speed:.9g}*T)/float2(.38,1.9);
float3 n=field.sample(p);
return normalize(float3(-n.y*.22,n.z*.08,1));
''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,{'UV':uv,'T':time})
    output(normal,unreal.MaterialProperty.MP_NORMAL)
    output(scalar(.5),unreal.MaterialProperty.MP_SPECULAR)
    output(scalar(0.),unreal.MaterialProperty.MP_METALLIC)
    output(scalar(0.),unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    M.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False):raise RuntimeError('Material save failed')
    return mat


fbx=source/'waterfall_core.foam.fbx';fbx_hash=sha(fbx)
name='SM_CliffFall_'+fbx_hash[:12]
mesh=unreal.load_asset(ROOT+'/'+name)
if mesh is None:
    unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
    options=unreal.FbxImportUI();options.automated_import_should_detect_type=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_mesh=True;options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False
    imp=options.static_mesh_import_data
    imp.combine_meshes=True;imp.convert_scene=True;imp.convert_scene_unit=True;imp.import_uniform_scale=1.
    imp.generate_lightmap_u_vs=False;imp.auto_generate_collision=False;imp.build_nanite=False
    imp.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE
    imp.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task=unreal.AssetImportTask();task.filename=str(fbx);task.destination_path=ROOT;task.destination_name=name
    task.automated=True;task.replace_existing=False;task.save=False;task.factory=unreal.FbxFactory();task.options=options
    tools.import_asset_tasks([task])
    meshes=[unreal.load_asset(p) for p in task.imported_object_paths]
    meshes=[m for m in meshes if isinstance(m,unreal.StaticMesh)]
    if len(meshes)!=1:raise RuntimeError('Expected one combined original fall mesh')
    mesh=meshes[0]
if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Missing core mesh')
slots=mesh.get_editor_property('static_materials')
expected=['WetCore','AeratedCore','ReceivingFoam']
if len(slots)!=3:raise RuntimeError('Expected exactly three authored water material slots')
seen=set()
for i,slot in enumerate(slots):
    slot_name=str(slot.material_slot_name)
    role=next((r for r in expected if slot_name==r or slot_name.startswith(r+'_')),None)
    if role is None or role in seen:raise RuntimeError('Unexpected/duplicate core slot '+slot_name)
    seen.add(role);mesh.set_material(i,material(role))
box=mesh.get_bounding_box();size=box.max-box.min
size_m=[float(size.x)*.01,float(size.y)*.01,float(size.z)*.01]
if not all(math.isfinite(x) and x>0. for x in size_m) or size_m[2]<attachment['drop_m']*.98 or size_m[2]>attachment['drop_m']+25.:
    raise RuntimeError('Unexpected physical core FBX bounds:'+str(size_m))
if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False):raise RuntimeError('Core save failed')
volumes=[]
for record in volume_import['volumes']:
    layer=record['name']
    volumes.append(dict(asset=record['asset'],material=volume_import['root']+'/M_'+layer,
        origin_arena_cm=profile['local_origin_arena_cm'],yaw_degrees=profile['local_yaw_degrees'],
        frames=48,fps=12.,density_per_cm=.004 if layer=='Falling' else .0015))
report=dict(schema='directorscut.cliff_preview_waterfall.v1',cliff_fbx_sha256=active['source_sha256'],
    source_candidate_sha256=candidate_hash,attachment_path=str(source/'attachment.json'),attachment_sha256=sha(source/'attachment.json'),
    profile_path=str(source/'waterfall_profile.json'),profile_sha1=profile['profile_sha1'],
    core=dict(asset=mesh.get_path_name(),source_fbx=str(fbx),source_fbx_sha256=fbx_hash,size_metres=size_m,
        origin_arena_m=attachment['frame_origin_arena_m'],basis_u_arena=attachment['basis_u_arena'],
        basis_v_arena=attachment['basis_v_arena'],basis_z_arena=attachment['basis_z_arena']),volumes=volumes,
    material_model='Scene-lit masked wet/aerated core and broken advected receiving foam; zero emissive; no refraction or fluid simulation claimed',
    uv_contract='Across0..1; original arc metres recovered by1-importedV; verify actual import UV direction in motion',
    runtime_integrated=False,visual_accepted=False)
(saved/'CliffPreviewWaterfall.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
unreal.log_warning('DC_CLIFF_PREVIEW_FALL_IMPORT '+json.dumps(report))
