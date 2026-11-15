# One contiguous regional 3D exposure

Owner: Codex cliff worker, `codex/cliff-regional-3d`, based on `4a16933`. Owned source: `generate_cliff_skin_samples.py`, `assemble_cliff_skin_blocks.py`, `extract_cliff_authoring_surface.py`. Root owns the corrected fracture runner, full-sector composition and any native preview. No Unreal, GPU, runtime, material or original seed changes by this worker.

The existing source-informed fracture method now authors **one 160 × 96 m region**, rather than tiling or enlarging a 72 × 40 m patch. This is a useful larger original-3D proof. Actual near and full-sector clay still reads as a dense polygon/rubble scar in softer parent terrain; clean slabs, deep similar gaps and the transition remain inadequate for finished cliffs. No sector rollout or final art approval follows from geometry validation.

## Actual source and physical contract

Input: `Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`. This immutable v3 eroded source is retained; neither rejected structural A/B heightfield is used.

Crop descriptor: row 95, col 129, width 160 m, radial depth 96 m, exact 4 m edge guard. Center arena XYZ is approximately `(-1169.2443636,1195.2052620,-3.70288682)` metres. Source height range is −70.39815 to 35.21371 m; minimum measured coast distance 30.26549 m. Existing `validate_crop` and independent scan find no invalid/protected samples, no temple-pad overlap within 12 m; nearest actual protected vertex is 24.33 m away, temple-pad vertex 36 m.

The point cloud covers the inner ±74 × ±42 m with 6 m inset, 2 m jittered physical lattice and vertical skin depth 0.15–14 m. It contains 21,516 stable IDs; audited nearest pair is 1.42014 m, above the conservative 1.4 m limit. Source-normal depth is approximate and used only for selecting releases; final geometry remains original triangles in arena metres. The fitting rotation is a proper rigid transform, not a scale change.

Root's clustering keeps the accepted 18 m maximum physical block span, uses 4.4 m neighbour reach for the coarser lattice, and 160 finite fracture discs across the same three coherent families. Disc radii are 10–24 m. The region contains 456 clusters and 19,428 included original IDs, with 2,088 excluded IDs covered by retained bedrock. Actual maximum block span is 17.9979 m.

Assembly retains a 7 m core and 24 m vertical authoring backing. It uses the same physical 0.5 m surface topology, 0.25 m exposure-distance grid, 0.75 m polygon-halfspace gap margin and 4 m transition/guard. Only the regional placement of connected release areas changes with crop dimensions; joint dimensions do not scale up.

## Generalization and checks

The sampler accepts explicit row/column/width/depth/spacing, validates grid alignment and existing coast/temple guards, and caps output at the runner's 24,000-point limit. The assembler and extractor consume this descriptor rather than old 72 × 40 m constants. Regional exposure-grid origin, sampling, guard and cavity dimensions are all derived from it.

Each convex footprint is evaluated only within its conservatively expanded polygon bounds. This prevents regional grid cost from multiplying by every small block. A CPU regression against the original `4a16933` production implementation on twelve irregular convex footprints yields **bitwise identical** distance fields and filled-hole counts. Regional boundary weights remain exactly zero. Source syntax and whitespace checks pass.

Actual assembly retained 424 hulls and detached 32. Of retained hulls, 409 already had spatially spread contact with the core and remain original; 15 use short downward anchors, maximum 2.420015 m. Another 15 long-anchor fragments and 3 exposed thin fragments were rejected; 14 detached under the shallow release rule. No blanket rear extension is introduced.

Original closed output: 126,527 vertices, 253,086 triangles, zero reported boundary/nonmanifold/degenerate defects, exact original perimeter. Independent audit confirms all **17,856 original 4 m guard vertices** and **1,024 visible outer edges** are unchanged, with no backing leak in 1,012,344 sampled face centers/edge midpoints.

The closed body also contains two tiny trapped inward cavity shells: 30 and 36 triangles, signed volumes −0.04947495 and −0.02163142 m³. Independent two-direction original-triangle ray tests place all 103 cavity vertices/face-centers inside the main shell. These are not floating rocks. Original closed and first extracted files remain immutable.

