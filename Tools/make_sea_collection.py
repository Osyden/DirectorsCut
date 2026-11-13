"""Builds /Game/DC/Materials/MPC_Sea: numbers the game hands to the sea's materials every frame.

Run with the editor closed:  Tools/py.sh Tools/make_sea_collection.py
  Trough   x = where the middle of the channel lies across the world (cm, world Y),
           y = how far the sea rises at the edge of the channel (cm; 0 = a flat sea),
           z = half the width of the channel (cm)
  Wake     x = how strong the ship's wake is (0 = none), y = how far astern of the origin it begins (cm),
           z = its half width there (cm), w = how fast it widens (cm per cm astern)
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
w = unreal.CollectionVectorParameter()
w.set_editor_property("parameter_name", "Wake")
w.set_editor_property("default_value", unreal.LinearColor(0.0, 1900.0, 320.0, 0.2))
mpc.set_editor_property("vector_parameters", [p, w])
unreal.log_warning("DC_RESULT MPC_Sea saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
