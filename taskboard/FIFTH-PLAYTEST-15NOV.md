# Fifth playtest fixes

User released the test slot after the20:48 playtest. Root owns serialized editor/build/GPU validation and integration. Existing selected package remains unchanged until acceptance; no itch upload.

## Scope and owners

- cannons_helm: slam target visibility, real contact/impact, general boss body damage marker, synchronized authored attack presentation. DCBossCombat, DCLeviathan and related policies.
- tunnel_boss: earlier statue target indicators, four-hit statues with adequate exposure, three additional general enemies on existing supported banks. DCEarlyEncounters and route policy.
- voice_audio: remove cannon miss text and strengthen surface-anchored water splash (integrated83f7992); next delay second music222→224s, louder descent preserving speech duck, earlier and stronger waterfall landing with deck spray/audio/shake. DCAudio and DCMaelstrom descent sections.
- Root: review/integrate, native visual checks, package and select only after acceptance. AI NPCs chat owns speech streaming and Phonon31M integration, now included in this candidate by explicit user request below.

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
- BossFull: six interrupts, both limbs severed, interrupt damage8 plus body damage34, zero ship damage, scheduled160-second finale preserved. Ending captured at470 and process exited0.

Those native gameplay checks used source compiled through6e81d38. Combined editor compilation throughd0529c9 subsequently passed, including final landing clarity, marker placement, speech defaults, pre-exit drain and late-health callback guards. All agent CPU checks passed; no claim of native Phonon acceptance yet.

Final landing rerender on d0529c9 passed: same single293.19 onset (elapsed23.19), landing water/timber sounds once, unchanged0 damage, descent.4368 returning to.208. Captures294–296 show a clearer deck through the wash and reduced late distortion. Some refraction remains in the last authored splash, so this is an improvement rather than a claim of artifact-free water. CPU/editor lane handed to AI NPCs chat at21:40 for selected-backend acceptance; root remains CPU-only until explicit return.

## Expanded next-test-build scope

Direct user instruction relayed by AI NPCs chat selects Phonon31M + Gemma E4B + streaming for the next local test build. Source/default/payload work integrated into main; existing20:48 playable still unchanged. Selected model found in user Downloads and hash-verified; no blocked download workaround. Initial native compile found four source/API issues, corrected by1df5735. Exit-worker/module lifetime audit found a P1; pre-exit drain barrier8c66b5f and late-health callback guardd0529c9 are integrated and compile. Root retains serialized lane for final game checks, then explicitly hands it to AI NPCs chat for model/voice/cancel/exit validation. Package only after concrete selected-backend evidence.

Selected-backend progress: native chunker caught an empty-prefix FString edge case, corrected byc9cec9b; all8 automation checks now pass. Gemma initially spent its entire80-token allowance on reasoning despite the template hint; explicit zero reasoning budget29ccbf3 passes all27 native orders/acknowledgements under unchanged deadlines. Actual Phonon three-crew smoke passes with positive audio/face samples; eight conversations pass across22 clips. Mid-speech cancellation and fresh critical-call recovery pass. Exit/failure/reload probes remain with AI NPCs chat before package acceptance. Portable staging guards49c846a and refreshed metadata pass ValidateOnly with all runtime/model/voice dependencies present.

## Added menu track

User requested The Atlantean Adventure as menu music, fading immediately at Start Game. Source487910a plus direct StartVoyage hook5bd5dfb adds a looping0.26-gain menu component and0.30-second audio-thread fade. The supplied224.48-second stereo master is prepared unchanged; import and native start/restart checks are pending the serialized engine lane. See [menu music](MENU-MUSIC-15NOV.md).

## Frozen native acceptance, 22:05

All required native checks now pass. The first reload probe found an actual world-leak assertion;70baed0 drains the old world's native speech and game-thread cleanup before actor destruction/GC. The unchanged reload probe subsequently passed, including new-world Ruby speech. Cancel, exit during speech and producer failure/recovery also pass. No unsafe lease release or GC suppression was used.

User requested smoother spoken replies in the coordinated NPC chat. Runtime394f9c9 gives each bounded Phonon reply one continuous utterance while retaining progressive PCM/A2F. Saved/PhononNatural/conversation.json passes8/8 replies, exactly8 speech/face sessions, and playback before native input completion in all8. Warm onset2.100–2.233s, cold4.400s; no claim of instant speech or second-machine validation.

Menu music was imported and passes actual start/fade/restart: two menu starts, two immediate0.30s fades, stopped after0.300/0.306s; TheFog starts fresh both times. Source frozen. Trailer chat has the explicitly reserved five-minute capture slot from22:05:47, then root owns fresh packaging and packaged cold Gemma/Phonon tests.20:48 playable remains selected until that acceptance. Native trailer helper is opt-in only and included by coordinated request.
