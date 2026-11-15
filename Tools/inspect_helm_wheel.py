"""Read the authored wheel rig without saving or changing any content asset."""
import unreal

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ship_class = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Brigantine/BP/BP_Brigantine')
ship = actors.spawn_actor_from_class(ship_class, unreal.Vector())
for component in ship.get_components_by_class(unreal.SkeletalMeshComponent):
    if component.get_name() != 'SK_Wheel':
        continue
    unreal.log_warning('DC_HELM component=%s location=%s rotation=%s bounds=%s' % (
        component.get_name(), component.get_world_location(), component.get_world_rotation(), unreal.SystemLibrary.get_component_bounds(component)))
    mesh = component.get_skeletal_mesh_asset()
    unreal.log_warning('DC_HELM mesh=%s anim_class=%s animation_mode=%s' % (
        mesh.get_path_name(), component.get_editor_property('anim_class'), component.get_editor_property('animation_mode')))
    for index in range(component.get_num_bones()):
        bone = component.get_bone_name(index)
        unreal.log_warning('DC_HELM bone=%s parent=%s component_pose=%s' % (
            bone, component.get_parent_bone(bone), component.get_socket_transform(bone, unreal.RelativeTransformSpace.RTS_COMPONENT)))
registry = unreal.AssetRegistryHelpers.get_asset_registry()
for asset in registry.get_assets_by_path('/Game/Brigantine/Animation/Wheel', recursive=True):
    unreal.log_warning('DC_HELM animation=%s class=%s' % (asset.package_name, asset.asset_class_path.asset_name))
actors.destroy_actor(ship)
