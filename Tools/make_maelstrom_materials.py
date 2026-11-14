"""Build the maelstrom water, rim cascade and spray materials in the current project.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Water UV0 is physical surface distance / 10000 cm. Vertex color R marks foam,
G is radial fraction, B is depth. With OverflowFoam=1, A is upstream overflow
supply: GetOverflowAmount(theta - (12/3.5)*log(clamp(radialFraction,.08,1))).
Supply proper mesh tangents and normals. Tunnel/portal leave OverflowFoam=0.
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
Cascade vertex R is overflow amount: 0 dry buttress, 1 full fall; it frays the sides.
Optional DRW textures are referenced in place and never copied or modified.
The DRW foam BaseColor was inspected as sRGB RGB (white cells over dark water);
linear red is used as foam density, never an assumed packed alpha channel.
UseLiveFFT defaults 0. When the optional DRW render-target arrays exist, a live
pixel-normal/foam and bounded vertex-displacement paths reference the installed
plugin helper. Hull displacement is suppressed; distant geometry is filtered.
DRWInvTileA/B, DRWShortestA/B and DRWSlopeVarA/B pack cascades 0..3 / 4..7.
Tile reciprocals are 1/cm, shortest wavelengths and foam level sizes are cm.
"""
import unreal

ROOT = "/Game/DC/Maelstrom"
MEL = unreal.MaterialEditingLibrary
FLOAT = unreal.CustomMaterialOutputType.CMOT_FLOAT1
FLOAT2 = unreal.CustomMaterialOutputType.CMOT_FLOAT2
FLOAT3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
FLOAT4 = unreal.CustomMaterialOutputType.CMOT_FLOAT4


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


def texture_sample(material, name, texture, coordinates, normal=False, y=0):
    node = expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -700, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", texture)
    node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    if not MEL.connect_material_expressions(coordinates, "", node, "UVs"):
        raise RuntimeError("Could not bind texture coordinates for " + name)
    return node


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
    float streak(float2 p, float2 direction) {
        // Filter in the local current direction without projecting absolute
        // coordinates into their own polar basis (which collapses into rings).
        float value=0;
        [unroll] for(int i=-2;i<=2;i++) { value+=noise((p+direction*i*4.0)/1.8); }
        return value*.2;
    }
    float3 noise_gradient(float2 p) {
        float2 i=floor(p), f=frac(p), u=f*f*(3.0-2.0*f), du=6.0*f*(1.0-f);
        float a=hash(i), b=hash(i+float2(1,0)), c=hash(i+float2(0,1)), d=hash(i+1);
        return float3(lerp(lerp(a,b,u.x),lerp(c,d,u.x),u.y),
                      du.x*lerp(b-a,d-c,u.y),du.y*lerp(c-a,d-b,u.x));
    }
    float3 wave_field(float2 p, float time) {
        // Modest wind waves, with independent phases and dispersion. Do not
        // nest phase warps: those produced the rubbery curls in grazing light.
        float h=0.0; float2 gradient=0.0;
        const float2 k[6]={float2(.13,.06),float2(.18,.13),float2(.29,-.08),
                          float2(.36,.18),float2(.10,-.39),float2(.49,.12)};
        const float weights[6]={.30,.23,.18,.13,.10,.06};
        [unroll] for(int i=0;i<6;i++) {
            float a=dot(p,k[i])-time*sqrt(9.81*length(k[i]))+i*2.37;
            h+=sin(a)*weights[i]; gradient+=cos(a)*k[i]*weights[i];
        }
        float3 e=noise_gradient(p*.027+float2(time*.015,19.7));
        float amp=.40+.60*e.x;
        float2 grad=gradient*amp+h*.60*.027*e.yz;
        h*=amp;
        return float3(h+.18*(h*h-.5),grad*(1+.36*h));
    }
    float waves(float2 p, float time) { return wave_field(p,time).x; }
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

