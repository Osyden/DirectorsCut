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
