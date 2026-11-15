"""Build ignored, adjustable cool-gray derivatives of the two owned bank scans.

Run in the project's Unreal Python commandlet. This changes only generated
materials, never the imported scan meshes, source materials or textures. Root
binds the results to the inlet instances; no world or runtime binding is done.
Defaults may be tuned with DC_CALDERA_BANK_TINT (JSON RGB),
DC_CALDERA_BANK_SATURATION and DC_CALDERA_BANK_COLOR_STRENGTH.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess

import unreal


DESTINATION = '/Game/DC/Maelstrom/GeneratedCliffPreview'
SOURCES = {
    'vefkehbva': (
        '/Game/LocalScans/vefkehbva/SM_Huge_Nordic_Coastal_Cliff_vefkehbva_Raw_53074843a3',
        '/Game/LocalScans/vefkehbva/M_vefkehbva_652c717246'),
    'vfmjadufa': (
        '/Game/LocalScans/vfmjadufa/SM_Massive_Nordic_Coastal_Cliff_vfmjadufa_Raw_4064086b07',
        '/Game/LocalScans/vfmjadufa/M_vfmjadufa_9191ee0556'),
}
SOURCE_TAG = 'DCCalderaBankSource'
COLOR_MARKER = 'DC_CalderaBankColor_v1'
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def connection(material, prop):
    node = MEL.get_material_property_input_node(material, prop)
    if node is None:
        raise RuntimeError('Missing source connection: '+str(prop))
    return node, MEL.get_material_property_input_node_output_name(material, prop)


def parameter(material, cls, name, value):
    found = [node for node in MEL.get_material_expressions(material)
             if isinstance(node, cls) and str(node.get_editor_property('parameter_name')) == name]
    if len(found) > 1:
        raise RuntimeError('Duplicate generated parameter: '+name)
    node = found[0] if found else MEL.create_material_expression(material, cls, -500, 700)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('group', 'Caldera Bank Color')
    node.set_editor_property('default_value', value)
    return node


def package_hash(project, asset):
    package = asset.get_path_name().split('.')[0]
    if not package.startswith('/Game/'):
        raise RuntimeError('Expected a local owned source: '+package)
    path = (project/'Content'/package[len('/Game/'):]).with_suffix('.uasset')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())).resolve()
    tint = json.loads(os.environ.get('DC_CALDERA_BANK_TINT', '[0.72,0.80,0.88]'))
    saturation = float(os.environ.get('DC_CALDERA_BANK_SATURATION', '.12'))
    strength = float(os.environ.get('DC_CALDERA_BANK_COLOR_STRENGTH', '1.0'))
    if (not isinstance(tint, list) or len(tint) != 3
            or not all(isinstance(v, (int, float)) and math.isfinite(v) and v >= 0 for v in tint)
            or not math.isfinite(saturation) or not 0 <= saturation <= 1
            or not math.isfinite(strength) or not 0 <= strength <= 1):
        raise RuntimeError('Expected finite RGB tint >=0, saturation/strength in [0,1]')
    props = unreal.MaterialProperty
    records = []
    for scan_id, (mesh_path, source_path) in SOURCES.items():
        destination = DESTINATION+'/M_CalderaBank_'+scan_id
        sentinel = (project/'Content'/destination[len('/Game/'):]).with_suffix('.uasset')
        if subprocess.run(['git', 'check-ignore', '--quiet', '--', str(sentinel)], cwd=project).returncode:
            raise RuntimeError('Generated bank material destination must be Git-ignored: '+str(sentinel))
        mesh, source = unreal.load_asset(mesh_path), unreal.load_asset(source_path)
        if not isinstance(mesh, unreal.StaticMesh) or not isinstance(source, unreal.Material):
            raise RuntimeError('Missing expected owned mesh/material for '+scan_id)
        slots = mesh.get_editor_property('static_materials')
        if not slots or any(mesh.get_material(i) != source for i in range(len(slots))):
            raise RuntimeError('Bank mesh source material binding changed: '+mesh_path)
        source_hash = package_hash(project, source)
        texture_paths = sorted({node.get_editor_property('texture').get_path_name()
                                for node in MEL.get_material_expressions(source)
                                if isinstance(node, unreal.MaterialExpressionTextureSample)
                                and node.get_editor_property('texture') is not None})
        source_textures = {path: package_hash(project, unreal.load_asset(path)) for path in texture_paths}
        for prop in (props.MP_BASE_COLOR, props.MP_NORMAL, props.MP_ROUGHNESS):
            connection(source, prop)
        if EAL.does_asset_exist(destination):
            material = unreal.load_asset(destination)
            if not isinstance(material, unreal.Material) or EAL.get_metadata_tag(material, SOURCE_TAG) != source.get_path_name():
                raise RuntimeError('Refusing to modify an unrelated generated asset: '+destination)
        else:
            material = EAL.duplicate_asset(source.get_path_name(), destination)
            if not isinstance(material, unreal.Material):
                raise RuntimeError('Source material duplication failed: '+destination)
            EAL.set_metadata_tag(material, SOURCE_TAG, source.get_path_name())
        # Duplicate the complete graph. Only BaseColor receives a new operation;
        # original UV0 sampling, normal convention and roughness remain intact.
        preserved = {prop: connection(material, prop) for prop in (props.MP_NORMAL, props.MP_ROUGHNESS)}
        base, base_pin = connection(material, props.MP_BASE_COLOR)
        already = isinstance(base, unreal.MaterialExpressionCustom) and base.get_editor_property('description') == COLOR_MARKER
        tint_node = parameter(material, unreal.MaterialExpressionVectorParameter,
                              'CalderaBankTint', unreal.LinearColor(*tint, 1.0))
        saturation_node = parameter(material, unreal.MaterialExpressionScalarParameter,
                                    'CalderaBankSaturation', saturation)
        strength_node = parameter(material, unreal.MaterialExpressionScalarParameter,
                                  'CalderaBankColorStrength', strength)
        if not already:
            color = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, -150, 0)
            color.set_editor_property('description', COLOR_MARKER)
            color.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            color.set_editor_property('code', '''
float luminance=dot(Color,float3(.2126,.7152,.0722));
float3 cooled=lerp(luminance.xxx,Color,saturate(Saturation))*max(Tint,float3(0,0,0));
return lerp(Color,cooled,saturate(Strength));
''')
            inputs = {'Color': (base, base_pin), 'Tint': (tint_node, ''),
                      'Saturation': (saturation_node, ''), 'Strength': (strength_node, '')}
            pins = []
            for name in inputs:
                pin = unreal.CustomInput(); pin.set_editor_property('input_name', name); pins.append(pin)
            color.set_editor_property('inputs', pins)
            for name, (node, pin) in inputs.items():
                if not MEL.connect_material_expressions(node, pin, color, name):
                    raise RuntimeError('Failed bank color input: '+name)
            if not MEL.connect_material_property(color, '', props.MP_BASE_COLOR):
                raise RuntimeError('Failed bank color output: '+destination)
        material.set_editor_property('used_with_nanite', True)
        material.set_editor_property('used_with_instanced_static_meshes', True)
        for prop, before in preserved.items():
            if connection(material, prop) != before:
                raise RuntimeError('Non-color source connection changed: '+str(prop))
        MEL.recompile_material(material)
        if not EAL.save_loaded_asset(material, only_if_is_dirty=False):
            raise RuntimeError('Could not save '+destination)
        if package_hash(project, source) != source_hash or any(
                package_hash(project, unreal.load_asset(path)) != digest for path, digest in source_textures.items()):
            raise RuntimeError('Owned source asset changed while building '+scan_id)
        records.append(dict(scan_id=scan_id, source_mesh=mesh.get_path_name(),
            source_material=source.get_path_name(), source_material_sha256=source_hash,
            generated_material=material.get_path_name(), source_texture_sha256=source_textures,
            parameters={'CalderaBankTint': tint, 'CalderaBankSaturation': saturation,
                        'CalderaBankColorStrength': strength},
            uv_mapping='Duplicated source graph; original UV0 and texture references retained',
            normal_and_roughness='Original graph connections unchanged; no wetness adjustment',
            used_with_nanite=True, used_with_instanced_static_meshes=True))
    report = dict(schema='directorscut.caldera_bank_materials.v1', materials=records,
                  vendor_assets_modified=False, runtime_binding_modified=False,
                  validation='Graph and source-file checks only; native visual acceptance pending')
    saved.mkdir(parents=True, exist_ok=True)
    report_path = saved/'CalderaBankMaterials.json'
    temporary = report_path.with_suffix('.pending')
    temporary.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    temporary.replace(report_path)
    unreal.log_warning('DC_CALDERA_BANK_MATERIALS '+str(report_path))


if __name__ == '__main__':
    main()
