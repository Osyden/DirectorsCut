# Maelstrom scale pass plan

Owner handoff, 13 November. Branch `codex/maelstrom-reveal`, independent clone `E:/Projects/DirectorsCut/Maelstrom-Clone`. This revision started from `52b7071` and now integrates `b26d3f4`, including Claude's hull-shudder fix and ruins wash line. It follows the previously integrated and packaged first pass. See [the handoff](MAELSTROM-HANDOFF.md) for completed validation and remaining acceptance work.

## Ownership and integration

Codex owns DCWaveCurl, DCPortal, DCMaelstrom, DCLeviathan and their material generators. DCGunnery has the narrow integration that queries the stage's water surface. Claude retains the storm front, fog, treadmill, ruins builders and sky/water plugin integration; existing ruins builders are called without modifying them. No source changes to DCRideDirector, DCFogFront, DCTreadmill, DCRuins or DCWeatherFX are required.

DCWaveCurl starts the stage. The stage configures public Breach/Maelstrom cues and the Ending blend, temporarily adjusts height fog, hides UDS's separate global fog material, DRW grid actors and legacy weather particles, then restores them on exit or backward seek. The earlier fog phase retains Claude's environment. Helm input is read through the treadmill's public API; C returns framing to the existing player camera toggle.

## Visual direction and implementation

The dark water tunnel should open onto overwhelming light that resolves into a vast, readable crater. The water, depth and distant rim must establish scale before combat begins. A full exposed creature made the arena feel smaller, so the current encounter keeps its body unseen. Reference: `E:/Projects/DirectorsCut/References/maelstrom-direction.jpg`.

1. **Crater and route:** 3.2 km across and 900 m deep. A continuous surface joins the broad entry ramp, approximately 240 m outer fall face and deeper inner slope. The entry lip descends immediately instead of hiding the abyss behind a rounded shoulder. The ship remains fixed while the arena moves around it; the route runs from 98.5% to 78% of the radius. Revised framing and bearing open the crater to the right of the bow.
2. **Water:** geometric swells use irregular noise-warped crests and crossing detail rather than uniform spiral bands. The latest pass reduces the far-wave amplitude while preserving smaller entry waves. Moving detail normals and foam remain shader-driven. Cannon aim, projectile water impacts and splashes use the same stage surface query, including the local hull shelf.
3. **Rim:** continuous land behind the overflow connects smaller ruin complexes and statues. Landmarks sit on that ground instead of reading as isolated giant islands. Their smaller silhouettes should help communicate the basin's size.
4. **Threat:** the Claw body, animation and shadows remain hidden below sampled water; no full-body emergence or chase. The view stays clear for 22 seconds after the breach. Local tentacles then peek at staggered intervals from roots 60 m below nearby water. Their radii are 55% smaller, with transported frames and continuous geometry through the damageable joints.
5. **Combat:** first attacks begin 65 seconds after the breach, with four arms offset by six seconds. Each retains a six-second tell, deck strike and recovery. Lower bright joints are lifted or moved forward as needed to clear sampled water and remain within cannon limits. Six broken joints force withdrawal; pain audio and dialogue describe the unseen threat letting go.
6. **Exit:** allow 20 seconds after victory, then seek the public timeline to Ending. The scheduled ending remains available if the creature survives. Restore the ordinary sea and environment behind the brief exposure transition; clear targets on exit or seek and prevent strike damage on paused/seek ticks.

## Acceptance still required

Build and regenerate in the isolated clone. Review full-size playable helm frames at tunnel exit, the quiet crater reveal, first tentacles, combat and open-sea return. Run continuous normal-speed combat and an unopposed passage, including the new bearing and surface profile; verify target hits, withdrawal, timed exit and environment restoration. Repeat relevant pause/seek checks and package this revision before claiming it ships.

Offline target-envelope checks are useful supporting evidence, not a replacement for those runtime checks. The cinematic reference is not yet met: waterfall breakup, atmospheric depth, water detail, grounded architecture and tentacle appearance still need visual judgment. Preserve Claude's ownership and in-progress work during integration.
