# Next local test build: Phonon 31M + Gemma E4B

The user explicitly requested these as the **defaults for the next build**, superseding the earlier opt-in preference. Config now selects LlamaCpp with `gemma-4-E4B-it-Q4_K_M.gguf`, Phonon31M, speech streaming and a1-second A2F prebuffer. The previously selected playable is unchanged. Native acceptance is still pending; do not describe the new combination as tested in-game yet.

## Local payload ready

- Gemma user download found at `C:/Users/ejobse/Downloads/gemma-4-E4B-it-Q4_K_M.gguf`, copied to main `Sidecars/models`. Size4,977,171,584bytes and SHA256 `85a896a047553e842f25297ee5b031d64ff30147d9c4af17b1e4b394cd1fab87` match at both locations. No replacement download was attempted. Revision `bfc15c382204943c3a8fff0c750b94ae2364d7a3`; attribution/licence in `Docs/ThirdPartyGemma`.
- Phonon model, voices, bridge and notices are staged at main `Sidecars/bin/phonon`. Model and three voice hashes verified by `Tools/Phonon/stage.py`.
- Self-contained Python3.13.13/ptts0.7.1/NumPy2.5.4 payload staged at `Sidecars/bin/phonon/python`, about95.9MB. `Tools/Phonon/stage_python.py` copies only selected runtime/stdlib/package files from existing local installations, retaining licences/metadata/SBOM. No credentials or environment files are copied.
- Isolation probe passed with invalid `PYTHONHOME`/`PYTHONPATH` and PATH limited to System32; all Python paths and non-Windows loaded DLLs resolve inside the payload. The **copied executable passed all15 real CPU bridge tests**, evidence `Saved/PhononComparison/portable-bridge.json`. This is local relocation evidence, not a clean second-machine test.

Packaging now reads the selected GGUF from `DefaultGame.ini`, checks the pinned Gemma hash and requires the Phonon payload before staging. It no longer silently stages only Qwen. Default runtime paths resolve under packaged Sidecars, without Saved-folder/venv overrides. VC runtime files were copied from the installed Python runtime; retain normal redistribution terms/notices before public release.

## Prevent misleading model tests

Llama readiness now checks `/v1/models` for the selected GGUF filename. A Qwen server already on the port is reported as `DIALOGUE_MODEL_MISMATCH` and is not used or replaced automatically. Only an absent endpoint triggers the owned llama launch. LLM/STT launch attempts are tracked separately. Missing selected files report `DIALOGUE_MODEL_MISSING`; commands can retain their authored/subtitle behavior, but no substitute LLM is called. Root found no8791 listener at its last check and will recheck before native startup.

Conversation and command JSON reports include `selected_model`; conversation reports additionally include `tts_backend`, `streaming_enabled` and `selected_model_ready`. Require both configuration and runtime evidence below.

## Native commands for the build owner

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
