# Crater seam and fitted waterfall pass — 14 November

Scheduled continuation from `989e064`, in `Maelstrom-Saturday` on `codex/maelstrom-saturday`. Three isolated workers handled cliff contact, basin/flow seams, and a fitted original VDB. No sky, lighting, early-game or boss edits. This is a review checkpoint, not a finished scene or main integration.

## Concrete defects addressed

The inlet's angular coordinate wrapped from about -8 to341 seconds, but the shoreline radius treated it as continuous. Across that cut the shore jumped658m sideways and427m vertically. Connecting the neighboring mesh vertices created the giant triangle visible late in the descent. The unused backward side now blends continuously into the inlet. A production-source CPU probe visits all688,128 angular edges: the longest edge falls from785.431m to12.543m, while full-steer clearance stays35m.

The inner water texture mapping had a separate singularity: it backtraced a rotating current by up to257m using a Cartesian offset, even at the pole. Both live-FFT and disabled-FFT normal captures retained the star-shaped pinch. Analytical angular/radial backtracing with a finite core reduces measured inner-coordinate stretch from16,924.5x to1.222x at1.2m radius and from80.989x to1.222x at250m. The entrance tunnel keeps its prior mapping. Outer angular-flow strain remains about20.4x in the worst sampled terminal-angle region and still needs refinement.

The old waterfall sheet used a different tessellation from the cliff, cutting through fracture peaks. Adding60cm in Z also pushed water into overhangs, and an opposite across-stream vector twisted its final blend. The surface core now clips directly from the rendered rock triangles, has an inward offset without upward lift on overhangs, and uses the same across direction at its foot. Cliff normals follow the actual triangles. Engine contact review reports24,259 core triangles and4.40cm minimum positive face clearance before the final receiving-water blend. These checks address intersection; they do not establish realistic waterfall motion.

See the detailed worker notes: [flow evidence and probe](FLOW-SEAMS-PASS2.md), [cliff contact and fixtures](CLIFF-CONTACT-PASS2.md), and [fitted volume generation](WATERFALL-FITTED-PASS2.md).

## One fitted volume, still opt-in

The exporter now samples a real lower80m cliff reach, nine lateral tracks, the local frame, current direction and receiving-water heights. Source8's path is234.9m long. The generator advects particles along that path with gravity/drag speed and outward ballistic spray; impact motion follows the current and sampled water surface. This is an art-directed profile model, not a fluid solver.

The fine candidate contains96 frames per layer at24fps, with0.5m voxels. Generation took57.08 seconds and produced137.84MiB. Falling imports at67x183x200; Impact at110x89x46. Loop/bounds/write-read tests pass. The actual source profile hash is `1A217C8FA356610A4EABC57482F4C6F8E8D797E8`.

Fitted assets are under `/Game/DC/Maelstrom/ProceduralWaterfalls/Feed08`. Selection uses the stable drainage source ID, not its array position: shoreline changes can skip other sources and renumber array entries. A generic old volume requires `-DCGenericWaterfallPrototype` explicitly. Both paths remain opt-in; no default waterfall replacement or multiplication has occurred.

The measured path still follows artificial ledges and dips below the receiving water before returning upward to the nominal foot. The new close render must be judged against that known limitation. Next, truncate falling water at its **first valid receiving-water intersection**, checking that water actually exists inside the basin boundary, then place impact and surface-foam sources there. Introduce short gravity-driven drops leaving ledges rather than wrapping water around every overhang. Do not increase density to hide these problems.

### Reproduce