# Flow phases crossfade over a 14.3-second cycle so advected detail cannot stretch indefinitely.
FLOW = r"""
// UV is basin-local XY / 100 metres, so flow stays aligned when the arena rotates.
float2 delta=UV*100.0;
float2 radial=delta/max(length(delta),1.0);
float2 tangential=float2(-radial.y,radial.x)*Handedness;
float arc=frac((3.14159265-atan2(delta.y,delta.x))/6.2831853)*6.2831853;
float inlet=smoothstep(0.0,.16,arc)*(1-smoothstep(.35,.60,arc));
float2 basinFlow=-radial*(3.5+12.0*inlet)+tangential*12.0;
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
water.set_editor_property("has_pixel_animation", True)
water.set_editor_property("refraction_method", unreal.RefractionMode.RM_NONE)
uv = expression(water, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
vc = expression(water, unreal.MaterialExpressionVertexColor, -1400, 150)
position = expression(water, unreal.MaterialExpressionWorldPosition, -1400, 300)
position.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
vertex_normal = expression(water, unreal.MaterialExpressionVertexNormalWS, -1400, 450)
camera = expression(water, unreal.MaterialExpressionCameraPositionWS, -1400, 600)
time = scalar(water, "Clock", 0.0)
center = vector(water, "BowlCenter", (0, 0, 0, 1))
flow = scalar(water, "FlowStrength", 1.0)
flow_handedness = scalar(water, "FlowHandedness", -1.0)
tunnel_flow = scalar(water, "TunnelFlow", 0.0)
wave_height = scalar(water, "WaveHeight", 115.0)
normal_strength = scalar(water, "NormalStrength", 0.95)
foam_coverage = scalar(water, "FoamCoverage", 1.1)
overflow_foam = scalar(water, "OverflowFoam", 0.0)
hull_flatten = scalar(water, "HullFlatten", 0.0)
sea_level = scalar(water, "SeaLevel", 230.0)
base_inputs = {"UV": uv, "VC": vc, "P": position, "T": time, "Center": center, "Flow": flow, "Tunnel": tunnel_flow, "Handedness":flow_handedness}

# Runtime bridge replaces these array references with the live grid's private
# render targets and copies its MPC values. No paid shader implementation is
# embedded here: the helper remains in the installed plugin and is included.
use_live_fft=scalar(water,"UseLiveFFT",0.0)
live_count=scalar(water,"DRWCascadeCount",0.0)
live_array_assets={
    "DRWDisplacementRT":unreal.load_asset("/DynamicRealWater/Textures/RenderTargets/RT_CascadeDisplacement"),
    "DRWDerivativesRT":unreal.load_asset("/DynamicRealWater/Textures/RenderTargets/RT_CascadeDerivatives"),
    "DRWFoamLevelsRT":unreal.load_asset("/DynamicRealWater/Textures/RenderTargets/RT_FoamLevels"),
}
fft_surface=None
fft_foam=None
fft_displacement=None
if all(live_array_assets.values()):
    live_objects={}
    for name,asset in live_array_assets.items():
        node=expression(water,unreal.MaterialExpressionTextureObjectParameter,-1500,1200)
        node.set_editor_property("parameter_name",name)
        node.set_editor_property("texture",asset)
        node.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        live_objects[name]=node
    live_vectors={}
    for name,default in {
        "DRWInvTileA":(0,0,0,0),"DRWInvTileB":(0,0,0,0),
        "DRWShortestA":(1,1,1,1),"DRWShortestB":(1,1,1,1),
        "DRWSlopeVarA":(0,0,0,0),"DRWSlopeVarB":(0,0,0,0),
        "DRWRoughness":(0.30,0,0,0),"DRWFoamLevelSizes":(1,1,0,0),
        "DRWWindDirection":(1,0,0,0),
    }.items():
        # Vector parameters default to RGB. Explicit RGBA is essential for the
        # fourth cascade and the four-component roughness coefficient packing.
        live_vectors[name]=(vector(water,name,default),"RGBA")
    live_common={"P":position,"Cam":camera,"Enabled":use_live_fft,"Count":live_count}
    surface_inputs=dict(live_common,
        Derivatives=live_objects["DRWDerivativesRT"],Displacement=live_objects["DRWDisplacementRT"],
        InvA=live_vectors["DRWInvTileA"],InvB=live_vectors["DRWInvTileB"],
        ShortA=live_vectors["DRWShortestA"],ShortB=live_vectors["DRWShortestB"],
        VarA=live_vectors["DRWSlopeVarA"],VarB=live_vectors["DRWSlopeVarB"],
        Rough=live_vectors["DRWRoughness"],Wind=live_vectors["DRWWindDirection"],
        Levels=live_objects["DRWFoamLevelsRT"],Sizes=live_vectors["DRWFoamLevelSizes"],
        Scale=scalar(water,"DRWNormalScale",1.0))
    fft_surface=custom(water,r"""
