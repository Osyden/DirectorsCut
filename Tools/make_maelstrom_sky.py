"""Original finale cloud volume; references stock engine noise, no vendor copies."""
import unreal

mel = unreal.MaterialEditingLibrary
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
#define NZ(q) saturate((Texture3DSampleLevel(Shape,ShapeSampler,(q),0).r-0.835)/0.125)
float3 p=P*0.00001;
float3 worldKm=p;
p.z-=RimHeight;
if(Debug>0.5) return Density;
if(p.z<0.55 || p.z>5.05) return 0;
// Slow advection, a coherent shape across every render and encounter seek.
float3 q=p-float3(T*0.0009,T*0.00025,0);
// Sub-bank turbulence distorts the analytic lobes without moving their anchors.
// Independent, offset hybrid-noise samples avoid a repeated cellular/foam pattern.
float3 turbulence=float3(NZ(q*0.50+0.21),NZ(q.zxy*0.53+0.73),NZ(q.yzx*0.57+0.41))-0.5;
float3 shaped=q+turbulence*0.18;
float2 cell=floor(q.xy/1.6);
float field=-10;
// Overlapping cumulus banks with independent widths, heights and offsets.
// A turbulent condensation floor breaks the hanging spherical undersides.
[unroll] for(int y=-1;y<=1;++y) {
 [unroll] for(int x=-1;x<=1;++x) {
  float2 id=cell+float2(x,y);
  float3 rnd=frac(sin(float3(dot(id,float2(127.1,311.7)),dot(id,float2(269.5,183.3)),dot(id,float2(419.2,371.9))))*43758.5453);
  float2 center=(id+0.5+(rnd.xy-0.5)*0.8)*1.6;
  float width=0.40+rnd.z*0.60;
  float top=1.1+rnd.x*1.0;
  float3 d=(shaped-float3(center,0.95+top*0.30))/float3(width,width*(0.8+rnd.y*0.4),top*0.55);
  float body=1-length(d);
  float3 head=(shaped-float3(center+float2(0.20,-0.15)*width,1.0+top*0.65))/float3(width*0.75,width*0.72,top*0.48);
  body=max(body,(1-length(head))*0.85);
  body=min(body,(shaped.z-(0.72+rnd.y*0.18))*3.5);
  field=max(field,body);
 }
}
if(field < -0.5) return 0;
float n=NZ(q*0.14+0.27);
float detail=NZ(q*0.42+0.61);
float fine=NZ(q*1.2+0.13);
// Large billows carry smaller rounded lobes; high frequencies only erode edges.
// Preserve the midpoint of the accepted density boundary while narrowing its ramp.
float sculpt=field+(n-0.45)*0.42+(detail-0.5)*0.30;
sculpt+=(fine-0.5)*0.055;
float d=saturate((sculpt-0.035)*6.5);
d=saturate(d-(1-fine)*Erosion*(1-d));
// An irregular angular window follows the actual sun, framed by thick shoulders.
float3 ray=normalize(worldKm-float3(0,0,0.02));
float3 right=normalize(cross(ToSun,float3(0,0,1)));
float3 up=normalize(cross(right,ToSun));
float2 window=float2(dot(ray,right)+0.035,dot(ray,up)-0.03);
float angle=length(window*float2(0.7,1.3));
float aperture=smoothstep(Opening*0.35,Opening+0.10,angle+(n-0.5)*0.28+(detail-0.5)*0.08);
// Erode the shape rather than multiplying a cylindrical hole through a solid bank.
d=saturate(d-(1-aperture)*1.2);
return d*Density;
'''
node = expr(unreal.MaterialExpressionCustom)
node.set_editor_property('code', code)
node.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
shape = expr(unreal.MaterialExpressionTextureObject)
shape.set_editor_property('texture', unreal.load_asset('/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128'))
shape.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
nodes = {'P':expr(unreal.MaterialExpressionWorldPosition), 'T':scalar('Clock',300), 'RimHeight':scalar('RimHeight',0), 'Shape':shape,
         'Density':scalar('Density',0.035), 'Debug':scalar('Debug',0), 'Erosion':scalar('Erosion',0.28), 'Opening':scalar('Opening',0.23),
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
for name,value in [('const_phase_g',0.45),('const_phase_g2',-0.2),('const_phase_blend',0.25),
                   ('const_multi_scattering_contribution',0.32),('const_multi_scattering_occlusion',0.65),
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
