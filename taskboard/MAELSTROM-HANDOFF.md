# Maelstrom handoff — Codex, 13 November

Branch: `codex/maelstrom-reveal`. Tested implementation: `1e3372d`. Working clone: `E:/Projects/DirectorsCut/Maelstrom-Clone`. Integration baseline: Claude's `1071550`. Scope and visual target: [MAELSTROM-PLAN.md](MAELSTROM-PLAN.md).

Integration completed: Claude's subsequent `2c2965e` cloud/outlier work was merged at `b45d579`, rebuilt successfully and captured again. That combined commit was fast-forwarded into `main` and pushed. All 43 ignored skin files were copied into the main project and checked for matching names/sizes. The main project's next build/relaunch will pick up the new code; its already running session was left alone.

## Implemented

- A 3.2 km crater, 650 m deep, with geometric spiral swells, rim cascades, modular ruin complexes and giant statues. One arena coordinate system keeps the slope, scenery and creature together while the ship stays fixed. Helm input changes its bearing with inertia.
- Hollow water entrance/exit geometry, a narrowing tunnel, a short exposure flare at the breach, then a clearer view over the bowl. The helm framing moves the mast away from the centre; mouse look still works and C hands framing back to the existing camera toggle.
- Animated Claw Kaiju at scale 18 (approximately 666 m tall), rising from the bottom then moving alongside. Four procedural tentacles connect to its body, telegraph deck strikes and expose cannon targets. Six destroyed joints trigger recoil/defeat, then the creature sinks and the ride cuts to open sea 20 seconds later. If it survives, the scheduled ending still occurs.
- Cannon aim, projectile water hits and splashes query the local bowl/tunnel surface instead of a flat sea plane. Weak points are kept above the uphill water and within gun elevation/range limits.
- Corrected private Kaiju skin generation: seven individual texture tiles, without Interchange accidentally importing the entire UDIM set into every tile. The imported skeleton faces -Y, so its body yaw and arm attachment orientation account for that conversion. Optional existing Sonniss pain audio plays on breaks and defeat.

## Integration boundaries

Changed existing code: `DCWaveCurl` starts the standalone stage; `DCPortal` draws the entrance; `DCGunnery` uses the new water query. New code: `DCMaelstrom` and `DCLeviathan`. No source edits to `DCFogFront`, `DCTreadmill`, `DCRuins`, `DCRideDirector`, `DCWeatherFX` or the sky/water plugins.

The stage changes only the public Breach/Maelstrom phase cues and Ending blend. During the reveal/battle it temporarily adjusts rendered height fog and disables UDS's global fog material, restoring both afterward. DRW's grid and the legacy weather actor are hidden during the encounter and restored on exit; the existing ocean supplies the ending. `-DCLegacyMaelstrom` keeps the previous encounter available.

## Local assets and review

The original water/spray assets and generators are committed. Generated Kaiju skin and all purchased source content stay ignored. See [SETUP.md](../SETUP.md) for regeneration; `Content/DC/Maelstrom/LeviathanSkin/` must be copied locally or regenerated before reviewing the textured boss in another checkout. The bowl uses its own geometry and shader with a reference to DRW detail normals; it does not use DRW's FFT ocean simulation.

`Tools/review_maelstrom.ps1` runs an offscreen 1600x900 review in its own checkout. Use `-Continuous` for normal playback from 238 seconds. Empty `-Orders ''` disables automatic firing. Seek captures advance several seconds before capture; continuous captures log their actual ride time. Use the preserved review images alongside the runtime log, rather than inferring timing from a seek filename.

## Validation and remaining polish

The UE 5.8 editor target builds successfully. Continuous combat demonstrated both batteries firing, all six joints breaking and defeat with 600 score and 100% hull. The final run's defeat call was followed exactly 20 seconds later by the Ending seek; captures at ride times 450/452/455 show the brief flare resolving into normal sea with no remaining arena/boss. A separate unopposed passage demonstrated tentacle damage and the scheduled sea exit. Numeric reachability checks covered all four arms from 322 to 449.9 seconds: targets remained within 101 m, below 10 degrees elevation, above water and with a clear firing line. Material commandlets completed without maelstrom shader failures. [Review frames](review/codex-maelstrom-2026-11-13/README.md) are preserved; local evidence is in `Saved/MaelstromFinal.log` and `Saved/MaelstromReview.log`.

This is a playable implementation and visual foundation, not a claim that the concept's cinematic finish has been reached. Further art work should focus on more irregular foam and spray, more varied rim architecture, smoother tunnel-lip breakup, stronger waterfalls and a more integrated tentacle surface. Temporal smearing around rigging during shake also needs attention; stage motion blur is now disabled. Hands-on steering/cannon feel and audio mix still need owner review. The new encounter has not yet been included in a packaged build; Claude's separately verified pack build predates it. Avoid quoting performance from simultaneous review runs on the same GPU.
