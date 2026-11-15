# Displaced crater face: independent audit

15 November 2026. Read-only CPU verification of `Flow-Seams/Saved/HistoricalCraterCut/DisplacedFaceA`, using existing original-triangle checks. Candidate SHA256 `f9138107bece82bc6b47dfb87607fed90a5a2d57a8a428ba63da2955f5b1c848`. **The geometry/contact contract passes, but the actual visual result is rejected for native promotion.** No Unreal, root-source or game mutation occurred during this audit.

## Geometry and protected regions

The same-topology baseline and final candidate have 934,088 vertices / 1,865,732 triangles. Conforming refinement preserves source-triangle lineage and original baseline positions: independent barycentric reconstruction error is below 5.2e-10 m. Recorded source weights are within 1.3e-9 of the original triangles, consistent with numerical roundoff on small refined faces. Face tags match original lineage.

The candidate is one clean open manifold with 2,442 subdivided boundary edges. It has no duplicate/degenerate faces, unused vertices, inconsistent winding or nonmanifold edges. The larger boundary-edge count is subdivision of the unchanged perimeter, not additional holes.

All 31,265 natural-top vertices, 1,775 shared top/cut seam vertices and every open-boundary vertex are bitwise unchanged from the refined baseline. Independent distance checks against the original 200 shoreline-curve segments identify 40,322 vertices within the protected 2 m U/Z strip; all are bitwise fixed. This verifies the actual internal waterline strip, which is distinct from the buried outer edge.

Displacement is 0..2.621301 m along landward local V. Actual U and Z changes are below 1.7e-13 m. Every cut triangle retains positive signed U/Z area, with no orientation reversal. Thus this face remains a graph V(U,Z); an XY projection reversal alone would not imply a local fold. This is not a general arbitrary-mesh self-intersection certificate.

The displaced face is also sampled against the exact original parent ABD/ADC triangles after the recorded rigid placement. Moved vertices remain at least 0.746 m below parent terrain; moved-face centroids remain at least 0.675 m below it. No cut-face centroid protrudes above the parent. These are original-surface samples, not a smoothed surrogate; they do not constitute exhaustive continuous triangle-intersection testing.

## Physical scan scale and provenance

Source, original parent, linear height array and provenance hashes match the actual files. The `marble_cliff_01` scan repeats at 4.299351692 m in physical U/Z axes, matching the recorded source material dimensions to 2e-10 m. The .600000024 m scalar relief setting matches the upstream authored material value to 1.6e-10 m. That value is material authoring, not independent survey calibration.

The field is sampled as linear data without stretching its scalar range. Actual scan-only recession spans about .000008..0.523349 m. Two additional unequal gully recesses derive from measured low points of the original parent/cut seam; their source prominence, widths and limited lengths are recorded in the manifest. This preserves provenance, but does not establish that either the periodic detail or the selected macro recesses look natural.

## Actual production-water and local route check

Root independently queried all 934,088 vertices and 1,865,732 triangle centroids against current static `HeightAt(false)`, `WaterEdgeRadius` and the declared production full-steer envelope. Its report matches this candidate SHA and the previously verified position-function hash `b0349251bec80a6ffa3677afcb742dc73eddb456d63dc820a0a73c32f2b3a524`.

All 2,444,549 sampled dry points are landward of water coverage; the closest is 3.72 mm landward. Minimum dry steering-envelope margin is 35.0037 m, with 30.9459 m minimum over all samples including submerged rock. Root evidence: `Maelstrom-Saturday/Saved/CraterCutProbe/DisplacedFaceA/report.json`.

This is sampled static local clearance, not animated-wave contact, continuous collision, whole-arena navigation or temple-support approval. Old seam and foundation limitations from HeadlandA remain. The audit's own evidence is `Saved/WeatheredAudit/displaced_crater_face_a.json`.

## Actual visual judgement and next useful change

I viewed the angle-179 `DisplacedFaceAClay` wall and close-wall captures and `DisplacedFaceAMaterial` oblique under the review worker's `Saved/ImplicitCliffReview/HistoricalCraterCut`. The scan creates small relief, but its obvious repeat sits across the same broad planar curtain. The two shallow recesses at the right barely affect the overall rock mass. The upper natural terrain remains useful and the safe contact construction survives, but this does not yet read as a credible geological cliff.

The next useful change is broad exposed-face structure related to upper spurs and ravines, while retaining valid contact and source scale. Increasing scan amplitude or enlarging its physical repeat would not address that missing hierarchy. No native import, performance claim or full-scene acceptance follows this candidate.
