"""Deck-only fire materials. Leaves the shared M_Flame and vendor assets intact.

Uses the already installed Paragon fire textures and Brigantine smoke texture.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
The /Game/DC materials retain hard texture references for cooking.
"""
import unreal

ROOT = '/Game/DC/Materials'
MEL = unreal.MaterialEditingLibrary


def connect(source, output, dest, pin):
    if not MEL.connect_material_expressions(source, output, dest, pin):
        raise RuntimeError('Material connection failed: '+dest.get_class().get_name()+' '+pin+'; inputs='+str(MEL.get_material_expression_input_names(dest)))


def node(mat, cls):
    return MEL.create_material_expression(mat, cls)


def scalar(mat, name, value):
    n = node(mat, unreal.MaterialExpressionScalarParameter)
    n.set_editor_property('parameter_name', name)
    n.set_editor_property('default_value', value)
    return n


def texture(mat, path):
    tex = unreal.load_asset(path)
    if not tex:
        raise RuntimeError('Required installed texture missing: ' + path)
    n = node(mat, unreal.MaterialExpressionTextureObject)
    n.set_editor_property('texture', tex)
    return n


def custom(mat, code, inputs):
    n = node(mat, unreal.MaterialExpressionCustom)
    n.set_editor_property('code', code)
    n.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', name)
        pins.append(pin)
    n.set_editor_property('inputs', pins)
    for name, source in inputs.items():
        connect(source, '', n, name)
    return n


def mask(mat, source, channels):
    n = node(mat, unreal.MaterialExpressionComponentMask)
    for ch in 'rgba':
        n.set_editor_property(ch, ch in channels)
    connect(source, '', n, '')
    return n


FIRE = r'''
float h = saturate(UV.y);
float seed = VC.r * 31.7;
float t = Clock * (0.8 + VC.g * 0.5);
float2 q = float2(UV.x * .65 + seed, h * .48 - t * .75);
float warp = Texture2DSample(Noise, NoiseSampler, q * .72 + float2(0, t*.15)).r;
float2 flow = q + float2((warp-.4)*.22*h, 0);
float fine = Texture2DSample(Wisps, WispsSampler, flow).r;
float broad = Texture2DSample(Noise, NoiseSampler, q * .55).r;
float curl = sin(h*8.0-t*3.2+seed)*.15*h + (warp-.45)*.48*h;
float x = abs((UV.x-.5)*2.0 + curl);
float width = lerp(.95, .08, pow(h,.8));
float torn = width-x + (broad-.35)*.44 + fine*.22;
float body = smoothstep(-.07,.20,torn);
float top = 1.0-smoothstep(.76,.99,h + (broad-.4)*.16);
float opacity = body*top*smoothstep(0,.035,h)*saturate(.22+fine*2.8+broad*.35);
float heat = saturate(fine*.8 + broad*.35 + (1-h)*.38);
float3 color = lerp(float3(.65,.025,.001), float3(2.0,.38,.018), heat);
color = lerp(color, float3(3.0,1.1,.18), pow(heat,5.0)*(1-h));
return float4(color * Energy, opacity * VC.a * .88);
'''

SMOKE = r'''
float2 p = UV*2-1;
float radial = 1-smoothstep(.28,1.0,length(p));
float angle = VC.r*6.283 + Clock*.055;
float2 q = float2(p.x*cos(angle)-p.y*sin(angle),p.x*sin(angle)+p.y*cos(angle))*.36+.5;
float n = Texture2DSample(Smoke, SmokeSampler, q + VC.gb*.2).r;
float density = saturate(n*1.8-.22)*radial;
return float4(lerp(float3(.065,.055,.048),float3(.19,.18,.17),n), density*VC.a*.34);
'''

EMBER = r'''
float2 p = (UV-.5)*2;
float a = pow(saturate(1-dot(p,p)),1.6)*VC.a;
return float4(float3(3.0,.38,.012)*Energy,a);
'''

for name, code, lit in [('M_DeckFire', FIRE, False), ('M_DeckSmoke', SMOKE, True), ('M_DeckEmber', EMBER, False)]:
    path = ROOT+'/'+name
    mat = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if mat:
        MEL.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT if lit else unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('enable_responsive_aa', True)
    mat.set_editor_property('compute_fog_per_pixel', True)
    if lit:
        mat.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate)
    vc = node(mat, unreal.MaterialExpressionVertexColor)
    vc4 = node(mat, unreal.MaterialExpressionAppendVector)
    connect(vc, '', vc4, 'A')
    connect(vc, 'A', vc4, 'B')
    inputs = {'UV': uv, 'VC': vc4, 'Clock': scalar(mat, 'Clock', 0), 'Energy': scalar(mat, 'Energy', 1)}
    if name == 'M_DeckFire':
        inputs['Noise'] = texture(mat, '/Game/ParagonRampage/FX/Textures/Fire/T_FireNoiseTile_02')
        inputs['Wisps'] = texture(mat, '/Game/ParagonRampage/FX/Textures/Fire/T_Fire_SmokeWisps_Tile')
    if lit:
        inputs['Smoke'] = texture(mat, '/Game/Brigantine/Texture/Particle-smoke/T_Smoke')
    result = custom(mat, code, inputs)
    rgb = mask(mat, result, 'rgb')
    alpha = mask(mat, result, 'a')
    fade = node(mat, unreal.MaterialExpressionDepthFade)
    fade.set_editor_property('fade_distance_default', 20 if lit else 5)
    connect(alpha, '', fade, str(MEL.get_material_expression_input_names(fade)[0]))
    MEL.connect_material_property(fade, '', unreal.MaterialProperty.MP_OPACITY)
    if lit:
        MEL.connect_material_property(rgb, '', unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        exposure = node(mat, unreal.MaterialExpressionEyeAdaptationInverse)
        names = MEL.get_material_expression_input_names(exposure)
        if len(names) != 2 or not MEL.connect_material_expressions(rgb, '', exposure, str(names[0])):
            raise RuntimeError('Cannot connect inverse-exposure light input: '+str(names))
        if not MEL.connect_material_expressions(scalar(mat, 'ExposureCompensation', .8), '', exposure, str(names[1])):
            raise RuntimeError('Cannot connect inverse-exposure weight input')
        MEL.connect_material_property(exposure, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.layout_material_expressions(mat)
    MEL.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError('Could not save '+path)
    unreal.log_warning('DC_FIRE_MATERIAL saved '+path)
