"""Original boss-only tells and spray; writes no arena or purchased assets."""
import unreal
M = unreal.MaterialEditingLibrary
ROOT = '/Game/DC/Boss'

def make(name):
    a = unreal.load_asset(ROOT + '/' + name)
    if a:
        for n in list(M.get_material_expressions(a)):
            M.delete_material_expression(a, n)
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

unreal.log_warning('DC_BOSS_MATERIALS_COMPLETE')
