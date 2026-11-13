"""Lists the water plugin's blueprints/data assets that deal with shorelines, depth foam and interaction. Run: Tools/py.sh Tools/inspect_drw_shore.py 200"""
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.scan_paths_synchronous(["/DynamicRealWater"], True)
for path in ["/DynamicRealWater/Blueprints", "/DynamicRealWater/Materials/DrawMaterialRT"]:
    for a in reg.get_assets_by_path(path, recursive=True):
        unreal.log_warning("DC_RESULT asset %s %s (%s)" % (a.package_path, a.asset_name, a.asset_class_path.asset_name))
def dump(obj, tag):
    cls = obj.get_class()
    names = set()
    for n in dir(obj):
        if n.startswith("_"): continue
        low = n.lower()
        if any(k in low for k in ["shore", "foam", "depth", "interact", "splash", "coast", "ripple", "capture", "terrain", "landscape"]):
            try:
                v = obj.get_editor_property(n)
                unreal.log_warning("DC_RESULT %s .%s = %s" % (tag, n, str(v)[:120]))
            except Exception:
                unreal.log_warning("DC_RESULT %s fn/attr %s" % (tag, n))
for a in reg.get_assets_by_path("/DynamicRealWater/Blueprints", recursive=True):
    asset = a.get_asset()
    if isinstance(asset, unreal.Blueprint):
        gc = unreal.load_object(None, str(a.package_name) + "." + str(a.asset_name) + "_C")
        if gc:
            dump(unreal.get_default_object(gc), str(a.asset_name))
    else:
        dump(asset, str(a.asset_name))
