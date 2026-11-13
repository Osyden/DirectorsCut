"""Lists the water plugin's material parameter collection and the ocean materials' parameters. Run: Tools/py.sh Tools/inspect_drw.py 80"""
import unreal
mpc = unreal.load_asset("/DynamicRealWater/Materials/MaterialParameterCollections/MPC_DRW")
if mpc:
    for p in mpc.get_editor_property("scalar_parameters"):
        unreal.log_warning("DC_RESULT mpc scalar %s = %s" % (p.get_editor_property("parameter_name"), p.get_editor_property("default_value")))
    for p in mpc.get_editor_property("vector_parameters"):
        unreal.log_warning("DC_RESULT mpc vector %s = %s" % (p.get_editor_property("parameter_name"), p.get_editor_property("default_value")))
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for a in reg.get_assets_by_path("/DynamicRealWater/Materials/OceanLOD", recursive=True):
    m = a.get_asset()
    try:
        sc = [str(x) for x in unreal.MaterialEditingLibrary.get_scalar_parameter_names(m)]
        ve = [str(x) for x in unreal.MaterialEditingLibrary.get_vector_parameter_names(m)]
    except Exception as e:
        sc, ve = ["ERR %s" % e], []
    unreal.log_warning("DC_RESULT %s (%s) scalars=%s vectors=%s" % (a.asset_name, a.asset_class_path.asset_name, sc, ve))
