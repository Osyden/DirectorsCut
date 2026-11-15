# Historical parent crater cut: independent audit

15 November 2026. Read-only CPU audit after merging root `8b84641`. Existing original-triangle helpers and source extraction only; no new audit framework, Unreal job or runtime modification. **HeadlandA passes the bounded construction/contact gate, but its broad smooth excavation face is not accepted rock art.**

## Actual source geometry and placement

Artifacts: `Flow-Seams/Saved/HistoricalCraterCut/HeadlandA`. Visible candidate SHA256 `fd6933153a11ad39b9010d956cb3f90e54931f5f2c032959fa0892417f72d6ff`; closed authoring bedrock `c2bb420e71859c9c6c224ee103fb52dbc9e65ce15a39bdf32c0e7ce727a2577a`. Source-parent, measured-coast, finite-water-mesh and water-query hashes match their actual files. The bundled generator matches its recorded hash.

The selected historical parent is translated exactly by local V=-600 m and world Z=-55 m, without rotation, rescale, bending or a new erosion pass. Its original source rows 300..510 and columns 20..220 place a 400 m-wide region in the current source08 frame. The visible footprint spans 400 x 378.861 m with 204.645 m height range; top elevation is +45.230 m.

Independent barycentric comparison against each recorded original parent triangle confirms natural top positions within 8.8e-13 m of the rigid placement. All recorded source weights lie inside their original triangles within floating-point tolerance. Excavation lowers the parent by up to 173.404 m and never raises it. No transformed source surface is reconstructed by projecting final vertices back into a replacement heightfield.

## Actual visible and closed meshes

The visible surface has 33,647 vertices and 66,267 triangles: one clean open manifold, 1,025 boundary edges, no degenerates, duplicate faces, inconsistent winding, unused vertices or nonmanifold edges. All projected triangle areas stay positive, consistent with this particular non-overhanging construction.

The closed authoring bedrock has 67,294 vertices and 134,584 triangles, one consistently wound positive closed solid, with no boundary or topology defects. Its artificial bottom and crop-side caps are explicitly tagged. The visible mesh contains only natural-top and excavation-face tags; its vertices and faces match the authoring solid's top exactly, with every cap excluded. The authoring solid's volume is not a claim about observed geology or visible geometry.

## Receiving-water contact

The 70-degree excavation is anchored to the jointly interpolated clearance-zero shoreline and mean-water height from the existing 2 m actual-source query grid. Its lower boundary lies 4 m waterward for submerged overlap; this is not a claim that dry rock may occupy that strip.

Independent queries of the final candidate triangles at all 201 declared shore controls match the sampled-water heights within 3.9e-13 m. The lower overlap matches its declared cut surface within 5.2e-13 m. Stored finite-water triangle indices and barycentric weights reconstruct the lower positions within 2.5e-13 m horizontally and exact stored water height; they lie inside actual finite receiving-water triangles. Actual overlap submergence is 11.130-11.287 m. The rigid parent's top stays 23.186-166.101 m above measured shore water before cutting, so this chosen section does not hide a submerged natural ravine.

I also verified that the CurrentSource08 query export's position-function hash exactly matches current root `8b84641` and its original `3bcdb2f` geometry source: `b0349251bec80a6ffa3677afcb742dc73eddb456d63dc820a0a73c32f2b3a524`.

Root separately evaluated every candidate vertex and triangle centroid against current `HeightAt(false)`, `WaterEdgeRadius` and the production full-steer radial envelope. Its report matches the candidate SHA and covers 33,647 vertices plus 66,267 centroids, including 64,517 dry samples. Every sampled dry point is landward of actual water coverage; the closest is 4.60 mm landward. Minimum dry steering-envelope margin is 35.0046 m; minimum over all samples, including submerged rock, is 30.9462 m.

Those fresh-query results are retained at `Maelstrom-Saturday/Saved/CraterCutProbe/HeadlandA/report.json`. They are static, sampled, local checks. They do not prove continuous triangle/solid collision clearance, animated water contact, runtime waterfall attachment or full-arena route safety. The independent finite-mesh audit is `Saved/WeatheredAudit/historical_crater_cut_a.json` in the audit worker.

## Visual and integration limits

I viewed actual wall and oblique captures under the review worker's `Saved/ImplicitCliffReview/HistoricalCraterCut/HeadlandA`, using continuous angle-179 normals. The connected upper ravines survive the rigid placement, while the new 70-degree front is a broad smooth curtain. This judgement concerns the actual cut surface; it does not repeat the earlier angle-15 normal-splitting diagnosis. The contact construction is usable for further authoring, but exposed rock structure remains absent.

Old arbitrary sector seams and temple foundations are not preserved or certified. Neighbours must eventually share an accepted authoritative surface; temple support and placement must be explicitly designed and checked. The historical parent remains unconverged. No native import, normal-game terrain replacement, water/sky/boss edit, main merge or package is approved by this audit.
