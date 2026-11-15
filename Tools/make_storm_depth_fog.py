"""Continuous ground mist for the existing heterogeneous storm front.

Generates M_StormComposite consumed by DCFogFront.
WorldPosition in a post-process material reconstructs the opaque scene-depth
endpoint, preserving wave crests/troughs and foreground ship occlusion.
"""
import unreal

mel = unreal.MaterialEditingLibrary
path = '/Game/DC/Materials/M_StormComposite'
mat = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_StormComposite', '/Game/DC/Materials', unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(mat)
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
mat.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)


def scalar(name, value):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    return node


scene = mel.create_material_expression(mat, unreal.MaterialExpressionSceneTexture)
scene.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition)
camera = mel.create_material_expression(mat, unreal.MaterialExpressionCameraPositionWS)
origin = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter)
origin.set_editor_property('parameter_name', 'Origin')
origin.set_editor_property('default_value', unreal.LinearColor(0, 0, 0, 0))

code = r'''
float amount=saturate(Shown)*smoothstep(38.0,56.0,StormTime);
if(amount<.0001) return Scene.rgb;
float3 delta=WP-Camera;
float distance=length(delta)*.01;
float end=min(distance,3000.0);
if(end<=25.0) return Scene.rgb;
float3 ray=delta/max(length(delta),.001);
float lengthFog=end-25.0;
float h0=(Camera.z-Origin.z)*.01+ray.z*25.0;
float h1=h0+ray.z*lengthFog;
// Exact integral of exp(-max(height,0)/15) along the depth-clipped ray.
// Its continuous primitive also handles waves below mean sea without a
// separate flat-plane cutoff or a discontinuous water-only pixel mask.
float integral;
if(abs(ray.z)<.0001) {
 integral=lengthFog*exp(-max((h0+h1)*.5,0.0)/15.0);
} else {
 float f0=h0<=0.0 ? h0 : 15.0*(1.0-exp(-h0/15.0));
 float f1=h1<=0.0 ? h1 : 15.0*(1.0-exp(-h1/15.0));
 integral=max((f1-f0)/ray.z,0.0);
}
float trans=exp(-.010*integral*amount);
float3 fogColor=float3(.58,.66,.76)*Light;
return Scene.rgb*trans+fogColor*(1.0-trans);
'''
custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom)
custom.set_editor_property('code', code)
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = {
    'Scene': scene, 'WP': wp, 'Camera': camera, 'Origin': origin,
    'Shown': scalar('Shown', 0), 'StormTime': scalar('StormTime', 0),
    'Light': scalar('Light', 1.4),
}
pins = []
for name in inputs:
    pin = unreal.CustomInput()
    pin.set_editor_property('input_name', name)
    pins.append(pin)
custom.set_editor_property('inputs', pins)
for name, source in inputs.items():
    assert mel.connect_material_expressions(source, '', custom, name)
assert mel.connect_material_property(custom, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
unreal.log_warning('STORM_DEPTH_FOG M_StormComposite saved')

