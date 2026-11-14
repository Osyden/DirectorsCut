"""Physical-scale cliff stone. Unit-scale basin mesh: R=wet, G=recess.
Installed tile textures are referenced in place, never copied.
"""
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
def output(n,p):
    if not M.connect_material_property(n,'',p): raise RuntimeError(str(p))
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
textures={}
for name,suffix in [('RockColor','D'),('RockNormal','N')]:
    tex=unreal.load_asset('/Game/AncientRuins/Textures/Tile/T_Ground_02_'+suffix)
    if not tex: raise RuntimeError('Installed mineral detail missing: '+suffix)
    obj=node(unreal.MaterialExpressionTextureObjectParameter)
    obj.set_editor_property('parameter_name',name); obj.set_editor_property('texture',tex)
    obj.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix=='N' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    textures[name]=obj

# Geometry supplies fractured volume. Weathering stays restrained and never
# paints giant round mottles or regular masonry joints across the whole cliff.
weather=custom(r'''
struct StoneNoise {
 float hash(float3 p) { return frac(sin(dot(p,float3(127.1,311.7,74.7)))*43758.5453); }
 float value(float3 p) {
  float3 i=floor(p),f=frac(p); f=f*f*(3-2*f);
  return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
   lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),lerp(hash(i+float3(0,1,1)),hash(i+1),f.x),f.y),f.z);
 }
}; StoneNoise sn;
float3 p=P*.01;
// Tilted, interrupted bedding. Broad transitions avoid painted contour lines.
float bedding=p.z*.075+dot(p.xy,float2(.0014,-.0011));
float strata=sn.value(float3(p.xy*.011,bedding+sn.value(p*.006)*1.7));
float streaks=sn.value(float3(p.xy*.12,p.z*.004));
float wet=saturate(Wet)*lerp(.65,1,smoothstep(.28,.72,streaks));
return float4(strata,streaks,wet,saturate(Recess));
''',unreal.CustomMaterialOutputType.CMOT_FLOAT4,{'P':pos,'Wet':(vc,'R'),'Recess':(vc,'G')})
projection=r'''
float3 p=P*.01,n=normalize(N),w=pow(abs(n),4); w/=max(dot(w,1),.001);
// Mirror projection coordinates and gradients together around the crater.
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float2 ux=float2(p.y*s.x,p.z),uy=float2(p.x*s.y,p.z),uz=float2(p.x*s.z,p.y);
'''
color=custom(projection+r'''
float3 tx=Texture2DSample(RockColor,RockColorSampler,ux/4).rgb;
float3 ty=Texture2DSample(RockColor,RockColorSampler,uy/4).rgb;
float3 tz=Texture2DSample(RockColor,RockColorSampler,uz/4).rgb;
float3 mineral=tx*w.x+ty*w.y+tz*w.z;
float luminance=dot(mineral,float3(.2126,.7152,.0722));
// Mineral variation without the source ground texture's saturated dirt tint.
float3 stone=lerp(float3(.255,.265,.265),float3(.305,.295,.272),Weather.x);
float grain=clamp(.70+luminance*.90,.72,1.16);
float runoff=lerp(.90,1.03,Weather.y);
return stone*grain*runoff*lerp(1,.78,Weather.w)*lerp(1,.62,Weather.z);
''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,{'P':pos,'N':normal,'Weather':weather,'RockColor':textures['RockColor']})
output(color,unreal.MaterialProperty.MP_BASE_COLOR)
detail=custom(projection+r'''
// Physical scales: fine mineral fracture 1.6m, intermediate normal detail12m.
// Texture mip filtering handles distance; no enlarged distant texture features.
float2 x=Texture2DSample(RockNormal,RockNormalSampler,ux/1.6).xy*2-1;
float2 y=Texture2DSample(RockNormal,RockNormalSampler,uy/1.6).xy*2-1;
float2 z=Texture2DSample(RockNormal,RockNormalSampler,uz/1.6).xy*2-1;
float2 mx=Texture2DSample(RockNormal,RockNormalSampler,(ux+float2(3.1,5.7))/12).xy*2-1;
float2 my=Texture2DSample(RockNormal,RockNormalSampler,(uy+float2(3.1,5.7))/12).xy*2-1;
float2 mz=Texture2DSample(RockNormal,RockNormalSampler,(uz+float2(3.1,5.7))/12).xy*2-1;
x=x*.52+mx*.30; y=y*.52+my*.30; z=z*.52+mz*.30;
float3 g=float3(0,x.x*s.x,x.y)*w.x+float3(y.x*s.y,0,y.y)*w.y+float3(z.x*s.z,z.y,0)*w.z;
return normalize(n+g-n*dot(g,n));
''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,{'P':pos,'N':normal,'RockNormal':textures['RockNormal']})
to_world=node(unreal.MaterialExpressionTransform)
to_world.set_editor_property('transform_source_type',unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
to_world.set_editor_property('transform_type',unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
M.connect_material_expressions(detail,'',to_world,'')
output(to_world,unreal.MaterialProperty.MP_NORMAL)
# Spray-darkened rock remains microscopically rough. A glossy wet apron under
# the low sun looked like a second metallic water surface in the cliff review.
output(custom('return lerp(.78+Weather.x*.09,.56,Weather.z);',unreal.CustomMaterialOutputType.CMOT_FLOAT1,{'Weather':weather}),unreal.MaterialProperty.MP_ROUGHNESS)
output(custom('return lerp(1,.72,Weather.w);',unreal.CustomMaterialOutputType.CMOT_FLOAT1,{'Weather':weather}),unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
output(constant(.32),unreal.MaterialProperty.MP_SPECULAR)
M.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
unreal.log_warning('DC_ROCK_MATERIAL_COMPLETE physical stone 1.6m/4m/12m; R wet G recess')
