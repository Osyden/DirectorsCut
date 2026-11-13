# Maelstrom implementation plan

Owner handoff, 13 November. Branch `codex/maelstrom-reveal`, independent clone `E:/Projects/DirectorsCut/Maelstrom-Clone`. Started at `0f9aaae`; integrated Claude's published storm-front, rain, cook-list and crew handoffs through `1071550` before the final combined build.

Codex owns DCWaveCurl, DCPortal, new DCMaelstrom and DCLeviathan files and their material generator. Claude owns the storm front, fog, treadmill, ruins builders and sky-pack integration. Existing ruins builders may be called without modification. The stage is started by the wave actor so the shared director needs no source edits.

The narrow integration changes include DCGunnery's water-height query (from Codex's earlier gunnery work). Claude supplied the Kaiju/ruins cook entries in his branch. The new stage configures only the Breach/Maelstrom lighting cues and the Ending blend through the director's public phase data. It hides DRW's grid actors (the water Blueprint itself is one) while the bowl is active, restoring them on exit. Neither the plugin nor Claude's source files are changed.

At runtime the stage temporarily lowers height fog to the crater floor, limits its rendered density, and hides UDS's separate sea-level global fog material. These settings are restored on exit or backward seek. This is necessary because the pack's additional volume otherwise obscures the crater despite the phase fog input. Ordinary volumetric height fog remains available. The earlier fog phase still uses Claude's original environment. Legacy weather particles are hidden only during this encounter. Helm input is read from the treadmill's public API to turn the arena with inertia; the treadmill implementation is unchanged. Pressing C releases the automatic reveal framing to the existing player camera toggle.

## Visual target

The captain exits a dark, hollow water tunnel onto the lip of a vast basin. Brief overwhelming sunlight resolves into a legible descent, far rim architecture and a giant creature. Preserve the view from the playable helm, with the mast offset and the basin visible beyond the bow. Reference is `E:/Projects/DirectorsCut/References/maelstrom-direction.jpg`.

1. Build a 3.2 km bowl, 650 m deep, with geometric spiral ridges and a continuous local surface near the ship. Move the arena around the stationary ship along a descending route. All landmarks and the boss use the same arena transform.
2. Stage the exit over 15–20 seconds, retaining dark tunnel edges briefly, resolving exposure promptly, then introducing the boss. Supply a real hollow tube rather than bending the whole sea into an overlapping sheet.
3. Create dedicated water and spray materials with reproducible Python tooling. Use medium-scale geometry and fine moving normals/foam. Inspect steep and flat sections under the actual lighting. A future DRW spectrum binding must preserve surface coordinates and normals.
4. Use the existing modular ruins builders to assemble rim sanctuaries, broken colonnades and statues outside the treadmill cleanup list. Separate depths with waterfall spray and retain a clear view into the basin.
5. Spawn the repaired Claw Kaiju with its material explicitly assigned. Use full animation clips, a slow emergence, and connected procedural tentacles with damage proxies, attack telegraphs and visible boss reactions.

   The Kaiju is approximately 666 m tall at scale 18, with emergence depth derived from its actual mesh bounds. Its body starts in the basin and moves alongside during descent. The four tentacles bring damageable joints into the cannons' 180 m range; the six-second warning precedes a deck strike. Six broken arms defeat the creature. Cannon aim and splashes use the bowl surface, including the small flat area needed around the stationary ship.
6. Cut cleanly back to the normal sea at the ending, covering the cut with a brief controlled exposure pulse and removing stage geometry, attacks and effects. A defeated boss finishes sinking for 20 seconds, then the new stage seeks the public director timeline to Ending, avoiding a prolonged empty arena. The scheduled ending still provides an escape if the boss survives.

## Verification and handoff

Build in the isolated clone. Generate only new assets there. Capture full-size helm views before, during and after emergence, through the encounter and after the exit. Check a continuous normal-speed passage for seams and temporal failures. Confirm early-stage behaviour is preserved. Record remaining visual limitations and the tested commit in STATUS.md before pushing and merging the branch. Preserve Claude's in-progress edits during integration.
