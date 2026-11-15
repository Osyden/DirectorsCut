# Next local test build: Phonon 31M + Gemma E4B

The user explicitly requested these as the **defaults for the next build**, superseding the earlier opt-in preference. Config now selects LlamaCpp with `gemma-4-E4B-it-Q4_K_M.gguf`, Phonon31M, speech streaming and a1-second A2F prebuffer. The previously selected playable is unchanged. Native commands, three-crew speech, eight full conversations and Cancel/Exit/Failure/Reload acceptance now pass. Root owns packaged cold launch and delivery.

## Native results, 15 November evening

Evidence is under main `Saved/PhononGemma/`.

### Phrase-pause correction, 22:05

The user requested smoother speech after the initial acceptance. Follow-up `5475213` (main `394f9c9`, `FifthBuild11.log`) gives Phonon each complete, validated, bounded reply as one utterance. Streaming text still undergoes the existing validation; progressive PCM delivery, Audio2Face buffering and all native cancellation/lifetime code are unchanged. This supplies full phrasing context and removes per-clause session/prebuffer resets. The small text lookahead is appropriate for the observed approximately 0.3-second Gemma replies; a slower model would incur its full text-generation wait before speech.

- `Saved/PhononNatural/conversation.json` and `.log`: **8/8 pass**, each exactly one speech/face session, all eight with positive samples and playback beginning before native audio sending finishes. Three crew, history, scene reset and no command execution remain covered. Gemma was launched by this editor run rather than reusing an existing model server.
- Warm question-to-playback observations: **2.100–2.233 seconds**, first cold Audio2Face line **4.400 seconds**. This is playback state timing, not microphone/STT or measured acoustic onset. Phonon's first-TTS admission is now observed directly instead of only watching Piper's WAV queue.
- `session-comparison.json` compares playback event logs: **22 sessions became eight**, and **14 internal session gaps of 1.566–1.966 seconds (median 1.933 seconds) became zero**. Natural pauses inside synthesized audio remain; this is not a claim of zero acoustic silence.
- One exactly matching Duncan reply used three sessions before and one afterward. Question-to-completion fell from **13.666 to 9.566 seconds**. Other generated replies differ, so the report does not present them as a controlled speed or voice-quality benchmark.
- Normal world cleanup drained in 26 ms; application exit had zero workers. Engine, owned Gemma and Phonon processes were gone, with port8791 clear, before returning the lane to root at 22:05. Source is frozen for the next package and trailer capture.

The older 22-chunk figures and phrase-gap limitation below describe the retained pre-correction baseline. Packaged cold launch and human listening remain with the build/playtest flow; local native tests do not establish long-session or second-machine reliability.

- `Automation2/index.json`: all eight dialogue tests pass (seven success, one success with expected warnings). Initial chunker failures were real: Unreal's `FString::StartsWith` rejects empty prefixes. Commit `b277048` fixes the three initial-prefix checks without weakening tests.
- `commands3.json`: **27/27 pass**, selected Gemma filename confirmed. The 24 command cases took 246–394 ms; three production-persona acknowledgements took 79–146 ms. This run used a resident model. Initial failed runs remain retained as evidence; these are not cold-start or cross-model benchmark figures.
- `smoke.log`: three authored Phonon lines completed actual Audio2Face playback. Rex/Duncan/Ruby respectively returned 115/98/91 animation samples and 91,999/78,399/72,799 audio samples. Normal exit drained the native worker in 16 ms.
- `conversation.json` and `.log`: **8/8 real questions pass**, across all crew, including follow-up history, roster/player appearance, duties, fog/weather and ship/fire facts. All **22** streamed chunks completed matching facial/audio playback; commands were not executed by conversation, and scene change reset old memory. Replies were checked against recorded context. Normal exit drained in 21 ms.
- Conversation model replies completed in about 236–370 ms (generation callback logs). Review-tick observations put first speech at 4.500 s for the first cold Audio2Face line and 1.946–2.133 s for subsequent questions. These measure question-to-playback state, not microphone/STT latency or recorded acoustic onset. The Phonon first-TTS field is null because the existing observer watches Piper's pending-WAV queue; do not treat it as zero latency.
- Chunks begin before final model completion and Audio2Face playback begins before all audio is sent. The fast model finishes before audible playback. Separate phrase sessions still create audible-pause risk; short fragments such as “Captain.” can become their own session. Voice taste, accent suitability and continuous listening remain human playtest items; catalog voices are British, not verified Scottish.

