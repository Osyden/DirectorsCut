"""Builds /Game/DC/Materials/MI_VdbTunnel01: M_VdbCloud with the bought tunnel volume set on it.

Run with the editor closed, after Tools/make_vdbcloud_material.py and with the pack installed:
  Tools/py.sh Tools/make_vdb_instances.py
The game sets the volume on its own instances at run time, loading it by path. Nothing in the cooked
content refers to it, so a packaged build would leave it out. This instance lies in /Game/DC, which
is always cooked, and its one reference brings in volume 01 alone (about 1 GB of the pack's 13 GB).
It holds a path, not pack data.
"""
import unreal

mel = unreal.MaterialEditingLibrary
PKG = "/Game/DC/Materials"
NAME = "MI_VdbTunnel01"
path = PKG + "/" + NAME
parent = unreal.load_asset(PKG + "/M_VdbCloud")
vol = unreal.load_asset("/Game/Clouds_Tunnel_Loop/Effects/CT_Clouds_Tunnel_Vdb_01")
if not parent or not vol:
    unreal.log_warning("DC_RESULT missing parent=%s volume=%s" % (parent, vol))
else:
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    f = unreal.MaterialInstanceConstantFactoryNew()
    mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.MaterialInstanceConstant, f)
    mel.set_material_instance_parent(mi, parent)
    ok = False
    try:
        ok = mel.set_material_instance_sparse_volume_texture_parameter_value(mi, "SparseVolumeTexture", vol)
    except Exception as e:
        unreal.log_warning("DC_RESULT no setter: %s" % e)
        v = unreal.SparseVolumeTextureParameterValue()
        info = unreal.MaterialParameterInfo(); info.set_editor_property("name", "SparseVolumeTexture")
        v.set_editor_property("parameter_info", info); v.set_editor_property("parameter_value", vol)
        mi.set_editor_property("sparse_volume_texture_parameter_values", [v]); ok = True
    mel.update_material_instance(mi)
    unreal.log_warning("DC_RESULT MI_VdbTunnel01 set=%s saved=%s refs=%s" % (ok, unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False), [str(p.parameter_value.get_name()) for p in mi.get_editor_property("sparse_volume_texture_parameter_values")]))
