"""Builds /Game/DC/Materials/M_StormCloud: the material for the volumetric cloud layer.

Run with the editor closed:  Tools/py.sh Tools/make_stormcloud_material.py

One ray-marched volume holds both the sky's cloud deck and the storm front, so both are lit and
shadowed by the sun and by themselves. The ride director sets the parameters every frame.

Parameters
  Sky     x = cover 0..1, y = cloud base (cm above the sea), z = cloud top (cm), w = extinction scale
  Front   x = world X of the face of the storm front (cm), y = strength 0..1 (0 = no front),
          z = radius of the tunnel mouth bored into it along the ship's course (cm, 0 = none),
          w = lightning 0..1 (lights the mouth and the inside of the wall)
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_StormCloud"
mel = unreal.MaterialEditingLibrary

DENSITY = """
// Noise is computed, not sampled: the engine noise volumes came through as plain white here.
struct FNoise
{
    float Hash(float3 p) { p = frac(p * 0.3183099 + 0.1); p *= 17.0; return frac(p.x * p.y * p.z * (p.x + p.y + p.z)); }
    float3 Hash3(float3 p)
    {
        p = float3(dot(p, float3(127.1, 311.7, 74.7)), dot(p, float3(269.5, 183.3, 246.1)), dot(p, float3(113.5, 271.9, 124.6)));
        return frac(sin(p) * 43758.5453);
    }
    float Value(float3 x)
    {
        float3 i = floor(x); float3 f = frac(x); f = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(lerp(Hash(i), Hash(i + float3(1, 0, 0)), f.x), lerp(Hash(i + float3(0, 1, 0)), Hash(i + float3(1, 1, 0)), f.x), f.y),
                    lerp(lerp(Hash(i + float3(0, 0, 1)), Hash(i + float3(1, 0, 1)), f.x), lerp(Hash(i + float3(0, 1, 1)), Hash(i + float3(1, 1, 1)), f.x), f.y), f.z);
    }
    // Distance to the nearest of a scatter of points, one per cell: 0 at a point, about 0.9 at most.
    // One minus this is a field of round heads, which is what a cumulus tower is made of.
    float Worley(float3 x)
    {
        float3 i = floor(x - 0.5); float3 f = x - i; float d = 8.0;
        for (int a = 0; a < 2; a++) { for (int b = 0; b < 2; b++) { for (int c = 0; c < 2; c++)
        {
            float3 g = float3(a, b, c);
            float3 o = g + 0.25 + 0.5 * Hash3(i + g) - f;
            d = min(d, dot(o, o));
        } } }
        return sqrt(d);
    }
};
FNoise Noise;
float alt = P.z;

// Most of the volume is empty air: find that out before paying for any noise.
float h = (alt - Sky.y) / max(Sky.z - Sky.y, 1.0);
bool inDeck = h > 0.0 && h < 1.0 && Sky.x > 0.01;
// The front leans out over the ship higher up, like a breaking wave.
float lean = 140000.0 * smoothstep(0.0, 1.0, alt / 450000.0);
float face = P.x - Front.x + lean;
bool inWall = Front.y > 0.001 && face > -130000.0 && face < 380000.0 && alt < 800000.0;
if (!inDeck && !inWall) { return float4(0.0, 0.0, 0.0, 0.0); }

