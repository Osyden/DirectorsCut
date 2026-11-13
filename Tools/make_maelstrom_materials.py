"""Build the maelstrom water and spray materials in the current project.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Water UV0 is physical surface distance / 10000 cm. Vertex color R marks foam,
G is radial fraction, B is depth. Supply proper mesh tangents and normals.
Spray uses UV0 across/along a curtain, vertex R density, B upper edge, A fade.
HullFlatten (default 0) creates a horizontal shelf at SeaLevel (default 230 cm)
inside a hull-shaped ellipse with 24m/6m longitudinal/lateral semiaxes,
joining the bowl by 70m/17.5m. Both WPO and pixel normal account for the shelf transition. Portal/tunnel instances leave it off. TunnelFlow=1 selects aft flow along UV.x.
Basin flow uses UV-space radial/tangential directions and follows actor rotation.
Optional DRW textures are referenced in place and never copied or modified.
"""
import unreal

ROOT = "/Game/DC/Maelstrom"
MEL = unreal.MaterialEditingLibrary
FLOAT = unreal.CustomMaterialOutputType.CMOT_FLOAT1
FLOAT2 = unreal.CustomMaterialOutputType.CMOT_FLOAT2
FLOAT3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3


def expression(material, cls, x=0, y=0):
    return MEL.create_material_expression(material, cls, x, y)


def scalar(material, name, value):
    node = expression(material, unreal.MaterialExpressionScalarParameter, -1100)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def vector(material, name, value):
    node = expression(material, unreal.MaterialExpressionVectorParameter, -1100)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(*value))
    return node


def custom(material, code, output_type, inputs, y=0):
    node = expression(material, unreal.MaterialExpressionCustom, -350, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output_type)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    node.set_editor_property("inputs", pins)
    for name, source in inputs.items():
        source_node, source_pin = source if isinstance(source, tuple) else (source, "")
        if not MEL.connect_material_expressions(source_node, source_pin, node, name):
            raise RuntimeError("Could not bind custom input " + name)
    return node


def output(node, prop):
    MEL.connect_material_property(node, "", prop)


def make_material(name):
    path = ROOT + "/" + name
    material = unreal.load_asset(path)
    if material:
        # UE 5.8 DeleteAllMaterialExpressions iterates the collection while removing
        # from it. Delete a snapshot instead so custom outputs cannot survive a rerun.
        for previous in list(MEL.get_material_expressions(material)):
            MEL.delete_material_expression(material, previous)
        if MEL.get_num_material_expressions(material) != 0:
            raise RuntimeError("Material graph was not fully cleared: " + path)
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("two_sided", True)
    return material


NOISE = r"""
struct WaterNoise {
    float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
    float noise(float2 p) {
        float2 i=floor(p), f=frac(p); f=f*f*(3-2*f);
        return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),
                    lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);
    }
    float fbm(float2 p) {
        return noise(p)*0.57+noise(p*2.03+7.2)*0.28+noise(p*4.11-3.4)*0.15;
    }
};
WaterNoise wn;
"""

# Phases alternate every seven seconds so advected detail cannot stretch indefinitely.
FLOW = r"""
// UV is basin-local XY / 100 metres, so flow stays aligned when the arena rotates.
float2 delta=UV*100.0;
float2 radial=delta/max(length(delta),1.0);
float2 tangential=float2(-radial.y,radial.x);
float2 basinFlow=-radial*3.5+tangential*8.0;
// Tunnel UV.x runs forward along its axis. Water rushes aft with slight side drift.
float2 flow=lerp(basinFlow,float2(-18.0,2.0),saturate(Tunnel))*Flow;
float phase=frac(T*0.07);
float phase2=frac(phase+0.5);
float blend=abs(phase*2.0-1.0);
float2 q=UV*100.0-flow*(phase/0.07);
float2 q2=UV*100.0-flow*(phase2/0.07);
"""