`14ba757` adds bounded `-DCPhononLifecycleReview=Reload` and `=Failure` probes alongside Cancel/Exit. Reload uses ordinary OpenLevel/EndPlay and verifies old-worker drain before fresh speech. Failure terminates only the worker's own producer handle, requires a failed batch and actual producer restart, then checks a fresh critical line.

- `cancel.log`: PASS. Rex was audibly active while native input was pending; cancellation advanced epoch 1 to 2 and fresh Ruby critical speech completed audio/faces. Exit drain 23 ms.
- `exit.log`: PASS. Normal application exit requested during actual Rex playback with native input pending. The worker drained in 141 ms before ACE module teardown; actual engine process exited and no owned Phonon process remained.
- `failure.log`: PASS. Owned producer terminated during active playback/pending native send. Interrupted batch retired with one failed chunk and zero completed faces, producer generation advanced 1 to 2, and fresh Ruby critical audio/faces completed. Exit drain 19 ms. This covers producer death during native speech; Python may already have generated the buffered PCM by that point.
- Initial Reload probe FAILED with an application assertion (`ReferenceChainSearch.cpp:2061`, old world not cleaned by GC), not a GPU/PC crash. Full evidence is retained in main `Saved/Crashes/UECC-Windows-A867A59D413D1E0194CE5581500589E4_0000/`. Source/timing evidence shows asynchronous EndPlay allowed native consumer leases and queued game-thread cleanup to outlive the world teardown boundary; the crash log does not print a complete reference chain. Correction `58a27c6` adds a world-scoped pre-teardown drain. It preserves leases until native and game-thread cleanup finish, then joins/removes only that world's workers; new-world initialization remains enabled.
- `reload2.log`: **corrected Reload PASS**, compiled in main `e2ab2b5` (`FifthBuild10.log`). Actual mid-speech old-world drain completed in 106 ms, the new world verified the old worker gone, and fresh Ruby audio/faces completed. Normal second-world teardown drained in 27 ms; application exit then had zero workers. Engine exited successfully with no world-leak assertion or orphan producer. The unchanged Reload scenario now passes; it does not establish safety for unsupported custom travel initiated inside a game-thread task.

Engine/GPU lane returned to root at 22:00 after all test engines exited. Owned Gemma server PID42236 stopped and port8791 listener cleared for a genuine packaged Gemma/Phonon cold launch. Pre-existing Whisper PID6852 was left intact by agreement; do not claim a cold packaged Whisper launch. Packaged cold launch, recorded acoustic continuity and clean second-machine checks remain separate.

## Local payload ready

### Selected-model packaging, 22:13

The user requested excluding redundant models from future builds. Tooling-only `effe438` makes `stage_sidecars.ps1` select the TTS runtime and voices from `DefaultGame.ini`: current builds copy `llama`, `whisper`, and `phonon`, plus the selected Gemma GGUF and Whisper model. Piper's three ONNX voices/configs and runtime are omitted when Phonon is selected; the Phonon runtime has no Piper fallback dependency. A Piper-selected build still validates/stages its configured three voices and omits Phonon. Unsupported backend settings fail staging.

This removes **242,217,935 bytes (242.2 MB / 231.0 MiB)** from the preceding Phonon layout. Qwen was already excluded, vendor Qwen auto-staging was already patched off, and the unused 2.57 GB legacy Nemotron file was already pruned. Those earlier exclusions are preserved, not counted again as new savings. Required Audio2Face/Audio2Emotion model files, Phonon default/crew voices, Whisper, Gemma and their runtimes remain. Source models are retained for development comparisons.

Full fresh-copy validation passed against the real main payload: exact selected model inventory, no Piper bin/voices, pinned copied Gemma hash and relocated Python imports (`ptts0.7.1`, `numpy2.5.4`). Sidecars payload: **5,416,738,647 bytes**, excluding the inventory itself. `staging-manifest.json` now records every staged Sidecars file and the selected runtime bins. Main evidence: `Saved/ModelPrune-validation.json` and `ModelPrune-staging-manifest.json`. Piper prerequisite selection and unknown-backend rejection also passed. The isolated worktree test uses the new optional `-ProjectRoot` to read main's complete config/notices without editing main.

