import json
from pathlib import Path
import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
rows = []
for role in ['Captain', 'Finch', 'Marlow', 'Odessa']:
    actor = sub.spawn_actor_from_class(unreal.EditorAssetLibrary.load_blueprint_class('/Game/PirateCast/Assembled/{0}/BP_{0}'.format(role)), unreal.Vector())
    row = {'role': role, 'grooms': []}
    for c in actor.get_components_by_class(unreal.GroomComponent):
        asset = c.get_editor_property('groom_asset')
        binding = c.get_editor_property('binding_asset')
        g = {'name': c.get_name(), 'asset': asset.get_path_name() if asset else None, 'binding': binding.get_path_name() if binding else None, 'cards': c.get_editor_property('use_cards'), 'visible': c.is_visible()}
        if asset:
            g['lods'] = [[{p: str(lod.get_editor_property(p)) for p in ['geometry_type', 'visible', 'curve_decimation']} for lod in group.get_editor_property('LODs')] for group in asset.get_editor_property('hair_groups_lod')]
            g['min_lod'] = str(asset.get_editor_property('min_lod'))
            g['cards_data'] = str(asset.get_editor_property('hair_groups_cards'))
        row['grooms'].append(g)
    for c in actor.get_components_by_class(unreal.LODSyncComponent):
        row['lod_sync'] = {p: str(c.get_editor_property(p)) for p in ['forced_lod', 'min_lod', 'num_lods', 'components_to_sync', 'custom_lod_mapping']}
    rows.append(row)
    sub.destroy_actor(actor)
Path(unreal.Paths.project_saved_dir(), 'PirateCast/grooms.json').write_text(json.dumps(rows, indent=2))
