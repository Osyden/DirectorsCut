# Caldera lake - 15 November

The user's latest direction replaces the spiral bowl and perimeter falls with a large lake, ordinary waves and one foaming downhill inlet from the tunnel. The complete rocky rim remains. The ship descends once, then circles on level water. Bank boulders, light shoreline mist and gathering mist behind the descending ship support this simpler layout. Boss polish remains deferred.

## Current assembly

The lake is at **-400 m**, with nominal crater radius **1,600 m**. The 440 m wide inlet follows `angle=pi-.40*(1-smoothstep(.58,.985,R/Radius))`, descending from height 0 near radius 1,576 m to lake height at radius 928 m. Both the water strip and excavated corridor now use that same polar cross-section.

The route reaches radius 928 m over encounter seconds **-8 to 32**, then holds it. Orbital velocity eases in during seconds 24-32 and settles at `.009 rad/s` (**8.35 m/s**). A 33.4 m tangential lead avoids stopping at the join. There is no further spiral descent or final throat. Route integration: `50d48ee`; initial entry-cut integration: `9a70929`.

The initial `CalderaEntry` mesh was an intermediate: it cut the upper rim but left much of the slope unsupported, and 955 original toe columns ended above the new lake. The final `CalderaSubmergedToe` extends terminal row 0 to at most **-445 m**, retaining upper faces, XY, connectivity and UVs. Runtime `DCMaelstromInlet.cpp` adds the missing connected landform: a bed beneath the cataract, irregular banks and submerged flanks/foot, plus **68** locally installed Nordic scan accents at **0.85-1.65x** scale with buried source aprons. These are bank accents, not stacked cliff courses.

The cataract's slope mask now fades to zero at the flat foot, sharing the lake's exact wave coordinates there. Only the overlap *below* radius `.58*Radius` sinks under the lake. This removes the mismatched flow/shader join; the calm lake has wind drift rather than vortex mapping. Rebuild with the **water-only** material script below.

There are **16 light shoreline fog volumes**, sampled from the 32 final toe anchors, and **8 slope mist volumes** gathering over encounter seconds **26-42**, after the ship descends. Eight soft EasyFog cards add visible drifting mist above the slope; two smaller foot volumes cover the contact. Near mist takes priority within the renderer's 32-volume budget. The ending uses four local fog banks and a matched camera fade from ride seconds **440-457**: engulf before the scene handoff, clear into open sea afterward. Exit state resets on seeks; the old exposure white flash is suppressed, including direct late seeks. Old perimeter waterfall bundles are disabled.

## Final local assets

The final source is `E:/Projects/DirectorsCut/Crater-Workers/Complete-Water/Saved/CompleteCliffRing/CalderaSubmergedToe/candidate.npz`, SHA-256 **`e4d85d005c5a0d47885ead72b5216242bbc810d332c08d5ba52be35c74e2d5e1`**.

The final export is `E:/Projects/DirectorsCut/Crater-Workers/Complete-Water/Saved/CompleteCliffRing/CalderaSubmergedToeNative/complete_caldera_submerged.fbx`, SHA-256 **`f574f84779f78677938dda0b38ccd9dfddc97c1cbb4f160e387a10a63bf6e3c0`**. Matching JSON/NPZ and `original_mesh_reference.json.gz` are alongside it. The imported mesh is `/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector_f574f84779f7`.

The source retains **863,513 vertices / 1,720,320 triangles**, arena XYZ metres, identity basis and export origin `(-1072,1154,-25)m`. The generator lowers 1,469 terminal vertices; upper rows and UVs are unchanged. The previous `SurfaceRockBounded` source (`a0ef5da4...`) remains the explicit ancestor of the retained surface-paint maps. Those maps are not rebaked geology for the new lower skirt. Actual ruin supports must be regenerated against the final mesh. Owned scans, textures and generated assets remain local and Git-ignored.

## Replay and install

The following names identify frozen outputs. For regeneration choose fresh output folders and propagate them throughout. The first cut's local input remains required; run `Tools/cut_caldera_entry_river.py` from `SurfaceRockBounded` if it is absent. Run Unreal jobs serially with the editor closed.

