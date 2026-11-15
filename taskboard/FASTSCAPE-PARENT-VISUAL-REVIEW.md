# Wider parent landform: actual clay review, 15 November

Independent CPU review after merging root `e39928c` on
`codex/implicit-exposure-review`. **ParentA and ParentB are rejected for native
promotion.** They develop two connected unequal ridges, but neither supplies
convincing branching headlands, low irregular rock platforms or cliff massing.
No runtime terrain, Unreal assets, clouds, water or gameplay changed.

## Exact geometry and comparison

Author artifacts: `Crater-Workers/Flow-Seams/Saved/FastscapeParent/ParentA/` and
`ParentB/`, each with `parent/` and `crop/` baseline/candidate NPZ pairs and a
manifest. Actual candidate SHA256 values:

| Version | Whole parent | 500 m crop |
| --- | --- | --- |
| A | `cd763f05e964f6cd476f84bf11afc33ba94b337ed98e70d09138dc8e64240492` | `c4586b8da370377bf79d8cf1bcf2bb627381870530a38ba6545519178b266b59` |
| B | `d885c8135999ed1a3437c9542143d1abaa45b7ab8a26163131eac4eff1b76941` | `bb9dcb6c7e9715911088df32ae630e17bbb8f9392d4c372e676c0fe69ca3d940` |

The parent sampling domain is 500 × 1,100 m at 2 m spacing; valid land spans
500 × 1,088 m. The exact source-domain crop spans 500 × 492 m of valid land.
Both use original absolute arena metres and the original source08 frame.
Independent NumPy checks confirm the origins and all basis vectors match the
old source and measured mean-water context bitwise. No terrain scaling,
exaggeration or projection was applied by the renderer.

Whole-parent pairs compare fresh initial terrain with the evolved candidate.
Crop pairs instead compare the **previous actual eroded massif** in
`Flow-Seams/Saved/ImplicitFaceProof/FullMassifContext/baseline.npz` with the new
candidate. That control has SHA256
`15583c3d1a9cd6ba6db5b2053db7386a816e1e3753c490824154d8d9487c6809`.
Do not describe the crop pairs as an isolated solver before/after comparison.

## Physical method difference

A starts from low terrain, with no imposed tall step, plateau lid or reused
interior. Two connected, unequal broad divide forcings uplift the domain over
10,000 model years (100 × 100 years), at 0.002–0.012 m/year, opposed by
stream-power incision and masked linear diffusion D=0.005 m²/year. Its router
uses Single/MST/Multi1.1. Only the measured coast plus a triangle collar are
fixed; side/back boundaries are closed to flux but may move vertically.

B retains initial geometry, coast, forcing layout and total integrated uplift.
It simultaneously changes to **80,000 model years** at one-eighth uplift rate
(400 × 200 years), and **Single/MST** routing without Multi1.1. These are two
changes, so the image difference cannot establish either one's isolated effect.
Neither run is a calibrated geological history.

A's maximum elevation is 38.073 m, with 207.632 m total relief. B lowers the
maximum to 27.174 m and relief to 196.733 m. Parent median elevation falls from
−42.716 m to −52.521 m. This real loss of height remains visible; no later scale
or camera adjustment restores it.

## Actual captures and decision

All images are in this worker's ignored
`Saved/ImplicitCliffReview/FastscapeParent/`:

- `ParentAWhole/WHOLE PARENT 500x1100m oblique_fractured.png`
- `ParentAWholePlan/WHOLE PARENT 500x1100m plan_fractured.png`
- `ParentACrop/wall_fractured.png` and `oblique_fractured.png`
- `ParentBWhole/WHOLE PARENT 500x1100m oblique_fractured.png` and
  `WHOLE PARENT 500x1100m plan_fractured.png`
- `ParentBCrop/wall_fractured.png` and `oblique_fractured.png`

Matching `_baseline.png` images and `review.json` retain their exact inputs.
The generic filename `fractured` means candidate; no fracture was generated.
Whole oblique uses a fixed 1,750 m orthographic width; the final whole plan uses
2,100 m for both A and B. The initial A plan at 1,750 m left too little label
margin and is superseded by `ParentAWholePlan`. Crop wall/oblique cameras are
unchanged from all previous 500 m comparisons. Only the crop adds measured
mean water, without waves. Existing CPU Blender rendering used eight threads,
24 samples, neutral clay, angle-15 normals and unchanged original triangles.

