# Scan-displaced historical crater face

Owner: Codex cliff worker, branch `codex/historical-crater-cut`. Owned tool: `Tools/generate_displaced_crater_face.py`. Input is the immutable [HeadlandA cut](HISTORICAL-CRATER-CUT-15NOV.md). Root owns scan download/extraction and Unreal work; this tool is NumPy-only offline geometry. No parent erosion, world scaling, coastline edit, temple edit, game asset or runtime build occurs here.

## Two bounded revisions

**A is visually rejected.** Actual continuous-normal clay shows native-scale repeating fine stucco on essentially the same smooth400m curtain. Only two crest troughs survived an overly conservative length gate, producing maximum2.6213m landward recession. The gate subtracted both guards, shortened the remaining face, then applied the full guard again. Four actual left-hand channels were incorrectly omitted. The unfiltered2048² scan also contained frequencies far beyond a0.5m mesh, causing aliased pitting. Evidence: `Fitted-Waterfall/Saved/ImplicitCliffReview/HistoricalCraterCut/DisplacedFaceAClay/{oblique,close_wall}_fractured.png`.

**B corrects those specific defects.** It includes eight unequal guides from actual parent crest troughs and drainage crossings. Left faces are only28–35m high, so their recesses remain24–31m long with5.6–7m requested depths. Taller right faces use60–92m lengths and10–18.66m requested depths. After physical guard taper, actual maximum recession is17.0668m. Guide widths derive from the neighboring original spurs; upstream parent trough direction supplies their drift. These are explicitly authored extensions of existing drainage, not a new erosion simulation or measured subsurface fracture network.

The scan is filtered in physical Fourier frequencies: a radial raised-cosine response passes up to0.5cycles/m and stops at1cycle/m, matching the0.5m maximum source-face edge. No physical tile enlargement or data-range normalization occurs. Filtered height range is0.374587–0.542643, giving about0.1008m of scan relief under the source-authored0.6m scale. Higher-frequency PBR detail belongs in the registered material; creating more geometry or inflating the scan is not the missing broad landform solution. Periodic repetition remains an explicit visual limitation.

**B is also visually rejected.** The actual oblique clay shows eight vertical slots with capped, nearly horizontal mouths. Recession is finally visible, but the fixed top seam and rapid deep taper manufacture quarry-like pockets in the same curtain. Worker and independent reviewer inspected `DisplacedFaceBClay/oblique_fractured.png`. Physical scan filtering is a valid sampling correction; it does not fix this larger construction failure. No native promotion or third displacement variant follows this note. Numerical validity does not change the visual verdict.

## Geometry contract

Conforming adaptive edge subdivision refines only the exposed `face_kind1` triangles to0.5m. Adjacent parent triangles receive necessary coplanar edge splits, avoiding T-junctions; their surface is unchanged. Both output variants share934,088 vertices and1,865,732 triangles. Original HeadlandA and revisionA files remain immutable.

Displacement is nonnegative localV, into the bedrock. U/Z stay unchanged, so the cut remains a single-valued V(U,Z) surface even if a later XY projection becomes steep. Exact segment-distance guards cover the original top join, open crop borders and the actual201-point water-contact curve, not merely the buried lower border. Recession is zero within2m and fades through2–8m. All original-parent top vertices are fixed. This leaves130,157 exact fixed vertices. Source-triangle IDs and barycentrics reconstruct the same-topology baseline; requested/actual recession, guards, parent limits and guide measurements are exported.

Requested recession is checked against the exact original ABD/ADC parent triangles at eight physical depth fractions; the first detected intersection is bisected. This is a bounded sampled collision check, not proof against every possible intersection. Neither actualA norB needed a clamp. Independent checks include actual face centroids, source lineage, contact and graph orientation.

IndependentA audit passed: clean connected open manifold; source correspondence within5.2e-10m;31,265 original-top,1,775 shared-seam, all outer-border and40,322 contact-strip vertices bitwise fixed. U/Z changed by at most1.6e-13m with zero graph orientation reversals. Sampled moved vertices/face centres remain beneath the original parent by at least0.746/0.675m. Root's production-query dry steering margin is35.0037m in local static samples. Report: `Cliff-Contact/Saved/WeatheredAudit/displaced_crater_face_a.json`. This does not approve whole-route clearance, old temple foundations or dynamic water contact.

Independent B audit also passed its bounded contract. Topology, source correspondence, frame and guards are identical to A. All 7,228 contact-crossing triangles and their 7,230 vertices remain bitwise unchanged; there are no U/Z orientation reversals. Evaluating 702,307 exact parent-triangle breakpoints along moved vertex paths found no parent penetration. Moved face-centre checks also pass. The physical filter replays bitwise to the saved float32 map. Report: `Cliff-Contact/Saved/WeatheredAudit/displaced_crater_face_b.json`; candidate SHA256 `4b26b17a78dafc9f24805fda7c76cbd662840d16b9a5880bb7e39487f41b72e1`. These checks do not establish exhaustive continuous solid intersection or whole-route safety.

## Verified source and co-registration

Source: [Poly Haven Marble Cliff01](https://polyhaven.com/a/marble_cliff_01), Amal Kumar, [CC0](https://polyhaven.com/license). Root saved original maps, source `.blend`, source manifest and linear height extraction under `Maelstrom-Saturday/Saved/CliffSources/PolyHaven/marble_cliff_01`.

Declared physical width and height are4.299351692m. Displacement scale0.600000024 is the upstream material's authoring setting, **not independent survey calibration**. Input height is2048² float32, linear Non-Color, bottom-to-top rows from Blender image pixels. SHA256: `b1f91b981d6b4ab1e4f9584f2ffdf061c0888c67c5e9729d43cb41229ce7f7ef`.

Texture coordinates are exactly `(localU/4.299351692, localZ/4.299351692)`, periodic, with no offset or rotation. Because U/Z never move, those final coordinates match the original displacement coordinates. The independent material reviewer can register diffuse/normal/roughness to the same field. Landward scan recession uses `(max(filteredHeight)-sampledHeight)*scale`, preserving relative protrusions without pushing rock waterward. No stochastic tile offsets or undisclosed UV changes accompanyB.

## Reproduction

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_displaced_crater_face.py --input Saved/HistoricalCraterCut/HeadlandA/candidate.npz --heightmap E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffSources/PolyHaven/marble_cliff_01/height_linear.npy --provenance E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffSources/PolyHaven/marble_cliff_01/height_manifest.json --width-m 4.299351692 --height-m 4.299351692 --relief-m 0.600000024 --edge-m 0.5 --output Saved/HistoricalCraterCut/MyFaceReplay
```

The tracked generator implements B. Exact A source is retained in its immutable output `generator_source.py`; current defaults do not reproduce A. Actual outputs are `Flow-Seams/Saved/HistoricalCraterCut/DisplacedFaceA` and `DisplacedFaceB`, each with baseline/candidate NPZ, manifest and exact generator snapshot. B also saves its filtered scalar height field. The first input cut hash is `fd6933153a11ad39b9010d956cb3f90e54931f5f2c032959fa0892417f72d6ff`; the original parent path/hash and scan manifest/hash are recorded in each output.

The source scan remains physically small. Its availability does not resolve the major rock masses, crest composition or missing coherent exposed-face geology by itself. Judge B's actual wall/oblique/close views before any native import.
