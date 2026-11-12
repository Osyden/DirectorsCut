import unreal

path = "/Game/DC/Maps/L_Ride"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log("L_Ride already exists")
else:
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, path)
    unreal.log("L_Ride saved: %s" % ok)
