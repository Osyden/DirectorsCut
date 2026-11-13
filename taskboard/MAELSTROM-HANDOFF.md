# Maelstrom scale pass handoff — Codex, 13 November

Branch: `codex/maelstrom-reveal`. Integrated baseline: `b26d3f4` (the scale pass started at `52b7071`). Clone: `E:/Projects/DirectorsCut/Maelstrom-Clone`. Scope and ownership: [MAELSTROM-PLAN.md](MAELSTROM-PLAN.md). The owner's critique and rejected experiments are recorded in [MAELSTROM-SCALE-CRITIQUE.md](MAELSTROM-SCALE-CRITIQUE.md).

**Current status: revised scale and hidden-body encounter build and run; the concept's cinematic finish remains open.** Packaging of this revision and hands-on control/audio review are pending.

## Changes in this revision

- The 3.2 km crater is now 900 m deep, with a continuous entry ramp, approximately 240 m outer fall face and deeper inner slope. The route follows 98.5% to 78% of its radius. Revised camera framing and a 40–49° arena bearing expose the basin beside the bow.
- Continuous rim land grounds smaller ruin complexes and statues. Irregular, noise-warped swells replace the stronger regular bands; the current water tuning reduces far-wave amplitude while retaining fine moving normals and foam.
- The entry tangent now slopes down immediately, removing the shoulder that hid the bottom. Water crests break in both phase and height at about 45 m; the basin casts shadows on its own slopes. The lit cascade follows the actual fall face, with branching downward flow and a torn base. Depth haze, softer highlights and brighter broken foam support the distance cues.
- The full Claw body is permanently hidden, including shadows. There is no emergence, animation or approach toward the ship. After 22 quiet seconds, local tentacles rise from roots 60 m below nearby water; radii are reduced 55%. Transported mesh frames, smooth joint derivatives, denser rings and end caps remove the split/faceted bend seen in the first close-up. This mesh correction does not move the combat proxies.
- First attacks start at breach +65 seconds. Four arms still telegraph, strike the deck and expose eight-hit joints. Six breaks trigger withdrawal and an Ending seek 20 seconds later. Dialogue and optional Sonniss pain audio suit the unseen creature.
- Lowered joints use the stage's sampled surface to stay at least 5 m above water, shifting forward when needed to preserve cannon elevation and yaw. Hidden-body, seek cleanup and paused-time strike guards remain in place.

The integration boundary is unchanged: DCWaveCurl starts the stage, DCPortal supplies the entrance, DCGunnery queries its water, and DCMaelstrom/DCLeviathan own the encounter. Claude's director, fog, treadmill, ruins builders, weather and plugins are not edited. Stage-only fog/grid/weather overrides restore on exit or backward seek. `-DCLegacyMaelstrom` retains the earlier encounter.

## Validation record

**Current revision:** UE 5.8 editor build passes after integrating `b26d3f4`; the water/cascade/spray generator reports zero errors. The final geometry passes 6,330 offline exposed-joint samples with full helm deflection: maximum elevation 9.9994°, maximum range 117.21 m, minimum joint-water clearance 5 m, minimum sampled aiming-path clearance 1.27 m. These calculations support the runtime checks; they do not establish visual quality.

Two normal-speed continuous combat runs exercised the new hidden-body behavior and entry profile. The later run also used the final 45 m wave breakup: all six joints broke, hull stayed at 100%, score reached 600, and the stage exited exactly 20 seconds after the victory call. Sea-return captures at 450/452/455 show the arena and arms removed and the normal camera/ocean restored. A no-fire run from 334 seconds at 3x speed confirmed repeated strikes, hull falling to 60%, and the scheduled exit at 450 seconds. The last foam and lighting adjustments do not change combat geometry. Local evidence: `Saved/MaelstromScaleCombat.log`, `Saved/MaelstromScaleFinal.log`, `Saved/MaelstromScaleUnopposed.log`, and `Saved/MaelstromScaleMaterials6.log`.

**Historical first pass:** earlier editor builds and continuous tests demonstrated both batteries firing, six joint breaks, score 600, hull 100%, victory-to-Ending timing and restoration of the ordinary sea. An unopposed run demonstrated ship damage and the scheduled exit. Claude subsequently packaged that first pass successfully. Those results predate this 900 m crater/hidden-body revision and must not be presented as its validation. Earlier [review frames](review/codex-maelstrom-2026-11-13/README.md) and `Saved/MaelstromFinal.log` / `Saved/MaelstromReview.log` are historical evidence.

## Review and local assets

`Tools/review_maelstrom.ps1` runs offscreen 1600x900 review in its checkout; `-Continuous` starts normal playback at 238 seconds, and `-Orders ''` disables automatic firing. Use actual logged ride times: seek captures advance before taking the image.

Paid content and generated skin stay ignored. See [SETUP.md](../SETUP.md) for local asset regeneration. The private skin generator remains available for the historical visible-body version; the current hidden-body encounter does not require a visible skin. The bowl has custom geometry/shading and references DRW detail normals; it does not run DRW's FFT ocean simulation.

Remaining art priorities are less uniform waterfall silhouettes, richer rim architecture, cinematic cloud/light-shaft composition, spray, and tentacle materials. The current offset view exposes the crater but does not reproduce the reference's central wheel/hands composition. Rigging still crosses the view, and the hull shelf remains visually smooth. Hands-on steering/cannon/audio feel and a package of this revision remain to verify; do not quote performance from overlapping GPU review runs.
