"""Create isolated M_CliffRock for the generated Nanite cliff preview.

Run in Unreal's Python commandlet; root owns shader compilation/GPU review.
Only /Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffRock is changed by default.
Set DC_CLIFF_PREVIEW_ASSIGN=1 to assign it to that folder's SM_CliffSector only.
No map, runtime cliff, vendor texture, sky, lighting or other material is edited.

Contract: imported local coordinates are UE centimetres at actor scale1.
Vertex linear R=normalized log cumulative flow, G=gross deposition, B=erosion.
These historical transport masks guide an art-directed wetness/deposit response;
they do not establish current water saturation or physically correct geology.

This version is original procedural stone: no cliff atlas or copied pixels.
Large fracture volume belongs to the mesh. Detail is shallow centimetre/metre
surface relief with derivative filtering; no displaced vertices or fake blocks.
"""
import json
import os
from pathlib import Path

import unreal


ROOT = "/Game/DC/Maelstrom/GeneratedCliffPreview"
MATERIAL_PATH = ROOT + "/M_CliffRock"
M = unreal.MaterialEditingLibrary

# Value plus exact analytical gradient of smooth 3D value noise. Hash lattice
# corners have no colour/height faces; interpolation is continuous across cells.
STONE_NOISE = r"""
struct StoneField {
 float hash(float3 p) {
  uint3 q=asuint(int3(p));
  uint h=q.x*1597334677u+q.y*3812015801u+q.z*2798796415u+1013904223u;
  h=(h^(h>>16))*2246822519u; h=(h^(h>>13))*3266489917u; h=h^(h>>16);
  return float(h&0x00ffffffu)/16777215.0;
 }
 float4 evaluate(float3 p) {
  float3 i=floor(p),f=frac(p),u=f*f*(3-2*f),du=6*f*(1-f);
  float a=hash(i),b=hash(i+float3(1,0,0)),c=hash(i+float3(0,1,0)),d=hash(i+float3(1,1,0));
  float e=hash(i+float3(0,0,1)),f1=hash(i+float3(1,0,1)),g=hash(i+float3(0,1,1)),h=hash(i+1);
  float value=lerp(lerp(lerp(a,b,u.x),lerp(c,d,u.x),u.y),lerp(lerp(e,f1,u.x),lerp(g,h,u.x),u.y),u.z);
  float gx=du.x*lerp(lerp(b-a,d-c,u.y),lerp(f1-e,h-g,u.y),u.z);
  float gy=du.y*lerp(lerp(c-a,d-b,u.x),lerp(g-e,h-f1,u.x),u.z);
  float gz=du.z*lerp(lerp(e-a,f1-b,u.x),lerp(g-c,h-d,u.x),u.y);
  return float4(value,gx,gy,gz);
 }
}; StoneField rock;
"""

WEATHER = STONE_NOISE + r"""
float3 p=P*.01;
float mineral=.65*rock.evaluate(p/29+float3(8.1,13.2,1.7)).x
             +.35*rock.evaluate(p/11+float3(-3.7,4.3,19.1)).x;
// A normalized logarithm is not water saturation. Threshold only concentrated
// paths, with a bounded strength parameter; avoid wetting the whole plateau.
float wet=smoothstep(.64,.94,saturate(Masks.r))*saturate(FlowWetness);
float upward=smoothstep(.05,.72,normalize(N).z);
float deposited=smoothstep(.04,.65,saturate(Masks.g))*(.25+.75*upward);
float eroded=smoothstep(.06,.72,saturate(Masks.b));
deposited*=1-.35*eroded;
return float4(mineral,wet,deposited,eroded);
"""

MINERAL_RELIEF = STONE_NOISE + r"""
float3 p=P*.01;
float footprint=max(length(ddx(p)),length(ddy(p)));
float3 gradient=0; float grain=.5;
// Three shallow, stationary 3D height bands. Geometric normal is perturbed by
// the tangential derivative later, so this does not create polygon-sized tilts.
// 75cm band:12mm height;18cm band:2mm;4.5cm band:0.55mm.
[branch] if(footprint<.38) {
 float fade=1-smoothstep(.12,.38,footprint);
 float4 a=rock.evaluate(p/.75+float3(11.3,7.1,21.7));
 gradient+=a.yzw*(.012/.75)*fade;
 grain+=(a.x-.5)*.65*fade;
}
[branch] if(footprint<.09) {
 float fade=1-smoothstep(.025,.09,footprint);
 float4 b=rock.evaluate(p/.18+float3(5.1,-13.7,6.3));
 gradient+=b.yzw*(.002/.18)*fade;
 grain+=(b.x-.5)*.25*fade;
}
[branch] if(footprint<.0225) {
 float fade=1-smoothstep(.006,.0225,footprint);
 float4 c=rock.evaluate(p/.045+float3(-19.3,1.7,8.1));
 gradient+=c.yzw*(.00055/.045)*fade;
 grain+=(c.x-.5)*.10*fade;
}
return float4(gradient,saturate(grain));
"""

