# Initial voice-control hint

Owner: voice_audio, `codex/voice-onboarding-hint`. Scope: ADCHUD in DCGame.cpp/.h; DCVoice remains owned by the AI NPCs chat.

Shows **Hold V to talk to your AI crew** below the hull/alerts during the first 60 gameplay seconds. The clock begins on the first gameplay HUD frame after Start Game, uses world gameplay time (not ride time or wall time), and resets while the menu is visible or when the HUD is recreated on restart. Paused time and menu waiting do not consume the minute.

Only `ADCVoice::HasIssuedSuccessfulVoiceCommand()` dismisses the hint early. The AI NPCs implementation of that accessor must be integrated before compiling this commit; typed commands, conversation, and merely pressing V must not set it. Dismissal stays latched for the run.

The dark, compact plate is separated from bottom speech/interaction UI and the right-hand crew cards. Projected station labels skip its rectangle. Menu and `-DCNoHUD` suppress it. No existing interaction controls are removed.

CPU source checks pass for menu/no-HUD gate ordering, gameplay clock, voice-only accessor, exact text, 60-second expiry, reset, and station-label exclusion. `git diff --check` passes. No Unreal build or native visual test was run by this agent.

Native verification: wait in the menu for over 60 seconds, start and confirm the hint remains; issue typed orders and confirm it remains; give a successful microphone command and confirm it disappears. A fresh run without successful voice input must retain it until 60 gameplay seconds, then remove it. Restart must show it again. Logs: `VOICE_HINT shown gameplay=...`, then exactly one `VOICE_HINT dismissed reason=voice_command|timeout elapsed=...` per run.
