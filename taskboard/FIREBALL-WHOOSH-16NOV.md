# Selected isolated fireball hit sound

Owner: voice_audio, `codex/fireball-whoosh-impact`, base `39f875a`. Root owns engine import/build/cook/native review. No engine or GPU job was run by this agent.

The user selected `Isolated-Flame-Whoosh.mp3` as the fireball **hit** sound. The game asset is prepared directly from the original lossless `E:/Projects/DirectorsCut/AudioAuditions/Fireball/fire.wav`, using the same `loudnorm=I=-18:TP=-3:LRA=7` normalization. No MP3 decoding, mixing, pitch shift, or time stretch. Output is stereo PCM16/48 kHz, 2.483958 seconds, `/Game/Sonniss/fireball_flame_whoosh`.

Exact provenance: **Epic Stock Media — Elemental Mutation Whooshes and Impacts**, `FIREWhsh_Whoosh Fire Deep Growl Monster Saturated Crisp 03_ESM_EMWI.wav`, from `Sonniss.com-GDC2026-GameAudioBundle2of5.zip`. Epic Stock Media/Sonniss is already credited in `Docs/ArtCredits.md`. Licensed source/WAV/imported asset remain local/ignored, as with existing Sonniss recordings.

- Original SHA256: `344c8a9a31205fd5842feea827a491d89528f7db1ee5b1ffd4a3a018e23395ef`.
- Prepared WAV SHA256: `4ffe7ac94925bbce1d90829435dd9ace69ec8c2a430ae9577b0d19e0d495cf6b`.
- Prepared files: `E:/Projects/DirectorsCut/Fireball-Whoosh/Saved/AudioImport/fireball_flame_whoosh.wav` and adjacent `fireball-whoosh-provenance.json`.

`ADCAudio::FireballImpact` applies the unchanged normal DamageShip call while a scoped flag suppresses only ADCAudio's synchronous generic OnShipHit water/timber response. The normal gameplay broadcast and trauma remain. It then plays the dedicated whoosh once at pitch1, gain1 × master volume (default0.65), with ongoing speech ducking and tracked cleanup. Early creature fireballs retain1.2 damage; Kraken fireballs retain2.0. Their additional Boom water/thunder calls are removed. Statue beams, other impacts and `landing_thunderous` are unchanged. Missing content logs a warning without restoring the rejected impact layers.

## Root integration

Copy both prepared files into main `Saved/AudioImport`, integrate source, then run the selective script with the existing editor module (the new C++ method is not needed for import):

```powershell
& 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'E:/Projects/DirectorsCut/DirectorsCut/DirectorsCut.uproject' -run=pythonscript -script='E:/Projects/DirectorsCut/DirectorsCut/Tools/import_fireball_whoosh.py' -unattended -nosplash -NullRHI -nosound
```

`Tools/prepare_fireball_whoosh.py --ffmpeg E:/Projects/DirectorsCut/DirectorsCut/Saved/MusicTools/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v9.0.2.exe` reproduces the WAV and provenance from the pinned original. The import script verifies hash/format/duration, imports only this asset, sets non-looping/volume1/pitch1/compression90, and writes `Saved/AudioImport/fireball-whoosh-import-validation.json`. Existing `/Game/Sonniss` always-cook rule includes it; a fresh cook is required.

CPU checks: both scripts parse; WAV format/duration and non-clipping peak pass; source checks confirm one damage/one sound/native pitch, scoped generic suppression, unchanged statue Boom and removed Kraken Boom. `git diff --check` passes. Native verification remains root-owned: force an actual early fireball and Kraken fireball hit, require exactly one `FIREBALL_AUDIO asset=fireball_flame_whoosh ... pitch=1 ... generic_layers=0` per impact with the respective damage and fire, listen for the isolated whoosh, then confirm statue/landing audio still use their existing paths.
