"""Import owned raw cliff FBX/PBR maps into an ignored local Nanite folder.

Unreal Python commandlet (no world/map edits):
  -run=pythonscript -script=".../import_owned_cliff_scans.py --source <folder>
    --destination /Game/LocalScans/vefkehbva --normal-format dx" -nullrhi
Or set DC_OWNED_CLIFF_SOURCE, DC_OWNED_CLIFF_DESTINATION and
DC_OWNED_CLIFF_NORMAL_FORMAT. Explicit --fbx/--albedo/--normal/--roughness
paths override discovery. No source asset or texture pixels are changed.

Only this script belongs in Git. Sources stay in the owned Fab cache/Saved;
Content/LocalScans/ MUST be ignored before running. Imported names include
source hashes; the report goes to Saved/OwnedCliffImports/<id>.json.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess

import unreal


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as handle:
        for data in iter(lambda: handle.read(4 * 1024 * 1024), b''):
            digest.update(data)
    return digest.hexdigest()


def clean(name):
    return re.sub(r'[^A-Za-z0-9_]', '_', name)


def discover_meshes(folder):
    files = sorted(p for p in folder.rglob('*') if p.is_file() and p.suffix.lower() == '.fbx')
    families = {}
    for path in files:
        stem = path.stem
        lod = re.search(r'(?i)(?:^|[_ -])lod[_ -]?(\d+)(?:$|[_ -])', stem)
        high = re.search(r'(?i)(?:^|[_ -])(?:high|highpoly|raw)(?:$|[_ -])', stem)
        family = re.sub(r'(?i)(?:[_ -]lod[_ -]?\d+|[_ -](?:high|highpoly|raw))(?=$|[_ -])', '', stem)
        rank = int(lod.group(1)) if lod else (-1 if high else 0)
        families.setdefault(family.lower(), []).append((rank, path))
    return [min(group, key=lambda x: (x[0], -x[1].stat().st_size, str(x[1])))[1]
            for group in families.values()]


def discover_texture(folder, role):
    patterns = {
        'albedo': r'(?:^|[_ -])(albedo|basecolor|base_color|diffuse|diff)(?:$|[_ -])',
        'normal': r'(?:^|[_ -])(normal|normalmap|nor_dx|nor_gl)(?:$|[_ -])',
        'roughness': r'(?:^|[_ -])(roughness|rough)(?:$|[_ -])',
    }
    files = [p for p in folder.rglob('*') if p.is_file()
             and p.suffix.lower() in ('.jpg', '.jpeg', '.png', '.tga', '.tif', '.tiff', '.exr')
             and re.search(patterns[role], p.stem, re.I)]
    def rank(path):
        res = re.search(r'(?:^|[_ -])(\d+)k(?:$|[_ -])', path.stem, re.I)
        lod = re.search(r'lod[_ -]?(\d+)', path.stem, re.I)
        return (int(lod.group(1)) if lod else 0, -(int(res.group(1)) if res else 0), -path.stat().st_size, str(path))
    if not files:
        raise RuntimeError('Missing ' + role + ' map; provide an explicit override')
    return min(files, key=rank)


def import_task(source, destination, name, options=None):
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
    task.replace_existing = False
    task.save = False
    if options is not None:
        task.factory = unreal.FbxFactory()
        task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [unreal.load_asset(path) for path in task.imported_object_paths]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=os.environ.get('DC_OWNED_CLIFF_SOURCE'))
    parser.add_argument('--destination', default=os.environ.get('DC_OWNED_CLIFF_DESTINATION'))
    parser.add_argument('--normal-format', choices=('dx', 'gl'), default=os.environ.get('DC_OWNED_CLIFF_NORMAL_FORMAT'))
    parser.add_argument('--fbx', type=Path, action='append')
    for role in ('albedo', 'normal', 'roughness'):
        parser.add_argument('--'+role, type=Path)
    args = parser.parse_args()
    if args.source is None or not args.source.is_dir():
        raise RuntimeError('Provide --source extracted raw asset folder')
    if not args.destination or not re.fullmatch(r'/Game/LocalScans/[A-Za-z0-9_]+', args.destination):
        raise RuntimeError('Destination must be /Game/LocalScans/<asset_id>')
    if args.normal_format is None:
        raise RuntimeError('Specify --normal-format dx or gl from the source metadata; Normal alone is ambiguous')
    source = args.source.resolve()
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())).resolve()
    asset_id = args.destination.rsplit('/', 1)[1]
    sentinel = project/'Content/LocalScans'/asset_id/'owned_asset.uasset'
    ignored = subprocess.run(['git', 'check-ignore', '--quiet', '--', str(sentinel)], cwd=project)
    if ignored.returncode != 0:
        raise RuntimeError('Ignore Content/LocalScans/ before importing owned assets into this public repository')
    try:
        source.relative_to(project)
    except ValueError:
        pass
    else:
        ignored_source = subprocess.run(['git', 'check-ignore', '--quiet', '--', str(source/'owned_source.fbx')], cwd=project)
        if ignored_source.returncode != 0:
            raise RuntimeError('Source folder must also be ignored or outside the project repository')
    meshes = [p.resolve() for p in args.fbx] if args.fbx else discover_meshes(source)
    if not meshes:
        raise RuntimeError('No FBX found in '+str(source))
    maps = {role: (getattr(args, role).resolve() if getattr(args, role) else discover_texture(source, role))
            for role in ('albedo', 'normal', 'roughness')}
    for path in meshes + list(maps.values()):
        if not path.is_file():
            raise RuntimeError('Missing source '+str(path))
    # Scope remains process-local; this preserves the verified native FBX options.
    unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
    if unreal.SystemLibrary.get_console_variable_int_value('Interchange.FeatureFlags.Import.FBX') != 0:
        raise RuntimeError('Could not select native FBX import options')
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    textures, map_report = {}, {}
    for role, path in maps.items():
        digest = sha(path)
        name = 'T_'+asset_id+'_'+role+'_'+digest[:10]+('_'+args.normal_format if role == 'normal' else '')
        target = args.destination+'/Textures/'+name
        texture = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
        if texture is None:
            imported = import_task(path, args.destination+'/Textures', name)
            texture = next((a for a in imported if isinstance(a, unreal.Texture2D)), None)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError('Texture import failed: '+str(path))
        texture.set_editor_property('srgb', role == 'albedo')
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP if role == 'normal'
                                    else unreal.TextureCompressionSettings.TC_DEFAULT)
        if role == 'normal':
            texture.set_editor_property('flip_green_channel', args.normal_format == 'gl')
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError('Could not save '+target)
        textures[role] = texture
        map_report[role] = dict(source=str(path), sha256=digest, asset=texture.get_path_name(), srgb=role == 'albedo')
    material_hash = hashlib.sha256(json.dumps(map_report, sort_keys=True).encode()).hexdigest()[:10]
    material_name = 'M_'+asset_id+'_'+material_hash
    material_path = args.destination+'/'+material_name
    material = unreal.load_asset(material_path) if unreal.EditorAssetLibrary.does_asset_exist(material_path) else None
    if material is None:
        material = tools.create_asset(material_name, args.destination, unreal.Material, unreal.MaterialFactoryNew())
        material.set_editor_property('used_with_nanite', True)
        material.set_editor_property('used_with_instanced_static_meshes', True)
        material.set_editor_property('tangent_space_normal', True)
        material.set_editor_property('two_sided', False)
        for index, (role, prop) in enumerate((('albedo', unreal.MaterialProperty.MP_BASE_COLOR),
                                            ('normal', unreal.MaterialProperty.MP_NORMAL),
                                            ('roughness', unreal.MaterialProperty.MP_ROUGHNESS))):
            node = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, index*180)
            node.texture = textures[role]
            node.sampler_type = (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                                 (unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if role == 'roughness' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR))
            # Unconnected texture coordinates preserve source UV0. Roughness is R, never metallic.
            if not mel.connect_material_property(node, 'R' if role == 'roughness' else '', prop):
                raise RuntimeError('Could not connect '+role)
        metallic = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 600)
        metallic.r = 0.0
        if not mel.connect_material_property(metallic, '', unreal.MaterialProperty.MP_METALLIC):
            raise RuntimeError('Could not connect zero metallic')
        mel.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
            raise RuntimeError('Could not save material')
    records = []
    for path in meshes:
        digest = sha(path)
        name = 'SM_'+clean(path.stem)+'_'+digest[:10]
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
            data.convert_scene = True
            data.convert_scene_unit = True
            data.import_uniform_scale = 1.0
            data.generate_lightmap_u_vs = False
            data.auto_generate_collision = False
            data.build_nanite = True
            data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
            imported = import_task(path, args.destination, name, options)
            found = [a for a in imported if isinstance(a, unreal.StaticMesh)]
            if len(found) != 1:
                raise RuntimeError('Expected one combined static mesh: '+str(path))
            mesh = found[0]
        if not isinstance(mesh, unreal.StaticMesh) or not mesh.get_editor_property('nanite_settings').enabled:
            raise RuntimeError('Expected Nanite StaticMesh: '+target)
        slots = len(mesh.get_editor_property('static_materials'))
        for index in range(max(1, slots)):
            mesh.set_material(index, material)
        box = mesh.get_bounding_box()
        minimum = [float(box.min.x), float(box.min.y), float(box.min.z)]
        maximum = [float(box.max.x), float(box.max.y), float(box.max.z)]
        dimensions = [(b-a)*.01 for a,b in zip(minimum, maximum)]
        if not all(math.isfinite(v) and v > 0 for v in dimensions):
            raise RuntimeError('Invalid imported dimensions '+str(dimensions))
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
            raise RuntimeError('Could not save '+target)
        records.append(dict(source_fbx=str(path), source_sha256=digest, asset=mesh.get_path_name(),
                            bounds_min_cm=minimum, bounds_max_cm=maximum, size_metres=dimensions,
                            center_metres=[(a+b)*.005 for a,b in zip(minimum,maximum)], nanite_enabled=True,
                            render_lod0_triangles=int(mesh.get_num_triangles(0)), material_slots=slots))
    report = dict(schema='directorscut.owned_cliff_import.v1', source_folder=str(source), destination=args.destination,
                  normal_source_convention=args.normal_format, normal_green_flipped=args.normal_format == 'gl',
                  material=material.get_path_name(), textures=map_report, meshes=records,
                  import_scale=1.0, convert_fbx_scene_units=True, generate_lightmap_uvs=False,
                  uv_mapping='Original FBX UV0; no tiling, remapping or material world projection',
                  license='User-owned Fab source; source and imported vendor assets must remain outside public Git',
                  runtime_integrated=False)
    report_path = saved/'OwnedCliffImports'/(asset_id+'.json')
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    unreal.log_warning('DC_OWNED_CLIFF_IMPORT '+json.dumps(report))
    unreal.log_warning('DC_OWNED_CLIFF_REPORT '+str(report_path))


if __name__ == '__main__':
    main()
