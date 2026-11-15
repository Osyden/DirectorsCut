"""Inventory imported presets, wardrobes and exposed assembly properties."""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved/PirateCast'
out.mkdir(parents=True, exist_ok=True)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/PirateCastVendor'], force_rescan=True)
details = []
for data in registry.get_assets_by_path('/Game/PirateCastVendor', recursive=True):
    cls = str(data.asset_class_path.asset_name)
    if cls not in ('MetaHumanCharacter', 'MetaHumanWardrobeItem', 'Blueprint', 'ChaosClothAsset', 'ChaosOutfitAsset'):
        continue
    row = {'path': str(data.package_name), 'class': cls}
    obj = data.get_asset()
    if cls == 'MetaHumanCharacter':
        row['properties'] = [p for p in dir(obj) if not p.startswith('_')]
        row['has_high_resolution_textures'] = bool(obj.has_high_resolution_textures)
        row['collection'] = str(obj.internal_collection)
        row['instance_properties'] = [p for p in dir(obj.internal_collection.default_instance) if not p.startswith('_')]
        row['selections'] = [str(s) for s in obj.internal_collection.default_instance.get_slot_selection_data()]
        subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
        row['editable'] = subsystem.try_add_object_to_edit(obj)
        if row['editable']:
            row['can_build'] = subsystem.can_build_meta_human(obj, True)
            subsystem.remove_object_to_edit(obj)
    details.append(row)
(out / 'presets.json').write_text(json.dumps(details, indent=2), encoding='utf-8')
unreal.log('PIRATE_INSPECT: {} candidate assets'.format(len(details)))
