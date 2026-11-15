# Late combat balance and statue sweep

Owner: cannons_helm, isolated codex/combat-balance-late from main7cafdfa. Source only; accepted23:10 upload untouched. Voice owns warning predicates/group identity; tunnel owns obstacles.

Regular creatures: three normal hits; independent25% instant kill on each valid shot. Kill distribution25%/18.75%/56.25%. Statues remain four hits. Waves3/4/4/4 at unchanged times, unchanged four-second charge spacing. Later walkers hold3300cm apart so fourth target remains inside cannon range. First exposure12s instead of8 permits reaction2 + initial reload3 + inter-shot reloads6 + maximum flight3 before first release15s.

Boss interrupts: slam3, mouth2, route2. Tells15/12/15s; route earliest collision12s. Worst loaded-or-reloading final impacts14/11/11s respectively. Slam recovery3s ends before next mouth. Final fireball429, unchanged finale430. Shortened debug finales omit late attacks instead of compressing safe windows. General body damage unchanged.

Route joint side offset3500 to9500cm: old joint entered the real cannon arc too late for two shots. New geometry preserves committed root/anchor, collision and dodge side. Replay includes threatened lanes, +/-10deg heading and +/-1000cm muzzle X; minimum final-impact margin1.69s. Requires native authored-limb visual acceptance.

Statue beam: existing eye cylinders35cm diameter instead of10, moving contact glow and elongated warning ellipse along a straight line between two surveyed same-level deck points. Contacts0.2/1.0s ignite up to two fires under unchanged MaxFires2/deduplication. Damage1.2 once at first contact, no second boom/damage. Delayed frames resolve each contact once. Redundant Stage2 spoken warning removed:1.2s cannot fit synthesis/delivery. Precharge speech preserved. No new assets.

CPU checks passed: creature43,427; fog/beam297,082; boss56,775 including1,152 actual dodges; readability11,708; boss damage278. git diff --check clean. No engine/native/GPU jobs.

Root validation: DCEncounterVerify through ride242 (one-to-three hits, beam line contacts, capped fires, cleanup); DCStatueVerify remains four hits. Boss ordinary shots326/330 route,346/354 slam,364/369 mouth. DCBossReviewAim+DCAutoFire at matching station: BOSS_HIT counts2/3/2 and truthful remaining labels. Passive slam contacts355/411. Inspect new route joint silhouette and statue beam/contact glow; CPU does not establish visual quality.

Integration conflict: voice ec69058 changes Stage2 beam report removed here. Remove that entire redundant Stage2 report block, preserve voice GroupId and all other warning lifecycle guards. Never overwrite the whole shared encounter file.
