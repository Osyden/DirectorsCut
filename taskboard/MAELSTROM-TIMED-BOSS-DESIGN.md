# Owner direction: larger half-pipe and timed maelstrom climax

Owner instructions via Codex, Friday 13 November, 16:22. This is a design/feedback handoff, not an implemented feature or validation claim.

## Priority and ownership

Immediate direction for Claude: **the Atlantis half-pipe needs to be larger and partly obscured by fog.** Once the current authorised work runs out, the owner explicitly authorises Claude to take a maelstrom visual and gameplay pass rather than wait for Codex's usage reset. This supersedes the previous instruction to leave that stage entirely to Codex. Claim the files/branch in STATUS before changing them; leave a concrete handoff for Codex's next session so both agents do not edit the stage at once.

The maelstrom and boss are core unfinished work, not optional polish. Saturday is the main build-out; Sunday prioritises a complete tested and submitted build, then improvements to its worst parts. Preserve a known-good playable route throughout.

## Larger half-pipe, partly fog covered

Make the fog-stage sea feel like a broad, immense channel rather than a narrow gutter around the ship. Increase the lateral scale of its shoulders and the surrounding scene coherently, maintaining the shared sea-height rule for water, ship, obstacles and fog. Wider geometry also needs steering authority, return force and obstacle placement tuned to remain playable.

Use uneven banks across the outer shoulders and upper slopes so their full extent is not immediately visible. Reveal portions with lightning, passing gaps and parallax. Keep the navigable middle readable; occasional veils may cross it but should not hide every obstacle or all steering feedback. Avoid two identical walls or a uniformly opaque fog belt. The new heterogeneous banks may provide the needed sculpted foreground shapes; keep cheap haze farther away as appropriate. Compare from the actual helm and ship camera, in motion, not just overhead.

## Maelstrom emergence: rollercoaster reveal

Keep the reference's enormous water crater, monumental distant rim, rich violent water and luminous sky. Existing reference: `E:/Projects/DirectorsCut/References/maelstrom-direction.jpg`; the owner's attached concept in the chat remains the visual target. Existing 3.2 km diameter / 900 m depth is the current implementation baseline, not a reason to make the whole perimeter visible at once.

Suggested shot/experience sequence:

1. **Compression:** near the tunnel exit, dark detailed curling water/cloud overhead, rushing peripheral motion, wet ship foreground. A bright opening ahead grows while most of the crater stays hidden. Keep the opening in view; do not turn the camera into the near tunnel wall.
2. **Threshold:** sail through into overwhelming daylight, with a short glare/exposure response that quickly resolves. Hold the dark tunnel as a framing edge briefly and preserve readable ship/rigging silhouettes. A white screen that conceals a scene swap is insufficient.
3. **The lip:** a few seconds of slower, anticipatory travel at the crest. Show the distant sunlit ruin rim and majestic layered clouds first. Near water geometry and spray initially conceal some of the drop. Sound/music can open up while motion seems to hang for a breath.
4. **Depth revelation:** as the bow crosses the shoulder, progressively expose the falling slope, the middle-distance whitewater, then the deep central abyss. The player should discover how far down it goes through occlusion, parallax, slope angle and motion. Compose god rays and low spray banks as depth layers, preserving the floor's legibility. Give the player look control; use eased framing rather than a hard camera snap.
5. **Descent:** a controlled initial pitch/drop that grows into a sweeping slide. Broad unequal waves, narrow local crest detail and accelerating foam/spray communicate speed and scale. Irregular overflows and grounded smaller ruins establish distance. Avoid a circular swimming-pool wall, terrain-like blue hills, concentric corrugation and smooth flat water under the foreground.
6. **Encounter approach:** settle into a relatively slow orbit around the central threat, with the immense bowl still dominating. The reveal gets room to breathe before the first attack. Keep the prior feedback: a huge full-body monster emerging next to the ship makes the world feel small. Use submerged bulk, partial forms and scale-aware distance; expose readable combat anatomy without filling the whole reveal with the Claw model.
7. **Release:** at the scheduled musical climax, a convincing boss death/defeat and withdrawal beat, followed by a clean transition back to open sea and an exhausted aftermath. The victory must be visible and audible, not merely a phase label changing.

## Boss fight: authored duration, player-earned performance

The owner's direction is a **Shadow of the Colossus / Soulslike-inspired naval boss**: awe, legible wind-ups, committed attacks, dangerous recovery windows and rewarding precision, played from a slowly circling ship. The creature is ancient and magnificent; scale and choreography matter as much as the damage numbers.

**The boss dies at a configured elapsed time T so music stays synchronised.** Player damage determines performance/score and phase reactions, not whether the finale happens early. Choose T from the selected music once available; expose it as data rather than burying an arbitrary duration in code. Use one authoritative encounter clock; seeks, pause and speed changes must not desynchronise hazards, weak-point windows, music cues or the finale.

