# Connected massif seed with semantic constraints

Owner: Codex cliff worker, `codex/cliff-massif-anchors`, based on `f06d420`.
Only the new generator, its CPU tests and this handoff belong to this pass.
No Unreal/GPU/runtime/material/erosion-kernel changes.

## Result and remaining visual issue

The new authored mountain field continues through the cliff face and hinterland.
It removes the previous broad cap/apron locks and the radial `4*q*(1-q)` relief
window. Actual locked CPU clay reviews show a varied skyline, saddles and unequal
buttresses reaching the toe. Broad smooth inclined faces remain; this is a better
macro authoring basis, **not finished rock or a runtime integration**. The 2m grid
still shows sample triangles in flat shading. Real joint cuts/fracture and better
support shoulders are later work, not claims made by an erosion mask.

New physical field: Blender's installed `hetero_terrain` at 220/110m, bounded 35m
relief, plus `ridged_multi_fractal` at 96/48/24/12m, bounded 22m relief. A 22m domain
warp at 240m bends the connected network. These are authored mountain scales, not
a calibrated geological model. A screened 2D Laplace correction (60m screening
length, 592 red/black SOR iterations in the final artifact) fits actual constraints
while tending back to this mountain field elsewhere. No radial ramp or forced
monotonic columns are used.

Hard constraints are physical coast within 6m Euclidean XY distance of explicit
outlet vertices, 12m tile seams, invalid padding, and authoritative temple pads.
Coast/edge heights are bitwise copied from the **measured** grid. Temple pads use
support height inside their convex footprint plus supplied 10m margin and one
2m-cell diagonal, ensuring covered triangle vertices are fixed. Global screening
handles shoulders; per-anchor `blend_m` is recorded but does not set a separate
local falloff in this first implementation.

Fresh clean input froze 65.5056% of physical land; final measured-boundary candidate
fixes 15.2886%, releasing27,354 formerly fixed cells. Physical grid/frame/domain
remain 253x251 at2m over 500x504m, 52,536 valid samples and 251 shoreline outlets.
Three complete temple footprints pass independent exact triangle support checks
within 1.6e-6m. Actor 36 is only 30.7% inside this tile; the seam takes precedence at
its conflict and **full support is not approved**. The generator reports partial
coverage/conflicts instead of silently moving the boundary.

## Why the measured boundary source is required

The clean constraint generator itself altered edge heights: at this tile's 12m
seams it differed from its fresh measured input by up to77.30m, and the narrow
coast by3.80m. Thus a clean seed's `protected_mask` cannot certify current surface
compatibility. `--boundary-source` now validates exact XY/frame/valid-domain
identity and supplies actual immutable target heights. The clean input supplies
the frame/outlet definitions and regional elevation datum; its wall interior and
blanket locks are discarded. Omitting this option is explicitly labelled in
metadata and is unsuitable for a current-boundary integration claim.

The first historical/current-clean candidates and measuredv1 are retained only
as superseded diagnostics. Measuredv2 fixed strict seam precedence (even below
0.05m mismatch) and a 1e-6m physical comparison tolerance at the 6m coast. Final v3
has identical numeric arrays to audited v2 and adds complete source provenance.

## Reproduce the final candidate

Run from the worker/project root. Inputs and all output assets remain ignored.
The generator refuses to overwrite an existing output. Blender 5.3.0,
hash `02f0899d78e3`, was used for CPU noise evaluation only.

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 `
  --python Tools/generate_massif_cliff_seed.py -- `
  --input E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/clean_constraint_seed.npz `
  --boundary-source E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz `
  --anchors E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffSupportAnchors.json `
  --output Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz

& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_eroded_cliff_sector.py `
  --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz `
  --output Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz `
  --steps 256 --seed 71421 --rainfall-m .003 --thermal-rate .05 `
  --sediment-capacity .4 --erosion-rate .08 --deposition-rate .2 `
  --max-erosion-cell-fraction .005

& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_massif_cliff_seed.py
```

Measured source was exported by root from `3bcdb2f183164454758a55b99b478aae346aa670`
with `--geometry-ref HEAD`. Its position-function SHA256 is
`b0349251bec80a6ffa3677afcb742dc73eddb456d63dc820a0a73c32f2b3a524`.
The anchors identify canonical mesh SHA1
`87bb77d76a90baee8dd8515cc4db7e92f4c49f38`. The generator carries complete source
provenance, input hashes and anchor JSON; these measured dependencies must be
preserved or re-exported from that exact geometry version. They are not assets
included in this source commit. Do not replace them with today's formula and
call it the same candidate.

| Artifact | SHA256 |
| --- | --- |
| CurrentSource08/clean_constraint_seed.npz | `5cf296870251fc755b9c83c8725af336a1582047359a4e1402407624542786b0` |
| CurrentSource08/cliff_erosion_seed.npz | `8564969f5447949c72c598a9640fd5d86a20a99744bd0080c492711f38748200` |
| CliffSupportAnchors.json | `2e89c7506e68390eea305f5cb937261dde2376bd473cb9740c4068d7534772b5` |
| Final v3 seed | `2f45a1d758d298066e6ec5fa4580ecd0b454e1ef8ae6266abb318411c007ca6f` |
| Final v3 open256 | `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74` |

Final outputs are in `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/Massif-Cliff/`.
`massif_v3_morphology.json` records actual measurements and complete paths/hashes.
Reproducibility means matching numeric arrays; elapsed timing, file paths or
archive metadata can change archive bytes. An actual same-input replay reproduced
every numeric seed array exactly.

## Bounded erosion and verification

- Seven focused CPU regressions pass: released blanket locks, exact coastline,
  conservative support raster margin, immutable seams at small mismatches,
  measured-source frame rejection, incompatible anchors, discrete screened-fit
  equation and no influence from invalid padding.
- Independent worker audit passes domain/frame/outlet identity, measured coast
  and seams, fixed targets and complete clipped triangle footprint support.
  Partial Actor 36 remains unapproved. Audit files/tools are owned separately.
- Existing conservative open-outflow solver is unchanged. Final run preserves
  every fixed height bitwise, cuts at most 2.269m and deposits at most 2.204m. Solver
  solid mass error 0m3; water residual 5.53e-10m3. Float32 serialized solid error
  0.000358m3 is below the 0.510m3 quantization bound.
- Net change P5/median/P95 is -0.294/-0.054/+0.248m. 607 active cells cut>0.5m;
  314 are within 30m of the coast, though that band is only 6.4% of active area.
  Largest such connected cut component is 28x20m. Log-flow/gross-cut correlation
  +0.359 is a measurement, not proof of realistic gullies. Erosion remains subtle
  and still favours the foot; do not increase strength to disguise a weak shape.
- Parent/reviewer own matched clay/export and any isolated engine review. No
  imported Nanite asset, current water contact, waterfall binding, whole-arena
  seam stitching or runtime temple integration is supplied by this pass.
