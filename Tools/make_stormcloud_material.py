"""Builds /Game/DC/Materials/M_StormCloud: the material for the volumetric cloud layer.

Run with the editor closed:  Tools/py.sh Tools/make_stormcloud_material.py

One ray-marched volume holds both the sky's cloud deck and the storm front, so both are lit and
shadowed by the sun and by themselves. The ride director sets the parameters every frame.

Parameters
  Sky     x = cover 0..1, y = cloud base (cm above the sea), z = cloud top (cm), w = extinction scale
  Front   x = world X of the face of the storm front (cm), y = strength 0..1 (0 = no front),
          z = radius of the tunnel mouth bored into it along the ship's course (cm, 0 = none),
          w = lightning 0..1 (lights the mouth and the inside of the wall)
  Mist    x = amount of sea mist 0..1, y = height of its top (cm), z = how thick it is next to full cloud
  Portal  xy = where the portal out of Atlantis stands (cm), z = radius of its tunnel (cm), w = strength 0..1
  Face0..2  xyz = centre of a face in the clouds (cm), w = its size (half the height of the head, cm; 0 = none)
  Mist.w  = how solid the faces are, 0..1
"""
import unreal

PKG = "/Game/DC/Materials"
NAME = "M_StormCloud"
mel = unreal.MaterialEditingLibrary

