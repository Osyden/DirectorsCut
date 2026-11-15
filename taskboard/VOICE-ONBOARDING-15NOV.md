# Initial voice-control hint

Owner: voice_audio, `codex/voice-onboarding-hint`. Scope: ADCHUD in DCGame.cpp/.h; DCVoice remains owned by the AI NPCs chat.

Shows **Hold V to talk to your AI crew** below the hull/alerts during the first 60 gameplay seconds. The clock begins on the first gameplay HUD frame after Start Game, uses world gameplay time (not ride time or wall time), and resets while the menu is visible or when the HUD is recreated on restart. Paused time and menu waiting do not consume the minute.

Only `ADCVoice::HasIssuedSuccessfulVoiceCommand()` dismisses the hint early. The AI NPCs implementation of that accessor must be integrated before compiling this commit; typed commands, conversation, and merely pressing V must not set it. Dismissal stays latched for the run.

The dark, compact plate is separated from bottom speech/interaction UI and the right-hand crew cards. Projected station labels skip its rectangle. Menu and `-DCNoHUD` suppress it. No existing interaction controls are removed.

CPU source checks pass for menu/no-HUD gate ordering, gameplay clock, voice-only accessor, exact text, 60-second expiry, reset, and station-label exclusion. `git diff --check` passes. No Unreal build or native visual test was run by this agent.

Root native acceptance: the actual post-STT command path retained the hint for typed commands, conversation, rejected commands and WAV/file input, then dismissed it after accepted microphone-provenance dispatch at 5.14 seconds. This exercises microphone provenance after transcription, not physical microphone hardware. A separate fresh run retained the visible hint at 58 seconds and logged `VOICE_HINT dismissed reason=timeout elapsed=60.00`; the 61-second frame confirms removal. Evidence: `Saved/FinalCommands/Native/Execution/runtime.log` and `Saved/LatePolish/Native/Intro/`.

Menu clock/reset ordering is source-checked; a separate minute-long menu wait and hardware microphone run were not performed. Logs: `VOICE_HINT shown gameplay=...`, then exactly one `VOICE_HINT dismissed reason=voice_command|timeout elapsed=...` per run.
