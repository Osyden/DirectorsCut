# Accepted cliff geometry fields

Owner: Codex cliff-design worker, `codex/cliff-scan-demo`. Material-only support task; accepted geometry is unchanged.

`Tools/bake_cliff_geometry_fields.py` rasterizes the existing UVs of the original Faceted triangles. It exports arena-metre position, area-weighted normal, slope, signed cotangent curvature (positive concavity, inverse metres), actual triangle/barycentric correspondence, roof/cliff masks and a roof-adjacency start mask. The last mask is a material guide, not simulated drainage.

Reproduce from the worker checkout:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/bake_cliff_geometry_fields.py --source Saved/ContinuousScanCliff/Faceted/full_ring_candidate.npz --output Saved/CliffGeology/FacetedFields --width 2048 --height 1024
```

Use a new output directory for a replay. The source SHA-256 is `87ccac911fdf761c90a0fd790d9a9b283ffd05a5f66ff392c3acfe3a642882d6`. Output is ignored under `Saved`; no licensed source pixels or geometry are checked into Git.

Actual result: 1,565,266 covered pixels, 247,322 roof pixels and 1,105,694 cliff pixels. Sampled barycentric position round-trip error is at most 0.145 mm after float32 atlas storage. The source UVs overlap on 772 pixels with over 3 cm radial separation; these are explicitly marked by `uv_overlap_mask` and must not be treated as trusted drainage geometry. Uncovered values are zero, with triangle index -1. The 2048×1024 image rows increase with V; its physical domain is 497.418836818384×280 metres, face Z origin -200 m, with the existing radial UV continuation on the plateau.

No Unreal jobs, rendering, source-geometry edits or acceptance claims were made in this task. The material worker consumes this bake for the unique geology maps.
