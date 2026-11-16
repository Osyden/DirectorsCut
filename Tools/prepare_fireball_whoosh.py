"""Prepare only the selected isolated whoosh from its lossless source; no Unreal required."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import wave

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--ffmpeg', required=True)
parser.add_argument('--source', type=Path, default=root.parent/'AudioAuditions/Fireball/fire.wav')
args = parser.parse_args()
source_hash = '344c8a9a31205fd5842feea827a491d89528f7db1ee5b1ffd4a3a018e23395ef'
assert hashlib.sha256(args.source.read_bytes()).hexdigest() == source_hash, 'Original audition source changed'
out = root/'Saved/AudioImport'
out.mkdir(parents=True, exist_ok=True)
wav_path = out/'fireball_flame_whoosh.wav'
filter_graph = 'loudnorm=I=-18:TP=-3:LRA=7'
command = [args.ffmpeg, '-hide_banner', '-nostdin', '-y', '-i', str(args.source),
           '-map_metadata', '-1', '-af', filter_graph, '-ar', '48000', '-ac', '2',
           '-c:a', 'pcm_s16le', '-bitexact', str(wav_path)]
subprocess.run(command, check=True, capture_output=True)
with wave.open(str(wav_path)) as wav:
    assert (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) == (48000, 2, 2)
    duration = wav.getnframes()/wav.getframerate()
    assert abs(duration - 2.48) < .02
manifest = dict(asset='/Game/Sonniss/fireball_flame_whoosh', creator='Epic Stock Media',
    collection='Elemental Mutation Whooshes and Impacts',
    archive='Sonniss.com-GDC2026-GameAudioBundle2of5.zip',
    original='Epic Stock Media - Elemental Mutation Whooshes and Impacts/FIREWhsh_Whoosh Fire Deep Growl Monster Saturated Crisp 03_ESM_EMWI.wav',
    source_sha256=source_hash, wav_sha256=hashlib.sha256(wav_path.read_bytes()).hexdigest(),
    selected_preview='Isolated-Flame-Whoosh.mp3', filter=filter_graph,
    ffmpeg_sha256=hashlib.sha256(Path(args.ffmpeg).read_bytes()).hexdigest(),
    duration=duration, rate=48000, channels=2, bits=16, pitch=1,
    note='Normalized directly from original lossless WAV; no mixing, pitch shift, time stretch, or MP3 decode.')
(out/'fireball-whoosh-provenance.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print(json.dumps(manifest, indent=2))
