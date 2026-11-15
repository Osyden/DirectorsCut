"""Read-only assembled/restored groom lighting audit. NullRHI; no asset saves."""
import json
from pathlib import Path
import unreal

rows = []
materials = {}
mel = unreal.MaterialEditingLibrary
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def prop(obj, name):
    try:
        return str(obj.get_editor_property(name))
    except Exception as exc:
        return 'UNAVAILABLE: ' + str(exc)

def record_material(material):
    if not material or material.get_path_name() in materials:
        return
    base = material.get_base_material()
    record = {'path': material.get_path_name(), 'base': base.get_path_name() if base else None}
    if base:
        record['base_properties'] = {p: prop(base, p) for p in (
            'blend_mode', 'shading_model', 'two_sided', 'use_material_attributes', 'used_with_hair_strands')}
    if isinstance(material, unreal.MaterialInstanceConstant):
        record['scalars'] = {str(p): mel.get_material_instance_scalar_parameter_value(material, p)
                             for p in mel.get_scalar_parameter_names(material)}
        record['switches'] = {str(p): mel.get_material_instance_static_switch_parameter_value(material, p)
                              for p in mel.get_static_switch_parameter_names(material)}
        record['vectors'] = {str(p): str(mel.get_material_instance_vector_parameter_value(material, p))
                             for p in mel.get_vector_parameter_names(material)}
    materials[material.get_path_name()] = record

for role in ('Odessa', 'Marlow', 'Finch'):
    root = '/Game/PirateCast/Assembled/' + role
    actor = sub.spawn_actor_from_class(unreal.EditorAssetLibrary.load_blueprint_class(root + '/BP_' + role), unreal.Vector())
    if not actor:
        raise RuntimeError('Failed to construct ' + role)
    try:
        for component in actor.get_components_by_class(unreal.GroomComponent):
            if component.get_name() != 'Hair':
                continue
            row = {'role': role, 'component': component.get_name(), 'settings': {
                p: prop(component, p) for p in ('cast_shadow', 'cast_dynamic_shadow', 'cast_contact_shadow',
                    'hair_shadow_density', 'hair_raytracing_radius_scale', 'use_cards', 'lighting_channels',
                    'hair_groups_desc', 'override_materials')}}
            for label in ('assembled', 'restored'):
                if label == 'restored':
                    component.set_groom_asset(unreal.load_asset(root + '/Grooms/Gr_Hair_Strands'))
                    component.set_binding_asset(unreal.load_asset(root + '/Grooms/GB_Hair_Strands'))
                groom = component.get_editor_property('groom_asset')
                data = {'groom': groom.get_path_name() if groom else None,
                        'groom_materials': prop(groom, 'hair_groups_materials') if groom else None,
                        'rendering': prop(groom, 'hair_groups_rendering') if groom else None,
                        'slots': []}
                for index in range(component.get_num_materials()):
                    material = component.get_material(index)
                    data['slots'].append(material.get_path_name() if material else None)
                    record_material(material)
                row[label] = data
            rows.append(row)
    finally:
        sub.destroy_actor(actor)

graph_dir = Path(unreal.Paths.project_saved_dir()) / 'PirateCast/HairLightingGraph'
graph_dir.mkdir(parents=True, exist_ok=True)
base = unreal.load_asset('/Game/PirateCastVendor/QuangPhan/Common/Materials/MaterialMasters/GroomMaterials/M_hair_v4')
nodes = [base] + list(mel.get_material_expressions(base))
visited = set()
for node in nodes:
    if node.get_path_name() in visited:
        continue
    visited.add(node.get_path_name())
    task = unreal.AssetExportTask()
    task.object = node
    task.filename = str(graph_dir / (node.get_path_name().replace('/', '_').replace(':', '_') + '.t3d'))
    task.exporter = unreal.ObjectExporterT3D()
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError('Graph export failed ' + node.get_path_name())
    if isinstance(node, unreal.MaterialExpressionMaterialFunctionCall):
        fn = node.get_editor_property('material_function')
        if fn and fn.get_path_name() not in visited:
            nodes.append(fn)
            nodes.extend(mel.get_material_function_expressions(fn))

out = Path(unreal.Paths.project_saved_dir()) / 'PirateCast/HairLightingAudit.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps({'grooms': rows, 'materials': materials}, indent=2), encoding='utf-8')
unreal.log_warning('DC_HAIR_LIGHTING complete grooms=%d materials=%d path=%s' % (len(rows), len(materials), out))
