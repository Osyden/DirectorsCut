# Historical headland and measured-water crater cut

Owner: Codex cliff worker, branch `codex/historical-crater-cut`, starting from `8b84641`. Owned source: `Tools/generate_historical_crater_cut.py`. Offline original-triangle proof only; no Unreal assets, actor placement, runtime code, further erosion or imported dependencies changed.

## Actual result

The useful historical drainage terrain can be placed rigidly above the real receiving water and cut into a connected bedrock body. Its unequal ridges and branching valleys remain connected to the exposed face. **The first clay result is not accepted cliff appearance:** the constant 70-degree excavation produces a broad smooth curtain below those ridges. This is a verified construction and contact prototype, not finished geology or arena terrain. Adding a rock material would not repair the missing face structure.

Actual matched clay, continuous 179-degree vertex normals: `Fitted-Waterfall/Saved/ImplicitCliffReview/HistoricalCraterCut/HeadlandA/{wall,oblique}_fractured.png`. The measured-water view is `HeadlandAWater/oblique_fractured.png`. The visible surface contains no artificial rear or bottom caps. Water correctly hides the submerged toe; it does not disguise the smooth face. Worker and independent reviewer inspected these actual images.

## Construction and physical contract

Input is the existing `Flow-Seams/Saved/FastscapeNaturalBenchmark/PortableReplay/terrain.npz`; no simulation is rerun. Its original 2 m triangles are selected over source rows300..510 and columns20..220. Their rigid chart translation is `[0, -600, -55]` metres, with no rotation, scale, bending, noise or post-simulation height adjustment. In the original arena frame this is world translation `-600 * basis_v_world + [0,0,-55]`. The new 400 m-wide crop covers chart U40..440 and V0..420. Its highest visible point is +45.230 m; this is a low headland placement, not a generated temple foundation.

At each 2 m U sample, the shoreline is the interpolated zero of the actual source query `WaterEdgeRadius(angle) - radius`; `HeightAt(P,false)` is interpolated at the same crossing. The cut is `Z = shoreline_height + tan(70 degrees) * (V - shoreline_V)`. Each original triangle is split at its intersection with that affine surface. Retained top faces remain portions of the exact rigidly placed source triangles; excavated portions are lowered to the cut. Per-face source triangle IDs and cut/source/cap tags are exported.

The lower authoring edge extends 4 m waterward, but the slope is anchored at the actual shoreline, not at this overlap edge. Thus the overlap is 11.130–11.287 m beneath the **finite measured water mesh**, checked with recorded triangle IDs and barycentric weights. At every one of the201 shoreline controls, the placed parent top is23.186–166.101 m above the water: no natural ravine is silently filled to manufacture dry land. The source body is closed below with a flat authoring bottom and boundary caps; those are omitted from the review surface.

## Independent checks and limits

The auditor reports a clean connected visible open mesh of33,647 vertices/66,267 triangles and a clean single positive closed authoring solid. Source top reconstruction is within9e-13 m; source barycentrics are valid, and excavation only lowers. All201 actual triangle contact samples agree with the interpolated water controls within4e-13 m. The lower-overlap barycentrics reconstruct the finite receiving-water surface and its stated submergence.

Root additionally queried the actual production water/route functions at candidate vertices and face centres. Sampled static dry rock is landward of the shoreline, with minimum full-steering margin35.0046 m; submerged overlap has minimum sampled margin30.9462 m. These are **sampled local, static checks**, not continuous whole-route clearance, collision or dynamic-wave validation. The shoreline construction uses2 m sampled interpolation, and the context water omits FFT/WPO and waves.

The old tile seams, adjacent terrain joins and temple supports are not preserved or approved. The historical parent remains nonconverged. The current cut lacks stratified rock faces, fracture hierarchy, weathering and detailed contact treatment. No native Nanite preview or live scene integration is claimed. The arbitrary crop closure is authoring-only and must never be rendered as the cliff face.

## Reproduction and immutable files

Run from the worker checkout with the existing Blender Python (NumPy only):

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_historical_crater_cut.py --parent Saved/FastscapeNaturalBenchmark/PortableReplay/terrain.npz --source-coast E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz --water E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/StructuralCliff/mean_water.npz --output Saved/HistoricalCraterCut/MyReplay
```

The first immutable output is `Flow-Seams/Saved/HistoricalCraterCut/HeadlandA`. It contains `baseline.npz` (placed uncut parent), `candidate.npz` (visible cut), `closed_bedrock.npz` (tagged authoring caps), `manifest.json` and the exact generator snapshot. Outputs are original absolute-arena metre triangles with the original frame; no heightfield reprojection occurs after cutting. The tool deliberately targets this measured source08 grid and parent layout, not arbitrary input dimensions. Choose a fresh ignored Saved directory.

Input SHA256 values:

| Input | SHA256 |
| --- | --- |
| Portable historical parent | `6b3996b5932dc0c45af05ff0e68633e198ca14805f11dcad910b8ae5c5d4a3dc` |
| Current measured source08 coast | `8564969f5447949c72c598a9640fd5d86a20a99744bd0080c492711f38748200` |
| Finite measured mean-water mesh | `bd4514e69d97799d52a78c2100d39a5d85be6b02662036a2866d818333177ff2` |
| Referenced `grid_queries.bin` | `74da6a26564e48947ca30fed0d59b348049a2f553f828f94b9e2c5179ceb28c7` |
| First generator snapshot | `3fc3934e8c4f0d817f1a4b4d1a4fbc40dac2b55822f1632f57d6716f3565ce42` |

The water NPZ metadata identifies the exact `grid_queries.bin` dependency, whose bytes are verified before generation. Reproduction requires that measured query bundle, the tracked `generate_weathered_cliff_mesh.original_mesh` helper and the source parent. See [historical parent provenance and licence scope](FASTSCAPE-HISTORICAL-TERRAIN-15NOV.md). This cutter uses no Fastscapelib runtime and vendors no external implementation.

Next gate: preserve this useful natural parent and water-contact contract while testing an actual geological exposed-face construction. Do not interpret the successful contact/topology checks as permission to import the smooth cut into the game or to repeat a noise/material-only pass.
