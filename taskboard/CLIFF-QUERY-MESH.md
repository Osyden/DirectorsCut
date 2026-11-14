# Original cliff triangle query — offline, not integrated

Owner: Codex cliff worker, `codex/cliff-authoritative-query`, based on `bba948a`.
Only the new exporter, standalone query header, standalone test and this note
are changed. No runtime callers, Unreal builds, GPU work or asset imports.

## Contract

`Tools/export_cliff_query_mesh.py` consumes the same seed/result NPZ pair as
`Tools/export_eroded_cliff_fbx.py`. Both import the same `triangles(valid)` helper:
the diagonal is **a–d**, with `(a,b,d)` and `(a,d,c)`, and a triangle exists only
when all three source samples are valid. This differs from the old procedural
cliff's b–c diagonal.

The JSON v1 artifact stores compact original vertices in arena centimetres,
exact original triangle indices, FBX UV0, and the row-major grid-to-compact map.
An absent sample has map index `-1`. It also records the orthonormal XY frame,
source file hashes, typed-array hashes and one combined geometry hash. Generated
bulk remains under ignored Saved. These are the original authoring triangles,
**not Nanite's simplified fallback**, and no render-LOD data is consulted.

`Source/DirectorsCut/DCCliffQueryMesh.h` is dependency-free C++17 math:

- Construct `FGridView` over caller-owned parsed vertex/map arrays.
- Run `Validate` once after loading; it checks frame, counts, indices, finite
  values and every mapped vertex's XY agreement with the physical grid.
- `QueryHeight(view, arenaXcm, arenaYcm, hit)` returns triangle height, normal,
  vertex IDs and barycentric weights. Failed queries leave the hit unchanged.
- A missing triangle returns false. At exact diagonal/grid boundaries, an
  existing adjacent triangle may own the point. No extrapolation bridges holes.

The header deliberately does not introduce an Unreal JSON loader or runtime
caller. A future loader can use the existing Json module to validate the schema,
hashes and counts, retain arrays, and build `FGridView`. The triangle list is
serialized for source verification/export; sampling uses the versioned fixed
grid topology and compact map.

This is a **single-valued arena XY heightfield**. It does not solve overhangs,
vertical faces, arbitrary 3D rays or collision. Inputs are arena coordinates
before the moving ship-centred pose; integration must handle that transform
explicitly. No shader displacement or later geometry changes are represented.

## Actual preferred candidate and reproduction

```powershell
& E:/Utilities/Blender/5.3/python/bin/python.exe -B Tools/export_cliff_query_mesh.py `
  --seed E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/Ridged-Cliff/source08_ridged4_detail_seed.npz `
  --eroded E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/Erosion-Outflow/Ridged4-Source08/ridged4_detail_open256.npz `
  --fbx-metadata E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/RidgedPreview/source08_ridged4_detail_open256.json `
  --output Saved/CliffQuery/source08_ridged4_query.json

& E:/Utilities/Blender/5.3/python/bin/python.exe -B Tools/test_cliff_query_mesh.py `
  --mesh Saved/CliffQuery/source08_ridged4_query.json
```

The FBX sidecar check passed: exact seed/result SHA256, vertex/triangle counts
and local-to-arena origin match. The source pair is the corrected four-octave
candidate documented in `RIDGED-CLIFF-LANDFORM.md`; historical input dependencies
are preserved there.

Artifact: `E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CliffQuery/source08_ridged4_query.json`

- 52,536 vertices, 104,078 triangles, 7,126,189 JSON bytes in this export.
- Geometry SHA256: `f4c525586af86739caeb04929112f045c50ea7b6d4365a7b30f894a7df65a1f9`.
- Hash encoding is specified in the JSON: row-major little-endian float64
  positions, uint32 triangles, float32 UVs, then int32 grid map.

## Verification

The test compiles the actual header with MSVC in a standalone CPU executable;
there is no copied production query formula and no Unreal process. JSON numeric
roundtrip and hashes are checked before feeding the source arrays to C++.

All checks pass:

- Rotated negative-coordinate frame and centimetre conversion.
- A saddle fixture distinguishes the correct diagonal from bilinear sampling.
- Both barycentric halves, missing-corner triangles, shared diagonal, outer
  corners and the valid neighbouring cell at a missing grid boundary.
- Nonfinite points, corrupt indices and nonfinite vertices reject correctly.
- **208,156** actual triangle centre/interior samples have maximum height error
  **1.82e-10 cm**, zero missing samples, and exact corresponding face normals.
- All **52,536** original used vertices, including the ragged boundary, query
  successfully; **21,922** absent-triangle centres correctly return false.

The first run caught a real boundary bug: a rotated-frame dot product placed
some exact vertices just below a grid integer. The fixed code snaps within
`1e-8` grid units before selecting neighbouring cells (2e-6 cm at this spacing).
Tests also verify that this tolerance cannot extend into a missing cell.

This establishes source-surface agreement only. The visual tile remains an
isolated preview; replacing the old geometry and all waterfall/temple/shoreline
queries still requires root's explicit integration work.
