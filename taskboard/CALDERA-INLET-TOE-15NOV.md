# Caldera inlet shoulder correction — 15 November, 16:15

Review checkout: `E:/Projects/DirectorsCut/Maelstrom-Saturday`, branch `codex/maelstrom-saturday`, based on combined main `3368536`. **Player-view verification and the unchanged overview rerun now pass. Ready for local integration review.** The first overview's shutdown crash remains documented below; its root cause is unresolved. Main and its packaged playable have not been changed.

## Result and scope

The pointed gray patch near the ship was distant rock protruding beside the receiving river, not the adjacent hull bed. [Exact native camera rays](CALDERA-NAVIGATION-BED-15NOV.md) identified two original shoulder faces 220–247m away, 24–26m outside the narrowed river. Increasing early impact-fog density did not visibly hide them; that trial was reverted and both binaries rebuilt with the original fog settings.

`Tools/round_caldera_inlet_toe.py` instead lowers only the two finite lower shoulder tips. It starts from the original accepted source, **not the rejected WetFoot trial**. The tapered correction changes 2,206 vertices / 4,773 triangles, within radius940–1060m and absolute river-across167–245m. Maximum lowering is28.29m; the full-weight core ends5m below the mean lake. XY, topology, UVs, masks, circumference seam, upper banks and every triangle touching the protected wet corridor remain exact. This changes the local shoreline deliberately, so shoreline fog contacts and source/support data were regenerated.

- [Before: pointed shoulder beside the port stern](review/caldera-inlet-toe-15nov/before.png).
- [After: continuous visible water at the same native player shot](review/caldera-inlet-toe-15nov/after.png).
- [Current overview from the successful unchanged rerun](review/caldera-inlet-toe-15nov/overview.png).
- [Current overview, captured before failed process shutdown](review/caldera-inlet-toe-15nov/overview-shutdown-failed.png).

These are unedited native1600x900 captures, not generated concepts. The overview still shows broad sculpted banks, uniform river width and visible impact-fog lobes. This is one defect correction, not final reference-quality art. Water, route, boss, sky and boulder placement were not changed.

The low bank veil retains160 volumes of46m radius, radial extinction0.09, height extinction0.28 and shallow falloff500. New contacts cover9,130 actual source shoreline segments with a40.878m conservative coverage bound. Six volumes intersect the nominal928m orbit and ten intersect the outer1038m steering circle; opacity remains shallow. Total fog budget remains253/256. The nine dense waterfall-foot volumes retain their original density and footprint.

## Exact local deliverable

| Item | Identity |
| --- | --- |
| Source | `Saved/CalderaInletToe/Rounded/candidate.npz` |
| Source SHA256 | `4e8340ee57a6e36d550a0b41930e35a1fc82b8cddacd914614fa126156f01f6d` |
| Immutable parent SHA256 | `6eac5bba828f84ef2196f3a6598c23829c503722eb056f7ad59269bd6ab444e7` |
| FBX | `Saved/CalderaInletToe/RoundedNative/caldera_inlet_toe.fbx` |
| FBX SHA256 | `5e32bbed5853fe00c7d9d90d6b1e3da9ce1939184f8e9036ff2b6246a4bf60da` |
| Ignored Unreal mesh | `/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector_5e32bbed5853` |
| Shore contact SHA256 | `750e69e19a7e0f096666734738382b149648902951a44c4b65498c36b40e007a` |

All paths above are relative to the review checkout. The generated mesh and its seven matching runtime files are frozen under `Saved/CalderaInletToe/Handoff/`, with sizes and SHA256 values in `files.json`. They belong under `Content/DC/Maelstrom/GeneratedCliffPreview/` and its `Runtime/` subdirectory respectively. Existing `M_CliffScanSurface` and texture dependencies remain required. Do not copy the whole Content folder or the four pending VFX material edits. Do not publish owned scan-derived files to Git.

This bundle is now ready for local integration review, not automatic release promotion. Copy its runtime files as one set; do not mix the old source marker with the new mesh. `CliffPreviewWaterfall.json` remains historical data; perimeter waterfalls are disabled by `CompleteArena.json`. The old installed mesh and exact Saved/runtime rollback are preserved under `Saved/CalderaInletToeRollback/`.

## Verification and limits

The CPU generator reports zero flipped/degenerate/native-float32-collapsed triangles and exact seam continuity. All four native pixel rays become water-first; the two faulty rays now hit terrain8.77m and9.06m behind water. All56 original boulder anchor samples remain supported with zero height change. All50 recorded landmark footprints lie outside the changed geometry; the nearest starts at1477.756m versus1064.148m for changed triangles. This proves unchanged underlying source geometry, not identical runtime actor selection or whole-scan footprint clearance. Native verified landmark counts were39 before and40 after; no explanation for that variation is asserted.

The actual imported SourceModel audit checks863,513 vertices and1,720,320 triangles, connectivity and masks, with maximum position error0.03711cm. Nanite import, supports, shoreline generation, installer content preflight and restored-default Editor/Game builds pass. Evidence lives in `Saved/ImportCalderaInletToe.log`, `AuditCalderaInletToe.log`, `PrepareCalderaInletToeSupports.log`, `InstallCalderaInletToe.log`, `BuildCalderaMistRestoreEditor.log` and `BuildCalderaMistRestoreGame.log`.

`CalderaRoundedToe` exited successfully. Actual player captures at285/294/315 show the revised scene; open sea463 and backward seek275 restore. These are the current source baseline's timeline, not guaranteed equivalents in newer main. No whole-route dynamic collision, long-run performance or packaged-build acceptance is claimed.

`CalderaRoundedToeOverview` captured at actual316, 16:08:11, then requested normal exit and logged shutdown. Windows Application Error1000 independently records PID42028 crashing at16:08:14 with `0xc0000005` in `UnrealEditor-Core.dll`; the harness rejected that run. Earlier UE crash folders contain handled startup MTAccessDetector ensures and rendering continued afterward. Their relationship to the final crash is unproven. **Keep this image as visual evidence but do not call the overview a clean pass.** No speculative renderer or geometry fix is justified by the shutdown stack alone.

At16:17, after the ACE chat explicitly released the GPU, `CalderaRoundedToeOverviewRecheck` repeated identical content, settings and camera. It produced the fresh actual316 overview at16:17:40, verified40 landmarks and exited0 at16:17:42, without the earlier handled ensures or fatal/material validation failures. This satisfies the requested clean repeat; it does not establish a fix or cause for the earlier shutdown crash. The GPU was immediately released again before ACE's final packaged check. No extra build, material, geometry or fog edits occurred between these two overview runs.

## Reproduction

Use a fresh output directory; the generator refuses to overwrite an existing candidate:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/round_caldera_inlet_toe.py --output Saved/CalderaInletToe/Replay
```

Then use the established export/import/source audit pipeline with the new candidate and its generated manifest, regenerate support data and shoreline contacts against its exact source/FBX hashes, and run `Tools/install_complete_maelstrom.py --caldera`. The frozen sidecar and `original_mesh_reference.json.gz` are beside the FBX. Existing parent-source bank install commands in `CALDERA-LAKE-15NOV.md` are historical and must not silently overwrite this revision. The diagnostic camera-ray hook is optional and does not affect default rendering.
