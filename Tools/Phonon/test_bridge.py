"""Exercise the real local Phonon CPU bridge; no subjective voice-quality verdict.

Run with the staged ptts Python environment. Only this script's child is stopped.
The server must use its existing CPU-only local-model/local-voice contract.
"""
import argparse
import base64
from collections import deque
import hashlib
import json
import os
from pathlib import Path
import queue
import subprocess
import sys
import threading
import time


class Bridge:
    def __init__(self, command, timeout):
        self.timeout = timeout
        self.events = queue.Queue(maxsize=1024)
        self.stderr = deque(maxlen=32)
        environment = dict(os.environ, CUDA_VISIBLE_DEVICES='', HF_HUB_OFFLINE='1',
                           TRANSFORMERS_OFFLINE='1', PYTHONUNBUFFERED='1')
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, env=environment, shell=False,
                                        creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        threading.Thread(target=self._stdout, daemon=True).start()
        threading.Thread(target=self._stderr, daemon=True).start()

    def _stdout(self):
        try:
            while True:
                line = self.process.stdout.readline(131073)
                if not line:
                    self.events.put(('eof', None, time.perf_counter()))
                    return
                if len(line) > 131072 or not line.endswith(b'\n'):
                    raise ValueError('Oversized or unterminated stdout event')
                event = json.loads(line)
                if not isinstance(event, dict):
                    raise ValueError('Stdout event is not an object')
                self.events.put(('event', event, time.perf_counter()))
        except Exception as error:
            self.events.put(('reader_error', str(error), time.perf_counter()))

    def _stderr(self):
        while True:
            line = self.process.stderr.readline(4096)
            if not line:
                return
            self.stderr.append(line.decode('utf-8', errors='replace').rstrip())

    def send(self, request):
        raw = request if isinstance(request, bytes) else (json.dumps(request) + '\n').encode('utf-8')
        self.process.stdin.write(raw)
        self.process.stdin.flush()

    def receive(self, deadline, eof=False):
        remaining = deadline - time.perf_counter()
        if remaining <= 0:
            raise TimeoutError('Operation deadline exceeded')
        try:
            kind, event, arrival = self.events.get(timeout=remaining)
        except queue.Empty:
            raise TimeoutError('Timed out waiting for bridge event') from None
        if kind == 'eof' and eof:
            return None, arrival
        if kind != 'event':
            raise AssertionError(f'Unexpected {kind}: {event}')
        return event, arrival

    def stop(self):
        # Never enumerate or terminate unrelated Python, model or game processes.
        if self.process.poll() is None:
            try:
                self.process.stdin.close()
                self.process.wait(timeout=2)
            except (BrokenPipeError, OSError, subprocess.TimeoutExpired):
                self.process.terminate()
                try:
                    self.process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait(timeout=2)
        for stream in (self.process.stdin, self.process.stdout, self.process.stderr):
            if not stream.closed:
                stream.close()


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def speak(bridge, request_id, speaker, text, cancel=False):
    start = time.perf_counter()
    deadline = start + bridge.timeout
    bridge.send(dict(op='speak', id=request_id, speaker=speaker, text=text))
    sequence = samples = 0
    first = cancelled_at = None
    audible_data = False
    digest = hashlib.sha256()
    while True:
        event, arrival = bridge.receive(deadline)
        require(type(event.get('id')) is int and event['id'] == request_id,
                f'Stale or mismatched request id: expected {request_id}, got {event.get("id")}')
        if event.get('type') == 'pcm':
            require(type(event.get('seq')) is int and event['seq'] == sequence, 'PCM sequence is not contiguous')
            require(type(event.get('rate')) is int and event['rate'] == 24000, 'Wrong PCM sample rate')
            require(isinstance(event.get('data'), str), 'PCM data is not base64 text')
            pcm = base64.b64decode(event['data'], validate=True)
            require(0 < len(pcm) <= 38400 and len(pcm) % 2 == 0, 'Invalid mono PCM16 payload size')
            samples += len(pcm) // 2
            require(samples <= 24000 * 30, 'Utterance exceeds server sample limit')
            audible_data = audible_data or any(pcm)
            digest.update(pcm)
            sequence += 1
            if first is None:
                first = arrival
                if cancel:
                    bridge.send(dict(op='cancel', id=request_id))
                    cancelled_at = time.perf_counter()
            continue
        expected = 'cancelled' if cancel else 'done'
        require(event.get('type') == expected, f'Expected {expected}, received {event}')
        require(sequence > 0 and samples > 0, 'No PCM arrived before terminal event')
        require(audible_data, 'All received PCM samples are zero')
        if not cancel:
            require(type(event.get('samples')) is int and event['samples'] == samples,
                    'Done sample count does not match all decoded PCM')
        return dict(id=request_id, speaker=speaker, terminal=expected, chunks=sequence,
                    samples=samples, sample_rate=24000, pcm_sha256=digest.hexdigest(),
                    first_pcm_ms=round((first-start)*1000, 3),
                    operation_ms=round((arrival-start)*1000, 3),
                    cancel_to_terminal_ms=round((arrival-cancelled_at)*1000, 3) if cancel else None)


