"""Lists every material used by the Brigantine pack's meshes with its parameters and parent. Run: Tools/py.sh Tools/inspect_ship_materials.py 80"""
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
seen = {}
for a in reg.get_assets_by_path("/Game/Brigantine", recursive=True):
    if str(a.asset_class_path.asset_name) != "StaticMesh":
        continue
    m = a.get_asset()
    for sm in m.static_materials:
        mi = sm.material_interface
        if mi:
            seen[mi.get_path_name()] = (mi, seen.get(mi.get_path_name(), (None, 0))[1] + 1)
for path, (mi, n) in sorted(seen.items()):
    sc = [str(x) for x in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mi)]
    ve = [str(x) for x in unreal.MaterialEditingLibrary.get_vector_parameter_names(mi)]
    parent = mi.get_base_material().get_path_name() if mi.get_base_material() else ""
    unreal.log_warning("DC_RESULT %s x%d parent=%s scalars=%s vectors=%s" % (path.split(".")[-1], n, parent.split(".")[-1], sc, ve))
