# Fracture clusters from the actual curved cliff skin — 15 November

Owner: Codex `cliff_material`, branch `codex/cliff-implicit-face-proof`, separate extension after checkpoint `f3559db`.

`Tools/run_implicit_cliff_reference.py` now accepts `--sample-points` and `--planes-only`. The new mode clusters actual physical points from the existing curved massif instead of producing another flat tile. It exports stable input membership, physical hull planes, fracture discs, original frame/provenance and supplied depths. It does **not** generate a rectangular backing, clip visible faces to the smooth source envelope, scale the sample, extract a mesh, or change Unreal. Root owns selective exposure and union with deeper original bedrock; only that resulting geometry can receive visual approval.

## Exact data and reproduction

Input: `E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/ImplicitCliffFit/SourceSkin/points.json`.
Input SHA256: `422e5eea7e53b0a4c5ddcd01f9431b7f67a531b0fe3a7deee70cc10a9d66ac8e`.
Its recorded source is `source08_measured_boundary_massif_v3_open256.npz`, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.

The parent's input is a 15,539-point jittered physical lattice in the inner original 72 × 40 m crop. Canonical point extents are approximately 61.96 × 17.56 × 63.45 m because the frame follows the steep wall. Spacing is 1.15 m with a declared conservative 0.805 m separation. Depth is the supplied approximate normal depth used for selection; it is not a new exact collision/support measurement. Original arena/crop frame and source hash pass through unchanged.

From `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/run_implicit_cliff_reference.py --upstream Saved/ReferenceCode/Rock-fracturing --output Saved/ImplicitFaceProof/SourceSkinReplay --sample-points E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/ImplicitCliffFit/SourceSkin/points.json --planes-only
```

The exact inspected upstream revision, patches and licence attribution remain as documented in [the corrected proof](IMPLICIT-CLIFF-FACE-PROOF.md). Only installed MSBuild/C++ and Python are used. Bulk inputs, upstream source and outputs stay ignored in `Saved`. Fresh output directories are required.

Final result: `Saved/ImplicitFaceProof/SourceSkinPlanesFinal/Geometry/block_planes_m.json`.
Result SHA256: `8ab1e5ec274324b98c7ded24043f3fc40c37b8e9575286b07ae34fc4ae9306b3`.
`metadata.json` beside it records actual source provenance, activity, used/excluded points and timing. The earlier `SourceSkinPlanes` has byte-identical plane data but generic preliminary sidecar descriptions; use **Final** metadata.

## Geometric contract

- Input `points_m[N][3]` and `depth_below_face_m[N]`, with stable zero-based array indices. Physical canonical coordinates are Z-up / front +Y. No point-cloud rescaling occurs: the internal source-unit conversion is inverted on export.
- Each output `blocks[]` entry contains `cluster_index`, exact `point_indices`, original `points_m`, depth values/statistics and ordered unit plane equations. Interior convention: `dot(normal, canonical_position_m) - distance_m <= 0`.
- Canonical plane equations have not been translated to arena coordinates. Under a proper rigid transform `p_world=R*p+t`, use `n_world=R*n` and `d_world=d+dot(n_world,t)`. The supplied frame is retained in `source_metadata` and the final sidecar.
- Three coherent finite fracture families share the entire physical cloud. Nominal canonical normals before normalization are `(0.91,0.24,0.34)`, `(-0.30,0.36,0.88)`, `(0.48,0.80,-0.36)`, with ±0.10 component jitter, explicit normalization and radii 10–24 m. This run generates 36 discs; exact values and centres are exported. There is no zero-normal upstream stub, independent tile seed or periodic stamp.
- Adjacency radius is 2.6 m; maximum separation of any two points in a cluster is 9 m. Original small-cluster rejection remains explicit: groups of ten or fewer points are omitted. This creates gaps that root's retained core must address; it is not a complete space-filling geological partition.

## Measured checks and limitations

The run yields **269 hull-bearing clusters**, retaining **14,814 unique input IDs** and excluding **725 points**. Actual fracture tests reject **100,566 of 301,299** candidate adjacent pairs (33.38%). Build and cluster/plane export took about **3.3 seconds**, with no marching-cubes extraction. A second fresh run produced byte-identical physical plane/membership JSON. Parser/finite-input checks, unique output membership and Python syntax checks pass.

Independent audit confirms exact depth copies, point-ID mapping within 7.50e-11 m, unit plane normals within 3.33e-16 and input members inside their hull halfspaces within 1.558e-6 m numerical tolerance. Maximum actual cluster point span is 8.99923 m. Original world-frame reconstruction differs by at most 2.27e-13 m. IDs remain authoritative if reconstructing from the original point cloud, avoiding the harmless text-rounding error in the convenience point copy.

This extends the meaningful part of the corrected reference—active fracture-constrained clusters—while removing the isolated panel domain. It does not claim a visually successful wall, closed output mesh, integrated coast/support preservation or finished geology. The earlier cube/backed-panel clay remains rejected. Root's source-informed exposed/retained cluster construction and actual clay review are the next gate.
