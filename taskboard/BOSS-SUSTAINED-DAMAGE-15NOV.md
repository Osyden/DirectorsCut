# Boss sustained damage and actionable slam

Owner: cannons_helm, branch `codex/boss-sustained-damage`, based on mainded0760. Root owns engine, native/GPU and packaging. Accepted20:48 package remains unchanged.

## Diagnosis and changes

The passive native baseline showed an actual slam contact and rigging tear, with hull100→95. Thus contact was not absent: its damage and presentation were weak. The main Kraken also played a complete smash animation stretched across the12-second tell, allowing unrelated body limbs to appear to strike before the real attacking tentacle. Keep the main body idle for slam; the authored detached limb exclusively supplies anticipation, final one-second downswing and actual contact. Mouth, hit and death body poses remain.

The held joint now follows the actual deck/battery heading, keeping it broadside during full helm. Its marker remains visible after a partial hit; previously the1.1-second hit feedback replaced the target. A persistent port/starboard warning and countdown supplements the projected marker, including when looking at the other battery. Existing mouth and obstacle target windows remain unchanged.

Uninterrupted slam strength is now3, causing exactly10 hull damage, stronger physical hull jolt, torn rigging and spray emitted at the deck contact rather than projected down to the sea. Splinters and impact sound remain. The HUD briefly reports actual hull loss. Contact stays once-only and is suppressed after interruption/cancellation/seek. The worst passive encounter budget rises deliberately from26.4 to36.4 hull damage; still survivable without repairing hull.

A separate nonurgent body target exists from combat44s until the scheduled finale, including while interrupt targets are open. It sits1700cm above the mouth, with700cm radius; the mouth's450cm region stays disjoint, including cannonball radius. Crew target selection already prioritizes urgent targets. Body damage accumulates separately from the eight interrupt-hit budget, feeds the existing hidden performance accounting, and appears as a restrained boss-only DAMAGE tally and BODY HIT feedback. No score scoreboard or misleading remaining-health bar is added; damage does not accelerate the music-timed death. Body hits cannot flinch/move the mouth during an urgent tell. The quiet body marker yields label space near an urgent weak point without disabling body damage.

## Validation

-278 production-policy CPU checks: continuous damage beyond eight hits, pause/seek/end gates, partial-hit marker priority, mirrored full-helm battery arc, body/mouth ray separation, once-only10-damage slam at15/30/60/120fps.
-6,907 existing marker/authored-curve checks pass.
-47,839 existing route/encounter checks pass; passive budget uses the production slam-damage constant and asserts36.4.
-`git diff --check` passes. Agent ran no Unreal jobs.

## Root native review

Use the existing boss review script/camera/auto-fire controls. `-DCBossBodyReview` aims the held cannon camera at the live body target; it does not fire on its own. It intentionally overrides interrupt review aim for the body test only.

1. Passive slam: hold fire, seek before340, capture346/351/352/353; no secondary full-body smash during tell. Explicit joint marker and warning should remain; `BOSS_SLAM_READY reachable=1`. Exactly one `BOSS_CONTACT slam=1`, `strength=3`, `damage=10`, hull100→90, rigging loose, spray/splinters at rail/deck. Repeat mirrored starboard and full helm.
2. Interrupt slam: normal review aim/fire, two real impacts; after the first, joint marker still present with one-hit instruction; after second, sever confirmation and no subsequent contact.
3. Body: `-DCBossBodyReview` plus usual fire control, capture the quieter BODY marker. Multiple `BOSS_BODY_HIT` with increasing body/total damage, including between attacks and during a tell. Ensure shots actually hit and camera elevation can reach the marker. `BOSS_BODY_RESULT` preserves total; finale remains160.
4. Mouth/obstacle regressions: existing interrupt tests still pass. General target must not intercept a shot aimed into the mouth. The mouth reachability helper now uses actual muzzle/deck orientation, so test both full helm directions.

General body marker placement and new splash strength still require actual rendered acceptance; CPU checks cannot certify the authored anatomy or image composition.
