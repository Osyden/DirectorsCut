"""Original finale cloud volume and periodic cellular erosion; no vendor copies."""
import unreal

mel = unreal.MaterialEditingLibrary
noise_path = '/Game/DC/Maelstrom/Sky/T_MaelstromErosion'
noise = unreal.load_asset(noise_path)
if not noise:
    noise = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'T_MaelstromErosion', '/Game/DC/Maelstrom/Sky', unreal.VolumeTexture, unreal.VolumeTextureFactory())
if not unreal.DCMaelstromAtmosphere.build_sky_noise(noise):
    raise RuntimeError('Could not bake cloud erosion volume')
unreal.EditorAssetLibrary.save_loaded_asset(noise,False)
path = '/Game/DC/Maelstrom/Sky/M_MaelstromCumulus'
mat = unreal.load_asset(path)
if mat:
    mel.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_MaelstromCumulus', '/Game/DC/Maelstrom/Sky', unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_VOLUME)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property('used_with_volumetric_cloud', True)

def expr(cls):
    return mel.create_material_expression(mat, cls)

def scalar(name, value):
    n = expr(unreal.MaterialExpressionScalarParameter)
    n.set_editor_property('parameter_name', name)
    n.set_editor_property('default_value', value)
    return n

def vector(name, value):
    n = expr(unreal.MaterialExpressionVectorParameter)
    n.set_editor_property('parameter_name', name)
    n.set_editor_property('default_value', unreal.LinearColor(*value))
    return n

code = r'''
#define NOISE(q) Texture3DSampleLevel(Shape,ShapeSampler,(q),0)
float3 p=P*0.00001;
float3 worldKm=p;
p.z-=RimHeight;
if(Debug>0.5) return Density;
if(p.z<0.55 || p.z>5.05) return 0;
// Slow advection, a coherent shape across every render and encounter seek.
float3 q=p-float3(T*0.0009,T*0.00025,0);
float4 billows=NOISE(q*0.80+0.27);
// Bend the larger masses before carving them, breaking the ellipsoid profiles.
float3 warped=q+(billows.rgb-0.5)*0.22;
float2 cell=floor(q.xy/1.6);
float field=-10;
// Overlapping cumulus towers with independent widths, heights and offsets.
// Their rounded lobes are genuine density, visible in self-shadow and silhouette.
[unroll] for(int y=-1;y<=1;++y) {
 [unroll] for(int x=-1;x<=1;++x) {
  float2 id=cell+float2(x,y);
  float3 rnd=frac(sin(float3(dot(id,float2(127.1,311.7)),dot(id,float2(269.5,183.3)),dot(id,float2(419.2,371.9))))*43758.5453);
  float2 center=(id+0.5+(rnd.xy-0.5)*0.8)*1.6;
  float width=0.40+rnd.z*0.60;
  float top=1.1+rnd.x*1.0;
  float3 d=(warped-float3(center,0.95+top*0.30))/float3(width,width*(0.8+rnd.y*0.4),top*0.55);
  float body=1-length(d);
  float3 head=(warped-float3(center+float2(0.20,-0.15)*width,1.0+top*0.65))/float3(width*0.75,width*0.72,top*0.48);
  body=max(body,(1-length(head))*0.85);
  field=max(field,body);
 }
}
if(field < -1.0) return 0;
float n=billows.r, detail=billows.g;
// Distinct 310/155/78 m folds and a narrow density ramp retain the rims.
// The finest cells only bite into the silhouette instead of adding wispy opacity.
float sculpt=field+(n-0.50)*0.95+(detail-0.5)*0.45+(billows.b-0.5)*0.20;
sculpt-=0.03+(1-billows.a)*Erosion*0.22;
// An irregular angular window follows the actual sun, framed by thick shoulders.
float3 ray=normalize(worldKm-float3(0,0,0.02));
float3 right=normalize(cross(ToSun,float3(0,0,1)));
float3 up=normalize(cross(right,ToSun));
float2 window=float2(dot(ray,right)+0.035,dot(ray,up)-0.03);
float angle=length(window*float2(0.7,1.3));
float aperture=smoothstep(Opening*0.15,Opening,angle+(n-0.5)*0.08+(detail-0.5)*0.04);
// Carve the distance field BEFORE the opacity ramp. Carving an already clamped
// density left a translucent luminous shell around the entire opening.
sculpt-=(1-aperture)*1.30;
return saturate(sculpt*12.0)*Density;
'''
node = expr(unreal.MaterialExpressionCustom)
node.set_editor_property('code', code)
node.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
shape = expr(unreal.MaterialExpressionTextureObject)
shape.set_editor_property('texture', noise)
shape.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
nodes = {'P':expr(unreal.MaterialExpressionWorldPosition), 'T':scalar('Clock',300), 'RimHeight':scalar('RimHeight',0), 'Shape':shape,
         'Density':scalar('Density',0.035), 'Debug':scalar('Debug',0), 'Erosion':scalar('Erosion',0.35), 'Opening':scalar('Opening',0.14),
         'ToSun':vector('ToSun',(0.4,-0.85,0.4,1))}
inputs=[]
for name in nodes:
    i=unreal.CustomInput(); i.set_editor_property('input_name',name); inputs.append(i)
node.set_editor_property('inputs',inputs)
for name,n in nodes.items():
    if not mel.connect_material_expressions(n,'',node,name): raise RuntimeError(name)
mel.connect_material_property(node,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
mel.connect_material_property(vector('Albedo',(0.87,0.94,1,1)),'',unreal.MaterialProperty.MP_BASE_COLOR)
adv=expr(unreal.MaterialExpressionVolumetricAdvancedMaterialOutput)
for name,value in [('const_phase_g',0.55),('const_phase_g2',-0.2),('const_phase_blend',0.2),
                   ('const_multi_scattering_contribution',0.3),('const_multi_scattering_occlusion',0.4),
                   ('const_multi_scattering_eccentricity',0.5),('multi_scattering_approximation_octave_count',2),
                   ('ground_contribution',True),('gray_scale_material',False),('ray_march_volume_shadow',True)]:
    adv.set_editor_property(name,value)
mel.layout_material_expressions(mat)
errors=mel.recompile_material(mat)
if errors: raise RuntimeError(str(errors))
if not unreal.DCMaelstromAtmosphere.compile_sky_material(mat):
    raise RuntimeError('Cloud material did not finish compilation; use -AllowCommandletRendering, not -nullrhi')
unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
unreal.log_warning('DC_RESULT Maelstrom cumulus material saved')