The 22:11 package reached old Sidecars staging just before integration. Root is replacing Sidecars in that unaccepted candidate through fresh selective staging before packaged tests; no runtime rebuild is required. `package_isolated.ps1` was not changed during its invocation. Automatic approval review blocked cleanup of generated `Saved/ModelPruneValidation-2213` and `-2214`; these temporary copies remain outside release packaging. No retry or deletion workaround was attempted.

- Gemma user download found at `C:/Users/ejobse/Downloads/gemma-4-E4B-it-Q4_K_M.gguf`, copied to main `Sidecars/models`. Size4,977,171,584bytes and SHA256 `85a896a047553e842f25297ee5b031d64ff30147d9c4af17b1e4b394cd1fab87` match at both locations. No replacement download was attempted. Revision `bfc15c382204943c3a8fff0c750b94ae2364d7a3`; attribution/licence in `Docs/ThirdPartyGemma`.
- Phonon model, voices, bridge and notices are staged at main `Sidecars/bin/phonon`. Model and three voice hashes verified by `Tools/Phonon/stage.py`.
- Self-contained Python3.13.13/ptts0.7.1/NumPy2.5.4 payload staged at `Sidecars/bin/phonon/python`, about95.9MB. `Tools/Phonon/stage_python.py` copies only selected runtime/stdlib/package files from existing local installations, retaining licences/metadata/SBOM. No credentials or environment files are copied.
- Isolation probe passed with invalid `PYTHONHOME`/`PYTHONPATH` and PATH limited to System32; all Python paths and non-Windows loaded DLLs resolve inside the payload. The **copied executable passed all15 real CPU bridge tests**, evidence `Saved/PhononComparison/portable-bridge.json`. This is local relocation evidence, not a clean second-machine test.

Packaging now reads the selected GGUF from `DefaultGame.ini`, checks the pinned Gemma hash and requires the Phonon payload before staging. It no longer silently stages only Qwen. Default runtime paths resolve under packaged Sidecars, without Saved-folder/venv overrides. VC runtime files were copied from the installed Python runtime; retain normal redistribution terms/notices before public release.

Tooling follow-up `afe57bc` reconciles portable-runtime notices/metadata and verifies tokenizer, default voice, all three crew embeddings, runtime inventory hashes, isolated `._pth`, native ptts/NumPy dependencies and retained notices. Root refreshed existing metadata using `stage.py --metadata-only`; the actual updated main payload passed `stage_sidecars.ps1 -ValidateOnly` without copying any files. This validates staging prerequisites, not a packaged cold launch.

## Prevent misleading model tests

Llama readiness now checks `/v1/models` for the selected GGUF filename. A Qwen server already on the port is reported as `DIALOGUE_MODEL_MISMATCH` and is not used or replaced automatically. Only an absent endpoint triggers the owned llama launch. LLM/STT launch attempts are tracked separately. Missing selected files report `DIALOGUE_MODEL_MISSING`; commands can retain their authored/subtitle behavior, but no substitute LLM is called. Root found no8791 listener at its last check and will recheck before native startup.

Conversation and command JSON reports include `selected_model`; conversation reports additionally include `tts_backend`, `streaming_enabled` and `selected_model_ready`. Require both configuration and runtime evidence below.

## Native commands for the build owner

### Gemma reasoning-budget correction

The initial native command review passed **13/27** cases; a subsequent review with the model resident passed **18/27**. Neither run is accepted. Initial results are in main `Saved/PhononGemma/commands.json`; cold-start/deadline failures and content failures must remain distinct.

The captured held-out request in `Saved/PhononGemma/raw-heldout-response.json` confirms a content failure: despite `chat_template_kwargs.enable_thinking=false`, the server generated all 80 completion tokens as reasoning, returned empty content and stopped with `finish_reason=length`. The same prompt with `reasoning_effort="none"` and `reasoning_budget_tokens=0` returned the correct Ruby/starboard/man JSON in 28 completion tokens, with 235.296 ms combined server prompt/generation time, in `raw-heldout-bounded-response.json`. This is a single direct HTTP reproduction; cache reuse differs, so it is not a controlled model-speed comparison.

