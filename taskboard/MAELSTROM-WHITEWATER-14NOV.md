# Whitewater, impact trails and moving crests

Owner: Codex water chat, isolated `codex/maelstrom-saturday` checkout. Builds on the banked spiral in `73300a6`. Crater dimensions and route are unchanged. Clouds, lighting, atmosphere source, boss behaviour and early-game systems remain outside this pass.

## Implementation

- Added `DCMaelstromWhitewater.cpp`: twelve irregularly spaced tributaries run from the outer rim to explicit contact points along the spiral. Their widths vary along the fall, widening toward the contact. Falling sections lift off the slope on a gravity-shaped arc, stay above the CPU basin surface, and reconnect at the contact. An original lit translucent material supplies streaming metre-scale breakup. Some feeds cross toward the visible inner part of the course. Hull proximity fades prevent the overlay crossing the ship.
- Each impact generates a downstream trail following the route tangent. The original transient source atlas now stores crest sources in red and contact/trail density in green. Shader advection and CPU trail placement use the same descending-current direction. A stationary sample anchors the source; advected samples move bubbles away and fine breakup prevents filled white islands.
- Added small, staggered curved spray lobes and gravity-driven droplets at contacts. The new `M_MaelstromImpactMist` also replaces the old plume material on crest spray. These are local water effects, not changes to the cloud chat's sky or atmosphere materials. The spray uses directional volumetric translucency lighting, zero specular and no emissive glow.
- Added packet-modulated travelling crests with matching height/normal derivatives and narrow breaking-lip foam. Wave heights stay in metres; distant displacement fades rather than enlarging the waves. Reduced static noise relief so animated water motion carries more of the surface.
- Existing optional DRW wind-wave displacement remains bounded and hull-protected. Surface trails are part of the water material, not floating opaque decals. The feed overlays and particle origins follow the CPU surface; they do not read GPU wind-wave height, so this remains an authored effect rather than fluid simulation.

## Iteration and limits

The first pass had thin parallel-looking white feeds and too much solid white foam. Reduced the feed count, varied widths/placement, broke up opacity at several scales, removed the impact-mask coverage floor and narrowed the moving foam lips. A second pass exposed contacts hidden on the ship's far side; some feeds now enter the visible current. These choices preserve the crater scale.

The close-contact review exposed three parallel spray sheets reading as horizontal shelves. Replaced those with eleven smaller lobes at staggered positions, varied widths and heights, and crowned profiles. Removed surface specular from mist. Waterfalls are authored falling sheets rather than a full volumetric simulation. Their distant silhouettes still read too much as ribbons, the distant slopes remain too smooth, and the old rim mist repeats. Do not describe the scene as finalized or photorealistic on the strength of a shader build.

## Integration

Build the Editor target with `-gather` on the first checkout so UBT discovers `DCMaelstromWhitewater.cpp`, then run `Tools/make_maelstrom_materials.py` in that checkout. Original generated assets live under `Content/DC/Maelstrom`; purchased source and textures were not modified or copied. The material generator also regenerates the existing cascade/spray materials because they share its noise helpers.

Merge `DCMaelstrom.cpp` and `.h` by hunk with the sky branch: this pass only adds the whitewater component/lifecycle, changes water wave amplitude/static relief and wires whitewater generation/update. No sky values or atmosphere functions were changed. No main merge or packaged release is part of this pass.

## Validation checkpoints

- Editor build and material generation succeeded. The native 1920×1080 `whitewater-c` continuous scene review ran from 295 to 451 seconds and restored open sea. This preceded only the final lifted-sheet geometry, radius parameter and crest reveal wiring; it validates the new effects' stage lifecycle.
- The lifted-sheet revision passed the 300–342-second review, including consecutive frames. A subsequent close-contact review at 309–316 seconds exposed the spray-shelf issue described above. Existing plugin/editor-utility startup errors are unrelated; the review helper specifically rejects a maelstrom material shader failure.
- No changes to cloud composition, lighting values, crater dimensions or route timing. `/Game/DC` already has an always-cook rule covering the new original materials, but no packaged build was made.

- Final spray-lobe revision built successfully and passed a native 1920x1080 continuous contact review at 309-316 seconds. Visual inspection confirms the three horizontal shelves are gone; individual spray lobes remain visible and need further art tuning. Other checkouts were rendering concurrently, so these runs are not performance benchmarks.

## Actual engine captures

