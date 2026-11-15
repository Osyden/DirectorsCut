# Escarpment workflow: physical adaptation tested, primary landform still rejected

Branch: `codex/maelstrom-saturday`. **The game scene is unchanged.** The verified upstream method now has two fresh, physically sized adaptations to the measured crater coast. Both fail actual visual review. Their boundary and numerical checks pass separately; neither is a finished cliff or ready for integration.

## Latest physical adaptation, 15 November 03:53 pass

`Tools/generate_fastscape_crater_sector.py` builds a fresh 500 x 504 m sampling domain at 2 m spacing in the exact source frame. Its triangulated land spans 500 x 492 m with about 208 m vertical range. It uses the measured coast and external seams, without reusing the old interior, shrinking the 20 km reference or exaggerating height. All 992 external edges and 5,664 fixed physical samples remain exact. Invalid padding cannot feed the drainage; 251 actual coastal outlets are the only flow base levels. See [implementation and reproduction](FASTSCAPE-PHYSICAL-SECTOR-15NOV.md), [independent audit](CLIFF-FASTSCAPE-ADAPTATION-AUDIT.md) and [actual visual review](FASTSCAPE-CRATER-PHYSICAL-REVIEW.md).

The original grid-only ADI smoothing moved constrained points and coupled excluded padding into land. It was rejected before export. A uses stream-power incision alone. B retains precisely the same initial landform and incision settings, adding stable valid-edge diffusion with explicit material exchange at fixed boundaries. Independent balance, no-new-extrema, padding isolation and boundary checks pass. These establish numerical behavior, not art approval or calibrated geology.

- **A fails as vertical drapery.** A narrow, roughly uniform rise and nearly flat contributing plateau produce many parallel full-height grooves. Drainage on top does not form convincing unequal cliff masses. [Actual wall](review/cliff-fastscape-15nov/physical-a-wall.png), [oblique](review/cliff-fastscape-15nov/physical-a-oblique.png).
- **B fails as a smooth dam.** Lateral transport removes most grooves without adding the missing mass hierarchy. The flat lid and deep fitted recess at the left remain. It tests the transport hypothesis and disproves its sufficiency; another diffusion-strength sweep is not the next step. [Actual wall](review/cliff-fastscape-15nov/physical-b-wall.png), [oblique](review/cliff-fastscape-15nov/physical-b-oblique.png).

Root and independent workers inspected these matched CPU renders. Mean-water context is the measured static analytic surface in the same source frame, without animated waves. Primary wall/oblique views fail, so distant/native promotion was unwarranted. B's locked close camera becomes nearly featureless/occluded by the changed surface and is not independent proof of detail quality.

Existing temple actors were untouched. Their islands were not frozen into the new simulation, and the proposals do **not** support their old foundations: some are buried about 12-14 m, others lose tens of metres of ground beneath them. A coast/seam pass does not certify foundations. Any eventual support or placement revision must be explicit and retain the irregular city composition.

## Next construction: parent terrain before cliff extraction

