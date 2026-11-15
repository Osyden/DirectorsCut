# Phonon 31M alternative: measured CPU producer, unvalidated native adapter

Owner: AI NPCs chat, `codex/speech-streaming`, following streaming candidate `8cbf6a7`. The public **31 million parameter** English model is the selected target. No 40M model or credentials are used. Piper remains the default and the current playable is unchanged.

## Completed and measured

The resident Python `ptts` 0.7.1 CPU model emits signed 16-bit, mono, 24 kHz PCM through a private stdin/stdout protocol. Each request is bounded to 240 characters and 30 seconds of audio. It keeps one model loaded, serializes synthesis, validates all three local voice embeddings by hash, and performs no runtime downloads. Cancel closes the active generator before the next request can start. Model and voice files were staged and verified under `DirectorsCut/Saved/PhononComparison/persistent-runtime`.

Actual CPU measurements on this PC, four threads, q8, 18 matched lines per run:

| Configuration | Median first returned PCM | Median full synthesis |
| --- | ---: | ---: |
| Resident Phonon, default voice | 17.64 ms | 393.36 ms |
| Resident Phonon, three British crew candidates | 19.98 ms | 492.85 ms |

These exclude LLM, Audio2Face, transport and audible playback. The first invocation/first use of each voice is included in the 18 observations; files and individual timings are retained. Background CPU load and different voices prevent a controlled voice-quality or hardware throughput conclusion. The previous root comparison measured process-start-to-full-WAV, including load: Piper859.4ms and Phonon765.2ms. It does not measure persistent Piper or Piper first PCM, so do not present a direct persistent speed ratio.

`Tools/Phonon/test_bridge.py` ran the actual local producer and **passed 15 checks**: startup; all three speakers' PCM ordering/rate/sample count/nonzero signal; cancel after first PCM and recovery; invalid speaker, oversized text, malformed JSON and duplicate ID rejection with recovery; and shutdown/EOF. Cancel-to-terminal was74.1ms in that run. Separate generator close/recovery probes passed after one and five chunks (crew benchmark13.8ms and97.2ms). These are CPU producer tests, not native cancellation evidence.

Evidence (local, not committed):

- `E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/persistent-default/results.json`
- `E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/persistent-crew/results.json`, plus18 WAVs
- `E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/bridge-tests/results.json`
- `E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/persistent-runtime/staging.json`

## Candidate native integration

Opt in with `-DCTTSBackend=Phonon`; this also enables phrase-level LLM streaming. The new `UDCPhononSpeech` owns a persistent Python process and one background worker. It validates request IDs, sequences, format, total samples, limits and deadlines, then feeds incoming PCM into `FACERuntimeModule::AnimateFromAudioSamples` on that serialized worker. A final `EndAudioSamples` closes the native input. The actor retains its existing prebuffer and animation lifecycle checks, and does not replay a whole line after partial failure. The model producer runs on CPU; Audio2Face remains on the configured local GPU provider with separate CUDA and paced input.

Installed SDK audit identified an unprotected native session map and a lock-order hazard between `Stop()` and callbacks. The worker serializes its native calls, excludes legacy WAV requests, and owns detach/end before game-thread Stop. It retains the consumer through drain and suppresses cancelled completions. Reuse waits two frames from the actual game-thread Stop so the component can leave ENDING before accepting fresh audio. Native send/drain calls themselves cannot be forcibly timed out; their ownership remains retained while they finish, without blocking the game thread. Unlike the Piper candidate's lookahead WAV slot, this initial Phonon adapter streams PCM for one phrase at a time; the next phrase starts after current playback retires. It does not yet preserve one A2F session across every LLM phrase. Evaluate phrase-boundary gaps before release.

**Native C++ has not been compiled or run.** No claim of in-game latency, smoothness, facial output, restart safety or complete integration acceptance is made. Root owns the serialized engine/GPU lane for combat fixes and will schedule validation afterward.

## Reproduce CPU checks

```powershell
$python = 'E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/auth-env/Scripts/python.exe'
$payload = 'E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/persistent-runtime'
& $python 'E:/Projects/DirectorsCut/Speech-Streaming/Tools/Phonon/test_bridge.py' --model "$payload/model" --voices "$payload/voices.json" --output 'E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/bridge-tests/recheck.json'
```

Stage from the already-downloaded pinned model using `Tools/Phonon/stage.py`; voice downloads require explicit `--download-voices`, and every artifact is hash checked. `benchmark.py` accepts `--model`, `--voices`, `--output` and optional `--repeats`. No GPU is selected by these Python tools.

## Native validation after root releases the lane

First integrate both commits into the dependency-complete project and follow the build/automation commands in [streaming handoff](SPEECH-STREAMING-15NOV.md). Append these flags to that handoff's conversation review command, with distinct Phonon report/log paths:

```powershell
-DCTTSBackend=Phonon
-DCPhononPython=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/auth-env/Scripts/python.exe
-DCPhononScript=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/persistent-runtime/serve.py
-DCPhononModel=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/persistent-runtime/model
-DCPhononVoices=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononComparison/persistent-runtime/voices.json
```

Run one backend at a time. Require three-crew face/audio output, phrase continuity under fog, onset measured from the actual player request, short-tail flush and unchanged command counts. Cancel while generating, sending and playing; test menu, phase change and reload. Kill only the owned producer during a partial utterance and confirm no replay/stale PCM and successful next request. Test malformed PCM/native send failure using a dedicated test producer, not the shipped bridge. Verify the old consumer drains before a new request can reuse it. Producer checks do not replace any of these native tests.

## Voices and distribution

Candidates are Rex=Edwin, Duncan=Hugh, Ruby=Bryony. All are catalogued British; no explicitly Scottish entry was found. They were synthesized but **not auditioned or approved for quality**. Preserve Piper's accepted voices until listening review. See [voice provenance and alternatives](PHONON-VOICES-15NOV.md).

Model/voice revisions and hashes are pinned; licence texts and attribution are in `Tools/Phonon/THIRD-PARTY-NOTICES.md` and `licenses/`, also copied into the staged payload. The model and voice catalog declare Apache-2.0; the installed runtime wheel declares MIT OR Apache-2.0. The development venv is **not a portable release runtime**: Python/NumPy and compiled-runtime dependency notices, packaging and second-machine validation remain before distribution. No default launcher, packaged build or upload was changed.
