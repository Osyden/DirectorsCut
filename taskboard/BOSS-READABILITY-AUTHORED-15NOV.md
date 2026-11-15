# Explicit boss targets and authored attack tentacle

Owner: cannons_helm, `codex/boss-band-contrast`. This follows pending contrast commit `82eee57`; integrate that first. Human 19:24 playtest remains untouched. No engine, build, import or GPU job was run by this agent. Root owns integration and native review.

## Readability and feedback

Boss targets now have screen-space brackets, a dark-backed label, hit count and countdown at the actual projected damage target. The mouth explicitly says `SHOOT THE MOUTH`; limbs say `SHOOT THIS JOINT`. Off-screen targets get directional edge arrows and battery labels. The obstacle also states the safe steering direction. While a joint is still submerged during emergence, the marker points to the disturbed surface and says `TENTACLE RISING` instead of pretending the hidden joint is already a clear shot. The actual damage target/window is unchanged.

The final three seconds tint toward orange/red without strobing. Successful hits immediately show remaining hits, and full interruption confirms severing, obstacle clearance or mouth-charge cancellation. These labels remain legible over foreground rigging; the separate world band still uses depth testing and the pending boss-only contrast material. Hall cues are unchanged.

Slam anticipation pulls the tip much higher and farther back, holds the joint at its original reachable position, then uses the original last-second downswing. Timings, health, hit volumes and one-time impact behavior are unchanged. Both boss references to buckets now ask for smothering flames. `tunnel_boss` agreed a maximum 40-degree steering heading; the existing fixed slam joint remains inside its cannon arc at that yaw, unlike 55 degrees.

## Authored limb evidence and implementation

Installed assets contain the full `/Game/Kraken/Meshes/KRAKEN` plus body/limb materials and animations, but no standalone tentacle mesh. Existing native audition evidence in `Boss-Caldera/Saved/KrakenAudition.log` records 190 bones. H_1 starts at bone51, H_2 at52, and SplineIK081..095 continue linearly through bone67: seventeen bones. B_1 starts at87 and provides the existing mirrored permanent body cut. The original mesh uses `Mat_TENTACLES_LEGS_CLAWS`; its actual geometry, UVs, textures and material are retained.

`UDCAuthoredTentacle` uses a second poseable instance of the original skeletal mesh. It keeps the H branch and its ancestors, collapses unrelated branches with zero local scale, and masks non-tentacle material slots. This avoids reading stripped cooked vertex buffers or exporting licensed mesh data. The full selected authored skin is bent by reference-bone transforms along the current committed attack curve, using continuous transported frames. A real authored bone is pinned exactly to curve U=.72, the unchanged cannon joint. Body-width scale follows the actual boss instance rather than the old procedural tube dimensions. Roots remain submerged.

The authored instance is the default attack limb. The old tube remains only behind explicit `-DCProceduralBossArms` (or the existing Claw comparison). Missing required assets/chain/material emit `KRAKEN_LIMB` errors; they do not silently substitute the tube. Existing main-body H/B bone hiding still makes successful slam cuts permanent; the isolated limb withdraws through the original recoil/end logic. Route obstacles reuse the selected authored skin as permitted detached appendages.

This is a source implementation, not a claim of validated finished art. Native review must check skin deformation, suction-cup orientation, torso leakage at the submerged root, cue-band clearance around the new skin, and performance of the additional full skeletal draw. The old procedural contact curve/target remains authoritative, so an authored mesh can have local visual thickness differences. No import or asset mutation of the vendor Kraken was performed.

## Required generation and tests

After the human releases the slot, root compiles then runs both scripts through the Unreal Python commandlet with `-AllowCommandletRendering`:

- `Tools/make_boss_weakpoint_material.py` → `/Game/DC/Materials/M_BossWeakPoint` (pending82eee57).
- `Tools/make_authored_tentacle_material.py` → `/Game/DC/Materials/M_BossLimbHidden` (fully masked, skeletal usage).

Both are idempotent and preserve vendor assets. Release preflight requires the generated materials. Default startup must log `KRAKEN_LIMB ready source=/Game/Kraken/Meshes/KRAKEN... root=H_1 bones=17 ... skinSlots=1`; zero `KRAKEN_LIMB` errors. One shared full authored mesh is used; only the selected branch is visibly skinned.

`Tools/probe_boss_readability.py` passed 6,907 CPU iterations for marker bounds/edge guidance across viewport sizes, exact hit-window suppression, monotonic authored-bone mapping with pinned root/joint/tip, curve continuity and stable joint under the stronger wind-up. Existing cue probe passed 11,117 iterations. These are math/geometry checks, not GPU skinning or visual tests. Python syntax and diff checks pass.

Native review sequence:

1. Passive close port slam: `-DCStartAt=340 -DCRunThrough -DCStation=port -DCBossReviewAim -DCOrders=Hold_fire -DCAutoShots=342,346,351,353`. Inspect real authored cups, clean hidden remainder, obvious `SHOOT THIS JOINT`, two-hit requirement, readable wind-up, exact strike, then marker removal.
2. Normal-speed interrupted slam with `-DCAutoFire -DCBossReviewAim`; check first-hit confirmation and remaining1, then `BOSS_INTERRUPT event=1`, immediate shoot-marker removal and persistent body sever. This is automation, not human reaction validation.
3. Obstacle at ride320–332: watch emerging warning, committed target, safe-side instruction and clearance on hit. Mouth at358–368: explicit mouth marker/charge, interrupt and passive release paths. Other-side slam at396–408 confirms presentation from both batteries.
4. Existing cannon, boss timeline and navigation checks remain unchanged. Re-run both success/passive paths; require bounded hits/contacts and the fixed deadline. Cook a new candidate only after authored visual/native acceptance; preserve accepted19:24 until then.
