"""Import only the approved landing composite; run with root's editor slot.
Uses the current editor module; the LandingImpact C++ patch is not needed to import.
Source WAV and cooked asset remain local because the thunder layer is licensed Sonniss.
"""
from pathlib import Path
import hashlib
import json
import wave
import unreal

root = Path(unreal.Paths.project_dir())
source = root / "Saved/AudioImport/landing_thunderous.wav"
expected = "1902c96ad5cb935d038ec79b6598f6ecf2187f42a08f24e54482f2618ce87bfc"
assert hashlib.sha256(source.read_bytes()).hexdigest() == expected, "Approved WAV hash mismatch"
with wave.open(str(source)) as wav:
    assert (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) == (48000, 2, 2)
    assert abs(wav.getnframes() / wav.getframerate() - 4.4) < .01

task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = "/Game/Sonniss"
task.destination_name = "landing_thunderous"
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
path = "/Game/Sonniss/landing_thunderous"
sound = unreal.EditorAssetLibrary.load_asset(path)
assert isinstance(sound, unreal.SoundWave), "Landing SoundWave import failed"
sound.set_editor_property("looping", False)
sound.set_editor_property("volume", 1.0)
sound.set_editor_property("pitch", 1.0)
sound.set_editor_property("compression_quality", 90)
unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
duration = sound.get_editor_property("duration")
channels = sound.get_editor_property("num_channels")
assert channels == 2 and abs(duration - 4.4) < .01
result = dict(asset=path, source_sha256=expected, duration=duration, channels=channels,
              looping=sound.get_editor_property("looping"), gain_in_code=.85, pitch_in_code=1.0)
(root / "Saved/AudioImport/landing-import-validation.json").write_text(json.dumps(result, indent=2))
unreal.log_warning("LANDING_IMPORTED " + json.dumps(result))
