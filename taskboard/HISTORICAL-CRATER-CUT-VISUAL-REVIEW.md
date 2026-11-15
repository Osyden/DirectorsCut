# Historical headland and crater cut: independent visual review

15 November, after root `8b84641`, on `codex/implicit-exposure-review`.
**HeadlandA is a useful construction/contact proof, but its exposed face is
not accepted as finished rock or for native art promotion.** No Unreal,
runtime scene, shader, water or cloud changes were made by this review.

## Actual geometry and placement

Author artifacts: `Crater-Workers/Flow-Seams/Saved/HistoricalCraterCut/HeadlandA/`.
Visible candidate SHA256:
`fd6933153a11ad39b9010d956cb3f90e54931f5f2c032959fa0892417f72d6ff`.
Baseline SHA256:
`4d4006d3b9895862c60d9a42bd19eb7c239f13ece20ffdfb2f5b9d4976dd07fb`.
The baseline is the selected, rigidly placed historical parent surface before
excavation, not the old game cliff. The candidate includes only original upper
terrain and new cut faces; artificial authoring bottom/rear caps are excluded.

Placement explicitly translates the historical parent by **V=−600 m and
Z=−55 m** in the original source chart, with no rotation, bending or scaling.
The selected headland is U=40..440 m, V=0..420 m; the visible result spans
400 × 378.861 × 204.645 m, with maximum elevation +45.230 m. Original top
triangles remain unchanged except at the cut boundary. The new excavation is
a 70-degree surface anchored to the sampled modern shore, with a 4 m inward
toe overlap. This is original triangle/volume construction, not a renderer
displacement or a projection back into the old wall heightfield.

## Captures and actual visual finding

This worker's ignored `Saved/ImplicitCliffReview/HistoricalCraterCut/` contains:

- `HeadlandA/wall_fractured.png` and `oblique_fractured.png`: visible geometry
  without water, useful for seeing the entire exposed surface.
- `HeadlandAWater/wall_fractured.png` and `oblique_fractured.png`: same geometry
  and cameras with the measured static mean-water mesh.

Each directory retains paired `_baseline.png` images and `review.json`.
The existing source-frame 500 m wall/oblique cameras were reused without fitting
or zooming to this narrower 400 m headland. The oblique includes its complete
upper landform. Neutral CPU Blender clay used eight threads, 24 samples and
continuous angle-179 normals: zero sharp shading edges, geometry unchanged.
The recorded geometry hash is
`9ac291082c76c28cce9719d5a9c25c44e12d3b1c51f11dc7e7410be23c64d4fc`.

The branching upper terrain remains coherent with unequal ridges and valleys.
Its restrained upper height and irregular skyline are more useful than the
previous smooth forced domes. However, the excavation adds a broad, almost
featureless wall across the front. Large exposed areas have no readable rock
mass relief, recesses or geological planes. The valley mouths meet this wall
abruptly. This is a real smooth cut surface, not the previous angle-15 normal
striping artifact.

The water-context views hide the audited submerged toe but leave that blank
front plainly visible. The curtain criticism therefore does not arise merely
from showing an underwater closure in isolation. No distant render or export
was justified as a substitute for resolving this primary exposed surface.

## Contact and mesh checks are separate from art approval

The existing reviewer finds one valid open surface: 33,647 vertices, 66,267
triangles, 1,025 boundary edges, with no invalid edge, winding, duplicate or
degenerate flags. It does not certify exhaustive self-intersections or supports.

The independent audit worker confirmed exact rigid correspondence on retained
natural top, a clean closed authoring bedrock body, and no caps in the visible
export. At 201 shoreline controls the candidate matches the interpolated
receiving surface within floating-point error; finite-water triangle checks
put the lower overlap 11.13–11.29 m below mean water. Root's fresh analytic
vertex/centroid query reports sampled dry rock landward of water coverage,
with minimum sampled dry route margin 35.0046 m. These are **static local
sampled checks**, not complete animated-wave, full-route or temple certification.

Only after those checks was water context added. Its source-frame equality is
enforced by the renderer; water mesh hash is
`bd4514e69d97799d52a78c2100d39a5d85be6b02662036a2866d818333177ff2`.
Old outer seams and temple foundations remain unfit, and no existing actors
were relocated. The historical parent itself remains nonconverged.

The next bounded surface step should preserve this parent/cut relationship and
audited toe, while giving the exposed face a few coherent geological masses and
recesses related to its existing headlands/valley mouths. Another parent solve,
texture application or shading change cannot establish that missing geometry.
