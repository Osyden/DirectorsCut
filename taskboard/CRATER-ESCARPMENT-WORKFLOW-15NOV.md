# Escarpment workflow: rejected cuts and erosion, verified upstream reference

Branch: `codex/maelstrom-saturday`, following `235a0fa`. **The game scene is unchanged.** The connected-cell and deeper-erosion trials fail their actual visual reviews. A complete upstream escarpment model now reproduces correctly and provides a better starting point for coherent drainage and cliff retreat, but has not been adapted to our arena.

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

## Next implementation

Use the verified model as an offline authoring component for one fresh, physically sized 500 m escarpment experiment with the actual coastal datum and a finer grid. Recalculate the terrain at those dimensions; do not shrink the 20 km reference mesh, stretch its channels around the crater, or feed the same rejected soft wall through another blind strength sweep. Set the broad cliff rise and drainage boundary conditions before erosion. Inspect the resulting complete landform before adding fractures or materials.

Preserve the arena's shoreline and route clearance. Treat temple support as a deliberate integration step with coherent surrounding ground, not proof that frozen islands look natural. Any proposed change in landmark elevation or support platform must be explicit, retain the irregular rim-city composition and use the accepted final surface; the current live landmarks remain unchanged. Do not silently relax supports or claim a free upstream test validates them.

Once a coherent primary escarpment is accepted at near, 1.5 km and 3.2 km distances, add selective original-3D rock faces and physically scaled stone detail. Then regenerate waterfall contacts and receiving foam from that same surface. Sky, lighting, water, boss and gameplay ownership remains unchanged. No main merge, package or performance claim this pass.