Build one physically sized wider hinterland with unequal divides, branching valleys and coastal outlets before selecting its exposed cliff. The official [Mountain model](https://fastscapelib.readthedocs.io/en/latest/examples/mountain_py.html) offers uplift opposed by channel/hillslope erosion as a process reference. Do not rescale its 75 x 50 km demonstration or repeat a long reference-verification exercise. Define metres, elevations and contributing extent first. Preserve the actual coast and ship route, but judge the complete parent form before imposing old arbitrary tile seams. Terrain beyond the measured crop is authored proposal data; it cannot certify neighbouring scene support. Generate matching sector seams from an accepted parent field later.

Favor low irregular upper plateaus and deep side ravines for this arena, so temples and columns remain clear against the clouds. Their supported placement follows the accepted surface; no floating or frozen pedestal islands. Accept near and opposite-rim views before extending around the crater.

Then extract steep faces into detailed three-dimensional rock geometry. [SideFX's Project Pegasus workflow](https://www.sidefx.com/tutorials/project-pegasus-cliff-generation/) follows terrain creation with cliff extraction, subdivision/displacement and tiled Nanite imports. This is a production reference, not a claim that Houdini is installed. Channel erosion alone does not generate joint planes, stratified ledges, undercuts or mineral detail.

[Terrain material layers](https://www.sidefx.com/docs/houdini/heightfields/texturelayers.html) can blend physically scaled stone with slope, curvature and drainage masks, using triplanar projection on steep faces. Use a sediment layer only if the selected solver actually computes one; this SPL/diffusion test does not. Existing slate material can support a later proof, but texture detail cannot rescue these primary shapes. No purchase is needed to continue the landform work.

No new runtime build, normal-game replacement, cloud/light/water/boss edit, main merge or packaged release occurred. The following sections retain the preceding rejected trials and upstream reproduction.

## Actual rejected geometry

The new `Tools/generate_jointed_bedrock_sector.py` starts with one continuous 500 m source solid and a shared anisotropic Voronoi partition. It removes selected surface cells without excavating a central window or adding separated hulls. The first 26-cell selection makes polygonal quarry pockets. A ten-cell convex-shoulder revision reduces clutter but leaves the main soft silhouette unchanged. Root and independent reviewers rejected both.

The final revision is technically sound after cancelling a pair of exact opposite-winding duplicate faces: one open manifold, 52,206 vertices / 103,418 triangles, all 992 external boundary edges unchanged. All 16,359 original protected triangles and their 9,324 vertices remain exact. Numerical cleanup does not alter the rejected appearance. The earlier collection Boolean's topology failures remain recorded. See [generator](CONTINUOUS-BEDROCK-15NOV.md), [audit](CLIFF-CONTINUOUS-BEDROCK-AUDIT.md) and [visual review](CONNECTED-BEDROCK-VISUAL-REVIEW.md).

- [First cell-release result](review/cliff-continuous-15nov/rejected-cell-releases.png)
- [Corner-release revision](review/cliff-continuous-15nov/rejected-corner-releases.png)
- [First result at the actual 3.2 km opposite-rim distance](review/cliff-continuous-15nov/rejected-opposite-rim.png)

The distant review adds a 3.2 km camera with the same 35 mm lens / 36 mm sensor as the existing 1.5 km comparison. A 500 m sector occupies roughly 230 pixels across the 1440-pixel image at that distance. Small rock features must diminish naturally; zooming the camera would conceal the scale problem.

## Deeper erosion is also rejected

The original source's latest erosion pass allowed at most 0.01 m removal per step and produced only 2.27 m maximum net lowering. Root added `Tools/probe_cliff_deep_erosion.py` to measure a substantially deeper response using the existing conservative solver, without another noise field. It expands the frozen set to include complete protected triangles and uses an explicitly authored erodible-depth budget, rather than claiming measured subsurface geology.

The actual 640-step probe lowers terrain by up to 52.43 m and deposits up to 17.17 m. Median absolute change over free land is 8.11 m; 64.75% changes by over 5 m. Conservation checks pass within floating-point error. An independent check confirms exact source frame, XY, domain, topology, outlets and every protected triangle, with no bedrock-bound violations.

Nevertheless, the [actual clay render](review/cliff-continuous-15nov/rejected-deep-erosion.png) is worse: smooth aprons, grid-like flutes and conspicuous pedestals around frozen temple platforms. The longest triangle edge grows from 27.26 m to 55.03 m. Preserving foundation coordinates is not enough to make their surrounding terrain believable. This test does not support another strength/timestep sweep. [Independent visual review](MACRO-EROSION-RESPONSE-REVIEW.md).

Actual data: `Saved/MacroErosionResponse/Deep640`; candidate SHA256 `d87ba287494eeb480131cd1355ddfaddaee43b4c275ffa6d851d31c82ba362c3`. Reproduce with Blender's NumPy Python and the new probe's `--source` / `--output` arguments. The input remains the immutable `Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`. Solver steps are accelerated morphology units, not calibrated geological time. No Unreal import/build or scene integration followed these failures.

## Complete upstream escarpment reproduced

We inspected and ran the official [Fastscapelib Escarpment example](https://fastscapelib.readthedocs.io/en/latest/examples/escarpment_py.html) at source commit `707fcc10976b0e729f1415fd16b0a161e1f746c3`, using pinned Fastscapelib 0.3.0. Its model cells run unchanged. It combines drainage routing, implicit stream-power incision and hillslope diffusion on a 20 x 10 km domain with a 400 m plateau step. The 100 m grid and model time remain the upstream values, with no mesh scaling or vertical exaggeration.

`Tools/run_fastscape_escarpment_reference.py` executes only the reviewed model cells and exports the original physical surface. The dependency is isolated under `Crater-Workers/Flow-Seams/Saved/FastscapeReference/python_packages`; Blender and system packages are unchanged. The downloaded notebook, full GPL-3.0 text and attribution accompany each generated bundle. The dependency and notebook are not linked or embedded in Unreal; see the [tracked notice](../Tools/ThirdParty/Fastscapelib-NOTICE.md) and [reproduction handoff](FASTSCAPE-UPSTREAM-REFERENCE-15NOV.md).

Independent replay reproduces initial elevation, final elevation and the saved last-step drainage array bitwise. The surface has 20,301 vertices and 40,000 triangles, positive-Z winding and one valid open manifold. The fixed left base level stays exact. Actual top/bottom boundaries are periodic (`LOOPED`), following the source code despite the documentation prose saying reflective.

Valid CPU clay images show connected branching ravines, interlocking spurs and escarpment retreat. That is useful evidence for this macro-process. The coarse 100 m grid also produces obvious stepped/faceted edges; this is not a game-ready cliff or visual acceptance of our arena. Initial full-domain images were blank because the existing renderer clipped at 10 km; a Saved-only 100 km far-clip override corrected the review without changing geometry. See [actual upstream visual review](UPSTREAM-ESCARPMENT-VISUAL-REVIEW.md).

- [Actual native-scale upstream plan](review/cliff-continuous-15nov/upstream-plan.png)
- [Actual native-scale 7 km detail](review/cliff-continuous-15nov/upstream-detail.png)

## Previous implementation plan, now exercised above

Use the verified model as an offline authoring component for one fresh, physically sized 500 m escarpment experiment with the actual coastal datum and a finer grid. Recalculate the terrain at those dimensions; do not shrink the 20 km reference mesh, stretch its channels around the crater, or feed the same rejected soft wall through another blind strength sweep. Set the broad cliff rise and drainage boundary conditions before erosion. Inspect the resulting complete landform before adding fractures or materials.

Preserve the arena's shoreline and route clearance. Treat temple support as a deliberate integration step with coherent surrounding ground, not proof that frozen islands look natural. Any proposed change in landmark elevation or support platform must be explicit, retain the irregular rim-city composition and use the accepted final surface; the current live landmarks remain unchanged. Do not silently relax supports or claim a free upstream test validates them.

Once a coherent primary escarpment is accepted at near, 1.5 km and 3.2 km distances, add selective original-3D rock faces and physically scaled stone detail. Then regenerate waterfall contacts and receiving foam from that same surface. Sky, lighting, water, boss and gameplay ownership remains unchanged. No main merge, package or performance claim this pass.
