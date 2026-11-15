"""Import the original generated Atlantean Odyssey title with its alpha intact."""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
task = unreal.AssetImportTask()
task.filename = os.path.join(root, 'Art/UI/AtlanteanOdyssey-logo.png')
task.destination_path = '/Game/DC/UI'
task.destination_name = 'T_AtlanteanOdysseyLogo'
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset('/Game/DC/UI/T_AtlanteanOdysseyLogo')
assert texture, 'Logo import failed'
texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('srgb', True)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log_warning('MENU_LOGO imported with UI alpha')
