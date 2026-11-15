# Fifth playtest fixes

User released the test slot after the20:48 playtest. Root owns serialized editor/build/GPU validation and integration. Existing selected package remains unchanged until acceptance; no itch upload.

## Scope and owners

- cannons_helm: slam target visibility, real contact/impact, general boss body damage marker, synchronized authored attack presentation. DCBossCombat, DCLeviathan and related policies.
- tunnel_boss: earlier statue target indicators, four-hit statues with adequate exposure, three additional general enemies on existing supported banks. DCEarlyEncounters and route policy.
- voice_audio: remove cannon miss text and strengthen surface-anchored water splash (integrated83f7992); next delay second music222→224s, louder descent preserving speech duck, earlier and stronger waterfall landing with deck spray/audio/shake. DCAudio and DCMaelstrom descent sections.
- Root: review/integrate, native visual checks, package and select only after acceptance. AI NPCs chat owns isolated speech streaming and Phonon31M integration; these are not part of this candidate yet.

## Baseline evidence

Native `Saved/FourthPlaytest/BossPassive` rerun21:19–21:20 from pre-change binary: target marker visible at346 from the aimed port cannon; one slam contact352.01 (boss elapsed82.01), hull100→95 and rigging torn. This confirms weak effect/readability rather than absent contact logic. Main body previously played a full unrelated smash over the12-second tell, which can visually strike before gameplay contact; remove that misleading secondary smash and keep the authored attacking limb authoritative.

Water FirstEntry can return an already-submerged point when a wave engulfs the projectile. Terminal effect now reprojects to the sampled surface, while collision/damage remain unchanged. Cooked splash gets stronger plume and no shadow; miss text removed at source.

## Validation pending

Root local harness `Saved/ReviewFifth.ps1` covers passive/interrupt/helm slam, sustained body shots, statue markers/toughness, ordinary/caldera water impacts, landing and music envelope. Review flags must be checked against delivered source. Require one contact/damage application, target priority over body, no phantom early strikes, real rendered water plumes, readable non-overlapping markers, safe extra enemy placement,224-second track start, preserved speech duck, and one appropriately placed earlier landing. Source/CPU checks alone are not native acceptance.

## Native checks completed before speech integration

Evidence under `Saved/FifthPlaytest/Native`:

- Water and WaterCaldera: four actual water contacts each, sampled/analytic surface placement, cooked plume and bounded live bursts; near starboard plume visually confirmed, no miss text.
- Slam: joint reachable; one352.01 contact, actual hull100→90, torn rigging, deck spray/splinters and readable held warning. Body marker inspected beside boss face. Main body stays idle during authoritative authored limb strike.
- Body: actual captain shots register increasing body damage through an obstacle tell. Review aim was corrected to use the same AimRotation that ResolveAim/fire uses; previous camera-only hook was insufficient for ballistic acceptance.
- Interrupt: first real hit leaves one-hit joint guidance, second severs at346.35; no contact, hull100 through355.
- StatueVerify: all six native assertions pass, exactly four fired shots silence statue before release; ignored threats cause20 fireball and3 beam impacts, phase cleanup passes. Eye marker inspected at118; physical statue remains faint through dense fog, an existing visual limitation.
- Steering: held A/D cycle preserves radial drift, approximately9° heading (10° cap) and6°/s smoothing. Inlet outward safety clamp remains intentional. No navigation route changes.
- Landing (before clarity refinement): one onset293.19, no damage, three-second wash, cooked water bursts; landing water/timber audio logs once. Descent music.4368, normal combat.208; separate statue run starts track224.01. Native images show mist too dominant, so follow-up61ffac9 reduces only landing cards to40% opacity and late authored bursts to1.6; final comparison still pending.
- BossFull: six interrupts, both limbs severed, interrupt damage8 plus body damage34, zero ship damage, scheduled160-second finale preserved. Final ending capture pending process completion at this entry.

Native game source compiled through6e81d38. Later clarity/arrow and speech changes await the next successful compile. All agent CPU checks passed; no claim of native Phonon acceptance yet.

## Expanded next-test-build scope

Direct user instruction relayed by AI NPCs chat selects Phonon31M + Gemma E4B + streaming for the next local test build. Source/default/payload work integrated into main; existing20:48 playable still unchanged. Selected model found in user Downloads and hash-verified; no blocked download workaround. Initial native compile found four source/API issues, corrected by1df5735. Exit-worker/module lifetime audit found a P1; owner is implementing a pre-exit drain barrier before native speech tests. Root retains serialized lane for final game checks, then explicitly hands it to AI NPCs chat for model/voice/cancel/exit validation. Package only after concrete selected-backend evidence.