DENSITY = """
// All the shapes come from one engine noise volume read at several scales. One tile of it holds
// about eight cells across, so a scale of 0.006 per km gives masses some 20 km across. Its red channel sits
// in a narrow band (measured: about 0.83 to 0.96, half of it above 0.895), so it is stretched to 0..1
// here, which puts the median near 0.5.
#define NZ(p) saturate((Texture3DSampleLevel(Shape, ShapeSampler, (p), 0.0).r - 0.835) / 0.125)
float alt = P.z;

// Most of the volume is empty air: find that out before paying for any noise.
float h = (alt - Sky.y) / max(Sky.z - Sky.y, 1.0);
bool inDeck = h > 0.0 && h < 1.0 && Sky.x > 0.01;
bool inMist = Mist.x > 0.001 && alt < Mist.y;
bool inFace = Mist.w > 0.001 && (Face0.w > 1.0 || Face1.w > 1.0 || Face2.w > 1.0) && alt < 400000.0;
bool inPortal = Portal.w > 0.001 && P.x > Portal.x - Portal.z * 2.5 && P.x < Portal.x + Portal.z * 7.0 && abs(P.y - Portal.y) < Portal.z * 6.5 && alt < Portal.z * 6.5;
// The front leans out over the ship higher up, like a breaking wave, and its foot runs ahead of it
// along the water as a skirt of rain and spray.
float lean = 50000.0 * smoothstep(0.0, 1.0, alt / 450000.0) + 60000.0 * exp(-alt / 15000.0);
float face = P.x - Front.x + lean;
bool inWall = Front.y > 0.001 && face > -260000.0 && face < 640000.0 && alt < 900000.0;
if (!inDeck && !inWall && !inMist && !inPortal && !inFace) { return float4(0.0, 0.0, 0.0, 0.0); }

float3 km = P * 0.00001;
float3 wind = float3(T * 0.0002, T * 0.00008, 0.0);
float deck = 0.0;
float wall = 0.0;
float glow = 0.0;
if (inDeck)
{
    // The deck: flat-bottomed, heaped on top, as much of the sky as Sky.x says. Masses 10 km and
    // more across carry heads of 3 km and 800 m, and the thin parts are eaten into wisps.
    float big = NZ(km * 0.007 + wind);
    float mid = NZ(km * 0.026 + wind * 3.0 + 0.37);
    float det = NZ(km * 0.1 + wind * 9.0 + 0.71);
    float n = big * 0.55 + mid * 0.3 + det * 0.15;
    float need = lerp(0.72, 0.2, Sky.x) + 0.12 * h;
    deck = saturate((n - need) * 5.0) * saturate(h * 9.0) * saturate((1.0 - h) * 3.0);
    float fine = NZ(km * 0.4 - wind * 20.0);
    deck = saturate(deck - (0.15 + fine * 0.7) * (1.0 - deck) * 0.8);
}
if (inWall)
{
    // The front. Its noise travels with it, or the face would boil as it came on; it turns over
    // slowly instead, rising up the face.
    float3 q = float3(P.x - Front.x, P.y, alt) * 0.00001;
    float churn = T * 0.0006;
    float big = NZ(q * 0.006 + float3(0.0, 0.0, -churn));
    float mid = NZ(q * 0.022 + float3(churn, 0.0, -churn * 3.0) + 0.21);
    float det = NZ(q * 0.085 + float3(0.0, churn * 2.0, -churn * 8.0) + 0.63);
    float fine = NZ(q * 0.32 + float3(churn * 5.0, 0.0, -churn * 20.0));
    // How far inside the surface of the cloud this point is, cm. Towers kilometres across, heads
    // of a kilometre on them, knuckles of a few hundred metres on those.
    float surf = face + (big - 0.5) * 220000.0 + (mid - 0.5) * 80000.0 + (det - 0.5) * 24000.0;
    float bore = 1.0;
    if (Front.z > 1.0)
    {
        // The mouth: a tunnel along the ship's course, narrowing as it goes in and closing in the
        // dark a kilometre and a half inside. The face of the front is drawn back into it like a
        // funnel, and spiral arms of cloud wind down into the hole.
        float R = Front.z * lerp(1.0, 0.4, saturate((face - 60000.0) / 200000.0));
        float2 m = float2(P.y, alt - Front.z * 0.4);
        float rr = length(m) / R;
        float near = saturate(1.0 - (rr - 1.0) / 4.0);
        float ang = atan2(m.y, m.x);
        float spiral = sin(ang * 3.0 + log(max(rr, 0.3)) * 6.0 + T * 0.35 - q.x * 3.0);
        float arms = spiral * 0.6 + 0.4 * sin(ang * 5.0 - log(max(rr, 0.3)) * 9.0 - T * 0.2);
        surf += (arms * 80000.0 - near * near * 50000.0) * near;
        float edge = rr - 1.0 - 0.22 * spiral - (det - 0.5) * 0.5;
        bore = lerp(saturate(edge / 0.3), 1.0, saturate((face - 330000.0) / 120000.0));
        // Cold light in the walls of the tunnel: faint, and bright with each stroke of lightning.
        glow = (0.03 + 0.15 * Front.w) * exp(-max(edge, 0.0) * 5.0) * saturate(face / 60000.0 + 0.5) * (0.5 + 0.5 * arms);
    }
    float topAlt = 300000.0 + big * 380000.0 + (mid - 0.5) * 150000.0;
    // Density builds up over half a kilometre in from the surface. A hard-edged cloud renders flat
    // and unlit: the marcher's first step inside is already too deep for the sun to reach.
    float body = smoothstep(0.0, 1.0, saturate(surf / 20000.0));
    // Behind the towers the front is solid, so the bays between them are dark cloud, not sky.
    body = max(body, smoothstep(0.0, 1.0, saturate((face - 110000.0 + (mid - 0.5) * 60000.0) / 90000.0)));
    // A veil ahead of the solid face: thin scud and rain that the towers stand behind.
    float veil = saturate(1.0 + surf / 35000.0) * 0.1 * fine * fine * (1.0 - body);
    wall = (body + veil) * saturate((topAlt - alt) / 60000.0) * saturate((600000.0 - face) / 120000.0) * bore * Front.y;
    wall = saturate(wall - fine * 0.5 * (1.0 - wall));
    // Sheet lightning inside the cloud: now and then one stretch of the wall lights from within.
    float cell = floor(P.y / 500000.0 + 0.37);
    float slot = floor(T / 0.6 + cell * 0.37);
    float pick = frac(sin(cell * 12.9898 + slot * 78.233) * 43758.5453);
    float flick = frac(sin(floor(T * 14.0) * 3.17 + cell) * 9631.7);
    if (pick > 0.9 && flick > 0.35)
    {
        float3 c = float3(70000.0, (cell + 0.13 + 0.5 * (frac(pick * 37.0) - 0.5)) * 500000.0, 90000.0 + 260000.0 * frac(pick * 91.0));
        float3 dd = float3(face, P.y, alt) - c;
        // Faded out toward the edge of its stretch, so the light never ends in a straight line.
        float within = saturate((250000.0 - abs(P.y - (cell + 0.13) * 500000.0)) / 150000.0);
        glow += 0.05 * within / (1.0 + dot(dd, dd) / (30000.0 * 30000.0));
    }
}
if (inPortal)
{
    // The way out of Atlantis: a standing ring of storm cloud on the sea with a tunnel through it,
    // spiral arms winding into the hole, and light at the far end.
    float R = Portal.z;
    float pf = P.x - Portal.x;
    float2 m = float2(P.y - Portal.y, alt - R * 0.4);
    float rr = length(m) / R;
    float ang = atan2(m.y, m.x);
    float lr = log(max(rr, 0.3));
    float spiral = sin(ang * 3.0 + lr * 6.0 + T * 0.5 - pf / R * 1.3);
    float arms = spiral * 0.6 + 0.4 * sin(ang * 5.0 - lr * 9.0 - T * 0.3);
    float lump = NZ(float3(pf, m.x, m.y) * 0.0000012 + 0.4);
    float fray = NZ(float3(pf, m.x, m.y) * 0.000006 + float3(T * 0.004, 0.0, 0.0));
    // Thick near the rim of the hole, thinning outward into torn arms.
    float ring = saturate(1.0 - (rr - 1.0) / (2.6 + 1.2 * arms + 1.5 * (lump - 0.5)));
    float hole = saturate((rr - 1.0 - 0.2 * spiral - 0.3 * (lump - 0.5)) / 0.3);
    float along = saturate((pf + R * 2.5) / (R * 1.5)) * saturate((R * 7.0 - pf) / (R * 3.0));
    float pd = ring * hole * along * Portal.w;
    pd = saturate(pd * 1.6 - fray * 0.6 * (1.0 - pd));
    // The far end of the tunnel is closed by bright cloud: the light the ship steers for.
    float core = exp(-rr * rr * 1.5) * saturate((pf - R * 3.0) / (R * 2.0)) * saturate((R * 7.0 - pf) / R) * Portal.w;
    pd = max(pd, core * 0.5);
    float rim = exp(-max(rr - 1.0 - 0.2 * spiral, 0.0) * 3.0);
    glow = max(glow, (0.05 + 0.35 * Front.w) * rim * (0.5 + 0.5 * arms) * along + core * 0.9);
    wall = max(wall, pd);
}
if (inFace)
{
    // Faces in the cloud: heads half a kilometre tall, modelled as cloud. Each looks toward the
    // ship and a little down at it. Dim until lightning lights them from inside.
    float4 FaceSet[3] = { Face0, Face1, Face2 };
    for (int fi = 0; fi < 3; fi++)
    {
        float S = FaceSet[fi].w;
        if (S < 1.0) { continue; }
        float3 rel = float3(P.x, P.y, alt) - FaceSet[fi].xyz;
        if (dot(rel, rel) > S * S * 7.3) { continue; }
        float2 toShip = normalize(-FaceSet[fi].xy);
        float3 fwd = normalize(float3(toShip * 0.96, -0.28));
        float3 rgt = float3(-toShip.y, toShip.x, 0.0);
        float3 up = cross(fwd, rgt);
        float3 p = float3(dot(rel, fwd), dot(rel, rgt), dot(rel, up)) / S;
        float sy = abs(p.y);
        #define ELL(c, r) (1.0 - length((float3(p.x, sy, p.z) - (c)) / (r)))
        float k = ELL(float3(-0.10, 0.0, 0.15), float3(0.75, 0.68, 0.90));           // skull
        k = max(k, ELL(float3(0.08, 0.0, -0.45), float3(0.55, 0.50, 0.50)));         // jaw
        k = max(k, ELL(float3(0.52, 0.0, 0.30), float3(0.22, 0.62, 0.13)));          // brow
        k = max(k, ELL(float3(0.66, 0.0, -0.02), float3(0.18, 0.12, 0.30)));         // nose
        k = max(k, ELL(float3(0.44, 0.36, -0.10), float3(0.26, 0.22, 0.26)));        // cheekbones
        k = max(k, ELL(float3(0.50, 0.0, -0.70), float3(0.20, 0.24, 0.18)));         // chin
        k = max(k, ELL(float3(0.58, 0.0, -0.40), float3(0.13, 0.26, 0.09)));         // lips
        float fd = saturate(k * 5.0);
        fd *= 1.0 - saturate(ELL(float3(0.66, 0.30, 0.12), float3(0.22, 0.21, 0.14)) * 6.0);   // eye sockets
        fd *= 1.0 - saturate(ELL(float3(0.70, 0.0, -0.47), float3(0.16, 0.24, 0.06)) * 6.0);   // parted mouth
        // Shadowed planes: under the brow, beside the nose, under the cheekbones, the mouth.
        float hollow = saturate(ELL(float3(0.60, 0.30, 0.12), float3(0.3, 0.26, 0.19)) * 3.0);
        hollow = max(hollow, saturate(ELL(float3(0.60, 0.0, -0.47), float3(0.3, 0.3, 0.1)) * 3.0));
        hollow = max(hollow, saturate(ELL(float3(0.45, 0.38, -0.42), float3(0.3, 0.16, 0.2)) * 3.0));
        hollow = max(hollow, saturate(ELL(float3(0.62, 0.17, -0.12), float3(0.2, 0.07, 0.2)) * 3.0));
        // Hair and shoulders: torn cloud streaming back and down from the head.
        float tear = NZ(rel * 0.0000035 + float3(T * 0.002, 0.0, 0.0));
        float hair = ELL(float3(-0.55, 0.0, 0.25), float3(1.25, 1.15, 1.25)) + (tear - 0.5) * 0.9;
        float neck = ELL(float3(-0.35, 0.0, -1.45), float3(1.5, 1.9, 0.9)) + (tear - 0.5) * 0.9;
        float soft = saturate(max(hair, neck) * 2.5) * saturate(0.55 - p.x) * 0.7;
        float eye = saturate(ELL(float3(0.57, 0.30, 0.13), float3(0.06, 0.08, 0.05)) * 8.0);
        fd = max(max(fd, soft), eye);
        fd = saturate(fd - NZ(rel * 0.000012) * 0.35 * (1.0 - fd));
        if (fd > 0.0)
        {
            // Lit from inside by the storm; the eyes burn cold all the time.
            glow = max(glow, (0.03 + 0.08 * Front.w) * saturate(k * 3.0 + 0.3) * (1.0 - soft) * (1.0 - 0.9 * hollow) + eye * 0.6);
            wall = max(wall, fd * Mist.w);
        }
    }
}
float d = max(deck, wall);
if (inMist)
{
    // Sea mist: torn banks lying on the water, thick in places and open in others, so that things
    // stand in it at different depths instead of fading evenly into haze.
    float mh = saturate(1.0 - alt / max(Mist.y, 1.0));
    float mn = NZ(km * 0.06 + wind * 6.0 + 0.11) * 0.6 + NZ(km * 0.2 - wind * 15.0 + 0.47) * 0.4;
    float bank = saturate((mn - (0.8 - Mist.x * 0.5)) * 3.5) * mh * (0.35 + 0.65 * mh);
    bank = saturate(bank - NZ(km * 0.65 + wind * 40.0) * 0.5 * (1.0 - bank));
    d = max(d, bank * Mist.z);
}
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
mist = vector(mat, "Mist", (0, 40000, 0.35, 0), -1400, 1000)
portal = vector(mat, "Portal", (0, 0, 21000, 0), -1400, 1050)
faces = [vector(mat, "Face%d" % i, (0, 0, 0, 0), -1400, 1600 + i * 120) for i in range(3)]
albedo = colour(mat, "Albedo", (0.9, 0.9, 0.9, 1), -1400, 1100)
storm = colour(mat, "StormAlbedo", (0.4, 0.43, 0.5, 1), -1400, 1300)
flash = colour(mat, "FlashColor", (0.16, 0.22, 0.4, 1), -1400, 1500)

node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -800, 300)
node.set_editor_property("code", DENSITY)
node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
inputs = []
for n in ["P", "T", "Shape", "Sky", "Front", "Mist", "Portal", "Face0", "Face1", "Face2"]:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
node.set_editor_property("inputs", inputs)
shape = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -1400, 300)
shape.set_editor_property("texture", unreal.load_asset("/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128"))
shape.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
mel.connect_material_expressions(shape, "", node, "Shape")
mel.connect_material_expressions(wp, "", node, "P")
mel.connect_material_expressions(time, "", node, "T")
mel.connect_material_expressions(sky, "", node, "Sky")
mel.connect_material_expressions(front, "", node, "Front")
mel.connect_material_expressions(mist, "", node, "Mist")
mel.connect_material_expressions(portal, "", node, "Portal")
for i in range(3):
    mel.connect_material_expressions(faces[i], "", node, "Face%d" % i)


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
