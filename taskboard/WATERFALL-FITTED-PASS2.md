# Fitted waterfall pass 2

Owner: Codex waterfall worker, branch `codex/crater-fitted-waterfall`, isolated checkout `Crater-Workers/Fitted-Waterfall`. Owns waterfall runtime file, generator, importer if needed and this handoff. Root owns all builds, Unreal execution, imports and captures. No cloud, cliff or shared header edits.

First delivery exports the chosen actual lower core profile with `-DCExportWaterfallProfile=absolute/path.json` (or the bare flag for `Saved/WaterfallProfile.json`). It works without `-DCProceduralFalls`; `-DCFallFeed=8` chooses a feed. `-DCWaterfallProfileHeight=8000` selects lower height in centimetres. The export records centimetre frame origin/basis, source/foot, half-width, current velocity, lower core samples, nine lateral surface tracks, a 25x25 receiving-water height field and a profile SHA-1. Runtime source remains opt-in.

Exporter follow-up requires the whitewater worker's `DCCliffGenerator::MeshSectors`, `MeshRows`, `SampleRenderedSurface` and `WaterContactOffset` helpers. It samples actual rendered triangles from `Cliffs` rather than the analytic surface, offsets only radially inward 180 cm and preserves the last six-percent blend to `Foot + Z40 cm`. The old +Z60 offset is removed because it can place water inside downward-facing ledges. Nine lateral tracks prevent a fitted centreline from simply being extruded through adjacent rock. A missing rendered section is logged and export stops instead of silently falling back to mismatched geometry.

## Offline generator

`Tools/generate_waterfall_vdb.py --profile Saved/WaterfallProfile.json --output Saved/GeneratedWaterfallsFitted` enables profile fitting. Without `--profile`, the original ballistic model is preserved. Grid names, physical centimetre transform, actor origin and nominal foot stay compatible with the existing importer/runtime. Keep `-DCFallFeed` equal to the manifest's fitted feed when rendering; this prototype does not automatically distribute one fitted cache across different cliffs.

The fitted model integrates speed along the measured physical path using gravity projected onto each segment and drag, starting with momentum from the upstream fall. It interpolates the exported lateral tracks, uses radial air-side clearance, emits short ballistic secondary fragments and aligns impact drift with the exported current. Impact lifetimes are clipped against the sampled receiving-water height field rather than a horizontal plane. This remains an art-directed particle model, not a coupled liquid/air solver.

The output manifest records the profile file SHA-256, exported profile SHA-1, feed index, arena origin/yaw and physical path length. Each sequence retains fixed bounds and numerical seam/write-read checks. Additional side-view density projections expose whether the field actually follows the sloping path. Do not accept the effect based on these CPU projections: source connection, geometry clipping, illumination and motion still need Unreal captures.

Synthetic slope and lateral-relief tests passed: both original no-profile first-frame density grids are bit-identical to the previous prototype; fitted fields remain bit-identical at period boundaries and pass VDB roundtrip/bounds checks. The synthetic 110 m horizontal / 80 m vertical test generated two 48-frame loops in 3.19 seconds, 13.39 MiB. Synthetic files remain ignored under `Saved`; they are test inputs, not the real cliff cache.

Next: root exports the real rendered profile after combining the whitewater contact helper; then generate and render the compact real candidate. Prior foot-only placement failed because the fixed vertical module did not follow the real sloping core; see `WATERFALL-PROCEDURAL-PASS1.md`.