// The installed helper declares generic global constants. AdditionalDefines
// aliases them only while its header is parsed; restore engine names here.
#undef PI
#undef Luminance
LiveFoam=0.0;
LiveDisplacement=float3(0,0,0);
[branch] if (Enabled < 0.001 || Count < 0.5) { return float4(0,0,1,0.30); }
DRWWorldCascades fft;
#if VERTEXSHADER
// Use the installed vertex-safe helper, not the pixel Surface function.
// A minimum footprint respects our approximately 3–5m corridor mesh edges.
LiveDisplacement=fft.Displacement(Displacement,DisplacementSampler,P,InvA,InvB,
    ShortA,ShortB,Count,max(350.0,fft.VertexFootprintCm(P)));
return float4(0,0,1,0.30);
#else
float unusedFoam=0.0, unusedHeight=0.0, variance=0.0;
// Surface's Foam out parameter is unused in this plugin version. Accumulated
// foam lives in its separate camera-centred array and must use actual world XY.
LiveFoam=saturate(fft.FarFoam(Levels,LevelsSampler,P,Sizes,ddx(P.xy),ddy(P.xy),0.0));
// SampleGrad plus the helper's unresolved-slope treatment filter the cascades
// by the actual world-XY pixel footprint. A further distance fade is below.
return fft.Surface(Derivatives,DerivativesSampler,Displacement,DisplacementSampler,
    P,InvA,InvB,ShortA,ShortB,VarA,VarB,Count,ddx(P.xy),ddy(P.xy),Scale,Rough,
    normalize(Wind.xy+float2(1e-7,0)),normalize(Cam-P),unusedFoam,unusedHeight,variance);
#endif
""",FLOAT4,surface_inputs,1200)
    # A single expression exposes both stages. The project wrapper guards the
    # installed header when Unreal emits it again in vertex/pixel permutations.
    foam_pin=unreal.CustomOutput()
    foam_pin.set_editor_property("output_name","LiveFoam")
    foam_pin.set_editor_property("output_type",FLOAT)
    displacement_pin=unreal.CustomOutput()
    displacement_pin.set_editor_property("output_name","LiveDisplacement")
    displacement_pin.set_editor_property("output_type",FLOAT3)
    fft_surface.set_editor_property("additional_outputs",[foam_pin,displacement_pin])
    fft_displacement=(fft_surface,"LiveDisplacement")
    aliases=[]
    for symbol in ("PI","Luminance"):
        alias=unreal.CustomDefine()
        alias.set_editor_property("define_name",symbol)
        alias.set_editor_property("define_value","DC_DRW_"+symbol)
        aliases.append(alias)
    fft_surface.set_editor_property("additional_defines",aliases)
    fft_surface.set_editor_property("include_file_paths",["/DCMaelstrom/InstalledWater.ush"])
    fft_foam=(fft_surface,"LiveFoam")

# These are optional references to the user's installed pack, not imported copies.
foam_texture = unreal.load_asset("/DynamicRealWater/Textures/T_DRW_OceanFoam_BaseColor")
foam_normal_texture = unreal.load_asset("/DynamicRealWater/Textures/T_DRW_OceanFoam_Normal")
normal_texture = unreal.load_asset("/DynamicRealWater/Textures/T_DRW_Ocean_DetailNormal")
foam_inputs = dict(base_inputs, Coverage=foam_coverage, Flatten=hull_flatten, Cam=camera, Enabled=use_live_fft, Count=live_count,
                   Overflow=overflow_foam, Supply=(vc,"A"))
# Runtime binds a crest field generated from the actual bowl. Portal/tunnel
# leave Available=0. Advection moves foam off the ridge into a trailing wake.
crest_available=scalar(water,"CrestSourceAvailable",0.0)
crest_inv_span=scalar(water,"CrestSourceInvSpan",1.0/3968.0)
crest_default=unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
foam_inputs["SourceAvailable"]=crest_available
for index,coordinate in enumerate(("q","q2")):
    source_uv=custom(water,FLOW+"return "+coordinate+"*InvSpan+.5;",FLOAT2,dict(base_inputs,InvSpan=crest_inv_span))
    foam_inputs["Source"+str(index)]=texture_sample(water,"CrestSource",crest_default,source_uv)
foam_fine_uvs = []
if foam_texture:
    for index, coordinate in enumerate(("q", "q2")):
        raft_uv = custom(water,NOISE+FLOW+"float2 p="+coordinate+r""";
