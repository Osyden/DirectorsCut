# Cascade liquid-core shading pass

Owner: Codex cascade worker, `codex/crater-cascade-shading`, based on `d04c743`, isolated `Crater-Workers/Flow-Seams`.

Claim: only the `M_MaelstromWhitewater` material-generator block in `Tools/make_maelstrom_materials.py` and this handoff. Geometry/contact, volume caches, basin/tunnel flow, clouds and sky remain with their owners. Root runs Unreal, generation and captures.

Actual references inspected: `core_contact_0300.png` and `fitted_combined_03060.png` in `taskboard/review/crater-seams-fitted-14nov`. Both show creamy uninterrupted fall strips, flat lit faces and dark transverse bands. This pass addresses material causes; ledge geometry and contact are separate work.

## Concrete causes

- Every surviving pixel used the same pale base color and roughness 0.75, with no normal input. Even transparent/dilute portions therefore looked like a white decal on the cliff planes.
- The stationary `reach` opacity term depended only on downstream distance, producing whole-width dark bands at roughly 21m intervals. This compounded real ledge shadows.
- Advection used `UV.y - Clock * (18 + 12 * VC.b)`. Because progress varies down the fall, coordinate compression grew with elapsed time. For illustration, on a500m fall with linear progress its downstream derivative would be0.28 after30s, reach zero after41.7s, then reverse. That is not stable accelerating flow.
- The width decoding imposed a4m minimum even on physically narrower branches. Edge fades used28% of the full width, giving every stream a broad, uniformly soft outline.

## Implemented material-only correction

The shader now computes a lit liquid/aeration state. Dark blue-green liquid and pale aerated coverage have separate roughness responses, mixed by covered area. Opacity retains a thin liquid core while aerated fragments become denser. There is no emissive output or unlit substitute.

Each branch advects at a fixed22–28m/s, preserving feature scale through the full encounter. Geometry and volumes continue to supply large-scale changes in trajectory. Two-dimensional packets approximately3m across by9m downstream carry0.4m by0.9m aerated detail; the former whole-width reach mask is removed. Subpixel detail averages to coverage instead of turning distant falls into white noise or making them vanish.

Width follows the actual vertex encoding, with only a0.5m numerical floor. Edge fraying uses approximately0.2–0.6m physical fades. Ripple normals represent3.5cm folds plus8mm fine disturbances. Their world-space basis is derived from the real across/downstream metric UVs, avoiding reliance on a supplied bitangent whose orientation can change across folded contact triangles. Both ripple bands fade by pixel footprint.

Only the `M_MaelstromWhitewater` block changed. The fixed basin `FLOW`, tunnel, cascade curtain material, impact mist, volume data, source geometry and lighting are untouched.

## Validation and integration

Python AST parsing and `git diff --check` pass. No Unreal process, material generation, GPU work or visual acceptance ran in this worker. Root must regenerate `Tools/make_maelstrom_materials.py` and inspect the material compile result. No C++ build is required for this shader-only commit.

Review the same close core-only and combined views, plus the far deck view. Compare moving frames at early and late timestamps to verify feature speed/direction remains stable. Check that the liquid intervals have readable highlights, white aeration is patchy rather than a continuous creamy strip, narrow distant falls remain visible, and normals follow the fall on folded surfaces. The core may need brightness/coverage calibration after an actual render; this is not visual sign-off. Geometric ledge shadows, intersections and the separate volume silhouette remain outside this change.
