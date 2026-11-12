"""Lists static meshes (with size in metres) and blueprints in the Fab packs. Run: Tools/py.sh Tools/list_pack_meshes.py 400"""
import unreal
reg = unreal.AssetRegistryHelpers.get_asset_registry()
out = []
for a in reg.get_assets_by_path("/Game/AncientRuins", recursive=True):
    cls = str(a.asset_class_path.asset_name)
    if cls == "StaticMesh":
        m = a.get_asset()
        b = m.get_bounding_box()
        s = b.max - b.min
        out.append("M %s %.1f x %.1f x %.1f m  tris=%d mats=%d" % (str(a.package_name).replace("/Game/AncientRuins/", ""), s.x / 100, s.y / 100, s.z / 100, m.get_num_triangles(0), len(m.static_materials)))
    elif cls == "Blueprint":
        out.append("B %s" % str(a.package_name).replace("/Game/AncientRuins/", ""))
with open(unreal.Paths.project_saved_dir() + "ruins_assets.txt", "w") as f:
    f.write("\n".join(sorted(out)))
unreal.log_warning("DC_RESULT listed %d" % len(out))
