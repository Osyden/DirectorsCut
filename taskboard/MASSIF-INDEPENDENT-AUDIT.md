# Independent massif constraint and geology audit

Owner: Codex cliff audit worker, branch `codex/cliff-massif-audit` from `f06d420`.
Only the new audit tool, focused tests and this note are changed. The massif
generator and erosion result were produced by the separate Flow-Seams worker.
No generator/runtime edits, Unreal jobs, rendering jobs or erosion reruns here.

## Result

The final measured-boundary **v3 eroded candidate passes the numerical audit**.
This is not runtime integration, full temple coverage or final visual approval.

- Same 253×251 grid at 2 m: 52,536 physical vertices and 251 outlets.
- Measured coast and seam heights are bitwise retained, including outlet heights.
- All 18,999 fixed samples (including 10,967 padding samples) match the explicit
  float32 targets. There are 44,504 active samples.
- Frame reconstruction error is 4.55e-13 m; finite values, bedrock, fixed
  erodibility and loose-sediment restrictions pass.
- Independently computed exact Euclidean coast distances match the saved
  float32 distances within 1.53e-5 m. Every physical sample within 6 m is fixed.
- Actor_33, Actor_34 and Actor_48 are fully covered on the actual rectangular
  a–d triangles, with maximum support-target errors below 1.6e-6 m. Their ground
  stays approximately 1 m above the existing buried footing bottoms.
- Actor_36 overlaps only 1,024.43 of its 3,331.65 m² footprint (30.7%). The covered
  part has no holes and stays above its footing, but differs from the requested
  flat target by up to 0.383 m where the immutable seam wins. **Full support is
  not established**; the rest needs an adjoining surface/provider.

The audit clips each entire convex footprint against source triangles, including
missing triangles. It checks height extrema at the resulting polygon vertices;
valid corners or a protected-mask count cannot conceal an interior hole or bump.

## Problems found and corrected during the pass

1. Three seam samples could move by less than 5 cm because a temple conflict
   tolerance was also used to decide whether a seam could be overwritten. The
   generator now gives seam/coast priority regardless of the height difference.
2. Floating-point distances just above exactly 6 m excluded some intended coast
   samples. The generator now applies a small physical tolerance. Both issues
   have focused regression fixtures in the independent audit tests.
3. The clean constraint seed is not an authoritative copy of the live boundary.
   Its reconstructed edge geometry and optional preview catchment cuts can
   differ substantially from the measured source. The final generator therefore
   accepts the fresh measured NPZ separately for immutable boundary targets.

I initially observed a 2.092 m old-clean/current-ground difference at Actor_36.
The fresh source comparison corrected the attribution: the **new clean seed has
the same error**, while the fresh measured grid is only 0.0236 m above that live
sample. Clean polar resampling, rather than simply historical source drift,
caused the large discrepancy. Never call that difference harmless or preserved
live support. The fresh measured grid's residuals at the other sampled ruins
are at most about 1.3 cm; its rectangular resampling still is not an exact seam
with the original polar triangles.

## Anchor export review

Read-only review of root's `BuildLandmarks` export found the coordinate contract
consistent: arena-relative placement plus the same yaw-rotated pre-placement
bounds used by the existing nine-point support checks, followed by cm→m
conversion. The support target is `Ground`, and `footing_bottom_m` is `Ground−1m`.
Each exported ground sample queries the current rendered polar BC triangles.
Fifty groups were exported; four intersect this tile. Source geometry SHA1 is
`87BB77D76A90BAEE8DD8515CC4DB7E92F4C49F38`.

## Actual visual review

I viewed the CPU clay files `Massif8-CurrentFlat/oblique_before.png`,
`Massif8-CurrentAngle35/deck_before.png`, and final
`Massif8-MeasuredV2Flat/{deck,oblique}_before.png` under the Fitted-Waterfall
worker's `Saved/CliffLandform` folder. These are authoring views, not gameplay.

There is a useful structural gain: unequal connected ridges, recesses and
setbacks continue into the hinterland, replacing the repeated full-height slots
and smooth cap of the rejected clean-wall seed. The corrected boundary does not
erase that gain. However, some masses still look like soft terrain fins; the
flat-shaded 2 m triangles and sawtooth tile termination remain conspicuous.
Real jointed rock exposures and a proper shoreline cut are still needed.

Temple skyline readability is not proved by support. Terrain peaks reach
61.08 m while the three fully covered pads sit at 15.64–33.01 m. Actual ruin
heights and silhouettes must be shown against the sky in the next composition
review. I audited the eroded v3 result numerically; these visual observations
refer to the seed views and do not pretend to assess unseen erosion renders.

## Reproduction and evidence

```powershell
& E:/Utilities/Blender/5.3/python/bin/python.exe -B Tools/test_massif_audit.py

& E:/Utilities/Blender/5.3/python/bin/python.exe -B Tools/audit_massif_cliff_seed.py `
  --baseline E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/clean_constraint_seed.npz `
  --boundary-source E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz `
  --candidate E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz `
  --anchors E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffSupportAnchors.json `
  --output Saved/MassifAudit/measured_boundary_massif_v3_open256_audit.json
```

Seven focused tests pass: rotated negative-coordinate frames/area accounting,
interior holes despite valid footprint corners, partial coverage, internal
triangle extrema, winding/convexity, immutable seam priority and exact coast
band protection. The unchanged thirteen erosion-kernel tests were not rerun.

The full report includes every footprint, per-sample current-ground comparison,
provenance paths and SHA256 hashes. Final candidate SHA256:
`1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.

Measured boundary NPZ SHA256:
`8564969f5447949c72c598a9640fd5d86a20a99744bd0080c492711f38748200`.

Actual anchor JSON SHA256:
`2e89c7506e68390eea305f5cb937261dde2376bd473cb9740c4068d7534772b5`.
