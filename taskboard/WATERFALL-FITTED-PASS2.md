# Fitted waterfall pass 2

Owner: Codex waterfall worker, branch `codex/crater-fitted-waterfall`, isolated checkout `Crater-Workers/Fitted-Waterfall`. Owns waterfall runtime file, generator, importer if needed and this handoff. Root owns all builds, Unreal execution, imports and captures. No cloud, cliff or shared header edits.

First delivery exports the chosen actual lower core profile with `-DCExportWaterfallProfile=absolute/path.json` (or the bare flag for `Saved/WaterfallProfile.json`). It works without `-DCProceduralFalls`; `-DCFallFeed=8` chooses a feed. `-DCWaterfallProfileHeight=8000` selects lower height in centimetres. The export records centimetre frame origin/basis, source/foot, half-width, current velocity, lower core samples, nine lateral surface tracks, a 25x25 receiving-water height field and a profile SHA-1. Runtime source remains opt-in.

Exporter follow-up requires the whitewater worker's `DCCliffGenerator::MeshSectors`, `MeshRows`, `SampleRenderedSurface` and `WaterContactOffset(Angle,V)` helpers. It samples actual rendered triangles from `Cliffs` rather than the analytic surface, offsets radially inward 180 cm and preserves the last six-percent blend to `Foot + Z40 cm`. The helper adds vertical clearance only on low talus (`V<.20`), fading it out before the wall; the old universal +Z60 is removed because it can place water inside downward-facing ledges. Nine lateral tracks prevent a fitted centreline from simply being extruded through adjacent rock. A missing rendered section is logged and export stops instead of silently falling back to mismatched geometry.

## Offline generator

`Tools/generate_waterfall_vdb.py --profile Saved/WaterfallProfile.json --output Saved/GeneratedWaterfallsFitted` enables profile fitting. Without `--profile`, the original ballistic model is preserved. Grid names, physical centimetre transform, actor origin and nominal foot stay compatible with the existing importer/runtime. Keep `-DCFallFeed` equal to the manifest's fitted feed when rendering; this prototype does not automatically distribute one fitted cache across different cliffs.

The importer reads the manifest's fitted feed index and stores its volumes/materials under `/Game/DC/Maelstrom/ProceduralWaterfalls/Feed08` (or the matching feed number). Runtime loads only that chosen feed's folder. `feed_index`, asset folders and `-DCFallFeed` refer to stable `FWaterFeed.SourceIndex` values, not the compact array position; root owns adding/assigning that field in the shared header and whitewater source. Nearest-feed selection still uses the array internally. A direct late seek selecting another nearest feed logs a missing fitted asset instead of incorrectly applying feed 8's cache. The old generic root remains available only with the explicit additional `-DCGenericWaterfallPrototype` flag. Re-export/regenerate after changing cliff geometry or the water-core contact helper.

The fitted model integrates speed along the measured physical path using gravity projected onto each segment and drag, starting with momentum from the upstream fall. It interpolates the exported lateral tracks, uses radial air-side clearance, emits short ballistic secondary fragments and aligns impact drift with the exported current. Impact lifetimes are clipped against the sampled receiving-water height field rather than a horizontal plane. This remains an art-directed particle model, not a coupled liquid/air solver.

The output manifest records the profile file SHA-256, exported profile SHA-1, feed index, arena origin/yaw and physical path length. Each sequence retains fixed bounds and numerical seam/write-read checks. Additional side-view density projections expose whether the field actually follows the sloping path. Do not accept the effect based on these CPU projections: source connection, geometry clipping, illumination and motion still need Unreal captures.

Synthetic slope and lateral-relief tests passed: both original no-profile first-frame density grids are bit-identical to the previous prototype; fitted fields remain bit-identical at period boundaries and pass VDB roundtrip/bounds checks. The synthetic 110 m horizontal / 80 m vertical test generated two 48-frame loops in 3.19 seconds, 13.39 MiB. Synthetic files remain ignored under `Saved`; they are test inputs, not the real cliff cache.

## Actual fitted candidate ready

Root combined the exporter/contact helper, built the editor and passed all three maelstrom automation checks before exporting `Saved/WaterfallProfile.json` in this worktree. The actual profile has **351 points, nine lateral tracks and a 234.86 m path**, SHA-1 **`1A217C8FA356610A4EABC57482F4C6F8E8D797E8`**, for physical source 8 at angle 2.35 radians. Local source is approximately `(-0.79,-6560.75,8000)` cm, foot `(0,1000,0)` cm, current `(-1748.61,427.06,0)` cm/s. This corrects the old guessed source `(0,0,8000)` cm.

Generated output is **`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/GeneratedWaterfallsFitted`**, with the standard `Falling/Falling_0000.vdb` and `Impact/Impact_0000.vdb` prefixes and `manifest.json`.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_waterfall_vdb.py --profile Saved/WaterfallProfile.json --output Saved/GeneratedWaterfallsFitted --voxel-m 0.5 --fps 24 --duration 4 --emission-rate 14000
```

Both sequences have **96 frames, 24 fps, four-second loops, 0.5 m voxels**. Actual CPU generation time was **57.08 seconds**, total disk **144,533,056 bytes / 137.84 MiB**. Generated files are ignored and not committed.

| Sequence | XYZ resolution | Voxel-centre bounds, cm | Peak density | Bytes |
| --- | --- | --- | --- | --- |
| Falling | 67 x 183 x 200 | (-1650,-7500,-1350) to (1650,1600,8600) | 0.539092 | 115,561,496 |
| Impact | 110 x 89 x 46 | (-3800,-900,-1050) to (1650,3500,1200) | 0.219933 | 28,971,560 |

Exact-period density and VDB roundtrip errors are zero; all frames have fixed bounds, nonnegative finite densities and zero clipped particle mass. Falling seam-frame mean absolute change is 0.0001659 versus interior median 0.0001642; Impact is 0.0000519 versus 0.0000530. Particle transit through the whole measured path ranges from 11.70 to 15.83 seconds while emissions repeat every four seconds, with previous-cycle particles retained.

**Self-critique:** the side-density projection now follows the actual ledges instead of hanging vertically in open air. It also exposes a problem in the supplied water-core path: the low talus section dips underwater and then curls upward to the nominal foot. Matching that geometry does not make the flow physically plausible. Preserve this candidate for the first actual comparison; a subsequent pass should detect the first receiving-water intersection within the real water footprint, trim the falling reach there and emit its impact at that contact. The water-height field alone does not say where water geometry exists, so the exporter should also expose the shoreline/coverage for that trim. Do not compensate by increasing density or stretching the cache.

Root owns Unreal import, fitted-vs-generic captures, moving-camera review and performance checks. This worker has not rendered the real fitted candidate. Prior foot-only placement failure is documented in `WATERFALL-PROCEDURAL-PASS1.md`; this pass remains opt-in pending visual acceptance.
