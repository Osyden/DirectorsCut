"""Create role presets and request the required Epic MetaHuman rig/texture data.

Run in the isolated project. -PirateRoles=Captain,Finch selects which presets
to prepare in this invocation. Original purchased presets are never edited.
"""
import json
import re
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/PirateCast'
OUT.mkdir(parents=True, exist_ok=True)
VENDOR = '/Game/PirateCastVendor'
WARDROBE = VENDOR + '/QuangPhan/Pirates/'
PRESETS = '/Game/PirateCast/Presets'
CAST = [
    ('Captain', 'MHC_BreezaBundle', 'WI_SovereignCorsair', 'WI_FPirateHat', 'WI_PirateHatHair'),
    ('Finch', 'MHC_DavyBundle', 'WI_DreadCaptain', 'WI_MPirateHat', 'WI_MalePirateHatHair'),
    ('Marlow', 'MHC_DavyBundle', 'WI_TideDuelist', 'WI_MPirateHeadBand', None),
    ('Odessa', 'MHC_BreezaBundle', 'WI_RogueSwashbuckler', 'WI_FPirateHeadBand', 'WI_PirateHeadBandHair'),
]
OUTFIT_NAMES = {'WI_SovereignCorsair', 'WI_DreadCaptain', 'WI_TideDuelist', 'WI_RogueSwashbuckler'}
HAT_NAMES = {'WI_FPirateHat', 'WI_MPirateHat', 'WI_FPirateHeadBand', 'WI_MPirateHeadBand'}
subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)

def reference_name(key):
    ref = key.get_editor_property('principal_asset_or_external_wardrobe_item')
    asset = ref.get_editor_property('asset')
    return asset.get_name() if asset else ''

report = []
for name, source, outfit_name, hat_name, hair_name in CAST:
    path = PRESETS + '/' + name
    character = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.EditorAssetLibrary.duplicate_asset(VENDOR + '/' + source, path)
    if character is None:
        raise RuntimeError('Could not create ' + path)
    collection = character.internal_collection
    instance = collection.default_instance
    previous = []
    keep = []
    for data in instance.get_slot_selection_data():
        selection = data.selection
        item_name = reference_name(selection.selected_item)
        previous.append({'slot': str(selection.slot_name), 'item': item_name})
        if str(selection.slot_name) == 'Outfits' and item_name not in OUTFIT_NAMES | HAT_NAMES:
            keep.append(selection.selected_item)
    outfit = unreal.load_asset(WARDROBE + outfit_name)
    hat = unreal.load_asset(WARDROBE + hat_name)
    if outfit is None or hat is None:
        raise RuntimeError('Missing wardrobe for ' + name)
    outfit_key = collection.try_add_item_from_wardrobe_item('Outfits', outfit)
    instance.set_single_slot_selection('Outfits', outfit_key)
    for key in keep + [collection.try_add_item_from_wardrobe_item('Outfits', hat)]:
        instance.try_add_slot_selection(unreal.MetaHumanPipelineSlotSelection(slot_name='Outfits', selected_item=key))
    if hair_name:
        hair = unreal.load_asset(WARDROBE + hair_name)
        instance.set_single_slot_selection('Hair', collection.try_add_item_from_wardrobe_item('Hair', hair))
    unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
    report.append({'role': name, 'preset': path, 'outfit': outfit_name, 'hat': hat_name, 'hair': hair_name, 'previous': previous})
    unreal.log('PIRATE_PRESET: ' + name + ' ' + outfit_name)
(OUT / 'cast-presets.json').write_text(json.dumps(report, indent=2), encoding='utf-8')

match = re.search(r'-PirateRoles=([A-Za-z,]+)', unreal.SystemLibrary.get_command_line())
roles = match.group(1).split(',') if match else ['Captain']
for role in roles:
    character = unreal.load_asset(PRESETS + '/' + role)
    if not subsystem.try_add_object_to_edit(character):
        raise RuntimeError('Cannot open preset for editing: ' + role)
    try:
        if not subsystem.can_build_meta_human(character):
            unreal.log('PIRATE_RIG_REQUEST: ' + role)
            request = unreal.MetaHumanCharacterAutoRiggingRequestParams()
            request.blocking = True
            request.report_progress = False
            request.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
            subsystem.request_auto_rigging(character, request)
            unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
            if not character.has_high_resolution_textures:
                unreal.log('PIRATE_TEXTURE_REQUEST: ' + role)
                texture_request = unreal.MetaHumanCharacterTextureRequestParams()
                texture_request.blocking = True
                texture_request.report_progress = False
                subsystem.request_texture_sources(character, texture_request)
                unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
        ready = subsystem.can_build_meta_human(character, True)
        unreal.log('PIRATE_READY: {} {}'.format(role, ready))
        (OUT / (role + '-ready.json')).write_text(json.dumps({'role': role, 'ready': ready, 'high_resolution_textures': bool(character.has_high_resolution_textures)}, indent=2), encoding='utf-8')
        if not ready:
            raise RuntimeError('MetaHuman service preparation incomplete for ' + role + '; see log.')
    finally:
        if subsystem.is_object_added_for_editing(character):
            subsystem.remove_object_to_edit(character)
