# Continuous shared-cell bedrock — bounded experiment, appearance rejected

Owner: Codex cliff worker, branch `codex/cliff-connected-bedrock`, based on `235a0fa`. Owns only new `Tools/generate_jointed_bedrock_sector.py` and this handoff. No runtime, Unreal, GPU, material, original-source or support-anchor changes. User Blender PID 19812 was left alone; all generation used separate factory-startup CPU processes with eight threads.

**Both visual attempts are rejected.** First makes scattered quarry-like pockets in the old soft massif. The single permitted corner-release revision reduces the number of pockets, but the broad silhouette remains essentially unchanged and surviving cuts still have flat polygonal mouths. The final geometry is numerically valid after one exact flap cancellation; that is not visual acceptance. No third morphology attempt, import, arena integration or material camouflage is implied.

## Construction tested

The immutable v3 eroded massif remains one connected original bedrock volume. Its true irregular valid-domain boundary is extruded downward by 160 m for authoring closure only. The final original-3D surface omits this tagged back and sides; the backing is not game geometry.

A globally shared oblique anisotropic Voronoi partition defines finite cells. Metric scales are 38 / 27 / 24 m, rotated coherently, with deterministic irregular site offsets of ±0.29 lattice units. Each cell is constructed from exact pairwise halfspaces; computed vertices are checked against all competing sites. This is a geometric cell partition, not new height or material noise. It does not claim a calibrated tectonic/fracture process or exactly three planar joint families.

Most cell interfaces remain closed inside the original solid. Only selected surface-crossing cells are removed. There are no shrunken block hulls, gaps tied to point-cloud spacing, refill volumes or central ellipse. Physical amplitudes come from cell geometry; removal candidates are bounded to approximate normal recession below 23 m and vertical recession below 120 m. Source-normal depth is a selection approximation, not a new collision/contact model.

All original triangles touching any protected source corner are frozen: 16,359 triangles / 9,324 unique vertices. A cell is rejected if its entire conservative source-XY bounding box plus four source cells (8 m) touches that protected-vertex union, an invalid sample or the domain edge. Thus support volumes are protected vertically, not just at their vertices. This leaves the actual measured coast, seams and temple pads unchanged.

The first attempt selects 26 cells, preferentially near existing concavities, and uses a collection Boolean difference. The corner revision selects ten convex-shoulder-preferred cells and performs sequential exact differences. It additionally tests that outside cell vertices span at least 100° in XY direction and their sampled source normals span at least 35°. These are **corner-selection proxies**, not proof that a cut has a useful open geological silhouette. The actual images show the limitation.

## Actual failures and numerical correction

`Saved/ContinuousBedrock/First` retains the initial 26-cell output. Actual wall/oblique views were rejected. `FirstClean` applies the existing numerical Boolean cleaner to the same shape; independent audit still finds 92 nonmanifold edges, 74 duplicate triangles and 11 added interior boundary edges. Both preserve the protected source geometry, but neither is an integration candidate.

`CornerRelease` is the one morphology revision. It produces one exact opposite-winding triangle pair: visible faces 101097 and 101297 share the same three vertices in reverse order, creating a zero-thickness flap. Each has area 0.331819 m², so this is **not** a tiny-area deletion. The independent auditor identified the pair and proved that removing both faces and the unused vertex yields a valid open manifold.

`CornerReleaseClean` uses narrowly defined cancellation of exact opposite-winding face pairs after Boolean cleanup. The matching closed-source loop-triangle IDs were 207159 / 207359. The closed and original visible diagnostics remain immutable; the new clean artifact has 52,206 vertices / 103,418 triangles, 992 original open boundary edges, no degenerate triangles and no extra nonmanifold edges. The shared `mesh_metrics` helper counts open boundary edges in its `nonmanifold_edges` field, hence the valid exposed result reports 992 in both fields. Signed volume of an open surface is not physical rock volume.

The actual clean output independently passes as one valid open manifold; the closed diagnostic is one positive manifold with 209,480 triangles. All 16,359 frozen source triangles / 9,324 protected vertices and the 992 original boundary edges are bitwise exact. The audit proves only the two opposite faces and one orphan vertex were removed: no other triangle sets or positions changed. Report: `Cliff-Contact/Saved/WeatheredAudit/continuous_bedrock_corner_release_clean.json`. Visual rejection remains unchanged; this is a rejected experiment, not the next production method.

Original source-vertex restoration is restricted to XY regions disjoint from every actual cutter bounding box. New Boolean intersection vertices are never snapped back to a heightfield. Protected original coordinates therefore remain exact, while new underfaces retain their complete original 3D geometry. The generator saves a failed candidate and exits nonzero if its basic exposed-surface geometry gate fails; it does not silently claim success from a written file.

## Reproduction and immutable inputs

Worktree: `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`.

Input: `Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`. This ignored immutable artifact is required; its measured-source provenance is inherited from the earlier massif pipeline. The tool deliberately asserts the actual source datum Z=0 and Z-up basis rather than silently mishandling another frame. It uses the exact original ABD/ADC source sampler.

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 --python-exit-code 1 --python Tools/generate_jointed_bedrock_sector.py -- --source Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz --output Saved/ContinuousBedrock/ReplayCorner --release-mode shoulder --max-releases 10
```

Blender 5.3.0 `02f0899d78e3`. The final run took 13.56 s. The tool refuses overwrite and writes only under a fresh Saved path. `--release-mode gully --max-releases 26` retains the rejected first selection strategy for diagnostics; later cleanup/provenance changes mean it is not a promise to reproduce the original failed NPZ container byte for byte.

Recorded candidate NPZ SHA256 values:

| Artifact | SHA256 |
| --- | --- |
| First | `fe8a08a7253de3e6f8019744780fa615dd2214dcb6d9ef5ee8797ab3ad1ea9c5` |
| FirstClean | `b33ad44246c5b6b532f5eb900ae3baf009cea2c034b893fa40479bae4a64284b` |
| CornerRelease | `cd41b13b28ef1056fe84cb54101b1e82fbcef90c04d18ac0bb36b9eb440c0b54` |
| CornerReleaseClean | `bd07b26867e8200766a2a58331f7c0988ad5f06d3e54204324ae2d0b9f024a5c` |

Final contiguous-array SHA256: `vertices_m` = `8e081d47a5d96232d824b507953c91ef2a7baba7cbc9c653e61d1712f8caf854`; `triangles` = `4ce5c24ed1837a0d55bc495631024eb5aa3d1806e15deec3f40eed8d87829d60`. Containers also include original source-protected triangle indices, rigid frame and full manifest. No new scientific transport masks are fabricated.

## Actual visual gate

Images are under `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/ImplicitCliffReview/ConnectedBedrock/{First,CornerRelease}`. Wall and oblique views use the original 500 m sector, measured mean-water context, identical camera and 15° angle normals. Earlier distance views also test 1.5 km and the actual 3.2 km opposite rim without enlarging geometry. The corner revision was rejected at the primary wall/oblique gate, so there was no reason to render more distant variants.

The whole-mass silhouette is still inherited from the soft heightfield, while cell removals read as Boolean excavation. Cleaner topology, fewer holes or more material detail cannot be claimed to solve that. The next substantive work must establish a coherent primary escarpment and its exposed mass hierarchy. This experiment only establishes that shared closed cell boundaries remove the point-hull gap dependency; it does not supply the required finished landform.