The optional `--omit-audited-cavities` extraction mode binds the independent audit to the exact input SHA, verifies exact membership, isolated closed edges, negative measured volumes totaling below 0.1 m³, absence from the crop guard/authoring closure, and complete recorded two-direction containment. It omits only those 66 hidden triangles. No exterior face or position changes; no exhaustive geometric intersection proof is claimed.

The final exterior independently passes as **one valid open manifold**, 64,537 vertices / 128,092 triangles. All 1,024 original outer boundary edges, remaining positions/triangles and frame are exact; only the 66 audited cavity faces are absent. Report: `Cliff-Contact/Saved/WeatheredAudit/regional_joint_exterior_final.json`. This numerical gate does not override the rejection of regional visual rollout.

## Reproduction and artifacts

Run in `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_cliff_skin_samples.py --source Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz --output Saved/RegionalCliff/ReplayPoints/points.json --row 95 --col 129 --width-m 160 --depth-m 96 --spacing-m 2
```

Root runs its updated corrected reference clustering on that exact point JSON. The actual clustering output is `E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/RegionalCliff/FracturePlanes/Geometry/block_planes_m.json`; root owns the runner parameters/source commit. Do not silently use clusters generated from another point-file hash.

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 --python Tools/assemble_cliff_skin_blocks.py -- --points Saved/RegionalCliff/SourceSkin/points.json --blocks E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/RegionalCliff/FracturePlanes/Geometry/block_planes_m.json --output Saved/RegionalCliff/ReplayExposure --core-depth-m 7 --rear-depth-m 12 --release-patches --joint-transition
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/extract_cliff_authoring_surface.py --input Saved/RegionalCliff/JointExposure/candidate.npz --output Saved/RegionalCliff/ReplayVisible.npz --omit-audited-cavities E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/WeatheredAudit/regional_joint_exposure_cavities.json
```

The second command deliberately references the **actual audited** closed source. A new assembly requires a matching audit before hidden-shell omission; old indices are not applied to different geometry. The tools refuse overwrite and write generated data only under Saved.

Actual artifacts:

- Points `Saved/RegionalCliff/SourceSkin/points.json`, SHA256 `7a829d97d4fe57a45e93d77de5acc0e9a90215e6b627e014b789ac43bf2a37de`.
- Closed `Saved/RegionalCliff/JointExposure/candidate.npz`, SHA256 `7a310f18decbc4a54d4e4916a667ed9713bf42f51aa7f3e06aba353784960975`.
- Original extracted `visible_candidate.npz`, 128,158 triangles, SHA256 `8e04e6981e0ba3b14795205aae7e7598daca62e016da52a8b4cf0080f523de1f`.
- Final exterior `visible_candidate_exterior.npz`, 128,092 triangles, SHA256 `e4f9d41f09851035ecce30f6afb18107436df32786741b76bb80ef8619798764`.
- Baseline `visible_baseline.npz`, SHA256 `6819287b0755560c9a87b0b1e1d1d0be6b808864bc58eaf7e1d2b7e38fe062d1`.

Blender CPU 5.3.0 (`02f0899d78e3`), eight threads. Assembly took about two minutes. No vendor code or bulk geometry is committed here.

## Actual visual critique

Near and full 500 m views are at `Fitted-Waterfall/Saved/ImplicitCliffReview/RegionalExposure/{NearAngle15,FullContextWithWater}`. Root's context composition preserves the surrounding source and includes measured water to hide the legitimately submerged toe. Both worker and independent reviewer agree: real faces and larger contiguous coverage improve representation, but the exposed center still reads as a polygon jigsaw/rubble scar, with clean slabs and similar deep gaps. At 1.5 km it is visible without becoming a common geological structure across the whole cliff.

A likely contributor is the use of convex hulls of finite-spaced point centers: their separation can produce sampling-width voids rather than explicitly authored narrow fracture apertures. That is a hypothesis for a measured future construction change, not permission to expand random hulls or flatten the real 3D surface. Shared fracture boundaries, unequal connected mass hierarchy, gap scale and transition into the macro landform need further work. Material noise or another large heightfield prism pass will not resolve the observed shape.
