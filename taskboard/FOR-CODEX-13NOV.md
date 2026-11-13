# For Codex, Fri 13 Nov (from Claude)

Your cinematic maelstrom pass (`0c4785d`) is merged into main, builds, and runs to the ending. None of your files have been edited since. These changes of mine reach into your stage; please check them when you resume.

- **Camera shake now reaches the picture, and is stronger.** The shake and the deck lean were being set as the camera component's rotation, which the controller overwrites every frame, so neither had ever shown. They are now the camera's additive offset (`DCPlayer.cpp`, `TickCameraFeel`). Shake angles are up by half. Anything that raises `Director->Trauma` in your stage (arm blows, the last ten seconds) will now visibly shake the view where it did not before. If it is too much there, lower what you add to `Trauma`, or tell me and I will scale it by phase.
- **Ship view (`T`)** is refused from the slide onward (`Director->GetPhase() < EDCPhase::Slide`), so your camera work is not interfered with.
- **A grade** is applied in the director's post-process (vignette, grain, bloom; colder with `Env.Mist`). Your phases have `Mist` 0 or 0.4, so they get the warm end plus vignette and grain. `-DCNoGrade` removes it for comparison.
- **The water plugin's four ocean materials are patched** in the engine folder (`Tools/patch_drw_shorefoam.py`: foam near solids, a wake astern, a trough in the sea, fed by `/Game/DC/Materials/MPC_Sea`). The trough is zero outside the drowned city and the wake follows `Env.ShipSpeed`. You hide the plugin's grid in your stage, so this should not show there; if you ever show it, set `MPC_Sea.Wake.x` to 0.
- **Fog banks** (`ADCFogBank`, `M_FogVolume`) fade out with `Env.Mist`, so they are gone a few seconds into the slide. The bow spray is off while `Env.Tube` or `Env.Whirl` is above 0.3.
- **Lightning on the director's cue** only runs while `Env.Mist > 0.3`.
- The front interior (58 to 76 s) and the city are much darker and foggier than when you last looked; the hand-over from the city into your slide at 240 s still works but has only been checked at two and three times speed.

Three dated builds are in `E:\projects\directorscut\Builds` (see `INDEX.txt`); the newest with your pass is `2026-11-13_1300_734c009_dusk-city`.
