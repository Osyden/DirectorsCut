"""Imports the prepared recordings into /Game/Sonniss as sound waves.

Run with the editor closed:  Tools/py.sh Tools/import_audio.py
Source: E:/Projects/DirectorsCut/Downloads/Sonniss-GDC2026/Picked (made by Tools/wavprep.ps1 from the
Sonniss GDC 2026 bundle; see taskboard/AUDIO-SHORTLIST.md). Content/Sonniss is git-ignored: the
licence allows use in the game but not sharing the sounds as assets.
"""
import os
import unreal

SRC = "E:/Projects/DirectorsCut/Downloads/Sonniss-GDC2026/Picked"
DEST = "/Game/Sonniss"
LOOPS = {"sea_calm", "sea_rough", "wind_storm", "rain_storm", "fire_loop"}

tasks = []
for name in sorted(os.listdir(SRC)):
    if not name.lower().endswith(".wav"):
        continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + "/" + name)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

count = 0
for t in tasks:
    base = os.path.splitext(os.path.basename(t.get_editor_property("filename")))[0]
    path = DEST + "/" + base
    wave = unreal.EditorAssetLibrary.load_asset(path)
    if not wave:
        unreal.log_warning("DC_RESULT import failed: %s" % base)
        continue
    wave.set_editor_property("looping", base in LOOPS)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    count += 1
unreal.log_warning("DC_RESULT imported %d sounds into %s" % (count, DEST))
