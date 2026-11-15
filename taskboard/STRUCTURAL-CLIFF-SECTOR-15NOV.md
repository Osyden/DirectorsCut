# Structural cliff macro proof — rejected appearances

Owner: Codex cliff worker, branch `codex/cliff-structural-sector`, based on `cac2cbf`. Owns only `Tools/generate_structural_cliff_sector.py` and this handoff. No runtime, material, sky, source constraints, UE asset or mesh integration changes.

Both bounded shape trials were reviewed in actual neutral clay and **rejected**. No third morphology trial, erosion run, import or sector rollout is authorized by this result. These are retained diagnostics, not finished cliffs.

## What changed and what failed

The previous raw mountain field spans only −17.15 to 68.31 m. Its 60 m screened boundary fit creates much of the measured 230 m cliff drop: 16.26% of free cells need more than 50 m correction and 28.59% need more than 25 m. This explains the smooth curtains independently of later shading.

The new raw field directly spans the measured coast-to-plateau height. It uses an 18 m residual boundary fit and preserves the existing semantic constraints. Trial A uses an angular escarpment, four finite plane-bounded buttresses, two oblique drainage cuts and a tributary. It reads as enormous concrete prisms and straight trenches, including at 1.5 km. Reject.

Trial B retains the existing heterogeneous/ridged mountain hinterland at 40% relief about a 24 m datum, with eleven unequal overlapping buttresses capped at 36–90% cliff height. Drainage becomes two finite 20–29 m recesses and a 10 m tributary. No new noise is generated. The upper terrain improves, but exposed faces still form clean triangular fins with a repeated retaining-wall shadow rhythm. Reject. Adding fine noise or running more erosion is not evidence this macro geometry will become realistic.

| Measurement | Previous v3 macro | Corrected A | B |
| --- | ---: | ---: | ---: |
| Free cells with residual >50 m | 16.26% | 0.699% | 0.571% |
| Free cells with residual >25 m | 28.59% | 2.260% | 1.962% |
| Median absolute residual | — | 0.983 m | 0.647 m |
| Maximum final height | 61.08 m | 52.72 m | 41.25 m |

The reduced boundary correction is a real architectural improvement. It is not visual acceptance of either plane construction.

## Source coordinate correction and guards

An initial A diagnostic derived inland distance from the first valid raster row. Its 2 m row jumps become 7–10 m artificial steps when multiplied by steep face slopes. The committed generator instead interpolates the zero crossing of measured `shore_clearance_m`; exact submerged toe targets remain fixed. Initial `EscarpmentA` is retained only as failed bug evidence. `EscarpmentAContinuous` and B use the correction.

The inherited triangular toe fringe is mostly submerged: first valid points sit 5.76–8.11 m inside receiving-water coverage. Do not change those exact samples merely because a waterless clay preview exposes them. Water-context review remains necessary.

Independent existing audits pass for corrected A and B:

- All 52,536 valid points, 8,032 fixed land cells, 251 outlets, semantic arrays, frame, XY coordinates and original triangulation are preserved. No new geometry defects; 104,078 triangles and 992 original boundary edges.
- Measured coast/seam targets remain exact. Fully covered temple supports 33/34/48 remain at target. Actor 36 remains explicitly partial; the tile does not establish support outside its boundary.
- No calibrated tectonic model, overhangs, true 3D joint exposure, live integration or successful erosion is claimed. RGB masks in the mesh outputs are diagnostic zero, alpha one; no new transport simulation is invented.

Audit reports: `Cliff-Contact/Saved/WeatheredAudit/structural_escarpment_continuous_{constraints,mesh}.json` and `structural_massed_b_{constraints,mesh}.json`.

## Reproduction

Worktree: `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`.

Required immutable input: `Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz`, SHA256 `2f45a1d758d298066e6ec5fa4580ecd0b454e1ef8ae6266abb318411c007ca6f`. This measured-constraint NPZ is an ignored artifact from the earlier massif pipeline, not synthesized silently from current runtime geometry. Its frame, anchors and historical boundary provenance remain in metadata. See the earlier massif handoff for its generation. Reproduction requires this exact input or an explicitly versioned replacement.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_structural_cliff_sector.py --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz --output Saved/Structural-Cliff/ReplayA --variant escarpment
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_structural_cliff_sector.py --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz --output Saved/Structural-Cliff/ReplayB --variant massed
```

The tool refuses overwrite and writes only under a new Saved directory. It uses the existing `screened_fit` and original triangle conversion; no packages or engine required. Syntax and diff-whitespace checks pass. Provenance labeling was corrected after the recorded runs so future baseline exports identify their actual pre-erosion source, rather than inheriting candidate settings. Geometry reproduction is unchanged; NPZ container hashes can differ with metadata/path changes.

Recorded corrected A: `Saved/Structural-Cliff/EscarpmentAContinuous`. Seed SHA256 `214f0de2fb71b8033929284109cf49b4da5df99a34035482403ad3cf3716a05b`; candidate container SHA256 `a623901e35df45171ffccfff353df3f25739f65e266560ad93f0429994135fbc`.

Recorded B: `Saved/Structural-Cliff/MassedB`. Seed SHA256 `dab1290c0d7d2e0619b6fd328ae2324ccddacd9142f942cbbef72ecdceb6ef01`; candidate container SHA256 `6d8f275add9749e57af795aa91b98e54cd0be29a7fc4a6df4585997c34f53cfa`.

Array SHA256 checks (contiguous bytes): corrected A `vertices_m` = `3fa8692a9be903a4134a313cde1ecdec0f5f2332586343e9be2e39fcdc7bc69b`; B = `519241b5b05155a72cebdf218e7738928336ca0e544cd930de1c6364942a640c`. Shared `triangles` = `fb09a9d051985b01f0877579bbb8c4c2e8bc77aa4d89381396201ffda84a361e`.

## Actual review evidence and next boundary

Matched wall, oblique and 1.5 km images are under `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/ImplicitCliffReview/StructuralMacro/{EscarpmentAContinuous,MassedB}/`, named `{wall,oblique,far_crater}_fractured.png`. Reviewer used the previous full-sector **eroded** v3 baseline to match earlier review cameras; the generator's supplied baseline is the pre-erosion v3 seed. This minor baseline difference is documented, not hidden.

Next work should retain a directly authored full-height coast-to-plateau envelope but use geological mass construction across the exposed face, with connected irregular fracture-defined volumes and a common structural hierarchy. These 2D cap/face intersections are not a substitute for that. The prior accepted local original-3D outcrop remains a separate bounded proof; neither of these new macro fields should replace the live scene.
