"""Builds /Game/DC/Materials/MPC_Sea: numbers the game hands to the sea's materials every frame.

Run with the editor closed:  Tools/py.sh Tools/make_sea_collection.py
  Trough   x = where the middle of the channel lies across the world (cm, world Y),
           y = how far the sea rises at the edge of the channel (cm; 0 = a flat sea),
           z = half the width of the channel (cm)
Read by the water plugin's ocean materials once Tools/patch_drw_shorefoam.py has been run.
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "MPC_Sea"
path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mpc = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
p = unreal.CollectionVectorParameter()
p.set_editor_property("parameter_name", "Trough")
p.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 5000.0, 0.0))
mpc.set_editor_property("vector_parameters", [p])
unreal.log_warning("DC_RESULT MPC_Sea saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
