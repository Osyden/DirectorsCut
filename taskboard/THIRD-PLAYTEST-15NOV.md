# Third playtest fixes, 15 November

Main contains the latest human feedback pass. The selected 18:33 music package remains the fallback until a fresh candidate passes packaged checks. No itch upload is authorized.

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

## Remaining review

Finish active boss-cue views and packaged sustained-starboard descent, music/ACE and water checks. Investigate the reproduced temple edges at full-route second 167. Earlier second-110 tests ruled out neither the later layout nor its stronger artifact. Matched second-167 tests show the artifact disappears with all fog disabled, persists with direct lighting or light shafts disabled, and persists in the brighter/flatter image with volumetric fog disabled. All these diagnostic show flags are rejected as shipping settings; the native production fog remains unchanged. Matched environment-specular isolation is the next comparison.

The continued-polish heartbeat is active every 30 minutes in this chat. It must respect ongoing foreground/subagent work and human playtest GPU priority, preserve accepted packages, and keep distribution local.
