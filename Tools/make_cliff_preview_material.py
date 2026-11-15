"""Create isolated M_CliffRock for the generated Nanite cliff preview.

Run in Unreal's Python commandlet; root owns shader compilation/GPU review.
Only /Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffRock is changed by default.
Set DC_CLIFF_PREVIEW_ASSIGN=1 to assign it to that folder's SM_CliffSector only.
No map, runtime cliff, vendor texture, sky, lighting or other material is edited.

Contract: imported local coordinates are UE centimetres at actor scale1.
Vertex linear R=normalized log cumulative flow, G=gross deposition, B=erosion.
These historical transport masks guide an art-directed wetness/deposit response;
they do not establish current water saturation or physically correct geology.

Default is original procedural stone: no cliff atlas or copied pixels.
DC_CLIFF_STONE=slate uses installed Paragon tileable albedo/normal by reference,
at an authored4m tile size. Vendor pixels never enter the repository. Roughness
remains authored: no unverified packed-map channels are used.
DC_CLIFF_TRANSPORT_MASKS=none is required when the source mesh has no verified
erosion colours (e.g. the ring pilot). It avoids treating default white vertex
colours as full deposition/flow. Slate then uses its original colour, unit
normal strength and an explicitly authored0.80 dry roughness.
DC_CLIFF_STONE=scan uses the verified CC0 material imported by
import_cliff_scan_textures.py at its source-reported physical tile size.
Exposed-face U/Z coordinates match the mesh displacement; no tile enlargement.
DC_CLIFF_SCAN_SAMPLING=offsets enables shared three-offset colour/normal/roughness
sampling ONLY for geometry without raw scan displacement. Default raw preserves
the A/B displacement registration.
DC_CLIFF_SCAN_PROJECTION=triplanar is an optional physical-scale ring preview;
default sourcechart preserves U/Z registration. The triplanar candidate uses
source normal strength1.0; sourcechart retains0.5. No scan geometry is enlarged.
Large fracture volume belongs to the mesh. Detail is shallow centimetre/metre
surface relief with derivative filtering; no displaced vertices or fake blocks.
DC_CLIFF_SCAN_CANDIDATE=1 creates ONLY M_CliffScanSurface: verified scan at
native scale, signed triplanar projection, co-registered rotated offset samples,
source colour and roughness, and physical-footprint normal filtering. It leaves
the default material/clay unchanged. Intended for geometry without registered
raw scan displacement; no unverified drainage/vertex masks are interpreted.
DC_CLIFF_ALBEDO_SCALE adjusts only candidate brightness (default1); it preserves
source chromaticity. DC_CLIFF_FITTED_WETNESS=1 adds an authored spray-proximity
response from the active source-matched waterfall trajectories.
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

# Three offset samples share a triangular lattice. Albedo and normal use the
# same offsets/weights; no UV rotation that would desynchronise normal axes.
# Gradients come from the continuous physical UV, not hashed sample offsets.
STONE_TILE = r"""
struct StoneTile {
 float2 offset(float2 p) {
  return frac(sin(float2(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3))))*43758.5453);
 }
 float4 sampleMap(Texture2D tex,SamplerState samp,float2 uv) {
  float2 g=float2(uv.x+uv.y*.57735027,uv.y*1.15470054)*.65;
  float2 cell=floor(g),f=frac(g),a=cell,b,c;float3 w;
  if(f.x+f.y<1) { b=cell+float2(1,0);c=cell+float2(0,1);w=float3(1-f.x-f.y,f.x,f.y); }
  else { a=cell+1;b=cell+float2(0,1);c=cell+float2(1,0);w=float3(f.x+f.y-1,1-f.x,1-f.y); }
  w=pow(max(w,0),3);w/=max(w.x+w.y+w.z,.00001);
  float2 dx=ddx(uv),dy=ddy(uv);
  return tex.SampleGrad(samp,uv+offset(a),dx,dy)*w.x
        +tex.SampleGrad(samp,uv+offset(b),dx,dy)*w.y
        +tex.SampleGrad(samp,uv+offset(c),dx,dy)*w.z;
 }
};StoneTile tile;
"""

# Candidate-only stochastic rotation retains the scan's authored metric size.
# D/N/R share exact sample coordinates; decoded tangent normals are rotated back
# before blending, so colour seams and lighting do not disagree on joint axes.
SCAN_TILE_FILTERED = r"""
struct StoneTile {
 float2 hash(float2 p) {
  return frac(sin(float2(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3))))*43758.5453);
 }
 void lattice(float2 uv,out float2 a,out float2 b,out float2 c,out float3 w) {
  float2 g=float2(uv.x+uv.y*.57735027,uv.y*1.15470054)*.65;
  float2 cell=floor(g),f=frac(g);a=cell;
  if(f.x+f.y<1) { b=cell+float2(1,0);c=cell+float2(0,1);w=float3(1-f.x-f.y,f.x,f.y); }
  else { a=cell+1;b=cell+float2(0,1);c=cell+float2(1,0);w=float3(f.x+f.y-1,1-f.x,1-f.y); }
  w=pow(max(w,0),3);w/=max(w.x+w.y+w.z,.00001);
 }
 float2 rotate(float2 p,float2 cs) { return float2(cs.x*p.x-cs.y*p.y,cs.y*p.x+cs.x*p.y); }
 float2 rotation(float2 id) { float angle=(hash(id+float2(31.7,-19.3)).x-.5)*.9;return float2(cos(angle),sin(angle)); }
 float4 sampleMap(Texture2D tex,SamplerState samp,float2 uv) {
  float2 a,b,c;float3 w;lattice(uv,a,b,c,w);
  float2 ra=rotation(a),rb=rotation(b),rc=rotation(c),dx=ddx(uv),dy=ddy(uv);
  return tex.SampleGrad(samp,rotate(uv,ra)+hash(a),rotate(dx,ra),rotate(dy,ra))*w.x
       +tex.SampleGrad(samp,rotate(uv,rb)+hash(b),rotate(dx,rb),rotate(dy,rb))*w.y
       +tex.SampleGrad(samp,rotate(uv,rc)+hash(c),rotate(dx,rc),rotate(dy,rc))*w.z;
 }
 float3 sampleNormal(Texture2D tex,SamplerState samp,float2 uv) {
  float2 a,b,c;float3 w;lattice(uv,a,b,c,w);
  float2 ra=rotation(a),rb=rotation(b),rc=rotation(c),dx=ddx(uv),dy=ddy(uv);
  float3 na=UnpackNormalMap(tex.SampleGrad(samp,rotate(uv,ra)+hash(a),rotate(dx,ra),rotate(dy,ra))).xyz;
  float3 nb=UnpackNormalMap(tex.SampleGrad(samp,rotate(uv,rb)+hash(b),rotate(dx,rb),rotate(dy,rb))).xyz;
  float3 nc=UnpackNormalMap(tex.SampleGrad(samp,rotate(uv,rc)+hash(c),rotate(dx,rc),rotate(dy,rc))).xyz;
  na.xy=rotate(na.xy,ra*float2(1,-1));nb.xy=rotate(nb.xy,rb*float2(1,-1));nc.xy=rotate(nc.xy,rc*float2(1,-1));
  return normalize(na*w.x+nb*w.y+nc*w.z);
 }
};StoneTile tile;
"""


def fitted_wet_rock(saved):
    """Authored spray proximity around the actual fitted paths, in mesh metres.

    This is a visual wet-rock response, not a saturation simulation. The paths
    come from the active fall contract; no vertical noise stripes are invented.
    """
    contract = json.loads((saved/'CliffPreviewWaterfall.json').read_text())
    audit = json.loads((saved/'CliffPreviewSourceAudit.json').read_text())
    if contract['cliff_fbx_sha256'] != audit['source_fbx_sha256']:
        raise RuntimeError('Wet-rock fall contract does not match active cliff')
    profile = json.loads(Path(contract['profile_path']).read_text())
    if profile['profile_sha1'] != contract['profile_sha1']:
        raise RuntimeError('Wet-rock profile mismatch')
    frame = audit['geometry']['arena_reconstruction']
    origin, axes = frame['origin_cm'], frame['basis_for_imported_xyz']
    segments = []
    def mesh_point(outlet, point):
        basis = [outlet['basis_x_arena'],outlet['basis_y_arena'],outlet['basis_z_arena']]
        arena = [outlet['local_origin_arena_cm'][j]+sum(point[k]*basis[k][j] for k in range(3)) for j in range(3)]
        return [sum((arena[j]-origin[j])*axis[j] for j in range(3))*.01 for axis in axes]
    for outlet in profile.get('ballistic_outlets',[profile]):
        samples = outlet['samples_local_cm']
        # Twenty-four chords per actual trajectory are sufficient for a broad
        # metre-scale spray proximity mask, not used for collision/contact.
        indices = sorted(set(round(i*(len(samples)-1)/24) for i in range(25)))
        points = [mesh_point(outlet,samples[i]) for i in indices]
        width = outlet['source_half_width_cm']*.01
        for a,b in zip(points,points[1:]):
            if sum((b[j]-a[j])**2 for j in range(3)) > 1.e-8:
                segments.append((a,b,width+2.5))
    if not segments or len(segments)>96:
        raise RuntimeError('Invalid fitted wet-rock segment count')
    def vector(values): return 'float3('+','.join(f'{v:.6f}' for v in values)+')'
    code = ['float3 p=P*.01; float wet=0;']
    for a,b,radius in segments:
        code.append('{ float3 a='+vector(a)+',b='+vector(b)+'; float3 d=b-a; '
                    'float t=saturate(dot(p-a,d)/max(dot(d,d),.000001)); '
                    f'float distance=length(p-a-t*d); wet=max(wet,1-smoothstep({radius*.55:.6f},{radius:.6f},distance)); }}')
    code.append('return saturate(wet*Strength);')
    return '\n'.join(code), dict(profile_sha1=profile['profile_sha1'],
        cliff_fbx_sha256=contract['cliff_fbx_sha256'], segments=len(segments),
        response='Authored proximity to actual falling-water trajectories; not measured rock saturation',
        dry_chromaticity_preserved=True, albedo_multiplier_at_full_wetness=.52,
        roughness_at_full_wetness=.24)


def create_material():
    candidate = os.environ.get('DC_CLIFF_SCAN_CANDIDATE', '0')
    if candidate not in ('0', '1'):
        raise RuntimeError('DC_CLIFF_SCAN_CANDIDATE must be0 or1')
    candidate = candidate == '1'
    albedo_scale = float(os.environ.get('DC_CLIFF_ALBEDO_SCALE','1')) if candidate else 1.
    if not 0.05 <= albedo_scale <= 2.:
        raise RuntimeError('Candidate albedo scale must be finite and within .05..2')
    material_name = 'M_CliffScanSurface' if candidate else 'M_CliffRock'
    material_path = ROOT + '/' + material_name
    assign = os.environ.get("DC_CLIFF_PREVIEW_ASSIGN", "0")
    if assign not in ("0", "1"):
        raise RuntimeError("DC_CLIFF_PREVIEW_ASSIGN must be 0 or 1")
    preset = os.environ.get('DC_CLIFF_STONE', 'procedural')
    if candidate:
        preset = 'scan'
    if preset not in ('procedural', 'slate', 'scan'):
        raise RuntimeError('DC_CLIFF_STONE must be procedural, slate or scan')
    texture_assets = {}
    scan = None
    transport_masks = os.environ.get('DC_CLIFF_TRANSPORT_MASKS', 'vertex')
    if candidate:
        transport_masks = 'none'
    if transport_masks not in ('vertex', 'none'):
        raise RuntimeError('DC_CLIFF_TRANSPORT_MASKS must be vertex or none')
    slate_strength = 1. if transport_masks == 'none' else .7
    scan_sampling = os.environ.get('DC_CLIFF_SCAN_SAMPLING', 'raw')
    if scan_sampling not in ('raw', 'offsets'):
        raise RuntimeError('DC_CLIFF_SCAN_SAMPLING must be raw or offsets')
    scan_projection = os.environ.get('DC_CLIFF_SCAN_PROJECTION', 'sourcechart')
    if scan_projection not in ('sourcechart', 'triplanar'):
        raise RuntimeError('DC_CLIFF_SCAN_PROJECTION must be sourcechart or triplanar')
    scan_strength = 1. if scan_projection == 'triplanar' else .5
    scan_tile_code = STONE_TILE if scan_sampling == 'offsets' else r"""
