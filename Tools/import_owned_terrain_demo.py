"""Import the complete owned Desert Cliffs GLTF-derived FBX and original PBR.

Unreal Python only, no map edits. --gltf <scene.gltf> --fbx <whole_scene.fbx>
--destination /Game/LocalScans/desert_terrain. Environment equivalents:
DC_OWNED_TERRAIN_GLTF, DC_OWNED_TERRAIN_FBX, DC_OWNED_TERRAIN_DESTINATION.
Keep vendor inputs/Content/LocalScans ignored. No pixels or source geometry
are changed. The GLTF material's UV indices, channels and factors are retained.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_owned_cliff_scans import clean, import_task, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gltf', type=Path, default=os.environ.get('DC_OWNED_TERRAIN_GLTF'))
    parser.add_argument('--fbx', type=Path, default=os.environ.get('DC_OWNED_TERRAIN_FBX'))
    parser.add_argument('--destination', default=os.environ.get('DC_OWNED_TERRAIN_DESTINATION', '/Game/LocalScans/desert_terrain'))
    args = parser.parse_args()
    if not args.gltf or not args.gltf.is_file() or not args.fbx or not args.fbx.is_file():
        raise RuntimeError('Provide existing --gltf and complete-scene --fbx')
    if not re.fullmatch(r'/Game/LocalScans/[A-Za-z0-9_]+', args.destination):
        raise RuntimeError('Owned assets must remain in /Game/LocalScans/<id>')
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())).resolve()
    asset_id = args.destination.rsplit('/', 1)[1]
    sentinel = project/'Content/LocalScans'/asset_id/'owned_asset.uasset'
    if subprocess.run(['git', 'check-ignore', '--quiet', '--', str(sentinel)], cwd=project).returncode:
        raise RuntimeError('Content/LocalScans must be ignored before import')
    source = json.loads(args.gltf.read_text())
    if source.get('asset', {}).get('version') != '2.0' or len(source.get('materials', [])) != 1:
        raise RuntimeError('This narrow importer expects GLTF2 with one source material')
    material_source = source['materials'][0]
    pbr = material_source.get('pbrMetallicRoughness', {})
    if material_source.get('alphaMode', 'OPAQUE') != 'OPAQUE':
        raise RuntimeError('This terrain importer only supports the source opaque material')
    if material_source.get('normalTexture', {}).get('scale', 1.0) != 1.0:
        raise RuntimeError('Non-default normal scale needs explicit handling')
    source_maps = {'base_color': pbr.get('baseColorTexture'),
                   'metallic_roughness': pbr.get('metallicRoughnessTexture'),
                   'normal': material_source.get('normalTexture'),
                   'occlusion': material_source.get('occlusionTexture')}
    for role, info in source_maps.items():
        if not info or info.get('extensions'):
            raise RuntimeError('Missing map or unsupported texture transform: '+role)
    unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
    if unreal.SystemLibrary.get_console_variable_int_value('Interchange.FeatureFlags.Import.FBX') != 0:
        raise RuntimeError('Native FBX importer was not selected')
    textures, maps, imported_by_image = {}, {}, {}
    for role, info in source_maps.items():
        image_index = source['textures'][info['index']]['source']
        uri = source['images'][image_index]['uri']
        path = (args.gltf.parent/uri).resolve()
        if not path.is_file():
            raise RuntimeError('Missing original source image '+str(path))
        digest = sha(path)
        normal = role == 'normal'
        srgb = role == 'base_color'
        key = (image_index, normal, srgb)
        if key in imported_by_image:
            texture = imported_by_image[key]
        else:
            name = 'T_'+asset_id+'_'+role+'_'+digest[:10]+('_gl' if normal else '')
            target = args.destination+'/Textures/'+name
            texture = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
            if texture is None:
                texture = next((a for a in import_task(path, args.destination+'/Textures', name)
                                if isinstance(a, unreal.Texture2D)), None)
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError('Texture import failed: '+str(path))
            texture.set_editor_property('srgb', srgb)
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP if normal else unreal.TextureCompressionSettings.TC_DEFAULT)
            if normal:
                texture.set_editor_property('flip_green_channel', True)
            if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
                raise RuntimeError('Texture save failed: '+target)
            imported_by_image[key] = texture
        textures[role] = texture
        maps[role] = dict(source=str(path), sha256=digest, asset=texture.get_path_name(), uv=int(info.get('texCoord', 0)), srgb=srgb)
    contract = dict(material_source=material_source, maps=maps, importer_version=1)
    material_hash = hashlib.sha256(json.dumps(contract, sort_keys=True).encode()).hexdigest()[:10]
    name = 'M_'+asset_id+'_'+material_hash
    target = args.destination+'/'+name
    material = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
    mel = unreal.MaterialEditingLibrary
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, args.destination, unreal.Material, unreal.MaterialFactoryNew())
        material.set_editor_property('used_with_nanite', True)
        material.set_editor_property('tangent_space_normal', True)
        material.set_editor_property('two_sided', bool(material_source.get('doubleSided', False)))

        def node(kind):
            return mel.create_material_expression(material, kind)

        def link(a, output, b, input_name):
            if not mel.connect_material_expressions(a, output, b, input_name):
                raise RuntimeError('Material expression connection failed: '+input_name)

        def prop(a, output, property_name):
            if not mel.connect_material_property(a, output, property_name):
                raise RuntimeError('Material property connection failed: '+str(property_name))

        def constant(value):
            n = node(unreal.MaterialExpressionConstant)
            n.r = float(value)
            return n

        def sample(role):
            n = node(unreal.MaterialExpressionTextureSample)
            n.texture = textures[role]
            n.sampler_type = (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                              unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'base_color' else
                              unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
            uv = node(unreal.MaterialExpressionTextureCoordinate)
            uv.coordinate_index = maps[role]['uv']
            link(uv, '', n, 'Coordinates')
            return n

        def scaled(a, output, value):
            n = node(unreal.MaterialExpressionMultiply)
            link(a, output, n, 'A')
            link(constant(value), '', n, 'B')
            return n

        base = sample('base_color')
        factor = pbr.get('baseColorFactor', [1., 1., 1., 1.])
        color = node(unreal.MaterialExpressionConstant3Vector)
        color.constant = unreal.LinearColor(float(factor[0]), float(factor[1]), float(factor[2]), 1.)
        multiply = node(unreal.MaterialExpressionMultiply)
        link(base, '', multiply, 'A')
        link(color, '', multiply, 'B')
        prop(multiply, '', unreal.MaterialProperty.MP_BASE_COLOR)
        prop(sample('normal'), '', unreal.MaterialProperty.MP_NORMAL)
        packed = sample('metallic_roughness')
        prop(scaled(packed, 'G', pbr.get('roughnessFactor', 1.)), '', unreal.MaterialProperty.MP_ROUGHNESS)
        prop(scaled(packed, 'B', pbr.get('metallicFactor', 1.)), '', unreal.MaterialProperty.MP_METALLIC)
        strength = float(material_source['occlusionTexture'].get('strength', 1.))
        ao = scaled(sample('occlusion'), 'R', strength)
        add = node(unreal.MaterialExpressionAdd)
        link(ao, '', add, 'A')
        link(constant(1.-strength), '', add, 'B')
        prop(add, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        spec = material_source.get('extensions', {}).get('KHR_materials_specular', {})
        if spec.get('specularColorFactor', [1.,1.,1.]) != [1.,1.,1.]:
            raise RuntimeError('Colored dielectric specular needs a different UE shading model')
        # GLTF dielectric F0=.04*factor; UE nonmetal F0=.08*Specular.
        prop(constant(.5*float(spec.get('specularFactor', 1.))), '', unreal.MaterialProperty.MP_SPECULAR)
        mel.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
            raise RuntimeError('Material save failed')
    digest = sha(args.fbx)
    name = 'SM_'+clean(args.fbx.stem)+'_'+digest[:10]
    target = args.destination+'/'+name
    mesh = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
    if mesh is None:
        options = unreal.FbxImportUI()
        options.automated_import_should_detect_type = False
        options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        options.import_mesh = True
        options.import_as_skeletal = False
        options.import_materials = False
        options.import_textures = False
        data = options.static_mesh_import_data
        data.combine_meshes = True
        data.transform_vertex_to_absolute = True
        data.convert_scene = True
        data.convert_scene_unit = True
        data.import_uniform_scale = 1.
        data.generate_lightmap_u_vs = False
        data.auto_generate_collision = False
        data.build_nanite = True
        data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
        found = [a for a in import_task(args.fbx, args.destination, name, options) if isinstance(a, unreal.StaticMesh)]
        if len(found) != 1:
            raise RuntimeError('Expected one whole-scene combined StaticMesh')
        mesh = found[0]
    if not mesh.get_editor_property('nanite_settings').enabled:
        raise RuntimeError('Terrain Nanite build is not enabled')
    for slot in range(max(1, len(mesh.get_editor_property('static_materials')))):
        mesh.set_material(slot, material)
    box = mesh.get_bounding_box()
    minimum = [float(box.min.x), float(box.min.y), float(box.min.z)]
    maximum = [float(box.max.x), float(box.max.y), float(box.max.z)]
    dimensions = [(b-a)*.01 for a,b in zip(minimum,maximum)]
    if not all(math.isfinite(x) and x>0 for x in dimensions):
        raise RuntimeError('Invalid terrain native bounds')
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
        raise RuntimeError('Terrain save failed')
    record = dict(asset=mesh.get_path_name(),source_fbx=str(args.fbx.resolve()),source_sha256=digest,
                  bounds_min_cm=minimum,bounds_max_cm=maximum,size_metres=dimensions,
                  center_metres=[(a+b)*.005 for a,b in zip(minimum,maximum)],nanite_enabled=True,
                  render_lod0_triangles=int(mesh.get_num_triangles(0)))
    report = dict(schema='directorscut.owned_terrain_import.v1',source_gltf=str(args.gltf.resolve()),
                  source_gltf_sha256=sha(args.gltf),destination=args.destination,material=material.get_path_name(),
                  textures=maps,original_material=material_source,meshes=[record],import_scale=1.,
                  source_scene_transforms_applied=True,whole_scene_combined=True,normal_green_flipped=True,
                  uv_mapping='Original UV0 baseColor/packed; original UV1 normal. Export audit confirmed UV0==UV1 for this source.',
                  roughness='Original packed G * roughnessFactor; no invented map',metallic='Original packed B * metallicFactor',
                  license='User-owned Fab source; no vendor assets in public Git',runtime_integrated=False)
    path = saved/'OwnedCliffImports'/(asset_id+'.json')
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(report,indent=2)+'\n')
    unreal.log_warning('DC_OWNED_TERRAIN_REPORT '+str(path))
    unreal.log_warning('DC_OWNED_TERRAIN_IMPORT '+json.dumps(report))


if __name__ == '__main__':
    main()
