# Structural cliff macro review, 15 November

Scope: independent CPU clay review of the 500 m source-8 sector. The reviewer
does not alter generator geometry, runtime, material, source frame or lighting.
The root owns any Unreal preview. No native integration is recommended merely
because these mesh checks pass.

## Reproducible comparison

The control is the previous eroded massif's original-triangle surface,
`Flow-Seams/Saved/ImplicitFaceProof/FullMassifContext/baseline.npz` (SHA256
`15583c3d1a9cd6ba6db5b2053db7386a816e1e3753c490824154d8d9487c6809`). This is
the same control as the earlier full-context review; it differs slightly from
the structural worker's supplied pre-erosion baseline. Both share the exact
sector frame. Images report the actual source hashes.

Use existing `Tools/review_cliff_outcrop.py`, explicit `--open-surface`,
`--normal-mode angle --normal-angle-deg 15 --samples 24`, CPU/eight threads.
The fixed manifest is this worker's ignored
`Saved/ImplicitCliffReview/StructuralMacro/cameras.json`. Wall and oblique
cameras match the previous massif review. The `far_crater` view is an isolated
physical-scale perspective at 1,500 m horizontal range, 35 mm focal length,
36 mm sensor, with a 100 m reference bar and 10 m posts. It is not an actual
ride-camera or combined water/atmosphere capture. No vertical exaggeration,
geometry smoothing, texture or displacement is added by the reviewer.

The isolated lower fringe is mostly a cropped submerged toe, per the separate
receiving-water audit. It is excluded from the macro-shape rejection and is
not grounds to change coast locks.

## Escarpment A: rejected

Inputs are under `Flow-Seams/Saved/Structural-Cliff/` and outputs under this
worker's `Saved/ImplicitCliffReview/StructuralMacro/`, in matching subfolders.
Each output has wall, oblique and far-crater baseline/candidate pairs.

`EscarpmentA/candidate.npz`, SHA256
`6ea33ec0f54ff08d6b19f0b472ff57c9591f1368eff36b155ee18b5852354248`, creates
huge smooth prism faces and straight dark trenches. Raster shoreline-coordinate
steps also cause full-face vertical corrugation. The coordinate-only correction
`EscarpmentAContinuous/candidate.npz`, SHA256
`a623901e35df45171ffccfff353df3f25739f65e266560ad93f0429994135fbc`, removes much
of that corrugation but leaves the structural problem: folded concrete-like
panels remain obvious even from 1.5 km. It is less natural than the old massif.

Both have 52,536 vertices / 104,078 triangles, one component, matching original
outer boundary edges, and pass the existing open-surface audit. The corrected
dimensions are 500 × 492 × 222.28 m. Those numerical passes do not change the
appearance rejection. Adding fine noise would not solve the dominant shape.
The single remaining structural trial should use unequal staggered masses and
bounded recesses, avoiding whole-height straight cuts or repeated shelves.

## Massed B: rejected; stop this approach for the current pass

Input `MassedB/candidate.npz`, SHA256
`6d8f275add9749e57af795aa91b98e54cd0be29a7fc4a6df4585997c34f53cfa`.
Matched outputs are `Saved/ImplicitCliffReview/StructuralMacro/MassedB/`, with
`wall_fractured.png`, `oblique_fractured.png` and `far_crater_fractured.png`
beside their `_baseline.png` controls. All three candidate images were viewed.

The hinterland has more varied relief and the deep whole-height cuts are
reduced. The exposed face nevertheless consists of giant clean triangular fins
on an almost vertical plane. Their repeated silhouette and shadow rhythm reads
as engineered retaining-wall supports even at 1.5 km. Changing sizes and cap
heights has not produced natural hierarchical rock masses. This is not more
geologically coherent than the previous massif, and requires neither a native
preview nor a sector rollout. The generation worker independently reached the
same rejection after viewing the actual images.

The mesh still passes the existing open-surface audit: 52,536 vertices /
104,078 triangles, unchanged 992 outer boundary edges, physical chart bounds
500 × 492 × 210.81 m. Numerical integrity is useful but distinct from visual
quality. The cropped submerged toe was excluded from this appearance judgment.

No third structural tuning iteration was run. Retain these as failed source
experiments. Future work needs a better mass/exposure representation with a
coherent geological hierarchy rather than additional noise on these prisms.
The earlier bounded original-3D fracture proof remains a separate experiment;
this review does not revoke its local geometry validation or approve it as a
solution to the whole massif. The reviewer changed no source geometry or UE
assets during this pass.