water = make_material("M_MaelstromWater")
water.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
water.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
water.set_editor_property("tangent_space_normal", False)
uv = expression(water, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
vc = expression(water, unreal.MaterialExpressionVertexColor, -1400, 150)
position = expression(water, unreal.MaterialExpressionWorldPosition, -1400, 300)
position.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
vertex_normal = expression(water, unreal.MaterialExpressionVertexNormalWS, -1400, 450)
time = scalar(water, "Clock", 0.0)
center = vector(water, "BowlCenter", (0, 0, 0, 1))
flow = scalar(water, "FlowStrength", 1.0)
tunnel_flow = scalar(water, "TunnelFlow", 0.0)
wave_height = scalar(water, "WaveHeight", 115.0)
normal_strength = scalar(water, "NormalStrength", 0.65)
hull_flatten = scalar(water, "HullFlatten", 0.0)
sea_level = scalar(water, "SeaLevel", 230.0)
base_inputs = {"UV": uv, "VC": vc, "P": position, "T": time, "Center": center, "Flow": flow, "Tunnel": tunnel_flow}

foam_code = NOISE + FLOW + r"""
float small=lerp(wn.fbm(q*0.65),wn.fbm(q2*0.65),blend);
float breakup=lerp(wn.fbm(q*0.13),wn.fbm(q2*0.13),blend);
// Broad islands survive the far-rim pixel footprint and interrupt uniform white terraces.
float large=lerp(wn.fbm(q*0.023),wn.fbm(q2*0.023),blend);
float island=smoothstep(0.36,0.66,large);
float crest=smoothstep(0.30,0.86,VC.r);
float2 direction=flow/max(length(flow),0.01);
float2 across=float2(-direction.y,direction.x);
float stripe1=pow(saturate(sin(dot(q,across)*0.7+wn.noise(q*0.065)*4.0)*0.5+0.5),6.0);
float stripe2=pow(saturate(sin(dot(q2,across)*0.7+wn.noise(q2*0.065)*4.0)*0.5+0.5),6.0);
float streak=lerp(stripe1,stripe2,blend);
float whitecap=smoothstep(0.70,0.88,small)*smoothstep(0.46,0.82,breakup)*0.24;
float crestFoam=crest*island*(0.25+1.05*small)*smoothstep(0.2,0.62,breakup);
return saturate(crestFoam+whitecap+streak*crest*island*0.18);
"""
foam = custom(water, foam_code, FLOAT, base_inputs)
color = custom(water, r"""
float3 body=lerp(float3(0.012,0.056,0.069),float3(0.004,0.018,0.030),saturate(VC.b));
return lerp(body,float3(0.72,0.80,0.77),Foam);
""", FLOAT3, {"VC":vc,"Foam":foam}, 100)
output(color, unreal.MaterialProperty.MP_BASE_COLOR)
roughness = custom(water, "return lerp(0.13,0.58,Foam);", FLOAT, {"Foam":foam}, 200)
output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
output(scalar(water,"Specular",0.28),unreal.MaterialProperty.MP_SPECULAR)
opacity = custom(water,"return lerp(0.12,1.0,Foam);",FLOAT,{"Foam":foam},300)
output(opacity,unreal.MaterialProperty.MP_OPACITY)

normal_texture = unreal.load_asset("/DynamicRealWater/Textures/T_DRW_Ocean_DetailNormal")
normal_inputs = dict(base_inputs, Strength=normal_strength, Foam=foam)
if normal_texture:
    for index, coordinate in enumerate(("q", "q2")):
        sample_uv = custom(water,FLOW+"return "+coordinate+"*0.21;",FLOAT2,base_inputs,400+index*100)
        sample = expression(water,unreal.MaterialExpressionTextureSampleParameter2D,-700,500+index*100)
        sample.set_editor_property("parameter_name","WaterDetailNormal")
        sample.set_editor_property("texture",normal_texture)
        sample.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_expressions(sample_uv,"",sample,"UVs")
        normal_inputs["Detail"+str(index)] = sample

normal_code = FLOW + r"""
float2 slope1=float2(cos(q.x*2.2+q.y*0.5)*0.13+cos(q.x*5.7-q.y*1.8)*0.07,
                    cos(q.y*2.9-q.x*0.8)*0.16+cos(q.x*2.1+q.y*6.3)*0.06);
float2 slope2=float2(cos(q2.x*2.2+q2.y*0.5)*0.13+cos(q2.x*5.7-q2.y*1.8)*0.07,
                    cos(q2.y*2.9-q2.x*0.8)*0.16+cos(q2.x*2.1+q2.y*6.3)*0.06);
float2 slope=lerp(slope1,slope2,blend);
"""
if normal_texture:
    normal_code += "slope += lerp(Detail0.xy,Detail1.xy,blend) * 0.4;\n"
normal_code += "return normalize(float3(slope*Strength*(1.0-Foam*0.8),1.0));"
detail_normal = custom(water,normal_code,FLOAT3,normal_inputs,600)
# Transform the detailed tangent normal to world space before reshaping the shelf normal.
detail_world = expression(water,unreal.MaterialExpressionTransform,-200,650)
if not MEL.connect_material_expressions(detail_normal,"",detail_world,""):
    raise RuntimeError("Could not bind water normal transform")
flatten_normal_code = r"""
float3 baseNormal=normalize(VN);
float3 detail=normalize(DetailWS);
if (Flatten <= 0.0 || baseNormal.z < 0.02) { return detail; }
float radius=max(length(P.xy*float2(0.5,2.0)),1.0);
float t=saturate((radius-1200.0)/2300.0);
float weight=saturate(Flatten)*(1.0-t*t*(3.0-2.0*t));
float derivative=-saturate(Flatten)*6.0*t*(1.0-t)/2300.0;
float2 slope=-baseNormal.xy/max(baseNormal.z,0.02);
float2 shelfSlope=(1.0-weight)*slope+(Level-P.z)*derivative*(P.xy*float2(0.25,4.0)/radius);
float3 shelfNormal=normalize(float3(-shelfSlope,1.0));
// The detailed ripple is a small perturbation; preserve it around the changed base normal.
return normalize(shelfNormal+(detail-baseNormal));
"""
output(custom(water,flatten_normal_code,FLOAT3,{"VN":vertex_normal,"DetailWS":detail_world,"P":position,"Flatten":hull_flatten,"Level":sea_level},700),unreal.MaterialProperty.MP_NORMAL)

# Geometric displacement is bounded to 1.2 x WaveHeight and fades near the hull.
wpo_code = r"""
float2 p=UV*100.0;
float h=sin(dot(p,float2(0.11,0.18))-T*1.1)*0.56
       +sin(dot(p,float2(-0.26,0.15))-T*1.6)*0.31
       +sin(dot(p,float2(0.48,0.17))-T*2.2)*0.13;
float nearHull=smoothstep(650.0,5000.0,length(P.xy));
float flatten=saturate(Flatten)*(1.0-smoothstep(1200.0,3500.0,length(P.xy*float2(0.5,2.0))));
float3 waveNormal=normalize(lerp(VN,float3(0,0,1),flatten));
float3 wave=waveNormal*h*min(max(Amp,0),180.0)*lerp(0.18,1.0,nearHull);
return wave+float3(0,0,(Level-P.z)*flatten);
"""
output(custom(water,wpo_code,FLOAT3,{"UV":uv,"P":position,"T":time,"VN":vertex_normal,"Amp":wave_height,"Flatten":hull_flatten,"Level":sea_level},800),unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

volume = expression(water,unreal.MaterialExpressionSingleLayerWaterMaterialOutput,0,1000)
for input_name, value, name in [
    ("ScatteringCoefficients",(0.00016,0.00034,0.00039,1),"Scattering"),
    ("AbsorptionCoefficients",(0.0042,0.0017,0.0009,1),"Absorption")]:
    node=vector(water,name,value)
    if not MEL.connect_material_expressions(node,"",volume,input_name):
        raise RuntimeError("Could not bind single layer water " + input_name)
MEL.connect_material_expressions(scalar(water,"ScatterDirection",0.35),"",volume,"PhaseG")
MEL.recompile_material(water)
unreal.EditorAssetLibrary.save_loaded_asset(water,only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_MaelstromWater saved; DRW detail normal=%s" % bool(normal_texture))

spray = make_material("M_MaelstromSpray")
spray.set_editor_property("blend_mode",unreal.BlendMode.BLEND_TRANSLUCENT)
spray.set_editor_property("shading_model",unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
spray_uv=expression(spray,unreal.MaterialExpressionTextureCoordinate,-1100)
spray_vc=expression(spray,unreal.MaterialExpressionVertexColor,-1100,150)
spray_t=scalar(spray,"Clock",0)
reveal=scalar(spray,"Reveal",1)
density=scalar(spray,"Density",0.55)
spray_code=NOISE+r"""
float2 q=float2(UV.x*16.0,UV.y*5.0-T*0.55);
float n=wn.fbm(q)+wn.noise(q*3.1)*0.20;
float edge=smoothstep(0.0,0.18,UV.x)*smoothstep(0.0,0.18,1.0-UV.x);
float vertical=smoothstep(0.0,0.14,UV.y)*pow(saturate(1.0-UV.y),1.3);
return saturate(smoothstep(0.35,0.75,n)*edge*vertical*Density*(0.25+VC.r*0.75)*Alpha*Reveal);
"""
output(custom(spray,spray_code,FLOAT,{"UV":spray_uv,"VC":spray_vc,"Alpha":(spray_vc,"A"),"T":spray_t,"Reveal":reveal,"Density":density}),unreal.MaterialProperty.MP_OPACITY)
output(vector(spray,"SprayColor",(0.65,0.75,0.80,1)),unreal.MaterialProperty.MP_BASE_COLOR)
output(scalar(spray,"SprayRoughness",0.8),unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(spray)
unreal.EditorAssetLibrary.save_loaded_asset(spray,only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_MaelstromSpray saved")
