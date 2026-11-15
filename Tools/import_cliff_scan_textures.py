"""Import verified CC0 scan maps into the isolated cliff preview folder only."""
import hashlib
import json
import os
from pathlib import Path
import unreal


source = Path(os.environ['DC_CLIFF_SCAN_MANIFEST']).resolve()
manifest = json.loads(source.read_text(encoding='utf-8'))
if manifest['schema'] != 'directorscut.cliff_scan.v1':
    raise ValueError('Unsupported scan manifest')
root = '/Game/DC/Maelstrom/GeneratedCliffPreview/Scan'
assets = {}
for role, token in [('D', '_diff_'), ('N', '_nor_gl_'), ('R', '_rough_')]:
    record = next(r for name, r in manifest['files'].items() if token in name)
    path = Path(record['path'])
    if hashlib.sha256(path.read_bytes()).hexdigest() != record['sha256']:
        raise ValueError('Source texture changed: ' + str(path))
    name = 'T_' + manifest['asset'] + '_' + role + '_' + record['sha256'][:10]
    texture = unreal.load_asset(root + '/' + name)
    if texture is None:
        task = unreal.AssetImportTask()
        task.filename = str(path)
        task.destination_path = root
        task.destination_name = name
        task.automated = True
        task.replace_existing = False
        task.save = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.load_asset(root + '/' + name)
    if not isinstance(texture, unreal.Texture2D):
        raise ValueError('Texture import failed for ' + role)
    texture.set_editor_property('srgb', role == 'D')
    texture.set_editor_property('compression_settings',
        unreal.TextureCompressionSettings.TC_NORMALMAP if role == 'N' else unreal.TextureCompressionSettings.TC_DEFAULT)
    if role == 'N':
        texture.set_editor_property('flip_green_channel', True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError('Could not save scan texture')
    assets[role] = texture.get_path_name()
report = dict(source_manifest=str(source), asset=manifest['asset'], texture_references=assets,
              width_metres=manifest['width_metres'], license=manifest['license'],
              normal_convention='Source GL, imported green flipped for DX; shader reconstructs source-chart normal')
saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
(saved / 'CliffScanImport.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
unreal.log_warning('DC_CLIFF_SCAN_IMPORT ' + json.dumps(report))