// Distorted planar cells have a physical scale and no angular seam or rings.
float2 warp=float2(wn.noise(p*0.031),wn.noise(p*0.027+19.3))-0.5;
float2 cell=float2(p.x*.8+p.y*.6,-p.x*.6+p.y*.8)+warp*13.0;
return lerp(cell/float2(9.0,17.0),float2(p.y/18.0,p.x/65.0),saturate(Tunnel));
""",FLOAT2,base_inputs,100+index*100)
        cell_uv = custom(water,FLOW+"float2 p="+coordinate+"; return float2(p.x*0.8+p.y*0.6,-p.x*0.6+p.y*0.8)/1.5;",FLOAT2,base_inputs,300+index*100)
        foam_fine_uvs.append(cell_uv)
        foam_inputs["Raft"+str(index)] = texture_sample(water,"FoamPattern",foam_texture,raft_uv,y=100+index*100)
        foam_inputs["Cells"+str(index)] = texture_sample(water,"FoamPattern",foam_texture,cell_uv,y=300+index*100)

foam_code = NOISE + FLOW + r"""
float shelf=saturate(Flatten)*(1-smoothstep(1200,3500,length(P.xy*float2(.5,2))));
float transported=lerp(Source0.r,Source1.r,blend);
// Fresh narrow lip plus a trailing source which loses bubbles as it travels.
float ageFade=lerp(exp(-phase*3.0),exp(-phase2*3.0),blend);
float source=lerp(VC.r,max(VC.r*.30,transported*ageFade),saturate(SourceAvailable));
float crest=smoothstep(.12,.62,source)*(1-shelf);
// Long torn filaments follow the current. A thin crest sheds a sparse wake;
// broad positive wave heights never become filled white islands.
// Use a FIXED planar basis for texture coordinates. Dotting absolute q
// with its own rotating radial basis collapses BOTH axes to radius and
// turns every noise sample into a concentric contour. Flow advection above
// supplies the local circular movement without that coordinate singularity.
float2 direction=normalize(flow+float2(.001,0));
float n=lerp(wn.streak(q,direction),wn.streak(q2,direction),blend);
float thread=smoothstep(.50,.66,n);
float fine=lerp(wn.noise(q*.9),wn.noise(q2*.9),blend);
float wave=lerp(wn.waves(q,T),wn.waves(q2,T),blend);
float cap=smoothstep(.34,.60,wave)*(1-smoothstep(45000.0,180000.0,length(P-Cam)))
    *(1-saturate(Enabled)*step(.5,Count));
float supply=saturate(Overflow)*Supply*smoothstep(.92,.99,VC.g);
float torn=lerp(wn.fbm(q*float2(.17,.23)),wn.fbm(q2*float2(.17,.23)),blend);
float breakup=smoothstep(.43,.64,torn);
float body=crest*breakup+cap*.75*thread+supply*.10*thread;
float veil=thread*.08*smoothstep(.4,.7,fine)*smoothstep(.35,.7,torn)*max(crest,cap);
float lace=smoothstep(.35,.70,fine);
"""
if foam_texture:
    foam_code += "float cells=lerp(Cells0.r,Cells1.r,blend); lace=smoothstep(.025,.24,cells);\n"
foam_code += "return saturate((body*lace*1.35+veil)*Coverage*(1-shelf*.8));"
foam = custom(water, foam_code, FLOAT, foam_inputs)
source_foam=foam
if fft_foam:
    live_foam_inputs={"Fallback":foam,"Live":fft_foam,"Enabled":use_live_fft,"Count":live_count}
    live_foam_code="float detail=0.15;\n"
    if foam_texture:
        live_foam_inputs.update({"C0":foam_inputs["Cells0"],"C1":foam_inputs["Cells1"],"T":time})
        live_foam_code="float blend=abs(frac(T*0.07)*2.0-1.0); float detail=lerp(smoothstep(.08,.50,C0.r),smoothstep(.08,.50,C1.r),blend);\n"
    # The flat-ocean accumulation field is a subtle wind-wave aeration layer,
    # not a dense white coating over the crater. Actual breaking lips are the
    # brighter source above; no constant floor fills the live accumulation mask.
    live_foam_code+="float liveWeight=saturate(Enabled)*step(0.5,Count); return max(Fallback,saturate(Live)*detail*.08*liveWeight);"
    foam=custom(water,live_foam_code,FLOAT,live_foam_inputs,1350)
color = custom(water, r"""
float3 body=lerp(float3(0.012,0.043,0.052),float3(0.005,0.020,0.028),saturate(VC.b));
float3 foamColor=lerp(float3(0.74,0.81,0.80),float3(0.64,0.73,0.79),saturate(VC.b));
return lerp(body,foamColor,Foam);
""", FLOAT3, {"VC":vc,"Foam":foam}, 100)
output(color, unreal.MaterialProperty.MP_BASE_COLOR)
roughness = custom(water, "return lerp(0.22,0.78,sqrt(saturate(Foam)));", FLOAT, {"Foam":foam}, 200)
if fft_surface:
    roughness=custom(water,r"""