```powershell
$dcRoot = 'E:/Projects/DirectorsCut/Maelstrom-Saturday'
$cliffRoot = 'E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact'
$waterRoot = 'E:/Projects/DirectorsCut/Crater-Workers/Complete-Water'
$py = 'E:/Utilities/Blender/5.3/python/bin/python.exe'
$blender = 'E:/Utilities/Blender/blender.exe'
$ue = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$mesh = "$waterRoot/Saved/CompleteCliffRing/CalderaSubmergedToe/candidate.npz"
$fbx = "$waterRoot/Saved/CompleteCliffRing/CalderaSubmergedToeNative/complete_caldera_submerged.fbx"
$sidecar = "$waterRoot/Saved/CompleteCliffRing/CalderaSubmergedToeNative/complete_caldera_submerged.json"
$reference = "$waterRoot/Saved/CompleteCliffRing/CalderaSubmergedToeNative/original_mesh_reference.json.gz"

& $py "$dcRoot/Tools/extend_caldera_cliff_toe.py" --source "$cliffRoot/Saved/CompleteCliffRing/CalderaEntry/candidate.npz" --output "$waterRoot/Saved/CompleteCliffRing/CalderaSubmergedToe" --toe-height-m -445 --feather-rows 1
& $blender --background --factory-startup --threads 8 --python-exit-code 1 --python "$dcRoot/Tools/export_cliff_surface_fbx.py" -- --input $mesh --output $fbx --normal-angle-deg 60
& $py "$dcRoot/Tools/audit_cliff_preview_import.py" --prepare $sidecar --output $reference

$env:DC_CLIFF_PREVIEW_FBX = $fbx
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/import_cliff_landform_preview.py" -unattended -nullrhi -nosplash
$env:DC_CLIFF_AUDIT_SOURCE = $reference
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/audit_cliff_preview_import.py" -EnablePlugins=GeometryScripting -unattended -nullrhi -nosplash
& $py "$dcRoot/Tools/prepare_cliff_preview_surface.py" --sidecar $sidecar --anchors "$dcRoot/Saved/CliffSupportAnchors.json" --saved "$dcRoot/Saved"
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/make_maelstrom_water_only.py" -unattended -nullrhi -nosplash
& $py "$dcRoot/Tools/install_complete_maelstrom.py" --project $dcRoot --caldera
```

Do **not** run the full `make_maelstrom_materials.py` replay: it would also rebuild independently edited spray/cascade/mist materials. The water-only wrapper rebuilds just `M_MaelstromWater`. Retained rock material inputs are described in the historical [complete-scene note](COMPLETE-SCENE-15NOV.md); its perimeter falls are superseded.

The installer verifies mesh/source audit, actual support binary and retained material ancestry, stages Runtime data, and writes `CompleteArena.json` last with `caldera_lake=true`, `lake_height_m=-400`, `waterfalls=null` and normal-game activation enabled. Caldera installation/launch no longer read, copy or require the obsolete falls JSON; legacy mode still requires it. After validation, `Tools/play_complete_maelstrom.ps1` opens the playable ship view; `-FullRun` starts at the beginning. No isolated preview flag is needed.

## Validation and remaining review

CPU source checks report no collapsed native-float triangles, no reversed affected normals and zero angular seam error; all final toe vertices are below -445 m. The route's inverse-angle and velocity-join checks pass. These do not establish visual quality or full dynamic collision safety.

Editor and Game builds pass with the lake, inlet, shoreline closure and exit fog. Actual imported geometry and vertex-mask audits pass for the final FBX; the complete arena installer passes and normal gameplay activates it without preview flags. The native normal-game `CalderaFogExitFinal` run captured fog gathering at ride 445, full coverage at 448 and 451 (including the 450 scene switch), clearing at 454, open sea at 458 and a successful backward seek to 275. No fatal errors or failed maelstrom shader compilation. Existing unrelated UDS editor-utility / editor-toolset startup warnings remain.

`Play Caldera.cmd` opens the local Unreal game at ride 265, before the reveal; `Tools/play_complete_maelstrom.ps1 -FullRun` plays from the beginning. This is not a newly cooked standalone release. Generated cliff/material/runtime content and both owned `LocalScans` folders must be copied or regenerated on the integration machine; Git does not contain those assets. The two scan directories are explicitly included in packaging. Preserve the newer crew HUD/names/voice changes from main when merging this branch.

Visual limits: lower cliff extensions stretch the retained rock maps, the inlet remains a broad authored ramp, and boss attacks/placement have not been retuned for the lake. These changes resolve the unsupported geometry and simplify the encounter space; they do not establish AAA visual quality or final boss balance. Frame samples are not a sustained performance benchmark.

Final actual captures: [descent](review/caldera-15nov/inlet-descent.png), [inlet mist after descent](review/caldera-15nov/inlet-after-descent.png), [ending fog gathering](review/caldera-15nov/exit-fog-gathering.png), [open sea restored](review/caldera-15nov/open-sea-restored.png). The inlet images use an external inspection camera; the ending images use the playable helm camera. `CalderaMistDelivery` verifies eight loaded EasyFog cards with soft masks and zero emissive; they cover parts of the slope after descent while leaving the central lake readable. Both final build targets pass (`BuildCalderaMistFinal*.log`).
