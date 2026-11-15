# Cliff structure: rejected surface-detail trials and the next algorithm

15 November, continuation from `10b1fdc` on `codex/maelstrom-saturday`.
**No new terrain has been promoted into normal gameplay.** The scene is still
below the visual target; boss, water/falls, early game and sky/light are unchanged.

## What this pass established

The measured 500 m massif was refined conformingly to 1,025,013 vertices and
2,045,576 triangles, with original-triangle correspondence and numeric transport
masks retained. Two field-based detail trials and one separately authorized
radial-shear crop were actually rendered and rejected:

- Ridged/heterogeneous relief makes a softly lumpy skin over the broad slopes.
- Manhattan Voronoi relief adds mottling and weak repeated marks, without a
  convincing hierarchy of broken rock faces.
- Radial shear makes stronger genuine 3D relief, but the resulting ridges remain
  puffy. Small overhangs are real; they do not establish good-looking geology.

For the first two trials, keeping projected triangles positive required reducing
the displacement normal's horizontal components to 0.125. Median relief normal
to steep source faces was only 0.136/0.151 m. The radial crop preserves U/Z and
instead displaces V by a field independent of V, lifting median normal relief to
0.311 m. Its continuous invertibility argument and discrete crop checks are
separate; see [radial proof](CLIFF-RADIAL-SHEAR-PROOF.md) and
[independent audit](CLIFF-RADIAL-SHEAR-AUDIT.md). It contains no protected source
points, so its fixed-point check does not approve real shore or temple supports.

The full-sector variants preserve all 153,790 fixed and 4,448 boundary vertices,
plus masks and covered temple samples. Actor_36 still extends outside this tile.
Original-mesh BVH/oracle comparisons pass. These are geometry-contract results,
not visual acceptance. [Generator](WEATHERED-CLIFF-MESH.md),
[full audit](WEATHERED-CLIFF-INDEPENDENT-AUDIT.md),
[matched close/deck/far review](CLIFF-WEATHERED-REVIEW-PASS10.md).

Retained representative failures are under `taskboard/review/crater-pass10-15nov`.
The regular lower teeth and cut sides are inherited authoring boundaries; they
must be stitched to the actual arena/water before any final integration, rather
than dismissed as harmless because they match a baseline.

## Decision: stop tuning generic surface noise

Deformation, subdivision and source-query infrastructure are now sufficient to
test a genuinely different shape. More amplitude, vertices, noise octaves or
normal smoothing would repeat the failure. The missing feature is a coherent
set of exposed geological faces, joints and irregular broken edges.

A directly relevant primary reference was found:
[Paris et al., Modeling Rocky Scenery using Implicit Blocks (2020)](https://perso.liris.cnrs.fr/aparis/public_html/projects/paris2020_Blocks.html).
Its method constructs fracture-defined volumetric blocks and combines them on
steep terrain. The authors provide an
[MIT reference implementation](https://github.com/aparis69/Rock-fracturing).
The repository explicitly says it is a reimplementation and omits the paper's
implicit replication, periodic/aperiodic tiling and some scenes. Do not promise
paper-quality results or a ready-made complete cliff generator from this code.

The reference at `b91965b3011ed269cbc3a051b00c9b284aaa2e36` has now been inspected,
built and run locally: all four examples completed successfully in about 193 s.
Only an omitted standard-library include and an eight-thread limit were patched
in the ignored scratch checkout. [Reproduction, licence and source audit](ROCK-FRACTURING-REFERENCE.md).

Eight neutral-clay views of the supplied examples were also reviewed. Polyhedral
has the most useful unequal planes and deep joints; rhombohedral is second.
Equidimensional reads as masonry, tabular as stacked pancakes. Even the better
examples have pitted/bulbous surfaces and separated cube assemblies: **none is a
finished cliff or an approved scene asset**. [Actual comparisons](ROCK-FRACTURING-REFERENCE-REVIEW.md).

The source exports marching-cubes grid positions directly, not physical metres.
Adaptation must reconstruct positions from the actual extraction bounding box
and grid spacing, and convert its Y-up convention. The next concrete experiment
is one physical-scale exposure made from fracture-defined volumes embedded in
connected bedrock, with minimal smoothing/warping. Remove outer blocks to expose
unequal faces, retain backing, and protect complete shore/seam/temple-support
volumes including displacement margins. Do not tile or enlarge the sample cubes.

Keep source and bulky sample geometry under ignored `Saved`; retain MIT and
bundled notices if code is incorporated. Continue with one accepted outcrop,
then the measured 500 m sector, before extending around the crater. Original 3D
geometry remains authoritative for future mesh, falls and support queries;
new topology needs explicit material-mask correspondence rather than assuming
the previous vertex masks transfer automatically.

## Root integration and verification

- Added optional `-DCCliffPreviewDistance=1500` (valid range 50–4000 m), retaining
  the default camera, its target, angle and 60-degree FOV. It is available only
  inside the existing opt-in isolated preview. Editor and Game builds pass:
  `Saved/CliffWeatheredEditorBuild.log` and `CliffWeatheredGameBuild.log`.
- Actual offscreen native 1600x900 review `Saved/SceneReview/cliff10-baseline-far`
  uses the existing pass8 asset, **not one of the new rejected candidates**.
  The log establishes active backward-seek restoration at elapsed -4.99 and
  exit restoration at 181.03. The requested 265 screenshot occurs after the
  preview reactivates; it is not evidence of its negative-time appearance.
- The source-import audit now accepts original XYZ triangle archives and their
  numeric RGBA directly. It validates a rigid right-handed export frame instead
  of rebuilding a heightfield. Three regression tests preserve stacked XY
  surfaces, reordered reflected imports and masks, and reject invalid frames
  or numeric masks. The previous 52,536-vertex heightfield reference still
  prepares successfully. No new FBX/native import is claimed from this change.
- No performance approval: the native run is a still/restoration check, and the
  existing VSM marking warning and unrelated toolset Python startup errors remain.

No asset purchase, main merge, package or publication was performed.
