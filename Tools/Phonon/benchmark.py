"""Persistent CPU Phonon benchmark. First PCM is not speaker/A2F onset."""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import time
import wave

MODEL = 'hf://gradium/phonon-31m-en@92741c5b07d085ff840e26fea8ce43110b87852f'
LINES = [
    ('Rex', 'Hold her steady, Captain. There is clear water to starboard.'),
    ('Duncan', 'Port guns ready! Aim for the glowing joint before it strikes!'),
    ('Ruby', 'Fire on deck! I will smother it. Keep us clear of those pillars.'),
    ('Rex', 'That fog hides more than rocks. Keep your eyes on the channel, and I will keep us on course.'),
    ('Duncan', 'The creature is charging a fireball. Put a cannonball through its mouth before it lets fly!'),
    ('Ruby', 'The main rigging is damaged. Take the wheel while I repair it, and try to keep this ship in one piece.'),
]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--model', default=MODEL)
    parser.add_argument('--repeats', type=int, default=3)
    parser.add_argument('--voices', type=Path)
    args = parser.parse_args()
    import numpy as np
    import ptts
    ptts.set_num_threads(4)
    args.output.mkdir(parents=True, exist_ok=True)
    voice_options = json.loads(args.voices.read_text(encoding='utf-8')) if args.voices else {}
    catalog = voice_options.get('catalog')
    voices = voice_options.get('crew', {})
    started = time.perf_counter()
    tts = ptts.TTS(args.model, lang='en', device='cpu', quant='q8', voice_catalog=catalog)
    if args.voices:
        for info in voices.values():
            local_voice = args.voices.parent / info['filename']
            if local_voice.is_file():
                tts.add_voice(info['voice_id'], str(local_voice))
    load_ms = (time.perf_counter() - started) * 1000
    rows = []
    for repeat in range(args.repeats):
        for index, (speaker, text) in enumerate(LINES):
            voice = voices.get(speaker, {}).get('voice_id')
            start = time.perf_counter()
            arrivals, chunks = [], []
            with tts.stream(text, voice=voice) as audio:
                rate = audio.sample_rate
                for pcm in audio:
                    if not len(pcm):
                        continue
                    arrivals.append(time.perf_counter() - start)
                    data = np.asarray(pcm, dtype=np.float32).copy()
                    if data.ndim != 1 or not np.isfinite(data).all():
                        raise ValueError('Invalid mono PCM')
                    chunks.append(data)
            elapsed = time.perf_counter() - start
            if not chunks:
                raise ValueError('Empty synthesis')
            samples = np.concatenate(chunks)
            pcm16 = (np.clip(samples, -1, 1) * 32767).astype('<i2').tobytes()
            wav = args.output / f'{repeat}_{index}_{speaker}.wav'
            with wave.open(str(wav), 'wb') as out:
                out.setparams((1, 2, rate, 0, 'NONE', 'not compressed'))
                out.writeframes(pcm16)
            row = dict(repeat=repeat, speaker=speaker, voice=voice or 'model default', text=text,
                first_pcm_ms=arrivals[0] * 1000, full_synthesis_ms=elapsed * 1000,
                audio_seconds=len(samples) / rate, chunks=len(chunks), sample_rate=rate,
                maximum_chunk_gap_ms=max([b-a for a, b in zip(arrivals, arrivals[1:])] or [0]) * 1000,
                peak=float(np.max(np.abs(samples))), rms=float(np.sqrt(np.mean(samples**2))),
                sha256_pcm16=hashlib.sha256(pcm16).hexdigest(), wav=str(wav))
            rows.append(row)
            print(json.dumps({k: row[k] for k in ('repeat','speaker','first_pcm_ms','full_synthesis_ms','chunks')}), flush=True)
    # Closing a partially consumed stream must join its workers, then a fresh
    # request must still produce usable audio. This does not exercise UE cancellation.
    cancellations = []
    for consumed in (1, 5):
        with tts.stream(LINES[-1][1]) as stream:
            for _ in range(consumed):
                next(stream)
            start = time.perf_counter()
            stream.close()
            close_ms = (time.perf_counter() - start) * 1000
            stopped = next(stream, None) is None
        with tts.stream('Aye, Captain.') as recovery:
            recovered = sum(len(chunk) for chunk in recovery)
        cancellations.append(dict(chunks_consumed=consumed, close_ms=close_ms, stopped=stopped,
                                  recovered_samples=recovered))
        if not stopped or not recovered:
            raise RuntimeError('Cancellation/recovery failed')
    report = dict(model=args.model, runtime=ptts.__version__, build=ptts.build_info(), device=tts.device,
        quant=tts.quant, threads=ptts.get_num_threads(), model_load_ms=load_ms,
        measurement='Loaded model to first returned PCM and full synthesis; excludes A2F, transport, playback and text generation. First use of a catalog voice may download/load it.',
        median_first_pcm_ms=statistics.median(row['first_pcm_ms'] for row in rows),
        median_full_synthesis_ms=statistics.median(row['full_synthesis_ms'] for row in rows),
        rows=rows, cancellations=cancellations)
    (args.output / 'results.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k not in ('rows', 'build')}), flush=True)

if __name__ == '__main__':
    main()
