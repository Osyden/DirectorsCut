"""Inventory only the shipped ARKit mapping's hard /Game dependencies."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/MetaHumans'], force_rescan=True)
options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
pending = ['/Game/MetaHumans/Common/Common/Mocap/mh_arkit_mapping_pose', '/Game/MetaHumans/Common/Common/Mocap/mh_arkit_mapping_anim']
seen = set()
while pending:
    package = pending.pop()
    if package in seen or not package.startswith('/Game/'):
        continue
    seen.add(package)
    if not (root / 'Content' / (package.removeprefix('/Game/') + '.uasset')).is_file():
        raise RuntimeError('Missing face mapping dependency: ' + package)
    pending.extend(str(p) for p in registry.get_dependencies(package, options))
out = root / 'Saved/ACE/face-dependencies.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(sorted(seen), indent=2), encoding='utf-8')
unreal.log('ACE_FACE_DEPENDENCIES: {} packages'.format(len(seen)))
