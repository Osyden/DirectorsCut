# Physical-scale cliff clay and normals review

Worker branch `codex/cliff-clay-normals-pass8`, from `f06d420`. CPU authoring only; no Unreal launch, asset import, lighting, gameplay or runtime terrain changes.

## Optional normals, separate from geometry

Both the original FBX exporter and clay reviewer now accept `--normal-mode angle --normal-angle-deg 35`. The default remains `flat`, preserving prior diagnostic/replay behavior. The shared helper uses Blender5.3's sharp-edge dihedral threshold: adjacent faces below the threshold share smooth shading; larger breaks keep split corner normals. It does not infer geological fractures, change vertices, add overhangs or create detail.

The normal configuration records its threshold, sharp-edge count and exact position/connectivity SHA256. It checks positions and topology remain bitwise unchanged. UVs and color masks are untouched. Each before/after clay stage recalculates its own edge splits after updating coordinates.

Tests on a shallow and steep crease confirm the intended distinction: the shallow shared-edge normals coincide (dot0.99999997); the54.7-degree crease stays split (dot0.57735). Source positions, connectivity and FLOAT_COLOR masks remain identical. A tiny twelve-corner FBX roundtrip through the original exporter settings preserves custom normals with maximum component error5.96e-8. This fixture is not a full new terrain export or Unreal import.

## Identical-geometry A/B on the previous rejected landform

Actual CPU Cycles views, twelve samples, eight threads, locked to `Saved/CliffLandform/CleanOpen256Review/review.json`:

- `Saved/CliffLandform/Normals8-Ridged4Flat/{wall,oblique,deck}_after.png`
- `Saved/CliffLandform/Normals8-Ridged4Angle35/{wall,oblique,deck}_after.png`

The generated reports prove identical cameras and identical before/after geometry hashes across the normal modes. After erosion,1,573 of156,613 edges retain sharp splits. The angle version removes the conspicuous two-metre triangle grid but leaves the artificial continuous cap, apron and inflated buttresses clearly visible. **The old landform remains rejected.** Smoothing does not solve its silhouette or drainage.

New massif geometry is judged in the same flat clay views first; angle shading is an explicitly labeled second diagnostic, not a substitute for shape acceptance. Full terrain export/import remains gated on the parent's visual acceptance.

## First massif: useful shape, invalid boundary source

Actual fixed-camera flat and angle reviews are under `Saved/CliffLandform/Massif8-CurrentFlat` and `Massif8-CurrentAngle35`, each with wall, oblique, deck and plan views. Both used seed SHA256 `b3f57f6c069abcf8874263abb56c041816636efc9bed18234628670693133c06` and identical position/connectivity hash `3b4b110a158b56c8475fe5a9bac342304b398c54e4843daeea054a2911a7bde5`. The angle version retains5,025 sharp edges.

The continuous upper cap is removed. Unequal ridges and saddles now connect into a hinterland, and buttresses extend to the toe. This is a clear macro-shape improvement in flat shading. Remaining failures: broad smooth inclined faces, inflated rather than fractured ridges, conspicuous rectangular support pads in the oblique/plan views, and the isolated tile's exposed side/shore seams. The normals A/B removes grid facets but does not fix those shapes. No erosion result was supplied for these first-seed views.

**Do not use this first variant as boundary-preserving terrain.** Parent found that its clean constraint seed differs from the current measured boundary by up to77m. A corrected variant must use the fresh measured NPZ as its explicit boundary source; re-render that before exporting. The first candidate's own metrics report65.5%→15.1% fixed physical samples and27,440 released samples; earlier message figures based on an older input do not apply to this file. No full FBX was exported.

## Corrected measured-boundary v3 and bounded erosion

The corrected v2 seed was reviewed first. Final v3 preserves its numeric geometry and supplies complete provenance, including measured boundary source `3bcdb2f`. It uses the measured NPZ for narrow immutable coast/seams, not the earlier clean constraint approximation.

Final paired before/after views and diagnostic maps:

- `Saved/CliffLandform/Massif8-MeasuredV3ErodedFlat`
- `Saved/CliffLandform/Massif8-MeasuredV3ErodedAngle35`

Both sets contain wall, oblique, deck and plan images under the same original pass-6 cameras. Seed SHA256: `2f45a1d758d298066e6ec5fa4580ecd0b454e1ef8ae6266abb318411c007ca6f`. Eroded result SHA256: `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`. Source/result hashes, camera transforms and geometry hashes match exactly between normal modes. After erosion, 4,747 of 156,613 edges retain sharp splits. Fixed physical land is 15.29% of samples.

The revised ridge/saddle hierarchy survives the corrected boundaries. Bounded erosion changes are modest (maximum net cut 2.26935m, fill 2.20384m): slight foot/ridge relaxation and localized channels, not a new silhouette or convincingly fractured cliff. The flow map follows several routes to the coast, while most large visible faces retain the same shape. The angle-normal view exposes their remaining smooth, inflated character without the sampling-triangle distraction. Rectangular temple pads and hard outer seam strips remain obvious in the isolated view. These need actual supported structures and neighboring terrain context; neither is validated by these clay renders. Real fractures/overhangs remain future work.

## Export authorized for the isolated engine preview only

Parent accepted corrected v3 eroded macro geometry for the **isolated engine preview gate**, explicitly not final geology acceptance. The original preferred FBX remains untouched. New artifact:

`Saved/CliffLandform/MassifPreview/source08_massif_v3_open256_angle35.fbx`

FBX SHA256: `88e587e7794e153bb662e4ff9c609116674fb9b847a2c9d47da192605fa81e44`. JSON sidecar records the exact frame, dimensions, mask divisors and shading policy. Dimensions are 500 × 492 × 230.62712m. It retains 52,536 vertices and 104,078 triangles.

Full Blender FBX roundtrip checks all physical corner positions, RGBA and triangle-corner normals. Maximum position error is 2.28882e-5m; mask error is zero; custom normals survive with maximum angular error 0.02704°. The normal check explicitly allows 0.1° for imported custom-normal storage quantization, while retaining strict topology and mask checks. It rejects lost smooth/sharp splits rather than falsely requiring bit-identical encoded normals.

Prepared read-only UE audit reference: `Saved/CliffImportAudit/source08_massif_v3_open256_angle35.json.gz` (2,337,149 bytes). Parent owns import/audit/render and the decision to show independently verified supports only. This worker did not launch Unreal, change materials or alter live geometry. All generated assets/reviews remain ignored under `Saved`; only tools and this handoff are committed.
