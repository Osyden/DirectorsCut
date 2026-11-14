# Actual source-8 erosion audit — diagnostic, not an accepted cliff

The measured candidate-wall seed was evaluated; no runtime surface or seed cache changed. Its geometry provenance is `4bb5b0c` (export source head `3339c57`), which root has rejected visually and is reverting from default use. These diagnostics must not be described as the current accepted scene.

Input: `E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/Source08-Seed/cliff_erosion_seed.npz`. The 253 × 251 grid covers 504 × 500 m at 2 m spacing. There are 12,206 active cells, 52,536 physical rays and 10,967 nonphysical padding cells. Active elevations span about 172 m; their 10th–90th percentile span is 117 m. This is a local cliff sector, not the 900 m water bowl. The seed preserves toe, shoreline, tile boundary and plateau/temple support. Its upper-envelope conversion discards overhang surfaces: 315 rays are multivalued, up to 7 intersections and 28.81 m of vertical span lost.

## Actual numerical result

| Run | Net height 5th / median / 95th percentile on active cells | Maximum cut / fill | Cells cut >1 m | Bedrock hits |
| --- | --- | --- | --- | --- |
| Defaults, 256 steps | -8.06 / -0.61 / +5.97 m |13.13 /12.82 m |4,599 |32 |
| Bounded 192 | -0.48 / -0.014 / +0.66 m |2.17 /4.16 m |131 |0 |
| Bounded 256 | -1.09 / -0.060 / +1.23 m |4.16 /4.09 m |664 |0 |
| Bounded 384 | -0.91 / -0.048 / +1.19 m |4.18 /4.05 m |538 |0 |
| Bounded 256, overhang neighborhoods fixed | -0.54 / -0.054 / +0.39 m |3.72 /5.27 m |121 |0 |

The default is aggressive, not merely surface noise. But it does not demonstrate useful natural drainage: 79% of bounded 256 cells cut more than 1 m lie within 4 m of projected-overhang rays, even though that neighborhood occupies only 14.5% of active cells. The gentler run concentrates 96% there. Gross erosion/accumulated-flow correlation is only 0.04 in bounded 256. This indicates the kernel is predominantly reacting to upper-envelope discontinuities, rather than repairing the smooth/clay macro landform. Correlation alone is not an artistic quality metric; the projection concentration is the stronger diagnostic.

Protecting those neighborhoods leaves 10,435 active cells. Most changes become centimetres to submetres, and one 5.27 m deposition spike persists near a protection boundary. Fixed bits and bedrock checks pass, but this is not a credible final cliff or an approved mesh-export candidate. More iterations would amplify the wrong input representation. The seed exporter/preview worker received both bounded candidates for honest paired CPU views.

## Reproduction

All outputs remain ignored under `E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/Erosion`. Key files are `actual_source08_bounded256.npz`, `actual_source08_bounded256_overhang_protected.npz`, paired `actual_source08_overhang_protected_seed.npz`, `actual_source08_comparison.json`, and `actual_source08_overhang_protected_metrics.json`. Run with the existing `E:/Utilities/Blender/5.3/python/bin/python.exe`:

```text
Tools/generate_eroded_cliff_sector.py --input <seed.npz> --output <result.npz> --steps 256 --seed 71421 --rainfall-m 0.0006 --sediment-capacity 1.6 --erosion-rate 0.12 --thermal-rate 0.08 --max-erosion-cell-fraction 0.0075
```

Use the original measured seed for bounded 256, or the saved protected variant for the last row. This CLI exactly reproduces the saved physical arrays. For bounded 192 use steps 192, rain 0.0003, capacity 1.2, erosion 0.10, thermal 0.05 and erosion fraction 0.005; bounded 384 uses steps 384, rain 0.00035, capacity 1.4, erosion 0.12, thermal 0.06 and fraction 0.005. All other settings remain recorded defaults in each output's `metrics_json`.

To regenerate the protected variant without changing the source, copy all NPZ arrays; take `intersection_count > 1`, dilate it twice with an 8-neighbor boolean stencil (4 m Chebyshev buffer at 2 m cells), set `protected_mask=1`, `erodibility=0` and `loose_m=0` only there, and save a new seed. Height, bedrock, frame and original raw profiles remain unchanged. The variant's `analysis_variant_json` records this additional protection; original seed metadata is retained as provenance.

## Kernel correction and limits

The generator now honors optional `heightfield_valid_mask`: invalid padding receives no rainfall and cannot send or receive hydraulic/thermal flux. Protected valid rock still passes water while remaining immutable. Derived slope/curvature masks are suppressed beside invalid padding, with `diagnostic_valid_mask` marking their valid region. A regression raises invalid padding by 1,000 m and confirms all physical-cell transport/height outputs remain identical. Eight tests pass. In this particular seed the padding correction does not change active deformation, but it fixes an otherwise false catchment source.

The protected actual run takes about 1.7–1.9 s for 256 steps. Solid solver mass error is 0 m³, water error 1.13e-10 m³, and serialized solid error -0.000254 m³ remains inside its 0.554 m³ float32 rounding bound. Numerical conservation does not establish visual quality.

Next: construct a clean macro landform from the agreed crater shoreline/plateau bounds, establish meaningful bedrock and catchment drainage, then erode that continuous source before attaching real fractured faces/overhangs. Retain one geometry source for mesh export and water contact. Hydraulic erosion is useful for catchments and runoff cuts; it cannot recover discarded overhangs, create fracture mechanics or repair an artificial base wall merely by running longer. Root owns the new seed contract, mesh/Nanite export and rendered acceptance. No additional candidate-wall tuning is authorized by this audit.
