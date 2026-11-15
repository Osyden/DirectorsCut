# Caldera boss fight — 15 November

Branch: `codex/boss-caldera-15nov`. Isolated worktree: `E:/Projects/DirectorsCut/Boss-Caldera`. Base: `a368e8f`. Revised implementation checkpoint: **`66aadbe`**, pushed for source review.

## Latest owner direction

The owner replaced the earlier waterspout/add plan with **tentacle obstacle → interruptible tentacle slam → interruptible mouth/fireball charge**. Only one urgent response at a time. Main slam limbs remain severed; separate route appendages may be used as production shortcuts. Damage earns performance; the finale stays at breach +160 seconds (ride 430), followed by the parent's fog handoff at 450. No escape channel or spiral restoration.

**Current state: revised source candidate, CPU replay passing, current revision not yet built or accepted visually.** A five-minute shared engine/GPU window was requested for the Kraken audition and remains pending. Do not present the earlier Claw/spout screenshots as evidence for this revision. The playable launcher rejects source newer than its binary.

## Changes

- `DCBossTimeline.h`: six scheduled events at encounter 50/70/88/106/126/144, with 12s obstacle lead, 9s slam tell, 10s mouth charge and the existing 3s fireball flight. Windows derive from `FinaleAt`; no HP or limb threshold advances the clock.
- `DCBossCombat.*`: committed arena-space obstacle, explicit safe-side cue, shootable joint, swept hull collision; reachable broadside slam joint, interrupt/recoil, one-shot contact and persistent two-limb state; mouth charge gated by actual gun range, yaw, pitch and water clearance. Leaving a usable arc cancels the charge. Failure releases the shared fog fireball. No spout or boarding loop remains active.
- `DCBossFireball.h` extracts the fog giant's original rock mesh, orange glow/light, flight arc, closest-deck-point fire and boom into one helper used by both encounters. The fog's timing and damage remain the same.
- `DCLeviathan.*`: bounded inner-lake swimming path independent of helm offset; no old crater-floor placement. Procedural appendage roots stay underwater and remain outside the hull except for controlled rail contact. Body hit/death poses, pain sound and surface contacts use encounter time.
- `-DCBossKraken` selects the Kraken candidate. It is **opt-in pending audition**, not a visual acceptance claim. The candidate hides `KRAKEN_ARM_L/R` for severed slam limbs. Anatomical suitability and cut quality still need visual proof. The default body is still the Claw candidate until that choice is made.
- `DCKrakenAudition.inl`: isolated cannon-camera audition of idle, smash, sweep, hit, death, hidden `KRAKEN_TENTACLE_BASE_A`, and hidden `KRAKEN_ARM_L`. Loads licensed assets in place; never exports or modifies them. Bone hierarchy and clip lengths are logged.
- Cannon integration accepts health-capped effective damage through an optional external-owner delegate. Closed, exhausted, interrupted, paused and post-finale windows score nothing. Damage, interruptions, severed limbs and cumulative ship damage are separate; repairs do not erase the penalty.
- Native HUD delegate adds encounter cues/results without editing `DCGame.cpp` or the crew UI. Three small `DCMaelstrom.cpp` hooks disable old attack scheduling, read boss sound pressure, and fade the obsolete port-looking reveal camera bias out by encounter 40 so forward obstacles can be seen.
- `DCTroubles` stops random rigging tears while this arena is active, exposes encounter cleanup, and uses the ride-scaled delta for fire lifetime. Slam impact uses existing hull notifications/impulse plus spray, boom, splinters and visible loose ropes.

## Validation and limits

`Tools/test_boss_caldera.ps1` compiles the production encounter policy and route header with a minimal math adapter, then passes **4,951 checks**: event crossings at 15/30/60/120fps, clear-side avoidance after two seconds of reaction including 35% steering authority, no overlapping urgent windows, swept contacts, exhausted-limb damage cap, deadline closure, score caps and a **26.4 hull-point passive upper bound**. These are CPU policy checks, not Unreal physics, asset or visual acceptance.

The superseded spout prototype built and ran its direct-seek/finale/sea-return review (`Saved/BossFirstNative.log`). A normal-speed passive run was stopped immediately when the owner asked to pause; it did not complete. Those findings do not validate the revised three attacks. `Saved/BuildKrakenAuditionFinal.log` built the earlier six-pose audition before the subsequent source rewrite; rebuild now.

A later build exposed shared vendor plugin binaries being locked by another engine job. The original plugin junctions are retained under ignored `Saved/SharedPluginLinks`; the active plugin roots now have **local Binaries/Intermediate** and junctions for source/content/resources. No licensed source assets were duplicated or pushed. `Saved/BuildBossIsolatedEditor.log` passed after isolating the outputs, but predates the revised encounter. Parent/main source checkouts were not edited. Concurrent scene jobs mean earlier frame samples are not a performance benchmark.

A parse-only check of current `DCTreadmill`, `DCTroubles` and `DCMaelstrom` succeeded. The boss units require refreshed UHT output after header-line changes; do not treat that parse attempt as a current build pass. Audio balance, manual cannon interruption, permanent cut seams, obstacle contact composition, mouth opening and late-seek reconstruction remain native checks.

## Local dependencies and next native window

Engine `E:/Utilities/UE_5.8`; MSVC `14.44.35207`; CPU Python `E:/Utilities/Blender/5.3/python/bin/python.exe`. Purchased content and the installed cliff runtime are ignored junctions into the existing local checkouts. The shared cliff runtime is the parent's current installation, not a frozen copy of a368e8f's generated assets. Boss-only `Content/DC/Boss/M_BossTell.uasset` is original and reproducible with `Tools/make_boss_materials.py`.

1. Check running engine/build jobs and obtain the reserved slot. Do not stop the scene chat's process.
2. Build `DirectorsCutEditor Win64 Development -Project=E:/Projects/DirectorsCut/Boss-Caldera/DirectorsCut.uproject -WaitMutex -MaxParallelActions=4` through the installed engine's `Build.bat`.
3. Run `Tools/review_kraken_audition.ps1 -Wait`. It refuses to start while another engine job exists. Shots are saved under `Saved/SceneReview/KrakenAudition`. Check the **actual ride timestamps in the log**: the screenshot harness waits about 5 seconds after each seek.
4. Inspect the frames and hierarchy. Choose/harden the body and actual severable appendages; do not assume a named bone gives a clean cut. Main-limb stump treatment and the mouth-charge pose are provisional.
5. Build and run the new loop from helm and both guns: no-fire, successful interrupts, failed slam, obstacle dodge, mouth eligibility/fire, fixed deadline, backward seek and cleanup. Check cannon ball flight time, not just firing before the tell ends.
6. Re-run CPU replay, record exact build/native results, then update this handoff and push the review checkpoint.

## Merge boundaries

Only take the listed boss files, cannon/fire helper and narrow hooks. Do not replace the parent's `DCMaelstrom.cpp` wholesale. Route, water, banks, cliffs, shoreline mist, sky and exit geometry remain parent-owned. Preserve main's Rex/Duncan/Ruby, portraits, voice and helm fixes; this branch starts from the older crew baseline and does not modify their UI files. There is no packaged-build or finished-art claim.
