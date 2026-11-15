"""Load each assembled actor and inventory its render components and hard dependencies."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
out = root / 'Saved/PirateCast'
subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True, include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
report = []
for role in ['Captain', 'Finch', 'Marlow', 'Odessa']:
    path = '/Game/PirateCast/Assembled/{0}/BP_{0}'.format(role)
    actor_class = unreal.EditorAssetLibrary.load_blueprint_class(path)
    if not actor_class:
        raise RuntimeError('Missing assembled class: ' + role)
    actor = subsystem.spawn_actor_from_class(actor_class, unreal.Vector())
    components = []
    for component in actor.get_components_by_class(unreal.PrimitiveComponent):
        row = {'name': component.get_name(), 'class': component.get_class().get_name()}
        if isinstance(component, unreal.SkeletalMeshComponent):
            mesh = component.get_skeletal_mesh_asset()
            row['mesh'] = mesh.get_path_name() if mesh else None
            row['bones'] = component.get_num_bones()
            row['materials'] = [m.get_path_name() if m else None for m in component.get_materials()]
        if isinstance(component, unreal.GroomComponent) and component.get_name() == 'Hair':
            hair = component.get_editor_property('groom_asset')
            binding = component.get_editor_property('binding_asset')
            row['hair_asset'] = hair.get_path_name() if hair else None
            row['binding_asset'] = binding.get_path_name() if binding else None
            row['curve_counts'] = [g.get_editor_property('num_curves') for g in hair.get_editor_property('hair_groups_info')] if hair else []
        components.append(row)
    meshes = {c['name']: c for c in components if 'mesh' in c}
    if not meshes.get('Body', {}).get('mesh') or not meshes.get('Face', {}).get('mesh'):
        raise RuntimeError('Incomplete body/face components for ' + role)
    hair_base = '/Game/PirateCast/Assembled/' + role + '/Grooms/'
    hair_paths = [hair_base + 'Gr_Hair_Strands', hair_base + 'GB_Hair_Strands']
    for hair_path in hair_paths:
        if not unreal.EditorAssetLibrary.does_asset_exist(hair_path):
            raise RuntimeError('Missing restored strand asset: ' + hair_path)
    restored_hair = unreal.load_asset(hair_paths[0])
    restored_counts = [g.get_editor_property('num_curves') for g in restored_hair.get_editor_property('hair_groups_info')]
    if not restored_counts or min(restored_counts) < 100:
        raise RuntimeError('Runtime hairstyle has no strand geometry: ' + role)
    pending, seen, missing, outside, engine_dependencies = [path] + hair_paths, set(), [], [], set()
    while pending:
        package = pending.pop()
        if package in seen or not package.startswith('/Game/'):
            continue
        seen.add(package)
        disk = root / 'Content' / (package.removeprefix('/Game/') + '.uasset')
        if not disk.exists():
            missing.append(package)
        if not package.startswith(('/Game/PirateCast/', '/Game/PirateCastVendor/')):
            outside.append(package)
        dependencies = [str(p) for p in registry.get_dependencies(package, options)]
        engine_dependencies.update(p for p in dependencies if not p.startswith('/Game/'))
        pending.extend(dependencies)
    report.append({'role': role, 'blueprint': path, 'components': components, 'runtime_hair_curve_counts': restored_counts, 'hard_dependency_count': len(seen), 'missing_packages': missing, 'external_game_packages': outside, 'game_packages': sorted(seen), 'engine_dependencies': sorted(engine_dependencies)})
    subsystem.destroy_actor(actor)
(out / 'validated-cast.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
if any(r['missing_packages'] for r in report):
    raise RuntimeError('Assembled cast has missing hard dependencies; inspect validated-cast.json')
unreal.log('PIRATE_VALIDATED: four complete actor classes with body, face and saved hard dependencies')
