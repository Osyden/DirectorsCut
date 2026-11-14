"""Build stage-owned spray detail and the daylight-opening light-function mask.

Run with the project's UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Only writes /Game/DC/Maelstrom/M_MaelstromPlume and M_MaelstromShaftMask.

DCMaelstromAtmosphere supplies real LocalFogVolume components for the soft 3D
billows. Plume material adds torn moving spray, not their opaque silhouette.
Plume UV0: across/height 0..1; vertex R density, G random seed, B sheet index,
A sheet opacity. Clock is encounter time; Reveal is 0..1; Density defaults .65.
DepthFade softens contact with the water. There is no visible light-beam mesh:
ShaftMask is a LightFunction on three shadowed volumetric spots with nearly zero
surface contribution, positioned and oriented by the runtime atmosphere actor.

Preview requirements: inspect the actual helm at breach+10 and +35, then a side
gun. Verify that billows sit at active falls feet, the bowl remains readable,
shafts have soft broken edges, and both disappear on an ending/backward seek.
Their appearance/performance requires runtime review; generation is not a render
quality claim. Actual shafts require the existing stage height fog's volumetrics.
"""
import unreal

ROOT = "/Game/DC/Maelstrom"
MEL = unreal.MaterialEditingLibrary
FLOAT = unreal.CustomMaterialOutputType.CMOT_FLOAT1
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


def material(name):
    result = unreal.load_asset(ROOT + "/" + name)
    if result:
        for previous in list(MEL.get_material_expressions(result)):
            MEL.delete_material_expression(result, previous)
        if MEL.get_num_material_expressions(result):
            raise RuntimeError("Could not clear old graph: " + name)
    else:
        result = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not result:
        raise RuntimeError("Could not create material: " + name)
    return result


NOISE = r"""
struct PlumeNoise {
    float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
    float noise(float2 p) {
        float2 i=floor(p), f=frac(p); f=f*f*(3.0-2.0*f);
        return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),
                    lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);
    }
    float fbm(float2 p) {
        return noise(p)*0.57+noise(p*2.03+7.2)*0.28+noise(p*4.11-3.4)*0.15;
    }
};
PlumeNoise pn;
"""

plume = material("M_MaelstromPlume")
plume.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
plume.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
plume.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
plume.set_editor_property("two_sided", True)
# Spray is kilometres from the captain: the short-range translucency lighting
# volume cannot provide reliable light there. Forward-lit detail receives the
# actual sun at its world position; the soft body remains true local-volume fog.
plume.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
uv = expression(plume, unreal.MaterialExpressionTextureCoordinate, -1300)
vc = expression(plume, unreal.MaterialExpressionVertexColor, -1300, 160)
position = expression(plume, unreal.MaterialExpressionWorldPosition, -1300, 300)
camera = expression(plume, unreal.MaterialExpressionCameraPositionWS, -1300, 400)
clock = scalar(plume, "Clock", 0.0)
reveal = scalar(plume, "Reveal", 1.0)
density = scalar(plume, "Density", 0.65)
crest_spray = scalar(plume, "CrestSpray", 0.0)

opacity_code = NOISE + r"""
float seed=VC.g*41.0+VC.b*17.0;
float2 p=UV*float2(5.5,7.0)+seed;
// Slow billows carry finer, faster spray upward. Noise is not a repeating stripe.
float2 warp=float2(pn.noise(p*0.6+T*0.025),pn.noise(p*0.55-7.0))-0.5;
float large=pn.fbm(p+warp*1.4-float2(T*0.045,T*0.13));
float fine=pn.fbm(p*2.7-float2(T*0.12,T*0.41));
float lobe=smoothstep(0.24,0.65,large+fine*0.18);
// A wandering centre and width break the regular elliptical card silhouette.
float edgeWander=(pn.noise(float2(UV.y*3.1+seed,T*0.018))-0.5)*0.30;
float edgeWidth=lerp(0.70,1.15,pn.fbm(float2(UV.y*4.7+seed,seed+3.9)));
float2 edge=float2(((UV.x-0.5)*2.0+edgeWander)/edgeWidth,(UV.y-0.48)*1.95);
float silhouette=pow(saturate(1.0-dot(edge,edge)),1.15);
float torn=smoothstep(0.18,0.55,large+silhouette*0.25);
float proximity=lerp(smoothstep(3000.0,9000.0,length(P-Camera)),smoothstep(500.0,2500.0,length(P-Camera)),Crest);
// Short downstream tendrils at a breaker; keep distant fall plumes unchanged.
float tendrils=pn.fbm(UV*float2(35.0,9.0)+float2(seed,-T*1.4));
float spray=smoothstep(.34,.67,tendrils)*(1-smoothstep(.35,1.0,UV.y));
return saturate(lerp(lobe*torn,spray,Crest)*silhouette*VC.r*Alpha*Density*Reveal*proximity);
"""
opacity = custom(plume, opacity_code, FLOAT, {
    "UV": uv, "VC": vc, "Alpha": (vc, "A"), "P": position, "Camera": camera,
    "T": clock, "Reveal": reveal, "Density": density, "Crest":crest_spray,
})
fade = expression(plume, unreal.MaterialExpressionDepthFade, 0, 120)
fade.set_editor_property("fade_distance_default", 1200.0)
if not MEL.connect_material_expressions(opacity, "", fade, "Opacity"):
    raise RuntimeError("Could not bind plume depth fade")
MEL.connect_material_expressions(scalar(plume,"DepthFadeDistance",1200.0),"",fade,"FadeDistance")
output(fade, unreal.MaterialProperty.MP_OPACITY)
output(vector(plume, "SprayAlbedo", (0.68, 0.77, 0.82, 1.0)), unreal.MaterialProperty.MP_BASE_COLOR)
output(scalar(plume, "Roughness", 1.0), unreal.MaterialProperty.MP_ROUGHNESS)
output(scalar(plume, "Specular", 0.0), unreal.MaterialProperty.MP_SPECULAR)
MEL.recompile_material(plume)
unreal.EditorAssetLibrary.save_loaded_asset(plume, only_if_is_dirty=False)

mask = material("M_MaelstromShaftMask")
mask.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)
mask.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
mask.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
light_uv = expression(mask, unreal.MaterialExpressionTextureCoordinate, -1300)
mask_code = NOISE + r"""
float2 p=UV*2.6+Seed+float2(T*0.0018,-T*0.0013);
float2 warp=float2(pn.noise(p*0.8),pn.noise(p*0.7+9.0))-0.5;
float n=pn.fbm(p+warp*0.7);
// Fractional opening with feathered cloud fingers. Never pulse the whole beam.
return lerp(0.12,1.0,smoothstep(0.30,0.70,n));
"""
mask_value = custom(mask, mask_code, FLOAT3, {
    "UV": light_uv, "T": scalar(mask, "Clock", 0.0), "Seed": scalar(mask, "Seed", 0.0),
})
output(mask_value, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mask)
unreal.EditorAssetLibrary.save_loaded_asset(mask, only_if_is_dirty=False)
unreal.log_warning("DC_RESULT atmosphere materials saved: M_MaelstromPlume + M_MaelstromShaftMask; runtime visual review still required")
