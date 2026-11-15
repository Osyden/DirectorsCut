# Radial-shear crop audit — 15 November

Independent audit worker; one bounded mapping proof, not a new live cliff pass.
Candidate: `Flow-Seams/Saved/WeatheredCliff/radial_shear/candidate.npz`, SHA-256
`efe9c799778c9da5925fe9b5c4ec97323f0890e61aa1e7a3f0572aeb6a185c58`.
Source remains the v3 eroded massif used by the previous weathering audit.

## Continuous mapping check

The implementation applies `T(U,V,Z) = (U, V + G(U,V)*F(U,Z), Z)`. Code review
confirms F uses only rotated U/Z coordinates and fixed noise offsets; it has no
V-dependent slope, hardness, erosion, normal or height-fade multiplier. Clamping
gives `|F| <= 2 m`. G is solely smoothstep of protected-point XY distance over
3..15 m, without a hard override. Euclidean distance is 1-Lipschitz; smoothstep
has maximum derivative `1.5/12 = 0.125 m^-1`.

Thus `det(DT) = 1 + F*dG/dV >= 0.75` where differentiable. Equivalently, at fixed
U/Z, the difference between mapped V coordinates is at least 0.75 times their
original separation, which also covers nondifferentiable nearest-point ties.
Source-corner guard flags must naturally fall in G=0: every point of a 2 m
source triangle is within its 2.829 m diagonal of a constrained corner. The
generator asserts this rather than imposing an extra discontinuous override.

These statements concern the ideal continuous mapping. Floating-point sampled
triangle chords need their own checks; a positive continuous determinant is not
an automatic discrete collision certificate.

## Actual discrete crop

- 19,481 vertices / 38,496 original triangles. Source reconstruction error
  `7.68e-13 m`; mask error <=`2.99e-8`. No degenerates, duplicate faces,
  nonmanifold edges, inconsistent winding or >90-degree relative normal flips.
- Maximum radial displacement 2 m; P95 1.220 m. Maximum edge stretch 1.319x and
  maximum actual edge 1.278 m. No unbounded edge spike was found by these checks.
- U/Z positions stay unchanged within `1.71e-13 m`. Every source and candidate
  triangle retains the same nonzero UZ orientation. The mesh is one connected
  disk (Euler characteristic 1), has one degree-two boundary loop, and its UZ
  boundary has no nonadjacent crossings. UZ triangle area change is <=`2.17e-13`.
  This crop therefore retains its discrete graph structure in UZ; the radial
  displacement changes the third coordinate rather than folding that parameter
  domain. This is a distinct check from a generic continuous determinant bound.
- 136 triangles reverse their XY projection. This is allowed here. Rays targeted
  at the XY centroids of all 136 hit multiple separated surfaces, up to seven,
  while the same baseline rays have at most one hit. This proves small actual
  overhangs in this crop. The coarse 64x64 vertical grid missed them; do not use
  that negative coarse result to conclude there are no overhangs.
- 64 short original-triangle normal probes hit correctly. The C++ BVH matches
  256 independent brute-force oracle rays, zero mismatches, max error `6.01e-11 cm`.

Example original-mesh vertical probe at arena XY metres
`(-1259.6670554756918, 1113.6167271342529)` hits Z=10.0111111, 9.8225981 and
9.4367529 m, with normal Z components approximately +0.387, -0.00198, +0.0995.
These are separate original triangle intersections, not sampled height layers.

## Boundary and visual limits

This crop contains **zero fixed vertices** and G=1 everywhere. It cannot provide
measured shoreline or temple-guard approval. The continuous guard construction
was checked in code; a future sector must still test actual fixed points.
Its arbitrary review-cut boundary moves by up to 1.781 m, so it is not a finished
seam tile. Explicit audit allowances permit crop-edge motion and XY reversals;
they do not relax constrained-source, topology, mask or finite-ray checks.
A new regression verifies these allowances still reject moved fixed points.

Viewed `Fitted-Waterfall/Saved/WeatheredReview/RadialShearClose/close_oblique_fractured.png`.
The increased relief is visible, but the large face still reads as soft terrain
with irregular dents rather than convincing fractured rock. The mapping proof
is valid; visual acceptance and expansion around the crater are not approved.
No Unreal, GPU, runtime or material edits were made by this audit worker.

## Reproduction

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_weathered_mesh_audit.py
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/audit_weathered_cliff_mesh.py --mesh 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/WeatheredCliff/radial_shear/candidate.npz' --source 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz' --output Saved/WeatheredAudit/radial_shear.json --allow-projected-reversal --allow-crop-boundary-motion
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_cliff_triangle_surface.py --mesh 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/WeatheredCliff/radial_shear/candidate.npz' --output Saved/WeatheredAudit/BVH-RadialShear
```

Nine audit regressions pass. Focused multi-hit and UZ topology measurements are
retained in `Saved/WeatheredAudit/radial_shear_focused.json`; its examples include
original triangle IDs and ordered hit heights/normals for inspection.
