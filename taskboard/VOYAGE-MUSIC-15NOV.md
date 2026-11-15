# Voyage music

Root owns DCAudio, music import and the build/GPU slot for this pass on main.

- User's `../Music/the fog.mp3`: 224.4 seconds, stereo 48 kHz. Starts when the new voyage starts, with a 1.5-second fade in; remains silent on the menu.
- User's `../Music/the caldera.mp3`: 298.76 seconds, stereo 48 kHz. Starts 20 seconds before the Slide phase (currently ride 220, 3:40). Four-second linear crossfade fits the first recording's ending. Fade out over the final ten seconds of the ride.
- MusicVolume .25 times MasterVolume .65 gives .1625 maximum playback gain (about -16 dB below the supplied masters). Existing real speech activity ducks this by another 75%, with fast attack and smooth release. Stereo, no spatial attenuation, no looping. Seeks/restarts select the appropriate track offset; menu never consumes either track.

Regenerate: decode with `Tools/prepare_music.py --ffmpeg <ffmpeg.exe>`, then run `Tools/import_music.py` with the Unreal Python commandlet. Prepared WAVs and source/hash manifest go in Saved/MusicImport. Imported Content/DC/Music stays local; release preflight requires both assets and /Game/DC is always cooked. Source MP3s are unchanged.

Editor Development build and both stereo imports pass. Packaged validation pending. Use `-DCMusicReview` to log actual component playback and gains; test Start/menu/restart, real ACE ducking, the 220–224 handoff and ending.