float distanceM=length(P-Cam)*0.01;
float filtered=max(Surface.a,lerp(0.22,0.48,smoothstep(250.0,1400.0,distanceM)));
return lerp(Fallback,max(Fallback,filtered),saturate(Enabled)*step(0.5,Count));
""",FLOAT,{"Fallback":roughness,"Surface":fft_surface,"P":position,"Cam":camera,"Enabled":use_live_fft,"Count":live_count},1400)
output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
specular=scalar(water,"Specular",0.18)
output(specular,unreal.MaterialProperty.MP_SPECULAR)
opacity = custom(water,"return lerp(lerp(0.20,1.0,saturate(Foam*2.0)),1.0,saturate(Tunnel));",FLOAT,{"Foam":foam,"Tunnel":tunnel_flow},300)
output(opacity,unreal.MaterialProperty.MP_OPACITY)

normal_inputs = dict(base_inputs, Strength=normal_strength, Foam=foam, Amp=wave_height, Cam=camera)
if normal_texture:
    for index, coordinate in enumerate(("q", "q2")):
        sample_uv = custom(water,FLOW+"return "+coordinate+"/9.0;",FLOAT2,base_inputs,400+index*100)
        normal_inputs["Detail"+str(index)] = texture_sample(water,"WaterDetailNormal",normal_texture,sample_uv,normal=True,y=500+index*100)
if foam_texture and foam_normal_texture:
    for index, coordinates in enumerate(foam_fine_uvs):
        normal_inputs["FoamNormal"+str(index)] = texture_sample(water,"FoamNormal",foam_normal_texture,coordinates,normal=True,y=700+index*100)

normal_code = NOISE + FLOW + r"""
float nearHull=smoothstep(1200.0,4000.0,length(P.xy*float2(.5,2)));
float distanceM=length(P-Cam)*0.01;
float2 slope=-lerp(wn.wave_field(q,T).yz,wn.wave_field(q2,T).yz,blend)*(min(max(Amp,0.0),650.0)*0.01);
slope*=lerp(0.0,1.0,nearHull)*lerp(1.0,.18,smoothstep(180.0,1000.0,distanceM));
// Intermediate wind waves connect the large swells to the close surface detail.
// Fade unresolved frequencies with true camera distance, including crater depth.
float2 chop0=cos(dot(q,float2(0.55,0.28))-T*2.45)*float2(0.55,0.28)*0.18
            +cos(dot(q,float2(-0.91,0.43))-T*3.14)*float2(-0.91,0.43)*0.08;
float2 chop1=cos(dot(q2,float2(0.55,0.28))-T*2.45)*float2(0.55,0.28)*0.18
            +cos(dot(q2,float2(-0.91,0.43))-T*3.14)*float2(-0.91,0.43)*0.08;
slope-=lerp(chop0,chop1,blend)*(1.0-smoothstep(450.0,1500.0,distanceM))*(1.0-Foam*0.65);
"""
if normal_texture:
    normal_code += "float detailAmount=lerp(0.20,0.012,smoothstep(50.0,400.0,distanceM));\n"
    normal_code += "slope += lerp(Detail0.xy,Detail1.xy,blend) * detailAmount * (1.0-Foam*0.85);\n"
if foam_texture and foam_normal_texture:
    normal_code += r"""
float2 foamSlope=lerp(FoamNormal0.xy,FoamNormal1.xy,blend);
// Inverse of the rotation used by the paired 12m colour/normal samples.
foamSlope=float2(foamSlope.x*0.8-foamSlope.y*0.6,foamSlope.x*0.6+foamSlope.y*0.8);
slope+=foamSlope*Foam*lerp(0.30,0.04,smoothstep(80.0,650.0,distanceM));
"""
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
final_normal=custom(water,flatten_normal_code,FLOAT3,{"VN":vertex_normal,"DetailWS":detail_world,"P":position,"Flatten":hull_flatten,"Level":sea_level},700)
if fft_surface:
    # The simulation's XY derivatives are world aligned, not the bowl's rotating
    # UV tangent frame. Rotate its flat-water normal from world-up to the actual
    # curved/shelf normal; never treat its xyz as an already-curved world normal.
    shelf_base_code=flatten_normal_code.replace("float3 detail=normalize(DetailWS);", "float3 detail=baseNormal;").replace("return normalize(shelfNormal+(detail-baseNormal));", "return shelfNormal;")
    shelf_base=custom(water,shelf_base_code,FLOAT3,{"VN":vertex_normal,"P":position,"Flatten":hull_flatten,"Level":sea_level},1450)
    final_normal=custom(water,r"""
