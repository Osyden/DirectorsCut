import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
seen = set()
for a in reg.get_assets_by_path("/Game/Brigantine", recursive=True):
    if str(a.asset_class_path.asset_name) != "StaticMesh" or "amp" not in str(a.asset_name):
        continue
    m = a.get_asset()
    for i, sm in enumerate(m.static_materials):
        mi = sm.material_interface
        if not mi:
            continue
        info = ""
        if mi.get_path_name() not in seen:
            seen.add(mi.get_path_name())
            try:
                sc = [str(x) for x in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mi)]
                ve = [str(x) for x in unreal.MaterialEditingLibrary.get_vector_parameter_names(mi)]
                info = "scalars=%s vectors=%s" % (sc, ve)
            except Exception as e:
                info = "ERR %s" % e
        unreal.log_warning("DC_RESULT %s slot %d %s (%s) %s" % (a.asset_name, i, mi.get_path_name(), mi.get_class().get_name(), info))