- [Wide view at 300 seconds](review/maelstrom-whitewater-14nov/wide-0300.png) and [descent at 340 seconds](review/maelstrom-whitewater-14nov/descent-0340.png): lifted waterfall geometry and moving crests, before the final spray-lobe correction.
- [Contact at 310 seconds](review/maelstrom-whitewater-14nov/contact-0310.png), [311](review/maelstrom-whitewater-14nov/contact-0311.png), [312](review/maelstrom-whitewater-14nov/contact-0312.png): final shader and spray geometry, consecutive actual engine frames. Original PNGs, no paintover or retouching.

Next water priorities: break the remaining straight feed silhouettes into branched falling strands, improve distant flow detail without enlarging physical wave scale, and reduce the visibility of individual spray lobes. Keep clouds/lighting with the sky owner. This is a pushed review checkpoint, not a claim that convincing whitewater or the reference image has been fully achieved.

## Follow-up: turbulent slopes and branched feeds

The owner rejected the remaining ribbon falls and smooth slopes. This follow-up keeps the route and crater dimensions, and changes only water geometry/materials and local water spray:

- Four unequal, meandering branches per feed replace its single sheet. Short sections separate from and rejoin the slope; their height above the slope is bounded, removing the long diagonal suspended sheet. Streaming opacity has metre-scale gaps rather than uninterrupted white strands.
- Retain resolved wave normals/displacement across the crater instead of suppressing them at 1km. Physical wavelengths and heights remain small. Add sparse, advected aeration on steep banks; the impact field continues to supply downstream trails.
- First trial exposed a diagonal comb in distant whitecaps. Replaced the one coherent breaking sine with three irregular short packets, keeping analytic height derivatives for normals. The first trial is rejected, not a deliverable.
- Smaller staggered impact lobes, less opaque spray, finer breakup and more small ballistic droplets replace the broad soft puffs. No atmosphere, sky material or light setting changes.

The second trial exposed broad, painted-looking white bands near the ship. A foam-only diagnostic confirmed these were coverage, not merely sky reflections. Narrowed the moving breaking lip, removed its coverage floor, broke foam into smaller cells and retained dark water between impact trails. Strengthened short chop and raised the minimum water roughness from .22 to .30. Increased sparse bank aeration separately so the distant slopes retain fine whitewater without those broad bands.

Validation so far: Editor rebuild and material regeneration passed; trial B ran continuously from 295 to 451 seconds and restored open sea at 450.02. Final spray/foam revisions have separate short scene reviews; no route, boss, atmosphere or lighting code changed. Main checkout and packaged build are unchanged. Parallel Unreal processes were running, so the captures are not a controlled performance benchmark.

The higher-aeration trial revealed distant speckling/grid-like breakup. Warped the coarse patch field and added screen-footprint filtering: subpixel bubble/streak detail averages to coverage while resolved nearby foam retains its fine texture. This does not enlarge the distant wave features. It is a spatial filter; temporal stability still needs a human motion review.

The feed shader also has fixed gaps between aerated reaches, independent of the moving noise. This prevents temporal averaging of streaming noise from reconstructing an uninterrupted white ribbon. The underlying water remains continuous through the darker sections.

Final material generation and the `turbulence-review` continuous capture run passed, including fresh 1920x1080 images and no maelstrom shader-compilation failure. Source rebuild passed after the final geometry/particle changes. The filtered water revision also passed a continuous 295-340-second review; the last change after that only added fixed gaps in falling whitewater opacity.

Actual, unretouched engine captures:

- [Wide view, 300 seconds](review/maelstrom-turbulence-14nov/shot_0300.png).
- [Impact pass, 310 seconds](review/maelstrom-turbulence-14nov/shot_0310.png), [311](review/maelstrom-turbulence-14nov/shot_0311.png), [312](review/maelstrom-turbulence-14nov/shot_0312.png): consecutive frames from the final build/materials.
- [Later descent, 340 seconds](review/maelstrom-turbulence-14nov/filtered-descent-0340.png): same filtered surface and spray, before the final fixed gaps in waterfall opacity.

Visual assessment: falls now have darker interruptions and smaller branches; distant water has resolved texture instead of the initial diagonal comb, and local spray no longer forms dense soft puffs. Remaining issues are the overly even broad bank silhouette, some narrow feed silhouettes, and soft-looking near foam. This is an incremental water checkpoint, not completed photorealism. A subsequent geometry/art pass should vary the bank crest and spill faces while preserving the route and physical wave scale. The shared sky/rim lighting composition is still the other chat's responsibility; it was not changed to make these captures look better.