float weight=saturate(Enabled)*step(0.5,Count)*(1-smoothstep(12000,50000,length(P-Cam)));
[branch] if (weight < 0.001) { return Fallback; }
float3 base=normalize(Base);
float distanceM=length(P-Cam)*0.01;
float strength=lerp(0.65,0.16,smoothstep(180.0,1600.0,distanceM));
float3 flat=normalize(lerp(float3(0,0,1),Surface.xyz,strength));
float3 axis=float3(-base.y,base.x,0);
float3 curved=flat+cross(axis,flat)+cross(axis,cross(axis,flat))/max(1.0+base.z,0.01);
// Blend the wind spectrum with the underlying current-aligned water banks.
float3 live=normalize(curved+(Fallback-base)*0.65);
return normalize(lerp(Fallback,live,weight));
""",FLOAT3,{"Fallback":final_normal,"Base":shelf_base,"Surface":fft_surface,"P":position,"Cam":camera,"Enabled":use_live_fft,"Count":live_count},1500)
output(final_normal,unreal.MaterialProperty.MP_NORMAL)

# Geometric displacement is bounded to WaveHeight (max 650cm), with the same
# advected lobes used by the normal. The mesh owns the much larger breaking ridges.
wpo_code = NOISE + FLOW + r"""
float h=lerp(wn.waves(q,T),wn.waves(q2,T),blend);
float nearHull=smoothstep(1200.0,4000.0,length(P.xy*float2(.5,2)));
float flatten=saturate(Flatten)*(1.0-smoothstep(1200.0,3500.0,length(P.xy*float2(0.5,2.0))));
float3 waveNormal=normalize(lerp(VN,float3(0,0,1),flatten));
float resolved=1-smoothstep(35000.0,120000.0,length(P-Cam));
float3 wave=waveNormal*h*min(max(Amp,0),650.0)*nearHull*resolved;
"""
wpo_inputs=dict(base_inputs,VN=vertex_normal,Amp=wave_height,Flatten=hull_flatten,Level=sea_level,Cam=camera)
if fft_displacement:
    wpo_inputs.update(Live=fft_displacement,Enabled=use_live_fft,Count=live_count)
    wpo_code+=r"""
