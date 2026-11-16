# Approved thunderous ramp landing sound

Owner: trailer chat, `codex/landing-thunder-approved`, based on main d02c35b. Human approved the synced audition and explicitly requested the same sound in game.

`ADCAudio::LandingImpact` now plays dedicated `/Game/Sonniss/landing_thunderous` once at native pitch and gain0.85 multiplied by game master volume and speech ducking. Its tail remains tracked by OneShots, so later dialogue still ducks it. The old pitched water impact and procedural timber hit are removed from this event. CannonSplash, Boom, OnShipHit and shared water_impact are unchanged. Missing dedicated content logs a warning; it must be cooked before distribution.

The source is the approved4.4-second stereo48kHz composite: initial recorded thunder crack, low thunder body and water splash. The local source is converted from audition PCM24 to import PCM16 without gain/pitch change. Source hash: `1902c96ad5cb935d038ec79b6598f6ecf2187f42a08f24e54482f2618ce87bfc`.

Sources/provenance:
- Thunder: existing Sonniss GDC2026 Bundle2, Epic Stock Media Public Spaces, `STORM_Texas Rain Thunder Initial Crash Boom Storm 01 Clap Lightning_ESM_CPS.wav`, excerpt3.70-8.30s with filtered lower body and tail fades.
- Water: Big Water Splash by qubodup, https://freesound.org/people/qubodup/sounds/442773/, CC0, pitch0.85, layered65ms after thunder. Freesound page and downloaded preview are retained under Trailer/audio/auditions/splash.
- Exact FFmpeg filter and source metadata: `DirectorsCut/Saved/AudioImport/landing-provenance.json`; approved master: `Trailer/audio/auditions/splash/D-Thunderous-landing.wav`.
- Licensed raw/composite recordings and the imported asset remain local/ignored, matching existing Content/Sonniss policy; no source audio is committed to the public repo.

Import completed in the main project with its current editor module using this branch's `Tools/import_landing_thunder.py`. It does not require compiling the new audio hook. Command: UnrealEditor-Cmd.exe DirectorsCut.uproject -run=pythonscript -script=E:/Projects/DirectorsCut/Landing-Thunder-Approved/Tools/import_landing_thunder.py -unattended -nosplash -nullrhi. Script checks source hash, PCM format/duration, asset type, two channels and4.4s duration; saves non-looping SoundWave, volume/pitch1, compression quality90. Validation JSON and log: `DirectorsCut/Saved/AudioImport/landing-import-validation.json`, `landing-import.log`. Commandlet exited0 with0 errors. Existing /Game/Sonniss packaging rule includes the dedicated asset.

Root owns remaining integration, native landing check and fresh cook/package. Expected runtime: Audio recording landing_thunderous loaded, one LANDING_AUDIO asset=landing_thunderous gain=0.5525 at default master0.65, pitch1, duration4.4. No original water/timber stack. Accepted0007 package remains unchanged. Engine import slot released after commandlet exit.
