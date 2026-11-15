# Cliff boundary repair and actual engine/context review — 15 November

Branch: `codex/maelstrom-saturday`. This pass corrects the bounded fractured-rock prototype and imports its original 3D surface into an isolated Unreal preview. **The normal arena has not been replaced.** Local rock planes improve; the complete 500 m terrain still fails the natural-cliff target.

## What changed

The previous construction extended every rock cluster to a fixed backing depth. Independent inspection found that 325 of 620 anchors actually moved upward, producing artificial fins. The corrected assembler preserves already-supported original hulls, uses only short downward anchors where necessary, and removes unsupported thin whole fragments. Its irregular exposure follows the union of the actual cluster footprints. The first attempt incorrectly treated internal overlapping polygon edges as exposure boundaries and produced sawtooth skirts; it was rejected. The final union-distance transition fixes that cause. See [construction and rejected candidate](CLIFF-EXPOSURE-BOUNDARY-PASS.md).

Final `JointBoundaryUnion` is one connected closed solid. The extracted visible surface is one valid open component, 14,839 vertices / 29,240 triangles, with all original crop boundary edges and all 7,488 protected guard vertices preserved. The independent audit clears this bounded preview only; it does not certify arena contacts or visual quality.

`Tools/export_cliff_surface_fbx.py` exports the original 3D triangles without reconstructing a heightfield or flattening overhangs. It explicitly supplies constant diagnostic vertex colours because this fractured surface does not yet have valid erosion/flow masks. `Tools/audit_cliff_preview_import.py` now retains all candidates within position-search buckets: genuine submillimetre fracture edges previously collided in those buckets. Unique nearest positions, unchanged residual limits, complete coverage and triangle correspondence are still required; ambiguous coincident source vertices remain rejected.

The optional `-DCCliffPreviewFrameMesh` camera targets the imported mesh's actual bounds. The normal camera and arena remain unchanged.

## Actual native result

Both the previous `LargeMassClean` control and corrected `JointBoundaryUnion` were imported and source-audited, then captured with matched native 1600 x 900 settings at 140 m camera distance. The final asset is `/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector_a80b7df2efe3`, 72 x 40 x 65.133 m, Nanite enabled. Its full source mesh retains all 29,240 triangles; the 1,861-triangle render fallback is not the authored source.

Maximum imported source-position error is 0.0003662109375 cm. All diagnostic RGBA channels match exactly. This proves import fidelity, not scientific material-mask validity.

Retained actual captures:

- [Previous native clay control](review/cliff-context-15nov/native-control.png)
- [Corrected native clay](review/cliff-context-15nov/native-union.png)
- [Corrected outcrop in unchanged 500 m context](review/cliff-context-15nov/context-oblique.png)
- [Context wall view](review/cliff-context-15nov/context-wall.png)

The native result has clearer connected planes and fewer invented fins, but remains angular, pristine and sharply separated from soft surrounding terrain. It is not a finished photorealistic cliff. No textured comparison is claimed: the new surface has diagnostic masks only.

## Full-sector context verdict

The new context helper splices exactly one accepted 72 x 40 m outcrop into the original 500 x 492 x 230.6 m massif. Only the adjacent source triangles are subdivided to match its boundary. Independent checks establish a conforming seam, the original outer boundary and bitwise unchanged retained outside geometry/masks. See [construction and audit](SINGLE-OUTCROP-CONTEXT.md) and [visual review](IMPLICIT-EXPOSURE-TRANSITION-REVIEW.md).

Root and reviewer inspected the actual wall and oblique images. The outcrop is correctly subordinate at this scale, but it reads as a fractured scar on broad soft fluted terrain. Triangular fringes and stepped seams visible in the baseline also remain. **Do not repeat this patch over the arena or promote this context mesh.** The next step is coherent geological structure across a representative sector, not more isolated rock dressing.

## Existing algorithms and next implementation

The current offline macro generator already uses heterogeneous terrain and ridged multifractals, fitted to actual shoreline, seam and ruin constraints. It already has water-limited hydraulic sediment transport/erosion/deposition and conservative thermal movement of loose sediment, with real flow, erosion, deposition, slope and curvature masks. The measured 500 m run only cuts/fills roughly 2.2 m; increasing detail alone cannot repair hundred-metre soft landforms. These are custom prototypes, not Houdini running in the project.

Continue with the established [terrain workflow](CRATER-CLIFF-ALGORITHMS-15NOV.md): author connected escarpments, catchments, buttresses and protected plateau supports at the crater's real scale; coordinate regional rock layers and fracture directions; then erode the fitted shape. Keep genuine 3D exposed faces and controlled recesses rather than projecting them back to a heightfield. Recompute final-surface material masks and blend physically scaled stone PBR, dry fractures, deposits and wet runoff. Historical heightfield erosion masks are not current wetness on a new fracture face. Do not start another generic noise/material pass on the unchanged soft parent terrain.

Finish one representative sector at deck distance and across-crater distance before propagation. Far features stay physically small; long drops and supported temple/statue silhouettes provide scale. Rebuild waterfall contacts and receiving foam from the same accepted final surface afterward. The ship route, 3.2 km crater, 900 m depth, sky/light ownership and deferred boss remain unchanged.

## Verification and limits

Editor and Game builds pass (`Saved/CliffTransitionEditorBuild.log`, `CliffTransitionGameBuild.log`). Five focused original-mesh import tests pass, including the real bucket-collision regression and duplicate-position rejection. Native import/audit logs are `Saved/CliffTransition-{import,audit}-union.log`; the matching source reports are retained under `Saved/CliffTransition/Union`.

Offscreen `cliff12-native-union` captures include active backward seek and final exit: the log confirms terrain/camera restoration at elapsed -4.99 and 181.03 seconds. No keyboard focus was taken. The existing non-Nanite virtual-shadow-map marking warning still appears elsewhere in the scene; this pass does not establish motion quality or performance. No package, main merge or publication.