`DCDialogueRouting.cpp` now includes both explicit fields for LlamaCpp requests, retaining the existing template hint, production token/deadline limits and strict intent validation. The installed server fingerprint is `b12317-e0949d09b`. Its [Gemma parser](https://github.com/ggml-org/llama.cpp/blob/e0949d09b130707b3fa93d2cff616024231cf939/common/parsers/gemma4.cpp#L213) permits an optional thought block before structured output; the zero reasoning budget closes that route before it consumes the response allowance. The exact server version already enables Jinja by default, so omission of an explicit `--jinja` flag is not evidence of the failure's cause.

**The two-field fix is compiled and passes all 27 native cases in `commands3.json`, followed by the eight full conversations above.** Production deadlines and token limits were retained. Cold packaged launch remains separate validation.

After integrating all three candidate commits and compiling, use the dependency-complete main project, one process at a time. The supplied paths need no Phonon dev overrides now. Do not run these alongside the root's other native reviews.

```powershell
$editor = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
$project = 'E:/Projects/DirectorsCut/DirectorsCut/DirectorsCut.uproject'
# Real Gemma command/acknowledgement review, no A2F required for this arm.
& $editor $project '/Engine/Maps/Entry?game=/Script/DirectorsCut.DCDialogueReviewGameMode' -game -windowed -RenderOffscreen -unattended -DCDialogueProbe -DCNoACE -DCDialogueBackend=LlamaCpp '-DCDialogueReport=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononGemma/commands.json' '-abslog=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononGemma/commands.log'
# Eight actual conversations, three crews, generated streaming speech and faces.
& $editor $project /Game/DC/Maps/L_Ride -game -windowed -ResX=1600 -ResY=900 -DCNoMenu -DCEnableACE -DCConversationReview '-DCConversationReport=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononGemma/conversation.json' '-abslog=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononGemma/conversation.log' '-ExecCmds=t.MaxFPS 60'
# Three authored critical calls with real Phonon PCM/A2F; this alone does not test Gemma.
& $editor $project /Game/DC/Maps/L_Ride -game -windowed -DCNoMenu -DCEnableACE -DCVoiceSmokeTest '-abslog=E:/Projects/DirectorsCut/DirectorsCut/Saved/PhononGemma/critical.log' '-ExecCmds=t.MaxFPS 60'
```

Conversation acceptance: `DIALOGUE_READY backend=LlamaCpp model=gemma-4-E4B-it-Q4_K_M.gguf`, `TTS_BACKEND selected=Phonon31M`, producer ready24kHz, `PHONON_SPEECH_STARTED` for every spoken chunk, positive A2F samples per clip, and8 successful review rows whose selected model/TTS fields match. Inspect responses against live context for quality. Reject missing-model, fallback or failed-batch runs; do not call a metadata-only selection a successful test.

Cancellation/failure/reload remain separate required native tests: cancel while generating, sending and playing, then issue a fresh critical call; confirm epoch changes, no stale PCM/replayed prefix, native worker drain and healthy next speech. Kill only the owned producer after a played prefix and test recovery. The existing `DCACEIntegrationReview` requires the ACE dialogue backend and a Piper WAV, so **it cannot validate this LlamaCpp/Phonon combination without adaptation**. Keep the boundary explicit rather than passing the wrong harness. Full original checklist and known phrase-boundary limitation: [Phonon handoff](PHONON-INTEGRATION-15NOV.md).

## Shutdown correction and targeted probes

Compile follow-up `dec4d05` fixes the first native compiler findings and adds `-DCPhononLifecycleReview=Cancel` / `=Exit` on the normal voyage with `-DCNoMenu -DCEnableACE`. These probes start a long Rex line and wait for actual audio playback while native input is still pending. Cancel resets speech, queues a fresh Ruby critical call, and requires its audio/face lifecycle plus worker drain. Exit requests normal application exit at that same in-flight point. No `DCVoiceSmokeTest` flag is needed with these probes.

A follow-up exit barrier replaces the discarded background-thread future with registry-owned threads and completion events. Actor EndPlay remains asynchronous. `OnEnginePreExit` (fallback `OnPreExit`) closes new admission, signals cancellation, services required game-thread preparation/cleanup, waits for native and tracked cleanup completion, then joins finished threads and removes both delegates before ACE module unload. Never time out into unsafe unload: a stuck native driver can delay application exit. Require `PHONON_EXIT_DRAIN_COMPLETE` before module teardown and actual process exit with no orphan Python. Exercise initial preparation, paced input, cancellation/restart and buffered playback exit stages; the initial Exit probe specifically covers paced input while playing. Cancel, Exit, Failure and corrected Reload native evidence is recorded above; the first Reload failure and its verified correction are retained. Unsupported custom travel initiated inside a game-thread task with outstanding cleanup fails explicitly rather than nesting task execution or continuing into GC.
