"""Build the maelstrom water, rim cascade and spray materials in the current project.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Water UV0 is physical surface distance / 10000 cm. Vertex color R marks foam,
G is radial fraction, B is depth. Supply proper mesh tangents and normals.
Spray uses UV0 across/along a curtain, vertex R density, B upper edge, A fade.
HullFlatten (default 0) creates a horizontal shelf at SeaLevel (default 230 cm)
inside a hull-shaped ellipse with 24m/6m longitudinal/lateral semiaxes,
joining the bowl by 70m/17.5m. Both WPO and pixel normal account for the shelf transition. Portal/tunnel instances leave it off. TunnelFlow=1 selects aft flow along UV.x.
Basin flow uses UV-space radial/tangential directions and follows actor rotation.
Cascade UV0 is physical rim arc length / 100m in U and distance DOWN from the lip
/ 100m in V. Its tangent follows +U. Clock animates its falling streaks; FallSpeed
is metres/second, CascadeDepth is centimetres and CascadeFoam scales white water.
Cascade vertex B is normalized distance down the mesh (0 lip, 1 base). The last
quarter frays through to the underlying bowl; the basin must continue beneath it.
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
    if not MEL.connect_material_property(node, "", prop):
        raise RuntimeError("Could not bind material output " + str(prop))


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
    float3 noise_gradient(float2 p) {
        float2 i=floor(p), f=frac(p), u=f*f*(3.0-2.0*f), du=6.0*f*(1.0-f);
        float a=hash(i), b=hash(i+float2(1,0)), c=hash(i+float2(0,1)), d=hash(i+1);
        return float3(lerp(lerp(a,b,u.x),lerp(c,d,u.x),u.y),
                      du.x*lerp(b-a,d-c,u.y),du.y*lerp(c-a,d-b,u.x));
    }
    float3 wave_field(float2 p) {
        // Irregular 13-45m lobes. The same height field drives WPO and its normal.
        // Analytic derivatives avoid eight extra height evaluations per pixel.
        float3 nx=noise_gradient(p*0.019+13.4), ny=noise_gradient(p*0.023-27.1);
        float2 warp=float2(nx.x,ny.x)-0.5;
        float2 gx=nx.yz*0.019, gy=ny.yz*0.023;
        float2 q=p+warp*12.0;
        float a=dot(q,float2(0.085,0.12));
        float2 ga=float2(0.085,0.12)+12.0*(0.085*gx+0.12*gy);
        float b=dot(q,float2(-0.23,0.14))+sin(a)*0.6;
        float2 gb=float2(-0.23,0.14)+12.0*(-0.23*gx+0.14*gy)+cos(a)*ga*0.6;
        float c=dot(q,float2(0.41,0.22))+sin(b)*0.45;
        float2 gc=float2(0.41,0.22)+12.0*(0.41*gx+0.22*gy)+cos(b)*gb*0.45;
        float h=sin(a)*0.53+sin(b)*0.31+sin(c)*0.16;
        float2 gradient=cos(a)*ga*0.53+cos(b)*gb*0.31+cos(c)*gc*0.16;
        return float3(h,gradient);
    }
    float waves(float2 p) { return wave_field(p).x; }
    float cascade(float2 p) {
        // No sinusoidal corrugation: regular ridges become metallic cables in
        // grazing sun. Wide nonperiodic folds carry only a gentle perturbation.
        float warp=(noise(p*float2(0.018,0.012))-0.5)*24.0;
        float2 q=p+float2(warp,0.0);
        return (noise(q*float2(0.035,0.014))*2.0-1.0)*0.65
             +(noise(q*float2(0.085,0.028)+11.0)*2.0-1.0)*0.27
             +(noise(q*float2(0.19,0.06)-7.0)*2.0-1.0)*0.08;
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
normal_strength = scalar(water, "NormalStrength", 0.95)
foam_coverage = scalar(water, "FoamCoverage", 1.1)
hull_flatten = scalar(water, "HullFlatten", 0.0)
sea_level = scalar(water, "SeaLevel", 230.0)
base_inputs = {"UV": uv, "VC": vc, "P": position, "T": time, "Center": center, "Flow": flow, "Tunnel": tunnel_flow}

foam_code = NOISE + FLOW + r"""
// Broad islands and folded edges survive a kilometre-scale view. There are no
// regular sine stripes: white water fractures into connected but irregular sheets.
float2 warp1=float2(wn.noise(q*0.026),wn.noise(q*0.021+9.0))-0.5;
float2 warp2=float2(wn.noise(q2*0.026),wn.noise(q2*0.021+9.0))-0.5;
float2 f1=q+warp1*18.0;
float2 f2=q2+warp2*18.0;
float small=lerp(wn.fbm(f1*0.32),wn.fbm(f2*0.32),blend);
float breakup=lerp(wn.fbm(f1*0.09),wn.fbm(f2*0.09),blend);
float large=lerp(wn.fbm(f1*0.018),wn.fbm(f2*0.018),blend);
float island=smoothstep(0.29,0.62,large+breakup*0.16);
// A separate, hard-enough 10-35m mask must be able to remove crest foam entirely.
// Threshold each advection phase BEFORE blending: blending raw noise first
// narrows it toward 0.5 and previously turned nearly every crest into a ribbon.
float patchNoise1=wn.noise(f1*0.037)*0.72+wn.noise(f1*0.095+17.0)*0.28;
float patchNoise2=wn.noise(f2*0.037)*0.72+wn.noise(f2*0.095+17.0)*0.28;
float patches=lerp(smoothstep(0.43,0.65,patchNoise1),smoothstep(0.43,0.65,patchNoise2),blend);
patches=smoothstep(0.20,0.76,patches);
// The mesh now carries roughly 10m ridges normalized by 16m. A 0.72 threshold
// mostly rejected their peaks. This selects breaking tops while patches still
// decide WHERE on each crest white water is allowed to form.
float crest=smoothstep(0.035,0.30,VC.r);
float wave=lerp(wn.waves(q),wn.waves(q2),blend);
float waveCrest=smoothstep(0.28,0.72,wave);
float fold1=1.0-abs(wn.noise(f1*0.12)*2.0-1.0);
float fold2=1.0-abs(wn.noise(f2*0.12)*2.0-1.0);
float lace=smoothstep(0.59,0.88,lerp(fold1,fold2,blend));
float2 direction=flow/max(length(flow),0.01);
float2 across=float2(-direction.y,direction.x);
float trail1=wn.fbm(float2(dot(f1,across)*0.11,dot(f1,direction)*0.022));
float trail2=wn.fbm(float2(dot(f2,across)*0.11,dot(f2,direction)*0.022));
float trail=smoothstep(0.48,0.72,lerp(trail1,trail2,blend));
float crestSheet=crest*patches*(0.80+breakup*0.20);
float tornEdge=crest*lace*patches*0.25;
float whitecap=waveCrest*smoothstep(0.35,0.60,small)*(0.35+patches*0.65)*0.70;
// Falling water leaves broad turbulent rafts down the outer slope, rather than
// restricting every patch of white water to the mesh's long regular crests.
float rimChurn=smoothstep(0.60,0.84,VC.g)*(1.0-smoothstep(0.96,1.0,VC.g));
float wash=rimChurn*patches*smoothstep(0.40,0.64,breakup)*0.75;
float crestFoam=crestSheet+tornEdge+trail*crest*patches*0.18;
float coverage=max(crestFoam,max(whitecap,wash));
// Meso-scale (2-5m) turbulence remains visible from across the bowl, unlike
// sub-metre pores. It varies within each patch and erodes its perimeter.
float detail1=wn.fbm(f1*0.23)*0.65+wn.noise(f1*0.48+17.0)*0.35;
float detail2=wn.fbm(f2*0.23)*0.65+wn.noise(f2*0.48+17.0)*0.35;
float turbulence=lerp(detail1,detail2,blend);
float filament1=wn.noise(float2(dot(f1,across)*0.40,dot(f1,direction)*0.14));
float filament2=wn.noise(float2(dot(f2,across)*0.40,dot(f2,direction)*0.14));
float filaments=smoothstep(0.48,0.74,lerp(filament1,filament2,blend));
float fray=(turbulence-0.5)*0.26+(filaments-0.5)*0.10;
float white=smoothstep(0.08,0.48,coverage*Coverage+fray*smoothstep(0.02,0.12,coverage));
float interior=smoothstep(0.32,0.68,turbulence);
return saturate(white*(0.20+interior*0.65+filaments*0.15));
"""
foam = custom(water, foam_code, FLOAT, dict(base_inputs, Coverage=foam_coverage))
color = custom(water, r"""
float3 body=lerp(float3(0.012,0.043,0.052),float3(0.005,0.020,0.028),saturate(VC.b));
float3 foamColor=lerp(float3(0.74,0.81,0.80),float3(0.64,0.73,0.79),saturate(VC.b));
return lerp(body,foamColor,Foam);
""", FLOAT3, {"VC":vc,"Foam":foam}, 100)
output(color, unreal.MaterialProperty.MP_BASE_COLOR)
roughness = custom(water, "return lerp(0.30,0.68,sqrt(saturate(Foam)));", FLOAT, {"Foam":foam}, 200)
output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
output(scalar(water,"Specular",0.18),unreal.MaterialProperty.MP_SPECULAR)
opacity = custom(water,"return lerp(0.20,1.0,Foam);",FLOAT,{"Foam":foam},300)
output(opacity,unreal.MaterialProperty.MP_OPACITY)

normal_texture = unreal.load_asset("/DynamicRealWater/Textures/T_DRW_Ocean_DetailNormal")
normal_inputs = dict(base_inputs, Strength=normal_strength, Foam=foam, Amp=wave_height)
if normal_texture:
    for index, coordinate in enumerate(("q", "q2")):
        sample_uv = custom(water,FLOW+"return "+coordinate+"*0.075;",FLOAT2,base_inputs,400+index*100)
        sample = expression(water,unreal.MaterialExpressionTextureSampleParameter2D,-700,500+index*100)
        sample.set_editor_property("parameter_name","WaterDetailNormal")
        sample.set_editor_property("texture",normal_texture)
        sample.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_expressions(sample_uv,"",sample,"UVs")
        normal_inputs["Detail"+str(index)] = sample

normal_code = NOISE + FLOW + r"""
float nearHull=smoothstep(650.0,5000.0,length(P.xy));
float2 slope=-lerp(wn.wave_field(q).yz,wn.wave_field(q2).yz,blend)*(min(max(Amp,0.0),180.0)*0.01);
slope*=lerp(0.18,1.0,nearHull);
"""
if normal_texture:
    normal_code += "float detailAmount=lerp(0.22,0.06,smoothstep(10000.0,30000.0,length(P.xy)));\n"
    normal_code += "slope += lerp(Detail0.xy,Detail1.xy,blend) * detailAmount * (1.0-Foam*0.7);\n"
normal_code += "return normalize(float3(slope*Strength*(1.0-Foam*0.22),1.0));"
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

# Geometric displacement is bounded to WaveHeight (max 180cm), with the same
# advected lobes used by the normal. The mesh owns the much larger breaking ridges.
wpo_code = NOISE + FLOW + r"""
float h=lerp(wn.waves(q),wn.waves(q2),blend);
float nearHull=smoothstep(650.0,5000.0,length(P.xy));
float flatten=saturate(Flatten)*(1.0-smoothstep(1200.0,3500.0,length(P.xy*float2(0.5,2.0))));
float3 waveNormal=normalize(lerp(VN,float3(0,0,1),flatten));
float3 wave=waveNormal*h*min(max(Amp,0),180.0)*lerp(0.18,1.0,nearHull);
return wave+float3(0,0,(Level-P.z)*flatten);
"""
output(custom(water,wpo_code,FLOAT3,dict(base_inputs,VN=vertex_normal,Amp=wave_height,Flatten=hull_flatten,Level=sea_level),800),unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

volume = expression(water,unreal.MaterialExpressionSingleLayerWaterMaterialOutput,0,1000)
for input_name, value, name in [
    ("ScatteringCoefficients",(0.00013,0.00025,0.00032,1),"Scattering"),
    ("AbsorptionCoefficients",(0.0042,0.0022,0.0014,1),"Absorption")]:
    node=vector(water,name,value)
    if name == "Scattering":
        # Keep the abyss darker/cooler than the sunlit lip without a screen-space
        # haze layer. This also avoids a uniform teal body at every bowl depth.
        node=custom(water,"return Coefficient*lerp(1.0,0.68,saturate(VC.b));",FLOAT3,{"Coefficient":node,"VC":vc},1050)
    if not MEL.connect_material_expressions(node,"",volume,input_name):
        raise RuntimeError("Could not bind single layer water " + input_name)
MEL.connect_material_expressions(scalar(water,"ScatterDirection",0.35),"",volume,"PhaseG")
MEL.recompile_material(water)
unreal.EditorAssetLibrary.save_loaded_asset(water,only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_MaelstromWater saved; DRW detail normal=%s" % bool(normal_texture))

# The steep lip is a separate masked sheet. Single Layer Water's deep-volume
# approximation is inappropriate here: this material supplies actual lit white
# water and dark channels, while the mesh and spray own the silhouette and mist.
cascade = make_material("M_MaelstromCascade")
cascade.set_editor_property("blend_mode",unreal.BlendMode.BLEND_MASKED)
cascade.set_editor_property("opacity_mask_clip_value",0.33)
cascade.set_editor_property("shading_model",unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
cascade.set_editor_property("tangent_space_normal",False)
cascade_uv=expression(cascade,unreal.MaterialExpressionTextureCoordinate,-1400,0)
cascade_vc=expression(cascade,unreal.MaterialExpressionVertexColor,-1400,150)
cascade_vn=expression(cascade,unreal.MaterialExpressionVertexNormalWS,-1400,300)
cascade_t=scalar(cascade,"Clock",0.0)
fall_speed=scalar(cascade,"FallSpeed",28.0)
cascade_depth=scalar(cascade,"CascadeDepth",45.0)
cascade_foam_strength=scalar(cascade,"CascadeFoam",1.0)
cascade_strength=scalar(cascade,"NormalStrength",0.25)
cascade_inputs={"UV":cascade_uv,"VC":cascade_vc,"T":cascade_t,"Speed":fall_speed}
CASCADE_FLOW=r"""
// V increases down the wall. Positive speed transports the pattern downward.
float2 p=UV*100.0;
float2 q=p-float2(0.0,T*Speed);
// Two scales of lateral drift merge/split the streams as they descend; neither
// the colour nor its normal has a repeating one-dimensional stripe generator.
float shift=(wn.fbm(q*float2(0.009,0.012))-0.5)*42.0;
float2 bent=q+float2(shift,0.0);
float split=(wn.noise(bent*float2(0.034,0.023)+8.0)-0.5)*16.0;
float2 streaming=bent+float2(split,0.0);
float baseBlend=smoothstep(0.64,1.0,saturate(VC.b));
"""
cascade_foam_code=NOISE+CASCADE_FLOW+r"""
float broad=wn.fbm(streaming*float2(0.010,0.006));
float braids=wn.fbm(streaming*float2(0.045,0.020));
float threads=wn.fbm(streaming*float2(0.16,0.065));
float broken=smoothstep(0.20,0.64,broad+threads*0.20);
float white=smoothstep(0.30,0.72,braids)*(0.66+threads*0.34)*broken;
// A rolling white lip becomes long separated strings as it falls. Keep dark
// gaps through the sheet instead of a uniformly white, self-lit curtain.
float lip=exp(-max(p.y,0.0)*0.075)*smoothstep(0.25,0.58,broad)*0.5;
float fine=smoothstep(0.48,0.76,threads)*broken*0.12;
float lowerFoam=baseBlend*smoothstep(0.40,0.68,wn.fbm(p*0.045))*0.18;
return saturate((white+lip+fine+VC.r*0.06*broken)*Amount*(1.0-baseBlend*0.5)+lowerFoam);
"""
cascade_foam=custom(cascade,cascade_foam_code,FLOAT,dict(cascade_inputs,Amount=cascade_foam_strength),100)
cascade_color=custom(cascade,NOISE+CASCADE_FLOW+r"""
float channels=wn.fbm(streaming*float2(0.022,0.015));
float3 body=lerp(DeepColor,WaterColor,smoothstep(0.25,0.8,channels));
body=lerp(body,float3(0.014,0.052,0.064),baseBlend);
return lerp(body,FoamColor,Foam);
""",FLOAT3,dict(cascade_inputs,Foam=cascade_foam,
    DeepColor=vector(cascade,"DeepColor",(0.040,0.075,0.095,1)),
    WaterColor=vector(cascade,"WaterColor",(0.13,0.205,0.24,1)),
    FoamColor=vector(cascade,"FoamColor",(0.62,0.72,0.76,1))),200)
output(cascade_color,unreal.MaterialProperty.MP_BASE_COLOR)
output(custom(cascade,"return lerp(0.42,0.76,sqrt(saturate(Foam)));",FLOAT,{"Foam":cascade_foam},300),unreal.MaterialProperty.MP_ROUGHNESS)
output(scalar(cascade,"Specular",0.20),unreal.MaterialProperty.MP_SPECULAR)
output(custom(cascade,NOISE+CASCADE_FLOW+r"""
// The base is an overlap onto the existing water mesh. Irregular torn openings
// uncover that surface progressively instead of ending at a perfectly round line.
float tear=wn.fbm(p*float2(0.055,0.07));
float fringe=saturate((VC.b-0.70)/0.30);
return 1.0-fringe*(1.45+tear*0.95);
""",FLOAT,cascade_inputs,350),unreal.MaterialProperty.MP_OPACITY_MASK)
cascade_normal=custom(cascade,NOISE+CASCADE_FLOW+r"""
float e=0.3;
float2 dx=float2(e,0), dy=float2(0,e);
float2 slope=-float2(wn.cascade(q+dx)-wn.cascade(q-dx),wn.cascade(q+dy)-wn.cascade(q-dy));
slope*=clamp(Depth,0.0,120.0)*0.01/(2.0*e);
return normalize(float3(slope*Strength*(1.0-Foam*0.2)*(1.0-baseBlend*0.7),1.0));
""",FLOAT3,dict(cascade_inputs,Depth=cascade_depth,Strength=cascade_strength,Foam=cascade_foam),400)
cascade_normal_world=expression(cascade,unreal.MaterialExpressionTransform,-100,500)
if not MEL.connect_material_expressions(cascade_normal,"",cascade_normal_world,""):
    raise RuntimeError("Could not bind cascade normal transform")
output(cascade_normal_world,unreal.MaterialProperty.MP_NORMAL)
output(custom(cascade,NOISE+CASCADE_FLOW+r"""
return normalize(VN)*wn.cascade(q)*clamp(Depth,0.0,120.0)*(1.0-baseBlend);
""",FLOAT3,dict(cascade_inputs,VN=cascade_vn,Depth=cascade_depth),600),unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
MEL.recompile_material(cascade)
unreal.EditorAssetLibrary.save_loaded_asset(cascade,only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_MaelstromCascade saved; V points down, FallSpeed in m/s")

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
