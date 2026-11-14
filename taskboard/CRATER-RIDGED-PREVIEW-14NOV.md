# Ridged cliff authoring proof — 14 November, 22:48 continuation

Branch `codex/maelstrom-saturday`, starting at `540c059`. This pass changes offline authoring tools and an isolated ignored preview asset. **The gameplay terrain, waterfalls, route, lighting and boss are unchanged.**

## Outcome

The clean radial-profile variants still looked like a machined dam, even after cutting connected tributaries into their plateaus. Those shapes were rejected in matched CPU clay views. Increasing erosion did not repair their broad slab geometry.

The useful alternative is Blender's built-in **ridged multifractal plus heterogeneous terrain**, evaluated in physical two-dimensional coordinates. It produces diagonal projecting masses, saddles and recesses of different heights instead of repeated full-height slots. A corrected four-octave version adds smaller breaks while retaining the two-octave macro form. This is a better authoring basis, not final AAA geology: the plateau is still flat, some fins remain soft, and true overhangs/fractures are absent.

Actual bounded erosion of that field creates localized changes, rather than the previous wall-wide lower band. It has real flow, sediment and erosion masks. The preferred candidate was exported and imported into Unreal as a **Nanite static-mesh preview**, at the correct physical size. It has not been rendered with the game lighting or stitched into the arena.

## Reproduction and provenance

See [the ridge generator handoff](RIDGED-CLIFF-LANDFORM.md) for exact commands, immutable input hashes, the historical measured geometry dependency and verified numeric regeneration. The raw measured sector came from the rejected candidate at source head `3339c57`, geometry reference `4bb5b0c`; only its shore/plateau/frame constraints are reused in the new interior. Do not silently substitute current source geometry and call it the same proof.

Tracked tools:

- `generate_cliff_landform.py`: clean boundary/seed proposal and rejected catchment variants, documented in [CLEAN-CLIFF-LANDFORM.md](CLEAN-CLIFF-LANDFORM.md).
- `generate_ridged_cliff_seed.py`: actual two-dimensional Blender terrain basis, `--octaves 2..4`. Added-octave centering is for controlled comparison; it is not a general rule forbidding intentional rock mass changes.
- `generate_eroded_cliff_sector.py`: conservative hydraulic/thermal prototype, now with explicit fixed physical boundary outlets. Water and suspended sediment leaving the domain are accounted separately; no end-of-run dump.
- `review_eroded_cliff_sector.py`: matched physical-scale CPU clay, including locked cameras across different seeds.
- `export_eroded_cliff_fbx.py`: centred metre-space FBX with explicit arena frame and linear vertex masks R=log-normalized cumulative flow, G=gross deposition, B=gross erosion, A=1.
- `import_cliff_landform_preview.py`: original FBX import into the ignored `/Game/DC/Maelstrom/GeneratedCliffPreview` folder. It checks the sidecar/hash, expected physical bounds and Nanite setting. It edits no map or runtime actor.

Preferred local FBX:

`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/RidgedPreview/source08_ridged4_detail_open256.fbx`

SHA256: `143c4a6b78e87c2ca0bab1cfccafc1f054997490662eeb996d83135b7d90ec57`. Adjacent JSON records the source/result hashes, full frame and mask divisors. Generated NPZ/FBX/preview assets remain ignored, not in public Git.

```powershell
$env:DC_CLIFF_PREVIEW_FBX = 'E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/RidgedPreview/source08_ridged4_detail_open256.fbx'
& 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$PWD/DirectorsCut.uproject" -run=pythonscript "-script=$PWD/Tools/import_cliff_landform_preview.py" -unattended -nosplash -nullrhi -nosound
```

## Measured validation

- Thirteen erosion tests pass, including fixed heights, invalid-padding exclusion, explicit outflow conservation and exact unchanged closed-mode behavior. Six clean-landform tests pass. Ridge generation checks deterministic arrays, input/fixed-height preservation, finite geometry and no samples above the measured rim.
- Explicit outflow removes 9.45 m of artificial fixed-toe ponding in the earlier clean-seed diagnostic. Water and solid solver budgets remain below `1e-9 m3` error. This fixes the boundary reservoir, not the cliff's artistic shape. The simplified D8 capacity still has roughly 8% directional bias.
- Corrected four-octave actual erosion: maximum net cut 2.27 m, fill 4.34 m. Physical support samples remain fixed, but they do not establish exact compatibility with every current temple footprint or live triangle seam.
- FBX source: 52,536 vertices, 104,078 triangles. Blender roundtrip preserves masks exactly at matched physical corners; maximum corner position error about 23 micrometres.
- UE5.8 import: 500.000039 x 492.0 x 206.131836 m, matching expected size; Nanite build and persisted setting pass. Build log reports approximately 2.18 s for the first Nanite build. `GetNumTriangles(0)` reports 27,045 render/fallback triangles on the final import (27,883 on the initial import), **not** the full source mesh. Do not use that reduced fallback as an exact waterfall/contact surface.
- A preliminary 2 x 3 x 4 m asymmetric unit mesh caught a commandlet issue: `StaticMeshEditorSubsystem` is not instantiated there. The final importer enables Nanite during FBX import and verifies the actual mesh setting directly. Both unit and actual candidate imports pass. Latest log: `Saved/CliffPreviewRidgedImportFinal.log`; report: `Saved/CliffLandformPreviewImport.json`.

Other chats were using offscreen rendering. The initial serialized import slot was deferred; the eventual imports used **NullRHI only**, concurrently with their independent jobs, and touched only this checkout's ignored preview assets. No other process was stopped and no focus was taken. No new game C++ or default material changes required another full game build. NullRHI import is not a GPU render, runtime performance test or in-game visual approval.

## Visual evidence and next gate

The full CPU reviews are in the waterfall worker's `Saved/CliffLandform`: `CleanOpen256Review`, `CatchmentSeedReview`, `RidgedSeedReview`, `RidgedOpen256Review` and `Ridged4DetailOpen256Review`. [The review handoff](CLIFF-CLAY-REVIEW-PASS6.md) records each rejection and the qualified acceptance for isolated preview. The same cameras, raking light and physical scale are used; there is no texture/displacement shader or vertical exaggeration.

Retained examples are in `taskboard/review/crater-pass6-14nov`: `rejected-clean-dam.png`, `ridged-clay.png`, `ridged-four-eroded-clay.png`, and `ridged-four-deck-clay.png`. These are geometry authoring renders, **not gameplay screenshots**.

Next: render the imported candidate in an isolated Unreal review under a known camera and the current scene lighting, verify arena orientation and actual UE vertex-mask values, and establish a single authoritative mesh/query surface before replacing a sector. The current procedural cliff/whitewater/landmark code assumes a 1536 x 160 radial grid. A new static mesh cannot simply be displayed while those queries keep using the old wall. Use exact source geometry for edge stitching and contacts, retain the safe water boundary, and recheck temple support. Then build real fractured faces/overhangs and material variation on the accepted masses. Extend around the rim only after that one sector is convincing.

The automation remains active; its prompt was updated to follow the clean-landform direction and current handoffs. No main merge, packaged build, purchase or boss work.
