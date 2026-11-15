# Caldera boss fight — 15 November

Branch: `codex/boss-caldera-15nov`. Isolated worktree: `E:/Projects/DirectorsCut/Boss-Caldera`. Base: `a368e8f`. Three-attack source baseline: `66aadbe`; the polish checkpoint is the commit containing this handoff update.

## Latest owner direction

The owner replaced the earlier waterspout/add plan with **tentacle obstacle → interruptible tentacle slam → interruptible mouth/fireball charge**. Only one urgent response at a time. Main slam limbs remain severed; separate route appendages may be used as production shortcuts. Damage earns performance; the finale stays at breach +160 seconds (ride 430), followed by the parent's fog handoff at 450. No escape channel or spiral restoration.

**Current state: Kraken polish pass built and playable locally, ready for source review.** The owner authorized the polish pass and the engine slot was free. The current Editor target builds and 4,951 CPU checks pass. The playable launcher rejects source newer than its binary. Earlier Claw/spout captures remain superseded.

## 15 November polish pass

- Kraken is now the default; `-DCBossClaw` retains the earlier body for comparison. Facing uses the shell-to-beak direction and placement anchors the torso: the bounding-box centre was ahead of the beak because of the long tentacles, incorrectly presenting its back and then putting its mouth out of range when facing was corrected. The bounded reference footprint is capped at 45m; the mouth is held 6.5m above the sampled lake to submerge the lower body while retaining a usable shot.
- The actual skeleton has `KRAKEN_TENTACLE_BASE_A_1`, not `KRAKEN_TENTACLE_BASE_A`. Auditioned the corrected cut. The two combat cuts use the mirrored H_1/B_1 tentacles, not the claw arms. Remaining authored animation continues after hiding a limb.
- Obstacle starts with disturbed water, then rises/uncoils over three seconds. Removed the glowing water disk. Slam progressively pulls back, holds a reachable joint, then accelerates over its last second. Successful interruption kicks the limb back with a water burst. Contact still applies damage once, with timber/spray/impulse and slack rigging.
- A flattened exposed band replaces the floating joint sphere. Mouth charge has a growing local orange light. Weakpoint emission pulses faster as impact approaches and flashes on hit. Original `M_BossSkin` gives detached appendages warmer skin, pale cups and geometric sucker-rim relief; it contains no purchased textures.
- Moving route tentacles use a wider projectile-hit allowance (1400cm external radius, versus 450cm for stationary slam/mouth targets). This covers their roughly 8m/s movement during a staggered ballistic volley. The initial tighter allowance let correctly aimed volleys trail the target and made the shoot-to-clear option unreliable.
- HUD shows severed limbs, a short response cue and a diminishing warning bar. The obstacle bar uses earliest possible hull contact; post-contact shots cannot claim an interruption. Mouth crew calls use the actual battery side. A severed limb no longer triggers the body's corresponding slam pose.
- `-DCBossReviewAim` aims the existing cannon camera at the active target within normal yaw/pitch limits, for unattended views and optional existing `-DCAutoFire`. It does not run in ordinary play. It is not evidence of manual aim or human reaction testing.

Build evidence: `Saved/BuildBossPolishHelm.log` (final Editor success). Original material generation: `Saved/BossPolishMaterials.log`, completion marker present. Auditions: `Saved/SceneReview/KrakenFront` and preceding facing/pose comparisons. Direct-seek three-attack review: `Saved/SceneReview/BossPolishPort`, including deadline at 160, sea return and backward seek.

Native findings:

