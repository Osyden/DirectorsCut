# Regional cliff exposure: independent audit

15 November 2026. Read-only audit by Codex in `Crater-Workers/Cliff-Contact`; no Unreal jobs or runtime edits. **The final exterior passes the bounded geometry checks. The actual clay scene remains visually rejected: an isolated rubble scar inside the soft original cliff, not finished regional geology.**

## Source and physical scale

The immutable input is `Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`. The selected original-grid crop is row 95, column 129, 160 × 96 metres, with a 4 m fixed guard. Its heights span -70.398 to 35.214 m, median radial drop 69.327 m. It contains no invalid or protected samples; nearest fixed vertex is 24.331 m away, nearest temple-pad vertex 36 m, and measured coast distance is at least 30.265 m. This preserves the declared source constraints; it does not recertify every live temple footing.

The source-skin JSON contains 21,516 physical points with a 6 m inset and 0.1503–13.99998 m vertical depth. Independent frame reconstruction agrees within 2.28e-13 m; the rotation is orthonormal with determinant one. No point pair violates the 1.4 m spacing floor. Depth attributes reproduce within 3.18e-13 m and remain approximate source-normal depths, not general signed distances.

The regional plane export uses 456 clusters containing 19,428 unique original point IDs; 2,088 points are omitted by the small-cluster cutoff. Member positions match within 7.51e-11 m, depths exactly, and own-hull halfspaces within 3.68 micrometres. Actual cluster pair spans are 4.81–17.9979 m; there is no physical upscaling. The 160 finite fracture discs have radii 10.0017–23.9047 m. The retained core must cover regions omitted from the point clusters.

## Cavity defect and explicit exterior extraction

The original closed regional candidate is `Flow-Seams/Saved/RegionalCliff/JointExposure/candidate.npz`, SHA256 `7a310f18decbc4a54d4e4916a667ed9713bf42f51aa7f3e06aba353784960975`. It has 126,527 vertices and 253,086 triangles, with no open/nonmanifold/inconsistent-winding/degenerate/duplicate faces, but **three connected shells**:

- Main positive shell: 253,020 triangles, volume 351,517.6016 m³.
- Internal negative shell: 30 triangles, volume -0.04947495 m³, crop-local centre (24.7152, 24.2271, -0.3131) m.
- Internal negative shell: 36 triangles, volume -0.02163142 m³, centre (-24.6693, 3.55245, -15.6518) m.

All 37 cavity vertices and 66 cavity face centres tested inside the main triangle shell by ray parity in two oblique directions. This is sampled containment evidence, not exhaustive mesh-intersection certification. The exact IDs and source hash are retained in the audit worker's ignored `Saved/WeatheredAudit/regional_joint_exposure_cavities.json`.

The final `visible_candidate_exterior.npz` has SHA256 `e4f9d41f09851035ecce30f6afb18107436df32786741b76bb80ef8619798764`: 64,537 vertices, 128,092 triangles, **one valid open manifold**, and all 1,024 original boundary edges preserved. It removes exactly the 66 audited inward-shell triangles. Every remaining vertex and triangle is unchanged; no faces were added and the frame is exact. The original closed mesh and first visible export remain immutable. Omitting these interior cavity faces is not an artistic improvement.

All 17,856 original 4 m guard vertices remain within 10 micrometres. A further 1,012,344 face-centre/edge-midpoint probes show no penetration below the original 24 m backing beyond that tolerance. Hull faces bridge the former source surface by up to 2.194 m at sampled locations, so old interior contact queries cannot be reused without integration.

## Full-context join and visual limit

The first root `Saved/RegionalCliff/FullContext/context.npz` still references the initial visible export and inherits its two cavity shells. Its splice itself passes: 48,823 outside source positions and masks remain bitwise exact; patch triangles and interior positions are exact; all 992 external boundary edges remain; the 1,024 seam vertices snap by at most 3.568 micrometres. Outside split faces remain coplanar within 1.36e-13 m, with area-sum error below 2.25e-13 m². Root will rebuild this context from the final exterior; that rebuilt artifact was not audited here.

The actual `Fitted-Waterfall/Saved/ImplicitCliffReview/RegionalExposure/FullContextWithWater/wall_fractured.png` shows a small, sharply bounded jigsaw-like rubble exposure inside a much larger soft wall. The physical region is larger, but the result still lacks coherent cliff-scale joint families and continuous geological massing. Do not promote a numerical pass into final visual acceptance, tile this scar around the crater, or claim the scene is ready for integration.

Detailed ignored reports: `Saved/WeatheredAudit/regional_joint_exposure.json`, `regional_joint_exposure_cavities.json`, `regional_joint_exterior_final.json`, and `regional_full_context.json`. Existing original-triangle audit functions were reused; no audit framework or runtime code was added.
