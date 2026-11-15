# Third playtest fixes, 15 November

The **19:24 local package is selected for human playtesting**, runtime `25076ed`, at `E:/Projects/DirectorsCut/Builds/2026-11-15_1924_third-playtest/Windows/Atlantean Odyssey.exe`. The usual `E:/Projects/DirectorsCut/Play Atlantean Odyssey.cmd` selects it through `Saved/Release/Current.json`. The 18:33 package is retained, with its prior selector copied to `Saved/Release/Previous-1833.json`. No itch upload is authorized. Root released the shared build/GPU slot after the final check; human testing has priority.

## Changes

- Fog architecture is fitted by its actual rotated bounds outside the full steering and hull envelope. Continuous masonry banks support nine creatures in four encounter groups; four extra statues preserve a clear route. The three apparent pillar obstacles were large architectural bases, so moving only the small procedural pillars did not solve them.
- Hall creatures retain directional locomotion through cooldown and carrying/charging; their authored rip and throw clips are timed to the actual attack window. Amber alert cues mark the charge; boss tentacle joints receive a depth-tested vulnerability band.
- Cannonballs query the rendered waves and the channel trough. Water hits emit a dedicated splash and water sound; upward shots survive until their ballistic return. The helm wheel's visual sign now matches steering.
- Caldera FFT sampling uses the same basin coordinates as its foam and fallback waves. The regenerated water material includes tilted hull contact. The deck follows the descent grade, then gets one compression/rebound and bow spray at contact. Outward steering is constrained to supported inlet water until the bow clears its edge.
- Music rises from .25 to .32 before master gain and speech ducking (about +2.1 dB). Kraken spray remains visible without a sound every .4 seconds; ambient arrival roar happens once.

## Verification before packaging

- Editor build passed after the final inlet guard and splash scale change. Native water material generation succeeded with live DRW FFT bindings.
- CPU policies: fog route 262,837 assertions; creature animation 3,102; combat cues 9,538; cannon water 24; caldera motion 26,754. Latest caldera probe confirms zero unsupported inlet frames at 30/60/120 Hz for sustained left, center and right input.
- `Saved/ThirdPlaytest/Fog`: continuous route, sustained right steering, shots 167/195/210. Ship stays clear of both banks; walker visibly on masonry at 167. Actual settled lane 7,219 cm, not the full 9,000 cm limit. All 34 scenery footprints satisfy the clearance check. No hull stone strikes or missing animation clips. Nine creatures spawned; cooldown and carrying clips advance.
- `Saved/ThirdPlaytest/Water`: four real shots produce four water contacts, including an upward shot returning to the sea. Contact effect inspected; final splash scale increased from 1.1 to 1.4 afterward and still needs packaged visual confirmation.
- `Saved/ThirdPlaytest/Landing`: deck pitch reaches -31.4 degrees; one contact at elapsed 29.18 with spray and rebound. First-person frames show the downward deck and landing spray. Ship-view mode is intentionally disabled after the slide begins. This run used no sound, so it does not establish audible mix quality.
- `Saved/ThirdPlaytest/CannonVerify`: all 14 checks pass, including empty-sky firing, separate player/NPC gun ownership, crew walking to a real target, and wheel clearance. Exit 0.
- `Saved/ThirdPlaytest/Helm`: positive steering reaches -45.5 degrees wheel angle, reverse reaches +45.5, then returns to center. All 56 pivot samples have hub drift at or below .0001 cm. Lane velocity reverses with inertia.
- `Saved/ThirdPlaytest/Boss`: actual cannon run interrupts all six attacks, severs two designated limbs, applies exactly eight effective damage, takes zero boss damage, and reaches the scheduled ending. Target bands are absent after interruption in the inspected images; active-band presentation still needs the packaged hold-fire view.

## Packaged acceptance

- Package completed with zero cook errors and exit 0. All 47 required assets and installed ocean shader hashes pass preflight. No Source/Config mutation occurred during packaging. Commits after runtime `25076ed` contain investigation tooling/documentation only.
- `ThirdFull.log`: normal menu start and continuous voyage through ride470 with sustained right steering. Zero stone strikes; one landing at29.19. Bow-down view at285 and spray/rebound at299.4 inspected. The outer inlet remains supported and reaches the lake without the former13m snap; full outward steering later returns to70.9m. Holding fire produces the expected boss contacts and scheduled result/ending. This accelerated run uses no sound and is not an audio acceptance test.
- `ThirdCue.log`: active port slam target and HUD inspected at342/350. The new wrapping band cannot be confidently distinguished from the existing target behind ropes/reticle. **Its readability is not accepted as finished polish.** No visible displaced geometry or leaked post-interruption band was found; rendering versus burial/occlusion still needs an unobstructed view.
- `ThirdWater.log`: four shots, four water contacts, including ballistic sky-shot return, exit0. The final1.4-scale water spray is visible in shot3.8. No thunder/boom is routed through the dedicated cannon water-contact function.
- `ThirdMusic.log`: actual ACE playback succeeds for Rex, Duncan and Ruby; second track starts220.01; music gain.208 falls to.052 during speech. Exit0. This establishes the louder mix and retained real speech ducking, while human listening judgement remains useful.
- Package logs and screenshots live under its `Windows/DirectorsCut/Saved`. `build-info.json` records the checks and limitations. The selector points to this candidate only after these checks.

## Remaining polish

The original temple/rigging outline report remains open. Matched second167 tests reject global changes to direct lighting, shafts, volumetrics and specular. Close crops locate the strongest reproduced white lattice within a sail panel; this does not explain every reported outline. The next bounded pass uses `Tools/audit_ship_sail_fog.py` to identify the actual sail component/material and its opacity/depth/fog handling. See [the diagnostic record](FOG-EDGE-COMPOSITION-15NOV.md). Keep the production fog settings intact until a scoped fix is demonstrated.

For the boss highlight, first inspect the active joint without the foreground ropes/reticle masking it. If the band is present, improve boss-specific contrast with a darker edge and warm core, rather than changing hall alerts or merely adding bloom. Preserve exact weak-point geometry, interrupt windows and depth testing. Caldera FFT sampling now uses basin coordinates and the generated material renders; direct human judgement of steering motion remains useful alongside the source and captured-frame evidence.

The continued-polish heartbeat is active every 30 minutes in this chat. It must respect ongoing foreground/subagent work and human playtest GPU priority, preserve accepted packages, and keep distribution local.