- `BossPolishPassive`: normal-speed continuous 306–451 run verified each obstacle applies 3 damage once, each failed slam applies 5 once, rigging consequences, and deadline/cleanup. It exposed the mouth placement bug (both charges skipped), so it is not evidence for the final mouth behaviour.
- `BossPolishFire`: after the torso fix, real player/crew cannon projectiles severed both designated limbs and interrupted both mouth attacks; result 13 effective damage, 6 ship damage, 2 severed limbs, 4 interruptions, score 605. Later mouth views show both limbs remain absent with the remaining pose intact. This run motivated the moving-obstacle hit allowance.
- `BossPolishMouth`: continuous no-fire replay from 354; both mouth attacks released the shared fireball and hit once at encounter 101.02/157.04, each causing 2 immediate damage and a deck fire. Deadline result 15.4 cumulative ship damage (including obstacle/slam/fire), with cleanup at 160 and sea return at 450.
- `BossPolishReady`: complete 306–451 firing replay before the final obstacle silhouette adjustment. **All six attacks interrupted by real cannon projectiles; 20 effective damage, 0 ship damage, 2 permanently severed limbs, 6 interruptions, score 1000 at the fixed 160-second deadline.** Both route obstacles clear before collision. Player port aim/auto-fire and the starboard crew exercise both batteries. No strike or fire impact occurs after successful interruption.
- `BossPolishHelmFinal`: normal-rate direct-seek wheel views of both obstacles. The final curve is higher, the low shootable joint is closer to the route, and the tapered tail is shorter/thicker. Port-side silhouette is clearer; the default wheel view's large starboard sail still occludes much of the first obstacle. The directional HUD remains legible; use the existing **C clear-view** control when playing and preserve main's newer helm changes when integrating.
- `BossPolishObstacleFinal`: targeted final-source replay after that curve/joint change. Both route obstacles, the port slam and the first mouth charge interrupt successfully (14 effective damage, four interruptions), with no ship contact or fire impact. This rechecks the changed obstacle targeting rather than treating the earlier full run as identical source.
- The firing and mouth replays use **2x global time dilation** to exercise outcomes; they are not normal-speed human reaction or balance evidence. Review aim remains constrained to the real gun arcs. No native fatal/assertion or boss-material compile failure was found. Baseline plugin/Python startup warnings remain unrelated.

## Changes

- `DCBossTimeline.h`: six scheduled events at encounter 50/70/88/106/126/144, with 12s obstacle lead, 9s slam tell, 10s mouth charge and the existing 3s fireball flight. Windows derive from `FinaleAt`; no HP or limb threshold advances the clock.
- `DCBossCombat.*`: committed arena-space obstacle, explicit safe-side cue, shootable joint, swept hull collision; reachable broadside slam joint, interrupt/recoil, one-shot contact and persistent two-limb state; mouth charge gated by actual gun range, yaw, pitch and water clearance. Leaving a usable arc cancels the charge. Failure releases the shared fog fireball. No spout or boarding loop remains active.
- `DCBossFireball.h` extracts the fog giant's original rock mesh, orange glow/light, flight arc, closest-deck-point fire and boom into one helper used by both encounters. The fog's timing and damage remain the same.
- `DCLeviathan.*`: bounded inner-lake swimming path independent of helm offset; no old crater-floor placement. Procedural appendage roots stay underwater and remain outside the hull except for controlled rail contact. Body hit/death poses, pain sound and surface contacts use encounter time.
- `DCKrakenAudition.inl`: isolated cannon-camera audition of idle, smash, sweep, hit, death, one and two hidden mirrored tentacles. Loads licensed assets in place; never exports or modifies them. Bone hierarchy, socket positions and clip lengths are logged.
- Cannon integration accepts health-capped effective damage through an optional external-owner delegate. Closed, exhausted, interrupted, paused and post-finale windows score nothing. Damage, interruptions, severed limbs and cumulative ship damage are separate; repairs do not erase the penalty.
- Native HUD delegate adds encounter cues/results without editing `DCGame.cpp` or the crew UI. Three small `DCMaelstrom.cpp` hooks disable old attack scheduling, read boss sound pressure, and fade the obsolete port-looking reveal camera bias out by encounter 40 so forward obstacles can be seen.
- `DCTroubles` stops random rigging tears while this arena is active, exposes encounter cleanup, and uses the ride-scaled delta for fire lifetime. Slam impact uses existing hull notifications/impulse plus spray, boom, splinters and visible loose ropes.

