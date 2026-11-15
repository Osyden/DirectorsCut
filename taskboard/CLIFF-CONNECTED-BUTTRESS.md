# Connected rock buttress: bounded native proof

Owner: Codex cliff worker, `codex/cliff-block-adapter-audit`. No runtime files, clouds, water or editor assets changed here.

The accepted structural direction for a native comparison is `Saved/RockButtress/PoissonDetailedGJoined`. It replaces a narrow part of RingPilotB with one connected 3D fracture volume. The first hand-shaped prism, edge chips and strong rounded noise were visually rejected. The final mass uses unequal Poisson sites and actual shared-cell interfaces, keeps its interior filled, and joins the original bedrock. It contains real oblique faces, recesses and overhangs. It is still too cellular/stylized in neutral clay and is **not final cliff art**. Native context must decide whether this representation is useful.

## Reproduction

Run CPU Blender with eight threads and this repository's Tools directory available:

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 --python-exit-code 1 --python Tools/generate_rock_buttress.py -- --input Saved/CompressedEscarpment/RingPilotB/candidate.npz --output Saved/RockButtress/NewReplay --fracture-volume --poisson-sites --weather-planes --detail-after-union --scan-manifest 'E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffSources/PolyHaven/marble_cliff_01/height_manifest.json' --helpers-dir 'E:/Projects/DirectorsCut/Maelstrom-Saturday/Tools'
```

RingPilotB input SHA256: `18e7b77ce3540d52ebb459f40571e2980a6632dcba84073d6f6de502e2112046`. The externally stored CC0 scalar scan is checked against its manifest; native width4.299351692m and source-authored .6m relief scale are preserved. It is low-passed to the .5m mesh spacing; source pixels are not committed. Fine relief is authored geometry, not simulated weathering. Detail is applied **after** the coarse Boolean, avoiding dense intersection slivers. No continuous injectivity claim is made for the geometry-dependent join fade.

## Actual outputs and checks

- Local final surface:533,157 vertices /1,066,006 triangles. One open component,314 original boundary edges preserved, no winding, nonmanifold or zero-area defects in the existing review tool. Closed backing is edge-manifold; no exhaustive self-intersection test is claimed.
- Whole ring:778,716 vertices /1,554,046 triangles. Exactly3,480 old crop triangles replaced;488,040 original triangles retained. Every retained original vertex is bitwise unchanged. The source grid is also saved as `original_ring_vertices_m` and `original_ring_triangles`; it is **not a contact provider for the new buttress**.
- New vertices lie at134.501–136.939degrees, inside the canonical573..585 crop (134.297–137.109degrees). Hero waterfall vertex72791/column552 is exact and unchanged. Sibling independent polygon clipping found zero positive-area overlap between all50 actual temple footprints and the replacement crop. Existing proposed temple offsets therefore remain applicable outside the angular exclusion; random runtime assemblies still require their actual-footprint check.
- Production-source static query:minimum dry full-steer margin27.645m; minimum including submerged vertices26.998m. Existing lower source shore still extends up to7.355m into the analytic water coverage. This is sampled vertices/centroids only, not continuous collision or an improved shoreline claim. Actual report is `Saved/RockButtress/PoissonDetailedGJoinedClearance/report.json`; its mesh hash predates metadata-only clarification, numeric geometry is unchanged.

Native files are `Saved/RockButtressNative/rock_buttress_g.fbx`, its JSON/NPZ sidecars, `original_mesh_reference.json.gz`, and `preview_supports.json`. Export uses the existing original-triangle exporter, identity arena frame and179-degree normals. FBX SHA256: `faa55f5964a40d5960b99cfe2a7a5ab5530823605dc154bed04ef516ec4866a4`. Root owns import, original-mesh verification and preview restoration. No imported-default or shippable-quality claim is made.

**Native precision correction:** the first G import lost212 triangles and was rejected by the unchanged original-mesh audit. Independent calculation reproduced exactly208 zero-area triangles after float32 arena metres, and212 after float32 centimetres. The full source remains unchanged; no triangle deletion is needed. The generator now exports using local arena origin `[-1140,1112,-25]m`, identity basis, and checks actual float32-metre-to-centimetre conversion before writing. This produces zero collapsed triangles while preserving all world vertices/connectivity bitwise. Use **`Saved/RockButtressNativeG2/rock_buttress_g2.fbx`**, its regenerated original-mesh reference and support manifest. G2 FBX SHA256: `f8a742ee681d0f14559916e2c7be7358dfb3b9c23cfdac6cfdce721094e9082e`. Runtime must honor the sidecar origin; temple-support grids remain absolute arena data. Root still owns native verification; no audit tolerance was relaxed.

Actual paired CPU images are under `Saved/RockButtress/PoissonDetailedGJoinedReview`; the image metadata precedes the same metadata-only clarification. They show structural progress over the blank prism but remaining cellular appearance and rough source surroundings. The next judgement belongs in the full scene, not another generic noise sweep.

## H2: wider headland, after actual G2 native rejection

G2 native displayed a rectangular cellular insert on the smooth parent. `--headland` widens the source replacement to columns568..591, introduces unequal5–19m Poisson site spacing, reduces the exposed depth to mostly shallow relief with two larger fractures, and returns the side/foot behind the original rock. Its skyline comes from the retained/released interface rather than a flat box cap. The first H diagnostic incorrectly compressed the entire buried backing as well as the exposed layer; H2 restores full rear depth and passes the existing single-surface/edge checks.

H2 is `Saved/RockButtress/HeadlandH2`. Its new mass is connected, all336 original crop boundary edges remain, and its whole-ring preview contains248,293vertices/493,196triangles. All original retained positions are exact. New vertices lie133.605–137.956degrees; hero outlets at row47/columns550,552,554 remain exact. The production-source sampled minimum dry route margin is35.496m; minimum including buried toe is26.959m. No continuous collision claim.

Actual close and whole-ring neutral CPU frames are in `HeadlandH2Review` and `HeadlandH2Context`. Self-critique: the abrupt rectangular insert is reduced and the top changes the silhouette, but compression/grouping has made large faces too quiet again; there is a broad shaded recess. This is **not natural-rock acceptance**. A coarse native diagnostic is ready in `Saved/RockButtressNativeH2/rock_headland_h2.fbx`, with original-mesh reference/supports. It uses35degree crease normals, no new detail variation. FBX SHA256 `2001fee6a12e1ea8dd4c8586776a68b438fc52ffb203a590f456cfcbaeeac6ca`. Its sidecar origin must be honored. Root owns actual scene judgement and any subsequent temple relocation; old prop locations are not a permanent terrain-art constraint.
