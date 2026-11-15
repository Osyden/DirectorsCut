"""Import prepared stereo WAVs from Saved/MusicImport into the cooked score.

Prepare with Tools/prepare_music.py first. Source MP3s stay in ../Music;
imported assets stay local, matching the project's other licensed recordings.
Run this script with Unreal's Python commandlet after closing the editor.
"""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
for name in ("TheFog", "TheCaldera"):
    source = root / "Saved/MusicImport" / (name + ".wav")
    if not source.is_file():
        raise RuntimeError("Missing prepared music: " + str(source))
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = "/Game/DC/Music"
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    path = "/Game/DC/Music/" + name
    wave = unreal.EditorAssetLibrary.load_asset(path)
    if not wave:
        raise RuntimeError("Music import failed: " + name)
    wave.set_editor_property("looping", False)
    wave.set_editor_property("sound_group", unreal.SoundGroup.SOUNDGROUP_MUSIC)
    wave.set_editor_property("virtualization_mode", unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    wave.set_editor_property("compression_quality", 70)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log_warning("MUSIC_IMPORTED %s duration=%.3f channels=%d" %
                       (name, wave.get_editor_property("duration"), wave.get_editor_property("num_channels")))
