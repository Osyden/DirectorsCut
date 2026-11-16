"""Import only the approved whoosh. Root owns the serialized Unreal commandlet lane."""
from pathlib import Path
import hashlib
import json
import wave
import unreal

root = Path(unreal.Paths.project_dir())
source = root/'Saved/AudioImport/fireball_flame_whoosh.wav'
manifest = json.loads((root/'Saved/AudioImport/fireball-whoosh-provenance.json').read_text(encoding='utf-8'))
assert manifest['source_sha256'] == '344c8a9a31205fd5842feea827a491d89528f7db1ee5b1ffd4a3a018e23395ef'
assert manifest['filter'] == 'loudnorm=I=-18:TP=-3:LRA=7'
assert hashlib.sha256(source.read_bytes()).hexdigest() == manifest['wav_sha256'], 'Prepared whoosh changed'
with wave.open(str(source)) as wav:
    assert (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) == (48000, 2, 2)
    assert abs(wav.getnframes()/wav.getframerate() - 2.48) < .02
task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = '/Game/Sonniss'
task.destination_name = 'fireball_flame_whoosh'
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
path = '/Game/Sonniss/fireball_flame_whoosh'
sound = unreal.EditorAssetLibrary.load_asset(path)
assert isinstance(sound, unreal.SoundWave), 'Whoosh SoundWave import failed'
for key, value in dict(looping=False, volume=1.0, pitch=1.0, compression_quality=90).items():
    sound.set_editor_property(key, value)
unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
assert sound.get_editor_property('num_channels') == 2
assert abs(sound.get_editor_property('duration') - manifest['duration']) < .01
result = dict(asset=path, duration=sound.get_editor_property('duration'), channels=2,
              source_sha256=manifest['source_sha256'], wav_sha256=manifest['wav_sha256'], looping=False, pitch=1)
(root/'Saved/AudioImport/fireball-whoosh-import-validation.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
unreal.log_warning('FIREBALL_WHOOSH_IMPORTED ' + json.dumps(result))
