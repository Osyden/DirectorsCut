# Eroded cliff: actual engine preview and next shape correction

14 November, continuation from `bba948a`, branch `codex/maelstrom-saturday`.

The user asked whether realistic mountain algorithms and terrain textures can be adapted to the crater. Yes: the useful pipeline is a designed macro landform, ridged/heterogeneous terrain, bounded hydraulic and thermal erosion, terrain-derived material masks, then actual three-dimensional fracture where needed. Shape constraints must precede the final erosion pass so drainage follows the finished landform. The current prototype demonstrates this pipeline technically, but the engine images **reject its current shape as finished geology**.

## What changed

- Added `Tools/make_cliff_preview_material.py`, generating original physical-scale procedural stone only in the ignored preview folder. No vendor pixels, emission, vertex displacement, runtime water material or lighting changes.
- Added `Tools/export_cliff_query_mesh.py`, `DCCliffQueryMesh.h`, a real-header CPU test and [query contract](CLIFF-QUERY-MESH.md). This uses the original ABD/ADC source triangles, not the old polar BC grid or Nanite fallback. It is not wired into gameplay yet.
- Added [read-only import audit](CLIFF-IMPORT-AUDIT-PASS7.md) and `Tools/audit_cliff_preview_import.py`. Actual UE SourceModel geometry, orientation and all corner masks now pass.
- Added `DCMaelstromCliffPreview.cpp` and small actor hooks. `-DCCliffPreview -DCSceneReview` opens a close, isolated view of the imported tile under the current maelstrom lighting. `-DCCliffPreviewRock` selects the new material; otherwise it uses clay. The hook reads the matching import/audit reports for the placement frame, including the FBX Y reflection. It hides the old cliffs, falls and landmarks in this authoring view. **It does not replace the live arena or claim waterfall/temple integration.** Normal gameplay is unchanged.
- Fixed the preview's active backward-seek restoration found during review. The preview requires scene-review mode and excludes the waterfall camera/volume diagnostic flags.

## Actual renders and critique

Actual offscreen native 1600x900 Unreal captures:

- `Saved/SceneReview/cliff7-clay-a/shot_0300.png`
- `Saved/SceneReview/cliff7-rock-a/shot_0300.png`
- Exit captures in both folders at `shot_0451.png` restore open sea.

Retained examples: `taskboard/review/crater-pass7-14nov/isolated-ue-clay.png` and `isolated-ue-stone.png`. These are engine authoring views, not final gameplay compositions. Existing atmosphere/water remain visible, but legacy terrain/falls/landmarks are deliberately hidden to expose the tile. The current warm light also warms the neutral material; no lighting values were edited here.

The rock material does not solve the geometry. Visible two-metre sampling triangles, the uninterrupted smooth cap and continuous apron still dominate. The interior has useful unequal diagonal masses, but these are trapped between artificial boundaries. The tile's open sides and stepped shoreline are also not stitched into the live arena. This is a successful import/render proof and an unsuccessful final cliff design.

The source audit explains the persistent dam shape: `generate_cliff_landform.py` fixes the entire plateau and lower apron, while `generate_ridged_cliff_seed.py` forces relief to zero at both ends and restores those boundaries. Only about 31% of the physical samples are free to change. Erosion cannot cut the protected cap or interrupt the protected skirt. More octaves, mask contrast or stronger erosion inside the same constraints will not fix the silhouette.

## Next implementation: release the artificial boundaries

1. Protect **specific constraints**: valid domain/seams, explicit outlets, the first 4–6 metres of receiving shoreline, and real temple footprints with small margins. Do not protect the whole plateau/apron. Export authoritative current temple footprint data before claiming their support; an unanchored version remains authoring-only.
2. Generate connected geology across the face and 100–150 metres behind the crest. Use broad heterogeneous terrain around 200/100 metre scales with restrained 20–35 metre relief, plus the useful 96/48/24/12 metre ridge structure. Fit constraints with a smooth two-dimensional correction instead of a radial ramp whose detail vanishes at both ends. These are starting art parameters to review, not validated final dimensions.
3. Cut unequal saddles and channel heads into the hinterland, retaining low temple promontories against the sky. Interrupt the talus at the foot. Judge the silhouette and drainage in locked physical-scale clay views before adding detail.
4. Erode that connected field through the existing explicit outlets. Add sparse real joint cuts to the accepted steep masses; suggested first trials are 4–12 metre spacing and 0.3–1.2 metre recesses. Heightfields cannot supply overhangs: those require a later true 3D surface. Replace blanket flat shading with controlled normals that preserve geological breaks without displaying every sample triangle.
5. Re-render one sector under the actual game lighting. The shape must read convincingly without its material before expanding around the rim. Retain physical feature scale at distance, rather than enlarging distant noise.

## Integration after visual acceptance

Use one arena-space surface provider reporting coverage, height, normal and source triangle. Keep separate legacy polar-BC and new rectangular-ABD/ADC backends. Clip/weld the actual render meshes at the same explicit tile boundary; overlapping hidden geometry or query-only precedence is not an integration solution.

Convert waterfalls to arena-XY paths sampling that provider. Regenerate core geometry, detached trajectories, first receiving-water contact, foam starts and VDB profiles from the same mesh hash. Move temple footprint queries to the same provider. Receiving-water coverage remains a separate query. Arena-local height queries do not represent a world-vertical ray when the ship-centred arena is pitched, and they cannot represent overhangs.

## Verification and limits

- Actual source audit: 52,536 vertices, 104,078 triangles and 312,234 corner colours checked; maximum imported position error 0.0024414 cm. Imported axes are `100*(X,-Y,Z)`. Numeric masks match within `1/255`; the source-storage gamma conversion is reversed by GeometryScript and the render-build path. No exporter colour correction was needed. GPU Nanite attribute interpolation has not been measured independently.
- Worker CPU query test checks the actual header against 208,156 source triangle samples, all 52,536 used vertices and 21,922 missing-triangle centres. Maximum height error 1.82e-10 cm. This proves source sampling, not runtime integration.
- Final Editor and Game builds pass: `Saved/CliffPreviewEditorBuildFinal.log`, `CliffPreviewGameBuildFinal.log`. The new material rendered without a material compile failure. Existing unrelated UDS editor-utility warnings remain in the game log.
- Actual rock sequence `300,265,300,451` proves active backward seek: log at elapsed -4.97 restores `active=1 cliff_visible=1`, then reactivates the preview, and exits at elapsed181 with terrain hidden and the camera restored. Capture labels are requested seek times; screenshots occur after settling. The 265 still is taken after reactivation at elapsed0, so the **log**, not that still, establishes the negative-time restoration.
- All five maelstrom engine regressions pass in `Saved/CliffPreviewAutomationFinal.log`: cliff continuity, cliff/water contact, detached trajectories, first receiving-water contact, and route/steering. No packaged build, main merge, boss, cloud/light or early-game work. Own review branch only. These short views are not motion-quality or long-run performance approval.

Reproduce after the ignored source asset/import audit exists:

```powershell
./Tools/review_maelstrom_sky.ps1 -EnvironmentOnly -Clean -Native -Shots '300,265,300,451' -ReviewName cliff-preview -ExtraArgs @('-DCCliffPreview','-DCCliffPreviewRock') -Wait
```

The final guard for conflicting review flags was compiled after these captures; it does not change the valid single-preview render path. The earlier [source/erosion provenance](CRATER-RIDGED-PREVIEW-14NOV.md) and immutable generated-file hashes remain authoritative. Generated assets/query bulk stay in ignored folders.
