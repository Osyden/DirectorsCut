"""Adds foam to the water plugin's ocean wherever the sea meets something solid: ruins, rocks, the hull.

Run with the editor closed, both steps:
    Tools/pyx.sh Tools/patch_drw_shorefoam.py      (restores the plugin's own files, or backs them up the first time)
    Tools/py.sh Tools/patch_drw_shorefoam.py 20    (adds the foam)
Undo:                        Tools/pyx.sh Tools/patch_drw_shorefoam.py --undo

The plugin's own shoreline surf is baked for land that stands still; our ruins slide past the ship,
so it cannot serve. This edits the plugin's two near ocean materials (M_DRW_High, M_DRW_Med) IN THE
ENGINE FOLDER. A copy of each is kept beside it (.dc_backup) the first time, and every run starts
again from that copy, so the patch can be re-run after changing it and removed with --undo.

It also lets the game lay the sea in a shallow trough (all four ocean materials): see TROUGH below and
Tools/make_sea_collection.py, which must have been run first.

What the foam adds: the engine knows, at every point, how far the nearest solid surface is (the global
distance field, which follows moving things). Within a few metres of one, the materials' existing
"Foam" input is raised: solid white water against the stone, breaking into lace further out,
washing in and out, and drifting aft with the current. Because it is part of the water's own
shading it lies on the real waves, with no gap and no painted band.
  DCShoreFoam        scalar parameter, 0 = off, 1 = full (default 1)
  DCShoreFoamWidth   scalar parameter, cm from the stone to where the foam ends (default 650)
"""
import os
import shutil
import sys

PLUGIN = "E:/Utilities/UE_5.8/Engine/Plugins/Marketplace/DynamicR1d9938e98fdbV5/Content/Materials/OceanLOD"
NAMES = ["M_DRW_High", "M_DRW_Med", "M_DRW_Far", "M_DRW_Low"]
FOAM_NAMES = ["M_DRW_High", "M_DRW_Med"]

if "--undo" in sys.argv:
    for n in NAMES:
        f = "%s/%s.uasset" % (PLUGIN, n)
        if os.path.exists(f + ".dc_backup"):
            shutil.copyfile(f + ".dc_backup", f)
            print("restored", f)
        else:
            print("no backup for", f)
    sys.exit(0)

try:
    import unreal
except ImportError:
    # Plain Python (Tools/pyx.sh), before the editor has the files open: start from the plugin's own
    # file every time.
    for n in NAMES:
        f = "%s/%s.uasset" % (PLUGIN, n)
        if os.path.exists(f + ".dc_backup"):
            shutil.copyfile(f + ".dc_backup", f)
        else:
            shutil.copyfile(f, f + ".dc_backup")
    print("prepared; now run: Tools/py.sh Tools/patch_drw_shorefoam.py 20")
    sys.exit(0)

mel = unreal.MaterialEditingLibrary

CODE = """
// Two ways of knowing that something solid is near: the distance field (the hull, which is in it), and
// how little water lies between the surface and whatever is seen through it (the ruins, which are not).
float near = min(Dist, max(Behind, 0.0) * 0.45);
float band = saturate(1.0 - near / max(Width, 1.0));
// The pattern is carried aft with the current (the sea streams past the ship toward -X).
float2 uv = float2(WP.x + T * 1200.0, WP.y) / 1100.0;
float lace = Texture2DSample(Tex, TexSampler, uv).r;
float lace2 = Texture2DSample(Tex, TexSampler, uv * 0.31 + float2(0.37, T * 0.01)).r;
float l = saturate((lace * lace2 - 0.12) * 3.2);
// The wash comes and goes along the stone.
float surge = 0.5 + 0.5 * sin(T * 0.8 + WP.x * 0.0021 + WP.y * 0.0017);
float reach = band * (0.7 + 0.6 * surge);
// A thin solid line of white where the water touches, torn lace beyond it, nothing at the outer edge.
float core = smoothstep(0.82, 1.0, band) * (0.6 + 0.4 * l);
float torn = saturate(reach * 2.3 - (1.0 - l) * 1.3);
float foam = max(core, torn);
return max(Existing, foam * Strength);
"""

