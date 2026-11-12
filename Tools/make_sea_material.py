"""Builds /Game/DC/Materials/M_Sea: an animated ocean driven entirely by parameters.

Run with the editor closed:
  UnrealEditor-Cmd.exe DirectorsCut.uproject -run=pythonscript -script=Tools/make_sea_material.py

The ride director sets the parameters every frame (wave height, scroll, colours, glow), so the
same material is calm water, storm water and the burning sea of the hell channel.
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_Sea"
mel = unreal.MaterialEditingLibrary

# Shared wave set. xy = direction, z = wavelength (cm), w = relative amplitude.
WAVES = """
const float4 Waves[7] = {
    float4( 1.00,  0.25, 14000.0, 1.00),
    float4( 0.80, -0.60,  8200.0, 0.62),
    float4( 0.35,  0.94,  5100.0, 0.40),
    float4( 0.96, -0.10,  3100.0, 0.25),
    float4(-0.30,  0.95,  1900.0, 0.15),
    float4( 0.70,  0.71,  1150.0, 0.09),
    float4( 0.20, -0.98,   700.0, 0.055)
};
float2 p = P.xy + Scroll.xy;
"""

DISPLACE = WAVES + """
float3 d = float3(0, 0, 0);
for (int i = 0; i < 7; i++)
{
    float2 D = normalize(Waves[i].xy);
    float k = 6.28318 / Waves[i].z;
    float w = sqrt(980.0 * k);
    float ph = k * dot(D, p) - w * T;
    float A = Waves[i].w * Amp;
    d.xy -= D * (0.55 * A) * cos(ph);
    d.z += A * sin(ph);
}
// Flatten the far sea so the horizon stays a clean line.
float fade = saturate(1.0 - Dist / 400000.0);
return d * fade;
"""

NORMAL = WAVES + """
float2 g = float2(0, 0);
for (int i = 0; i < 7; i++)
{
    float2 D = normalize(Waves[i].xy);
    float k = 6.28318 / Waves[i].z;
    float w = sqrt(980.0 * k);
    float ph = k * dot(D, p) - w * T;
    g += D * (Waves[i].w * Amp * k * cos(ph));
}
// Small ripples, pixel only, faded with distance so they do not shimmer.
const float4 Ripples[5] = {
    float4( 0.6,  0.8, 420.0, 5.0),
    float4(-0.9,  0.4, 260.0, 3.2),
    float4( 0.2, -1.0, 170.0, 2.2),
    float4( 1.0,  0.1, 110.0, 1.4),
    float4(-0.5, -0.8,  70.0, 0.9)
};
float rf = saturate(1.0 - Dist / 25000.0) * (0.35 + Chop);
for (int j = 0; j < 5; j++)
{
    float2 D = normalize(Ripples[j].xy);
    float k = 6.28318 / Ripples[j].z;
    float w = sqrt(980.0 * k) * 1.4;
    float ph = k * dot(D, p) - w * T;
    g += D * (Ripples[j].w * rf * k * cos(ph));
}
return normalize(float3(-g.x, -g.y, 1.0));
"""

FOAM = WAVES + """
float h = 0.0;
float sharp = 0.0;
for (int i = 0; i < 7; i++)
{
    float2 D = normalize(Waves[i].xy);
    float k = 6.28318 / Waves[i].z;
    float w = sqrt(980.0 * k);
    float ph = k * dot(D, p) - w * T;
    h += Waves[i].w * sin(ph);
    sharp += Waves[i].w * Amp * k * sin(ph);
}
// Cheap breakup so foam is streaky rather than a clean band.
float2 q = p * 0.004;
float n = sin(q.x * 1.7 + sin(q.y * 2.3 + T * 0.4)) * sin(q.y * 1.3 - sin(q.x * 2.9 - T * 0.3));
float n2 = sin(p.x * 0.011 + n * 2.0) * sin(p.y * 0.013 - T * 0.6) * 0.5 + 0.5;
float n3 = sin(p.x * 0.031 - p.y * 0.017 + T) * sin(p.y * 0.037 + p.x * 0.009) * 0.5 + 0.5;
float crest = saturate((h - 1.45) * 1.6 + sharp * 0.8 - 0.15);
float f = saturate(crest * 1.6 - (0.35 + 0.45 * n2 + 0.35 * n3) * (1.2 - crest));
return f * saturate(0.75 + 0.25 * n) * saturate(1.0 - Dist / 150000.0);
"""


def custom(mat, code, out_type, names, x, y, desc):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", out_type)
    node.set_editor_property("description", desc)
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    node.set_editor_property("inputs", inputs)
    return node


def scalar(mat, name, value, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", value)
    return n


def vector(mat, name, value, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*value))
    return n


def build():
    path = PKG + "/" + NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("tangent_space_normal", False)
    mat.set_editor_property("two_sided", True)

    wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, 0)
    wp.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
    cam = mel.create_material_expression(mat, unreal.MaterialExpressionCameraPositionWS, -1400, 200)
    dist = mel.create_material_expression(mat, unreal.MaterialExpressionDistance, -1150, 150)
    mel.connect_material_expressions(wp, "", dist, "A")
    mel.connect_material_expressions(cam, "", dist, "B")

    time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1400, 350)
    amp = scalar(mat, "WaveAmp", 120.0, -1400, 450)
    chop = scalar(mat, "Chop", 0.3, -1400, 550)
    scroll = vector(mat, "Scroll", (0, 0, 0, 0), -1400, 650)

    def wire(node, with_chop):
        mel.connect_material_expressions(wp, "", node, "P")
        mel.connect_material_expressions(time, "", node, "T")
        mel.connect_material_expressions(amp, "", node, "Amp")
        mel.connect_material_expressions(scroll, "", node, "Scroll")
        mel.connect_material_expressions(dist, "", node, "Dist")
        if with_chop:
            mel.connect_material_expressions(chop, "", node, "Chop")

    names = ["P", "T", "Amp", "Scroll", "Dist"]
    disp = custom(mat, DISPLACE, unreal.CustomMaterialOutputType.CMOT_FLOAT3, names, -800, 0, "Wave displacement")
    wire(disp, False)
    norm = custom(mat, NORMAL, unreal.CustomMaterialOutputType.CMOT_FLOAT3, names + ["Chop"], -800, 300, "Wave normal")
    wire(norm, True)
    foam = custom(mat, FOAM, unreal.CustomMaterialOutputType.CMOT_FLOAT1, names, -800, 600, "Foam mask")
    wire(foam, False)

    mel.connect_material_property(disp, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    mel.connect_material_property(norm, "", unreal.MaterialProperty.MP_NORMAL)

    # Foam strength is a parameter so calm phases stay glassy.
    foam_amt = scalar(mat, "FoamAmount", 1.0, -800, 800)
    foam_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 650)
    mel.connect_material_expressions(foam, "", foam_mul, "A")
    mel.connect_material_expressions(foam_amt, "", foam_mul, "B")

    deep = vector(mat, "DeepColor", (0.004, 0.02, 0.03, 1), -500, 0)
    foam_col = vector(mat, "FoamColor", (0.75, 0.8, 0.82, 1), -500, 180)
    base = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, 60)
    mel.connect_material_expressions(deep, "", base, "A")
    mel.connect_material_expressions(foam_col, "", base, "B")
    mel.connect_material_expressions(foam_mul, "", base, "Alpha")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_lo = scalar(mat, "Roughness", 0.06, -500, 380)
    rough_hi = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 470)
    rough_hi.set_editor_property("r", 0.55)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, 400)
    mel.connect_material_expressions(rough_lo, "", rough, "A")
    mel.connect_material_expressions(rough_hi, "", rough, "B")
    mel.connect_material_expressions(foam_mul, "", rough, "Alpha")
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -250, 520)
    spec.set_editor_property("r", 0.7)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    # Glow: zero for water. In the hell channel the troughs glow and the crests crust over.
    glow_col = vector(mat, "GlowColor", (0, 0, 0, 1), -500, 900)
    one_minus = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -350, 760)
    mel.connect_material_expressions(foam_mul, "", one_minus, "")
    glow = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 850)
    mel.connect_material_expressions(glow_col, "", glow, "A")
    mel.connect_material_expressions(one_minus, "", glow, "B")
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(mat)
    ok = unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("DC_RESULT M_Sea saved=%s expressions=%d" % (ok, mel.get_num_material_expressions(mat)))


build()
