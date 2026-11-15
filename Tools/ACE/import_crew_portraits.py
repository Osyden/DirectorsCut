"""Import actual in-engine portrait captures; do not modify licensed character assets."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
for name in ('Rex', 'Duncan', 'Ruby'):
    source = root / 'Saved/CrewPortraits' / f'T_Crew_{name}.png'
    if not source.is_file():
        raise RuntimeError(f'Missing rendered portrait: {source}')
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = '/Game/DC/UI'
    task.destination_name = f'T_Crew_{name}'
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f'/Game/DC/UI/T_Crew_{name}')
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f'Portrait import failed: {name}')
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('never_stream', True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    unreal.log(f'CREW_PORTRAIT_IMPORTED {name}')