This deliberately replaces the current six-joint-break early-exit rule. Do not retain an early HP/joint threshold that skips the musical climax. Strong play can stagger the boss, break armour or extend vulnerability within bounded scheduled windows; it must not advance the master timeline. At T, stop attacks and damage scoring, execute the defeat, and allow the release/escape beat. The player must still see their shooting produce reactions and visibly accumulate damage throughout.

### Player loop and hazard roles

- **Slow circling:** maintain a clear boss bearing and readable broadside opportunities while progressing around the encounter. Steering changes the ship's path enough to avoid threats, with momentum and recovery that can be understood. The established ship-fixed/world-moving implementation can express this; a rewrite of the whole sailing system is not inherently required. Keep cannon elevation/yaw reachability correct along the route.
- **Tornadoes:** the boss spawns them ahead along the ship's route, with clear spawn tells and enough lead time to choose a safe path. Steering away is the primary response. Contact tears ropes/sails and reduces sailing effectiveness, with visible rigging damage and crew feedback. Avoid spawning an unavoidable tornado directly on the hull.
- **Fireballs/boulders:** telegraph source, trajectory and impact area. Impacts damage the ship and start local deck fires, creating extinguish/crew-order pressure. Reuse and improve existing fire, crew and cannon systems where possible; avoid invisible damage ticks or screen-filling effects that hide the next tell.
- **Adds:** hostile sea creatures approach in readable lanes and can be shot before they board. Show distance/approach state clearly. Surviving arrivals initiate boarding pressure with a distinct consequence and response; a real boarding outcome is required, not just adds vanishing beside the hull. Start with one reusable add archetype and a complete approach/intercept/board loop.
- **Highlighted weak spots:** phase-specific vulnerable parts expose through animation and boss action. Cannons must be able to hit them from the available batteries; highlights stay legible through fog, lighting and spray. Include hit feedback, damage/stagger response and clear closed/open transitions. More vulnerable parts and longer/more valuable openings reward sustained accurate shooting.
- **Crew and stations:** one player cannot continuously hold the wheel, aim both batteries, fight boarders and put out fires. Stagger hazards/windows, allow sensible station switching, and use the existing quick/typed/voice crew orders for delegation. Distinguish player skill from automated crew contribution where useful, without making microphones mandatory.

### Suggested musical phase structure

These percentages are implementation starting points, not an owner-selected final duration. Store events/windows in data so the chosen score can reshape them.

| Encounter time | Boss behaviour | Player focus / musical purpose |
| --- | --- | --- |
| Reveal before combat clock | Scale, lip, descent, partial threat | Awe and orientation; no immediate hazard pile-up |
| 0–25% of T | One clear attack family; first exposed weak spot | Learn the tell, steering and cannon response |
| 25–55% | Tornado lanes plus scheduled recovery openings | Steering decisions and rewarding broadside hits |
| 55–80% | Fireball/boulder salvos and limited adds; changed anatomy exposure | Crew delegation, fire control, intercepts and higher-value shots |
| 80–100% | Authored escalation with staggered combined threats and largest exposure | Final musical build; accuracy rewarded without skipping the finale |
| T and aftermath | Forced defeat/death, attacks cease, withdrawal and escape | Musical resolution, then performance result |

Use cooldowns, seeded variation and explicit simultaneous-hazard caps within the authored schedule. Do not turn every later phase into all mechanics firing continuously. New phase behaviour should be visible in pose, animation, sound and attack cadence, not just more HP or a HUD label.

## Scoring and results

Track **boss damage dealt** and **ship damage taken** separately and present both. Proposed combined score: weighted effective boss damage plus capped add-interception bonuses minus a ship-damage penalty. Exact weights remain tuning data, not a final rule. Show damage as a meaningful performance grade even though boss death is scheduled.

Prevent farming: award effective vulnerable damage, cap rewards after an exposure or part is exhausted, exclude invulnerable hits and post-finale damage, and do not allow repairing then taking the same damage repeatedly to erase the cumulative penalty. Preserve cumulative ship damage in results even if repairs restore hull. If catastrophic failure/retry is retained, restart the encounter and musical clock coherently; do not let it desynchronise later phases. Survival rules still need explicit gameplay tuning.

## Delivery order and proof

1. Build one complete timed loop first: reveal/descent, slow orbit, telegraphed tornado dodge, reachable weak spot, actual hit response, forced finale at T, score and sea return.
2. Add projectile/fire and one add/boarding loop, then phase choreography and richer visual effects. Reuse existing targets, fires, rigging damage and crew commands where they work.
3. Review normal-speed gameplay from helm and both guns: no-fire run, strong-damage run, dodge success/failure, sails torn, fire response, add killed versus boarding, music-aligned finale in both high/low damage cases. Also check pause/backward seek and cleanup/restoration.
4. Provide captures of tunnel threshold, lip anticipation, progressive drop reveal, slow orbit with a tornado tell, weak-point exposure and finale. Include measured performance and a packaged second-machine test before claiming delivery.

The full vision remains the target; prioritise a complete convincing playable encounter over a collection of disconnected hazard effects. Sunday starts with a testable deliverable, then fixes the worst remaining issues.
