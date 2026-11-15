# Resolved bedding proof — visual target not reached

Owner: Codex cliff material worker, `codex/cliff-bedding-resolved`. This is an offline correction to the parent's `apply_cliff_bedding.py`, with no Unreal/runtime changes.

The previous layer fraction reset produced a depth discontinuity; a dense 1 cm sample at source U=230/V=180 measured a maximum neighboring jump of 0.845 m. Continuous profiles reduce that sample's maximum increment to 0.00755 m. The old 0.4–1.05 m joint half widths were also under-resolved by 1.5 m edges. The corrected proof uses 0.85 m edges, 3.7–5.2 m full bed support and 3.6–6.6 m full joint support. These are authored physical dimensions, not geological measurements.

Protection follows the actual curved top V in each source column and explicitly fixes every open boundary vertex. Contact/join triangles stay fixed. Local fold attenuation spreads through mesh edges over 6 m rather than independently halving adjacent vertices.

Reproduce from the worker with the installed NumPy Python:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/apply_cliff_bedding.py --input E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CompressedEscarpment/ParentCompressionSigma6/candidate.npz --output Saved/CliffBedding/Sigma6ResolvedReplay --edge-metres .85
```

Immutable source SHA256: `0753f94e7c54646dc2c2136cfc3a2296af241910dab5e7d699c6fd57f97d80f6`. Actual proof is `Flow-Seams/Saved/CliffBedding/Sigma6Resolved`: 1,237,228 vertices / 2,472,115 triangles; maximum recess 1.59990 m; 2,339 refined boundary vertices fixed; all protected surface displacement zero; no triangle reverses against its source normal. The limiter directly touches 19,153 vertices and influences 233,972 through its smooth envelope. This is not exhaustive self-intersection validation.

Matched actual clay views are in `Cliff-Contact/Saved/CliffBeddingReview/Sigma6Resolved/{oblique,close_wall}_{baseline,fractured}.png`, with angle179 normals and measured mean water. The corrected field removes the most abrupt scratches, but repeated horizontal bands and soft vertical slots still read as carved plaster on smooth folded terrain. Close shading serration remains in some steep areas; the baseline already contains some crest shading artifacts. **Not accepted as realistic rock, and no native rollout is recommended from this proof.** Physical scan textures are not a substitute for that missing geometric face hierarchy. No additional parameter trial was run.
