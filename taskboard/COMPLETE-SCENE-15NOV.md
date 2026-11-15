# Complete maelstrom assembly - 15 November

> Historical: superseded by the [caldera lake and single inlet](CALDERA-LAKE-15NOV.md); this multi-fall vortex assembly is no longer the active design.

The complete-scene assembly uses one connected 360-degree cliff mesh, a unique full-circumference geology material, and six waterfall feeds fitted to that exact mesh and the current production receiving water. The final fall bundle adds 46 detached curtains and 90 crossed low-opacity mist cards; runtime adds 12 local scene-lit impact plumes. The former isolated 497 m pilot is no longer the active terrain/material region: every new vertex has R=0 and selects the full context material. This note records the assembled inputs; native play results and visual acceptance remain pending below.

## Final identities

| Part | Final local artifact | SHA-256 / identity |
| --- | --- | --- |
| Original full-ring triangles | `E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CompleteCliffRing/SurfaceRockBounded/candidate.npz` | `a0ef5da4bd66510de7b0f560ca4d46a6d17de114630c38daa76adb66c612daea` |
| Imported full-ring FBX | `E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CompleteCliffRing/SurfaceRockNative/complete_cliff_surface_rock.fbx` | `6e075ebf11acb93aaefae6b628bc31568ddca94ce21ecf364933f9c3f22b5364` |
| Actual surface fields | `Saved/CompleteCliffFieldsRock/fields.npz` | `c082c63f0df7fc8438ac3bccf5d4be9af33ca3ce44b0ad662228966f040a24af` |
| Full context maps | `Saved/CompleteCliffGeologyRock/tile_manifest.json` | Bound to the full-ring source SHA above; texture hashes are in this manifest |
| Fitted falls bundle | `E:/Projects/DirectorsCut/Crater-Workers/Complete-Falls/Saved/CompleteFalls/DetachedMist` | Profile SHA-1 `2e4cd7831eed1e664bef3d7824408a11d1e9bcce` |
| Fitted falls FBX | `DetachedMist/waterfall_core.foam.fbx` within that bundle | `fc690fae5aaf17b3cba7edbe9d4774755defefc980904e07b1c0d71168e26704` |
| Native-scale detail normal | `E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CliffDetailNormals/NativeMeasuredBC5/T_NordicFace_NormalGL.tga` | `a79918b3ceffde679b7979ab2a02c65fe87d3d928e4d7a7b82ef9aee9138cd3e` |
| Actor catalogue | `Saved/CliffSupportAnchors.json` | `2e89c7506e68390eea305f5cb937261dde2376bd473cb9740c4068d7534772b5` |

Paths beginning `Saved/` in this note are relative to `E:/Projects/DirectorsCut/Maelstrom-Saturday`. The earlier `First`, `FirstJoined`, `SurfaceRock` and falls `Final`/`BroadSurfaceRock`/`BroadContactFine` outputs are superseded and must not be mixed with this assembly.

The source has 863,513 vertices / 1,720,320 triangles, arena XYZ metres, and export origin `(-1072,1154,-25)m`. The nominal crest radius remains about 1.6 km; the outer backing extends beyond that rim. Original toe rows 0-8 and the outer edge are retained, apart from closing the source's duplicate angular endpoint discrepancy of at most 0.378 mm. The joined source has zero seam-position error and no collapsed float32 export triangles. This is authored rock geometry, not a physical erosion simulation. The final `--surface-rock` correction evaluates ~24 m secondary and ~79 m primary planar faces in actual meridian-distance coordinates and displaces along the base wall normal. It replaces the weak radial-only relief on the several-hundred-metre lower apron; the toe collar is 15 physical metres. Protrusions are bounded landward of the existing toe. Native float32 export has zero collapsed triangles and minimum doubled area 0.152 m². Crest/platform coordinates remain unchanged from `FirstJoined`.

## Reproduce the local source and materials

Use the current integrated tools. The commands below name the actual final artifacts; for a replay, choose fresh output directories and propagate those paths through every later step. Generators reject overwriting immutable bundles. The accepted pilot NPZ and the owned scan projection are local input dependencies, not files supplied by Git.

