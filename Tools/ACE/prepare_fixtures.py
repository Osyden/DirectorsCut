"""Generate identical Piper inputs for the baseline and local Audio2Face test.

Uses only the Python standard library. Outputs stay in ignored Saved/ACE.
Timings measure standalone Piper process startup + synthesis, NOT gameplay latency.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import time
import wave

ROOT = Path(__file__).resolve().parents[2]
VOICES = {
    "Finch": "en_GB-vctk-medium.onnx",
    "Marlow": "en_GB-northern_english_male-medium.onnx",
    "Odessa": "en_GB-alba-medium.onnx",
}
LINES = (
    ("confirmation", "Aye, Captain. Bringing her hard to starboard."),
    ("warning", "Brace yourselves! Fire on deck, port side!"),
    ("quiet", "The sea has gone quiet. I do not like it."),
)


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sidecars", type=Path, default=ROOT / "Sidecars")
    parser.add_argument("--output", type=Path, default=ROOT / "Saved/ACE/Fixtures")
    args = parser.parse_args()
    piper = args.sidecars / "bin/piper/piper/piper.exe"
    if not piper.is_file():
        parser.error(f"Piper missing: {piper}")
    models = {name: args.sidecars / "models/voices" / model for name, model in VOICES.items()}
    for model in models.values():
        if not model.is_file() or not model.with_suffix(".onnx.json").is_file():
            parser.error(f"Voice model or config missing: {model}")
    args.output.mkdir(parents=True, exist_ok=True)
    rows = []
    for speaker, model in models.items():
        model_hash = sha256(model)
        for kind, text in LINES:
            target = args.output / f"{speaker.lower()}_{kind}.wav"
            # Piper writes only into this experiment's output directory.
            started = time.perf_counter()
            run = subprocess.run(
                [str(piper.resolve()), "--model", str(model.resolve()), "--output_file", str(target.resolve()),
                 "--speaker", "95" if speaker == "Finch" else "0",
                 "--length_scale", {"Finch": "1.04", "Marlow": "0.95", "Odessa": "1.0"}[speaker],
                 "--noise_scale", "0.5", "--noise_w", "0.333" if speaker == "Odessa" else "0.65",
                 "--sentence_silence", "0.12"],
                input=text + "\n", text=True, capture_output=True, timeout=30,
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
            )
            elapsed_ms = (time.perf_counter() - started) * 1000
            if run.returncode:
                raise RuntimeError(f"Piper failed for {target.name}: {run.stderr}")
            with wave.open(str(target), "rb") as wav:
                channels, width, rate, frames = (
                    wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()
                )
                if channels != 1 or width != 2 or frames == 0 or wav.getcomptype() != "NONE":
                    raise RuntimeError(f"Expected nonempty PCM16 mono: {target}")
                duration = frames / rate
            row = dict(speaker=speaker, kind=kind, text=text, wav=target.name,
                       sample_rate=rate, channels=channels, duration_seconds=round(duration, 4),
                       piper_process_ms=round(elapsed_ms, 2), wav_sha256=sha256(target),
                       voice_model=model.name, voice_model_sha256=model_hash)
            rows.append(row)
            print(f"{target.name}: {duration:.2f}s, Piper process {elapsed_ms:.0f}ms", flush=True)
    with (args.output / "manifest.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (args.output / "manifest.json").write_text(json.dumps({
        "purpose": "Use the same WAV bytes for baseline and Audio2Face playback",
        "timing_scope": "Standalone Piper startup plus synthesis; not in-game latency or ACE inference",
        "cases": rows,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