float3 d=Live*.65;
// Smooth bounds prevent a malformed render target from folding the bowl or
// pushing the water through the deck. The hull safety ellipse remains exact.
d.xy=d.xy/(1+abs(d.xy)/150.0);
d.z=d.z/(1+abs(d.z)/400.0);
float3 axis=float3(-waveNormal.y,waveNormal.x,0);
float3 curved=d+cross(axis,d)+cross(axis,cross(axis,d))/max(1+waveNormal.z,.01);
float liveWeight=saturate(Enabled)*step(.5,Count);
wave=lerp(wave,wave*.25+curved*nearHull*resolved,liveWeight);
"""
wpo_code+="wave*=1-saturate(VC.r*1.5); return wave+float3(0,0,(Level-P.z)*flatten);"
output(custom(water,wpo_code,FLOAT3,wpo_inputs,800),unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

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
# Diagnostic captures: 1 foam mask, 2 normals. Disabled in gameplay.
review=scalar(water,"WaterReview",0.0)
output(custom(water,"return Mode<.5 ? Opacity : 1.0;",FLOAT,{"Mode":review,"Opacity":opacity}),unreal.MaterialProperty.MP_OPACITY)
output(custom(water,"return Mode<.5 ? Spec : 0.0;",FLOAT,{"Mode":review,"Spec":specular}),unreal.MaterialProperty.MP_SPECULAR)
review_inputs={"Mode":review,"F":foam,"N":final_normal,"Source":source_foam}
review_inputs["Live"]=fft_foam if fft_foam else scalar(water,"NoLiveFoam",0.0)
review_emission=custom(water,"if(Mode<.5) return float3(0,0,0); if(Mode<1.5) return F.xxx; if(Mode<2.5) return N*.5+.5; if(Mode<3.5) return Source.xxx; return Live.xxx;",FLOAT3,review_inputs)
output(review_emission,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
review_color=custom(water,"return Mode<.5 ? C : float3(0,0,0);",FLOAT3,{"Mode":review,"C":color})
output(review_color,unreal.MaterialProperty.MP_BASE_COLOR)
MEL.recompile_material(water)
unreal.EditorAssetLibrary.save_loaded_asset(water,only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_MaelstromWater saved; DRW detail=%s foam colour=%s foam normal=%s" % (bool(normal_texture),bool(foam_texture),bool(foam_normal_texture)))
unreal.log_warning("DC_RESULT optional live FFT graph=%s; bounded vertex displacement=%s; UseLiveFFT defaults 0" % (bool(fft_surface),bool(fft_displacement)))

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
float initialSpeed=max(Speed,1.0);
float fallingSpeed=sqrt(initialSpeed*initialSpeed+19.62*max(p.y,0.0));
// Advect in travel-time coordinates: a strand accelerates and lengthens down
// a 90-350m fall rather than scrolling at one rigid speed over a painted wall.
float fallTravel=(fallingSpeed-initialSpeed)/9.81;
float2 q=float2(p.x,initialSpeed*(fallTravel-T));
// Two scales of lateral drift merge/split the streams as they descend; neither
// the colour nor its normal has a repeating one-dimensional stripe generator.
float shift=(wn.fbm(q*float2(0.009,0.012))-0.5)*42.0;
float2 bent=q+float2(shift,0.0);
float split=(wn.noise(bent*float2(0.034,0.023)+8.0)-0.5)*16.0;
float2 streaming=bent+float2(split,0.0);
float baseBlend=smoothstep(0.64,1.0,saturate(VC.b));
"""
cascade_foam_inputs=dict(cascade_inputs,Amount=cascade_foam_strength)
cascade_cell_uv=None
if foam_texture:
    cascade_raft_uv=custom(cascade,NOISE+CASCADE_FLOW+"return streaming/float2(42.0,42.0);",FLOAT2,cascade_inputs,10)
    cascade_cell_uv=custom(cascade,NOISE+CASCADE_FLOW+"return streaming/float2(9.0,10.0);",FLOAT2,cascade_inputs,40)
    cascade_foam_inputs["Raft"]=texture_sample(cascade,"FoamPattern",foam_texture,cascade_raft_uv,y=10)
    cascade_foam_inputs["Cells"]=texture_sample(cascade,"FoamPattern",foam_texture,cascade_cell_uv,y=40)
cascade_foam_code=NOISE+CASCADE_FLOW+r"""
float broad=wn.fbm(streaming*float2(0.012,0.028));
"""
if foam_texture:
    cascade_foam_code+=r"""
float raft=saturate((Raft.r-0.014)*2.6);
float cells=saturate((Cells.r-0.014)*2.6);
float aerated=smoothstep(0.18,0.80,raft*0.80+cells*0.20);
float supply=lerp(0.18,1.0,smoothstep(0.28,0.76,wn.noise(float2(p.x*0.015,p.y*0.006))));
float braids=smoothstep(0.28,0.75,wn.fbm(streaming*float2(0.038,0.016)));
float lip=exp(-max(p.y,0.0)*0.055)*raft*braids*0.16;
return saturate((aerated*(0.24+cells*0.72)+lip)*Amount*supply
    *lerp(0.25,1.0,braids)*lerp(0.55,1.0,broad)*(1.0-baseBlend*0.35));
"""
else:
    cascade_foam_code+=r"""
float braids=wn.fbm(streaming*float2(0.045,0.020));
float threads=wn.fbm(streaming*float2(0.16,0.065));
return saturate(smoothstep(0.23,0.70,braids)*(0.35+threads*0.65)*Amount*(1.0-baseBlend*0.4));
"""
cascade_foam=custom(cascade,cascade_foam_code,FLOAT,cascade_foam_inputs,100)
cascade_color=custom(cascade,NOISE+CASCADE_FLOW+r"""
float channels=wn.fbm(streaming*float2(0.022,0.015));
float3 body=lerp(DeepColor,WaterColor,smoothstep(0.25,0.8,channels));
body=lerp(body,float3(0.014,0.052,0.064),baseBlend);
return lerp(body,FoamColor,Foam);
""",FLOAT3,dict(cascade_inputs,Foam=cascade_foam,
    DeepColor=vector(cascade,"DeepColor",(0.012,0.035,0.050,1)),
    WaterColor=vector(cascade,"WaterColor",(0.045,0.10,0.135,1)),
    FoamColor=vector(cascade,"FoamColor",(0.62,0.72,0.76,1))),200)
