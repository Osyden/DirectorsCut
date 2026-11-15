# NVIDIA ACE LLM runtime notices and integration provenance

The optional ACE dialogue backend uses NVIDIA's **ACE LLM Unreal plugin v0.5**, its bundled **Game Agent SDK Chat API**, and its CUDA-enabled **llama.cpp/ggml** runtime. It replaces the external llama-server process for dialogue inference. It does not replace the underlying llama.cpp family of inference code, the game's crew logic, Whisper, Piper or Audio2Face.

Official download: <https://developer.download.nvidia.com/assets/ace/ue-plugins/ACE_LLM_5.7_v0.5.zip>

Downloaded archive SHA-256: `24e1aba890d6405fced0fc3fe587ca4b39cfb9070559f1f9cfade05790d58b79` (4,049,945,595 bytes).

The archive targets Unreal 5.5–5.7. Its plugin source is rebuilt locally for the project's UE 5.8.4. The descriptor's stale EngineVersion is removed; prebuilt Unreal binaries are excluded. SDK DLLs remain the vendor binaries. No NVIDIA account, cloud endpoint or API key is used.

## Licences

The archive's `LICENSE(Plugin).txt` and most source SPDX headers specify Apache-2.0. Its component/subsystem headers specify MIT, and its README's blanket MIT description does not match every source header. Retain both applicable notices. `Plugin-APACHE-2.0.txt` preserves the archive's license unchanged; `MIT-components.txt` retains the MIT notice for those files.

The Game Agent SDK is Apache-2.0, with separate third-party licences. `LICENSE.txt`, `NOTICE.txt`, `THIRD-PARTY-LICENSES.md` and `llamacpp-THIRD-PARTY-NOTICES.md` are copied unchanged from NVIDIA's official [Game Agent SDK repository](https://github.com/NVIDIA/game-agent-sdk), source revision `9aa6e1c59d9fd0ffda7d4cdc046f22cf85c00cd0`. These notices cover SDK code, llama.cpp, ggml, ONNX Runtime, OpenBLAS and CUDA runtime redistribution. CUDA DLLs are proprietary redistributables, not Apache-licensed code.

The comparison uses the existing `Qwen3.5-4B-Q5_K_M.gguf` model for both backends. [Qwen's official model card](https://huggingface.co/Qwen/Qwen3.5-4B) identifies Apache-2.0. The optional Q4_K_S model bundled in the vendor ZIP is retained in the local download but excluded from game staging to avoid duplicating model weights. Preserve the selected model's existing notices separately.

## Local source modifications

`Tools/ACE/setup_llm_plugin.ps1` reproduces these changes in the ignored local plugin:

- Rebuild the plugin's Unreal modules from source for this engine.
- Add an explicit `CUDA` backend and force `ACEDeviceType_None` graphics handoff. No D3D device or command queue is passed to the SDK. The existing A2F `-useCIGforAI=0` safeguard remains separate and necessary.
- Keep tasks registered with the game instance and mark their referenced chat/options/message objects for Unreal garbage collection.
- Poll inference events with a bounded wait; stop waiting when a failed producer has finished without an end event.
- Wait for active native workers before destroying model/context storage at process shutdown.
- Do not stage the unused bundled Q4 model.

The official SDK's `source/core/internal/model.cpp` only enables CUDA-in-Graphics when supplied a D3D12 device. Its `llama_interface.cpp` otherwise loads normal inference backends and sets `n_gpu_layers=-1`. That establishes the source-level route for local GPU inference without graphics interop. Runtime logs showing CUDA buffers and GPU layer offload are still required to validate the binary on this PC; a successful `ace_initContext` alone does not establish GPU use.

The public Chat API exposes no per-request cancellation or output-token limit. Current SDK source caps generation at the lesser of 1,024 tokens and the remaining context. A response deadline must suppress late replies while retaining the busy model until native work ends; it cannot honestly be called hard cancellation. Module shutdown can therefore wait for a current request to finish. External test watchdogs remain necessary for an unresponsive process.

The old Audio2Face plugin bundles cuBLAS 12.4.5 and this runtime bundles 12.8.3. Their files are retained at their original plugin paths rather than replacing the old plugin's dependencies. Both systems must be tested together in the packaged game before accepting the optional backend.
