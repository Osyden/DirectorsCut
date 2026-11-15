"""Restore strand-only vendor hair after Medium assembly zeroes its strand decimation.

Body, face and textures keep Medium quality. Only generated hair assets change;
the purchased source remains untouched. Run before final dependency validation.
"""
import json
from pathlib import Path
import unreal

names = {'Captain': 'PirateHatHair', 'Finch': 'MalePirateHatHair', 'Marlow': 'MalePirateHatHair', 'Odessa': 'PirateHeadBandHair'}
report = []
for role, name in names.items():
    base = '/Game/PirateCast/Assembled/' + role + '/Grooms/'
    source_base = '/Game/PirateCastVendor/QuangPhan/Pirates/Grooms/'
    source = unreal.load_asset(source_base + 'Gr_' + name)
    destination = base + 'Gr_Hair_Strands'
    groom = unreal.load_asset(destination) if unreal.EditorAssetLibrary.does_asset_exist(destination) else unreal.EditorAssetLibrary.duplicate_asset(source.get_path_name(), destination)
    groups = groom.get_editor_property('hair_groups_interpolation')
    for group in groups:
        settings = group.get_editor_property('decimation_settings')
        settings.set_editor_property('curve_decimation', 1.0)
        settings.set_editor_property('vertex_decimation', 1.0)
        group.set_editor_property('decimation_settings', settings)
    groom.set_editor_property('hair_groups_interpolation', groups)
    # UE only rebuilds for leaf interpolation properties or the outer LOD array;
    # assigning the outer interpolation array alone leaves the one-strand DDC intact.
    groom.set_editor_property('hair_groups_lod', groom.get_editor_property('hair_groups_lod'), notify_mode=unreal.PropertyAccessChangeNotifyMode.ALWAYS)
    unreal.EditorAssetLibrary.save_loaded_asset(groom, only_if_is_dirty=False)
    source_binding = unreal.load_asset(source_base + 'GB_' + name)
    source_mesh = source_binding.get_editor_property('target_skeletal_mesh')
    face = unreal.load_asset('/Game/PirateCast/Assembled/{0}/Face/SKM_{0}_FaceMesh'.format(role))
    binding_path = base + 'GB_Hair_Strands'
    binding = unreal.load_asset(binding_path) if unreal.EditorAssetLibrary.does_asset_exist(binding_path) else unreal.GroomLibrary.create_new_groom_binding_asset_with_path(binding_path, groom, face, 100, source_mesh, 0)
    if not binding:
        raise RuntimeError('Could not create strand binding for ' + role)
    binding.build(unreal.OnGroomBindingAssetBuildComplete())
    unreal.EditorAssetLibrary.save_loaded_asset(binding, only_if_is_dirty=False)
    curves = [g.get_editor_property('num_curves') for g in groom.get_editor_property('hair_groups_info')]
    if not curves or min(curves) < 100:
        raise RuntimeError('Hair restoration failed for {}: {}'.format(role, curves))
    report.append({'role': role, 'curves': curves, 'binding': binding.get_path_name()})
    unreal.log('PIRATE_STRANDS_RESTORED: {} {}'.format(role, curves))
Path(unreal.Paths.project_saved_dir(), 'PirateCast/restored-strands.json').write_text(json.dumps(report, indent=2))
