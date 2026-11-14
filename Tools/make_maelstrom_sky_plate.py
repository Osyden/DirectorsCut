"""Import the licensed storm HDRI and build a finale-only sky / sun mask.

Source: Poly Haven, Quarry Cloudy, CC0. Run fetch_maelstrom_sky_plate.ps1
first, then run this through UnrealEditor-Cmd with a rendering RHI enabled.
"""
import json
import math
from pathlib import Path
import unreal

ROOT='/Game/DC/Maelstrom/Sky'
project=Path(unreal.Paths.project_dir())
source=project/'Saved/SkyAssetReview/quarry_cloudy_8k.exr'
source.parent.mkdir(parents=True,exist_ok=True)
texture_path=ROOT+'/T_MaelstromStormPlate'
tex=unreal.load_asset(texture_path)
if not tex:
    if not source.is_file():
        raise RuntimeError('Run Tools/fetch_maelstrom_sky_plate.ps1 before importing the sky')
    task=unreal.AssetImportTask()
    task.set_editor_property('filename',str(source))
    task.set_editor_property('destination_path',ROOT)
    task.set_editor_property('destination_name','T_MaelstromStormPlate')
    task.set_editor_property('automated',True)
    task.set_editor_property('save',True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex=unreal.load_asset(texture_path)
if not isinstance(tex,unreal.Texture2D):
    raise RuntimeError('HDR plate must import as a Texture2D')
tex.set_editor_property('srgb',False)
tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_HDR_COMPRESSED)
tex.set_editor_property('max_texture_size',8192)
tex.set_editor_property('never_stream',True)
tex.set_editor_property('address_x',unreal.TextureAddress.TA_WRAP)
tex.set_editor_property('address_y',unreal.TextureAddress.TA_CLAMP)
unreal.EditorAssetLibrary.save_loaded_asset(tex,False)
sun=unreal.DCMaelstromAtmosphere.inspect_sky_plate(tex)
if sun.z<=0:
    raise RuntimeError('Could not inspect HDR source')
source_elevation=(0.5-sun.y)*math.pi
unreal.log_warning(f'DC_PLATE sunUV={sun.x},{sun.y}, peak={sun.z}, elevation={math.degrees(source_elevation)}')
MEL=unreal.MaterialEditingLibrary

projection=r'''
struct PlateProjection {
 float2 uv(float3 ray,float3 toSun,float anchorU,float anchorElevation) {
  float az=atan2(ray.y,ray.x)-atan2(toSun.y,toSun.x);
  float elevation=asin(clamp(ray.z,-1,1));
  float minimum=radians(12.0); // Exclude all photographic terrain at every azimuth.
  float target=max(0.05,asin(saturate(toSun.z))/(PI*0.5));
  float source=saturate((anchorElevation-minimum)/(PI*0.5-minimum));
  float power=log(clamp(source,0.05,0.98))/log(min(target,0.95));
  float photoElevation=lerp(minimum,PI*0.5,pow(saturate(elevation/(PI*0.5)),power));
  return float2(frac(anchorU+az/(2*PI)),0.5-photoElevation/PI);
 }
};
PlateProjection projection;
'''

def expr(mat,cls): return MEL.create_material_expression(mat,cls)
def scalar(mat,name,value):
    n=expr(mat,unreal.MaterialExpressionScalarParameter)
    n.set_editor_property('parameter_name',name); n.set_editor_property('default_value',value)
    return n
def vector(mat,name,value):
    n=expr(mat,unreal.MaterialExpressionVectorParameter)
    n.set_editor_property('parameter_name',name); n.set_editor_property('default_value',unreal.LinearColor(*value))
    return n

for name,is_mask in [('M_MaelstromSkyPlate',False),('M_MaelstromSkyShadow',True)]:
    mat=unreal.load_asset(ROOT+'/'+name)
    if mat: MEL.delete_all_material_expressions(mat)
    else: mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,ROOT,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_LIGHT_FUNCTION if is_mask else unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided',True)
    if not is_mask: mat.set_editor_property('is_sky',True)
    picture=expr(mat,unreal.MaterialExpressionTextureObject)
    picture.set_editor_property('texture',tex)
    picture.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    nodes={'P':expr(mat,unreal.MaterialExpressionWorldPosition),'Picture':picture,
           'ToSun':vector(mat,'ToSun',(0.664,-0.664,0.342,1)),
           'AnchorU':scalar(mat,'AnchorU',0.56),'AnchorElevation':scalar(mat,'AnchorElevation',math.radians(40))}
    if is_mask:
        nodes['RimHeight']=scalar(mat,'RimHeight',0)
        code=projection+r'''
float plane=(2.0+RimHeight)*100000;
float travel=max(0,(plane-P.z)/max(ToSun.z,0.1));
float3 ray=normalize(P+ToSun*travel);
float3 rgb=Texture2DSampleLevel(Picture,PictureSampler,projection.uv(ray,ToSun,AnchorU,AnchorElevation),0).rgb;
float luma=dot(rgb,float3(0.2126,0.7152,0.0722));
float blue=saturate((rgb.b-rgb.r)/max(luma,0.001));
float clearSky=smoothstep(0.08,0.35,blue);
float solar=smoothstep(PEAK*0.55,PEAK*0.9,luma);
float sunBreak=exp((dot(ray,ToSun)-1)*120)*smoothstep(1.0,2.2,luma);
return lerp(0.12,1.0,max(max(clearSky,solar),sunBreak));
'''.replace('PEAK',f'{sun.z:.5f}')
    else:
        nodes['Camera']=expr(mat,unreal.MaterialExpressionCameraPositionWS)
        nodes['Gain']=scalar(mat,'Gain',0.18)
        code=projection+r'''
float3 ray=normalize(P-Camera);
float3 rgb=Texture2DSampleLevel(Picture,PictureSampler,projection.uv(ray,ToSun,AnchorU,AnchorElevation),0).rgb;
float luma=dot(rgb,float3(0.2126,0.7152,0.0722));
rgb=lerp(luma.xxx,rgb,0.65);
rgb*=lerp(float3(0.70,0.84,1.0),float3(1.05,1.0,0.90),smoothstep(0.3,2.0,luma));
float sunlight=smoothstep(0.8,2.4,luma)*exp((dot(ray,ToSun)-1)*80);
float sunCore=exp((dot(ray,ToSun)-1)*6000);
return pow(max(rgb,0.0),1.10)*Gain+float3(1.0,0.86,0.64)*(sunlight*4.0+sunCore*6.0);
'''
    node=expr(mat,unreal.MaterialExpressionCustom)
    node.set_editor_property('code',code)
    node.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1 if is_mask else unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    pins=[]
    for key in nodes:
        p=unreal.CustomInput(); p.set_editor_property('input_name',key); pins.append(p)
    node.set_editor_property('inputs',pins)
    for key,n in nodes.items():
        if not MEL.connect_material_expressions(n,'',node,key): raise RuntimeError(key)
    MEL.connect_material_property(node,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.layout_material_expressions(mat)
    errors=MEL.recompile_material(mat)
    if errors: raise RuntimeError(str(errors))
    if not unreal.DCMaelstromAtmosphere.compile_sky_material(mat): raise RuntimeError('Shader compile failed: '+name)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
(project/'Saved/SkyAssetReview/source_sun.json').write_text(json.dumps({'u':sun.x,'v':sun.y,'peak':sun.z,'elevation_degrees':math.degrees(source_elevation)},indent=2))
unreal.log_warning('DC_RESULT Maelstrom photographic sky and shadow saved')
