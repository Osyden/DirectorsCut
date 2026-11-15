# Local model comparison

Owner: AI NPCs chat. Requested after free conversation is validated. Keep the current working Qwen default during the existing release checks; all GPU tests must use the integration root's serialized slot.

Requested artifacts:

| Model | GGUF | Bytes | Publisher |
| --- | --- | ---: | --- |
| Current Qwen | Qwen3.5-4B-Q5_K_M.gguf | 3143656608 | Existing project artifact |
| Plain Gemma 4 E4B Instruct | gemma-4-E4B-it-Q4_K_M.gguf | 4977171584 | [Unsloth conversion selected by the user](https://huggingface.co/unsloth/gemma-4-E4B-it-GGUF) |
| Gemma-derived Nemotron 3.5 Bolt E4B | Nemotron-3.5-Bolt-E4B-Q4_K_M.gguf | 6180446208 | [NVIDIA official](https://huggingface.co/nvidia/NVIDIA-Nemotron-3.5-Bolt-E4B-4GB) |

The bundled legacy `nemotron-4-minitron-1-4b-instruct-dpo2_q4_0.gguf` is Nemotron-Mini, not a Gemma model. The actual current ACE dialogue model is Qwen. Bolt's `4GB` product name is not a measurement of this application's VRAM usage.

`Tools/ACE/compare_models.ps1` accepts existing local files, verifies the requested Gemma/Bolt sizes and SHA256 against pinned publisher revisions, and runs each selected model through the same ACE runtime, 4096-token configured context, 27 command/acknowledgement cases and eight full-voyage conversation cases. It records the game-module/executable hash, source revision, raw cases, latency summaries, GPU offload evidence and positive facial-animation samples. It never downloads files or switches to another model on failure. The final semantic-quality field remains unset until answers are reviewed against their recorded context.

Example once artifacts and the GPU slot are available:

```powershell
./Tools/ACE/compare_models.ps1 -ProjectRoot E:/Projects/DirectorsCut/DirectorsCut -GemmaModel '<local>/gemma-4-E4B-it-Q4_K_M.gguf' -BoltModel '<local>/Nemotron-3.5-Bolt-E4B-Q4_K_M.gguf'
```

Use `-Models Gemma -Phase Conversation` to run a single remaining phase. Do not execute while another game/editor review owns the GPU. Pin checks are intentionally strict; update the provenance explicitly before changing artifact revisions.

Comparison limits: Qwen is the existing Q5_K_M configuration; the two requested alternatives are Q4_K_M with different tensor mixes and model sizes. ACE does not enforce the per-request token caps or JSON grammar available in the llama.cpp path. This is a comparison of candidate production configurations, not equal-parameter or equal-token-throughput research. Measure actual memory and latency rather than inferring them from file size or the E4B label. NVIDIA's optional conversational-mode template flag is not exposed by the current ACE wrapper; default non-thinking conversation is the initial test configuration.

Status: runner syntax and pure latency calculations checked; runner has not been executed. Alternative artifacts were not downloaded and no alternative-model speed/accuracy/quality result exists yet. The current Qwen free-conversation baseline and prompt correction are tracked in CREW-CONVERSATION-15NOV.md. No default model switch or package-model addition has been made.
