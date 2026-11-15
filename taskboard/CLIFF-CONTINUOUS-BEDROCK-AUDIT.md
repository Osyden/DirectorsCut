# Continuous bedrock: independent audit

15 November 2026. Audit worker merged root `235a0fa`, then inspected the new `generate_jointed_bedrock_sector.py` and its actual original-triangle outputs. No Unreal, runtime, generator or mesh edits were made here. **Final topology passes; actual clay appearance remains rejected.**

## Preserving real support

The immutable source remains `Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.

Preserving fixed samples alone is insufficient: every original ABD/ADC triangle touching a protected corner must remain intact. This source requires 16,359 of 104,078 triangles and 9,324 distinct vertices to be frozen, rather than only its 8,032 fixed samples. That is 15.72% of triangles and 13.51% of surface area, leaving most of the face free. A 3 m fixed collar covers the source grid's 2.828 m horizontal triangle diagonal.

The new construction conservatively excludes each cutter's full physical XY bounding box plus an 8 m collar from the frozen-vertex union and invalid domain. Its backing follows the actual irregular boundary. The current frame has zero origin Z and vertical +Z, consistent with the source-height calculations. A future nonzero-origin-Z input would need an explicit height conversion or assertion.

These checks preserve the immutable source support, including its declared partial-footprint limitations. They do not recertify current live temple footings or authorize old contact queries on new exposed faces.

## Rejected first output

`Flow-Seams/Saved/ContinuousBedrock/FirstClean/candidate.npz`, SHA256 `b33ad44246c5b6b532f5eb900ae3baf009cea2c034b893fa40479bae4a64284b`, fails: 92 nonmanifold edges, 74 duplicate triangles, 11 added interior boundary edges, two winding defects and three components. The original protected triangles were preserved, but that does not make the result usable. Its 26 releases also looked like quarry pockets; no further repair of this shape was requested.

## Final numerical correction

The ten-release `CornerRelease` precursor had one zero-thickness flap: opposite-winding triangles 101097 `[52042,50974,76]` and 101297 `[76,50974,52042]`, each area 0.331819 m². Their shared base edge had four incident faces. Canceling both faces and orphan vertex 52042 fixes that defect; removing only one face would not. This is cancellation of an exact opposite pair, not a tiny-area filter or geological refinement.

Final artifacts under `Flow-Seams/Saved/ContinuousBedrock/CornerReleaseClean`:

- `candidate.npz`: SHA256 `bd07b26867e8200766a2a58331f7c0988ad5f06d3e54204324ae2d0b9f024a5c`; 52,206 vertices, 103,418 triangles, one valid open manifold, 992 original boundary edges exactly preserved.
- `closed_diagnostic.npz`: SHA256 `ef8c02a90dc6bc597604a5697ed8d45e79ce03eed4a4774b0bea5c4e5a826d51`; 104,742 vertices, 209,480 triangles, one consistently oriented positive closed manifold, volume 33,221,942.7738 m³ including the artificial 160 m backing.

Both have zero nonmanifold, inconsistent-winding, degenerate or duplicate faces. All 16,359 protected source triangles match exact coordinate triples; all 9,324 corresponding source vertices are bitwise unchanged. Frame arrays and the 500 × 492 × 230.627 m exposed bounds remain unchanged. Against the precursor, every remaining vertex is exact, no triangle sets were added, only the opposite flap pair was removed, and all ten release-cell descriptions are identical.

This is a topology/support check, not exhaustive triangle self-intersection certification. The closed diagnostic's backing is an authoring closure, not game terrain.

## Visual decision

The actual `Fitted-Waterfall/Saved/ImplicitCliffReview/ConnectedBedrock/CornerRelease/oblique_fractured.png` still shows flat-floored polygonal mouths in a broadly unchanged soft massif. Fewer cuts and clean topology do not supply the missing connected cliff-scale structure. The two-face cancellation does not change that appearance. Do not export this as accepted final geology or trigger a native review solely because the numerical checks pass.

Existing audit functions were reused. Detailed ignored results remain in `Cliff-Contact/Saved/WeatheredAudit/continuous_bedrock_first_clean.json`, `continuous_bedrock_corner_release.json` and `continuous_bedrock_corner_release_clean.json`. No new audit framework was created.
