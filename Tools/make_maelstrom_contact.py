"""Original lit spray and wet tentacle materials. Run only in the isolated project."""
import unreal
M = unreal.MaterialEditingLibrary
root = '/Game/DC/Maelstrom'
def make(name):
    a = unreal.load_asset(root+'/'+name)
    if a:
        for n in list(M.get_material_expressions(a)): M.delete_material_expression(a,n)
    else:
        a=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,root,unreal.Material,unreal.MaterialFactoryNew())
    return a
def scalar(a,v):
    n=M.create_material_expression(a,unreal.MaterialExpressionConstant); n.set_editor_property('r',v); return n
def rgb(a,v):
    n=M.create_material_expression(a,unreal.MaterialExpressionConstant3Vector); n.set_editor_property('constant',unreal.LinearColor(*v)); return n
def custom(a,code,kind,inputs):
    n=M.create_material_expression(a,unreal.MaterialExpressionCustom)
    n.set_editor_property('code',code); n.set_editor_property('output_type',kind)
    pins=[]
    for key in inputs:
        p=unreal.CustomInput(); p.set_editor_property('input_name',key); pins.append(p)
    n.set_editor_property('inputs',pins)
    for key,(source,pin) in inputs.items(): M.connect_material_expressions(source,pin,n,key)
    return n
f1=unreal.CustomMaterialOutputType.CMOT_FLOAT1
f3=unreal.CustomMaterialOutputType.CMOT_FLOAT3
a=make('M_ContactSpray')
a.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
a.set_editor_property('two_sided',True)
a.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_DIRECTIONAL)
uv=M.create_material_expression(a,unreal.MaterialExpressionTextureCoordinate)
vc=M.create_material_expression(a,unreal.MaterialExpressionVertexColor)
# Smooth value noise, never periodic sine stripes on camera-facing spray.
spray_noise="""
float2 q=UV*7, i=floor(q), f=frac(q); f=f*f*(3-2*f);
float a=frac(sin(dot(i,float2(127.1,311.7)))*43758.5453);
float b=frac(sin(dot(i+float2(1,0),float2(127.1,311.7)))*43758.5453);
float c=frac(sin(dot(i+float2(0,1),float2(127.1,311.7)))*43758.5453);
float d=frac(sin(dot(i+1,float2(127.1,311.7)))*43758.5453);
float n=lerp(lerp(a,b,f.x),lerp(c,d,f.x),f.y);
float2 p=UV*2-1; float edge=saturate(1-dot(p,p)-n*.2);
return edge*edge*(.35+.65*n)*Alpha*.48;
"""
op=custom(a,spray_noise,f1,{'UV':(uv,''),'Alpha':(vc,'A')})
M.connect_material_property(op,'',unreal.MaterialProperty.MP_OPACITY)
M.connect_material_property(rgb(a,(0.72,0.84,0.88,1)),'',unreal.MaterialProperty.MP_BASE_COLOR)
M.connect_material_property(scalar(a,.65),'',unreal.MaterialProperty.MP_ROUGHNESS)
M.recompile_material(a); unreal.EditorAssetLibrary.save_loaded_asset(a)
a=make('M_LeviathanWet')
uv=M.create_material_expression(a,unreal.MaterialExpressionTextureCoordinate)
# UV.x is metres along the arm / 7; UV.y wraps its circumference.
pattern = """
float filter=1-saturate(max(fwidth(UV.x*173),fwidth(UV.y*139)));
float pores=0.5+0.5*sin(UV.x*173)*sin(UV.y*139)*filter;
float belly=pow(saturate(sin(UV.y*6.28318)),3);
float row=min(abs(UV.y-.19),abs(UV.y-.31));
float along=(frac(UV.x*2)-.5)*2;
float across=row*65;
float cup=sqrt(along*along+across*across);
// HLSL pow on a negative base can produce NaN even with exponent 2.
// A bad normal can contaminate the temporal lighting/upscaling history.
float ringDistance=(cup-.62)*8;
float rim=exp(-ringDistance*ringDistance)*belly;
float hollow=exp(-cup*cup*9)*belly;
"""
color=custom(a,pattern+'return lerp(float3(.035,.065,.060),float3(.18,.135,.085),belly)*(.86+pores*.14)+rim*float3(.09,.07,.045)-hollow*.024;',f3,{'UV':(uv,'')})
M.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
normal=custom(a,pattern+'float sy=abs(UV.y-.19)<abs(UV.y-.31) ? (UV.y-.19)*65 : (UV.y-.31)*65; float g=-16*(cup-.62)*rim; return normalize(float3(-g*along/max(cup,.01)*.1,-g*sy/max(cup,.01)*.1,1));',f3,{'UV':(uv,'')})
M.connect_material_property(normal,'',unreal.MaterialProperty.MP_NORMAL)
M.connect_material_property(scalar(a,.40),'',unreal.MaterialProperty.MP_ROUGHNESS)
M.connect_material_property(scalar(a,.30),'',unreal.MaterialProperty.MP_SPECULAR)
M.recompile_material(a); unreal.EditorAssetLibrary.save_loaded_asset(a)
a=make('M_LeviathanWeakpoint')
uv=M.create_material_expression(a,unreal.MaterialExpressionTextureCoordinate)
flesh=custom(a,'float vein=.65+.35*sin(UV.x*53+sin(UV.y*31)*2); return float3(.32,.065,.025)*vein;',f3,{'UV':(uv,'')})
M.connect_material_property(flesh,'',unreal.MaterialProperty.MP_BASE_COLOR)
M.connect_material_property(rgb(a,(.28,.035,.009,1)),'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
M.connect_material_property(scalar(a,.42),'',unreal.MaterialProperty.MP_ROUGHNESS)
M.recompile_material(a); unreal.EditorAssetLibrary.save_loaded_asset(a)
unreal.log_warning('DC_CONTACT_MATERIALS_COMPLETE')
