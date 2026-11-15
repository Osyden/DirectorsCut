"""Physical rock layer for extended caldera faces and inlet banks.

Called by import_cliff_geology.py after building the unique upper-cliff graph.
Can also run once against the existing graph in Unreal's Python commandlet.
DC_CLIFF_PHYSICAL_TILE overrides the owned projected tile directory;
DC_CLIFF_PHYSICAL_AUDIT overrides the latest imported source-frame audit.
Only the generated material is changed. Existing owned texture pixels stay intact.
"""
import json
import os
from pathlib import Path
import sys

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_cliff_preview_material import SCAN_TILE_FILTERED, STONE_NOISE, FRACTURES
from import_owned_cliff_scans import sha


MACRO = r"""
struct RegionalStone {
 float hash(float3 p) {
  p=frac(p*.1031);p+=dot(p,p.yzx+33.33);
  return frac((p.x+p.y)*p.z);
 }
 float noise(float3 p) {
  float3 i=floor(p),f=frac(p);f=f*f*(3-2*f);
  return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),
                   lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
              lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),
                   lerp(hash(i+float3(0,1,1)),hash(i+1),f.x),f.y),f.z);
 }
};RegionalStone region;
float broad=region.noise(P/281.+float3(7.1,11.3,2.9));
float middle=region.noise(P/113.+float3(21.7,-8.4,19.3));
float patch=region.noise(P/37.+float3(-3.1,8.8,12.2));
return float3(.50*broad+.32*middle+.18*patch,middle,patch);
"""

# The projected owned normal contains the source texture's fine grain only.
# Its measured depth relief is a separate installed normal. Regional structure
# adds low-frequency fracture shading on the newly exposed bank; it is not
# displacement and does not claim to change the bank silhouette or cast shadows.
STRUCTURE = STONE_NOISE + r"""
float3 p=P;
float footprint=max(length(ddx(p)),length(ddy(p)));
float3 q=float3(.8*p.x-.6*p.y,.6*p.x+.8*p.y,p.z);
float4 a=rock.evaluate(q/31.+float3(3.2,17.8,-6.7));
float4 b=rock.evaluate(p/11.+float3(-13.7,9.1,21.3));
float4 c=rock.evaluate(q/3.7+float3(33.1,-5.2,12.7));
// Fold the height field into discontinuous slopes at irregular joints. Two
// incommensurate scales and a rotated field avoid a tiled block-wall pattern.
float sa=lerp(-1.,1.,smoothstep(.47,.53,a.x));
float sb=lerp(-1.,1.,smoothstep(.475,.525,b.x));
float3 ga=float3(.8*a.y+.6*a.z,-.6*a.y+.8*a.z,a.w);
float3 gc=float3(.8*c.y+.6*c.z,-.6*c.y+.8*c.z,c.w);
float3 gradient=ga*(10./31.)*sa+b.yzw*(3.8/11.)*sb;
gradient+=gc*(.45/3.7)*(1-smoothstep(.6,2.2,footprint));
return float4(gradient,.6*a.x+.4*b.x);
"""

# Independent terminating joints, not a Voronoi grid or painted cell borders.
# Existing derivative-filtered centimetre joints are scaled to 11--28m length,
# 0.43--1.03m full width. P here is already arena metres.
REGIONAL_JOINTS = FRACTURES.replace('float3 p=P*.01;', 'float3 p=P/12.;')


