# Crater geology and native spray — pass 4

Integration: `codex/maelstrom-saturday`, continuing from `24c0fcb`. Three isolated workers covered cliff geology, water/rock materials and fitted waterfall volumes. All Unreal builds, imports and offscreen reviews were serialized in `Maelstrom-Saturday`. The 3.2 km crater, 900 m depth, route, supported temple positions and integrated sky are unchanged. Boss work remains deferred.

## Concrete changes

- Replaced the enormous projecting cliff shelves with shallower vertical fracture masses and uneven erosion clefts. The underlying wall has a steep central face between a battered foot and shoulder. The corrected low shoreline and all nine waterfall contacts are preserved. [Geometry and actual critique](CLIFF-GEOLOGY-PASS4.md).
- Corrected a native volume coefficient mismatch: UE's installed live-shading path integrates over voxel-local distances, while our material coefficients were expressed per world centimetre. At 50 cm voxels the old optical depth was fifty times too small. The importer now applies the manifest voxel size exactly once, and runtime rejects inconsistent native scales/materials. A zero-emission, nonconstant-density render proves the actual cached volume is visible. [Source audit, diagnostics, cache and critique](WATERFALL-VISIBILITY-PASS4.md).
- Re-exported the revised actual cliff/receiving-water profile and generated a new source-8 cache. Its lower reach is 156.121 m long, down from 230.768 m on the old shelves. The cache contains 96 frames per layer at 24 fps, 0.5 m voxels and 101.72 MiB of original VDB data. It imports successfully and remains opt-in.
- Corrected the basin normal's flow-map chain rule and conversion from planar wave derivatives to slopes on the actual tilted surface. This repairs orientation/stretch, not the overly smooth bank silhouette. [Normal calculation and measured bank geometry](WATER-BANK-REVIEW4.md).
- Removed the verified masonry-producing 22 m material field after matched isolation. Its replacement uses shallow erosion, mineral relief and sparse finite cracks at metre/sub-metre scales, with physical pixel-footprint filtering and early exits for unresolved detail. The saved rock material is regenerated with diagnostics off. [Physical scales and recipe](CLIFF-EROSION-CANDIDATE.md).

## What actual renders establish

The oversized horizontal roofs and black undersides are gone. The wall and water core now descend continuously. The wide deck view still fails the reference: the original material blankets the rim in large rectangular cells, the water has broad smooth banks, and whitewater is poorly concentrated around physical events.

A matched material isolation disables only the 22 m procedural fracture normal and its matching colour/AO contribution. The masonry cells disappear in both close and wide views, confirming that this layer was a principal cause. The exposed geometry is still too smooth; removing a false detail layer alone is not a finished cliff. Fine installed texture normals remain enabled in this comparison.

The fitted VDB now adds a visible cream sheath and tan landing plume at the default `.004/.0015` density coefficients. It still softens a bright attached strip instead of producing convincing detached sheets, fragmented streams and spray. The impact reads as dusty fog. This is a technical visibility/fit improvement, not waterfall visual acceptance. Keep it opt-in and do not multiply it around the crater yet.

The wide 420 view remains dominated by regular small specular ripples over a smooth mean bank. The new normal math does not solve that shape. At ride time 420 the outer recovery rises about 98.5 m over 132.4 m and reaches roughly 50.6 degrees. A geometry pass must address broken wave packets outside the steering corridor while retaining mean height, route and shoreline clearance.

## Reproduce the current fitted volume

The current exported profile is `Crater-Workers/Fitted-Waterfall/Saved/WaterfallProfileGeology4.json`, SHA1 `4E08EA4E6096EB9D73269BC6C8FCA36973DACE1E`. The ignored local cache is `Crater-Workers/Fitted-Waterfall/Saved/GeneratedWaterfallsGeology4`. Older Supported3 data is not fitted to this wall.

