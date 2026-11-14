"""Original local-space triplanar crater rock; installed textures referenced in place."""
import unreal
M=unreal.MaterialEditingLibrary
path='/Game/DC/Maelstrom/M_CraterRock'
mat=unreal.load_asset(path)
if not mat:
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_CraterRock','/Game/DC/Maelstrom',unreal.Material,unreal.MaterialFactoryNew())
for n in list(M.get_material_expressions(mat)): M.delete_material_expression(mat,n)
mat.set_editor_property('tangent_space_normal',False)
def node(cls): return M.create_material_expression(mat,cls)
def custom(code,kind,inputs):
    n=node(unreal.MaterialExpressionCustom)
    n.set_editor_property('code',code); n.set_editor_property('output_type',kind)
    pins=[]
    for key in inputs:
        p=unreal.CustomInput(); p.set_editor_property('input_name',key); pins.append(p)
    n.set_editor_property('inputs',pins)
    for key,v in inputs.items():
        source,pin=v if isinstance(v,tuple) else (v,'')
        if not M.connect_material_expressions(source,pin,n,key): raise RuntimeError(key)
    return n
def output(n,p): M.connect_material_property(n,'',p)
def constant(x):
    n=node(unreal.MaterialExpressionConstant); n.set_editor_property('r',x); return n
world=node(unreal.MaterialExpressionWorldPosition)
pos=node(unreal.MaterialExpressionTransformPosition)
pos.set_editor_property('transform_source_type',unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
pos.set_editor_property('transform_type',unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
M.connect_material_expressions(world,'',pos,'')
vn=node(unreal.MaterialExpressionVertexNormalWS)
normal=node(unreal.MaterialExpressionTransform)
normal.set_editor_property('transform_source_type',unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
normal.set_editor_property('transform_type',unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
M.connect_material_expressions(vn,'',normal,'')
vc=node(unreal.MaterialExpressionVertexColor)
inputs={'P':pos,'N':normal,'Wet':(vc,'R')}
for name,suffix in [('RockColor','D'),('RockNormal','N')]:
    tex=unreal.load_asset('/Game/AncientRuins/Textures/Tile/T_Ground_02_'+suffix)
    if not tex: raise RuntimeError('Installed rock detail texture missing')
    obj=node(unreal.MaterialExpressionTextureObjectParameter)
    obj.set_editor_property('parameter_name',name); obj.set_editor_property('texture',tex)
    obj.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix=='N' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    inputs[name]=obj
noise=r'''
struct StoneNoise {
 float hash(float3 p) { return frac(sin(dot(p,float3(127.1,311.7,74.7)))*43758.5453); }
 float value(float3 p) {
  float3 i=floor(p),f=frac(p); f=f*f*(3-2*f);
  return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
   lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),lerp(hash(i+float3(0,1,1)),hash(i+1),f.x),f.y),f.z);
 }
}; StoneNoise sn;
float3 p=P*.01, w=pow(abs(normalize(N)),4); w/=max(dot(w,1),.001);
'''
color=custom(noise+r'''
float3 tx=Texture2DSample(RockColor,RockColorSampler,p.yz/5).rgb;
float3 ty=Texture2DSample(RockColor,RockColorSampler,p.xz/5).rgb;
float3 tz=Texture2DSample(RockColor,RockColorSampler,p.xy/5).rgb;
float macro=sn.value(p*.018)*.65+sn.value(p*.071)*.35;
float layers=sn.value(float3(p.xy*.012,p.z*.09+sn.value(p*.025)*4));
float strata=.70+.30*smoothstep(.15,.65,layers);
float3 stone=lerp(float3(.12,.14,.16),float3(.25,.265,.27),macro);
return lerp(stone,(tx*w.x+ty*w.y+tz*w.z)*.65,.25)*strata*lerp(1,.48,Wet);
''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,inputs)
output(color,unreal.MaterialProperty.MP_BASE_COLOR)
detail=custom(noise+r'''
float2 x=Texture2DSample(RockNormal,RockNormalSampler,p.yz/5).xy*2-1;
float2 y=Texture2DSample(RockNormal,RockNormalSampler,p.xz/5).xy*2-1;
float2 z=Texture2DSample(RockNormal,RockNormalSampler,p.xy/5).xy*2-1;
float3 g=float3(0,x.x,x.y)*w.x+float3(y.x,0,y.y)*w.y+float3(z.x,z.y,0)*w.z;
float3 n=normalize(N); return normalize(n+(g-n*dot(g,n))*.65);
''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,inputs)
to_world=node(unreal.MaterialExpressionTransform)
to_world.set_editor_property('transform_source_type',unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
to_world.set_editor_property('transform_type',unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
M.connect_material_expressions(detail,'',to_world,'')
output(to_world,unreal.MaterialProperty.MP_NORMAL)
output(custom('return lerp(.82,.37,Wet);',unreal.CustomMaterialOutputType.CMOT_FLOAT1,{'Wet':(vc,'R')}),unreal.MaterialProperty.MP_ROUGHNESS)
output(constant(.25),unreal.MaterialProperty.MP_SPECULAR)
M.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
unreal.log_warning('DC_ROCK_MATERIAL_COMPLETE')
