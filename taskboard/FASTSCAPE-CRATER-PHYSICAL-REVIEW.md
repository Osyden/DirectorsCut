# Physical Fastscapelib crater proposals: rejected visual review

15 November, independent CPU review on `codex/implicit-exposure-review` after
merging root `763da86`. **Neither PhysicalA nor PhysicalB is accepted for native
preview or runtime integration.** No game geometry, materials or Unreal assets
changed. This pass rendered and reviewed the author's actual triangle surfaces.

## Inputs and honest comparison

Author artifacts are in `Crater-Workers/Flow-Seams/Saved/FastscapeCrater/`:

| Candidate | Actual candidate SHA256 | Method difference |
| --- | --- | --- |
| PhysicalA | `540778f443df0f7520ea621f43af7ed9cbe9c7d9f4b0c851f2cf888a501441e3` | Fastscapelib 0.3.0 stream-power erosion, 80 steps of 100 model years; ADI diffusion omitted because it violated interior fixed nodes. |
| PhysicalB | `99a401602894ef653a687db6c468da8f1b0ee412afa86788a0c760c50cfb23f5` | Same initial landform and stream-power settings, plus explicit masked finite-volume linear hillslope diffusion, D=0.05 m²/year, seven stable substeps per model step. |

The paired render control is the **previous actual eroded massif**, at
`Flow-Seams/Saved/ImplicitFaceProof/FullMassifContext/baseline.npz`, SHA256
`15583c3d1a9cd6ba6db5b2053db7386a816e1e3753c490824154d8d9487c6809`.
It is not the author's new fresh initial terrain. Each PhysicalA/B directory also
contains a different `baseline.npz`, which is that fresh initial terrain; do not
describe these review pairs as isolated solver before/after images.

Both proposals retain the actual source08 arena frame and metres. Their grid is
500 × 504 m at 2 m spacing; the visible valid surface spans 500 × 492 × 207.616 m.
There is no shrinking of the earlier 20 × 10 km upstream reference or vertical
exaggeration. Frame origin and all basis vectors match the previous source and
measured mean-water context exactly. The neutral water context is the mean
receiving surface, not animated water or a final shoreline render.

## Captures and findings

Actual captures are under this worker's ignored directory:
`Saved/ImplicitCliffReview/FastscapeAdaptation/PhysicalA/` and `PhysicalB/`.
Each contains paired `wall`, `oblique`, and `close_wall` images, named
`*_baseline.png` / `*_fractured.png`, plus `review.json`. The generic filename
`fractured` means candidate; these proposals are heightfields, not fracture meshes.

The camera manifest `Saved/ImplicitCliffReview/FastscapeAdaptation/cameras.json`
is unchanged from the earlier 500 m review. Both runs used existing CPU Blender
rendering with eight threads, 24 samples, neutral clay and angle-15 normals.
Original positions and triangles were retained; identical cameras and lighting
were used for A and B. Full wall and oblique are the decisive views.

- **A is a curtain:** the flat upper lid sits over many long, narrow, nearly
  parallel full-height grooves. It has the required relief, but little coherent
  branching, unequal promontory massing or rock hierarchy. The left recess is a
  conspicuous forced bay. The locked close view enlarges the smooth ribbons and
  grooves instead of revealing plausible rock.
- **B is a smooth bank:** diffusion removes the grooves but also removes almost
  all face hierarchy. Wall and oblique show a broad smooth dam-like face under
  the same lid, with the left recess surviving. It does not produce useful
  unequal masses or a convincing retreating escarpment. Its locked close view is
  almost uniform grey because the changed face fills/occludes the old detail
  camera; do not treat that image as independent detail or scale evidence.

Root, author and reviewer all reject both. No distant 1.5 km/3.2 km renders,
third morphology trial, FBX export or native integration were justified.

## What passed, and what remains unresolved

The existing mesh check reports each candidate as one valid open surface:
52,536 vertices, 104,078 triangles, 992 boundary edges, and no degenerate,
duplicate, winding or nonmanifold flags. This is not an exhaustive intersection
or art-quality test. The author's manifest records exact fixed coast/seam values
and zero error under the +1,000 m invalid-padding probe. For B, masked diffusion
has zero invalid edges and an accounted volume-ledger residual of about
4.1e-9 m³. Numerical correctness did not produce an acceptable landform.

**Temple support is explicitly unintegrated.** Existing placements were not moved
or preserved by the proposed terrain: B buries Actors 33/34 by roughly 11–14 m
and removes tens of metres beneath parts of Actors 36/48. These are rejected
offline art proposals, not drop-in replacements with working foundations.
The model years are not a calibrated geological age, and no physical PBR masks
or stream sediment transport were established by this review.

## Next representation experiment

Build and review a broader parent landform with real upstream contributing area,
unequal ridges, valleys and an irregular escarpment before extracting a 500 m
crater sector. Retain physical feature sizes rather than squeezing an entire
large reference landscape into the patch. The local narrow steep step and flat
lid constrained both trials: stream-power incision made drapes; diffusion
smoothed those drapes into a bank. More texture or blanket detail cannot replace
the missing macroshape.

Accept the next parent/crop only if neutral wall and oblique views show connected
drainage and unequal coherent rock masses without a continuous flat lid or
repetitive curtain. Then check the same geometry at 1.5 km and 3.2 km, and solve
coast seams and real temple support explicitly before native integration.
