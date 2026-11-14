# Rock crater and turbulent lower basin

The previous approach was structurally wrong: a water heightfield covered the whole crater up to its rim. Material breakup could not stop it reading as shiny terrain. The owner now wants exposed mountainous rock, waterfalls over those walls, and turbulent water confined to the lower basin and the ship's descending current.

## Build order and acceptance

1. **Separate land from water.** End the basin at an irregular shoreline inside the crater. Preserve the 3.2km rim, 900m depth and descending ship route, including a wider inlet where the route enters. The opposite rim must show a substantial exposed rock face, not a water ramp.
2. **Build connected rock walls.** Add an original procedural cliff annulus with buttresses, gullies, fractured strata and secondary ridges. Use physical-scale rock shading and wet lower faces. No stretched rock mesh enclosing the entire crater; existing ruins stay as scale markers. Terrain must meet water without cracks and support the landmarks.
3. **Connect waterfalls to both surfaces.** Use distinct overflow notches across the exposed walls, with short free-falling sections, impact spray and foam transported into the lower basin. Falls must not be a continuous ring or long suspended diagonal ribbons.
4. **Make the water visibly move.** Keep large-scale circulation but introduce irregular overlapping wave packets, larger nearby crest displacement and current-aligned short chop, with foam tied to breaking/contact. Keep small features small across the kilometre-scale view. Preserve hull clearance and steering space.
5. **Review actual renders.** Build and run reveal, contact, later descent and exit checks. Inspect both geometry and shaded views if necessary. Reject water-to-rim coverage, terrain gaps, repetitive cliff silhouettes, detached falls and broad painted foam. Save actual screenshots and report remaining gaps honestly; shader compilation alone is not visual acceptance.

## Ownership

Work stays in `codex/maelstrom-saturday`. New terrain code/materials belong here. Shared `DCMaelstrom.cpp/.h` changes are limited to terrain/water geometry and wiring. Clouds, atmosphere, light settings, early-game systems, ruins library and boss mechanics remain with their current owners. Sky chat has been notified of the boundary.

## Validation

Structural checkpoint built and captured. This is a substantial land/water separation change, **not AAA visual acceptance**. The final images below expose the remaining art problems.

## Implemented structure

- Basin mesh now ends at `WaterEdgeRadius`, an irregular lower shoreline around 66% of rim radius away from the route. It widens where needed for the incoming river and steering envelope. The 3.2km crater rim, 900m depth and route remain unchanged. The water no longer fills the entire space up to the opposite rim.
- `DCMaelstromTerrain.cpp` builds a connected 115,825-vertex rock annulus with buttresses, gullies and outer ridges. Larger geometric frequencies avoid the original trial's blocky small-scale relief. Installed Ancient Ruins cliff/rock meshes add detail with uniform scaling, protected route clearance and clear waterfall channels; no purchased files are copied or modified.
- Original `M_CraterRock` uses local-space triplanar detail, varied stone colour, warped strata and wet lower faces. Run `Tools/make_maelstrom_rock.py` after the usual water generator. Installed tile textures are referenced in place, so Ancient Ruins remains a setup dependency.
- Discrete waterfalls share notch positions with the terrain. Their bed is carved below the falling stream; streams begin on rock and meet the lower water. Several wider falls provide stronger visual accents; narrower branches and impact trails connect them to the current.
- Shorter, steeper standing breakers replace the long low ridges; increased wind-wave displacement remains hull-protected. Moving crests now have bounded lateral compression in the surface tangent frame as well as vertical displacement. The existing dynamic packet/foam system and local spray carry movement through the lower basin.
- Old water-coated wall and rim-strip mesh generation removed. Clouds, sky, lighting, ruins library and boss code untouched.

## Iteration / integration notes

Trial A demonstrated the exposed-rock separation but had a near-vertical uniform foot and faint falls. Trial B softened the profile and strengthened falls; it exposed rock/noise detail that was too blocky and streams following every protrusion. Trial C uses larger terrain features and shared carved drainage channels with airborne falling sections.

Dynamic rock shadows are limited to the nearest twelve instances within 600m. A first-frame VSM job warning is also present in pre-terrain captures (`turbulence-review` and `turbulence-filtered`); the new shadow budget does **not** establish that this baseline warning is resolved. It bounds added distant-rock work. No controlled performance benchmark while other Unreal checkouts are running.

