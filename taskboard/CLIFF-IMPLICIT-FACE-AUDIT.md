# Implicit cliff face and bounded fit audit — 15 November

Owner: Codex audit worker, `codex/cliff-block-adapter-audit`. Read-only audit of generated originals; no game assets, Unreal jobs or runtime changes. The proof is **visually rejected as a final cliff**: its coherent planes are structural progress, but the fitted result looks like a rectangular slab applique on a smooth hillside.

## Evidence

- Adapter source: `Tools/run_implicit_cliff_reference.py` in the Flow-Seams worker. Exact upstream revision `b91965b3011ed269cbc3a051b00c9b284aaa2e36`.
- Sample: `Crater-Workers/Flow-Seams/Saved/ImplicitFaceProof/ConnectedShallow/Geometry/sample.obj`, SHA256 `92d4adb35d7cafaf0640a4cf1f0379b68ad3dee76a356a35f04946bd58be55c2`. Earlier thick ConnectedFace is excluded.
- Fitted proof: `Maelstrom-Saturday/Saved/ImplicitCliffFit/First/{baseline,candidate}.npz`, placement contract `placement.json`; reviewed actual `FirstReview/wall_fractured.png` and `oblique_fractured.png`.
- Source terrain SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.
- Independent numerical reports retained under this worker's ignored `Saved/WeatheredAudit/connected_shallow_and_fit_audit.json` and `backing_caps.json`. Existing `review_cliff_outcrop.py` topology routines were applied to the actual original triangles; no new generic audit framework.

## Adapter and sample

Physical conversion is consistent: restore the extraction-box origin and each grid-axis pitch, apply 1.5 metres per source unit, then `(x,y,z) -> (x,-z,y)`. This is a proper rotation, not a reflection; the output front is +Y. The second-smallest grid-integer residual across marching-cubes vertices is at most 3.66e-6 cells, consistent with two coordinates lying on extraction-grid edges. Physical normals were recomputed after the anisotropic grid conversion and agree independently within 5.25e-9; unit-length error is below 8.13e-13.

The actual sample is 30.300 × 8.983 × 30.300 metres, matching metadata exactly. It has 191,144 vertices and 382,284 triangles, one connected component, no boundary/nonmanifold/inconsistent-winding edges, and no duplicate triangles. Signed volume is positive, 7,213.63 m³.

There is one tiny triangle below the review tool's twice-area threshold of 1e-10 m²: triangle151149 is 4.3786e-11 m². None is exactly zero. The adapter's smaller cutoff therefore reports zero discarded degenerates; these are different thresholds, not contradictory data. Its 127 outward / 2 inward signed-field orientation samples establish majority orientation, not a proof that every face points outward. Do not repeat the metadata wording as an exhaustive field-orientation guarantee.

Verified code corrections matter: the upstream zero `Sphere::RandomSurface()` normal stub is bypassed by explicit finite unit-normal fracture discs; actual hull-point bounds replace the defective Matrix4/Plane convex-point bounds; the image loader and gradient warp are bypassed. This is an adapted finite block proof, not evidence that the public sample implements every feature of the research paper.

## Fitted solid and backing

The fit is a rigid transform at scale1. Its rotation determinant is 1.0000000000000002, with orthogonality error 2.23e-16. Actual crop dimensions remain 72 × 40 × 89.133 metres. The host backing is **24 metres vertically**, not 24 metres along the fitted slope normal.

The result has 128,973 vertices and 257,942 triangles, one connected component, zero boundary/nonmanifold/inconsistent-winding/degenerate/duplicate faces, and positive volume. All 7,488 original vertices within the 4 m crop guard remain at the same coordinates within 10 micrometres. This verifies the actual crop collar, not merely a few points on its edge.

The inserted rear plane is buried: its 24,335 vertices sit 6.377–16.260 m below the original surface, retaining at least 7.740 m vertical clearance to the host's underside. No rear-cap triangle survives on the final exterior.

**Other artificial backing is exposed:** 124 triangles, 1.980 m², lie on the common backing's flat front at sampleY=-1.5 m; 784 triangles, 4.904 m², lie on its left side at sampleX=-15.15 m. This is a real construction artifact contributing to the plaque silhouette. A connected union alone does not eliminate visible caps.

The crop has no protected temple/coast vertices. Its source validation places it at least36.50 m from the sampled shoreline and outside the temple margin. These results do not authorize replacing a full sector or certify current live temple footings. Checks cover edge topology, source-coordinate preservation and the identified cap planes; no exhaustive mesh self-intersection proof was performed.

## Next bounded construction

Use the measured curved cliff to define the region in which physical fracture clusters exist, with a connected inner rock core. Do not stretch cubes or reuse this rectangular plaque across the wall. Shared fracture families should continue across several clusters with finite terminations and unequal spacing.

Do not intersect every exposed block against the same smooth outer envelope: that makes the envelope the visible surface and recreates the clay cliff. Use the envelope primarily for cluster membership and exposure selection; remove selected whole near-surface clusters to expose their actual internal joint planes. Keep deeper clusters and the original guarded bedrock united. Surface-cluster selection needs coherent outcrops and erosion recesses, not independent random holes.

At the protected perimeter, keep new backing faces entirely inside retained rock. Test for exposed construction caps as well as unchanged guard coordinates. The present render already proves that import density, polygon count and normal smoothing are not the principal blocker. The next acceptance gate is believable connected geological massing at physical scale, before texture work or another game integration.