float deck = 0.0;
float wall = 0.0;
float glow = 0.0;
if (inDeck)
{
    // The deck: flat-bottomed, heaped on top, as much of the sky as Sky.x says.
    float3 km = P * 0.00001;
    float3 wind = float3(T * 0.004, T * 0.0015, 0.0);
    float big = Noise.Value(km * 0.22 + wind * 30.0);
    float mid = Noise.Value(km * 0.75 + wind * 60.0 + 7.3);
    float w1 = 1.0 - Noise.Worley(km * 0.9 + wind * 80.0);
    float n = big * 0.5 + mid * 0.22 + w1 * 0.28;
    float need = 1.0 - Sky.x + 0.2 * h;
    deck = saturate((n - need) * 4.0) * saturate(h * 8.0) * saturate((1.0 - h) * 4.0);
    float er = Noise.Value(km * 3.1 + wind * 150.0) * 0.6 + Noise.Value(km * 8.7 - wind * 200.0) * 0.4;
    deck = saturate(deck - er * 0.4 * (1.0 - deck));
}
if (inWall)
{
    // The front. Its noise travels with it, or the face would boil as it came on.
    float3 q = float3(P.x - Front.x, P.y, alt) * 0.00001;
    float churn = T * 0.012;
    float wb = Noise.Value(q * 0.2 + float3(0.0, 0.0, -churn));
    float w1 = 1.0 - Noise.Worley(q * 0.7 + float3(churn, 0.0, -churn * 2.0));
    float w2 = 1.0 - Noise.Worley(q * 2.1 + 3.7 + float3(0.0, churn, -churn * 3.0));
    float billow = w1 * 0.65 + w2 * 0.35;
    // How far inside the surface of the cloud this point is, cm: towers 5 km across, heads of 1.4 km and 500 m on them.
    float surf = face + (wb - 0.5) * 260000.0 + (billow - 0.55) * 150000.0;
    float bore = 1.0;
    if (Front.z > 1.0)
    {
        // The mouth: a tunnel along the ship's course, narrowing as it goes in. The face of the front
        // is drawn back into it like a funnel, and spiral arms of cloud wind down into the hole.
        float R = Front.z * lerp(1.0, 0.45, saturate((face - 80000.0) / 240000.0));
        float2 m = float2(P.y, alt - R * 0.45);
        float rr = length(m) / R;
        float near = saturate(1.0 - (rr - 1.0) / 6.0);
        float spiral = sin(atan2(m.y, m.x) * 3.0 + log(max(rr, 0.3)) * 5.0 + T * 0.4 - q.x * 2.5);
        surf += (spiral * 30000.0 - near * 80000.0) * near;
        float edge = rr - 1.0 - 0.15 * spiral - (w2 - 0.5) * 0.35;
        bore = saturate(edge / 0.25);
        // Cold light in the walls of the tunnel, brighter with each stroke of lightning.
        glow = (0.1 + Front.w) * exp(-max(edge, 0.0) * 3.0) * saturate(face / 60000.0 + 0.5);
    }
    float topAlt = 380000.0 + wb * 280000.0 + (billow - 0.55) * 120000.0;
    wall = saturate(surf / 15000.0) * saturate((topAlt - alt) / 50000.0) * saturate((330000.0 - face) / 90000.0) * bore * Front.y;
    // Sheet lightning inside the cloud: now and then one stretch of the wall lights from within.
    float cell = floor(P.y / 500000.0);
    float slot = floor(T / 0.6 + cell * 0.37);
    if (Noise.Hash(float3(cell * 1.7 + 0.3, slot * 0.37, 2.1)) > 0.9 && Noise.Hash(float3(floor(T * 14.0), cell, 4.0)) > 0.35)
    {
        float3 c = float3(70000.0, (cell + 0.5 + 0.6 * (Noise.Hash(float3(cell, slot, 5.0)) - 0.5)) * 500000.0, 90000.0 + 260000.0 * Noise.Hash(float3(slot, cell, 9.0)));
        float3 dd = float3(face, P.y, alt) - c;
        glow += 1.6 / (1.0 + dot(dd, dd) / (60000.0 * 60000.0));
    }
    float erw = Noise.Value(q * 3.1 + churn * 4.0) * 0.6 + Noise.Value(q * 8.7 - churn * 6.0) * 0.4;
    wall = saturate(wall - erw * 0.35 * (1.0 - wall));
}
float d = max(deck, wall);
float mask = saturate(wall * 3.0);
return float4(d * Sky.w * lerp(1.0, 3.0, mask), glow * wall, mask, 0.0);
"""


def vector(mat, name, value, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*value))
    app = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, x + 250, y)
    mel.connect_material_expressions(n, "", app, "A")
    mel.connect_material_expressions(n, "A", app, "B")
    return app


def colour(mat, name, value, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*value))
    return n


path = PKG + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_VOLUME)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("used_with_volumetric_cloud", True)

wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, 0)
time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1400, 150)
sky = vector(mat, "Sky", (0.4, 150000, 550000, 0.02), -1400, 700)
front = vector(mat, "Front", (400000, 0, 0, 0), -1400, 900)
albedo = colour(mat, "Albedo", (0.9, 0.9, 0.9, 1), -1400, 1100)
storm = colour(mat, "StormAlbedo", (0.5, 0.56, 0.66, 1), -1400, 1300)
flash = colour(mat, "FlashColor", (0.16, 0.22, 0.4, 1), -1400, 1500)

node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -800, 300)
node.set_editor_property("code", DENSITY)
node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
inputs = []
for n in ["P", "T", "Sky", "Front"]:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
node.set_editor_property("inputs", inputs)
mel.connect_material_expressions(wp, "", node, "P")
mel.connect_material_expressions(time, "", node, "T")
mel.connect_material_expressions(sky, "", node, "Sky")
mel.connect_material_expressions(front, "", node, "Front")


def channel(r, g, b, y):
    m = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -450, y)
    m.set_editor_property("r", r); m.set_editor_property("g", g); m.set_editor_property("b", b); m.set_editor_property("a", False)
    mel.connect_material_expressions(node, "", m, "")
    return m


ext = channel(True, False, False, 300)
glow = channel(False, True, False, 500)
mask = channel(False, False, True, 700)
# The front is darker, bluer cloud than the fair-weather deck.
base = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, 100)
mel.connect_material_expressions(albedo, "", base, "A")
mel.connect_material_expressions(storm, "", base, "B")
mel.connect_material_expressions(mask, "", base, "Alpha")
mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
# In the volume domain the Subsurface Color pin is the extinction.
mel.connect_material_property(ext, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
glow_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 600)
mel.connect_material_expressions(glow, "", glow_mul, "A")
mel.connect_material_expressions(flash, "", glow_mul, "B")
mel.connect_material_property(glow_mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Scattering: mostly forward (bright rims against the light), less of the soft multiple-scatter fill
# than the engine default so the hollows between the heads stay dark.
try:
    adv = mel.create_material_expression(mat, unreal.MaterialExpressionVolumetricAdvancedMaterialOutput, 0, 900)
    for prop, value in [("const_phase_g", 0.55), ("const_phase_g2", -0.3), ("const_phase_blend", 0.35),
                        ("const_multi_scattering_contribution", 0.4), ("const_multi_scattering_occlusion", 0.5),
                        ("const_multi_scattering_eccentricity", 0.5), ("multi_scattering_approximation_octave_count", 1),
                        ("ground_contribution", False), ("gray_scale_material", False), ("ray_march_volume_shadow", True)]:
        try:
            adv.set_editor_property(prop, value)
        except Exception as e:
            unreal.log_warning("DC_RESULT advanced output: %s not set (%s)" % (prop, e))
except Exception as e:
    unreal.log_warning("DC_RESULT no advanced output node (%s)" % e)

mel.recompile_material(mat)
ok = unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
unreal.log_warning("DC_RESULT M_StormCloud saved=%s" % ok)
