# Earlier horizon warning

Owner: voice_audio, `codex/early-horizon-warning`, based on `ac37b0c`. Only DCRideDirector warning timing changes; approved landing audio remains separately owned. No engine/build/model/GPU jobs while the user playtests.

The selected 00:07 build's human log records Start Game at `00:27:10.324`, the horizon report at `00:27:52.343` (42.019 gameplay seconds), and actual ACE playback at `00:27:56.931` (46.607 seconds). Synthesis/startup adds 4.588 seconds in that run. The storm grows from 6 to 44 seconds, so the line is heard after the wall has fully grown, despite the nominal Fog phase beginning at 58 seconds.

Move the first horizon report from 42 to **22 seconds**, twenty seconds earlier. The optional idle-crew nudge moves from 17 to 8 seconds, so its longer line does not occupy the new warning's admission window. Existing wording and Fog-entry call remain unchanged.

The horizon report retries a crew-report cooldown only until 24 seconds, latches once accepted, and retains the existing six-second speech-start deadline. Its additional producer guard permits new playback only in open sea before 30 seconds; a late/seeked warning therefore cannot appear inside the front. An already-started sentence may finish while still in open sea. `Seek(0)` resets the hint stage; late seeks skip the early-only prompts. Menu time does not advance RideTime.

Lightweight timing checks read the production trigger/window constants and evaluate 24 combinations of 20/60/120 fps, 0/1.5-second report cooldown and 0/1.8/4.588/6-second speech delay. Every admitted warning begins before 30 seconds, over 16 seconds earlier than the observed old playback; delays outside the window are rejected. Open-sea continuation/reset source checks and `git diff --check` pass. This is not native playback validation.

Root follow-up after the user releases testing: start normally and require one `HORIZON_WARNING queued ride=22.xx latest_start=30.00`, followed by the existing Phonon/ACE line before 30 seconds. Leave the menu open first to verify gameplay-relative timing; restart for the same one-per-run behavior. With no player orders, confirm the optional 8-second nudge finishes ahead of the warning. Native visual/audio review should confirm the horizon still appears distant when spoken.
