"""Decode ../Music MP3s to stereo PCM for Unreal; requires ffmpeg on PATH or --ffmpeg.

Keeps the original levels of the supplied masters.
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
tracks = {"TheFog": "the fog.mp3", "TheCaldera": "the caldera.mp3", "TheAtlanteanAdventure": "The Atlantean Adventure.mp3"}
parser.add_argument("--asset", action="append", choices=tracks, help="Prepare only the selected asset(s); default all")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
dest = root / "Saved/MusicImport"
dest.mkdir(parents=True, exist_ok=True)
rows = []
for asset in args.asset or tracks:
    source_name = tracks[asset]
    source = root.parent / "Music" / source_name
    output = dest / (asset + ".wav")
    subprocess.run([args.ffmpeg, "-hide_banner", "-loglevel", "error", "-y", "-i", str(source),
                    "-vn", "-map", "0:a:0", "-ar", "48000", "-ac", "2", "-c:a", "pcm_s16le", str(output)], check=True)
    with wave.open(str(output), "rb") as wav:
        row = dict(source=source_name, asset=asset, sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                   seconds=wav.getnframes() / wav.getframerate(), channels=wav.getnchannels(), rate=wav.getframerate())
    rows.append(row)
    print(json.dumps(row))
manifest = dest / "manifest.json"
previous = json.loads(manifest.read_text(encoding="utf-8")) if manifest.is_file() else []
selected = {row["asset"] for row in rows}
rows = [row for row in previous if row["asset"] not in selected] + rows
manifest.write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
