"""Assemble prepared MetaHuman role presets into optimized game assets.

Requires Epic rigging and texture preparation to have succeeded first. This
script deliberately refuses to label an editable preset a playable character.
"""
import json
import re
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved/PirateCast'
out.mkdir(parents=True, exist_ok=True)
match = re.search(r'-PirateRoles=([A-Za-z,]+)', unreal.SystemLibrary.get_command_line())
roles = match.group(1).split(',') if match else ['Captain', 'Finch', 'Marlow', 'Odessa']
subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
results = []
(out / 'assembled-cast.json').write_text('[]', encoding='utf-8')
for role in roles:
    character = unreal.load_asset('/Game/PirateCast/Presets/' + role)
    if not character or not subsystem.try_add_object_to_edit(character):
        raise RuntimeError('Cannot edit ' + role)
    try:
        if not subsystem.can_build_meta_human(character, True):
            raise RuntimeError(role + ' still needs MetaHuman rigging/texture preparation.')
        params = unreal.MetaHumanCharacterEditorBuildParameters()
        params.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
        params.pipeline_quality = unreal.MetaHumanQualityLevel.MEDIUM
        params.absolute_build_path = '/Game/PirateCast/Assembled'
        params.common_folder_path = '/Game/PirateCast/Assembled/Common'
        params.name_override = role
        params.enable_wardrobe_item_validation = True
        try:
            subsystem.build_meta_human(character, params)
        except RuntimeError as error:
            # UE 5.8 reports redundant reroute-link removal during Control Rig
            # migration as a Python exception even after the collection reports
            # successful assembly. Keep the error visible, then require actual
            # baked outputs and a generated actor before saving the result.
            if 'Cannot break link' not in str(error):
                raise
            unreal.log_warning('PIRATE_CONTROL_RIG_MIGRATION: ' + str(error))
        # A failed collection build can leave a partial Blueprint behind. Check
        # baked outputs as well; a Blueprint alone is not proof of completion.
        required_textures = [
            '/Game/PirateCast/Assembled/' + role + '/Face/Baked/T_EyeIrisL_N',
            '/Game/PirateCast/Assembled/' + role + '/T_PreBakedGroom_Color_LOD5to7',
        ]
        missing = [p for p in required_textures if not unreal.EditorAssetLibrary.does_asset_exist(p)]
        if missing:
            raise RuntimeError('Assembly is incomplete; missing baked outputs: ' + repr(missing))
        unreal.EditorAssetLibrary.save_directory('/Game/PirateCast', only_if_is_dirty=True, recursive=True)
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        assets = registry.get_assets_by_path('/Game/PirateCast/Assembled', recursive=True)
        blueprints = [str(a.package_name) for a in assets if str(a.asset_class_path.asset_name) == 'Blueprint' and str(a.asset_name) == 'BP_' + role]
        if len(blueprints) != 1:
            raise RuntimeError('Expected one assembled actor Blueprint for ' + role + ', got ' + repr(blueprints))
        results.append({'role': role, 'blueprint': blueprints[0]})
        (out / 'assembled-cast.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
        unreal.log('PIRATE_ASSEMBLED: ' + role + ' ' + blueprints[0])
    finally:
        if subsystem.is_object_added_for_editing(character):
            subsystem.remove_object_to_edit(character)