## Validation and limits

`Tools/test_boss_caldera.ps1` compiles the production encounter policy and route header with a minimal math adapter, then passes **4,951 checks**: event crossings at 15/30/60/120fps, clear-side avoidance after two seconds of reaction including 35% steering authority, no overlapping urgent windows, swept contacts, exhausted-limb damage cap, deadline closure, score caps and a **26.4 hull-point passive upper bound**. These are CPU policy checks, not Unreal physics, asset or visual acceptance.

The superseded spout prototype built and ran its direct-seek/finale/sea-return review (`Saved/BossFirstNative.log`). A normal-speed passive run was stopped immediately when the owner asked to pause; it did not complete. Those findings do not validate the revised three attacks. `Saved/BuildKrakenAuditionFinal.log` built the earlier six-pose audition before the subsequent source rewrite; rebuild now.

A later build exposed shared vendor plugin binaries being locked by another engine job. The original plugin junctions are retained under ignored `Saved/SharedPluginLinks`; the active plugin roots now have **local Binaries/Intermediate** and junctions for source/content/resources. No licensed source assets were duplicated or pushed. `Saved/BuildBossIsolatedEditor.log` passed after isolating the outputs, but predates the revised encounter. Parent/main source checkouts were not edited. Concurrent scene jobs mean earlier frame samples are not a performance benchmark.

The earlier parse-only check is superseded by the full current Editor build. Audio balance and manual cannon timing still need hands-on review; automated aiming and silent captures do not establish those. Native screenshots establish individual poses, not a finished animation-quality claim.

## Local dependencies and reproduction

Engine `E:/Utilities/UE_5.8`; MSVC `14.44.35207`; CPU Python `E:/Utilities/Blender/5.3/python/bin/python.exe`. Purchased content and the installed cliff runtime are ignored junctions into the existing local checkouts. The shared cliff runtime is the parent's current installation, not a frozen copy of a368e8f's generated assets. Boss-only `Content/DC/Boss/M_BossTell.uasset` is original and reproducible with `Tools/make_boss_materials.py`.

1. Check running engine/build jobs before using the authorized slot. Do not stop the scene chat's process.
2. Build `DirectorsCutEditor Win64 Development -Project=E:/Projects/DirectorsCut/Boss-Caldera/DirectorsCut.uproject -WaitMutex -MaxParallelActions=4` through the installed engine's `Build.bat`.
3. Run `Tools/review_kraken_audition.ps1 -Wait`. It refuses to start while another engine job exists. Shots are saved under `Saved/SceneReview/KrakenAudition`. Check the **actual ride timestamps in the log**: the screenshot harness waits about 5 seconds after each seek.
4. `Play Boss.cmd` opens the current local Editor/game build at ride 306 with normal manual controls. It does not enable review aim or auto-fire.
5. For outcome replay, use `review_maelstrom_sky.ps1 -Native -Continuous -Start 306 -Wait -ReviewName BossPolishReady -Shots '327,345,360,382,401,419,430,451' -Station port -Orders 'starboard_battery_fire' -ExtraArgs '-DCBossReviewAim','-DCAutoFire','-DCTimeScale=2'`. Omit firing flags/orders for the failure path.
6. Remaining hands-on checks: audio mix, human recognition/aim timing, full-helm animated body clearance and close inspection of cut roots. Detached appendages are still procedural and visibly simpler than the purchased body; the ordinary wheel view has the sail occlusion described above. There is no claim of finished creature art or a newly packaged standalone release.

## Merge boundaries

Only take the listed boss files, cannon/fire helper and narrow hooks. Do not replace the parent's `DCMaelstrom.cpp` wholesale. Route, water, banks, cliffs, shoreline mist, sky and exit geometry remain parent-owned. Preserve main's Rex/Duncan/Ruby, portraits, voice and helm fixes; this branch starts from the older crew baseline and does not modify their UI files. There is no packaged-build or finished-art claim.
