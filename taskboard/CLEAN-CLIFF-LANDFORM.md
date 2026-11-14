# Clean cliff seed proof — rejected for runtime

Owner: Codex cliff worker. Branch `codex/cliff-landform-seed`, based on `540c059`.
Scope: only the new offline generator, focused tests and this note. No runtime,
engine, assets, rendering configuration, sky or water changes.

## Outcome

This pass produces a valid clean seed from the measured shoreline and plateau
constraints, without taking the upper envelope of the rejected folded wall.
It **does not produce a sufficiently natural cliff**. The matched untextured CPU
reviews reject both the branched setback version and the later catchment version.
Do not import/export either as the new runtime cliff or claim visual acceptance.

The final catchment preview has connected drainage, but still looks like three
stamped Y cuts on a smooth dam. Its repeated tall partitions and uninterrupted
mass are the wrong starting structure. More rainfall steps or texture detail are
not a fix. The next generator should form an irregular ridge/watershed mass first,
then constrain it to the crater shore and selected supported temple plateaus.

## Source and constraints

Input is the measured source8 seed at:

`E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/Source08-Seed/cliff_erosion_seed.npz`

It is a **boundary reference from rejected candidate `4bb5b0c`**, not the current
approved default surface. Only the first supported lower branch through `V=.20`
and the `V>=.84` plateau are used. The old heightfield and middle raw profile are
not read. The maximum protected dry apron is35m; actual measured width varies
4.82–34.45m. Freezing a full35m where the talus is narrow would retain most of the
old folded wall.

The grid is253×251 at2m, covering504×500m in the original design-world frame.
Interior height rises91–175m. The original3.2km crater is not edited.

The base shape integrates a positive slope distribution with irregular setback
and branching recesses. It is radially monotone and cannot contain overhangs. It
is still too similar to a machined escarpment. `--preview-catchments` adds explicit
2D trunk/tributary cuts in arc/rim-distance metres: variable width, depth and
reach length, with20–38m contributing head reaches behind the rim. The largest
cut is44.83m. The generated NPZ includes the actual graph in `drainage_paths_json`.

Catchment mode releases1714 plateau cells. Temple footing compatibility is
explicitly **unverified**; authoritative temple pad masks are needed before any
integration. Existing receiving shore, invalid padding and outer12m tile edges
remain protected. Fixed valid cells adjacent to ocean padding form251 outflow
receivers for the erosion worker's new conservative outflow solver.

Polar resampling is not an exact replacement for the original triangle sampler.
Against the rejected seed, fixed plateau differences have median1.1mm, P95 2.3cm
and maximum67cm. Lower-shore differences include the discarded folded upper
envelope and reach3.8m. Therefore this proof must remain isolated; it cannot be
dropped onto the existing rendered surface and called an exact seam match.

Old raw profiles/discarded spans are removed from output. `base_profile_*` is the
new monotone shape before optional catchment cuts, not the final carved surface.
`source_kind=clean_macro_landform` and the provenance labels distinguish those.

## Reproduction

From this checkout in PowerShell:

```powershell
& E:/Utilities/Blender/5.3/python/bin/python.exe Tools/test_cliff_landform.py
& E:/Utilities/Blender/5.3/python/bin/python.exe Tools/generate_cliff_landform.py --input E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/Source08-Seed/cliff_erosion_seed.npz --output Saved/CliffLandform/source08_catchment_seed.npz --preview-catchments
```

Omit `--preview-catchments` for the reproducible branched-setback baseline.
Seed71421 is deterministic. Six tests verify that rejected interior data cannot
affect output, base profiles are monotone and endpoint constrained, kernel schema
and outlet boundaries are valid, immutable constraints do not vary with seed,
old raw controls are not mislabelled, and tributaries join actual trunk nodes
while the receiving shore remains unchanged.

## Actual review evidence

The review worker rendered CPU clay using identical cameras, physical scale and
raking light, with no surface-detail shader:

- `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/CleanSeedReview`
- `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/BranchedSeedReview`
- `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/CatchmentSeedReview`

I inspected the oblique and wall views of the initial seed, and the oblique/plan
views of the final catchment seed. The initial slots become dark pointed openings
in the final form; cutting the plateau connects them but does not turn the slab
into convincing geological mass. Sawtooth toe triangles are a grid/domain display
artifact, not authored rock detail.

The erosion worker's bounded256-step test on the initial seed removes boundary
ponding with explicit outflow but barely changes the upstream morphology. Its
flow/incision correlation is-.055; broad relaxation dominates. The final catchment
candidate was deliberately not subjected to more erosion or exported to FBX.
