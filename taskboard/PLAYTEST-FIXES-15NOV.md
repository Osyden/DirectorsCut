# Human playtest fixes — 15 November

This pass starts from combined main, preserving the installed caldera, Kraken, ACE/crew and individual cannon integration. Local testing only; do not upload to itch. Account: Onibox.

## Changes

- Flattened the tunnel's lower hull corridor, shortened its irregular mouth and waterfall apron, and retired the approach tube once the stern clears. The backward descent view no longer retains a floating tube underside.
- Doubled the Kraken's scale and moved its berth to preserve ship clearance. Its mouth remains above water and within the extended 210m player aiming range.
- Rotated cloud sampling and both sun implementations with the caldera. Regenerated the cloud material with rendering enabled.
- Added one landing crash, six water-contact bursts, a strong pitch impulse and camera trauma. Continuous descent triggers it once; seeks do not replay it and it does not damage the hull.
- Player cannons fire into empty sky and nearby water. Added orange muzzle fire, smoke and a short light flash. Independent gun ownership, one ball per trigger and reloads are retained.
- Crew physically vacate the wheel when the player takes it.
- Authored hazard calls bypass LLM rewriting; phase changes invalidate queued/in-flight replies, synthesis and playback. Storm bell strikes only three times on entry.
- Removed duplicate weather audio, lowered ambience and ducked it under centred crew speech. Wind, rain, rumble, drone, rough sea and thunder fade to exact silence over the tunnel exit; calm water and gameplay impacts remain.
- Added readable speech bubbles with circular crew portraits, including text fallback when synthesis fails. Removed score UI and moved instructions into a Start Game menu showing the ship at open sea. The voyage remains held until Start.

## Validation

Editor and game targets build. Eight Unreal automation suites pass, including tunnel hull clearance, command parsing, synthetic speech fixtures and caldera route/contact checks. CPU boss replay passes 4,957 assertions.

Native menu capture and automatic Start transition pass. Fourteen cannon/helm assertions pass, including actual sky firing and physical wheel clearance. Backward waterfall frames at 275/277/280 seconds were inspected. Storm silence logged at 277.01 seconds. Continuous landing logged once at 299.41 seconds, with no damage.

The normal-speed native boss run interrupted all six attacks, permanently severed two limbs, dealt eight effective boss damage and took zero ship damage. Both enlarged mouth charges were reachable. Evidence is in `Saved/Playtest*.log`, `Saved/PlaytestAutomation.log` and `Saved/SceneReview/Playtest*`.

## Follow-up weather feedback

The city key at 76 seconds now fades rain and lightning to zero, with gentler waves and wind. Storm audio fades to zero by 94 seconds, retaining calm sea and a quiet mist tone. It resumes gradually in the descending tunnel, then silences again on exit. The independent radial storm no longer reappears over the city. Low sea mist remains around the ruins; ship-following fog starts with the city rather than overlapping the entering cloud wall.

The front now hands over to the interior atmosphere over 58–62 seconds. This removes the dark water intersection after entry. A dark strip remains during the approaching cloud-wall view: this is partially improved, not a complete renderer-level fix. Native A/B checks isolated it to the heterogeneous volume; disabling throat mist, water integration, shadows or fog integration did not fix it. No diagnostic renderer switches are shipped.

Native continuous captures at 50/58/60/64/94/150 seconds passed; 94 and 150 show zero rain/lightning and retained low mist. `Saved/PlaytestFogHandoff.log` records storm audio silence at 94.00. Editor build passes.

The 15:24 package passed menu/Start/restart and early ACE checks, but its full-voyage tests were deliberately interrupted for GPU contention and these new weather changes. It is superseded and was never selected. The 15:50 attempt failed immediately on an external UnrealBuildTool mutex; no runtime candidate was produced.

## Selected local playable

Runtime commit `8de1d9eab0b7bfd8977895b01cd7fc701f218c46`, pushed to main. Selected executable: `E:/Projects/DirectorsCut/Builds/2026-11-15_1551_playtest-fixes/Windows/DirectorsCut.exe`. The usual `E:/Projects/DirectorsCut/Play DirectorsCut.cmd` launcher now selects this package. Earlier packages remain available. Distribution stays local; no itch upload.

Build/cook/stage/archive and sidecar staging passed; no material compilation failure. The package contains 606 non-Saved files totalling 16,030,184,952 bytes. Source provenance and build information are in the candidate's `source-manifest.json` and `build-info.json`.

The actual packaged voyage completed normally at 16:01:55 GMT, exit 0. It showed the menu, started, reloaded once, returned to the menu, and played continuously through the 465-second ending capture. All three crew used ACE speech. Bell struck exactly three times. City captures at 94/150 seconds show zero rain/lightning; storm audio stopped at 94.00, returned for the tunnel, and stopped again at 277.00. Landing fired once at 299.15 with damage zero. All six boss attacks were interrupted, two limbs remained severed, boss damage taken was zero and final hull was 44.8%. The bot issued crew orders but did not steer.

Evidence: candidate `Windows/DirectorsCut/Saved/PlaytestFullVoyage.log` and `Saved/Screenshots/Auto`; menu, city, landing and ending images inspected. The run reused the earlier test's resident llama server; executable/model hashes match this package, and that orphan was stopped after validation. No game/GPU process remains owned by root. Normal restart logs include one audio-mix clearing notice and pawn EnableInput/DisableInput ownership messages; they did not prevent the run. No fatal/ensure/GPU-crash marker occurred.

Remaining acceptance: the approaching storm's dark waterline is only partially addressed, as described above; human headphone mix, live microphone input and another-machine checks remain. New ACE dialogue-backend work and isolated inlet-shoulder experiments are not part of this frozen candidate.
