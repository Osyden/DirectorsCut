"""Builds /Game/DC/Materials/M_Sea: an animated ocean driven entirely by parameters.

Run with the editor closed:  Tools/py.sh Tools/make_sea_material.py

The ride director sets the parameters every frame, so one material is calm water, storm water,
the burning sea of the hell channel, the water tunnel of the slide and the maelstrom funnel.

Parameters
  WaveAmp      height of the biggest swell (cm)
  Chop         small ripple strength
  Scroll       xy = how far the water has slid past the ship (cm)
  Shape        x = tube amount 0..1 (sea wraps up and over the ship), y = tube radius (cm),
               z = drop (cm of fall 100 m ahead; the sea falls away in front and rises behind)
  Whirl        xy = whirlpool centre relative to the ship (cm), z = depth (cm), w = radius (cm)
  Crater       x = rim radius (cm), y = height of the cliff of falling water at the rim (cm),
               z = half-width of that cliff (cm), w = tunnel mouth distance ahead of the ship (cm, 0 = no mouth)
  DeepColor, FoamColor, FoamAmount, Roughness, GlowColor (lava), ScatterColor (daylight inside the water)
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
d *= saturate(1.0 - Dist / 400000.0);

// Calm the water against the hull, and sink it inside the hull so it never shows on deck or below.
float nearShip = saturate(1.0 - length(P.xy - float2(320.0, 0.0)) / 7000.0);
d *= lerp(1.0, 0.3, nearShip * nearShip);
float2 e = float2((P.x - 150.0) / 1800.0, P.y / 400.0);
d.z -= 600.0 * smoothstep(0.0, 0.2, saturate(1.0 - dot(e, e)));

// q = where this bit of sea ends up, relative to the ship at sea level.
float3 q = float3(P.x + d.x, P.y + d.y, d.z);

// Whirlpool: a funnel centred away from the ship; height is zero at the ship itself.
float wr = max(Whirl.w, 1.0);
float r2 = dot(q.xy - Whirl.xy, q.xy - Whirl.xy) / (wr * wr);
float r02 = dot(Whirl.xy, Whirl.xy) / (wr * wr);
q.z -= Whirl.z * (1.0 / (1.0 + r2) - 1.0 / (1.0 + r02));

// Crater rim: outside a ring around the same centre the sea stands a cliff's height above the bowl,
// so the bowl is walled by falling water. The ship is inside the ring.
float rr = sqrt(r2) * wr;
float rr0 = sqrt(r02) * wr;
float inRim = 1.0 - smoothstep(Crater.x - Crater.z, Crater.x + Crater.z, rr);
float inRim0 = 1.0 - smoothstep(Crater.x - Crater.z, Crater.x + Crater.z, rr0);
q.z -= Crater.y * (inRim - inRim0);

// Drop: the sea falls away ahead and rises behind, like cresting a slide.
float xn = clamp(q.x / 10000.0, -6.0, 6.0);
q.z -= Shape.z * xn * abs(xn);

// Tube: roll the sea up around the ship's fore-aft axis until it closes overhead. With a mouth
// distance set, the tube opens out ahead of the ship: a tunnel the ship is bursting out of.
float tube = Shape.x;
if (Crater.w > 1.0) { tube *= 1.0 - smoothstep(Crater.w, Crater.w * 3.0 + 2500.0, q.x); }
if (tube > 0.001)
{
    float Rt = Shape.y / tube;
    float th = clamp(q.y / Rt, -3.14159, 3.14159);
    float rad = Rt - q.z;
    q.y = rad * sin(th);
    q.z = Rt - rad * cos(th);
}
return float3(q.x - P.x, q.y - P.y, q.z);
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
// Slope of the whirlpool funnel.
float wr = max(Whirl.w, 1.0);
float2 rel = P.xy - Whirl.xy;
float r2 = dot(rel, rel) / (wr * wr);
g += rel * (Whirl.z * 2.0 / (wr * wr) / ((1.0 + r2) * (1.0 + r2)));
// Slope of the crater wall: steep, facing the centre.
float rr = sqrt(r2) * wr;
float band = saturate(1.0 - abs(rr - Crater.x) / max(Crater.z, 1.0));
g += normalize(rel + 0.001) * (Crater.y / max(2.0 * Crater.z, 1.0)) * 1.5 * band;
// Slope of the drop.
g.x -= Shape.z * 2.0 * abs(clamp(P.x / 10000.0, -6.0, 6.0)) / 10000.0;

float3 n = normalize(float3(-g.x, -g.y, 1.0));
float tube = Shape.x;
if (Crater.w > 1.0) { tube *= 1.0 - smoothstep(Crater.w, Crater.w * 3.0 + 2500.0, P.x); }
if (tube > 0.001)
{
    float Rt = Shape.y / tube;
    float th = clamp(P.y / Rt, -3.14159, 3.14159);
    float s = sin(th);
    float c = cos(th);
    n = float3(n.x, n.y * c - n.z * s, n.y * s + n.z * c);
}
return n;
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
// Cheap breakup so foam is streaky and ragged rather than a clean band.
float2 q = p * 0.004;
float n = sin(q.x * 1.7 + sin(q.y * 2.3 + T * 0.4)) * sin(q.y * 1.3 - sin(q.x * 2.9 - T * 0.3));
float n2 = sin(p.x * 0.011 + n * 2.0) * sin(p.y * 0.013 - T * 0.6) * 0.5 + 0.5;
float n3 = sin(p.x * 0.031 - p.y * 0.017 + T) * sin(p.y * 0.037 + p.x * 0.009) * 0.5 + 0.5;
float crest = saturate((h - 1.5) * 1.6 + sharp * 0.3 - 0.1);
float f = saturate(crest * 1.6 - (0.35 + 0.45 * n2 + 0.35 * n3) * (1.2 - crest));
// Streaks that follow the flow: strong inside the tube and on the funnel wall.
float streak = saturate(sin(p.y * 0.006 + n * 3.0) * sin(p.x * 0.0007 + n2 * 2.0) - 0.55) * 2.2;
float wr = max(Whirl.w, 1.0);
float2 rel = P.xy - Whirl.xy;
float funnel = saturate(Whirl.z / 20000.0) / (1.0 + dot(rel, rel) / (wr * wr));
f = saturate(f + streak * saturate(Shape.x * 1.2 + funnel * 0.6));
f = f * saturate(0.75 + 0.25 * n) * saturate(1.0 - Dist / 150000.0);

// The maelstrom: white water pouring down the crater wall, churn where it lands, and spiral
// arms of foam winding down the bowl toward the centre.
if (Crater.y > 1.0)
{
    float rr = length(rel);
    float ang = atan2(rel.y, rel.x);
    float wall = saturate(1.0 - abs(rr - Crater.x) / (Crater.z * 1.25));
    float falls = 0.55 + 0.45 * sin(ang * 380.0 + sin(ang * 61.0 + T * 0.7) * 3.0) * sin(ang * 97.0 - T * 1.3);
    float churn = saturate(1.0 - abs(rr - (Crater.x - Crater.z * 1.8)) / (Crater.z * 1.2));
    float inside = saturate((Crater.x - rr) / Crater.x);
    float arms = saturate(sin(ang * 5.0 + log(max(rr, 100.0)) * 9.0 + T * 0.35) * 0.5 + 0.5 - 0.62) * 2.6;
    float armNoise = 0.6 + 0.4 * sin(rr * 0.004 + ang * 23.0 + n2 * 3.0);
    f = saturate(f + wall * falls * 1.3 + churn * (0.6 + 0.4 * n2) + arms * armNoise * inside * (0.35 + 0.65 * saturate(1.0 - rr / (Crater.x * 0.8))));
}
return f;
"""