def reject(bridge, request, expected_id):
    start = time.perf_counter()
    bridge.send(request)
    event, arrival = bridge.receive(start + bridge.timeout)
    require(event.get('type') == 'error' and type(event.get('id')) is int
            and event['id'] == expected_id, f'Expected rejection id={expected_id}, received {event}')
    require(isinstance(event.get('message'), str) and event['message'], 'Missing error message')
    return dict(id=expected_id, terminal='error', operation_ms=round((arrival-start)*1000, 3))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--voices', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True, help='JSON report filename')
    parser.add_argument('--python', default=sys.executable, help='Python executable with staged ptts 0.7.1')
    parser.add_argument('--server', type=Path, default=Path(__file__).with_name('serve.py'))
    parser.add_argument('--timeout', type=float, default=20, help='Whole-operation deadline, at most 20 seconds')
    args = parser.parse_args()
    if not 0 < args.timeout <= 20:
        parser.error('timeout must be greater than zero and at most 20')
    report = dict(test='Phonon real CPU bridge', quality_evaluated=False, passed=False, results=[])
    bridge = None
    current = 'local_inputs'

    def record(name, callback):
        nonlocal current
        current = name
        result = callback()
        report['results'].append(dict(case=name, passed=True, **result))

    try:
        model = args.model.resolve(strict=True)
        voices = args.voices.resolve(strict=True)
        server = args.server.resolve(strict=True)
        require(model.is_dir() and (model/'config.json').is_file(), 'Expected staged local model directory')
        require(voices.is_file() and server.is_file(), 'Expected local manifest and server files')
        command = [args.python, '-u', str(server), '--model', str(model), '--voices', str(voices)]
        report.update(command=command, server_sha256=hashlib.sha256(server.read_bytes()).hexdigest(),
                      manifest_sha256=hashlib.sha256(voices.read_bytes()).hexdigest(), timeout_seconds=args.timeout)
        current = 'startup'
        started = time.perf_counter()
        bridge = Bridge(command, args.timeout)
        ready, arrived = bridge.receive(started + args.timeout)
        require(ready.get('type') == 'ready' and ready.get('device') == 'cpu'
                and ready.get('runtime') == '0.7.1' and ready.get('sample_rate') == 24000
                and ready.get('threads') == 4, f'Invalid ready event: {ready}')
        report['results'].append(dict(case=current, passed=True, ready=ready, load_ms=round((arrived-started)*1000, 3)))
        for request_id, speaker in enumerate(('Rex', 'Duncan', 'Ruby'), 1):
            record('full_pcm_'+speaker, lambda i=request_id, s=speaker:
                   speak(bridge, i, s, 'Ready, Captain. Keep the ship steady.'))
        record('cancel_after_first_pcm', lambda: speak(bridge, 4, 'Duncan',
               'Keep your eyes on the channel, Captain. The fog hides the rocks ahead, '
               'and the current is pulling us towards them. I will watch the guns while '
               'Ruby checks the rigging. We must keep this ship clear of the cliffs.', cancel=True))
        record('cancel_recovery', lambda: speak(bridge, 5, 'Ruby', 'Ready again, Captain.'))
        record('unknown_speaker', lambda: reject(bridge, dict(op='speak', id=6, speaker='Unknown', text='Ready.'), 6))
        record('unknown_speaker_recovery', lambda: speak(bridge, 7, 'Rex', 'Standing by, Captain.'))
        record('oversized_text', lambda: reject(bridge, dict(op='speak', id=8, speaker='Rex', text='x'*241), 8))
        record('oversized_text_recovery', lambda: speak(bridge, 9, 'Duncan', 'Standing by, Captain.'))
        record('malformed_json', lambda: reject(bridge, b'{not-json}\n', 0))
        record('malformed_json_recovery', lambda: speak(bridge, 10, 'Ruby', 'Standing by, Captain.'))
        record('duplicate_request_id', lambda: reject(bridge, dict(op='speak', id=10, speaker='Ruby', text='Ready.'), 10))
        record('duplicate_request_recovery', lambda: speak(bridge, 11, 'Rex', 'All clear, Captain.'))
        current = 'orderly_shutdown_eof'
        shutdown_start = time.perf_counter()
        bridge.send(dict(op='shutdown', id=12))
        bridge.process.stdin.close()
        event, ended = bridge.receive(shutdown_start + args.timeout, eof=True)
        require(event is None, f'Unexpected trailing event on shutdown: {event}')
        remaining = shutdown_start + args.timeout - time.perf_counter()
        require(remaining > 0, 'Shutdown deadline exceeded')
        require(bridge.process.wait(timeout=remaining) == 0, 'Bridge exited unsuccessfully')
        report['results'].append(dict(case=current, passed=True, exit_code=0, operation_ms=round((ended-shutdown_start)*1000, 3)))
        report['passed'] = True
    except Exception as error:
        report['results'].append(dict(case=current, passed=False, error=type(error).__name__+': '+str(error)))
    finally:
        if bridge:
            bridge.stop()
            report['stderr_tail'] = list(bridge.stderr)
            report['exit_code'] = bridge.process.returncode
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(passed=report['passed'], cases=len(report['results']), output=str(args.output))))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