TROUGH = """
// The sea of the drowned city lies in a shallow trough: level along the ship's course, rising to
// either hand, steeply at first and then levelling off. Trough = (middle, rise at the edge, half width).
// The game moves the ship and the ruins by the same rule (ADCRideDirector::SeaRise).
float u = abs(WP.y - Trough.x) / max(Trough.z, 1.0);
float f = u <= 1.0 ? pow(u, 2.5) : (u <= 2.0 ? 1.0 + 2.5 * (u - 1.0) - 1.25 * (u - 1.0) * (u - 1.0) : 2.25);
float here = 0.0;   // the ship lies at world Y 0: she is the level everything else is measured from
float us = abs(here - Trough.x) / max(Trough.z, 1.0);
float fs = us <= 1.0 ? pow(us, 2.5) : (us <= 2.0 ? 1.0 + 2.5 * (us - 1.0) - 1.25 * (us - 1.0) * (us - 1.0) : 2.25);
return Existing + float3(0.0, 0.0, Trough.y * (f - fs));
"""

for n in NAMES:
    path = "/DynamicRealWater/Materials/OceanLOD/" + n
    m = unreal.load_asset(path)
    # The trough: added to whatever the material already does to the surface's position.
    wpo = mel.get_material_property_input_node(m, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    collection = unreal.load_asset("/Game/DC/Materials/MPC_Sea")
    if wpo and collection:
        wpo_out = mel.get_material_property_input_node_output_name(m, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        tnode = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -200, 3400)
        tnode.set_editor_property("code", TROUGH)
        tnode.set_editor_property("description", "DCTrough")
        tnode.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        tin = []
        for name in ["Existing", "WP", "Trough"]:
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", name)
            tin.append(ci)
        tnode.set_editor_property("inputs", tin)
        twp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -700, 3400)
        twp.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
        tcp = mel.create_material_expression(m, unreal.MaterialExpressionCollectionParameter, -700, 3500)
        tcp.set_editor_property("collection", collection)
        tcp.set_editor_property("parameter_name", "Trough")
        okt = mel.connect_material_expressions(wpo, wpo_out, tnode, "Existing")
        okt = mel.connect_material_expressions(twp, "", tnode, "WP") and okt
        okt = mel.connect_material_expressions(tcp, "", tnode, "Trough") and okt
        okt = mel.connect_material_property(tnode, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET) and okt
        unreal.log_warning("DC_RESULT %s: trough wired=%s" % (n, okt))
    if n not in FOAM_NAMES:
        mel.recompile_material(m)
        unreal.log_warning("DC_RESULT %s: saved=%s" % (n, unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)))
        continue
    shading = mel.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
    names = [str(x) for x in mel.get_material_expression_input_names(shading)]
    ins = mel.get_inputs_for_material_expression(m, shading)
    if "Foam" not in names:
        unreal.log_warning("DC_RESULT %s: no Foam input on %s; left alone" % (n, shading.get_name()))
        continue
    existing = ins[names.index("Foam")]
    existing_out = mel.get_input_node_output_name_for_material_expression(shading, existing)

    node = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -200, 2600)
    node.set_editor_property("code", CODE)
    node.set_editor_property("description", "DCShoreFoam")
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs = []
    for name in ["Existing", "Dist", "Behind", "WP", "T", "Width", "Strength", "Tex"]:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        inputs.append(ci)
    node.set_editor_property("inputs", inputs)

    dist = mel.create_material_expression(m, unreal.MaterialExpressionDistanceToNearestSurface, -700, 2500)
    sd = mel.create_material_expression(m, unreal.MaterialExpressionSceneDepthWithoutWater, -1000, 2400)
    pd = mel.create_material_expression(m, unreal.MaterialExpressionPixelDepth, -1000, 2500)
    behind = mel.create_material_expression(m, unreal.MaterialExpressionSubtract, -850, 2450)
    mel.connect_material_expressions(sd, "", behind, "A")
    mel.connect_material_expressions(pd, "", behind, "B")
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -700, 2600)
    time = mel.create_material_expression(m, unreal.MaterialExpressionTime, -700, 2700)
    width = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, 2800)
    width.set_editor_property("parameter_name", "DCShoreFoamWidth")
    width.set_editor_property("default_value", 650.0)
    strength = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, 2900)
    strength.set_editor_property("parameter_name", "DCShoreFoam")
    strength.set_editor_property("default_value", 1.0)
    tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureObject, -700, 3000)
    tex.set_editor_property("texture", unreal.load_asset("/DynamicRealWater/Textures/T_DRW_OceanFoam_BaseColor"))

    ok = mel.connect_material_expressions(existing, existing_out, node, "Existing")
    for src, name in [(dist, "Dist"), (behind, "Behind"), (wp, "WP"), (time, "T"), (width, "Width"), (strength, "Strength"), (tex, "Tex")]:
        ok = mel.connect_material_expressions(src, "", node, name) and ok
    ok = mel.connect_material_expressions(node, "", shading, "Foam") and ok
    mel.recompile_material(m)
    saved = unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log_warning("DC_RESULT %s: shore foam wired=%s saved=%s" % (n, ok, saved))
