# Crater visual iterations — 14 November

User authorized recurring visual improvement and parallel workers. The Codex app heartbeat `directorscut-crater-visual-iterations` is active every 30 minutes in the existing maelstrom chat. This is implementation work with actual render review, not status polling. Latest user feedback takes precedence.

Latest continuation: [source-shaped fractured bedrock and larger coherent masses](CRATER-CURVED-BEDROCK-15NOV.md). The [connected massif](CRATER-MASSIF-15NOV.md) remains the macro authoring base. The corrected reference now produces actual physical clusters inside its curved skin; free cubes/panels and the first dense rubble variant were rejected. Larger masses are useful progress, but the exposure transition and thin fins need resolution before native/500m expansion. Stop generic surface-noise variants or new audit scaffolding. Preserve explicit source units, upstream corrections/notices, original3D faces and the measured guard. Normal gameplay cliffs/water/falls and sky/light ownership remain unchanged.

## Ownership and priorities

- Integration checkout: `E:/Projects/DirectorsCut/Maelstrom-Saturday`, branch `codex/maelstrom-saturday`. Start checkpoint `e7d7f8b`.
- Latest cliff audit worker: `Crater-Workers/Cliff-Contact`, `codex/cliff-weathered-audit`. Delivered original-triangle and radial-shear audits, then independently inspected the implicit-block reference. No live integration.
- Latest landform worker: `Crater-Workers/Flow-Seams`, `codex/cliff-implicit-block-reference`. Refined/shear trials failed visually. Inspected and successfully ran all four upstream implicit-block examples; see `ROCK-FRACTURING-REFERENCE.md`. Scratch upstream code/models remain ignored.
- Latest waterfall/review worker: `Crater-Workers/Fitted-Waterfall`, `codex/cliff-weathered-review-pass10`. Rejected the refined/shear trials, then reviewed eight supplied implicit-block views. Polyhedral is the preferred structural starting point, not a ready-made cliff. No new export or Unreal import. Root owns Unreal imports and runtime integration. Workers finished this pass; check actual branches/status before reusing.
- Integrator owns shared `DCMaelstrom.cpp/.h`, runtime waterfall integration, import/material tooling, route review, builds and captures. Workers do not run Unreal or modify this checkout.

Preserve the 3.2 km crater, 900 m depth, descending spiral and ship-centric transform. Boss mechanics remain deferred. Cloud/sky/light composition and early-game files remain with their respective chats. Keep public Git free of vendor assets and large generated volume caches.

## Immediate visual failures

Actual `maelstrom-cliffs-14nov` captures show an excessively broad bright talus band, a largely continuous dark cliff face, narrow ribbon waterfalls, and broad flat foam patches over water that still reads as terrain. Temples now clear the skyline, but geology, whitewater and motion require further work. Passing a build does not establish visual quality.

1. Give the cliff a convincing large-scale fractured structure and reduce the bright rubble shelf while preserving shoreline clearance and the supported temple skyline.
2. Generate a compact gravity-driven falling-spray loop and impact plume. Match one actual cliff contact before multiplying volumes. Add the receiving surface-foam trail rather than expecting a VDB to provide surface flow.
3. Break up broad/regular foam and crest patterns using physically scaled sources and flow, then inspect the route and bank silhouettes from the moving deck camera.

## Original waterfall generation and import

