"""Install unique pilot/context geology maps on the local cliff preview only.

DC_CLIFF_GEOLOGY_ATLAS and DC_CLIFF_GEOLOGY_CONTEXT name generated directories.
Both domains clamp; context uses full-ring UV bounds. The pilot fades by its
original vertex mask AND UV coverage, so roof/outside UVs never repeat it.
Optional DC_CLIFF_GEOLOGY_DETAIL names an owned native-scale normal-only tile;
only this fine detail repeats, with two orientations and bounded slope.
No geometry, maps, sky, gameplay or vendor source files are changed.
"""
import json
import os
from pathlib import Path
import subprocess
import sys

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_owned_cliff_scans import import_task, sha


def main():
    directories = {}
    manifests = {}
    for key, env in [('pilot', 'DC_CLIFF_GEOLOGY_ATLAS'), ('context', 'DC_CLIFF_GEOLOGY_CONTEXT')]:
        value = os.environ.get(env)
        if not value:
            raise RuntimeError('Set ' + env)
        directories[key] = Path(value).resolve()
        manifests[key] = json.loads((directories[key] / 'tile_manifest.json').read_text())
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    for relative in ['Content/LocalScans/cliff_geology/owned.uasset',
                     'Content/DC/Maelstrom/GeneratedCliffPreview/M_CliffScanSurface.uasset']:
        if subprocess.run(['git', 'check-ignore', '--quiet', '--', str(project / relative)], cwd=project).returncode:
            raise RuntimeError('Generated preview assets must remain ignored: ' + relative)
    textures = {}
    detail = None
    detail_manifest = None
    filenames = {'albedo': 'T_NordicFace_BaseColor.tga', 'normal': 'T_NordicFace_NormalGL.tga',
                 'roughness': 'T_NordicFace_Roughness.tga'}
    for domain in directories:
        textures[domain] = {}
        for role, filename in filenames.items():
            path = directories[domain] / filename
            digest = sha(path)
            if manifests[domain]['files'].get(filename) != digest:
                raise RuntimeError('Generated map/hash mismatch: ' + str(path))
            name = 'T_Geology_' + domain + '_' + role + '_' + digest[:12]
            folder = '/Game/LocalScans/cliff_geology'
            target = folder + '/' + name
            texture = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
            if texture is None:
                texture = next((x for x in import_task(path, folder, name) if isinstance(x, unreal.Texture2D)), None)
            if texture is None:
                raise RuntimeError('Texture import failed: ' + str(path))
            texture.set_editor_property('srgb', role == 'albedo')
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP
                                        if role == 'normal' else unreal.TextureCompressionSettings.TC_DEFAULT)
            texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
            texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
            if role == 'normal':
                texture.set_editor_property('flip_green_channel', True)
            if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
                raise RuntimeError('Texture save failed: ' + str(path))
            textures[domain][role] = texture

    detail_path = os.environ.get('DC_CLIFF_GEOLOGY_DETAIL')
    if detail_path:
        detail_folder = Path(detail_path).resolve()
        detail_manifest = json.loads((detail_folder / 'tile_manifest.json').read_text())
        filename = 'T_NordicFace_NormalGL.tga'
        source = detail_folder / filename
        digest = sha(source)
        if detail_manifest['files'].get(filename) != digest:
            raise RuntimeError('Detail normal/hash mismatch')
        name = 'T_CliffDetail_' + digest[:12]
        folder = '/Game/LocalScans/cliff_geology'
        target = folder + '/' + name
        detail = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
        if detail is None:
            detail = next((x for x in import_task(source, folder, name) if isinstance(x, unreal.Texture2D)), None)
        if detail is None:
            raise RuntimeError('Detail normal import failed')
        detail.set_editor_property('srgb', False)
        detail.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        detail.set_editor_property('flip_green_channel', True)
        detail.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
        detail.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP)
        if not unreal.EditorAssetLibrary.save_loaded_asset(detail, only_if_is_dirty=False):
            raise RuntimeError('Detail normal save failed')

    folder = '/Game/DC/Maelstrom/GeneratedCliffPreview'
    name = 'M_CliffScanSurface'
    target = folder + '/' + name
    material = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    mel.delete_all_material_expressions(material)
    for prop, value in [('used_with_nanite', True), ('used_with_instanced_static_meshes', True),
                        ('tangent_space_normal', True), ('two_sided', False),
                        ('blend_mode', unreal.BlendMode.BLEND_OPAQUE),
                        ('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)]:
        material.set_editor_property(prop, value)
    def node(kind):
        return mel.create_material_expression(material, kind)
    def link(source, pin, dest, input_name):
        if not mel.connect_material_expressions(source, pin, dest, input_name):
            raise RuntimeError('Material connection failed: ' + input_name)
    def custom(code, output_type, inputs):
        expression = node(unreal.MaterialExpressionCustom)
        expression.set_editor_property('code', code)
        expression.set_editor_property('output_type', output_type)
        pins = []
        for input_name in inputs:
            pin = unreal.CustomInput()
            pin.set_editor_property('input_name', input_name)
            pins.append(pin)
        expression.set_editor_property('inputs', pins)
        for input_name, (source, pin) in inputs.items():
            link(source, pin, expression, input_name)
        return expression
    def scalar(value):
        expression = node(unreal.MaterialExpressionConstant)
        expression.set_editor_property('r', value)
        return expression
    def output(source, pin, prop):
        if not mel.connect_material_property(source, pin, prop):
            raise RuntimeError('Material output failed: ' + str(prop))

    uv = node(unreal.MaterialExpressionTextureCoordinate)
    uv.set_editor_property('coordinate_index', 0)
    vertex = node(unreal.MaterialExpressionVertexColor)
    types = unreal.CustomMaterialOutputType
    weight = custom('float2 d=min(UV,1-UV); return saturate(Mask)*smoothstep(0,.035,d.x)*smoothstep(0,.06,d.y);',
                    types.CMOT_FLOAT1, {'UV': (uv, ''), 'Mask': (vertex, 'R')})
    uv_min = manifests['context'].get('context_uv_min', [-7., -1.25])
    uv_max = manifests['context'].get('context_uv_max', [13.3, 2.35])
    if len(uv_min) != 2 or len(uv_max) != 2 or any(b <= a for a, b in zip(uv_min, uv_max)):
        raise RuntimeError('Invalid context UV extent')
    # Legacy FBX import turns authored V into 1-V. TGA import also reverses
    # the bottom-origin image rows; normalize within the transformed bounds
    # so the two flips align at the same physical elevation.
    runtime_uv_min = [uv_min[0], 1. - uv_max[1]]
    runtime_uv_max = [uv_max[0], 1. - uv_min[1]]
    transform = ('return (UV-float2(%s,%s))/float2(%s,%s);' %
                 (runtime_uv_min[0], runtime_uv_min[1], runtime_uv_max[0] - runtime_uv_min[0],
                  runtime_uv_max[1] - runtime_uv_min[1]))
    context_uv = custom(transform, types.CMOT_FLOAT2, {'UV': (uv, '')})
    for role, prop in [('albedo', unreal.MaterialProperty.MP_BASE_COLOR),
                       ('roughness', unreal.MaterialProperty.MP_ROUGHNESS),
                       ('normal', unreal.MaterialProperty.MP_NORMAL)]:
        samples = {}
        for domain in textures:
            sample = node(unreal.MaterialExpressionTextureSample)
            sample.set_editor_property('texture', textures[domain][role])
            sample.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
                                       if role == 'normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
                                       if role == 'albedo' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
            # Empty input name selects the texture's first (UV) pin. The UE5.8
            # Python API uses shortened names, not the displayed Coordinates.
            if domain == 'context':
                link(context_uv, '', sample, '')
                if role == 'normal':
                    # Strong full-ring cell facets resembled masonry in the
                    # native review. Keep only broad geological perturbation;
                    # measured fractured stone supplies the local detail.
                    context_strength=.55 if manifests['context'].get('full_ring_geometry_source') else .18
                    sample = custom('return normalize(float3(N.xy*%s,N.z));' % context_strength,
                                    types.CMOT_FLOAT3, {'N': (sample, '')})
            elif role == 'normal':
                sample = custom('return normalize(float3(N.xy*.55,N.z));',
                                types.CMOT_FLOAT3, {'N': (sample, '')})
            samples[domain] = sample
        blend = node(unreal.MaterialExpressionLinearInterpolate)
        pin = 'R' if role == 'roughness' else ''
        link(samples['context'], pin, blend, 'A')
        link(samples['pilot'], pin, blend, 'B')
        link(weight, '', blend, 'Alpha')
        if role == 'normal':
            blend = custom('return normalize(N);', types.CMOT_FLOAT3, {'N': (blend, '')})
            if detail is not None:
                physical = custom('return UV*float2(497.418836818384,280.);',
                                  types.CMOT_FLOAT2, {'UV': (uv, '')})
                width = float(detail_manifest['physical_width_m'])
                height = float(detail_manifest['physical_height_m'])
                detail_samples = []
                for second in (False, True):
                    code = ('float2 Q=float2(.838670568*P.x-.544639035*P.y,.544639035*P.x+.838670568*P.y)*1.173; '
                            if second else 'float2 Q=P; ')
                    code += 'return Q/float2(%s,%s)+float2(%s,%s);' % (width,height,.37 if second else 0.,.61 if second else 0.)
                    mapped = custom(code, types.CMOT_FLOAT2, {'P': (physical, '')})
                    detail_sample = node(unreal.MaterialExpressionTextureSample)
                    detail_sample.set_editor_property('texture', detail)
                    detail_sample.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
                    link(mapped, '', detail_sample, '')
                    detail_samples.append(detail_sample)
                # Rotate the second sampled slope back into the surface frame.
                # RNM preserves the unique broad normal while adding fine stone.
                blend = custom('''float3 B2=float3(.838670568*B.x+.544639035*B.y,-.544639035*B.x+.838670568*B.y,B.z);
float2 S=.32*(A.xy/max(A.z,.25)+B2.xy/max(B2.z,.25));
S*=min(1.,.445/max(length(S),.001));
S*=1.-.55*saturate(length(N.xy)/.65);
float3 D=normalize(float3(S,1));
float3 T=N+float3(0,0,1); float3 U=D*float3(-1,-1,1);
return normalize(T*dot(T,U)/max(T.z,.001)-U);''', types.CMOT_FLOAT3,
                               {'N': (blend, ''), 'A': (detail_samples[0], ''), 'B': (detail_samples[1], '')})
        output(blend, '', prop)
    output(scalar(0.), '', unreal.MaterialProperty.MP_METALLIC)
    output(scalar(.3), '', unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError('Could not save geology preview material')
    saved = project / 'Saved'
    report = dict(schema='directorscut.cliff_geology_import.v1', material=material.get_path_name(),
                  atlas_manifest=str(directories['pilot'] / 'tile_manifest.json'), atlas_source=manifests['pilot'],
                  context_manifest=str(directories['context'] / 'tile_manifest.json'), context_source=manifests['context'],
                  textures={domain: {role: tex.get_path_name() for role, tex in roles.items()} for domain, roles in textures.items()},
                  mapping='Original UV0 attached to arena; both samples clamp; pilot fades by vertexR and UV-domain boundaries',
                  normal='GL maps imported with green flip; restrained regional blend; optional measured detail RNM with 24-degree slope cap and steep-face attenuation',
                  context_uv_min=uv_min, context_uv_max=uv_max,
                  context_runtime_uv_min=runtime_uv_min, context_runtime_uv_max=runtime_uv_max,
                  detail_normal=detail.get_path_name() if detail is not None else None,
                  detail_source=detail_manifest,
                  geometry_modified=False,
                  runtime_opt_in='DCSceneReview DCCliffPreview DCCliffPreviewScan DCCliffPreviewSupports')
    for filename in ['CliffGeologyImport.json', 'OwnedCliffAtlasImport.json']:
        (saved / filename).write_text(json.dumps(report, indent=2))
    unreal.log_warning('DC_CLIFF_GEOLOGY_IMPORTED ' + str(saved / 'CliffGeologyImport.json'))


if __name__ == '__main__':
    main()
