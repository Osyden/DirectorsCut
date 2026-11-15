# Boss band contrast candidate

Owner: cannons_helm, branch `codex/boss-band-contrast`, from main `f31baab`. Pending candidate only. The human is testing the accepted 19:24 package; no editor, build, import or GPU job ran here, and that package/main were not changed.

## Diagnosis and change

The packaged active-slam frame `shot_0342.png` does not establish readable wrapping contrast: the old gold target, reticle and foreground rigging obscure the small cue. Source inspection finds no missing transform/material connection. The procedural tentacle's nominal joint radius is 130.5cm, at most 161.8cm with sucker relief, versus the 180cm band. That makes complete burial at its center unlikely. Curved neighboring mesh and foreground occlusion still require native inspection; this is not proof of correct rendering.

This candidate leaves center, axis, 180cm radius, visibility windows, attack logic and depth testing unchanged. Only the boss calls the new optional factory argument, selecting `/Game/DC/Materials/M_BossWeakPoint`. Hall alerts keep their original additive material and geometry. The boss material is unlit, translucent (not additive), two-sided and depth-tested. No new light or increased glow is added.

Four broken cylindrical sections now have a filled warm core (~87cm axial width at base radius), dark teal shoulders, and transparent outer edges. Full band half-width changes from 0.32 to 0.55 times radius. The previous triangular opacity profile had a very narrow visible center; the new high-coverage core and dark boundary should separate from both bright water and dark flesh. Foreground rigging still correctly occludes it.

## Validation

`Tools/probe_combat_cues.py` passed 11,117 geometry/policy iterations (not independent test scenarios), including the existing hall/boss lifecycle checks and new nondegenerate contrast-band topology, unchanged radius, soft outer edges, substantial core width/opacity and bounded alpha-composite contrast over dark/bright backgrounds. This mathematical blend check does not substitute for tone-mapped native frames. Python generator syntax and `git diff --check` pass.

## Root review after the human releases the GPU

1. Integrate only when coordinated. Compile source; generate the new material with `UnrealEditor-Cmd.exe <main>/DirectorsCut.uproject -run=pythonscript -script=<main>/Tools/make_boss_weakpoint_material.py -AllowCommandletRendering -unattended -nosplash`. This idempotent script writes only `M_BossWeakPoint`; existing `M_CombatCue` stays untouched. Preflight now requires the new asset.
2. Reproduce the passive port slam with `-DCStartAt=340 -DCRunThrough -DCStation=port -DCBossReviewAim -DCOrders=Hold_fire -DCAutoShots=342,346,351,353`. No auto-fire. Compare 342 directly with the accepted package. The band should be visible on the joint through the tell, then absent at 353 after the strike. Foreground rope occlusion should remain.
3. If the band is still ambiguous, take an offset diagnostic view of the same joint before changing its radius or brightness. Confirm a broad warm strip with dark shoulders rather than the old target spot alone. Check a second attack/side and the first obstacle; a passing frame at one angle is not full acceptance.
4. Repeat interruption using normal cannon automation (`-DCAutoFire -DCBossReviewAim`) and verify no lingering band after `BOSS_INTERRUPT`. Hall alerts should remain visually identical because neither their material nor ShowAlert changed. Only then cook a new candidate; preserve 19:24 as the accepted build until review passes.
