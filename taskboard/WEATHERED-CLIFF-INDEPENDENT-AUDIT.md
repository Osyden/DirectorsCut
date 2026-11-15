# Independent weathered cliff audit — 15 November

Owner: cliff audit worker, `codex/cliff-weathered-audit`, based on `10b1fdc`.
Scope: original source mesh checks only. No Unreal, runtime, material, mesh
generation or GPU changes. Visual acceptance belongs to the matched clay review.

## First full sector

Input: `Flow-Seams/Saved/WeatheredCliff/first/sector_weathered.npz`, SHA-256
`a0d335212d57ba7c152e8874b1b9048754b69249500e7ad681136e6e48cbb270`.
Original eroded source: `Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`,
SHA-256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.

The new audit independently enumerates original valid ABD/ADC triangles,
reconstructs baseline vertex positions and erosion-mask channels using supplied
barycentric correspondences, checks protected vertices and complete open boundary,
and tests topology/winding and local projected orientation. It queries the actual
3D triangles for steep-face and temple support probes; it does not substitute
source-grid heights for candidate contact results.

- 1,025,013 vertices, 2,045,576 triangles. No degenerate, duplicate, nonmanifold or
  inconsistently wound faces. All 153,790 fixed vertices and 4,448 open-boundary
  vertices are bitwise unchanged from the refined original surface.
- Baseline source reconstruction error: `3.98e-12 m`. Maximum RGBA mask error:
  `2.98e-8`. Masks are transported source erosion channels, not new erosion.
- Maximum displacement: 1.505 m; median 0.029 m, P95 0.552 m, P99 0.835 m.
  Maximum edge stretch 1.614x, maximum actual edge 1.513 m. These statistics do
  not prove believable geological form.
- No local triangle flips: positive XY area ratio minimum `0.354447`; no
  downward triangles or greater-than-90-degree normal reversals relative to
  the original. P95 normal change 11.19 degrees; maximum 63.50 degrees.
- 64 short rays into real steep triangle faces all hit their target without an
  intervening occluder. Maximum distance error `3.18e-13 m`.
- Actual original-mesh C++ BVH: 256 independent brute-force oracle comparisons,
  zero mismatches, maximum error `1.60e-10 cm`. Vertical probes hit at most once;
  this is a single-valued surface and does not demonstrate overhangs.

## Supports and remaining integration limits

Actual exported temple guard sample locations retain exactly their baseline
heights: Actor_33, Actor_34 and Actor_48 each have 9/9 samples in this tile;
Actor_36 has only 3/9. All queried samples remain approximately 1 m above the
recorded footing bottom. This is not a full-footprint proof or approval of
Actor_36 outside the tile.

The baseline intentionally levels temple pads. Some sampled pad heights differ
from the old live polar ground by up to 4.275 m, despite unchanged weathering
guards. Do not describe these as unchanged legacy ground or infer that an
unmodified live landmark/terrain join is already integrated.

## Displacement direction metadata

The first archive stores the **effective upward-biased** direction in
`stable_source_normal`. Multiplying it by `normal_displacement_m` reconstructs
actual positions within `1.37e-7 m` (float32 field storage). Independently
rebuilding the source normal field, scaling XY by exactly 0.125, then normalizing
matches the stored direction within `2.98e-8`. The fold metrics record this
factor and rejected larger factors correctly.

However, `settings.displacement_direction` still says original-surface unit
normal. That description is inaccurate; the worker was notified to correct it
without overwriting the measured first archive. Preserved scalar amplitude does
not mean preserved displacement perpendicular to the original face. No visual
approval follows from passing these geometric checks.

## Reproduction

From the audit checkout, with installed NumPy/MSVC:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_weathered_mesh_audit.py
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/audit_weathered_cliff_mesh.py --mesh 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/WeatheredCliff/first/sector_weathered.npz' --source 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz' --output Saved/WeatheredAudit/sector_first.json
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_cliff_triangle_surface.py --mesh 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/WeatheredCliff/first/sector_weathered.npz' --output Saved/WeatheredAudit/BVH
```

Eight audit regressions pass: valid source, moved fixed guard, changed baseline
correspondence, permuted mask, relaxed guard, reversed face, vertical face ray,
and stacked-surface underside ray. The existing original-triangle BVH suite also
passes. Reports/provenance stay in ignored `Saved/WeatheredAudit`.

No exhaustive self-intersection test, final FBX/Nanite import validation, live
waterfall contact integration, or rendering-quality acceptance is claimed.

## Final faceted trial

Reran the same unchanged audit and original-mesh BVH harness on
`Flow-Seams/Saved/WeatheredCliff/faceted/sector_weathered.npz`, SHA-256
`157769fb9a8c8dedbf83f177034538532627aa5c510ba3f77e77bd95f40273d3`.
Reports are `Saved/WeatheredAudit/sector_faceted.json` and
`Saved/WeatheredAudit/BVH-Faceted`. Use the reproduction commands above with
those input/output paths; the source NPZ stays the same.

All checks pass again. Topology, boundary, guard, source correspondence, material
mask errors and temple sample coverage are unchanged. Projected area ratio
minimum is `0.353329`; no normal reversal or downward face. Maximum relief is
1.702 m, P95 0.643 m, P99 0.979 m. Maximum edge stretch increases to 2.416x;
P99 is 1.234x. Sixty-four steep-face rays still hit their expected surfaces.
The actual BVH has zero mismatches across 256 oracle rays, with maximum distance
error `1.31e-10 cm`. Vertical probes still hit at most once.

The final metadata correctly states that a global XY factor is applied to the
source normals before normalization. That factor is again 0.125; the effective
direction/scalar fields reconstruct positions within `1.01e-7 m`.

Viewed the actual matched close oblique pair and final deck / 1,500 m clay frames
under `Fitted-Waterfall/Saved/WeatheredReview/FacetedClose` and `FacetedFull`.
The final shape adds small breakup, but nearby still reads as soft, wrinkled
terrain with isolated pits rather than convincing fractured rock exposures.
The distant wall conveys scale, but that does not resolve the close form.
Do not expand this candidate around the crater based on the numerical pass.
The lower sawtooth boundary and rectangular side cuts are inherited diagnostic
domain boundaries, not new weathering spikes; their eventual joins to water and
neighboring geometry still require validation in the real scene.

No further shape iterations or tool extensions were made for this trial.