Blender's `E:/Utilities/Blender/5.3/python/bin/python.exe` includes `openvdb` and NumPy. `Tools/generate_waterfall_vdb.py` now generates deterministic periodic ballistic whitewater and a separate impact-mist sequence. This is an art-directed particle/volume prototype, not a liquid solver with cliff collisions. The fine candidate has 0.5 m voxels, 96 frames at 24 fps, a four-second loop, and 69.66 MiB of VDB files. Generation took 12.70 seconds locally. Loop continuity, bounds, density normalization and write/read checks pass.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_waterfall_vdb.py --output Saved/GeneratedWaterfallsFine --voxel-m 0.5 --emission-rate 14000 --fps 24 --duration 4
$env:DC_WATERFALL_DIR = "$PWD/Saved/GeneratedWaterfallsFine"
& 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$PWD/DirectorsCut.uproject" -run=pythonscript "-script=$PWD/Tools/import_procedural_waterfalls.py" -unattended -nosplash -nullrhi
```

Native Unreal import of both fine sequences passed: Falling 77x44x167, Impact 105x53x27. Materials use scene-lit scattering/extinction without emission. Generated volume caches under `/Game/DC/Maelstrom/ProceduralWaterfalls` are ignored by Git; regenerate before using the prototype. Existing `/Game/DC` cooking rules include them when present. The source export from this pass is also available at `E:/Projects/DirectorsCut/Crater-Workers/Waterfalls/Saved/GeneratedWaterfallsFine`.

Enable with `-DCProceduralFalls`; use `-DCFallCamera` for a close review. Playback derives from frame count over four seconds; custom exports accept `-DCWaterfallLoopSeconds=` or explicit `-DCFallFPS=`. It spawns only one physically sized lower reach and its impact; it is NOT enabled by default. Seek/exit clears actors and the debug camera. The trace-range override restores only a value it actually changed and still owns. Other existing fog/encounter code already requests longer ranges, so the first suspicion that the 300 m default hid the volume was not established as the cause.

## Review and checkpoint requirements

Serialize integration/build/render operations. Inspect active Unreal/UBT processes; do not close or interrupt another chat's editor, tests or builds. Use offscreen review helpers to preserve the user's keyboard. Check source/material generation, geometry seams and clearance, entry/descent/exit and backward seek when affected. Compare matched captures with `References/maelstrom-direction.jpg`; record what is visually improved, rejected and still deficient. Examine motion/loop seams separately from stills and log volume streaming/runtime cost. Commit/push validated checkpoints to the owned branch; no automatic main merge or publication.

## Actual review and rejection record

- Pass A (`Saved/SceneReview/crater-iteration-a`): removed the overly broad bright talus but exposed a smooth curtain. Rejected for visual quality.
- Pass B (`Saved/SceneReview/crater-iteration-b`, 282/300/340/420/451): staggered cliff shelves still look like rounded clay and the foreground remains a broad pale film. Scale, supported temple silhouettes and entry/exit remain intact; the result is not close to the reference.
- Foam-only B (`Saved/SceneReview/crater-foam-mask-b`, 300/340): most of the pale water in the beauty render is black or finely speckled in the foam mask. The broad pale film is therefore not primarily dense foam. The few dense patches still have overly smooth boundaries.
- Waterfall close B (`Saved/SceneReview/waterfall-close-b`, continuous 305.8–306.2 across a loop boundary): the imported VDB exists, but its lower column hangs in front of the slope. Feed 8 has 164.5 m of horizontal retreat and 129.8 m of rise; the prototype assumes only 10 m retreat over its lower 80 m. Foot-only placement is insufficient. The old surface core also crosses a black ledge seam. Keep the volume opt-in, fit a lower segment to the actual revised cliff profile, then re-evaluate. Consecutive stills do not establish motion quality.

The foam worker traced an integration bug in the installed water plugin: `Surface.a` is a wave roughness mask, not ready-to-use perceptual roughness or unresolved-normal variance. The material had used it directly as a roughness lower bound (commonly 0.5–1). The corrected shader uses bounded clear-water roughness and blends foam separately. No vendor shader code was copied.

The route review also found that the ship could move sideways by a direction change of about 25–34 degrees while its visible yaw changed only about 3 degrees. The new steering heading follows the actual displaced route tangent, limited to 12 degrees for camera comfort; the centreline, dimensions and speed are unchanged. Neutral-heading/opposing-steering tests were added.

Representative actual captures are retained under `taskboard/review/crater-iterations-14nov`: `rejected-b_0300.png`, `foam-mask-b_0300.png`, and `rejected-waterfall-b_03060.png`. These are diagnostic/rejected images, not target-quality examples.

### Final C review

`Saved/SceneReview/crater-iteration-c` contains the final 282/300/340/420/451 captures. The 300/340 deck views show more readable rock fractures and clearer liquid reflections than B. This is an intermediate improvement, not visual acceptance: the overall cliff still leans across its fixed toe/rim endpoints, the surface is dark and repetitive, the hydraulic banks remain too broad/smooth, and the falls remain thin bright ribbons. The 420 view also shows a sharp hanging/vertical triangular feature near the upper-left water/rock boundary and a pinched star pattern in the lower water. Isolate geometry versus material coordinates there before further detail work.

`Saved/SceneReview/cliff-close-c/shot_0300.png` shows that the new rock planes become blocky and flat up close. Dark ledge crossings still interrupt the water core. The VDB has the same rejected foot-only placement. Preserve this close-up as a failure case; do not promote C as a final cliff asset. The corresponding retained files are `current-c_0300.png`, `current-c_0340.png`, `lower-descent-c_0420.png` and `cliff-close-c_0300.png` under the review folder.

## Build and regression evidence

Final Editor and Game builds pass (`Saved/CraterIterationEditorBuildC.log`, `CraterIterationGameBuildC.log`). Rock/water generation passes with texture diagnostics confirming sRGB colour, normal-map compression and a connected world-space normal (`CraterIterationMaterialsC.log`). Both `DirectorsCut.Maelstrom.CliffContinuity` and `RouteAndSteering` pass (`CraterIterationTestsC.log`, 20:16 UTC). The generator's density/period/roundtrip checks and native import are independent of these Unreal tests.

Pass B entry/descent/exit captures completed and cleared both prototype volume actors; the sky controller restored its 29 properties. The close review logs a non-Nanite virtual-shadow-map marking overflow. Do not suppress that warning or call performance verified; investigate baking the generated rock to a static Nanite/LOD asset and profiling shadow coverage separately. Existing UDS editor-utility, Pawn EnableInput and experimental Toolset Python startup errors are still present in review logs; there was no fatal error or new maelstrom material compile failure. The project has not been packaged or merged to main by this pass.

## Next scheduled passes

Latest continuation: [15 Nov regional exposure and assembly diagnosis](CRATER-REGIONAL-CLIFF-15NOV.md). One real 160 x 96 m exposure now fits the 500 m context, but is visually REJECTED as a polygon/rubble scar. The excavate-window-and-refill assembly is a concrete cause; three shared fracture families already exist. Next test a continuous bedrock body with shared cell boundaries, selective narrow openings/releases and coherent whole-face masses. Do not simply enlarge this patch, reopen every plane as a saw slot, add more generic noise, or spend another pass expanding audits. Preserve measured shore/foundations and review existing near/far cameras before native import.

Current continuation: [15 Nov full-height structure and rock material](CRATER-STRUCTURE-AND-ROCK-15NOV.md). A/B directly authored heightfield plane/wedge trials preserve all constraints but are visually rejected; do not promote or keep tuning them. Preserve the full-height physical envelope and measured continuous shoreline coordinate, then build connected 3D bedrock masses with finite joint exposures. The bottom fringe is largely submerged and now has measured water-context diagnostics. Optional slate sampling has native proof but is not a geometry or final-quality fix.

Latest checkpoint: [15 Nov cliff boundary, native import and full-sector context](CRATER-CLIFF-CONTEXT-15NOV.md). The corrected local outcrop passes its bounded geometry/native gate but the complete context fails coherent geology. Prioritize connected sector-scale escarpments and shared rock layers/fractures/drainage, retaining actual shore and supported ruins. Do not repeat the patch around the crater, restart noise-only surface trials or expand audit tooling. Keep water/falls integration behind the accepted final surface.

1. Use the pass-4 geology/spray handoff, not the older C defect list. Giant geometric shelves and the 22 m masonry normal/AO field are now removed. The remaining cliff reads too smooth: inspect actual installed cliff texture sets for tiling suitability, then improve localized real fractured faces and narrow wet runoff without reinstating uniform cell patterns. Preserve the corrected inlet, shore, far-rim scale and temple skyline. Do not hide defects under more fog.
2. Native VDB visibility is now established; the missing voxel-unit conversion is fixed. The fresh Geology4 cache fits the current face, but the combined effect still resembles an attached white strip and dusty plume. Introduce short genuinely detached falling reaches shared by mesh and export; remove bypassed attached triangles and make breakup, droplets, splash and foam spatially concentrated. Re-export after geometry changes. Keep opt-in until visually convincing.
3. The basin normal chain rule and surface metric are corrected. Resolve broad pale patches using matched foam/normal views, then add spatially indexed broken wave packets outside the steering corridor. The outer channel-to-bowl recovery at ride 420 rises about 98.5 m over 132.4 m with 50.6 degree maximum grade; its shape is not explained by the smaller 24 m bank shoulder alone. Preserve mean height, route and shore clearance.
4. Review continuous deck-camera motion, steering and entry/exit. Measure performance in a dedicated run without another chat using the GPU; current sparse samples are not a benchmark.
