"""Decode ../Music MP3s to stereo PCM for Unreal; requires ffmpeg on PATH or --ffmpeg.

Keeps the original levels (both supplied masters measure about -16 dB RMS).
The game applies a quiet score gain plus speech ducking at runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import wave

parser = argparse.ArgumentParser()
parser.add_argument("--ffmpeg", default="ffmpeg")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
dest = root / "Saved/MusicImport"
dest.mkdir(parents=True, exist_ok=True)
rows = []
for source_name, asset in (("the fog.mp3", "TheFog"), ("the caldera.mp3", "TheCaldera")):
    source = root.parent / "Music" / source_name
    output = dest / (asset + ".wav")
    subprocess.run([args.ffmpeg, "-hide_banner", "-loglevel", "error", "-y", "-i", str(source),
                    "-vn", "-map", "0:a:0", "-ar", "48000", "-ac", "2", "-c:a", "pcm_s16le", str(output)], check=True)
    with wave.open(str(output), "rb") as wav:
        row = dict(source=source_name, asset=asset, sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                   seconds=wav.getnframes() / wav.getframerate(), channels=wav.getnchannels(), rate=wav.getframerate())
    rows.append(row)
    print(json.dumps(row))
(dest / "manifest.json").write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
