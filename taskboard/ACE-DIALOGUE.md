# NVIDIA ACE dialogue alternative

Owner: AI NPCs chat, `codex/ace-dialogue`, isolated complete-content checkout `E:/Projects/DirectorsCut/ACE-Dialogue`.

Status: implementation, UE 5.8 builds, CPU command/response checks, both native GPU dialogue comparisons and combined Editor/standalone voice/reload verification passed. Validated comparison package: `E:/Projects/DirectorsCut/Builds/2026-11-15_1617_ace-dialogue/Windows/DirectorsCut.exe`. The accepted playable launcher is unchanged.

## Comparison contract

- `-DCDialogueBackend=LlamaCpp`: existing external llama-server.
- `-DCDialogueBackend=ACE`: NVIDIA ACE LLM v0.5 / Game Agent SDK Chat API, in process, separate CUDA context.
- Both use the same existing `Sidecars/models/Qwen3.5-4B-Q5_K_M.gguf`, command prompt, dialogue prompt and strict response validation. `-DCACEModel=<absolute path>` can override the ACE model for a separate experiment.
- Whisper speech recognition, Piper crew voices and Audio2Face lip-sync remain shared. This is an alternative intelligence runtime, not a replacement of the speech pipeline.
- The NVIDIA SDK itself uses llama.cpp/ggml internally. This comparison must not be described as a wholly unrelated inference engine or a Nemotron model.
- Default remains LlamaCpp while testing. No cloud account, API key or network inference is required.

## Integration and safeguards

The current official NVIDIA plugin is distributed for UE 5.7. `Tools/ACE/setup_llm_plugin.ps1` installs its source and reproduces the UE 5.8/safety changes. Download and source provenance are in `Docs/ThirdPartyACELLM/README.md`.

The stock plugin's RTX and GPU tiers hand Unreal's D3D device/queue to CUDA. The local patch forces its explicit CUDA tier with `ACEDeviceType_None` and null graphics pointers. Native logs verify **33/33 model layers offloaded to GPU**, with a 2,987.56 MiB CUDA model buffer. The old `useCIGforAI=0` flag alone does not configure this new plugin.

The adapter serializes requests, bounds queued work, validates structured intents, suppresses expired/previous-scene replies and preserves authored hazard calls. SDK chat inference has no hard cancellation or configurable per-request output-token cap. Application timeouts return the existing authored/parser fallback while the active native work finishes; live model/chat memory is not freed on a timeout. The SDK caps generation at 1,024 tokens or remaining context.

The plugin patch clears the previous response and propagates native failure, preventing a failed request from replaying an older command. A process-wide gate prevents overlapping inference against the shared model after level travel. Active task/data lifetime extends beyond the originating GameInstance until both native workers finish; response UObject creation and callbacks run on the game thread. Module teardown waits for workers and completion before releasing SDK resources. These changes require native restart/shutdown verification, not just compilation.

## Reproducible checks

- `DirectorsCut.Dialogue.ResponseValidation`: malformed responses, unknown crew, contradictory actions/stations/directions and valid intent normalization.
- `Tools/ACE/compare_dialogue.ps1`: sequential selected backends; 24 actual model-interpreted orders (including 12 held-out paraphrases) and three generated acknowledgements. Production timeouts, JSON outputs and latency per request; no canned fallback counted as a model success. Separate port8797 for the baseline test server.
- `Tools/ACE/verify_integration.ps1`: six generated lines through all three Piper voices and Audio2Face, positive audio/animation sample evidence, pending-request cancellation on a scene change, and another three-crew cycle after an actual level reload. Includes rendered-world screenshots.
- The comparison captures stdout/stderr as well as Unreal logs. Require actual GPU-layer-offload evidence, not just the selected-backend label.
- Run only during a coordinated idle GPU window. Do not overlap the integration chat's full-voyage testing.

New plugin DLLs use a newer cuBLAS than the Audio2Face package. Successful model-only checks do not establish that both systems work together: combined three-crew lip-sync, speech and visible-voyage validation is mandatory before integration.

## Native evidence, 15 November

Final standalone comparison reports are under `Saved/ACEDialogue/20261115_161953/ACE-comparison.json` and `LlamaCpp-comparison.json` in the ACE-Dialogue checkout.

| Runtime | Passed | Ready after | Mean order | Mean acknowledgement |
| --- | --- | --- | --- | --- |
| ACE | 27/27 | 2.21 seconds | 226 ms | 159 ms |
| llama.cpp | 27/27 | 6.15 seconds | 180 ms | 204 ms |

These are single local runs, not a broad performance benchmark. Ready time includes each backend's startup/health polling. ACE uses a 2,048-token context; the existing llama server uses 4,096, while these individual prompts fit both. One earlier llama.cpp run timed out on its first two requests before subsequent responses became fast; repeats passed all cases. Both initial implementations exposed ambiguous crew-array outputs, so the shared prompt now specifies flat string fields and distinguishes assigning guns from firing them. Strict validation remains unchanged.

Combined Editor evidence: `Saved/ACEDialogue/combined_20261115_161431/integration.json` and logs. Six generated lines passed Piper/Audio2Face with positive samples, scene cancellation and level reload, then exited normally. The settled `Saved/Screenshots/Auto/shot_0060.png` shows the ship, crew HUD and rendered ocean. An earlier test-harness run incorrectly counted Unreal's temporary startup world, failed immediately and stalled during exit; its owned process was stopped and the harness now creates only for L_Ride. No display-driver/reset/WHEA/Kernel-Power events were present in the checked System log window.

Final standalone combined evidence: `Saved/ACEDialogue/combined_20261115_162236/integration.json` and stdout/stderr/runtime logs. Six generated lines, positive facial/audio samples, cancellation, reload and normal exit all passed after removing the unused legacy GPT model. Its settled screenshot is inside the packaged game's `Saved/Screenshots/Auto/shot_0060.png`.

`stage_sidecars.ps1` now removes only the generated package's unused 2,567,624,064-byte legacy Nemotron GGUF via `prune_unused_gpt_model.ps1`. The original vendor source/model, all DLLs, A2F data, selected Qwen model and required notices are preserved. No second Qwen quantization is shipped.

## Local comparison launchers

`Play DirectorsCut ACE.cmd` and `Play DirectorsCut LlamaCpp.cmd` use the same tested package via the separate `Saved/Release/ACEComparison.json` marker. They run the GPU-idle check and select the dialogue backend; Whisper, Piper, Audio2Face and gameplay stay shared. The existing `Saved/Release/Current.json` and normal launcher are not changed. These are local comparison builds, not an itch.io upload.
