import unreal
for path in ['/Game/Brigantine/BP/Cannon/BP_EffectExplosionStartCannon', '/Game/Brigantine/BP/Cannon/BP_EffectExplosionEnd']:
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls, unreal.Vector())
    unreal.log('DC_RESULT EFFECT ' + path)
    for comp in actor.get_components_by_class(unreal.SceneComponent):
        unreal.log('DC_RESULT COMPONENT ' + comp.get_name() + ' ' + str(comp.get_editor_property('relative_rotation')) + ' ' + str(comp.get_editor_property('relative_scale3d')))
        if isinstance(comp, unreal.ParticleSystemComponent):
            unreal.log('DC_RESULT PARTICLE ' + str(comp.get_editor_property('template')))
world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/DC/Maps/Main') if False else None
registry = unreal.AssetRegistryHelpers.get_asset_registry()
for asset in registry.get_assets_by_path('/Game/Brigantine', recursive=True):
    name = str(asset.asset_name)
    if str(asset.asset_class_path.asset_name) in ['ParticleSystem', 'NiagaraSystem'] or ('Cannon' in name and str(asset.asset_class_path.asset_name) == 'SkeletalMesh'):
        unreal.log('DC_RESULT ASSET ' + str(asset.package_name))
        if str(asset.asset_class_path.asset_name) == 'SkeletalMesh':
            mesh = asset.get_asset()
            comp = unreal.new_object(unreal.SkeletalMeshComponent)
            comp.set_skeletal_mesh_asset(mesh)
            unreal.log('DC_RESULT SOCKETS ' + str(comp.get_all_socket_names()))
ship_cls = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Brigantine/BP/BP_Brigantine')
ship = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(ship_cls, unreal.Vector())
for comp in ship.get_components_by_class(unreal.SkeletalMeshComponent):
    if 'Cannon' in comp.get_name():
        unreal.log('DC_RESULT GUN ' + comp.get_name() + ' loc=' + str(comp.get_world_location()) + ' rot=' + str(comp.get_world_rotation()) + ' bounds=' + str(unreal.SystemLibrary.get_component_bounds(comp)))
        for bone in comp.get_all_socket_names():
            unreal.log('DC_RESULT BONE ' + str(bone) + ' ' + str(comp.get_socket_transform(bone, unreal.RelativeTransformSpace.RTS_COMPONENT)))
