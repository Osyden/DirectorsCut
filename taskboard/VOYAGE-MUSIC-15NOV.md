# Voyage music

Root owns DCAudio, music import and the build/GPU slot for this pass on main.

- User's `../Music/the fog.mp3`: 224.4 seconds, stereo 48 kHz. Starts when the new voyage starts, with a 1.5-second fade in; remains silent on the menu.
- User's `../Music/the caldera.mp3`: 298.76 seconds, stereo 48 kHz. Starts 20 seconds before the Slide phase (currently ride 220, 3:40). Four-second linear crossfade fits the first recording's ending. Fade out over the final ten seconds of the ride.
- MusicVolume .25 times MasterVolume .65 gives .1625 maximum playback gain (about -16 dB below the supplied masters). Existing real speech activity ducks this by another 75%, with fast attack and smooth release. Stereo, no spatial attenuation, no looping. Seeks/restarts select the appropriate track offset; menu never consumes either track.

Regenerate: decode with `Tools/prepare_music.py --ffmpeg <ffmpeg.exe>`, then run `Tools/import_music.py` with the Unreal Python commandlet. Prepared WAVs and source/hash manifest go in Saved/MusicImport. Imported Content/DC/Music stays local; release preflight requires both assets and /Game/DC is always cooked. Source MP3s are unchanged.

Editor Development build and both stereo imports pass. Runtime commit **1f4fac8** packaged to `E:/Projects/DirectorsCut/Builds/2026-11-15_1833_music`; selected by the normal Play Atlantean Odyssey launcher. Previous 18:19 selection is preserved in Saved/Release/Previous-1819.json. No itch upload.

Packaged checks, all exit0:

- MusicMenuRestart.log: silence in menu, first track starts at ride0 on both voyages across an actual level reload.
- MusicTransitionACE.log: second track starts at 220.00, first stops after the four-second handoff. All three ACE voices finish, VOICE_SMOKE_SUCCESS. Gain drops from .1625 to .0406 during actual speech and recovers smoothly. Initial review started at215 and its third queued smoke line was discarded at the240 scene epoch boundary; reran from205 to allow the three-voice check to complete before the transition.
- MusicEnding.log: second track resumes at correct offset at458, fades through460–470 and both components are stopped at470.

Use `-DCMusicReview` to log component playback/gains. Evidence and source hashes are in the package root build-info.json and music-source-manifest.json. These focused checks cover the music change; prior full-voyage visuals/gameplay remain the existing baseline.
