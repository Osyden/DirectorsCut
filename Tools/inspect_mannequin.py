"""Lists the crew stand-in's material slots and parameters. Run: Tools/py.sh Tools/inspect_mannequin.py 40"""
import unreal
for name in ["SKM_Manny_Simple", "SKM_Quinn_Simple"]:
    m = unreal.load_asset("/Game/Brigantine/Demo/Characters/Mannequins/Meshes/%s" % name)
    if not m:
        unreal.log_warning("DC_RESULT %s missing" % name)
        continue
    for i, sm in enumerate(m.materials):
        mi = sm.material_interface
        sc = [(str(x), unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mi, x)) for x in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mi)] if isinstance(mi, unreal.MaterialInstance) else []
        ve = [(str(x), str(unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value(mi, x))) for x in unreal.MaterialEditingLibrary.get_vector_parameter_names(mi)] if isinstance(mi, unreal.MaterialInstance) else []
        unreal.log_warning("DC_RESULT %s slot %d %s scalars=%s vectors=%s" % (name, i, mi.get_path_name() if mi else None, sc, ve))
