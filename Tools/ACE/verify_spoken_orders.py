"""Synthesize captain orders, transcribe with the installed Whisper, then save inputs for UE command tests.
Synthetic clean speech verifies the pipeline; it does not substitute for a live microphone test.
"""
import array
import io
import json
from pathlib import Path
import subprocess
import time
import urllib.request
import wave

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Saved/ACE/SpokenOrders"
CASES = [
    ("Finch, take the helm.", 1, 1, 0, "Finch"),
    ("Finch, hard to port.", 4, 1, 1, "Finch"),
    ("Finch, hard to starboard.", 5, 1, 2, "Finch"),
    ("Finch, steady as she goes.", 6, 1, 0, "Finch"),
    ("Marlow, man the port cannons.", 1, 2, 1, "Marlow"),
    ("Marlow, man the starboard guns.", 1, 2, 2, "Marlow"),
    ("Marlow, port battery, fire.", 2, 2, 1, "Marlow"),
    ("Marlow, starboard guns, fire.", 2, 2, 2, "Marlow"),
    ("Marlow, hold your fire.", 3, 2, 0, "Marlow"),
    ("Marlow, do not shoot.", 3, 2, 0, "Marlow"),
    ("Odessa, put out the fire.", 7, 3, 0, "Odessa"),
    ("Odessa, repair the rigging.", 8, 4, 0, "Odessa"),
    ("Odessa, report.", 9, 0, 0, "Odessa"),
]
PROMPT = "Orders on a sailing ship. Finch, Marlow, Odessa. Hard to starboard. Hard to port. Steady as she goes. Take the helm. Man the cannons. Port battery, fire. Starboard guns, hold fire. Put out the fire. Repair the rigging."

def pcm16k(path):
    with wave.open(str(path), "rb") as wav:
        assert wav.getnchannels() == 1 and wav.getsampwidth() == 2
        rate = wav.getframerate()
        src = array.array("h", wav.readframes(wav.getnframes()))
    samples = array.array("h")
    for i in range(int((len(src)-1)*16000/rate)):
        pos = i*rate/16000
        j = int(pos)
        samples.append(int(src[j] + (src[j+1]-src[j])*(pos-j)))
    output = io.BytesIO()
    with wave.open(output, "wb") as wav:
        wav.setparams((1, 2, 16000, 0, "NONE", "not compressed"))
        wav.writeframes(samples.tobytes())
    return output.getvalue()

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    server = ROOT / "Sidecars/bin/whisper/Release/whisper-server.exe"
    model = ROOT / "Sidecars/models/ggml-base.en.bin"
    piper = ROOT / "Sidecars/bin/piper/piper/piper.exe"
    voice = ROOT / "Sidecars/models/voices/en_GB-alba-medium.onnx"
    flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    log = (OUT / "whisper.log").open("w", encoding="utf-8")
    proc = subprocess.Popen([str(server), "-m", str(model), "--host", "127.0.0.1", "--port", "8794", "-t", "8", "--no-gpu"], stdout=log, stderr=log, creationflags=flags)
    try:
        ready = False
        for _ in range(60):
            if proc.poll() is not None:
                raise RuntimeError("Whisper exited during startup; inspect whisper.log")
            try:
                urllib.request.urlopen("http://127.0.0.1:8794/", timeout=1).close()
                ready = True
                break
            except OSError:
                time.sleep(.25)
        if not ready:
            raise RuntimeError("Whisper did not become ready")
        rows = []
        for index, (text, action, station, side, crew) in enumerate(CASES):
            wav = OUT / f"order_{index:02d}.wav"
            subprocess.run([str(piper), "--model", str(voice), "--output_file", str(wav), "--noise_scale", "0.5", "--noise_w", "0.333", "--quiet"], input=text+"\n", text=True, check=True, capture_output=True, timeout=15, creationflags=flags)
            audio = pcm16k(wav)
            wav.write_bytes(audio)
            boundary = "----DCBoundary7MA4YWxk"
            body = bytearray()
            for name, value in {"response_format":"json", "temperature":"0.0", "prompt":PROMPT}.items():
                body.extend(f'--{boundary}\r\nContent-Disposition: form-data; name="{name}"\r\n\r\n{value}\r\n'.encode())
            body.extend(f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="order.wav"\r\nContent-Type: audio/wav\r\n\r\n'.encode())
            body.extend(audio)
            body.extend(f"\r\n--{boundary}--\r\n".encode())
            request = urllib.request.Request("http://127.0.0.1:8794/inference", data=body, headers={"Content-Type":f"multipart/form-data; boundary={boundary}"})
            with urllib.request.urlopen(request, timeout=20) as response:
                heard = json.load(response)["text"].strip()
            rows.append(dict(source=text, text=heard, action=action, station=station, side=side, crew=crew))
            print(f"{index:02d}: {heard}", flush=True)
        (OUT / "transcripts.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        log.close()

if __name__ == "__main__":
    main()
