Current crew names: **Rex, Duncan and Ruby**. See [crew HUD and speech vocabulary update](CREW-UI.md) for the new names, portraits and fire-at-will recognition. Earlier verification records below retain their original names and build dates.

# Correction: incomplete ACE candidate (15 November, 11:02 GMT)

Do not use the 08:25 `2026-11-15_0825_ace-gpu-safe` package as a playable candidate. It was built from an isolated checkout missing ignored licensed ship, sky, ruins, audio and creature assets. Speech/face checks passed, but the HUD rendered over a black world. The saved 40-second screenshot and runtime missing-package warnings confirm the failure. Earlier claims of full-voyage validation for this candidate were too broad: they established ACE speech/face operation, not a complete rendered voyage or representative rendering load.

The complete combined package is `E:/Projects/DirectorsCut/Builds/2026-11-15_0927_final-polish/Windows/DirectorsCut.exe`. Its core scene was reviewed by the gameplay chat and a fresh 11:01 ACE check shows ship, sea and sky. See its `Saved/ACE/Smoke/20261115_110105_ACE/` evidence. It preserves the current main navigation work; unfinished maelstrom branches remain separate.

`Tools/verify_release_content.ps1` now checks essential dynamic assets and every configured game cook directory before packaging. It accepts main and rejects the incomplete ACE checkout. The voice smoke helper rejects missing core world packages and captures a fresh scene image for visual inspection. Voice completion alone cannot establish release readiness.

---

# GPU ACE crew dialogue — 15 November 2026

Owner: Codex submission/ACE chat, `codex/ace-comparison`.

The three purchased pirate crew faces now consume local NVIDIA Audio2Face Mark animation, driven by their Piper dialogue. Inference runs on the NVIDIA GPU using a separate CUDA context (`useCIGforAI=0`). No ACE cloud service or API key is configured. Whisper handles recognition; the existing local Qwen model interprets unfamiliar orders and rephrases replies. ACE supplies synchronized facial animation and audio playback, not the voice or language model.

## Runtime and controls

- GPU ACE is enabled by default. Hold **V** or the **middle mouse button** to speak an order. Existing typed orders still work.
- Finch uses British VCTK p226 (speaker 95), Marlow uses the northern-English male voice, and Odessa uses Scottish Alba. Pace/noise settings give distinct, restrained performances. See [voice notices](../Docs/ThirdPartyVoices/README.md).
- Replies are serialized: one Piper process and one ACE playback at a time. Short lines finish buffering before playback to avoid audio gaps under the voyage's rendering load. Gameplay commands execute immediately; speech follows the queue.
- `-DCNoACE` selects audio-only playback. Missing/failed local inference falls back to audio. `-DCNoLLM` retains deterministic command parsing and canned replies. `-DCNoVoice` suppresses the voice system for visual reviews.
- An explicit nonzero `-useCIGforAI=` is refused. Do not re-enable the shared CUDA/D3D context: see [crash evidence and safeguards](ACE-GPU-CRASH-15NOV.md).

The native face graph copies the body pose, applies ACE's ARKit curves, maps them through the MetaHuman pose asset, and retains each assembled face's RigLogic post-process. The captain keeps her purchased outfit; crew movement, station animation and hair restoration remain intact.

## Command fixes

The parser now uses complete words and normalized punctuation, understands plural guns/cannons/sails/ropes, prioritizes reports and cease-fire orders, and distinguishes "man port cannons" from firing them. "Fireworks" and "portrait" no longer trigger fire/port commands. Named reports answer from the named crew member. DCCrew movement, station and cannon gameplay are outside this change.

## Verification

- Editor and Windows Development game builds passed in isolation; main editor build passed with its original editor plugins retained.
- Main's two command suites passed: 38 deterministic cases and 13 synthesized speech transcripts recognized by Whisper. Evidence: `DirectorsCut/Saved/ACE/Commands-main.log`; speech fixtures were generated in the isolated checkout. Live microphone/device capture still needs a hands-on test.
- Real pirate face comparison showed nonzero curves and jaw rotation.
- Full-voyage separate-context GPU tests passed 18 lines, then 12 more with DLSS-SR=1 and buffered playback.
- Normal combined recognition/interpretation/voice/face flow passed, including "Odessa, report" and "Finch, keep our heading unchanged". Evidence: `ACE-Comparison/Saved/ACE/Full-voice-loop.log`.
- The Windows candidate at `Builds/2026-11-15_0825_ace-gpu-safe/Windows/DirectorsCut.exe` packaged successfully and passed nine crew lines with DLSS active. Evidence: its `Saved/ACE/Smoke/20261115_082859_ACE/runtime.log`.
- Standalone cooked-face comparison passed: Finch's baseline remained neutral; ACE produced 48 nonzero curve samples and a peak jaw rotation delta of 0.090380 radians. The captured pirate face was visually inspected. Evidence: the candidate's `Saved/ACE/Runs/20261115_083156_878_finch_confirmation/`.
- The standalone default-ACE Whisper/Qwen/Piper/face loop also passed both recorded report and unfamiliar steady-course commands, with normal exit and no GPU timeout. Evidence: candidate `Saved/ACE/Full-voice-loop.log`. Windows recorded zero new NVIDIA driver events between the reboot and the 08:34 verification.
- Before runtime logs were added the candidate measured 11,877,441,966 bytes across 606 files, including selected sidecars and the ACE model; this is an uncompressed folder size. It predates any later gameplay/art branch integration and is not yet the final submission selection.

## Reproduction and release handling

`Tools/ACE/smoke.ps1 -Mode ACE -Repeats 3` runs a bounded full-voyage test. Add `-Packaged -Executable <Windows/DirectorsCut.exe> -ProjectRoot <Windows/DirectorsCut>` for the standalone candidate. `-Mode Audio` verifies fallback. Helpers refuse to overlap another Unreal GPU job and terminate only the process they launched if a test times out.

`Tools/ACE/verify_spoken_orders.py` generates the speech fixtures. Run Unreal with `-NullRHI -ExecCmds="Automation RunTests DirectorsCut.Commands" -TestExit="Automation Test Queue Empty"` afterward. `Tools/ACE/compare.ps1 -AllowLocalInference` performs the separate-context face comparison; see [comparison notes](ACE-COMPARISON.md).

`Tools/package_isolated.ps1` cooks on CPU and disables only the failing unused-default-texture stripping optimization. It verifies that Source, Config and the project descriptor remain unchanged throughout packaging. It stages three selected voice models and notices rather than distributing every development voice. The existing `Tools/package.sh` now delegates to this helper; its Bash syntax check passed.

Vendor ACE plugins/model weights, mapping assets and purchased cast stay ignored by Git. `Tools/ACE/install_runtime.ps1` copies and verifies the vendor runtime and mapping dependencies from the isolated installation; main's local installation manifest is `Saved/ACE/installed-runtime.json` (350 files). Keep NVIDIA's plugin and model licences with distribution. A Git checkout alone does not contain these dependencies.

The compatibility patches compile ACE 2.5 for UE 5.8; they are not NVIDIA certification. Tests cover this workstation and installed driver. A fresh-machine test, live microphone test, sound-mix review and final release/licence reconciliation remain release gates.