Run from the integration checkout, using the profile above or a freshly exported profile from the same source geometry:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_waterfall_vdb.py --profile '../Crater-Workers/Fitted-Waterfall/Saved/WaterfallProfileGeology4.json' --output Saved/GeneratedWaterfallsGeology4 --voxel-m 0.5 --emission-rate 14000 --fps 24 --duration 4
$env:DC_WATERFALL_DIR = "$PWD/Saved/GeneratedWaterfallsGeology4"
$env:DC_WATERFALL_MATERIALS_ONLY = '0'
& 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$PWD/DirectorsCut.uproject" -run=pythonscript "-script=$PWD/Tools/import_procedural_waterfalls.py" -unattended -nosplash -nullrhi
./Tools/review_maelstrom_sky.ps1 -EnvironmentOnly -Clean -Native -Shots '300,451' -ReviewName fitted-falls-review -ExtraArgs @('-DCProceduralFalls','-DCFallFeed=8','-DCFallCamera') -Wait
```

Export a fresh profile after any future cliff/water-contact change using `-DCExportWaterfallProfile=<absolute-json-path>` and `-DCFallFeed=8` during a maelstrom review. Bulk caches and imported SVTs remain ignored. No purchased textures or cached vendor data were copied into Git.

## Checks and remaining work

Editor/Game targets and the four maelstrom engine tests pass. The production-source CPU cliff and contact probes pass; all nine mean-water contacts are within approximately 0.011 cm of the receiving surface. The new cache passes bounds, exact loop/roundtrip, contact and submerged-density checks across all 192 stored grids. Final builds are `Saved/CraterPass4EditorFinal.log` and `CraterPass4GameFinal.log`; engine test evidence is `CraterPass4Tests.log`.

The final integration audit found and fixed a diagnostic backward-seek bug. Hidden cliffs/whitewater follow encounter activity, not the basin's later reveal threshold. A CPU-only `-nullrhi` runtime check seeks 301→250→300→451 and logs both components restored to visible at elapsed -20 while active, then hidden at elapsed 181 after exit; both native volume layers clear at each transition. See `Saved/Waterfall4BackwardSeek.log`. NullRHI cannot supply the installed water plugin's GPU targets, so that run has expected FFT resource errors and is strictly a lifecycle check, not a visual or rendering test.

Actual wide/close and exit captures were inspected. These are still-frame checks; the helper's shot names denote requested seek times, with settling time before capture. They do not prove motion quality or a stable frame rate. Other chats used the GPU earlier in the pass. Existing non-Nanite VSM marking overflow and unrelated Toolset Python/UDS startup messages remain; no claim of error-free global startup or measured runtime performance is made.

Next priorities are localized fractured rock surface detail; attached-to-detached waterfall paths shared by mesh and cache; sharper impact sheets, droplets and advected foam; then broken physical wave packets on the broad outer bank. Do not hide the failures with more continuous fog, more uniform foam or enlarged far-rim texture features. Cloud/sky/light composition remains owned by its separate chat.

An installed-asset audit found `/Game/AncientRuins/Textures/Bake/Cliffs/T_Cliff_01_{D,N,M}` through `T_Cliff_03_{D,N,M}` and matching `MI_Cliff_01/02/03`. These are candidates for inspecting suitable stone detail, not permission to paste mesh-specific atlas seams across the generated cliff. Pixels, resolution, channel packing and tiling suitability have not yet been checked. No vendor files were copied or altered.

## Final material review and retained captures

`Saved/SceneReview/cliff4-erosion-wide` contains the final 300/420/451 views; `cliff4-erosion-close` contains the matching source-8 close view. Both compile/render successfully with the replacement material enabled, and the wide exit restores the scene. The masonry grid is gone. Fine vertical erosion and occasional small cuts are now visible near the waterfall instead of invented metre-high square faces. Keep this bounded correction, but reject the result as finished art: the wall still reads as a softly shaped, brown mass, large-scale rock variation is weak, and the fall/impact remain a bright strip with diffuse dusty spray. The far-view detail now stays appropriately small rather than enlarging to fill the image.

Representative actual images are in [review/crater-pass4-14nov](review/crater-pass4-14nov): `current-wide.png`, `current-close.png`, `current-bank.png`; historical comparisons `rejected-cell-material-wide.png`, `fitted-volume-old-cell-material.png`, and `fracture-layer-disabled-close.png`. The isolated image is diagnostic, not the saved default material. The retained source/material asset uses `DC_ROCK_REVIEW_NO_FRACTURES=0`.

The sky chat completed a read-only critique of the existing images; it made no changes or GPU runs. It agrees that cream horizon/water highlights and near-black deck shadows lack the reference's cool blue-grey separation. A later coordinated light pass should isolate neutral spray/material response from sunlight warmth and cool ambient fill; simply raising exposure, bloom or fog would wash out more detail. It also rejects the dusty plume and smooth cliff as geometry/effect problems that lighting cannot solve. Its sky/cloud ownership is preserved.

The next waterfall composition should eventually use uneven groups of narrow branches within selected catchments, with quieter exposed cliff between them, rather than one enlarged uniform ribbon per sector. First make one source convincingly detach, break and land. That shared trajectory must clear actual rock triangles and reach covered water before its mesh and volume are regenerated together; retain the measured first-contact and clearance checks.
