"""Read-only audit of materials on the city temples and foreground rigging.

Run in Unreal's Python commandlet (NullRHI is sufficient). Does not save assets.
All records use DC_EDGE_ASSET for filtering alongside other commandlet audits.
"""
import json
import unreal


def emit(kind, **fields):
    unreal.log_warning("DC_EDGE_ASSET " + json.dumps(dict(kind=kind, **fields), sort_keys=True))


mel = unreal.MaterialEditingLibrary
materials = {}
for path in (
    "/Game/AncientRuins/Meshes/Columns/SM_Colonnade_01",
    "/Game/AncientRuins/Meshes/Columns/SM_Colonnade_02",
    "/Game/AncientRuins/Meshes/Stone_blocks/SM_Fronton_01",
    "/Game/AncientRuins/Meshes/Stone_blocks/SM_Stone_blocks_04_block_01",
    "/Game/AncientRuins/Meshes/Wall_sections/SM_Wall_section_01",
    "/Game/Brigantine/Mesh/Ship/Mast/SK_Mast-brigantine",
    "/Game/Brigantine/Mesh/Ship/Rope/SM_Rope-1",
):
    mesh = unreal.load_asset(path)
    if not mesh:
        emit("missing", path=path)
        continue
    slots = mesh.get_editor_property("materials" if isinstance(mesh, unreal.SkeletalMesh) else "static_materials")
    paths = []
    for slot in slots:
        material = slot.get_editor_property("material_interface")
        if material:
            materials[material.get_path_name()] = material
            paths.append(material.get_path_name())
    emit("mesh", path=path, materials=paths)

for path, material in sorted(materials.items()):
    base = material.get_base_material()
    fields = dict(path=path, base=base.get_path_name() if base else None)
    if isinstance(material, unreal.MaterialInstanceConstant):
        fields["scalars"] = {
            str(name): mel.get_material_instance_scalar_parameter_value(material, name)
            for name in mel.get_scalar_parameter_names(material)
        }
        fields["vectors"] = {
            str(name): str(mel.get_material_instance_vector_parameter_value(material, name))
            for name in mel.get_vector_parameter_names(material)
        }
        fields["switches"] = {
            str(name): mel.get_material_instance_static_switch_parameter_value(material, name)
            for name in mel.get_static_switch_parameter_names(material)
        }
        fields["textures"] = {}
        for name in mel.get_texture_parameter_names(material):
            texture = mel.get_material_instance_texture_parameter_value(material, name)
            fields["textures"][str(name)] = texture.get_path_name() if texture else None
    if base:
        fields["blend_mode"] = str(base.get_editor_property("blend_mode"))
        fields["shading_model"] = str(base.get_editor_property("shading_model"))
        fields["two_sided"] = base.get_editor_property("two_sided")
    emit("material", **fields)

emit("complete", unique_materials=len(materials))
