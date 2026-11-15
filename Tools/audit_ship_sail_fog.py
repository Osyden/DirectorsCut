"""Read-only ship sail/rigging audit; run as a NullRHI Python commandlet.

Constructs the actual BP_Brigantine in the commandlet's temporary editor world,
records component overrides as well as mesh defaults, then destroys the actor.
Does not run voyage BeginPlay/ticks or save any asset/map. Exports graph text and
JSON only under Saved/ShipSailFogAudit. Missing properties are reported explicitly.
"""
from pathlib import Path
import json
import unreal

OUT = Path(unreal.Paths.project_saved_dir()) / "ShipSailFogAudit"
OUT.mkdir(parents=True, exist_ok=True)
records = []
materials = {}
mel = unreal.MaterialEditingLibrary


def path(obj):
    return obj.get_path_name() if obj else None


def emit(kind, **fields):
    record = dict(kind=kind, **fields)
    records.append(record)
    unreal.log_warning("DC_SAIL_FOG " + json.dumps(record, sort_keys=True))


def props(obj, names):
    result = {}
    for name in names:
        try:
            value = obj.get_editor_property(name)
            result[name] = value if isinstance(value, (bool, int, float, str)) else str(value)
        except Exception as error:
            result[name] = {"unavailable": str(error)}
    return result


def export_graph(obj):
    task = unreal.AssetExportTask()
    task.object = obj
    task.filename = str(OUT / (obj.get_path_name().replace("/", "_").replace(":", "_") + ".t3d"))
    task.exporter = unreal.ObjectExporterT3D()
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    ok = unreal.Exporter.run_asset_export_task(task)
    emit("graph_export", asset=path(obj), output=task.filename, success=bool(ok))


actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ship_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Brigantine/BP/BP_Brigantine")
if not ship_class:
    raise RuntimeError("Missing actual game ship class BP_Brigantine")
ship = actors.spawn_actor_from_class(ship_class, unreal.Vector())
if not ship:
    raise RuntimeError("Could not construct BP_Brigantine in temporary editor world")
try:
    for component in ship.get_components_by_class(unreal.MeshComponent):
        mesh = None
        if isinstance(component, unreal.SkeletalMeshComponent):
            mesh = component.get_skeletal_mesh_asset()
        elif isinstance(component, unreal.StaticMeshComponent):
            mesh = component.get_editor_property("static_mesh")
        slots = []
        defaults = []
        if mesh:
            slot_property = "materials" if isinstance(mesh, unreal.SkeletalMesh) else "static_materials"
            for slot in mesh.get_editor_property(slot_property):
                defaults.append(dict(name=str(slot.get_editor_property("material_slot_name")),
                                     material=path(slot.get_editor_property("material_interface"))))
        for index in range(component.get_num_materials()):
            material = component.get_material(index)
            slots.append(dict(index=index, material=path(material)))
            if material:
                materials[path(material)] = material
        emit("component", name=component.get_name(), component_class=component.get_class().get_name(),
             mesh=path(mesh), location=str(component.get_world_location()),
             rotation=str(component.get_world_rotation()), bounds=str(unreal.SystemLibrary.get_component_bounds(component)),
             slots=slots, mesh_defaults=defaults,
             properties=props(component, ["visible", "hidden_in_game", "cast_shadow", "render_in_main_pass",
                 "render_in_depth_pass", "render_custom_depth", "translucency_sort_priority", "bounds_scale",
                 "evaluate_world_position_offset", "world_position_offset_disable_distance"]))

    # Traverse parent instances so construction-time dynamic overrides do not hide
    # the authored constants. Audit every assigned material, not guessed sail names.
    pending = list(materials.values())
    seen = set()
    bases = {}
    while pending:
        material = pending.pop()
        name = path(material)
        if name in seen:
            continue
        seen.add(name)
        base = material.get_base_material()
        if base:
            bases[path(base)] = base
        data = dict(path=name, material_class=material.get_class().get_name(), base=path(base))
        if isinstance(material, unreal.MaterialInstance):
            parent = material.get_editor_property("parent")
            data["parent"] = path(parent)
            data["instance_properties"] = props(material, ["base_property_overrides", "scalar_parameter_values",
                "vector_parameter_values", "texture_parameter_values"])
            data["base_overrides"] = props(material.get_editor_property("base_property_overrides"),
                ["override_blend_mode", "blend_mode", "override_shading_model", "shading_model",
                 "override_two_sided", "two_sided", "override_opacity_mask_clip_value", "opacity_mask_clip_value",
                 "override_dithered_lod_transition", "dithered_lod_transition"])
            if parent:
                pending.append(parent)
        if isinstance(material, unreal.MaterialInstanceConstant):
            data["scalars"] = {str(n): mel.get_material_instance_scalar_parameter_value(material, n)
                               for n in mel.get_scalar_parameter_names(material)}
            data["switches"] = {str(n): mel.get_material_instance_static_switch_parameter_value(material, n)
                                for n in mel.get_static_switch_parameter_names(material)}
            data["vectors"] = {str(n): str(mel.get_material_instance_vector_parameter_value(material, n))
                               for n in mel.get_vector_parameter_names(material)}
            data["textures"] = {}
            for parameter in mel.get_texture_parameter_names(material):
                texture = mel.get_material_instance_texture_parameter_value(material, parameter)
                data["textures"][str(parameter)] = dict(path=path(texture), properties=props(texture,
                    ["compression_settings", "srgb", "compression_no_alpha", "lod_group"]) if texture else {})
        emit("material", **data)

    visited_functions = set()
    for name, base in sorted(bases.items()):
        emit("base", path=name, properties=props(base, ["blend_mode", "shading_model", "two_sided",
            "opacity_mask_clip_value", "dithered_lod_transition", "use_material_attributes",
            "disable_depth_test", "use_translucency_vertex_fog", "apply_cloud_fogging", "compute_fog_per_pixel", "translucency_lighting_mode",
            "enable_responsive_aa", "output_translucent_velocity", "pixel_depth_offset_mode",
            "max_world_position_offset_displacement"]))
        export_graph(base)
        for property_name in ("MP_OPACITY", "MP_OPACITY_MASK", "MP_WORLD_POSITION_OFFSET",
                              "MP_PIXEL_DEPTH_OFFSET", "MP_EMISSIVE_COLOR", "MP_NORMAL", "MP_MATERIAL_ATTRIBUTES"):
            try:
                node = mel.get_material_property_input_node(base, getattr(unreal.MaterialProperty, property_name))
                emit("input", material=name, property=property_name, node=path(node),
                     node_class=node.get_class().get_name() if node else None)
            except Exception as error:
                emit("input_unavailable", material=name, property=property_name, error=str(error))
        nodes = list(mel.get_material_expressions(base))
        for node in nodes:
            export_graph(node)
            if isinstance(node, unreal.MaterialExpressionMaterialFunctionCall):
                function = node.get_editor_property("material_function")
                if function and path(function) not in visited_functions:
                    visited_functions.add(path(function))
                    export_graph(function)
                    nodes.extend(mel.get_material_function_expressions(function))
    emit("complete", components=sum(r["kind"] == "component" for r in records),
         assigned_materials=len(materials), base_materials=len(bases),
         scope="Constructed BP defaults/overrides; no voyage ticks or BeginPlay; no asset saves")
finally:
    actors.destroy_actor(ship)
    (OUT / "manifest.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
