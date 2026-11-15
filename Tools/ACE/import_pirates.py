"""Run with UE PythonScript commandlet after Fab finishes the .mhpkg download.

Vendor assets stay in an ignored folder; this script records their actual paths
for the four-character assembly pass rather than guessing package names.
"""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
PACKAGE = Path('C:/ProgramData/Epic/EpicGamesLauncher/VaultCache/FabLibrary/MHC__Pirates___Metahuman_Mega_Bundle-cd3450e5/metahuman/UE_5.8/mhc_piratebundle.mhpkg')
DEST = '/Game/PirateCastVendor'
OUT = ROOT / 'Saved/PirateCast'
OUT.mkdir(parents=True, exist_ok=True)

task = unreal.AssetImportTask()
task.filename = str(PACKAGE)
task.destination_path = DEST
task.automated = True
task.replace_existing = False
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous([DEST], force_rescan=True)
assets = registry.get_assets_by_path(DEST, recursive=True)
inventory = [{'path': str(a.package_name), 'class': str(a.asset_class_path.asset_name)} for a in assets]
(OUT / 'vendor-assets.json').write_text(json.dumps(inventory, indent=2), encoding='utf-8')
characters = [a for a in inventory if a['class'] == 'MetaHumanCharacter']
unreal.log('PIRATE_IMPORT: {} assets, {} characters'.format(len(inventory), len(characters)))
if not characters:
    raise RuntimeError('No MetaHuman character presets imported; inspect the import log.')
unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=True, recursive=True)
