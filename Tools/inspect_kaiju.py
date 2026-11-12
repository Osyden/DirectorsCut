"""Reports what the Claw Kaiju import produced. Run: Tools/py.sh Tools/inspect_kaiju.py 80"""
import unreal
D = "/Game/Fab/Claw_Kaiju_Monster"
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for a in sorted(reg.get_assets_by_path(D, recursive=True), key=lambda x: str(x.asset_name)):
    cls = str(a.asset_class_path.asset_name)
    name = str(a.asset_name)
    info = ""
    o = a.get_asset()
    try:
        if cls == "SkeletalMesh":
            b = o.get_bounds()
            mats = [(str(m.material_slot_name), m.material_interface.get_name() if m.material_interface else "NONE") for m in o.materials]
            info = "extent(m)=%.1f,%.1f,%.1f origin=%.1f,%.1f,%.1f mats=%s skeleton=%s physics=%s" % (b.box_extent.x / 100, b.box_extent.y / 100, b.box_extent.z / 100, b.origin.x / 100, b.origin.y / 100, b.origin.z / 100, mats, o.skeleton.get_name() if o.skeleton else "NONE", o.physics_asset.get_name() if o.physics_asset else "NONE")
        elif cls == "AnimSequence":
            info = "length=%.2fs skeleton=%s" % (o.get_play_length(), o.get_skeleton().get_name() if o.get_skeleton() else "NONE")
        elif cls in ("Texture2D",):
            info = "%dx%d srgb=%s comp=%s vt=%s" % (o.blueprint_get_size_x(), o.blueprint_get_size_y(), o.srgb, str(o.compression_settings).split(".")[-1].split(":")[0], o.virtual_texture_streaming)
        elif cls in ("Material", "MaterialInstanceConstant"):
            if cls == "Material":
                n = unreal.MaterialEditingLibrary.get_num_material_expressions(o)
                tex = [t.get_name() for t in unreal.MaterialEditingLibrary.get_used_textures(o)]
                info = "expressions=%d blend=%s textures=%s" % (n, str(o.blend_mode).split(".")[-1].split(":")[0], tex)
            else:
                info = "parent=%s" % (o.parent.get_name() if o.parent else "NONE")
    except Exception as e:
        info = "ERR %s" % e
    unreal.log_warning("DC_RESULT %s | %s | %s" % (cls, name, info))
