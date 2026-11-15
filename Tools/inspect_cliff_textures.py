"""Export installed stone textures to ignored Saved/ for local visual inspection.

This reads assets; it does not modify them or copy licensed pixels into Git.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> -nullrhi.
"""
import json
import os
from pathlib import Path
import unreal

selection = os.environ.get('DC_CLIFF_TEXTURE_SET', 'ruins')
if selection not in ('ruins', 'paragon_rock'):
    raise RuntimeError('Choose ruins or paragon_rock for DC_CLIFF_TEXTURE_SET')
destination = Path(unreal.Paths.project_saved_dir()) / ('CliffTextureReview' if selection == 'ruins' else 'CliffTextureReviewParagon')
destination.mkdir(parents=True, exist_ok=True)
sources = [f'/Game/AncientRuins/Textures/Bake/Cliffs/T_Cliff_{i:02d}_{channel}'
           for i in range(1, 4) for channel in ('D', 'N')]
sources += [f'/Game/AncientRuins/Textures/Tile/T_Ground_02_{channel}' for channel in ('D', 'N')]
if selection == 'paragon_rock':
    root = '/Game/ParagonRampage/Characters/Global/MaterialLayers/Rock'
    sources = [f'{root}/T_{name}TextureTile_000_{channel}'
               for name in ('Rock', 'Slate') for channel in ('D', 'N')]
    sources += [f'{root}/T_RockTextureTile_000_M']
records = []
for asset_path in sources:
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError('Missing installed texture: ' + asset_path)
    task = unreal.AssetExportTask()
    task.object = asset
    task.filename = str(destination / (asset.get_name() + '.png'))
    task.exporter = unreal.TextureExporterPNG()
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError('Texture export failed: ' + asset_path)
    record = {'asset': asset_path, 'png': task.filename,
              'compression': str(asset.get_editor_property('compression_settings')),
              'srgb': bool(asset.get_editor_property('srgb'))}
    records.append(record)
    unreal.log_warning('DC_CLIFF_TEXTURE_INSPECTION ' + json.dumps(record))
(destination / 'manifest.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
