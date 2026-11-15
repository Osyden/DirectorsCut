# Streaming crew speech candidate

Branch: `codex/speech-streaming`, source worktree `E:/Projects/DirectorsCut/Speech-Streaming`, based on main `ded0760`. AI NPCs chat owns the voice/dialogue helpers and review changes. Main and the selected 20:48 playable are unchanged.

## Behavior

Opt in with `-DCStreamSpeech`. Conversational LlamaCpp HTTP SSE and ACE token callbacks feed a bounded sentence/clause assembler. It starts Piper as soon as a usable phrase arrives, prepares at most one subsequent WAV while the current clip plays, and starts Audio2Face progressively after a one-second prebuffer. `-DCStreamPrebuffer=1` can be varied between 0.5 and 2.5 seconds during controlled testing. This is phrase-pipelined Piper, not raw PCM streaming within a Piper invocation.

Structured command JSON still waits for complete validation before executing. Authored reports and order acknowledgements use the same progressive A2F playback/one-ahead synthesis when the flag is enabled. The normal no-flag behavior retains full-clip buffering. Existing paced A2F input, separate CUDA context and British/Scottish voices remain in place.

Queues are limited to eight chunks for a whole 40-word/220-character response, one Piper process and one ready clip. The parser rejects malformed/truncated SSE, tool calls, rewritten text, metadata and broken Unicode. Final tails flush exactly once. Short A2F clips still lower the buffer on successful send completion. Scene/menu/restart reset invalidates the epoch and clears model callbacks, queued text, pending synthesis, prepared clips and playback. Failed continuations discard unplayed chunks and remove the affected model-history turn; the full reply is never replayed after partial speech.

The SDK moves its audio-send delegate into a queued game-thread task, so removing the original binding alone cannot cancel it. A request-specific UObject relay verifies owner, epoch and request serial before forwarding. A two-frame drain after Stop prevents a prior component ENDING tick from being mistaken for the next streamed clip.

## Verification status

Source/API and lifecycle review only, plus `git diff --check`. Automation coverage has been added for SSE framing/limits, phrase boundaries/final flush, batch failure/cancellation and stale callbacks. **No Unreal compile, automation run, model run or native/GPU review has occurred for this candidate.** The integration chat has explicitly reserved the GPU/engine lane for the human playtest. No latency improvement or underrun-free playback is claimed yet.

The conversation review now tracks complete batches instead of comparing each individual clip to the entire answer, and records observed early synthesis/playback timing and overlap. Existing stable review behavior remains available without the flag. The old comparison script's single-clip log timing parser is not suitable for streaming timings; use the new conversation JSON and streaming log markers.

## Commands after the integration owner releases the lane

Integrate the candidate into the dependency-complete main project or a fully provisioned validation worktree first. This source-only worktree has no installed plugin/sidecar payload. The following use main's installed dependencies and must not run during the human playtest:

```powershell
& 'E:/Utilities/UE_5.8/Engine/Build/BatchFiles/Build.bat' DirectorsCutEditor Win64 Development '-Project=E:/Projects/DirectorsCut/DirectorsCut/DirectorsCut.uproject' -WaitMutex

& 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'E:/Projects/DirectorsCut/DirectorsCut/DirectorsCut.uproject' -unattended -nop4 -nosplash -NullRHI '-ExecCmds=Automation RunTests DirectorsCut.Dialogue' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=E:/Projects/DirectorsCut/DirectorsCut/Saved/SpeechStreaming/Automation'

& 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' 'E:/Projects/DirectorsCut/DirectorsCut/DirectorsCut.uproject' /Game/DC/Maps/L_Ride -game -windowed -ResX=1600 -ResY=900 -DCNoMenu -DCEnableACE -DCStreamSpeech -DCStreamPrebuffer=1 -DCDialogueBackend=LlamaCpp -DCConversationReview '-DCConversationReport=E:/Projects/DirectorsCut/DirectorsCut/Saved/SpeechStreaming/llama.json' '-abslog=E:/Projects/DirectorsCut/DirectorsCut/Saved/SpeechStreaming/llama.log' '-ExecCmds=t.MaxFPS 60'
```

Repeat the native command sequentially with `-DCDialogueBackend=ACE` and separate report/log paths, then run the existing command/voice/reload reviews with the streaming flag. Do not run two inference reviews together.

Acceptance requires early phrase synthesis, playback before complete A2F input where the clip exceeds the prebuffer, positive face samples for every chunk, whole-response completion, unchanged order counts, and listening/capture under fog load to check continuity. The SDK's private underrun counter is not exposed; completion markers alone do not prove smooth audio. Also exercise menu/restart/phase change with model, Piper, ready and playing work in flight; then issue a critical authored report and confirm no stale continuation. Exercise model and TTS failure after a played prefix, and very short final tails. Keep the flag off in the release until those checks pass.
