# Original triangle surface query — 15 November

Owner: Codex cliff audit worker, `codex/cliff-triangle-surface`, based on `2b80091`.
Scope: new standalone header and CPU harness. No runtime terrain, gameplay,
waterfall, material, Nanite, or asset integration changes.

## Contract

`DCCliffTriangleSurface::FSurface` builds a balanced BVH over a supplied
`FMeshView` of original authoring vertices and triangle indices. Arrays remain
caller-owned and must stay immutable and alive until the surface is cleared.
Coordinates are arena centimetres; transforms are caller-owned. This is an
arbitrary 3D ray query, not an XY heightfield or a Nanite fallback query.

`TraceNearest(Origin, Direction, Hit, MinCm, MaxCm, CullMode)` normalizes the ray
and returns centimetre distance, position, source triangle/index IDs,
barycentrics, winding-preserving unit normal, and an explicit front-face flag.
Two-sided is the default; front/back culling is explicit. Normals never silently
flip to face the ray. Misses leave the output unchanged. Invalid rebuilds clear
the old surface. Invalid indices/nonfinite vertices reject a mesh; degenerate
triangles are skipped without renumbering source IDs.

Shared-edge tolerance is `1e-10` in barycentric coordinates, bounds padding is
`1e-6 cm`; this is a practical double-precision query, not an exact arithmetic or
universally watertight predicate. Repeated nearest-hit queries can enumerate
separated surfaces by advancing the minimum distance; the harness uses 0.01 cm
to avoid double-counting coplanar/shared-edge intersections. There is no general
all-intersections API, parser, mesh mutation, or manifold-validation layer.

## Reproduction

From the worker checkout (or any checkout with these files), using a Python with
NumPy and installed MSVC:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_cliff_triangle_surface.py
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_cliff_triangle_surface.py --mesh 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/FracturedOutcrop/clean/fractured.npz' --output Saved/CliffTriangleSurfaceTests/Fractured
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/test_cliff_triangle_surface.py --mesh 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/FracturedOutcrop/clean/baseline.npz' --output Saved/CliffTriangleSurfaceTests/Baseline
```

The runner compiles the actual header and tests stacked layers, underside and
vertical rays, winding/culling, exact shared edges/corners, negative arena
coordinates, distance limits, nonunit/huge/subnormal directions, degenerate and
invalid input, and rebuild failure. It compares the BVH against independent
linear plane-intersection/dot-barycentric calculations. The oracle uses a
different formula; MSVC's `long double` is not higher precision than `double`.
All generated binaries/results/provenance stay in ignored `Saved` directories.

## Measured evidence

20 synthetic checks pass, including 5,000 oracle rays with zero mismatches
(maximum distance error `2.05e-12 cm` in the arbitrary 3D test). Each actual mesh
adds 256 oracle rays and two checks, all passing.

| Source | Vertices / triangles | Skipped degenerate | Actual oracle max error | Vertical rays with >=3 hits | Maximum hits / front faces |
| --- | --- | --- | --- | --- | --- |
| Clean uncut baseline | 23,490 / 46,976 | 0 | `2.36e-11 cm` | 0 | 2 / 1 |
| Clean joint-cut outcrop | 25,352 / 50,708 | 0 | `6.37e-11 cm` | 544 | 8 / 4 |

The identical 64x64 vertical probe grid covers 1,896 rays in each source. The
baseline's two hits are only closed-solid entry/exit. The cut mesh's additional
separated hits and four front-facing intervals demonstrate real multiple
surfaces along a vertical ray, so this data is not a heightfield projection.
Both BVHs contain 16,383 nodes; node count depends on the balanced leaf size.

Input SHA-256:

- Baseline: `d6a37d70c6cd1f0c4c9b085b90d19dccf2df9caca1a19833e753e0dee088d21e`
- Joint-cut: `30c5dac1e5871c9bc89e86a9da6cce7296f1da39318b55cf6e70c4de7f21ea9d`

These are the cleaned initial outcrop arrays, explicitly scaled from their
`vertices_m` key by 100. Source axes and triangle ordering are unchanged. JSON
result provenance records the queried position/index hashes as well.

## Acceptance limit

This proves ray-query correctness for tested meshes and real overhang topology.
It does **not** approve the cliff art. Root rejected this first cut shape as saw
kerfs on a smooth slab; a wedge-detachment revision is separate. No Unreal
render/build or final runtime contact agreement is claimed here. Any future
provider must load the exact rendered original-source arrays with the same
coordinate transform and invalidate fitted waterfall caches when they change.

## Subsequent finite-wedge source check

The same runner, unchanged, was rerun with
`Flow-Seams/Saved/FracturedOutcrop/wedges/fractured.npz` and output directory
`Saved/CliffTriangleSurfaceTests/Wedges`. Source SHA-256 is
`117868b62c7422e50d6360ef18023f92a56c67387f40826f9abe3f54fe12c46f`.
Its 18,290 vertices / 36,576 original triangles build with zero skipped
degenerates. All checks pass; the 256 actual-mesh oracle rays have zero
mismatches and maximum distance error `3.09e-11 cm`. The identical vertical
grid has 1,896 covered rays, 242 with at least three separated intersections,
and a maximum of six intersections / three front faces. This confirms real
multi-interval geometry survives the wedge revision; it does not decide its
visual acceptance. No geometry or query source changed for this measurement.