output(cascade_color,unreal.MaterialProperty.MP_BASE_COLOR)
output(custom(cascade,"return lerp(0.42,0.76,sqrt(saturate(Foam)));",FLOAT,{"Foam":cascade_foam},300),unreal.MaterialProperty.MP_ROUGHNESS)
output(scalar(cascade,"Specular",0.20),unreal.MaterialProperty.MP_SPECULAR)
cascade_alpha=custom(cascade,NOISE+CASCADE_FLOW+r"""
// The footprint stays put while thin trailing strands stream through it. The
// temporal coverage mask resolves soft partial coverage, rather than chopping
// this gradual blend at a fixed opacity threshold into another hard arc.
float footprint=wn.fbm(float2(p.x*0.030,p.y*0.008));
float strands=wn.fbm(streaming*float2(0.12,0.025));
float endStart=lerp(0.50,0.75,footprint);
float foot=1.0-smoothstep(endStart,0.98,saturate(VC.b));
float edge=smoothstep(0.16,0.50,VC.r);
float fringe=smoothstep(0.48,0.96,VC.b);
return saturate(foot*edge*lerp(1.0,0.25+0.75*strands,fringe));
""",FLOAT,cascade_inputs,350)
dither_function=unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA")
if not dither_function:
    raise RuntimeError("Required stock DitherTemporalAA function is missing")
cascade_dither=expression(cascade,unreal.MaterialExpressionMaterialFunctionCall,-100,350)
if not cascade_dither.set_material_function(dither_function):
    raise RuntimeError("Could not assign stock DitherTemporalAA function")
if not MEL.connect_material_expressions(cascade_alpha,"",cascade_dither,"Alpha Threshold"):
    raise RuntimeError("Could not bind temporal cascade coverage")
output(cascade_dither,unreal.MaterialProperty.MP_OPACITY_MASK)
cascade_normal_inputs=dict(cascade_inputs,Depth=cascade_depth,Strength=cascade_strength,Foam=cascade_foam)
if foam_texture and foam_normal_texture:
    cascade_normal_inputs["FoamNormal"]=texture_sample(cascade,"FoamNormal",foam_normal_texture,cascade_cell_uv,normal=True,y=450)
cascade_normal_code=NOISE+CASCADE_FLOW+r"""
float e=0.3;
float2 dx=float2(e,0), dy=float2(0,e);
float2 slope=-float2(wn.cascade(q+dx)-wn.cascade(q-dx),wn.cascade(q+dy)-wn.cascade(q-dy));
slope*=clamp(Depth,0.0,120.0)*0.01/(2.0*e);
slope.y*=initialSpeed/fallingSpeed;
"""
if foam_texture and foam_normal_texture:
    cascade_normal_code+="slope+=FoamNormal.xy*float2(1.0,(9.0/10.0)*initialSpeed/fallingSpeed)*Foam*0.22;\n"
cascade_normal_code+=r"""
return normalize(float3(slope*Strength*(1.0-Foam*0.2)*(1.0-baseBlend*0.7),1.0));
"""
cascade_normal=custom(cascade,cascade_normal_code,FLOAT3,cascade_normal_inputs,400)
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
// UV.y is zero at the top of the curtain, so positive sample motion rises.
// Broad billows carry smaller breakup instead of a uniformly noisy flat sheet.
float2 q=float2(UV.x*8.0-T*0.035,UV.y*3.0+T*0.09);
float2 warp=float2(wn.noise(q*0.7),wn.noise(q*0.8+8.0))-0.5;
float n=wn.fbm(q+warp*1.4)+wn.noise(q*3.1)*0.17;
float edge=smoothstep(0.0,0.18,UV.x)*smoothstep(0.0,0.18,1.0-UV.x);
float vertical=smoothstep(0.0,0.14,UV.y)*pow(saturate(1.0-UV.y),1.3);
return saturate(smoothstep(0.30,0.75,n)*edge*vertical*Density*(0.25+VC.r*0.75)*Alpha*Reveal);
"""
output(custom(spray,spray_code,FLOAT,{"UV":spray_uv,"VC":spray_vc,"Alpha":(spray_vc,"A"),"T":spray_t,"Reveal":reveal,"Density":density}),unreal.MaterialProperty.MP_OPACITY)
output(vector(spray,"SprayColor",(0.65,0.75,0.80,1)),unreal.MaterialProperty.MP_BASE_COLOR)
output(scalar(spray,"SprayRoughness",0.8),unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(spray)
unreal.EditorAssetLibrary.save_loaded_asset(spray,only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_MaelstromSpray saved")
