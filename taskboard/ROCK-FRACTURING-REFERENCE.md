# Implicit fractured blocks: inspected reference, 15 November

Owner: Codex `cliff_material`, branch `codex/cliff-implicit-block-reference`.
Scope: isolated source inspection, CPU reproduction and supplied-model clay review. No game code, assets, materials, vendor code or texture pixels are changed or committed by this pass.

## Result and provenance

The [primary implementation](https://github.com/aparis69/Rock-fracturing/tree/b91965b3011ed269cbc3a051b00c9b284aaa2e36) builds and generates all four examples locally. Its polyhedral family is a more useful starting point than our rejected smooth displaced surfaces: fractures separate whole irregular rock masses, with deep joints and unequal face planes. It is **not an accepted arena cliff**. The supplied examples retain a cubic envelope, broad gaps and excessive pitting; equidimensional reads as masonry and tabular as stacked plates. We have not adapted or integrated these meshes.

- Exact upstream revision: `b91965b3011ed269cbc3a051b00c9b284aaa2e36`.
- Local clone: `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/ReferenceCode/Rock-fracturing` (ignored).
- Licence: top-level MIT, copyright 2019 Axel Paris. Preserve its notice if substantial source is subsequently incorporated. Bundled convex-hull/STB/marching-cubes notices must also be retained; review those exact files before vendoring. The warp image's separate media provenance is not documented, so this pass does not redistribute its pixels.
- Citation: Axel Paris, Adrien Peytavie, Eric Guérin, Jean-Michel Dischler and Eric Galin, **Modeling Rocky Scenery using Implicit Blocks**, *The Visual Computer* 36(10), 2251–2261, 2020. [Author project page](https://aparis69.github.io/public_html/projects/paris2020_Blocks.html).
- The README explicitly describes a recoding, not the implementation that produced the paper's full results. Implicit replication, periodic/aperiodic tiling and several scenes are absent. Do not promise the paper's entire pipeline or timings from this sample.

## What the inspected code actually does

`main.cpp` uses seed 1234 and a nominal 20-unit cubic domain. It samples points using Poisson rejection, distributes finite oriented fracture discs, and clusters points only when their connecting segments do not cross fractures. A further all-pairs compatibility test limits cluster extent. Convex hulls turn clusters into plane sets; smoothed intersections form implicit solid blocks, combined with a BVH/union. An image-driven gradient warp adds surface relief before a 200³ marching-cubes extraction.

This constructs the underlying fractured masses, rather than engraving identical holes in a smooth wall or relying on texture normals. The sample still hardcodes dimensions: 2.5-unit adjacency, 10.5-unit maximum cluster separation, 0.25-unit block smoothing, warp coefficient 0.65 and texture frequency 0.1642. Smoothing is explicitly marked unsafe for some block sizes. These are source units, **not verified metres**.

## Export defects to fix before adaptation

1. `PolygonizeSDF` samples `box.min + index * cellDiagonal`, but returns raw marching-cubes grid positions. `main.cpp` writes those indices directly. Correct export is `box.min + gridPosition * (box.max - box.min) / (n - 1)`. Supplied OBJ extents around 174, or 98 for tabular, do not establish physical scale. For anisotropic boxes, transform normals by the inverse transpose or recompute from transformed triangles.
2. `MC.h` swaps x/z count names in its loop bounds. Cubic 200³ grids hide this; an anisotropic implementation must correct it before use.
3. The reference's tabular family treats Y as up. A future adapter must explicitly convert to arena Z-up with a handedness/winding check.
4. `LoadImageFileForWarping` does not check `stbi_load` failure and assumes its relative path exists. The reproduction prechecks the supplied texture and runs from the directory below; this loader needs a real failure path before reuse.

## Inspected execution and exact reproduction

The invoked program reads `../Textures/rock1.png` and writes four fixed OBJ filenames in its working directory. No network, subprocess or destructive filesystem action was found in that execution path, and the VS2022 project contains no custom build hooks. `blocks-sdf.cpp` contains duplicate implementations and is not part of the VS2022 build; compile the supplied project, not every source file indiscriminately.

The untouched revision failed here because `std::partition` lacked `<algorithm>`. Two changes exist **only in the ignored clone**: add `#include <algorithm>` after `<queue>` in `Code/Source/blocks.cpp`, and change its field pragma from `num_threads(16)` to `num_threads(8)`. No algorithm/geometry constants were changed. MSVC 14.44.35207 then built Release x64 in about four seconds; the remaining warning was the existing C4244 `std::distance` narrowing.

From the inspected clone root, after applying those two stated scratch changes:

```powershell
& 'C:/Program Files/Microsoft Visual Studio/2022/Professional/MSBuild/Current/Bin/MSBuild.exe' VS2022/RockFracturing.sln /m:2 /p:Configuration=Release /p:Platform=x64 /nologo /verbosity:minimal *> build_compat.log
New-Item -ItemType Directory -Force -Path Run-Inspected | Out-Null
Set-Location Run-Inspected
if (-not (Test-Path -LiteralPath '../Textures/rock1.png')) { throw 'Missing reference warp image' }
& '../VS2022/Out/Release/RockFracturing-TVC2020.exe' *> run.log
```

Completed 01:16:29–01:19:42 on 15 November 2026, about 193 seconds wall time, **exit 0**. `Run-Inspected/run.log` reports all four families done. An earlier wrong-working-directory invocation was stopped; only the correct `Run-Inspected` output below is accepted evidence. Supplied `Objs` were never overwritten. Generated meshes differ from the provided examples; this is reproduction of the recoded program on this compiler, not bitwise reproduction of the provided models or publication.

| Generated file in `Run-Inspected` | Bytes | SHA256 |
| --- | ---: | --- |
| `tile_equidimensional.obj` | 72,778,337 | `264d579f18834775054380221c3210110339f090db279d24a5a4fc2402eb8c6c` |
| `tile_rhombohedral.obj` | 72,243,693 | `b97edba82a25c58fff02e64b52b3b690fb465f73c221796c1ceedc81739abe93` |
| `tile_polyhedral.obj` | 52,388,921 | `833cc8c67fa0062f5bee67ab7772a4f8e2fe64e7a8793ac8b71926a4d9286ed5` |
| `tile_tabular.obj` | 72,015,321 | `55650eff359b17ef523d289f79d89c7284fb3689be357b5f7b58c30637c7a36f` |

Supplied warp image SHA256: `f202057042998f932805c0660a8db7b1d0b767de3fe92e32a37463f2f084f6ef`.

## Actual visual evidence and next bounded step

Reviewer CPU clay renders of the **provided**, unchanged meshes are in `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/ReferenceRockReview/ProvidedFitted`, with front/oblique views for each family and `review.json`. Polyhedral is the strongest starting family; rhombohedral second. I independently viewed both oblique renders and agree with the limitations above. These pictures are labelled with unknown source units and do not represent an arena import. Runtime-generated examples have not yet received equivalent visual approval.

Proposed adaptation, not implemented:

1. Fix physical coordinates first, then demonstrate a 20–40 m polyhedral outcrop with unequal joint families and reduced/suitably scaled warp. Measure actual block size and gap widths; do not enlarge a sample cube to 500 m.
2. Retain the accepted 3.2 km arena and massif as the macro envelope. Construct fracture-separated rock masses in its visible outer shell, constrained by the exact measured coast, tile edges and temple pads. Use common arena-space fracture seeds across overlapping extraction chunks so adjacent pieces share geology. The missing replication/tiling system needs an explicit replacement; repeated cubic stamps are not one.
3. Keep offline cell size and block dimensions physical. Spatially index local samples/fractures and mesh bounded surface chunks; sampling a full 500 m volume at close-up resolution with the naive 200³ demo is not scalable. Use the original final triangles for contacts/support and Nanite only as the rendering representation.
4. Review the first properly scaled clay outcrop before constructing the sector or adding a material. Preserve block-face hierarchy, reduce the pitted warp, and replace/relicense the image input before any redistribution. This algorithm is useful evidence for a better construction method; it does not yet satisfy the user's realistic cliff target.
