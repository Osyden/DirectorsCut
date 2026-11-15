# Corrected physical implicit-block proof — 15 November

Owner: Codex `cliff_material`, `codex/cliff-implicit-face-proof`, based on root `b43ba15`.
Only `Tools/run_implicit_cliff_reference.py` and this note are owned. All source snapshots, meshes and build products are ignored under `Saved/ImplicitFaceProof`. No Unreal, GPU, arena geometry, material, coast or temple constraint is changed.

## Current result

The corrected image-free sample gives real unequal planes and recesses. The shallow backed variant is a **construction proof, not an accepted realistic cliff**: actual clay still reads as large triangular plaques with deep straight gaps on a rectangular backing. The disconnected cube also fails as a cliff. Do not import either as a finished asset or enlarge them to fill the sector. Root may make a rigid context-placement preview; that is a separate gate.

This replaces neither the macro massif nor its support constraints. No integrated shore/support preservation is claimed. The backing is authored for this isolated proof only.

## Important corrections to the reference assessment

The inspected MIT source is `aparis69/Rock-fracturing` at **`b91965b3011ed269cbc3a051b00c9b284aaa2e36`**, cited in [the previous reference note](ROCK-FRACTURING-REFERENCE.md). This is now a **derived corrected experiment**, not visual reproduction of its supplied OBJ files:

- Independent audit found `Sphere::RandomSurface()` returns the zero vector. Thus the upstream polyhedral normals are zero, and its fracture discs never intersect segments. The earlier provided-model appearance is not evidence of active polyhedral fracturing.
- The adapter constructs twelve finite discs with explicit isotropic unit normals. Their default radii are 6–13.5 m; seed 1234, authoring scale 1.5 m/source-unit. Their original centres, normals, radii and pre-extension cluster points are saved in `structural_seed.json`.
- `Matrix4::operator()` uses a wrong stride and the triple-plane intersection is unreliable. Both `SDFBlock` bounds construction and the preceding reconstructed-point validity check are bypassed. Each block's conservative bounds come from its actual hull input points plus smoothing padding. The defective matrix/plane path is not invoked in this adapter.
- The original grid-coordinate export is corrected using `box.min + grid * (box.max-box.min)/(n-1)`. Cubic **counts** of 200³ avoid the upstream x/z-count loop defect even though physical grid spacing is anisotropic.
- Physical conversion is `1.5 * (source.x, -source.z, source.y)`: proper rotation, determinant +1, **metres / Z-up / exposed front +Y**. Normals are recomputed from transformed triangles; outward orientation is checked using positive signed-field differences. The upstream `SDFNode::Gradient` has the opposite sign and is not used for that check.
- Block smoothing is 0.05 m. `SDFGradientWarp` is completely bypassed and the image loader is never called. No image pixels are copied into the build snapshot or needed at runtime.

The wrapper checks the literal upstream revision and checks each patched source fragment occurs exactly once. It recreates committed upstream files using `git show`, preserving its MIT/bundled notices in ignored scratch output. It does not depend on the previous uncommitted reference-build patches, download packages, or vendor the implementation into the game repository.

## Actual outputs

Paths are relative to `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/ImplicitFaceProof/`.

| Variant | Physical extent X/Y/Z | Vertices / triangles | Active fracture evidence |
| --- | --- | --- | --- |
| `Polyhedral/Geometry/sample.obj` | 29.9564 × 29.9447 × 29.9476 m | 341,532 / 683,106 | 3,365 / 29,097 adjacent point-pairs blocked (11.56%) |
| `ConnectedShallow/Geometry/sample.obj` | 30.3000 × 8.9830 × 30.3000 m | 191,144 / 382,284 | 2,980 / 11,451 adjacent point-pairs blocked (26.02%) |

Each has exact extraction/source bounds, physical grid spacing, outward-orientation vote counts, units/axis conversion, hashes and source-snapshot paths in `Geometry/metadata.json`. Both build/run completed successfully on installed VS2022/MSVC14.44.35207; approximate combined times 12.6 s / 8.1 s. Python AST and `git diff --check` pass.

The polyhedral output discarded two numerically collapsed triangles. The shallow output discarded none at its threshold; it had 127 outward versus 2 inward sampled field-orientation votes. Independent audit finds one connected component, no boundary/nonmanifold/inconsistent-winding edges or duplicate triangles, exact bounds correspondence, and recomputed metre-space normals agreeing within 5.24e-9. One very small triangle is below the reviewer's twice-area threshold 1e-10 m², though not exactly zero. Majority field orientation is not proof that every face is outward. These geometric checks do not change the visual rejection.

- Polyhedral OBJ SHA256: `a15ef3731c884edf32e1a58c812684544ead3b1e5bceb3d82e4f78e686a70abb`.
- ConnectedShallow OBJ SHA256: `92d4adb35d7cafaf0640a4cf1f0379b68ad3dee76a356a35f04946bd58be55c2`.
- ConnectedShallow structural seed SHA256: `caa2687feda658263367a0dfbd98639c8eaaae57ee58059b48106da2cc22b88a`.

The older `ConnectedFace` folder is an immutable full-depth diagnostic, superseded before approval by the root's 9 m depth limit. Do not use it for context placement. Source snapshots preserve the exact earlier harnesses; the current wrapper produces the shallow backed version. A later comment clarification changes adapter-file hashes but not geometry.

## Connected construction and placement contract

Shallow mode samples a 30 × 30 × 6 m domain, fractures/clusters it, retains original cluster points and appends copies projected rearward before making each hull. The shared backing spans canonical X/Z ±15.15 m and **Y −6 to −1.5 m**: 4.5 m thickness. Rear hull projections reach Y −3 m, inside that backing. Union with it removes through-to-background joints. This is a derived construction: side planes can change and joint narrowing is not guaranteed.

The exposed face is toward **+Y**; the fixed rear plane is **Y = −6 m**. Do not view the back or infer scale from OBJ bounds. Root's context adapter can apply only the declared rigid transform unless it explicitly regenerates physical parameters. Actual face maximum Y is about +2.983 m. The 30.3 m rectangular outer perimeter is a proof boundary and still needs proper integration with a larger connected mass.

## Reproduce

Use fresh output directories; the wrapper intentionally refuses overwriting an experiment:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/run_implicit_cliff_reference.py --upstream Saved/ReferenceCode/Rock-fracturing --output Saved/ImplicitFaceProof/ReplayPolyhedral --mode polyhedral
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/run_implicit_cliff_reference.py --upstream Saved/ReferenceCode/Rock-fracturing --output Saved/ImplicitFaceProof/ReplayConnected --mode connected-face
```

The upstream clone must have the exact recorded HEAD. No texture input is used. Source copy/build/run happen only under this worker's ignored `Saved`; the current default is 1.5 m/source-unit, 0.05 m smoothing and 200³ samples, with eight field-evaluation threads. Parameters and exported geometry hashes must be retained if changed.

Actual matched clay reviews, with a 1.8 m figure and fixed physical camera distances, are in `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/ImplicitCliffReview/Polyhedral` and `.../ConnectedShallow`. Reviewer and author independently viewed the close/oblique results. Unequal clean faces improve the previous soft/pitted attempts, but wide joints, pristine surfaces and panel-shaped boundaries remain visually inadequate. No further noise trial is part of this handoff.