**A:** the whole landform does contain connected unequal ridges and a central
valley. However, broad smooth forcing mounds dominate the silhouette, and many
nearparallel narrow rills run down their sides. The cropped coast presents a
grooved vertical curtain below rounded hill peaks. The parent is connected,
but the required branching mass hierarchy and plausible rocky headlands are
not visible.

**B:** larger incisions concentrate near the central valley and fewer rills
survive elsewhere. The same smooth domes remain, with nearly parallel ribs or
terraces across one face. Neither whole nor cropped views reveal a developed
branching headland network. Lowering the domes has not turned their surfaces
into the irregular low rim composition needed to frame temples against sky.

Reviewer and author reject both; root rejected A and received B's matched
captures and rejection. No close/far follow-up, third sweep, FBX export or
native integration was justified by these primary images.

## Technical limits, separate from appearance

The existing reviewer reports each parent as one valid open surface with
127,334 vertices and 253,078 triangles; each crop has 52,536 vertices and
104,078 triangles. No invalid edge, winding, duplicate or degenerate flags were
reported. Generic boundary preservation is false, as expected: moving side/back
terrain was intentionally released rather than fitted to old arbitrary seams.
This is not a watertight solid, an exhaustive intersection check or a support
certification.

Old outer seams and existing temple foundations remain unfit; no live actors
were moved. Additional inland rows are authored proposal data, not measured
neighbouring terrain. The parent's process topology is a useful distinction
from a clipped wall, but its actual forcing shape still dominates. Any next
experiment must demonstrate a genuinely different primary landform in clay
before surface fracture or texture work, rather than another generic
erosion-strength sweep on these two mounds.

## Separate historical-datum benchmark: useful method proof, not arena approval

After those two rejections, root authorized one different Saved-only diagnostic:
`Flow-Seams/Saved/FastscapeNaturalBenchmark/UniformHistoricalA/`. It uses the
same physical frame and 2 m grid, but a deliberately hypothetical flat coastal
datum at −150 m, uniform uplift and no Gaussian divide forcing. Modern vortex
coast, route, outer seams and temple supports are explicitly **not preserved**.
This is not a third A/B strength sweep or a terrain replacement.

Actual hashes: parent
`28c3a39604f1a3f043c618e9443816c475c2a2c33d365c774e3874543f838af4`;
crop `3f241a59e09caf599130956afc079f7f648bc449baa5d4479d7baaa2d7114c95`.
The 250,000-model-year run uses uniform uplift 0.001 m/year, K=0.0002,
D=0.001 m²/year, Single/MST routing and 500 steps of 500 years. It is explicitly
**not converged**: the final checkpoint's maximum height-change rate is 1.272
times the uplift rate, above its stated stopping criterion. Parent relief is
250.264 m (−150 to +100.264 m); the actual 500 m crop has 220.242 m relief.
Neither geometry nor camera was rescaled to resemble a finished crater.

Existing neutral CPU clay captures, in the same parent and crop cameras:

- `Saved/ImplicitCliffReview/FastscapeParent/UniformHistoricalAWhole/WHOLE PARENT 500x1100m oblique_fractured.png`
- The same directory's `WHOLE PARENT 500x1100m plan_fractured.png`.
- `Saved/ImplicitCliffReview/FastscapeParent/UniformHistoricalACrop/wall_fractured.png`
  and `oblique_fractured.png`.

These pairs use the benchmark's own fresh initial surface as baseline, including
the crop; unlike the A/B crop review they do not use the old eroded massif.
No modern mean-water context was added. Both surfaces pass the existing open
manifold check; maximum triangle edges are 11.15 m (parent) and 9.25 m (crop).

The whole views now show **connected branching valleys and interlocking unequal
spurs**, a substantial change from the smooth two-mound forcing topology.
That hierarchy survives the actual-scale crop. It is useful evidence for the
alternative parent-landform construction. However, crop wall/oblique expose
conspicuous zigzag/terraced channel margins, sharp repeated planar faces and
a pointed skyline. These are not finished rock exposures or the low rim
composition needed for temples. No native integration or art approval follows.

This is also **not a causal isolation of coastline effects**: historical datum,
forcing, duration, K and diffusion all differ from the rejected runs. The
result does not prove that the modern sloping coast alone caused the earlier
failure. It supplies a more promising natural-drainage starting point whose
physical cliff extraction, surface geology, current coast and foundations
would still require explicit work and visual review.
