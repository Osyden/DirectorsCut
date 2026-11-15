# Cannon threat and tentacle vulnerability cues

Owner: cannons_helm, `codex/combat-cues`, based on main `1f4fac8`. Root owns the editor/build/GPU slot and the user is testing the frozen 18:33 package. No Unreal job was launched in this worktree.

## Presentation

`UDCCombatCueComponent` builds small depth-tested, collision-free meshes using a texture-free additive material. Hall charge cues are camera-facing amber exclamation marks with a thin broken sea-green/amber halo. The charge arc grows using the encounter's existing charge progress. Only the real charge state is visible; no permanent enemy markers. `tunnel_boss` owns and integrates the hall hook in `DCEarlyEncounters.*` alongside its animation fixes.

Boss vulnerable tentacles gain four soft-edged gold bands and thin sea-green rims around the existing joint. The band follows the exact target center/axis and real damage window. It disappears immediately on interruption and with the target on cancellation, strike, severing, seek, death or cleanup. A surviving first hit briefly lifts the band brightness. The existing center target is toned down; mouth charging keeps its existing look. Radius breathes by at most 2.5%, glow stays bounded, and no point lights or forced depth override are introduced.

All individual cannon, reload, hit radius, damage and attack timing code remains unchanged. The new component cannot collide, overlap, affect navigation or cast shadows. It creates a mesh section once per topology and updates vertices thereafter. The alert component must use a body-only or pre-cue cached head bound: including its own bounds would make the marker climb itself. This was coordinated with the hall agent.

## Required material generation and integration

Integrate this commit before the hall animation/cue-hook commit. Root should run:

```powershell
& E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe E:/Projects/DirectorsCut/DirectorsCut/DirectorsCut.uproject -run=pythonscript -script=E:/Projects/DirectorsCut/DirectorsCut/Tools/make_combat_cue_material.py -AllowCommandletRendering -unattended -nosplash
```

Expected log: `DC_RESULT combat cue material generated: depth-tested, additive, no texture dependencies`. Script is idempotent and writes only `/Game/DC/Materials/M_CombatCue`. Packaging preflight now requires that asset; `/Game/DC` is already included in the cook. No vendor material changes.

## Verification

`Tools/probe_combat_cues.py` compiles the actual geometry builder extracted from `DCCombatCue.cpp`, the production cue policy and `DCBossTimeline` using a small CPU-only math/container shim. It verifies valid nondegenerate triangle indices, soft edge opacity, bounded glow/scale, hall concealment/release/defeat states and the exact boss start/strike/deadline boundaries with interruption/cancellation/target removal. Passed. Python syntax and `git diff --check` passed. This does not substitute for Unreal compile/render checks.

When the user's GPU session ends, request root's visual slot for normal cannon-camera hall charges and both boss obstacle/slam windows. Confirm the glyph sits just above the head, is readable in fog without glowing white, does not reveal hidden enemies, and hides on release/death. Confirm the joint band hugs the actual target and only the vulnerable limb; first-hit flash, second-hit sever, untouched strike and seeking/death should all clear correctly. Run existing cannon/encounter/boss verification unchanged, then package a fresh candidate for cooked-material/lifecycle review. No changes to the selected 18:33 package.
