# Compressed eroded escarpment prototype

15 November 2026. Owner: Codex cliff-design worker, `codex/cliff-block-adapter-audit`. Scope: new offline `Tools/generate_compressed_escarpment.py`; no runtime geometry, lighting, materials or water assets changed. This is an authored deformation of an existing eroded parent, not another erosion simulation or a claim of final cliff quality.

## Construction

The input is the exact portable historical-parent terrain already reviewed in `Flow-Seams/Saved/FastscapeNaturalBenchmark/PortableReplay/terrain.npz`. The prototype retains the source's 400 m U span (40..440 m), and main-face U/Z coordinates, with the existing -55 m vertical placement. It compresses only V. The mapping derivative rises continuously from 0.12 near the shore to 0.35 at the crest; selected original radial spans of 425.27..807.16 m become 83.64..158.74 m. This carries the actual eroded ravines and interlocking ridges onto the face rather than cutting them away with a smooth 70-degree excavation.

The source does not supply a simple coastline matching the modern vortex: there are 1..10 upward height crossings per column, and blindly selecting the last crossing jumps 207.72 source metres over a 2 m column. The tool therefore uses the minimal landward Lipschitz envelope of those crossings, with source slope bounded to 2 m/m. This is explicitly an authored contour selection. A separate 6 m wide shore join bridges its 0..46.19 m height gap to the actual measured modern contact. Its lower overlap, waterline controls and finite-water barycentric payload are inherited from HeadlandA. No claim is made that this join is a naturally connected historical contour.

Main-face vertices use exact original ABD/ADC barycentric heights. The parameter-grid retessellation approximates original triangle interiors: sampled centroid height error is median 0.030 m, P90 0.265 m, P99 0.895 m and maximum 2.991 m. This approximation is declared; only vertex U/Z preservation is exact. The original source cell size remains 2 m, so this prototype supplies macro structure, not fine fracture detail.

## Reproduction and artifacts

Run the NumPy CLI from this checkout:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_compressed_escarpment.py `
  --parent 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/FastscapeNaturalBenchmark/PortableReplay/terrain.npz' `
  --shore-reference 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/HistoricalCraterCut/HeadlandA/candidate.npz' `
  --output Saved/CompressedEscarpment/ParentCompressionA-Replay
```

The original artifact is `Saved/CompressedEscarpment/ParentCompressionA/{baseline,candidate}.npz`, with the exact generator snapshot and `manifest.json`. Candidate SHA256: `d4902107e275836ea1d10ff6470615d8763a2a0c51166e1376573f23774a5992`. The baseline is the same selected parent surface before radial compression, with rigid V-600/Z-55 placement and without the new shore apron. It is deliberately much deeper than the candidate.

## Measured checks and limits

The candidate has 82,209 vertices and 163,200 triangles, with dimensions 400 x 203.038 x 204.650 m. There are no degenerate triangles, inconsistent winding or nonmanifold edges; the open perimeter has 1,216 edges. Every triangle has positive XY area, minimum twice-area 0.25264 m2. The main-face source height reconstruction error is below 1.5e-14 m, and every referenced source triangle corner belongs to the valid parent domain.

Root's independent production-source query sampled every vertex and triangle centroid: 244,607 dry samples, closest dry rock 0.6393 m landward of finite water, minimum dry full-steer-envelope margin 35.6393 m, and minimum margin including submerged rock 30.9463 m. Evidence: `Maelstrom-Saturday/Saved/CraterCutProbe/ParentCompressionA/report.json`. This is a static local sampled clearance check, not continuous collision, animated-wave contact or whole-arena navigation approval.

Old outer preview seams and temple footings are explicitly not preserved. Native integration would require a deliberate neighboring-terrain transition and new measured landmark support. No Unreal import or runtime activation has occurred. Actual clay review is pending; topology and clearance alone do not establish a realistic cliff.

## Actual visual review: rejected for native integration

I viewed the actual continuous-normal wall and oblique images in `Fitted-Waterfall/Saved/ImplicitCliffReview/HistoricalCraterCut/ParentCompressionA`. This representation removes the broad blank excavation curtain and retains real branching relief across the face, but the result still fails the intended cliff quality. Narrow source ravines become deep diagonal black slits between smooth fins. The explicit narrow shore join also produces a scalloped fringe. The reviewer reproduced the slits with flat normals, so the earlier split-normal explanation does not apply.

The radial compression shrinks nominal 2 m source sampling distances toward roughly 0.24..0.70 m while retaining 2 m tangential spacing. It amplifies the parent model's narrow channel cross-sections and creates strongly anisotropic faces. Clean topology and positive XY areas rule out new internal topological holes; they do not establish attractive geology or eliminate visibility/self-shadow problems. The existing source retessellation error is a further limitation, not a reason to call the slits a material defect.

An independent author check agrees with source U/Z reconstruction, the declared V map, the bitwise contact payload and one connected open surface. Its report is `Flow-Seams/Saved/ReadOnlyAudits/parent_compression_a.json`. No second mapping trial was made. This candidate is useful evidence that the parent relief can replace the blank cut, but its narrow channel cross-sections need deliberate geometric treatment before native integration; stronger textures or more amplitude would not fix the present fins.
