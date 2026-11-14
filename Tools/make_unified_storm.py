"""Generate the continuous storm front and fog-filled throat: /Game/DC/Materials/M_UnifiedStorm.

Run with Unreal closed through Tools/py.sh. Only this generated material is replaced.
DCFogFront supplies ride time, reveal progress and density. Local voxel coordinates map
to metres using Unit and the (650, 0, 800) centre; keep these in sync with its actor bounds.
No source textures or purchased assets are needed. The old VDB assembly remains available
through -DCLegacyFront for comparisons. See taskboard/STORM-SCULPT-ITERATIONS.md.
"""
import unreal
mel = unreal.MaterialEditingLibrary
path = '/Game/DC/Materials/M_UnifiedStorm'
if unreal.EditorAssetLibrary.does_asset_exist(path):
    mat = unreal.load_asset(path)
    mel.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_UnifiedStorm', '/Game/DC/Materials', unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_VOLUME)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property('used_with_heterogeneous_volumes', True)

def scalar(name, value):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
    n.set_editor_property('parameter_name', name); n.set_editor_property('default_value', value)
    return n
def vector(name, value):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter)
    n.set_editor_property('parameter_name', name); n.set_editor_property('default_value', unreal.LinearColor(*value))
    return n
def custom(code, inputs, kind):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionCustom)
    n.set_editor_property('code', code); n.set_editor_property('output_type', kind)
    pins = []
    for name in inputs:
        p = unreal.CustomInput(); p.set_editor_property('input_name', name); pins.append(p)
    n.set_editor_property('inputs', pins)
    for name, node in inputs.items():
        assert mel.connect_material_expressions(node, '', n, name), name
    return n

lp = mel.create_material_expression(mat, unreal.MaterialExpressionLocalPosition)
unit = vector('Unit', (1000,1200,750,0))
t = scalar('StormTime',0)
opening = scalar('Opening',0)
density = scalar('Density',0)
fog = scalar('InteriorFog',0.025)
tint = vector('Tint',(0.65,0.69,0.75,1))
ext = custom('''
struct CloudNoise {
 float H(float3 p) { p=frac(p*0.3183099+float3(0.11,0.17,0.23)); p*=17; return frac(p.x*p.y*p.z*(p.x+p.y+p.z)); }
 float N(float3 p) {
  float3 i=floor(p), f=frac(p); f=f*f*(3-2*f);
  return lerp(lerp(lerp(H(i),H(i+float3(1,0,0)),f.x),lerp(H(i+float3(0,1,0)),H(i+float3(1,1,0)),f.x),f.y),
   lerp(lerp(H(i+float3(0,0,1)),H(i+float3(1,0,1)),f.x),lerp(H(i+float3(0,1,1)),H(i+float3(1,1,1)),f.x),f.y),f.z);
 }
 float W(float3 p) {
  float3 i=floor(p), f=frac(p); float dist=8;
  [unroll] for(int z=-1;z<=1;++z) [unroll] for(int y=-1;y<=1;++y) [unroll] for(int x=-1;x<=1;++x) {
   float3 c=float3(x,y,z), k=i+c;
   float3 h=float3(H(k),H(k+17.1),H(k+41.7));
   float3 d=c+0.15+0.7*h-f; dist=min(dist,dot(d,d));
  }
  return saturate(1-sqrt(dist));
 }
}; CloudNoise noise;
#define NOISE(q) noise.N(q)
float3 p = LP * Unit.xyz * 0.01 + float3(650,0,800);
float depth = saturate(p.x / 1550.0);
float2 radial = float2(p.y - 65*sin(depth*3.0), p.z - 280 - depth*35);
float r = length(radial);
float a = (0.003*p.x + 0.0015*r) * Opening - T*0.025;
float2 flow = float2(cos(a)*radial.x-sin(a)*radial.y, sin(a)*radial.x+cos(a)*radial.y);
float3 q = float3(p.x*0.65,flow) / 65.0 + float3(-T*0.005,0.17,0.4);
float big = NOISE(q*0.5);
float mid = noise.W(q);
float detail = noise.W(q*2.71+0.31);
float fine = NOISE(q*7.13+0.67);
float n = mid*0.58 + detail*0.28 + fine*0.14;
float billow = (big-0.5)*450 + (mid-0.5)*600 + (detail-0.5)*220 + (fine-0.5)*50;
float theta = atan2(radial.y,radial.x+0.0001);
float coil = sin(theta*2.0 + r*0.013 - depth*6.0 - T*0.04 + (big-0.5)*3.0);
// A ramped face and descending shoulders, not a rectangular wall.
float height = 1350 * saturate(1-pow(abs(p.y+180)/2700,1.45)) + 180*sin(p.y*0.0018+0.4);
float face = -70 + max(p.z,0)*0.60 + p.y*p.y*0.00012 + billow + 300*coil*exp(-r/1800)*lerp(0.05,1.0,Opening);
float mass = smoothstep(face-35,face+55,p.x);
mass *= 1-smoothstep(height-90+billow,height+80+billow,p.z);
mass *= (1-smoothstep(2700,3040,abs(p.y))) * (1-smoothstep(1520,1740,p.x));
mass *= smoothstep(-115,-40,p.z);
float curl = sin(theta*3 + depth*12 - T*0.075);
float radius = lerp(540,135,pow(depth,0.7)) * Opening * (1+0.14*curl);
float wall = smoothstep(radius-40, radius+70+billow*0.20, r);
wall = lerp(1.0, wall, smoothstep(0.0,0.12,Opening));
float cloud = mass * wall * (0.025+smoothstep(0.30,0.70,n));
// Mist occupies the hollow itself and becomes opaque in depth: never a window onto blue sky.
float mist = (1-wall) * smoothstep(120,520,p.x) * (1-smoothstep(1540,1750,p.x));
mist *= smoothstep(-115,-40,p.z) * (0.08+4*n*n*n);
return Density * (cloud*0.9 + mist*Fog*(1+4*smoothstep(650,1250,p.x)));
''', {'LP':lp,'Unit':unit,'T':t,'Opening':opening,'Density':density,'Fog':fog}, unreal.CustomMaterialOutputType.CMOT_FLOAT1)
em = custom('''
float3 p = LP * Unit.xyz * 0.01 + float3(650,0,800);
float pocket = exp(-dot((p-float3(800,30,220))/float3(420,400,300),(p-float3(800,30,220))/float3(420,400,300)));
float flash = pow(saturate(sin(T*1.73)),40)*pow(saturate(sin(T*0.37+1)),3);
return D * pocket * float3(0.22,0.27,0.34) * (0.24 + flash*1.5) * Opening;
''',{'LP':lp,'Unit':unit,'T':t,'D':ext,'Opening':opening},unreal.CustomMaterialOutputType.CMOT_FLOAT3)
assert mel.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
assert mel.connect_material_property(ext,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
assert mel.connect_material_property(em,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_asset(path,only_if_is_dirty=False)
unreal.log_warning('STORM_RESULT unified material saved')
