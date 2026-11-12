"""Dumps the editable variables of Ultra Dynamic Sky / Weather to Saved/uds_vars.txt. Run: Tools/py.sh Tools/inspect_uds.py"""
import unreal
lines = []
for name in ["Ultra_Dynamic_Sky", "Ultra_Dynamic_Weather"]:
    cls = unreal.load_object(None, "/Game/UltraDynamicSky/Blueprints/%s.%s_C" % (name, name))
    cdo = unreal.get_default_object(cls)
    lines.append("== %s" % name)
    for p in sorted(dir(cdo)):
        if p.startswith("_"):
            continue
        try:
            v = cdo.get_editor_property(p)
        except Exception:
            continue
        s = str(v)
        if len(s) > 90:
            s = s[:90]
        lines.append("%s = %s" % (p, s))
with open(unreal.Paths.project_saved_dir() + "uds_vars.txt", "w") as f:
    f.write("\n".join(lines))
unreal.log_warning("DC_RESULT uds vars %d" % len(lines))