Export source8 using the built project with `-DCExportWaterfallProfile=E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/WaterfallProfile.json -DCFallFeed=8`. It needs to enter the maelstrom; the offscreen review helper can supply that. Then:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_waterfall_vdb.py --profile Saved/WaterfallProfile.json --output Saved/GeneratedWaterfallsFitted --voxel-m 0.5 --emission-rate 14000 --fps 24 --duration 4
$env:DC_WATERFALL_DIR = "$PWD/Saved/GeneratedWaterfallsFitted"
& 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$PWD/DirectorsCut.uproject" -run=pythonscript "-script=$PWD/Tools/import_procedural_waterfalls.py" -unattended -nosplash -nullrhi
```

This pass's export and generated files are in `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/`. Bulk caches and imported assets are ignored by Git. Re-export/regenerate after changing the relevant geometry. Review with `-DCProceduralFalls -DCFallFeed=8 -DCFallCamera`; loop duration defaults to4 seconds. The close comparison uses density overrides `.016` for falling spray and `.004` for impact, above the conservative defaults, to make attachment easier to judge. These are review settings, not an accepted final art direction.

## Validation

Final Editor and Game targets build. Water material generation and source-scoped native SVT import pass. `CliffContinuity`, `CliffWaterContact` and `RouteAndSteering` pass in `Saved/CraterSeamTests.log` at20:42 UTC. The standalone production-source geometry/flow probe passes in `Saved/CraterSeamProbe.log`. No main merge, packaged build or publication.

Actual post-change captures completed without a fatal error, new material compile failure or contact ensure. The final isolation helper also builds for Editor/Game (`WaterfallIsolatedEditorBuild.log`, `WaterfallIsolatedGameBuild.log`). Both the beauty run and isolated-volume exit clear the two volume actors and restore sky/fog. Sparse screenshot-time performance values are not a long-run benchmark, and the existing non-Nanite shadow-marking overflow remains open.

## Actual render critique

The matched late-descent beauty and normal views confirm that the large stretched inlet triangle and inner star-shaped texture pinch are gone. The shore now turns continuously; a thin brown shoreline strip and the broad smooth water bank still need art refinement. This is a concrete defect correction, not a claim that the water has reached the reference.

The corrected surface-water core no longer uses triangles crossing unrelated cliff peaks, but it visibly follows ledges and overhangs too closely. Dark blue/black bands remain where those surfaces turn away from the light. The cliff still looks blocky close up. A positive contact-clearance test cannot make water wrapping around a ledge physically convincing.

The fitted volume follows the measured reach, but the combined image is still dominated by the bright ribbon-like core. Added `-DCFallVolumesOnly` with the close camera to hide the surface-core/impact mesh; it leaves the surrounding atmosphere intact and restores visibility on cleanup. That view reveals weak, mist-like volume detail rather than convincing aerated falling water. The thin streak left of the main fall also appears in the zero-density/core review, so the earlier attribution of that streak specifically to the VDB was not established. Isolate the remaining atmosphere/contact components before assigning its cause. Keep all fitted volumes opt-in.

The continuous loop-boundary run requested targets 305.8–306.2 s. Screenshot readback stalls meant the actual logged times were **305.8, 306.2, 306.5, 306.8 and 307.1 s**. These still straddle the loop boundary, but they are not five adjacent frames or a full-motion review. Offline exact-period checks pass; no claim of full visual loop/performance acceptance.

Retained actual screenshots: `taskboard/review/crater-seams-fitted-14nov/descent_0420.png`, `normals_0420.png`, `core_contact_0300.png`, `fitted_combined_03060.png` and `fitted_core_hidden_0300.png`. Full runs are under `Saved/SceneReview/crater-seam-after`, `crater-seam-normal-after`, `waterfall-fitted-close` and `waterfall-fitted-isolated`.

## Next pass

1. Fix the first valid water-contact point and remove the submerged dip/upward curl. Synchronize the impact plume and surface-foam source to that point. Include a receiving-water coverage mask, not just extrapolated heights outside the basin.
2. Replace selected ledge-wrapping reaches with gravity-driven detached drops and outward spray; judge one source with the core hidden and then combined. Do not multiply a weak prototype around the crater.
3. Address the broad smooth hydraulic banks, overly uniform distant ripples, remaining outer-flow strain and close cliff faceting. Preserve the corrected inlet seam, inner-flow regularity, crater size and steering clearance.
