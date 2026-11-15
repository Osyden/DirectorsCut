# Cannon water feedback — 15 Nov

Owner: voice_audio, `codex/cannon-splash-feedback`, based on main `ded0760`. Scoped to DCGunnery.cpp; no shared HUD, encounter, boss or audio edits.

Water misses now return no crosshair text. Actual hits retain HIT for the existing duration; misses no longer extend an earlier hit indicator. DCGame HUD requires no change because it already ignores empty impact labels.

The real water burst already existed. One concrete placement problem is fixed: the first-entry sweep returns the current shot position when a rising wave engulfs it. That point may lie below opaque water, so the terminal visual is now reprojected to the same sampled/analytic surface used by the collision path. Collision, damage, timing and one-shot sound are unchanged. The cooked P_PillarWaterContact burst is scaled from 1.4 to 1.8 and emission lasts 0.20 instead of 0.12 seconds for better visibility at cannon distance. Its 32-burst cap, 1.6-second cleanup and fallback remain; shadows are disabled on the spray component. No new material/asset generation needed.

Validation: Tools/probe_cannon_water.py passes all 24 production math checks, including wave engulfing, first crest, delayed sample rejection and airborne shot return. Source checks confirm no SPLASH label, one terminal WaterContact call and real-hit-only feedback; git diff --check passes. This is not native visual verification. No engine/build/GPU jobs were run.

Root should run existing -DCStartAt=0 -DCStation=starboard -DCWaterImpactReview -DCGunLog for 24 seconds and inspect near/far/sky/port contacts from the cannon camera. Expect four WATER_IMPACT records, actual water plumes and no SPLASH text. Repeat a caldera miss. Check the larger plume is brief and readable rather than a persistent column. Human playtest retains GPU priority.
