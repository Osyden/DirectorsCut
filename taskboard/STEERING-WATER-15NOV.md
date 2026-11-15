# Carried steering and water phase, 15 November

Source handoff from `codex/inertial-steering-polish`, based on main `f11a89f`. No Unreal build, material generation, native launch or GPU work was performed by this agent. Root owns those checks and packaging.

## Behavior

`DCSteering.h` carries velocity and acceleration through helm release and reversal. Braking-distance anticipation progressively steers inward at channel margins, including when the player keeps holding outward. Live treadmill motion and navigation forecasting call the same helper. Ship yaw follows the carried turn (up to 10.8 degrees) with modest heel, independently of whether the wave sample happens to be zero. Slide/caldera transforms and route controls are unchanged.

The plugin water previously sampled the ship stage's world XY while scenery moved by negative lane offset. `DCOcean::WaveSamplePosition` adds the travelled lane to CPU wave queries. `DCWake` uses it; the parallel pillar contact helper must also use it. Place effects at their original stage positions, not at shifted query positions.

`Tools/patch_drw_steering.py` makes the matching material correction using existing `MPC_Sea.Trough.x == -LaneOffset`. It patches shared legacy UV sampling, world-cascade displacement, normals and foam, plus foam albedo/normal texture mapping. All ocean LODs use these shared functions. Physical depth, hull masking, shoreline and mesh stitching positions remain unchanged. Custom non-plugin sea already uses the correct lateral `Scroll` coordinate.

## Required root integration

Do not package the CPU query change without generating its material counterpart. From the main checkout, with Unreal jobs stopped:

```powershell
& E:/Utilities/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe Tools/patch_drw_steering.py --backup
& E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe E:/Projects/DirectorsCut/DirectorsCut/DirectorsCut.uproject -run=pythonscript -script=E:/Projects/DirectorsCut/DirectorsCut/Tools/patch_drw_steering.py -nullrhi -unattended -nosplash
```

The second command writes four installed DRW functions and recompiles/saves six dependent ocean materials. Expect `DC_RESULT steering wave mapping patched across all ocean LODs` with coverage counts 1/2/1/2. Unexpected graph coverage fails explicitly. Original assets and before/after SHA256 hashes are kept in `Saved/OceanSteeringPatch`; preserve that manifest with build evidence. Re-running succeeds without adding another offset. If generation fails midway, restore before retrying:

```powershell
& E:/Utilities/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe Tools/patch_drw_steering.py --restore
```

Restore also precedes rerunning any older DRW vendor-material patch. This script preserves the current installed baseline, including prior foam/trough edits. Vendor binary assets are not committed. A fresh machine needs the baseline plugin modifications and this generation step before cooking.

Then native-build, run `-DCSkipIntro -DCStation=helm -DCHelmSequence -DCLaneLog`, inspect water near both hull and far LOD boundary while steering both ways, and run `-DCNavigationVerify` to check actual scenery paths with the new momentum. Review the sustained edge input with `-DCSteer=1`. Recook into a new package; preserve the prior playable.

## CPU verification

`Tools/probe_steering.py` compiles the actual production helper with MSVC and passes 11 behavioral checks: gradual start, carried release, braking before reversal, eventual reversal, settling, inward recovery, port/starboard symmetry, frame-rate variation, forecast/live agreement, visual turn sign and consistent world-fixed wave coordinates. With sustained outward helm plus 120 cm/s current, maximum position was 4450.98 cm inside a 5000 cm boundary. The 30/60/144 fps position spread was 0.7115 cm. Python patch syntax and `git diff --check` pass.

Material graph execution, rendered wave/query agreement, native navigation and gameplay feel remain for root's integration review; CPU algebra does not establish those visual results.

Native patch follow-up: UE 5.8 blocks Python reads of named-reroute `Declaration`. The script now uses public `ObjectExporterT3D` / `AssetExportTask` serialization to resolve that reference against the function's expression list. UTF-8 and UTF-16 exports are supported; unresolved references fail explicitly. CPU checks cover qualified and relative references, exact-name matching and rejection of unresolved declarations. Originals were restored after the first failed run; root should rerun the corrected script before cooking.

Navigation follow-up: the old test's pillar at 4.5 seconds measured from ship center gives only 1.78 seconds before the padded bow contacts it at the observed 937.5 cm/s. CPU sweeps using the production inertia confirm every destination is blocked. Preserve that as an impossible-escape assertion; place the reachable case by 3.25 seconds from bow contact instead. Its center and starboard crossing remain blocked, while the narrow port escape clears. The production planner now immediately replaces a swept-blocked held route with a swept-clear alternative instead of wasting up to 2.4 seconds of the existing six-second OpenSea/Fog forecast. Preference-only course changes retain their hold/debounce. The native verification asserts an immediate safe switch with at least three seconds remaining, and resets acceleration between scenarios. The expanded CPU probe passes 18 checks, including the reachable route at 1500 cm/s fog speed. Root must rerun native collision verification after integration; no engine jobs ran in this worktree.

Animated-menu audit: `TimeScale=0` previously stopped forward scenery movement but left lateral inertia, avoidance/current, incoming projectiles and hostile timers ticking. Treadmill now holds all of those and spawning while continuing to initialize the ocean MPC parameters; the independent DRW simulation remains live. Crew can finish initial deck placement/station walks, but the open-sea menu has no enemies, fires or encounter triggers at RideTime zero. Voice input is already menu-gated. Separate finding passed to root: Wake's `RideTime<0.5` build guard prevents menu wave samples unless explicitly allowed during the title screen. Root should verify with a long menu wait and `-DCSteer=1 -DCMenuAutoStart=12 -DCLaneLog`: the first voyage lane remains near zero and then accelerates normally. No native run performed here.
