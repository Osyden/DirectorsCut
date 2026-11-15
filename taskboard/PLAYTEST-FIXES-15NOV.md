# Human playtest fixes — 15 November

This pass starts from combined main, preserving the installed caldera, Kraken, ACE/crew and individual cannon integration. Local testing only; do not upload to itch. Account: Onibox.

## Changes

- Flattened the tunnel's lower hull corridor, shortened its irregular mouth and waterfall apron, and retired the approach tube once the stern clears. The backward descent view no longer retains a floating tube underside.
- Doubled the Kraken's scale and moved its berth to preserve ship clearance. Its mouth remains above water and within the extended 210m player aiming range.
- Rotated cloud sampling and both sun implementations with the caldera. Regenerated the cloud material with rendering enabled.
- Added one landing crash, six water-contact bursts, a strong pitch impulse and camera trauma. Continuous descent triggers it once; seeks do not replay it and it does not damage the hull.
- Player cannons fire into empty sky and nearby water. Added orange muzzle fire, smoke and a short light flash. Independent gun ownership, one ball per trigger and reloads are retained.
- Crew physically vacate the wheel when the player takes it.
- Authored hazard calls bypass LLM rewriting; phase changes invalidate queued/in-flight replies, synthesis and playback. Storm bell strikes only three times on entry.
- Removed duplicate weather audio, lowered ambience and ducked it under centred crew speech. Wind, rain, rumble, drone, rough sea and thunder fade to exact silence over the tunnel exit; calm water and gameplay impacts remain.
- Added readable speech bubbles with circular crew portraits, including text fallback when synthesis fails. Removed score UI and moved instructions into a Start Game menu showing the ship at open sea. The voyage remains held until Start.

## Validation

Editor and game targets build. Eight Unreal automation suites pass, including tunnel hull clearance, command parsing, synthetic speech fixtures and caldera route/contact checks. CPU boss replay passes 4,957 assertions.

Native menu capture and automatic Start transition pass. Fourteen cannon/helm assertions pass, including actual sky firing and physical wheel clearance. Backward waterfall frames at 275/277/280 seconds were inspected. Storm silence logged at 277.01 seconds. Continuous landing logged once at 299.41 seconds, with no damage.

The normal-speed native boss run interrupted all six attacks, permanently severed two limbs, dealt eight effective boss damage and took zero ship damage. Both enlarged mouth charges were reachable. Evidence is in `Saved/Playtest*.log`, `Saved/PlaytestAutomation.log` and `Saved/SceneReview/Playtest*`.

Packaging and packaged runtime validation are pending. The prior tested launcher remains selected until they pass. Human headphone mix, microphone input and another-machine checks remain final acceptance tasks.