FRACTURES = r"""
float3 p=P*.01;
float footprint=max(length(ddx(p)),length(ddy(p)));
// Largest fracture half-width4.3cm: detail is fully unresolved above6.45cm/pixel.
[branch] if(footprint>=.0645) { return float4(0,0,0,0); }
struct FineCracks {
 float3 hash(float2 p) {
  uint2 q=asuint(int2(p));
  uint3 h=q.x*1597334677u+q.y*3812015801u+uint3(1013904223u,2246822519u,3266489917u);
  h=(h^(h>>16))*2246822519u;h=(h^(h>>13))*3266489917u;h=h^(h>>16);
  return float3(h&0x00ffffffu)/16777215.0;
 }
 float3 evaluate(float2 p,float footprint) {
  float2 cell=floor(p/2.2),gradient=0;float cavity=0;
  // Cells only distribute short independent segments. There are no cell
  // boundaries, closed brick tiles, colour seams or broad fake AO rectangles.
  [unroll] for(int y=-1;y<=1;y++) [unroll] for(int x=-1;x<=1;x++) {
   float2 id=cell+float2(x,y);float3 h=hash(id);
   if(h.z>.36) { continue; }
   float3 shape=hash(id+float2(37,91));
   float angle=(shape.x-.5)*2.4;
   float2 along=float2(sin(angle),cos(angle)),across=float2(along.y,-along.x);
   float2 d=p-(id+.1+.8*h.xy)*2.2;
   float halfLength=.45+.7*shape.y,halfWidth=.018+.025*shape.z;
   float v=dot(d,along),end=abs(v)/halfLength;
   if(end>=1) { continue; }
   float phase=v*3+h.x*6.2831853;
   float u=(dot(d,across)+.025*sin(phase))/halfWidth;
   if(abs(u)>=1) { continue; }
   float f=saturate((end-.55)/.45),envelope=1-f*f*(3-2*f);
   float envelopeDerivative=-6*f*(1-f)*sign(v)/(.45*halfLength);
   float crossShape=(1-u*u)*(1-u*u),crossDerivative=-4*u*(1-u*u)/halfWidth;
   float resolved=1-smoothstep(halfWidth*.35,halfWidth*1.5,footprint);
   float depth=.003+.006*h.y;
   gradient-=depth*(crossDerivative*envelope*(across+.075*cos(phase)*along)
                    +crossShape*envelopeDerivative*along)*resolved;
   cavity=max(cavity,crossShape*envelope*resolved);
  }
  return float3(gradient,cavity);
 }
}; FineCracks cracks;
float3 n=normalize(N),w=pow(abs(n),4);w/=max(dot(w,float3(1,1,1)),.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float3 x=cracks.evaluate(float2(p.y*s.x,p.z),footprint);
float3 y=cracks.evaluate(float2(p.x*s.y,p.z)+float2(1.1,3.7),footprint);
float3 z=cracks.evaluate(float2(p.x*s.z,p.y)+float2(7.3,2.1),footprint);
float3 gradient=float3(0,x.x*s.x,x.y)*w.x+float3(y.x*s.y,0,y.y)*w.y+float3(z.x*s.z,z.y,0)*w.z;
return float4(gradient,dot(float3(x.z,y.z,z.z),w));
"""


