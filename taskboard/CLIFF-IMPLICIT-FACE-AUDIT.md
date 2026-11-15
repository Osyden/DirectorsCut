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

## Follow-up: actual curved-source clusters and assembly

Inputs: `Maelstrom-Saturday/Saved/ImplicitCliffFit/SourceSkin/points.json`, SHA256 `422e5eea7e53b0a4c5ddcd01f9431b7f67a531b0fe3a7deee70cc10a9d66ac8e`; `Crater-Workers/Flow-Seams/Saved/ImplicitFaceProof/SourceSkinPlanesFinal/Geometry/block_planes_m.json`, SHA256 `8ab1e5ec274324b98c7ded24043f3fc40c37b8e9575286b07ae34fc4ae9306b3`. Use Final metadata: the earlier byte-identical plane export's metadata still contained generic cube labels.

Independent membership and coordinate checks pass within numerical precision:

- 15,539 source points;269 clusters use14,814 unique original IDs, with725 omitted by the small-cluster cutoff. No point belongs to multiple retained clusters.
- Exported points match their original IDs within7.51e-11 m; copied depths match exactly; cluster bounding boxes match exactly.
- Original hull-plane normals have unit-length error below3.34e-16. Member halfspace and supporting-plane residuals are at most1.558e-6 m; the largest actual cluster point separation is8.99923 m against the9 m cap.
- The source-to-arena frame is a proper rigid transform, determinant1.0000000000000002. Independent crop/world reconstruction differs by at most2.28e-13 m. These metre units have not been replaced by grid indices or scaled to fit.
- Source points occupy0.15010–13.99888 m vertical depth below the original surface. Recomputed approximate normal depths match within2.03e-13 m. These depths use local source slopes and remain selection estimates, not signed distances or general contact queries.

Root's assembled original mesh is `Maelstrom-Saturday/Saved/ImplicitCliffFit/CurvedSkin/candidate.npz`, SHA256 `563381a63fbf58c1a695e285913a38ca28801211374350af8b0de8b0d132abdd`. It retains226 clusters, removes43 whole shallow clusters and uses rear anchors20 m vertically below the original top. The final Blender hull construction uses original member positions plus anchors; exported source hull planes therefore document provenance but are not an exact plane contract for the rear-extended final hulls.

The assembly has30,281 vertices and60,646 triangles, with zero boundary/nonmanifold/inconsistent-winding/degenerate/duplicate faces. It has **30 connected surface components**, not one: one positive-volume exterior of64,319.39 m³ and29 negative-oriented shells totaling10.9432 m³. All29 negative-shell centroids lie inside the exterior according to an independent ray-parity check. This supports enclosed boolean voids rather than floating solid blocks, while remaining sampled containment evidence, not an exhaustive shell-intersection proof.

All7,488 original4 m crop-guard vertices remain within10 micrometres. No vertex extends below the original24 m underside by more than10 micrometres. A further242,584 face-centre and edge-midpoint samples also remain within that underside; sampled protrusion above the old top reaches0.18636 m. The guard is outside actual shore and temple supports, so their live integration remains unproven. Detailed reports are retained as `Saved/WeatheredAudit/source_skin_audit.json`, `source_skin_components.json` and `source_skin_backing_samples.json` in the audit worker.

Actual `CurvedSkinReview/wall_fractured.png` and `oblique_fractured.png` show a better integrated curved exposure, but **still fail the final visual target**. The face reads as a dense mosaic of roughly similar-sized angular rubble separated by deep narrow crevices, with a straight exposure boundary against the untouched smooth shell. The tiny source-plane residual is not responsible for this appearance. Broad connected quiet faces, unequal fracture scales, selective recesses and geological massing are still needed. Do not promote topology validation into a photorealism claim or tile this proof around the crater.

## Larger-mass checkpoint

`Maelstrom-Saturday/Saved/ImplicitCliffFit/LargeMassClean/candidate.npz`, SHA256 `0ced0f0d9e3140ae98e058c037126984bc517a903bb2280fea52793775d9ab1f`, changes the source cluster span limit to18 m, retains155 of177 clusters, removes22 through two connected shallow-release regions, and uses7 m core removal with12 m vertical rear anchors. No physical scaling is applied.

Independent final check:26,426 vertices,52,872 triangles, **one connected closed surface**, positive volume66,199.34 m³, and zero boundary/nonmanifold/inconsistent-winding/degenerate/duplicate faces. All7,488 original4 m guard vertices remain unchanged within10 micrometres. This candidate has no extra disconnected cavity shells. Bounds remain72 ×40 ×89.133 m. Results are in ignored `Saved/WeatheredAudit/large_mass_audit.json`.

The numerical repair is narrow and verified against the preserved `LargeMass` precursor. It had one triangle of area9.949349e-11 m². The cleanup qualifies only a triangle below1e-10 m² whose shortest edge is at most0.001 m; it does not lower the quality gate. Exactly one0.000392924383 m edge was collapsed: its two endpoints were replaced by their midpoint, one vertex and two triangles removed, with an endpoint movement of approximately0.1965 mm. No other vertices changed at1 micrometre comparison precision. The cleaned minimum triangle area is6.486266e-8 m². This repair is a numerical sliver collapse, not geological smoothing or general decimation.

Actual `LargeMassReview/wall_fractured.png` and `oblique_fractured.png` show the pre-cleanup geometry; the single sub-millimetre repair does not justify claiming a separate visual improvement. These images have meaningfully larger quiet faces and fewer uniform rubble pieces than CurvedSkin. They are useful structural progress, but the rectangular transition into the untouched smooth shell, pristine planes and some deep narrow cuts still need art refinement. This is a saved offline candidate, not accepted final cliff art or an Unreal integration.

## Fin diagnosis before the transition revision

The LargeMassClean construction unconditionally added four points at12 m vertical depth to every retained hull. Independent measurements of the actual source memberships show134 of155 retained clusters already have at least four original members more than0.5 m inside the graded7 m core;145 have at least one. This is strong evidence that most hulls do not need a new long rear extension, though a robust contact criterion should also require spatial spread.

Two distinct causes coexist.27 retained original clouds have a smallest PCA span below1 m;26 have a smallest/largest span ratio below0.1. Separately,22 retained hulls acquire an anchor extension over6 m, up to11.1766 m.325 of620 fixed-depth anchor endpoints move upward, since the12 m target is shallower than their original positions. These counts diagnose the construction; they do not claim every thin source cluster is visible in the final mesh.

Examples in source crop coordinates:

| Cluster | Centre XYZ metres | Principal spans metres | Core contacts deeper than0.5 m | Maximum downward anchor extension |
| --- | --- | --- | --- | --- |
|84|(-2.91,2.96,0.41)|10.50 ×3.51 ×0.314|3|5.36 m|
|93|(2.05,5.38,4.83)|7.90 ×4.21 ×0.414|1|7.05 m|
|100|(5.10,1.25,-2.25)|7.24 ×4.11 ×0.416|0|6.92 m|
|167|(25.75,-11.35,-13.72)|9.10 ×1.21 ×0.886|1|11.18 m|

Thin cluster35 is a useful counterexample: every member is already buried inside the core. A blanket thinness filter would remove invisible valid geometry unnecessarily. The bounded correction should retain original naturally connected hulls, detach unsupported exposed thin fragments, and permit only short downward joins to the actual local core. It should never pull anchors upward or grow long artificial wedge faces merely to force connectivity. Measurements are retained in ignored `Saved/WeatheredAudit/large_mass_fins.json`; this diagnosis is not itself visual acceptance of the upcoming revision.