```powershell
$dcRoot = 'E:/Projects/DirectorsCut/Maelstrom-Saturday'
$cliffRoot = 'E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact'
$fallRoot = 'E:/Projects/DirectorsCut/Crater-Workers/Complete-Falls'
$py = 'E:/Utilities/Blender/5.3/python/bin/python.exe'
$blender = 'E:/Utilities/Blender/blender.exe'
$ue = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$dcMesh = "$cliffRoot/Saved/CompleteCliffRing/SurfaceRockBounded/candidate.npz"
$dcFBX = "$cliffRoot/Saved/CompleteCliffRing/SurfaceRockNative/complete_cliff_surface_rock.fbx"
$dcSidecar = "$cliffRoot/Saved/CompleteCliffRing/SurfaceRockNative/complete_cliff_surface_rock.json"
$dcReference = "$cliffRoot/Saved/CompleteCliffRing/SurfaceRockNative/original_mesh_reference.json.gz"

& $py "$dcRoot/Tools/generate_complete_cliff_ring.py" --source "$cliffRoot/Saved/ContinuousScanCliff/Faceted/full_ring_candidate.npz" --tile 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/OwnedCliffProjection/vfmjadufa/UniquePilot/tile.npz' --output "$cliffRoot/Saved/CompleteCliffRing/SurfaceRockBounded" --surface-rock
& $blender --background --factory-startup --threads 8 --python-exit-code 1 --python "$dcRoot/Tools/export_cliff_surface_fbx.py" -- --input $dcMesh --output $dcFBX --normal-angle-deg 60
& $py "$dcRoot/Tools/audit_cliff_preview_import.py" --prepare $dcSidecar --output $dcReference

& $py "$dcRoot/Tools/bake_cliff_geometry_fields.py" --source $dcMesh --output "$dcRoot/Saved/CompleteCliffFieldsRock" --width 4096 --height 1024 --full-ring
& $py "$dcRoot/Tools/bake_cliff_context.py" --fields "$dcRoot/Saved/CompleteCliffFieldsRock/fields.npz" --output "$dcRoot/Saved/CompleteCliffGeologyRock" --width 8192 --height 2048 --fractured

& $blender --background --factory-startup --threads 8 --python-exit-code 1 --python "$dcRoot/Tools/build_complete_cliff_waterfalls.py" -- --cliff $dcMesh --project-root $dcRoot --output "$fallRoot/Saved/CompleteFalls/DetachedMist"
```

The full-ring field bake uses UV bounds `(-7,-1.25)..(13.3,2.35)` and must use `--full-ring`; the old R>0 pilot selection would be empty. Actual position, normal and curvature condition the unique context maps. The context is clamped, with no repeated 500 m basecolor panels. Only the owned 7.9875Ã-5.5125 m detail **normal** repeats at native scale with two orientations. It contains measured high-pass relief and source fine normals, not repeated scan basecolor.

The falls are unequal 38-108 m wide feeds at 18, 52, 215, 248, 323 and 342 degrees. Their recorded drops are approximately 489-511 m. `DetachedMist/attachment.json`, `waterfall_profile.json`, `support.npz`, `receiving_water.npz` and `water_provenance.json` bind them to the unchanged original cliff triangles and fresh production `HeightAt` samples. The final bundle contains 46 detached gravity-driven curtains and 90 crossed, bowed low-opacity impact-mist cards. These supplement the supported runoff and current-aligned receiving foam rather than stretching a single attached ribbon down each cliff. `volume_mode=fitted_mesh_spray` remains explicit: this is authored runoff and spray, not a fluid simulation or reused pilot VDB. Receiving-water samples exclude material WPO.

Root integration `089bc52` contains the detached-curtain/crossed-mist change (worker change `500f2c03...`). The native runtime adds two `ULocalFogVolumeComponent` plumes at each of the six actual receiving contacts: 12 total, attached to `Arena`, scene lit, and non-emissive. Their visibility follows the complete-arena entry/seek/exit lifecycle. These local foot plumes are distinct from the sky/cloud system.