def create_material():
    assign = os.environ.get("DC_CLIFF_PREVIEW_ASSIGN", "0")
    if assign not in ("0", "1"):
        raise RuntimeError("DC_CLIFF_PREVIEW_ASSIGN must be 0 or 1")
    material = unreal.load_asset(MATERIAL_PATH)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_CliffRock", ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material) or material.get_path_name().split(".")[0] != MATERIAL_PATH:
        raise RuntimeError("Unexpected preview material target")
    M.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property("tangent_space_normal", False)
    material.set_editor_property("two_sided", False)
    def node(cls):
        return M.create_material_expression(material, cls)
    def connect(source, pin, target, input_pin):
        if not M.connect_material_expressions(source, pin, target, input_pin):
            raise RuntimeError("Failed material connection: " + input_pin)
    def custom(code, kind, inputs):
        expression = node(unreal.MaterialExpressionCustom)
        expression.set_editor_property("code", code)
        expression.set_editor_property("output_type", kind)
        pins = []
        for name in inputs:
            pin = unreal.CustomInput()
            pin.set_editor_property("input_name", name)
            pins.append(pin)
        expression.set_editor_property("inputs", pins)
        for name, source in inputs.items():
            connect(source, "", expression, name)
        return expression
    def constant(value):
        expression = node(unreal.MaterialExpressionConstant)
        expression.set_editor_property("r", value)
        return expression
    def parameter(name, value):
        expression = node(unreal.MaterialExpressionScalarParameter)
        expression.set_editor_property("parameter_name", name)
        expression.set_editor_property("default_value", value)
        return expression
    def output(expression, prop):
        if not M.connect_material_property(expression, "", prop):
            raise RuntimeError("Preview output failed: " + str(prop))
        if M.get_material_property_input_node(material, prop) != expression:
            raise RuntimeError("Preview output is disconnected: " + str(prop))

    world = node(unreal.MaterialExpressionWorldPosition)
    position = node(unreal.MaterialExpressionTransformPosition)
    position.set_editor_property("transform_source_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
    position.set_editor_property("transform_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
    connect(world, "", position, "")
    vertex_normal = node(unreal.MaterialExpressionVertexNormalWS)
    normal = node(unreal.MaterialExpressionTransform)
    normal.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
    normal.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
    connect(vertex_normal, "", normal, "")
    masks = node(unreal.MaterialExpressionVertexColor)
    types = unreal.CustomMaterialOutputType
    weather = custom(WEATHER, types.CMOT_FLOAT4, {"P":position,"N":normal,"Masks":masks,"FlowWetness":parameter("FlowWetness", .65)})
    detail = custom(MINERAL_RELIEF, types.CMOT_FLOAT4, {"P":position})
    fractures = custom(FRACTURES, types.CMOT_FLOAT4, {"P":position,"N":normal})
    color = custom(r"""
float3 stone=float3(.22,.235,.245)*lerp(.94,1.06,Weather.x);
stone=lerp(stone,float3(.265,.267,.270),Weather.w*.28);
stone=lerp(stone,float3(.265,.260,.250),Weather.z*.38);
stone*=lerp(.97,1.03,Detail.w)*lerp(1,.94,Fractures.w);
return stone*lerp(1,.62,Weather.y);
""", types.CMOT_FLOAT3, {"Weather":weather,"Detail":detail,"Fractures":fractures})
    output(color, unreal.MaterialProperty.MP_BASE_COLOR)
    local_normal = custom(r"""
float3 n=normalize(N);
float3 gradient=(Detail.xyz+Fractures.xyz)*max(DetailStrength,0);
gradient-=n*dot(gradient,n);
return normalize(n-gradient);
""", types.CMOT_FLOAT3, {"N":normal,"Detail":detail,"Fractures":fractures,"DetailStrength":parameter("DetailStrength", 1.0)})
    world_normal = node(unreal.MaterialExpressionTransform)
    world_normal.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    world_normal.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    connect(local_normal, "", world_normal, "")
    output(world_normal, unreal.MaterialProperty.MP_NORMAL)
    roughness = custom("return clamp(.82+.08*Weather.x+.06*Weather.z-.05*Weather.w-.24*Weather.y,.53,.94);",
                       types.CMOT_FLOAT1, {"Weather":weather})
    output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    output(constant(.35), unreal.MaterialProperty.MP_SPECULAR)
    output(constant(0), unreal.MaterialProperty.MP_METALLIC)
    output(custom("return lerp(1,.95,Fractures.w);", types.CMOT_FLOAT1, {"Fractures":fractures}), unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    M.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("Could not save isolated cliff material")
    if assign == "1":
        mesh = unreal.load_asset(ROOT + "/SM_CliffSector")
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Missing isolated SM_CliffSector")
        mesh.set_material(0, material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError("Could not save preview mesh material assignment")
    report = dict(material=MATERIAL_PATH, assigned_preview_mesh=assign=="1", vendor_textures_used=False,
                  units="Local UE centimetres converted to metres; unit actor scale required",
                  vertex_contract="linear R normalized log-flow; G gross-deposition; B gross-erosion",
                  relief_bands_m=[.75,.18,.045], relief_heights_m=[.012,.002,.00055],
                  fracture_length_m=[.9,2.3], fracture_width_m=[.036,.086], fracture_depth_m=[.003,.009],
                  normal_fully_filtered_above_metres_per_pixel=.38, cracks_filtered_above_metres_per_pixel=.0645,
                  roughness_bounds=[.53,.94], emissive=False, displacement=False,
                  runtime_integrated=False, actual_shader_compile_and_visual_review_pending=True)
    destination = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / "CliffRockMaterialPreview.json"
    destination.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    unreal.log_warning("DC_CLIFF_ROCK_PREVIEW " + json.dumps(report))


if __name__ == "__main__":
    create_material()
