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
