# Local ACE comparison — 14 November 2026

Owner: Codex submission chat. Branch: `codex/ace-comparison`.
Checkout: `E:\projects\directorscut\ACE-Comparison` (based on main `cf49b3b`).

This document records the original 14 November comparison. On 15 November the isolated checkout gained real pirate-face integration, serialized gameplay replies, grounded British/Scottish voices and command tests. DLSS is now enabled. Editor-agent tooling remains disabled here: **do not merge this project descriptor wholesale into main**. See [the crash report and current safeguards](ACE-GPU-CRASH-15NOV.md) before GPU testing; the original shared-context path is blocked.

## What is installed

- NVIDIA ACE base plugin `2.5.0-20250614-2282`, official UE 5.6 archive, rebuilt locally for UE 5.8.4.
- NVIDIA local Mark regressive provider plugin `2.4.0-20250610-2270`, model 2.3, 30 FPS, explicit provider `LocalA2F-Mark-AR`.
- NVIDIA Kairos sample's MetaHuman `Face_Archetype` reference head and `Face_AnimBP` (already contains `Apply ACE Face Animations`). This is a grey reference head, not a purchased pirate or a finished crew replacement.
- Nine locally generated Piper WAVs: three identical lines each for Finch, Marlow and Odessa. WAV format and model/audio hashes are recorded in `Saved/ACE/Fixtures/manifest.json`.

The harness never chooses `Default` or a remote provider, and fails explicitly if the requested local provider is absent. No cloud endpoint or API key is configured for this experiment. The optional cloud-capable modules supplied by the vendor are still part of the plugin package.

## Run the comparison

Double-click `Tools\ACE\Compare-Voices.cmd` for the audio-only baseline. Use the explicit PowerShell opt-in below for the GPU comparison. Both modes use the same prerecorded line, repeated twice in each process. The default character is now the purchased Finch pirate. Finish other Unreal GPU jobs first; the helpers refuse overlapping tests.

From PowerShell in this checkout:

```powershell
./Tools/ACE/compare.ps1 -Mode Baseline -Show
./Tools/ACE/compare.ps1 -AllowLocalInference -Show -Clip odessa_warning
./Tools/ACE/compare.ps1 -Mode ACE -AllowLocalInference -Show -Clip marlow_quiet
```

Omit `-Show` for offscreen automation. `-CurvesOnly` skips the character. `-Character '/Game/.../BP_Name.BP_Name_C'` selects another assembled pirate; the native face graph applies ACE curves through the MetaHuman mapping pose and existing RigLogic post-process.

Each invocation saves `Saved/ACE/Runs/<timestamp>_<clip>/` with:

- Baseline and ACE `runtime.log`, `result.csv`, facial `curves.csv`, and `gpu.csv`. The unnumbered CSVs contain the last replay.
- Per-replay `result_01.csv`, `curves_01.csv`, `playback_01.png`, and corresponding `02` files. Use `-Repeats 1` for a single replay or up to nine for repeated tests.
- A combined `comparison.csv` after both succeed.
- Explicit failure status rather than treating subtitles or silent fallback as an ACE pass.

Baseline plays the WAV using Unreal's procedural audio, as the game's current pipeline does. ACE consumes that same WAV asynchronously and owns synchronized audio/face playback. The harness does not double-play Piper audio during the ACE run. It exits with failure if inference fails, never starts/ends playback, or produces no nonzero facial curves.

## Measurement limits

The baseline start timestamp is the audio playback **request**; ACE uses the plugin's playback-start event. These are not measurements of speaker/device latency and their difference is not a precise end-to-end latency benchmark. Piper timings in the fixtures manifest measure standalone process startup plus synthesis, not gameplay or ACE time.

Frame statistics cover playback in a small reference-head scene, not the voyage. The immediate screenshot readback frame is excluded from the summary. GPU memory/utilization samples cover the whole workstation, including other agents' editors and builds. The first ACE run may include model initialization/cache construction. Listen for synchronization and inspect facial motion before drawing quality conclusions.

## UE 5.8 compatibility changes

`Tools/ACE/patch_plugin.py` applies reproducible changes only to the ignored vendor copies:

1. Remove the UE 5.6 descriptor pin and mark both plugins as source builds. This permits rebuilding; it does not assert NVIDIA certification for UE 5.8.
2. Port ACE's old config-section migration away from removed `FConfigFile::FindOrAddSection`, preserving existing/new section entries.
3. Infer the JSON iterator element type in Omniverse LiveLink, accommodating UE 5.8's shared-string JSON keys.

The unmodified archives remain outside the repository. Plugin source/assets and model weights are excluded from Git. Keep their accompanying licence and attribution files with them. In particular, these downloaded model bundles contain a NVIDIA Models Community License PDF; do not assume every component is covered solely by the base plugin's MIT licence.

## Reproduce preparation

Use Python 3.11+ for the standard-library scripts:

```powershell
python Tools/ACE/prepare_fixtures.py
python Tools/ACE/download_model.py --output E:/projects/directorscut/Downloads/ace
python Tools/ACE/patch_plugin.py
./Tools/ACE/preflight.ps1
& E:/Utilities/UE_5.8/Engine/Build/BatchFiles/Build.bat DirectorsCutEditor Win64 Development -Project=E:/projects/directorscut/ACE-Comparison/DirectorsCut.uproject -WaitMutex -NoHotReloadFromIDE
```

The model downloader fetches only the Mark UE 5.6 member from the 8.3 GB multi-version archive using resumable byte ranges, then verifies the member's size and CRC. Extract that resulting ZIP under `Plugins` before patching/building. Base plugin and sample are obtained through NVIDIA's developer download links. `Sidecars/bin` and `Sidecars/models` are junctions to the existing installation and are only read; generated audio lives in this checkout's `Saved` directory. Compiled plugin copies are independent, not shared with the main checkout.

## Verification status

- Baseline UE 5.8.4 editor target: built successfully.
- ACE base and local Mark provider: rebuilt successfully for UE 5.8.4.
- Baseline audio/curve-only replay: passed.
- MetaHuman reference-head baseline: passed; camera orientation adjusted following screenshot inspection.
- End-to-end local ACE and matched final A/B: **passed**, including a second replay in the same process. ACE's animation curve and reference jaw bone both change; the baseline remains neutral.
- Production crew integration, full voyage performance and packaged distribution: not done by this experiment.

Final paired evidence: `Saved/ACE/Runs/20261114_173625_502_odessa_warning/`. The fixed 4.49-second Odessa clip was replayed twice in each mode. All four runs exited successfully.

| Observation | Baseline, second replay | ACE, first replay | ACE, warm second replay |
| --- | ---: | ---: | ---: |
| Start timing (see limitations above) | 0.15 ms request | 3,029 ms event | 511 ms event |
| Mean playback frame time | 3.18 ms | 4.01 ms | 4.39 ms |
| Nonzero sampled facial-curve frames | 0 | 79 | 86 |
| Peak reference jaw rotation change | 0 | 0.103 radians | 0.098 radians |

The very first Finch inference before model caches had been exercised took 8.29 seconds to begin playback. Preloading/warming before the voyage would be needed for production. These are single-workstation observations, not minimum hardware guidance or a statistically controlled performance benchmark.

Visual verification: the baseline renders a neutral reference head; ACE produces mouth/jaw deformation. Playback synchronization still needs a human listen. The older NVIDIA sample emits a nonfatal missing sample-controller warning from `Face_PostProcess_AnimBP` and old-asset version warnings; the measured facial animation still works. Original voice synthesis, command understanding, and crew gameplay are not being compared or replaced here.

## Sources

- [NVIDIA ACE downloads](https://developer.nvidia.com/ace-for-games)
- [Audio2Face Unreal 2.5 documentation](https://archive.docs.nvidia.com/ace/ace-unreal-plugin/2.5/ace-unreal-plugin-audio2face.html)
- [Facial animation setup](https://archive.docs.nvidia.com/ace/ace-unreal-plugin/2.5/ace-unreal-plugin-animation.html)
- [Official Audio2Face repository](https://github.com/NVIDIA/Audio2Face-3D)

Jam implication: this optional comparison is not needed to retain the existing DLSS route for Render Royalty eligibility. Do not add the unused NVIDIA language-model payload or the whole experiment to the jam upload.
