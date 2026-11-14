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
if not M.connect_material_expressions(world,'',pos,''): raise RuntimeError('Cliff local position disconnected')
vn=node(unreal.MaterialExpressionVertexNormalWS)
normal=node(unreal.MaterialExpressionTransform)
normal.set_editor_property('transform_source_type',unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
normal.set_editor_property('transform_type',unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
if not M.connect_material_expressions(vn,'',normal,''): raise RuntimeError('Cliff vertex normal disconnected')
vc=node(unreal.MaterialExpressionVertexColor)
textures={}
for name,suffix in [('RockColor','D'),('RockNormal','N')]:
    tex=unreal.load_asset('/Game/AncientRuins/Textures/Tile/T_Ground_02_'+suffix)
    if not tex: raise RuntimeError('Installed mineral detail missing: '+suffix)
    obj=node(unreal.MaterialExpressionTextureObjectParameter)
    obj.set_editor_property('parameter_name',name); obj.set_editor_property('texture',tex)
    obj.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix=='N' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    textures[name]=obj
    unreal.log_warning('DC_ROCK_TEXTURE '+name+' '+tex.get_path_name()+' compression='+str(tex.get_editor_property('compression_settings'))+' srgb='+str(tex.get_editor_property('srgb')))

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
// Mineral regions follow irregular 3D volumes, not horizontal strata. The old
// height-biased field reinforced the continuous curtain visible in review A.
float3 warp=float3(sn.value(p*.009),sn.value(p*.009+19.2),sn.value(p*.009-31.7));
float mineralMass=sn.value(p*.016+warp*1.8);
float fracture=sn.value(p*.057+warp*.7);
float strata=saturate(.70*mineralMass+.30*fracture);
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
# A ground normal repeated every 1.6m or 12m averages away on a cliff kilometres
# from the camera. Add real angular fracture normals at a separate 22m scale;
# retain the installed normal map for fine grain rather than magnifying it.
fractures=custom(projection+r'''
struct FracturePlanes {
 float3 hash(float2 p) {
  return frac(sin(float3(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3)),dot(p,float2(419.2,371.9))))*43758.5453);
 }
 float3 evaluate(float2 p) {
  float2 cell=floor(p); float best=0; float2 gradient=0;
  [unroll] for(int j=-1;j<=1;j++) [unroll] for(int i=-1;i<=1;i++) {
   float2 id=cell+float2(i,j); float3 h=hash(id);
   float2 d=p-id-(.18+.64*h.xy),extent=.46+.30*h.yz;
   float2 tilt=(h.zx-.5)*.6;
   float v=.42+dot(d,tilt); float2 g=tilt;
   float sx=1-abs(d.x+.16*d.y)/extent.x;
   float sy=1-abs(d.y-.12*d.x)/extent.y;
   if(sx<v) {v=sx;g=-sign(d.x+.16*d.y)*float2(1,.16)/extent.x;}
   if(sy<v) {v=sy;g=-sign(d.y-.12*d.x)*float2(-.12,1)/extent.y;}
   if(v>best) {best=v;gradient=g;}
  }
  return float3(gradient,1-smoothstep(.08,.36,best));
 }
}; FracturePlanes fp;
float3 fx=fp.evaluate(ux/22),fy=fp.evaluate((uy+float2(11,7))/22),fz=fp.evaluate((uz+float2(3,17))/22);
float footprint=max(length(ddx(p)),length(ddy(p)));
float fade=1-smoothstep(10,32,footprint);
// 3.5m relief over a22m fracture unit. Explicit derivatives preserve planar
// faces, unlike a smoothly interpolated low-frequency bump texture.
float3 g=(float3(0,fx.x*s.x,fx.y)*w.x+float3(fy.x*s.y,0,fy.y)*w.y+float3(fz.x*s.z,fz.y,0)*w.z)*(.159*fade);
return float4(-g,dot(float3(fx.z,fy.z,fz.z),w)*fade);
''',unreal.CustomMaterialOutputType.CMOT_FLOAT4,{'P':pos,'N':normal})
color=custom(projection+r'''
float3 tx=Texture2DSample(RockColor,RockColorSampler,ux/4).rgb;
float3 ty=Texture2DSample(RockColor,RockColorSampler,uy/4).rgb;
float3 tz=Texture2DSample(RockColor,RockColorSampler,uz/4).rgb;
float3 mineral=tx*w.x+ty*w.y+tz*w.z;
float luminance=dot(mineral,float3(.2126,.7152,.0722));
// Mineral variation without the source ground texture's saturated dirt tint.
float mineralMix=smoothstep(.24,.76,Weather.x);
float3 stone=lerp(float3(.175,.198,.215),float3(.335,.321,.290),mineralMix);
float grain=clamp(.70+luminance*.90,.72,1.16);
float runoff=lerp(.90,1.03,Weather.y);
return stone*grain*runoff*lerp(1,.78,Weather.w)*lerp(1,.82,Fracture.w)*lerp(1,.62,Weather.z);
''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,{'P':pos,'N':normal,'Weather':weather,'Fracture':fractures,'RockColor':textures['RockColor']})
output(color,unreal.MaterialProperty.MP_BASE_COLOR)
detail=custom(projection+r'''
// Physical scales: fine mineral fracture 1.6m, intermediate normal detail12m.
// Texture mip filtering handles distance; no enlarged distant texture features.
float2 x=UnpackNormalMap(Texture2DSample(RockNormal,RockNormalSampler,ux/1.6)).xy;
float2 y=UnpackNormalMap(Texture2DSample(RockNormal,RockNormalSampler,uy/1.6)).xy;
float2 z=UnpackNormalMap(Texture2DSample(RockNormal,RockNormalSampler,uz/1.6)).xy;
float2 mx=UnpackNormalMap(Texture2DSample(RockNormal,RockNormalSampler,(ux+float2(3.1,5.7))/12)).xy;
float2 my=UnpackNormalMap(Texture2DSample(RockNormal,RockNormalSampler,(uy+float2(3.1,5.7))/12)).xy;
float2 mz=UnpackNormalMap(Texture2DSample(RockNormal,RockNormalSampler,(uz+float2(3.1,5.7))/12)).xy;
x=x*.52+mx*.30; y=y*.52+my*.30; z=z*.52+mz*.30;
float3 g=float3(0,x.x*s.x,x.y)*w.x+float3(y.x*s.y,0,y.y)*w.y+float3(z.x*s.z,z.y,0)*w.z+Fracture.xyz;
return normalize(n+g-n*dot(g,n));
''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,{'P':pos,'N':normal,'Fracture':fractures,'RockNormal':textures['RockNormal']})
to_world=node(unreal.MaterialExpressionTransform)
to_world.set_editor_property('transform_source_type',unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
to_world.set_editor_property('transform_type',unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
if not M.connect_material_expressions(detail,'',to_world,''): raise RuntimeError('Cliff normal world transform disconnected')
output(to_world,unreal.MaterialProperty.MP_NORMAL)
# Spray-darkened rock remains microscopically rough. A glossy wet apron under
# the low sun looked like a second metallic water surface in the cliff review.
output(custom('return lerp(.70+Weather.x*.11,.56,Weather.z);',unreal.CustomMaterialOutputType.CMOT_FLOAT1,{'Weather':weather}),unreal.MaterialProperty.MP_ROUGHNESS)
output(custom('return lerp(1,.72,Weather.w)*lerp(1,.86,Fracture.w);',unreal.CustomMaterialOutputType.CMOT_FLOAT1,{'Weather':weather,'Fracture':fractures}),unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
output(constant(.32),unreal.MaterialProperty.MP_SPECULAR)
for prop,expected in [(unreal.MaterialProperty.MP_BASE_COLOR,color),(unreal.MaterialProperty.MP_NORMAL,to_world)]:
    if M.get_material_property_input_node(mat,prop)!=expected:
        raise RuntimeError('Cliff material output is disconnected: '+str(prop))
if M.get_material_property_input_node(mat,unreal.MaterialProperty.MP_ROUGHNESS) is None:
    raise RuntimeError('Cliff roughness disconnected')
M.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
unreal.log_warning('DC_ROCK_MATERIAL_COMPLETE physical stone 1.6m/4m/12m +22m angular fracture normals; world normal connected; R wet G recess')
