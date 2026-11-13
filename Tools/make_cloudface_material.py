"""Builds /Game/DC/Materials/M_CloudFace: a face in the storm cloud.

Run with the editor closed:  Tools/py.sh Tools/make_cloudface_material.py
Put on a statue's head enlarged to hundreds of metres and hung in the cloud over the drowned city
(DCCloudFace.cpp). It is drawn as cloud, not stone: unlit and translucent, thick where the surface
faces the eye and thinning to nothing round the outline, torn by the same drifting noise as the
fog, modelled by a soft light from above so the brow, nose and mouth read, and lit from inside
when lightning strikes.
  Tint    colour of the cloud in the light (rgb), a = how much of the face is there (0..1)
  Shade   colour in its hollows
  Flash   light added by lightning (rgb)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_CloudFace"
mel = unreal.MaterialEditingLibrary

CODE = """
#define NZ(p) saturate((Texture3DSampleLevel(Shape, ShapeSampler, (p), 0.0).r - 0.835) / 0.125)
float facing = saturate(dot(normalize(N), normalize(V)));
float3 q = WP / 26000.0 + float3(T * 0.004, 0.0, T * 0.002);
float n = NZ(q) * 0.6 + NZ(q * 3.1 + 0.4) * 0.4;
// Thick in the middle of every form, nothing at its outline, and eaten by the drifting noise.
float a = pow(facing, 1.6) * saturate(1.25 - n * 0.9) * Tint.a;
// Light from above and a little ahead models the features.
float lit = smoothstep(0.32, 0.78, normalize(N).z * 0.5 + 0.5);
float3 c = lerp(Shade.rgb, Tint.rgb, lit) * (0.8 + 0.4 * n) + Flash.rgb * (0.15 + 0.85 * lit);
return float4(c, saturate(a));
"""

path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", False)
# It hangs kilometres off in the sky: the sea fog must not paint it out.
mat.set_editor_property("use_translucency_vertex_fog", True)
# The sky's clouds are laid over ordinary translucency; drawn in the later pass, the face stands in front of the cloud deck.
# (Tried: drawing it in the later pass to stand in front of the sky's cloud deck. The deck still covered it.)

node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 300)
node.set_editor_property("code", CODE)
node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
inputs = []
for n in ["WP", "N", "V", "T", "Tint", "Shade", "Flash", "Shape"]:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
node.set_editor_property("inputs", inputs)


def vector(name, value, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1300, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(*value))
    a = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -1050, y)
    mel.connect_material_expressions(p, "", a, "A")
    mel.connect_material_expressions(p, "A", a, "B")
    return a


wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1300, -300)
nrm = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1300, -200)
cam = mel.create_material_expression(mat, unreal.MaterialExpressionCameraVectorWS, -1300, -100)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1300, 0)
tint = vector("Tint", (0.62, 0.68, 0.78, 1.0), 100)
shade = vector("Shade", (0.1, 0.12, 0.17, 1.0), 300)
flash = vector("Flash", (0.0, 0.0, 0.0, 0.0), 500)
shape = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -1300, 700)
shape.set_editor_property("texture", unreal.load_asset("/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128"))
for src, name in [(wp, "WP"), (nrm, "N"), (cam, "V"), (time, "T"), (tint, "Tint"), (shade, "Shade"), (flash, "Flash"), (shape, "Shape")]:
    mel.connect_material_expressions(src, "", node, name)
rgb = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -200, 250)
rgb.set_editor_property("r", True); rgb.set_editor_property("g", True); rgb.set_editor_property("b", True); rgb.set_editor_property("a", False)
alpha = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -200, 400)
alpha.set_editor_property("r", False); alpha.set_editor_property("g", False); alpha.set_editor_property("b", False); alpha.set_editor_property("a", True)
mel.connect_material_expressions(node, "", rgb, "")
mel.connect_material_expressions(node, "", alpha, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)

mel.recompile_material(mat)
unreal.log_warning("DC_RESULT M_CloudFace saved=%s" % unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False))
