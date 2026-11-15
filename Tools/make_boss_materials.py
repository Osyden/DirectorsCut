"""Original boss-only tells and spray; writes no arena or purchased assets."""
import unreal
M = unreal.MaterialEditingLibrary
ROOT = '/Game/DC/Boss'

def make(name):
    a = unreal.load_asset(ROOT + '/' + name)
    if a:
        M.delete_all_material_expressions(a)
    else:
        a = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    return a

def scalar(a, value, name=None):
    n = M.create_material_expression(a, unreal.MaterialExpressionScalarParameter if name else unreal.MaterialExpressionConstant)
    n.set_editor_property('default_value' if name else 'r', value)
    if name:
        n.set_editor_property('parameter_name', name)
    return n

def color(a, value):
    n = M.create_material_expression(a, unreal.MaterialExpressionVectorParameter)
    n.set_editor_property('parameter_name', 'Color')
    n.set_editor_property('default_value', unreal.LinearColor(*value))
    return n

a = make('M_BossTell')
c = color(a, (1.2, .28, .07, 1))
M.connect_material_property(c, '', unreal.MaterialProperty.MP_BASE_COLOR)
M.connect_material_property(c, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
M.connect_material_property(scalar(a, .65), '', unreal.MaterialProperty.MP_ROUGHNESS)
M.recompile_material(a)
unreal.EditorAssetLibrary.save_loaded_asset(a)

a = make('M_BossSkin')
uv = M.create_material_expression(a, unreal.MaterialExpressionTextureCoordinate)
def skin_node(code):
    n = M.create_material_expression(a, unreal.MaterialExpressionCustom)
    n.set_editor_property('code', code)
    n.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    pin = unreal.CustomInput()
    pin.set_editor_property('input_name', 'UV')
    n.set_editor_property('inputs', [pin])
    M.connect_material_expressions(uv, '', n, 'UV')
    return n

# Original procedural skin: warm dorsal ridges and two pale rows of cups.
# No purchased texture is copied or baked into this material.
pattern = '''
float filter=1-saturate(max(fwidth(UV.x*173),fwidth(UV.y*139)));
float pores=.5+.5*sin(UV.x*173)*sin(UV.y*139)*filter;
float belly=pow(saturate(sin(UV.y*6.28318)),3);
float row=min(abs(UV.y-.19),abs(UV.y-.31));
float along=(frac(UV.x*2)-.5)*2;
float cup=length(float2(along,row*65));
float rim=exp(-pow(abs((cup-.62)*6),2))*belly;
float hollow=exp(-cup*cup*9)*belly;
'''
c = skin_node(pattern + 'return lerp(float3(.16,.065,.035),float3(.48,.29,.18),belly)*(.75+pores*.25)+rim*float3(.23,.14,.08)-hollow*.10;')
M.connect_material_property(c, '', unreal.MaterialProperty.MP_BASE_COLOR)
n = skin_node(pattern + 'float sy=abs(UV.y-.19)<abs(UV.y-.31)?(UV.y-.19)*65:(UV.y-.31)*65; float g=-12*(cup-.62)*rim; return normalize(float3(-g*along/max(cup,.01)*.15,-g*sy/max(cup,.01)*.15,1));')
M.connect_material_property(n, '', unreal.MaterialProperty.MP_NORMAL)
M.connect_material_property(scalar(a, .38), '', unreal.MaterialProperty.MP_ROUGHNESS)
M.connect_material_property(scalar(a, .35), '', unreal.MaterialProperty.MP_SPECULAR)
M.recompile_material(a)
unreal.EditorAssetLibrary.save_loaded_asset(a)

unreal.log_warning('DC_BOSS_MATERIALS_COMPLETE')