struct StoneTile {
 float4 sampleMap(Texture2D tex,SamplerState samp,float2 uv) {
  return tex.SampleGrad(samp,uv,ddx(uv),ddy(uv));
 }
}; StoneTile tile;
"""
    if candidate:
        scan_sampling,scan_projection,scan_strength='rotated_offsets','triplanar',1.
        scan_tile_code=SCAN_TILE_FILTERED
    if preset == 'scan':
        scan = json.loads((Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) /
                           'CliffScanImport.json').read_text(encoding='utf-8'))
        for suffix, path in scan['texture_references'].items():
            if not path.startswith(ROOT + '/Scan/'):
                raise RuntimeError('Scan texture outside isolated preview directory')
            asset = unreal.load_asset(path)
            if not isinstance(asset, unreal.Texture2D) or bool(asset.get_editor_property('srgb')) != (suffix == 'D'):
                raise RuntimeError('Scan texture type/colour-space mismatch: ' + path)
            if suffix == 'N' and (asset.get_editor_property('compression_settings') != unreal.TextureCompressionSettings.TC_NORMALMAP
                                  or not asset.get_editor_property('flip_green_channel')):
                raise RuntimeError('Scan GL normal needs normal compression and green flip')
            texture_assets[suffix] = asset
    if preset == 'slate':
        for suffix in ('D', 'N'):
            path = '/Game/ParagonRampage/Characters/Global/MaterialLayers/Rock/T_SlateTextureTile_000_'+suffix
            asset = unreal.load_asset(path)
            if not isinstance(asset, unreal.Texture2D):
                raise RuntimeError('Missing installed tileable slate texture: '+path)
            if bool(asset.get_editor_property('srgb')) != (suffix == 'D'):
                raise RuntimeError('Unexpected slate colour-space setting: '+path)
            if suffix == 'N' and asset.get_editor_property('compression_settings') != unreal.TextureCompressionSettings.TC_NORMALMAP:
                raise RuntimeError('Slate normal map requires native normal compression')
            texture_assets[suffix] = asset
    material = unreal.load_asset(material_path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(material_name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material) or material.get_path_name().split(".")[0] != material_path:
        raise RuntimeError("Unexpected preview material target")
    M.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property("tangent_space_normal", False)
    material.set_editor_property("two_sided", False)
    material.set_editor_property("used_with_nanite", True)
    # Commandlet-created materials do not automatically acquire editor usage.
    # Repair the isolated clay too so geometry reviews never use a fallback.
    clay = unreal.load_asset(ROOT + "/M_CliffClay")
    if not candidate and isinstance(clay, unreal.Material):
        clay.set_editor_property("used_with_nanite", True)
        M.recompile_material(clay)
        if not unreal.EditorAssetLibrary.save_loaded_asset(clay):
            raise RuntimeError("Could not save clay Nanite usage")
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
    if transport_masks == 'none':
        masks = node(unreal.MaterialExpressionConstant4Vector)
        masks.set_editor_property('constant', unreal.LinearColor(0.,0.,0.,1.))
    else:
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
    if preset == 'slate':
        texture_nodes = {}
        for suffix, asset in texture_assets.items():
            texture = node(unreal.MaterialExpressionTextureObjectParameter)
            texture.set_editor_property('parameter_name', 'SlateTexture'+suffix)
            texture.set_editor_property('texture', asset)
            texture.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix == 'N' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
            texture_nodes[suffix] = texture
        tile_size = parameter('SlateTileMetres', 4.)
        color = custom(STONE_TILE + r"""