The vortex material source-flow correction is root commit `f2866edf57f8db02dca2d08ba97b803e95850071` (`f2866ed`, "Bound vortex material strain and isolate water diagnostics"). It changes `Tools/make_maelstrom_materials.py` and water-pass notes, not the accepted cliff geometry; regenerate the water materials from that corrected source in the same serialized Unreal workflow. Cliff source/FBX hashes remain `a0ef5da4...` / `6e075ebf...`.


## Import, verify and install

Run Unreal jobs serially with the editor closed. The importers check hashes; use an existing matching verified import or a fresh asset name for a replay. Do not bypass a mismatch or reuse falls from a previous cliff source.

```powershell
$env:DC_CLIFF_PREVIEW_FBX = $dcFBX
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/import_cliff_landform_preview.py" -unattended -nullrhi -nosplash

$env:DC_CLIFF_AUDIT_SOURCE = $dcReference
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/audit_cliff_preview_import.py" -EnablePlugins=GeometryScripting -unattended -nullrhi -nosplash
& $py "$dcRoot/Tools/prepare_cliff_preview_surface.py" --sidecar $dcSidecar --anchors "$dcRoot/Saved/CliffSupportAnchors.json" --saved "$dcRoot/Saved"

$env:DC_CLIFF_GEOLOGY_ATLAS = 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/CliffGeology/FacetedUniqueV3'
$env:DC_CLIFF_GEOLOGY_CONTEXT = "$dcRoot/Saved/CompleteCliffGeologyRock"
$env:DC_CLIFF_GEOLOGY_DETAIL = "$cliffRoot/Saved/CliffDetailNormals/NativeMeasuredBC5"
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/import_cliff_geology.py" -unattended -nullrhi -nosplash

$env:DC_CLIFF_FALL_DIR = "$fallRoot/Saved/CompleteFalls/DetachedMist"
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/import_cliff_preview_fall.py" -unattended -nullrhi -nosplash
& $py "$dcRoot/Tools/install_complete_maelstrom.py" --project $dcRoot
```

The importer still accepts the former pilot atlas as a material dependency, but the new mesh's all-zero R mask selects full context everywhere. The packages selected by these final source hashes are:

- `/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector_6e075ebf11ac`
- `/Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffScanSurface`
- `/Game/DC/Maelstrom/GeneratedCliffPreview/Falls/SM_CliffFall_fc690fae5aaf`

`install_complete_maelstrom.py` checks the imported geometry/masks, FBX/source hashes, current-triangle support binary, full-ring material source and waterfall source before writing `CompleteArena.json` **last**. It copies the verified contracts and triangle binary into `Content/DC/Maelstrom/GeneratedCliffPreview/Runtime`. `Config/DefaultGame.ini` stages that Runtime directory as UFS data. The installer confirms this generated directory is Git-ignored; owned source scans, derived textures, geometry bundles and imported local assets remain outside the public repository.

## Normal-game activation

With the valid installed `directorscut.complete_crater.v1` marker and matching local assets, the complete rim is enabled in the normal game, attached to `Arena` and viewed from the playable ship. It no longer requires `-DCCliffPreview` or `-DCSceneReview`. Ruin support uses current actor footprints against the actual arbitrary triangles. Complete-cliff mode activates for the entry transition; existing seek/exit restoration code owns its lifecycle.

Explicit `-DCCliffPreview` still uses authoring data in `Saved` and requires scene review. `-DCLegacyCrater` disables automatic complete-arena selection. Missing or mismatched complete-scene contracts retain the legacy fallback rather than enabling a partial assembly.

The user-invoked launcher is `Tools/play_complete_maelstrom.ps1`; default start is 295 seconds at the helm. `-FullRun` starts at zero. It requires the installed runtime files and opens an interactive game window only when invoked.

## Native results - pending root

Pending: build result, normal-game start and maelstrom entry, actual 360-degree/ship-view captures, supported ruins, fitted falls in motion, backward seek/exit restoration, and performance. Import/source audits alone are not native visual acceptance.

## Visual acceptance - pending root

Pending root's assessment of the final complete-scene captures. No finished-art, shipping-build, gameplay-balance or publication claim is made by this assembly note.
