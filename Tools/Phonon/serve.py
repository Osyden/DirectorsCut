"""Private stdin/stdout PCM bridge: local pinned assets, one resident CPU model.

Only NDJSON protocol goes to stdout. No sockets, tokens, model downloads or shell commands.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import queue
import sys
import threading
import time

MAX_TEXT = 240
MAX_SAMPLES = 24000 * 30

def load_model(model, manifest):
    import numpy as np
    import ptts
    if ptts.__version__ != '0.7.1':
        raise ValueError('Expected ptts 0.7.1')
    model = Path(model).resolve(strict=True)
    manifest = Path(manifest).resolve(strict=True)
    if not model.is_dir() or not (model / 'config.json').is_file():
        raise ValueError('A local model directory is required')
    config = json.loads(manifest.read_text(encoding='utf-8'))
    if set(config['crew']) != {'Rex', 'Duncan', 'Ruby'}:
        raise ValueError('Expected exactly the three crew voices')
    local_voices = {}
    for speaker, info in config['crew'].items():
        path = (manifest.parent / info['filename']).resolve(strict=True)
        if not path.is_relative_to(manifest.parent):
            raise ValueError('Voice must be inside manifest directory')
        data = path.read_bytes()
        if len(data) != info['bytes'] or hashlib.sha256(data).hexdigest() != info['sha256']:
            raise ValueError('Voice checksum mismatch')
        local_voices[speaker] = path
    ptts.set_num_threads(4)
    tts = ptts.TTS(str(model), lang='en', device='cpu', quant='q8')
    if tts.sample_rate != 24000:
        raise ValueError('Expected 24 kHz model')
    for speaker, path in local_voices.items():
        tts.add_voice(speaker, str(path))
    return tts, np

def serve(tts, np):
    output_lock = threading.Lock()
    state_lock = threading.Lock()
    jobs = queue.Queue(maxsize=1)
    shutdown = threading.Event()
    active = {'id': None, 'cancel': None}
    last_id = 0

    def emit(event):
        with output_lock:
            sys.stdout.write(json.dumps(event, ensure_ascii=True, separators=(',', ':')) + '\n')
            sys.stdout.flush()

    def read_commands():
        nonlocal last_id
        try:
            while not shutdown.is_set():
                line = sys.stdin.buffer.readline(8193)
                if not line:
                    break
                if len(line) > 8192 or not line.endswith(b'\n'):
                    emit(dict(type='error', id=0, message='Oversized or unterminated input'))
                    break
                request_id = 0
                try:
                    request = json.loads(line)
                    if not isinstance(request, dict):
                        raise ValueError('Expected object')
                    request_id = request.get('id', 0)
                    if type(request_id) is not int or not 0 <= request_id <= 2147483647:
                        request_id = 0
                        raise ValueError('Invalid request id')
                    operation = request.get('op')
                    if operation == 'shutdown':
                        break
                    if operation == 'cancel':
                        with state_lock:
                            if active['id'] == request_id and active['cancel']:
                                active['cancel'].set()
                        continue
                    if operation != 'speak':
                        raise ValueError('Unknown operation')
                    text = request.get('text')
                    speaker = request.get('speaker')
                    if speaker not in ('Rex', 'Duncan', 'Ruby'):
                        raise ValueError('Unknown speaker')
                    if not isinstance(text, str) or not text.strip() or len(text) > MAX_TEXT or '\x00' in text:
                        raise ValueError('Invalid text')
                    with state_lock:
                        if request_id <= last_id:
                            raise ValueError('Request id must increase')
                        if active['id'] is not None:
                            raise ValueError('Busy')
                        last_id = request_id
                        cancel = threading.Event()
                        active.update(id=request_id, cancel=cancel)
                        jobs.put_nowait((request_id, speaker, text, cancel))
                except (ValueError, KeyError, TypeError, queue.Full):
                    emit(dict(type='error', id=request_id, message='Invalid request or worker busy'))
        finally:
            shutdown.set()
            with state_lock:
                if active['cancel']:
                    active['cancel'].set()

    reader = threading.Thread(target=read_commands, daemon=True)
    reader.start()
    emit(dict(type='ready', sample_rate=24000, runtime='0.7.1', device='cpu', threads=4))
    while not shutdown.is_set():
        try:
            request_id, speaker, text, cancel = jobs.get(timeout=.1)
        except queue.Empty:
            continue
        samples = 0
        seq = 0
        terminal = None
        started = time.perf_counter()
        try:
            if not cancel.is_set():
                with tts.stream(text, voice=speaker) as audio:
                    for pcm in audio:
                        if cancel.is_set() or shutdown.is_set():
                            break
                        if time.perf_counter() - started > 15:
                            raise ValueError('Synthesis timed out')
                        data = np.asarray(pcm, dtype=np.float32)
                        if data.ndim != 1 or not np.isfinite(data).all():
                            raise ValueError('Invalid audio')
                        if not len(data):
                            continue
                        samples += len(data)
                        if samples > MAX_SAMPLES:
                            raise ValueError('Audio limit exceeded')
                        # Bound an individual protocol line as well as the utterance.
                        for offset in range(0, len(data), 19200):
                            if cancel.is_set() or shutdown.is_set():
                                break
                            raw = (np.clip(data[offset:offset+19200], -1, 1) * 32767).astype('<i2').tobytes()
                            emit(dict(type='pcm', id=request_id, seq=seq, rate=24000,
                                      data=base64.b64encode(raw).decode('ascii')))
                            seq += 1
            terminal = dict(type='cancelled', id=request_id) if cancel.is_set() else dict(type='done', id=request_id, samples=samples)
            if not cancel.is_set() and not samples:
                raise ValueError('Empty synthesis')
        except Exception:
            # Avoid exposing filesystem paths or arbitrary runtime error strings on the wire.
            terminal = dict(type='error', id=request_id, message='Synthesis failed')
        finally:
            # Clear busy before publishing terminal; the next speak can arrive
            # immediately after the client consumes this event.
            with state_lock:
                active.update(id=None, cancel=None)
            if not shutdown.is_set():
                emit(terminal)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', required=True)
    parser.add_argument('--voices', required=True)
    args = parser.parse_args()
    try:
        tts, np = load_model(args.model, args.voices)
        serve(tts, np)
    except BrokenPipeError:
        return 0
    except Exception as error:
        print('Phonon bridge could not start: ' + type(error).__name__, file=sys.stderr, flush=True)
        return 1
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
