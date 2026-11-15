# Foam worker pass 2

Owner: Codex foam worker, `codex/crater-foam-pass2`, isolated `E:/Projects/DirectorsCut/Crater-Workers/Foam`, starting at `e7d7f8b`.

Claim: `Tools/make_maelstrom_materials.py` water-surface foam/shading only. No geometry, route, cliff, sky, whitewater-feed source or asset changes. Root owns Unreal build, material generation and GPU review.

Diagnosis from the actual latest `cliffs_0300.png` and the supplied reference: the water shader generates widespread aeration from bank steepness even away from actual breaking fronts and waterfall sources. It then promotes low-density coverage through an exponential body, square-root roughness and opacity that becomes fully opaque at half coverage. This makes smooth banks read as pale terrain. The reference instead retains substantial dark liquid intervals around broken whitewater.

Implementation in progress: source-driven foam coverage with separate fresh-front and aged-transported components; remove slope-generated white blanket, preserve physical-size shredding and filtered distant coverage, and apply resulting coverage consistently to opacity/roughness.

## Implemented change

`Tools/make_maelstrom_materials.py` now interprets the existing source atlas as concentration and converts it into actual covered surface area:

- Fresh foam remains tied to the existing crest mask and short moving breaking-lip packets.
- Aged foam is supplied only by actual downstream crest wakes and waterfall-contact trails. A transported, fixed-planar threshold breaks those tails into patches and filaments, with no opacity floor on the source fringe.
- Removed world-space slope aeration and the unrelated radial overflow contribution from the basin foam. Bank inclination alone no longer coats a whole slope in white.
- Removed exponential whitening and the minimum resolved bubble opacity. Distant unresolved bubbles still average to coverage, with explicit zero-source gating so filtering cannot invent foam.
- Water/foam roughness now mixes by covered area; opacity reaches full foam opacity at full coverage instead of at half coverage. Dark liquid gaps therefore preserve water response.
- The shared entrance-tunnel path keeps its previous wind-cap/coverage and roughness behavior. No water geometry/displacement, cliff, route, shore, sky or falling-feed code changed.

This is one focused coverage/response correction, not a complete fluid simulation or new breaking-wave geometry. The reference's volumetric splash and overturning crests remain outside this worker's change.

## Verification and integration

Python AST parsing and `git diff --check` pass. No Unreal build, material generation, GPU capture or visual-quality acceptance has run in this worker by design. Root must cherry-pick the commit and regenerate materials via `Tools/make_maelstrom_materials.py` in the main isolated render checkout. There are no C++ changes, so the source target need not rebuild solely for this commit. The generator writes the existing original water/cascade/spray assets; inspect/stage the generated outputs only after a successful compile.

Review matching native deck views at requested300 and340, plus adjacent frames310/311/312 and a foam-only diagnostic. Verify that genuine waterfall contacts still seed visible trails, dark intervals are larger, narrow fresh fronts remain readable, and distant foam does not flicker or become an all-over speckle. Check the entrance tunnel once because the material is shared. If aged coverage becomes too sparse, adjust the concentration-to-coverage mapping after viewing those images; do not restore a slope-based white blanket or bubble opacity floor.

Files intentionally unchanged: `DCMaelstrom.cpp/.h`, `DCMaelstromBreakers.cpp`, `DCMaelstromWhitewater.cpp`, route, cliff generator/material, sky, vendor content and existing generated uassets.

## Follow-up after actual pass A, 14 Nov 19:58

Inspected the new native `Saved/Screenshots/Auto/shot_0300.png` and `shot_0340.png` in the render checkout against the committed cliff-review baseline. Dark liquid intervals on steep faces improved. The foreground still contains broad pale areas and the bank silhouette remains smooth and terrain-like. A beauty frame alone cannot establish how much of that pale area is foam versus sky reflection/scattering; root will capture matching `-DCWaterReview=1` diagnostics.

The remaining material causes addressed in this focused follow-up are:

- Fresh fronts still had a 0.55 filament floor followed by a 0.65 bubble floor. Thus a filtered crest footprint could never have actual water holes. Fresh coverage now uses source concentration, a transported threshold, filaments and bubble coverage, with no such floors. Only the current geometric crest or moving lip replenishes fresh foam.
- Advected red crest marks were previously promoted back to fresh foam up to approximately 250m downstream. They now join the aged concentration population, retaining the existing lifetime fade. Atlas samples are gated by availability so the default white placeholder cannot create foam when the runtime atlas is absent.
- Replaced the basin's three endless planar cosine detail waves with 1.4m, 3m and 4.8m wind-wave packets. Their normals use the derivative of the packet envelope and fade by pixel footprint before becoming unresolved. This breaks coherent highlight bars without changing mesh displacement or adding geometry. The entrance tunnel retains its previous detail waves and foam response.

This revision does not change the giant bank profile, the route, the bottom footprint, or any waterfall geometry. It cannot make those silhouettes turbulent by itself. If the new diagnostic shows black foam where the beauty frame is pale, do not continue reducing foam there: inspect the reflection/lighting and bank geometry instead.

Validation: Python AST and whitespace checks only; root must regenerate `Tools/make_maelstrom_materials.py` and compile the graph. No UE, GPU, source build, or visual acceptance in the worker. Capture the matching beauty and foam-mask views plus adjacent frames to check that fresh lips have not become too sparse, and that subpixel packet normals remain stable.
