"""Import the original generated Atlantean Odyssey title with its alpha intact."""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
for source, name in [('AtlanteanOdyssey-logo-v2.png', 'T_AtlanteanOdysseyLogoV2'),
                     ('AtlanteanOdyssey-menu-panel.png', 'T_AtlanteanMenuPanel')]:
    item = unreal.AssetImportTask()
    item.filename = os.path.join(root, 'Art/UI', source)
    item.destination_path = '/Game/DC/UI'
    item.destination_name = name
    item.automated = True
    item.replace_existing = True
    item.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([item])
    imported = unreal.load_asset('/Game/DC/UI/' + name)
    assert imported, 'Menu art import failed: ' + name
    imported.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    imported.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    imported.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    imported.set_editor_property('srgb', True)
    unreal.EditorAssetLibrary.save_loaded_asset(imported)
    unreal.log_warning('MENU_ART imported ' + name)