For the sky owner: `GetShorelinePoint(angle)` now returns the actual lower shoreline in basin coordinates. Old atmosphere placement based on `GetRimJoin` still refers to the original upper lip profile; use the new point for future waterfall-contact mist placement. This pass deliberately leaves that atmosphere code untouched.

## Checks completed

- Editor builds and both original material generators pass.
- Trials A, B and C continuous descents restored open sea. Trial C checked reveal at 282s through the ending at 451s; its log records stage exit at 450.01s. The 451s capture confirms terrain is hidden after exit.
- Runtime geometry check finds a minimum **35m** clearance beyond full outward steering plus a 15m hull allowance. Authored rock instances have an additional conservative bound check against the route corridor.
- Final review captured 300, 310, 311, 312 and 340s continuously, with the latest crest compression, waterfall source fade and extended contact. Review helper exited successfully; no maelstrom material compile failure, fatal error or shoreline ensure failure. Logs still contain unrelated UDS Editor Utility, Pawn EnableInput and experimental Toolset Python startup errors. This is not an error-free whole-project certification.
- These are offscreen native 1920x1080 game renders. There was no human motion assessment, packaged build, main merge or controlled performance test in this pass.

## Actual captures

- [Final lower basin / exposed cliffs, 300s](review/maelstrom-rock-basin-14nov/final_0300.png).
- [Final falls and nearer water, 310s](review/maelstrom-rock-basin-14nov/final_0310.png); [311s](review/maelstrom-rock-basin-14nov/final_0311.png) provides an adjacent frame for motion comparison.
- [Final descent, 340s](review/maelstrom-rock-basin-14nov/final_0340.png).
- [Trial C late descent, 420s](review/maelstrom-rock-basin-14nov/trial_c_0420.png), [open-sea restoration, 451s](review/maelstrom-rock-basin-14nov/trial_c_0451.png). These two precede the last crest/material and waterfall-source adjustments; do not label them final-shader captures.

## What still fails the reference, and the next art pass

1. **Rock surfacing and silhouettes.** The connected cliff reads as soft procedural terrain in several areas; isolated rock instances stand out as ornaments instead of coherent geology. Build clustered buttresses and ledges following drainage, blend their colour/roughness with the substrate, and add intermediate fracture detail between 5m texture tiles and 100m landforms. Avoid enlarging tiny noisy features into distant mountains. Review from the real 300s and 340s deck cameras.
2. **Falling water.** Some branches still read as bright flat strips, especially the narrow distant falls. Their shared notches fix placement, but shading alone does not supply volume. Add broken water cores with finer outer spray, accelerating flow streaks and asymmetric impact plumes. Keep airborne spray lighting tied to scene illumination. Preserve visible dark gaps between streams; don't fill the cliff with white cards.
3. **Turbulence.** The lower water is rougher and animated, but broad near-ship areas remain too smooth and foam too isolated. Add connected breaking fronts, collapsing crest spray and downstream foam filaments/eddies driven by the same circulation field. Preserve the protected hull footprint while allowing high-frequency normal detail beside it. Do not solve this by increasing all wave amplitudes or painting foam everywhere.
4. **Water shading artifacts.** The 300/310s frames show a grid-like near-water pattern in the ship reflection/shadow region. Diagnose reflection/shadow contribution with controlled render toggles before changing geometry. A shadow-queue startup warning is known, but its presence alone does not establish the cause of this pattern.
5. **Atmosphere integration, separate owner.** The old mist is anchored to the former upper water join; it can float or form conspicuous blobs after this structural change (visible late in trial C). Sky owner should use `GetShorelinePoint` for receiving-water mist and budget the plumes. Clouds/lighting were deliberately not changed here.

Accept the next art pass only after the same deck views show coherent fractured stone, three-dimensional falling water, crest breakup and connected foam transport; then inspect a normal-speed descent in motion before returning to boss work.

## Integration

Work is in the isolated `codex/maelstrom-saturday` branch. Merge shared `DCMaelstrom.cpp/.h` by function alongside the sky owner's work. The new source file needs UBT source discovery (`-gather` on the Editor build). Install Ancient Ruins in the destination project; this change references its meshes/tile textures and does not redistribute them. The original generated `Content/DC/Maelstrom` materials are included; to regenerate, run `Tools/make_maelstrom_materials.py` followed by `Tools/make_maelstrom_rock.py` through Unreal Python. Preserve the existing DRW setup/patch requirements from the main handoff. No purchased asset edits or copies are part of this commit.
