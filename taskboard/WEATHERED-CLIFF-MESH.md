# Refined cliff fields: two appearance trials rejected

15 November, `codex/cliff-weathered-mesh`, based on `10b1fdc`. New generator only;
no runtime, material, UE or existing terrain-generator changes.

`Tools/generate_weathered_cliff_mesh.py` conformingly refines the accepted v3
original ABD/ADC triangles to <=1m surface edges: 1,025,013 vertices and 2,045,576
triangles. It retains exact source triangle IDs/barycentrics, transported masks,
protected coast/seam/temple vertices and a matching undisplaced mesh. Full sector
and the same 72x40m row89/col64 crop are emitted as original 3D NPZs.

Two bounded shape trials were actually rendered and rejected:

1. `--relief-mode ridge`: existing Blender 3D ridged/heterogeneous terrain fields,
   18/11/14m primary scales through 4.5/2.75/3.5m, 30/20/24m broad modulation,
   2.8–3.6m small detail. It gives a subtle soft lumpy skin on the same rounded mass.
2. `--relief-mode faceted`: Blender nearest-feature Manhattan Voronoi distances
   at 14/8/11m plus 5.5/3.5/4.5m, 0.7m domain warp at42m, small0.12m noise detail.
   Actual images still look soft/mottled; no convincing rock-plane hierarchy.

The preservation gate needed a **0.125 horizontal-component bias** of the stable
source normals in both cases, followed by renormalization. This retains Euclidean
displacement magnitudes but weakens relief perpendicular to steep rock faces.
For source faces >45 degrees, median/P95/maximum actual normal relief is
0.136/0.481/1.011m in trial1, and0.151/0.568/1.185m in trial2. Do not present the
larger mathematical displacement clamp as achieved rock-face depth. The mapping
keeps positive projected XY triangle areas and makes no overhang claim.

Generator supports a gentle 3–15m guard measured from original protected grid
points, a 24–60 degree steepness fade and source-erodibility-derived authored
hardness. Existing hydraulic masks use the old v3 global normalization
(log flow/gross deposition/gross erosion/1) before barycentric interpolation.
They are **not** new transport simulated on the changed mesh.

## Reproduce and inspect

From the worker root, with immutable input SHA256
`1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`:

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 `
  --python-exit-code 1 --python Tools/generate_weathered_cliff_mesh.py -- `
  --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz `
  --output-dir Saved/WeatheredCliff/faceted --relief-mode faceted
```

Use a fresh output directory; existing results are immutable. `ridge` reproduces
the first shape. The final generator clarifies upward-bias metadata; first-trial
archives retain their exact source snapshot as `first/generator_source.py`.

Outputs under `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/WeatheredCliff/`:

- `first/sector_weathered.npz`: `a0d335212d57ba7c152e8874b1b9048754b69249500e7ad681136e6e48cbb270`
- `faceted/sector_weathered.npz`: `157769fb9a8c8dedbf83f177034538532627aa5c510ba3f77e77bd95f40273d3`
- `faceted/crop_weathered.npz`: `0d9aeed1d2c5cb472e475a0f2740460b6cd12be0c80ee4ea4e4dac5bf523ece7`

Each folder also has the matching `*_baseline.npz`, exact generator snapshot and
`weathered.json`. `comparison.json` records actual perpendicular relief and hashes.
Final same-input replay reproduces every numeric field exactly. Metadata/timing
can change compressed archive bytes on replay.

Actual matched close/full/deck/1500m-far clay views are in
`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/WeatheredReview/`
(`FirstClose`, `FirstFull`, `FacetedClose` and the review worker's final full folder).
Use the same angle35 normals and physical camera in both images. Fixed sawtooth
toe/side edges are inherited from the baseline; arbitrary close-crop boundaries
are not semantic constraints and may move.

Independent first-sector audit passes all153,790 fixed vertices and4,448 boundary
vertices bitwise, source reconstruction3.98e-12m, mask error<=2.98e-8 and topology/
winding checks. Actual source footprint samples are unchanged. The final trial
uses the same correspondence/guards; its independent audit is reported separately.
No shape is accepted for FBX, Nanite or live integration by this pass. A separate
radial-shear proof is newly authorized to address the weak displacement mapping;
it must not be relabelled as another noise setting or a successful result here.