float3 p=P*.01/max(TileMetres,.1),n=normalize(N);
float3 w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float3 x=tile.sampleMap(Albedo,AlbedoSampler,float2(p.y*s.x,p.z)).rgb;
float3 y=tile.sampleMap(Albedo,AlbedoSampler,float2(-p.x*s.y,p.z)).rgb;
float3 z=tile.sampleMap(Albedo,AlbedoSampler,float2(p.x*s.z,p.y)).rgb;
float3 stone=x*w.x+y*w.y+z*w.z;
stone=lerp(stone,float3(.265,.260,.250),Weather.z*.25);
return stone*lerp(1,.62,Weather.y);
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'Albedo':texture_nodes['D'],'TileMetres':tile_size,'Weather':weather})
    scan_nodes = {}
    scan_tile_size = None
    if preset == 'scan':
        for suffix, asset in texture_assets.items():
            texture = node(unreal.MaterialExpressionTextureObjectParameter)
            texture.set_editor_property('parameter_name', 'ScanTexture' + suffix)
            texture.set_editor_property('texture', asset)
            texture.set_editor_property('sampler_type',
                unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix == 'N' else
                unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if suffix == 'D' else
                unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
            scan_nodes[suffix] = texture
        scan_tile_size = parameter('ScanTileMetres', scan['width_metres'])
        # U/Z on the exposed face matches the geometry displacement exactly.
        # Unreal imports source chart V with reflected Y. PNG rows and DX
        # normal convention require -Z texture V and -normal.y below.
        color = custom(scan_tile_code + r"""
float3 p=P*.01/max(TileMetres,.1), n=normalize(N);
float top=smoothstep(.55,.88,abs(n.z));
float2 faceUV=float2(p.x,-p.z), topUV=p.xy;
float3 face=tile.sampleMap(Albedo,AlbedoSampler,faceUV).rgb;
float3 upper=tile.sampleMap(Albedo,AlbedoSampler,topUV).rgb;
float3 stone=lerp(face,upper,top);
// Restrained cool mineral tint; preserve measured fine-scale colour.
stone=lerp(dot(stone,float3(.2126,.7152,.0722)).xxx,stone,.4);
return stone*float3(.80,.86,.91)*lerp(.90,1.06,Weather.x);
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'Albedo':scan_nodes['D'],
                         'TileMetres':scan_tile_size,'Weather':weather})
        if scan_projection == 'triplanar':
            color = custom(scan_tile_code + r"""
float3 p=P*.01/max(TileMetres,.1), n=normalize(N);
float3 w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
// The signed projection convention matches slate; texture V is negated for
// top-origin imported pixels from the bottom-up source scan.
float3 x=tile.sampleMap(Albedo,AlbedoSampler,float2(p.y*s.x,-p.z)).rgb;
float3 y=tile.sampleMap(Albedo,AlbedoSampler,float2(-p.x*s.y,-p.z)).rgb;
float3 z=tile.sampleMap(Albedo,AlbedoSampler,float2(p.x*s.z,-p.y)).rgb;
float3 stone=x*w.x+y*w.y+z*w.z;
stone=lerp(dot(stone,float3(.2126,.7152,.0722)).xxx,stone,.4);
return stone*float3(.80,.86,.91)*lerp(.90,1.06,Weather.x);
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'Albedo':scan_nodes['D'],
                         'TileMetres':scan_tile_size,'Weather':weather})
    if candidate:
        color = custom(scan_tile_code + r"""
float3 p=P*.01/max(TileMetres,.1),n=normalize(N);
float3 w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float3 x=tile.sampleMap(Albedo,AlbedoSampler,float2(p.y*s.x,-p.z)).rgb;
float3 y=tile.sampleMap(Albedo,AlbedoSampler,float2(-p.x*s.y,-p.z)).rgb;
float3 z=tile.sampleMap(Albedo,AlbedoSampler,float2(p.x*s.z,-p.y)).rgb;
// Preserve source chromaticity: no blue lighting compensation, fake large
// cavities, unverified wetness or procedural colour camouflage.
return saturate((x*w.x+y*w.y+z*w.z)*max(AlbedoScale,0));
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'Albedo':scan_nodes['D'],
            'TileMetres':scan_tile_size,'AlbedoScale':parameter('AlbedoScale',albedo_scale)})
    fitted_wet = None
    fitted_wet_report = None
    if candidate and os.environ.get('DC_CLIFF_FITTED_WETNESS','0') == '1':
        saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
        wet_code,fitted_wet_report = fitted_wet_rock(saved)
        fitted_wet = custom(wet_code,types.CMOT_FLOAT1,
            {'P':position,'Strength':parameter('FittedWetness',1.)})
        color = custom('return Dry*lerp(1.,.52,Wet);',types.CMOT_FLOAT3,{'Dry':color,'Wet':fitted_wet})
    output(color, unreal.MaterialProperty.MP_BASE_COLOR)
    local_normal = custom(r"""
float3 n=normalize(N);
float3 gradient=(Detail.xyz+Fractures.xyz)*max(DetailStrength,0);
gradient-=n*dot(gradient,n);
return normalize(n-gradient);
""", types.CMOT_FLOAT3, {"N":normal,"Detail":detail,"Fractures":fractures,"DetailStrength":parameter("DetailStrength", 1.0)})
    if preset == 'slate':
        local_normal = custom(STONE_TILE + r"""
float3 p=P*.01/max(TileMetres,.1),n=normalize(N);
float3 w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float3 x=UnpackNormalMap(tile.sampleMap(NormalTex,NormalTexSampler,float2(p.y*s.x,p.z))).xyz;
float3 y=UnpackNormalMap(tile.sampleMap(NormalTex,NormalTexSampler,float2(-p.x*s.y,p.z))).xyz;
float3 z=UnpackNormalMap(tile.sampleMap(NormalTex,NormalTexSampler,float2(p.x*s.z,p.y))).xyz;
// Convert each tangent normal into a local slope, blend, then project to the
// true geometric tangent plane. Flat source normals preserve arbitrary N.
float3 perturb=(float3(0,x.x*s.x,x.y)/max(x.z,.2))*w.x
              +(float3(-y.x*s.y,0,y.y)/max(y.z,.2))*w.y
              +(float3(z.x*s.z,z.y,0)/max(z.z,.2))*w.z;
perturb-=n*dot(n,perturb);
return normalize(n+perturb*max(Strength,0));
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'NormalTex':texture_nodes['N'],'TileMetres':tile_size,'Strength':parameter('SlateNormalStrength', slate_strength)})
    if preset == 'scan':
        local_normal = custom(scan_tile_code + r"""
float3 p=P*.01/max(TileMetres,.1), n=normalize(N);
float2 uv=float2(p.x,-p.z);
float3 map=UnpackNormalMap(tile.sampleMap(NormalTex,NormalTexSampler,uv)).xyz;
float3 perturb=float3(map.x,0,-map.y)/max(map.z,.25);
perturb-=n*dot(n,perturb);
float face=1-smoothstep(.55,.88,abs(n.z));
return normalize(n+perturb*Strength*face);
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'NormalTex':scan_nodes['N'],
                         'TileMetres':scan_tile_size,'Strength':parameter('ScanNormalStrength', scan_strength)})
        if scan_projection == 'triplanar':
            local_normal = custom(scan_tile_code + r"""
float3 p=P*.01/max(TileMetres,.1),n=normalize(N);
float3 w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float3 x=UnpackNormalMap(tile.sampleMap(NormalTex,NormalTexSampler,float2(p.y*s.x,-p.z))).xyz;
float3 y=UnpackNormalMap(tile.sampleMap(NormalTex,NormalTexSampler,float2(-p.x*s.y,-p.z))).xyz;
float3 z=UnpackNormalMap(tile.sampleMap(NormalTex,NormalTexSampler,float2(p.x*s.z,-p.y))).xyz;
// DX maps contain negative texture-coordinate height gradients. Transform
// them with the SAME signed UV derivatives used by D/R, then project onto N.
float3 perturb=(float3(0,x.x*s.x,-x.y)/max(x.z,.25))*w.x
              +(float3(-y.x*s.y,0,-y.y)/max(y.z,.25))*w.y
              +(float3(z.x*s.z,-z.y,0)/max(z.z,.25))*w.z;
perturb-=n*dot(n,perturb);
return normalize(n+perturb*saturate(Strength));
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'NormalTex':scan_nodes['N'],
                         'TileMetres':scan_tile_size,'Strength':parameter('ScanNormalStrength', scan_strength)})
    if candidate:
        local_normal = custom(scan_tile_code + r"""
float3 metres=P*.01,p=metres/max(TileMetres,.1),n=normalize(N);
float3 w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float3 x=tile.sampleNormal(NormalTex,NormalTexSampler,float2(p.y*s.x,-p.z));
float3 y=tile.sampleNormal(NormalTex,NormalTexSampler,float2(-p.x*s.y,-p.z));
float3 z=tile.sampleNormal(NormalTex,NormalTexSampler,float2(p.x*s.z,-p.y));
float3 perturb=float3(0,x.x*s.x,-x.y)/max(x.z,.25)*w.x
    +float3(-y.x*s.y,0,-y.y)/max(y.z,.25)*w.y
    +float3(z.x*s.z,-z.y,0)/max(z.z,.25)*w.z;
perturb-=n*dot(n,perturb);
// Preserve the actual angular mesh planes once centimetre/metre scan normals
// become unresolved. Source4.299m texture scale never grows with camera range.
float footprint=max(length(ddx(metres)),length(ddy(metres)));
float resolved=1-smoothstep(.12,.65,footprint);
return normalize(n+perturb*saturate(Strength)*resolved);
""", types.CMOT_FLOAT3, {'P':position,'N':normal,'NormalTex':scan_nodes['N'],
            'TileMetres':scan_tile_size,'Strength':parameter('ScanNormalStrength',1.)})
    world_normal = node(unreal.MaterialExpressionTransform)
    world_normal.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    world_normal.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    connect(local_normal, "", world_normal, "")
    output(world_normal, unreal.MaterialProperty.MP_NORMAL)
    roughness = custom("return clamp(.82+.08*Weather.x+.06*Weather.z-.05*Weather.w-.24*Weather.y,.53,.94);",
                       types.CMOT_FLOAT1, {"Weather":weather})
    if preset == 'slate' and transport_masks == 'none':
        # No measured slate roughness map is installed. Do not borrow another
        # rock's map or interpret unknown packed channels as physical evidence.
        roughness = parameter('SlateDryRoughness', .80)
    if preset == 'scan':
        roughness = custom(scan_tile_code + r"""
float3 p=P*.01/max(TileMetres,.1),n=normalize(N);
float2 uv=float2(p.x,-p.z),topUV=p.xy;
float face=tile.sampleMap(Rough,RoughSampler,uv).r;
float top=tile.sampleMap(Rough,RoughSampler,topUV).r;
return clamp(lerp(face,top,smoothstep(.55,.88,abs(n.z))),.52,.94);
""", types.CMOT_FLOAT1, {'P':position,'N':normal,'Rough':scan_nodes['R'],'TileMetres':scan_tile_size})
        if scan_projection == 'triplanar':
            roughness = custom(scan_tile_code + r"""
float3 p=P*.01/max(TileMetres,.1),n=normalize(N);
float3 w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float x=tile.sampleMap(Rough,RoughSampler,float2(p.y*s.x,-p.z)).r;
float y=tile.sampleMap(Rough,RoughSampler,float2(-p.x*s.y,-p.z)).r;
float z=tile.sampleMap(Rough,RoughSampler,float2(p.x*s.z,-p.y)).r;
return clamp(dot(float3(x,y,z),w),.52,.94);
""", types.CMOT_FLOAT1, {'P':position,'N':normal,'Rough':scan_nodes['R'],'TileMetres':scan_tile_size})
    if fitted_wet is not None:
        roughness = custom('return lerp(Dry,.24,Wet);',types.CMOT_FLOAT1,{'Dry':roughness,'Wet':fitted_wet})
    output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    output(constant(.35), unreal.MaterialProperty.MP_SPECULAR)
    output(constant(0), unreal.MaterialProperty.MP_METALLIC)
    output(constant(1.) if preset in ('slate','scan') else custom("return lerp(1,.95,Fractures.w);", types.CMOT_FLOAT1, {"Fractures":fractures}), unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    M.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("Could not save isolated cliff material")
    if assign == "1":
        imported = json.loads((Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) /
                               "CliffLandformPreviewImport.json").read_text(encoding="utf-8"))
        mesh_path = imported["asset"]
        if not mesh_path.startswith(ROOT + "/"):
            raise RuntimeError("Preview import report names an asset outside the isolated folder")
        mesh = unreal.load_asset(mesh_path)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Missing isolated SM_CliffSector")
        mesh.set_material(0, material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError("Could not save preview mesh material assignment")
    report = dict(material=material_path, candidate_only=candidate, assigned_preview_mesh=assign=="1", vendor_textures_used=preset=='slate',
                  cc0_scan_used=preset=='scan', scan_source=scan,
                  preset=preset, texture_references={key:asset.get_path_name() for key,asset in texture_assets.items()},
                  texture_tile_metres=scan['width_metres'] if preset=='scan' else (4. if preset=='slate' else None),
                  texture_scale_note='Source-reported physical width' if preset=='scan' else 'Authored physical tile size, not a claimed scan measurement',
                  roughness_source='Verified linear scan roughness, clamped .52..94' if preset=='scan' else 'Authored response; packed vendor mask channels not used',
                  units="Local UE centimetres converted to metres; unit actor scale required",
                  vertex_contract="linear R normalized log-flow; G gross-deposition; B gross-erosion",
                  relief_bands_m=[.75,.18,.045] if preset=='procedural' else [], relief_heights_m=[.012,.002,.00055] if preset=='procedural' else [],
                  fracture_length_m=[.9,2.3] if preset=='procedural' else [], fracture_width_m=[.036,.086] if preset=='procedural' else [], fracture_depth_m=[.003,.009] if preset=='procedural' else [],
                  normal_fully_filtered_above_metres_per_pixel=.65 if candidate else .38 if preset=='procedural' else None, cracks_filtered_above_metres_per_pixel=.0645 if preset=='procedural' else None,
                  transport_masks=transport_masks,
                  slate_normal_strength=slate_strength if preset=='slate' else None,
                  slate_dry_roughness=.80 if preset=='slate' and transport_masks=='none' else None,
                  scan_normal_strength=scan_strength if preset=='scan' else None,
                  scan_sampling=scan_sampling if preset=='scan' else None,
                  scan_projection=scan_projection if preset=='scan' else None,
                  scan_cell_rotation_degrees=[-25.78310078,25.78310078] if candidate else None,
                  albedo_response='Source chromaticity preserved; authored achromatic brightness scale' if candidate else 'Existing preset response',
                  albedo_scale=albedo_scale if candidate else None,
                  fitted_wet_rock=fitted_wet_report,
                  drainage_response=('Fitted spray proximity only; no historical drainage masks' if fitted_wet_report else 'None: current whole-face geometry has no verified transport masks') if candidate else 'Existing preset response',
                  scan_coordinates=('Signed local triplanar, shared D/N/R offsets; DX gradients transformed by exact signed UV axes' if scan_projection=='triplanar' else 'Source-chart U/Z, native physical tile, top projection blend; DX normal mapped back to source axes') if preset=='scan' else None,
                  slate_sampling='Three-offset triangular blend per local triplanar projection; same albedo/normal offsets, explicit continuous UV mip gradients' if preset=='slate' else None,
                  roughness_bounds=[.52 if preset=='scan' else .53,.94], emissive=False, displacement=False,
                  runtime_integrated=False, actual_shader_compile_and_visual_review_pending=True)
    destination = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / ("CliffScanSurfacePreview.json" if candidate else "CliffRockMaterialPreview.json")
    destination.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    unreal.log_warning("DC_CLIFF_ROCK_PREVIEW " + json.dumps(report))


if __name__ == "__main__":
    create_material()