def apply_physical_layer(material, project, detail_source=None, rebuilding=False):
    project = Path(project)
    saved = project / 'Saved'
    mel = unreal.MaterialEditingLibrary
    marker = 'DCPhysicalCliffLayer.v1'
    # UE can enumerate pending-kill expression objects after delete_all_material_expressions.
    # The full importer has already cleared and rebuilt the graph explicitly.
    if not rebuilding and any(str(x.get_editor_property('desc')) == marker for x in mel.get_material_expressions(material)):
        raise RuntimeError('Physical layer already installed; rerun import_cliff_geology.py to rebuild cleanly')
    if detail_source is None:
        detail_source = json.loads((saved / 'CliffGeologyImport.json').read_text()).get('detail_source')
    tile_value = os.environ.get('DC_CLIFF_PHYSICAL_TILE') or (detail_source or {}).get('source_directory')
    if not tile_value:
        raise RuntimeError('Set DC_CLIFF_PHYSICAL_TILE to the measured owned face tile directory')
    tile_dir = Path(tile_value).resolve()
    tile = json.loads((tile_dir / 'tile_manifest.json').read_text())
    width, height = float(tile['physical_width_m']), float(tile['physical_height_m'])
    if width <= 0 or height <= 0:
        raise RuntimeError('Owned rock physical dimensions must be positive')
    audit_value = os.environ.get('DC_CLIFF_PHYSICAL_AUDIT')
    audit_path = Path(audit_value) if audit_value else saved / 'CliffPreviewSourceAudit.json'
    if not audit_path.is_file():
        audit_path = project / 'Content/DC/Maelstrom/GeneratedCliffPreview/Runtime/CliffPreviewSourceAudit.json'
    audit = json.loads(audit_path.read_text())
    if not audit.get('geometry_passed'):
        raise RuntimeError('Physical cliff mapping requires the passed imported-frame audit')
    frame = audit['geometry']['arena_reconstruction']
    origin, basis = frame['origin_cm'], frame['basis_for_imported_xyz']
    for i in range(3):
        for j in range(3):
            product = sum(basis[i][k]*basis[j][k] for k in range(3))
            if abs(product - (1. if i == j else 0.)) > 1.e-5:
                raise RuntimeError('Physical cliff mapping requires the audited signed unit-scale basis')

    def node(kind):
        return mel.create_material_expression(material, kind)

    def link(source, target, name=''):
        if not mel.connect_material_expressions(source, '', target, name):
            raise RuntimeError('Physical cliff connection failed: ' + name)

    def custom(code, kind, inputs):
        result = node(unreal.MaterialExpressionCustom)
        result.set_editor_property('code', code)
        result.set_editor_property('output_type', kind)
        pins = []
        for name in inputs:
            pin = unreal.CustomInput()
            pin.set_editor_property('input_name', name)
            pins.append(pin)
        result.set_editor_property('inputs', pins)
        for name, source in inputs.items():
            link(source, result, name)
        return result

    def transform(source, from_space, to_space):
        result = node(unreal.MaterialExpressionTransform)
        result.set_editor_property('transform_source_type', from_space)
        result.set_editor_property('transform_type', to_space)
        link(source, result)
        return result

    def vec(values):
        return 'float3(' + ','.join(format(float(x), '.12g') for x in values) + ')'

    types = unreal.CustomMaterialOutputType
    properties = {'albedo': unreal.MaterialProperty.MP_BASE_COLOR,
                  'normal': unreal.MaterialProperty.MP_NORMAL,
                  'roughness': unreal.MaterialProperty.MP_ROUGHNESS}
    original = {role: mel.get_material_property_input_node(material, prop) for role, prop in properties.items()}
    if not all(original.values()):
        raise RuntimeError('Build the existing unique cliff graph before adding its physical layer')
    world = node(unreal.MaterialExpressionWorldPosition)
    local = node(unreal.MaterialExpressionTransformPosition)
    local.set_editor_property('transform_source_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
    local.set_editor_property('transform_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
    link(world, local)
    local_n = transform(node(unreal.MaterialExpressionVertexNormalWS),
                        unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
                        unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
    force = node(unreal.MaterialExpressionScalarParameter)
    force.set_editor_property('parameter_name', 'PhysicalInletBank')
    force.set_editor_property('default_value', 0.)
    force.set_editor_property('use_custom_primitive_data', True)
    force.set_editor_property('primitive_data_index', 0)
    # Imported cliff local axes include the audited FBX Y reflection/offset.
    # Procedural fallback inlet geometry already uses arena coordinates (CPD0=1).
    basis_code = '+'.join('P.%s*%s' % (axis, vec(basis[i])) for i, axis in enumerate('xyz'))
    position = custom('return lerp(%s+%s,P,saturate(Bank))*.01;' % (vec(origin), basis_code),
                      types.CMOT_FLOAT3, {'P': local, 'Bank': force})
    normal = custom('return normalize(lerp(%s,P,saturate(Bank)));' % basis_code,
                    types.CMOT_FLOAT3, {'P': local_n, 'Bank': force})
    weight = custom('return max(saturate(Bank),1-smoothstep(0.,45.,P.z));',
                    types.CMOT_FLOAT1, {'P': position, 'Bank': force})
    weight.set_editor_property('desc', marker)
    macro = custom(MACRO, types.CMOT_FLOAT3, {'P': position})
    structure = custom(STRUCTURE, types.CMOT_FLOAT4, {'P': position})
    joints = custom(REGIONAL_JOINTS, types.CMOT_FLOAT4, {'P': position, 'N': normal})

    texture_nodes, texture_paths = {}, {}
    for role, filename in [('albedo', 'T_NordicFace_BaseColor.tga'),
                           ('normal', 'T_NordicFace_NormalGL.tga'),
                           ('roughness', 'T_NordicFace_Roughness.tga')]:
        digest = sha(tile_dir / filename)
        if tile['files'].get(filename) != digest:
            raise RuntimeError('Owned physical tile hash mismatch: ' + filename)
        target = '/Game/LocalScans/nordic_face_tile/' + Path(filename).stem + '_' + digest[:10]
        if role == 'normal':
            if not detail_source or abs(float(detail_source['physical_width_m'])-width) > .001 or abs(float(detail_source['physical_height_m'])-height) > .001:
                raise RuntimeError('Physical cliffs need the matching measured-depth detail normal')
            # This is the installed normal derived from the measured owned
            # height, not the fine-source-only texture used in the first pass.
            relief_digest = detail_source['files']['T_NordicFace_NormalGL.tga']
            target = '/Game/LocalScans/cliff_geology/T_CliffDetail_' + relief_digest[:12]
        asset = unreal.load_asset(target)
        if not isinstance(asset, unreal.Texture2D):
            raise RuntimeError('Import the owned face tile first: ' + target)
        if bool(asset.get_editor_property('srgb')) != (role == 'albedo'):
            raise RuntimeError('Unexpected owned tile color space: ' + target)
        if asset.get_editor_property('address_x') != unreal.TextureAddress.TA_WRAP or asset.get_editor_property('address_y') != unreal.TextureAddress.TA_WRAP:
            raise RuntimeError('Physical tile needs its existing wrap sampler: ' + target)
        if role == 'normal' and not asset.get_editor_property('flip_green_channel'):
            raise RuntimeError('Owned OpenGL tile must already have exactly one Unreal green flip')
        texture = node(unreal.MaterialExpressionTextureObject)
        texture.set_editor_property('texture', asset)
        texture.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal'
                                    else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'albedo'
                                    else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        texture_nodes[role], texture_paths[role] = texture, asset.get_path_name()

    common = SCAN_TILE_FILTERED + '''
float3 n=normalize(N),w=pow(abs(n),4);w/=max(w.x+w.y+w.z,.00001);
float3 s=float3(n.x<0?-1:1,n.y<0?-1:1,n.z<0?-1:1);
float2 dimensions=float2(%s,%s);
float2 ux=float2(P.y*s.x,-P.z)/dimensions;
float2 uy=float2(-P.x*s.y,-P.z)/dimensions;
float2 uz=float2(P.x*s.z,-P.y)/dimensions;
''' % (width, height)
    albedo = custom(common + '''
float3 stone=tile.sampleMap(Tex,TexSampler,ux).rgb*w.x
            +tile.sampleMap(Tex,TexSampler,uy).rgb*w.y
            +tile.sampleMap(Tex,TexSampler,uz).rgb*w.z;
// Keep restrained measured grain, but remove the source's ochre mineral tint.
// The bank is the same cool grey bedrock family as the preserved upper cliff.
float grain=lerp(1.,clamp(dot(stone,float3(.2126,.7152,.0722))/.18,.78,1.22),.32);
stone=float3(.130,.145,.160)*grain*lerp(.72,1.28,Macro.x);
stone*=lerp(.88,1.12,Structure.w);
stone*=1-.09*Joints.w;
return clamp(stone,.04,.26);
''', types.CMOT_FLOAT3, {'P': position, 'N': normal, 'Tex': texture_nodes['albedo'],
                       'Macro': macro, 'Structure': structure, 'Joints': joints})
    rough = custom(common + '''
float r=tile.sampleMap(Tex,TexSampler,ux).r*w.x
       +tile.sampleMap(Tex,TexSampler,uy).r*w.y
       +tile.sampleMap(Tex,TexSampler,uz).r*w.z;
return clamp(r+(Macro.z-.5)*.06,.55,.94);
''', types.CMOT_FLOAT1, {'P': position, 'N': normal, 'Tex': texture_nodes['roughness'], 'Macro': macro})
    physical_n = custom(common + '''
float3 x=tile.sampleNormal(Tex,TexSampler,ux);
float3 y=tile.sampleNormal(Tex,TexSampler,uy);
float3 z=tile.sampleNormal(Tex,TexSampler,uz);
float3 perturb=float3(0,x.x*s.x,-x.y)/max(x.z,.3)*w.x
              +float3(-y.x*s.y,0,-y.y)/max(y.z,.3)*w.y
              +float3(z.x*s.z,-z.y,0)/max(z.z,.3)*w.z;
perturb-=n*dot(perturb,n);
// Keep the owned 8m relief at native scale; suppress only unresolved detail.
float footprint=max(length(ddx(P)),length(ddy(P)));
perturb*=.60*(1-smoothstep(.8,3.2,footprint));
float3 structural=Structure.xyz+Joints.xyz*1.7;
structural-=n*dot(structural,n);
perturb-=structural;
perturb*=min(1.,.95/max(length(perturb),.001));
return normalize(n+perturb);
''', types.CMOT_FLOAT3, {'P': position, 'N': normal, 'Tex': texture_nodes['normal'],
                       'Structure': structure, 'Joints': joints})
    inverse = 'float3(' + ','.join('dot(N,%s)' % vec(axis) for axis in basis) + ')'
    physical_local_n = custom('return normalize(lerp(%s,N,saturate(Bank)));' % inverse,
                              types.CMOT_FLOAT3, {'N': physical_n, 'Bank': force})
    physical_tangent_n = transform(physical_local_n, unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                                   unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    for role, physical in [('albedo', albedo), ('roughness', rough), ('normal', physical_tangent_n)]:
        code = 'return normalize(lerp(Upper,Physical,Weight));' if role == 'normal' else 'return lerp(Upper,Physical,Weight);'
        output = custom(code, types.CMOT_FLOAT1 if role == 'roughness' else types.CMOT_FLOAT3,
                        {'Upper': original[role], 'Physical': physical, 'Weight': weight})
        if not mel.connect_material_property(output, '', properties[role]):
            raise RuntimeError('Physical cliff output failed: ' + role)
    report = dict(schema='directorscut.physical_cliff_mapping.v2', material=material.get_path_name(),
                  source_audit=str(audit_path), arena_frame=frame, tile_manifest=str(tile_dir / 'tile_manifest.json'),
                  textures=texture_paths, tile_metres=[width, height], upper_blend_z_m=[0.,45.],
                  mapping='World position/normal transformed to object, then audited arena frame; no world-locked projection',
                  repetition='Three stochastic rotated offsets per triplanar axis; shared D/N/R sampling; macro noise 37/113/281m',
                  rock_structure='Owned measured-depth normal at native8m;31/11/3.7m folded relief and sparse11--28m terminating joints',
                  rock_color='Cool neutral grey reflectance(.130,.145,.160), owned luminance grain limited to7percent',
                  structure_limit='Material normals only; geometric silhouette and cast-shadow relief remain the mesh responsibility',
                  normal_source=detail_source,
                  fallback_inlet='Custom primitive float0=1 uses direct arena frame and full physical layer',
                  unique_upper_preserved=True, geometry_modified=False)
    (saved / 'CliffPhysicalMappingImport.json').write_text(json.dumps(report, indent=2))
    return report


if __name__ == '__main__':
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    material = unreal.load_asset('/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffScanSurface')
    if not isinstance(material, unreal.Material):
        raise RuntimeError('Install the unique cliff material first')
    apply_physical_layer(material, project)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError('Could not save physical cliff material')
    unreal.log_warning('DC_CLIFF_PHYSICAL_MAPPING_IMPORTED')