LAVA = WAVES + """
// Glowing cracks between drifting plates of crust.
float a = sin(p.x * 0.0021 + sin(p.y * 0.0031 + T * 0.05) * 1.7);
float b = sin(p.y * 0.0019 + sin(p.x * 0.0027 - T * 0.04) * 1.3);
float c = abs(a * b);
float fine = abs(sin(p.x * 0.007 + b * 4.0 + sin(p.y * 0.004) * 2.0) * sin(p.y * 0.0083 + a * 4.0 + sin(p.x * 0.005) * 2.0));
float cracks = pow(saturate(1.0 - c * 9.0), 2.0) + 0.08 * pow(saturate(1.0 - fine * 7.0), 3.0);
// Far lava sinks into the smoke, so the channel has depth and is not one bright sheet.
return (cracks + 0.012) * (0.12 + 0.88 * saturate(1.0 - Dist / 45000.0));
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
    shape = vector(mat, "Shape", (0, 3200, 0, 0), -1400, 850)
    whirl = vector(mat, "Whirl", (0, -70000, 0, 60000), -1400, 1050)
    crater = vector(mat, "Crater", (100000, 0, 5000, 0), -1400, 1250)

    # Vector parameters output RGB on the first pin; the custom nodes need all four channels.
    def rgba(node, x, y):
        app = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, x, y)
        mel.connect_material_expressions(node, "", app, "A")
        mel.connect_material_expressions(node, "A", app, "B")
        return app

    scroll4 = rgba(scroll, -1150, 650)
    shape4 = rgba(shape, -1150, 850)
    whirl4 = rgba(whirl, -1150, 1050)
    crater4 = rgba(crater, -1150, 1250)

    def wire(node, with_chop):
        mel.connect_material_expressions(wp, "", node, "P")
        mel.connect_material_expressions(time, "", node, "T")
        mel.connect_material_expressions(amp, "", node, "Amp")
        mel.connect_material_expressions(scroll4, "", node, "Scroll")
        mel.connect_material_expressions(dist, "", node, "Dist")
        mel.connect_material_expressions(shape4, "", node, "Shape")
        mel.connect_material_expressions(whirl4, "", node, "Whirl")
        mel.connect_material_expressions(crater4, "", node, "Crater")
        if with_chop:
            mel.connect_material_expressions(chop, "", node, "Chop")

    names = ["P", "T", "Amp", "Scroll", "Dist", "Shape", "Whirl", "Crater"]
    disp = custom(mat, DISPLACE, unreal.CustomMaterialOutputType.CMOT_FLOAT3, names, -800, 0, "Wave displacement and sea shape")
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
    foam_sat = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, -400, 650)
    mel.connect_material_expressions(foam_mul, "", foam_sat, "")

    deep = vector(mat, "DeepColor", (0.004, 0.02, 0.03, 1), -500, 0)
    foam_col = vector(mat, "FoamColor", (0.75, 0.8, 0.82, 1), -500, 180)
    base = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, 60)
    mel.connect_material_expressions(deep, "", base, "A")
    mel.connect_material_expressions(foam_col, "", base, "B")
    mel.connect_material_expressions(foam_sat, "", base, "Alpha")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_lo = scalar(mat, "Roughness", 0.06, -500, 380)
    rough_hi = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 470)
    rough_hi.set_editor_property("r", 0.55)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, 400)
    mel.connect_material_expressions(rough_lo, "", rough, "A")
    mel.connect_material_expressions(rough_hi, "", rough, "B")
    mel.connect_material_expressions(foam_sat, "", rough, "Alpha")
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -250, 520)
    spec.set_editor_property("r", 0.7)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    # Glow: zero for water. In the hell channel the troughs glow and the crests crust over.
    glow_col = vector(mat, "GlowColor", (0, 0, 0, 1), -500, 900)
    one_minus = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -350, 760)
    mel.connect_material_expressions(foam_sat, "", one_minus, "")
    glow = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 850)
    mel.connect_material_expressions(glow_col, "", glow, "A")
    lava = custom(mat, LAVA, unreal.CustomMaterialOutputType.CMOT_FLOAT1, names, -800, 1000, "Lava crack mask")
    wire(lava, False)
    lava_mask = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 950)
    mel.connect_material_expressions(lava, "", lava_mask, "A")
    mel.connect_material_expressions(one_minus, "", lava_mask, "B")
    mel.connect_material_expressions(lava_mask, "", glow, "B")
    # Scatter: light carried inside the water, so sunlit walls of water glow teal instead of going black.
    scatter = vector(mat, "ScatterColor", (0, 0, 0, 1), -500, 1100)
    emissive = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, 0, 950)
    mel.connect_material_expressions(glow, "", emissive, "A")
    mel.connect_material_expressions(scatter, "", emissive, "B")
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(mat)
    ok = unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log_warning("DC_RESULT M_Sea saved=%s expressions=%d" % (ok, mel.get_num_material_expressions(mat)))


build()
