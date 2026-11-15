# NVIDIA ACE dialogue alternative

Owner: AI NPCs chat, `codex/ace-dialogue`, isolated complete-content checkout `E:/Projects/DirectorsCut/ACE-Dialogue`.

Status: implementation and UE 5.8 Editor build complete; response-validation automation passed. Native GPU comparison and combined Audio2Face/voyage verification are pending. Do not replace the accepted playable launcher yet.

## Comparison contract

- `-DCDialogueBackend=LlamaCpp`: existing external llama-server.
- `-DCDialogueBackend=ACE`: NVIDIA ACE LLM v0.5 / Game Agent SDK Chat API, in process, separate CUDA context.
- Both use the same existing `Sidecars/models/Qwen3.5-4B-Q5_K_M.gguf`, command prompt, dialogue prompt and strict response validation. `-DCACEModel=<absolute path>` can override the ACE model for a separate experiment.
- Whisper speech recognition, Piper crew voices and Audio2Face lip-sync remain shared. This is an alternative intelligence runtime, not a replacement of the speech pipeline.
- The NVIDIA SDK itself uses llama.cpp/ggml internally. This comparison must not be described as a wholly unrelated inference engine or a Nemotron model.
- Default remains LlamaCpp while testing. No cloud account, API key or network inference is required.

## Integration and safeguards

The current official NVIDIA plugin is distributed for UE 5.7. `Tools/ACE/setup_llm_plugin.ps1` installs its source and reproduces the UE 5.8/safety changes. Download and source provenance are in `Docs/ThirdPartyACELLM/README.md`.

The stock plugin's RTX and GPU tiers hand Unreal's D3D device/queue to CUDA. The local patch forces its explicit CUDA tier with `ACEDeviceType_None` and null graphics pointers. Official SDK source confirms this retains GPU layer offload; runtime GPU evidence is still required. The old `useCIGforAI=0` flag alone does not configure this new plugin.

The adapter serializes requests, bounds queued work, validates structured intents, suppresses expired/previous-scene replies and preserves authored hazard calls. SDK chat inference has no hard cancellation or configurable per-request output-token cap. Application timeouts return the existing authored/parser fallback while the active native work finishes; live model/chat memory is not freed on a timeout. The SDK caps generation at 1,024 tokens or remaining context.

The plugin patch clears the previous response and propagates native failure, preventing a failed request from replaying an older command. A process-wide gate prevents overlapping inference against the shared model after level travel. Active task/data lifetime extends beyond the originating GameInstance until both native workers finish; response UObject creation and callbacks run on the game thread. Module teardown waits for workers and completion before releasing SDK resources. These changes require native restart/shutdown verification, not just compilation.

## Reproducible checks

- `DirectorsCut.Dialogue.ResponseValidation`: malformed responses, unknown crew, contradictory actions/stations/directions and valid intent normalization.
- `Tools/ACE/compare_dialogue.ps1`: sequential selected backends; 12 actual model-interpreted orders and three generated acknowledgements. Production timeouts, JSON outputs and latency per request; no canned fallback counted as a model success. Separate port8797 for the baseline test server.
- The comparison captures stdout/stderr as well as Unreal logs. Require actual GPU-layer-offload evidence, not just the selected-backend label.
- Run only during a coordinated idle GPU window. Do not overlap the integration chat's full-voyage testing.

New plugin DLLs use a newer cuBLAS than the Audio2Face package. Successful model-only checks do not establish that both systems work together: combined three-crew lip-sync, speech and visible-voyage validation is mandatory before integration.
