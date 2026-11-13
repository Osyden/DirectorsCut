# Evening hand-over, Fri 13 Nov (Claude)

What changed while you were at work, what to play, and what only you can judge. Detail, measurements and the test switches are in [STATUS.md](STATUS.md); pictures are in `taskboard/review/30` to `43`.

## Play this build

`E:\projects\directorscut\Builds\2026-11-13_1824_4c3cc16_no-bands\Windows\DirectorsCut.exe`

It has the fix for the deformed band on the half-pipe (the 18:2x entries in STATUS) and ran at 133 fps on the open sea and 96 to 97 in the drowned city at 1600x900. At true 4K (DLSS 67%, editor build) it gives 94 on the open sea and 77 to 91 in the city; this morning the city was 90 to 101, so the sharp cloud costs about a seventh there. Every build of the day is kept in `Builds\` (eight dated folders plus the two older ones; `Builds\INDEX.txt` lists them) for the dev log.

## Your notes from this morning, and where they stand

| Note | Now |
| --- | --- |
| Fog should be truly volumetric, with a path through it and structures hidden and revealed | Done in two layers. Soft placed banks first; then, from mid-afternoon, the walls along the course and the curtains before each colossus and temple are sharp, self-shadowed cloud (the engine's heterogeneous volumes). Review set 43, frame at 150 s, is the nearest yet to `fog-city-reveal.jpg`. |
| Storm front should come from nothing and grow into the wall | It starts as a smudge six seconds in and is the full front by 44 s, backlit. It is still towers, not one black curling wall. |
| Wisps across the deck | Not achieved. There is a faint mist on deck; distinct streaks are finer than the soft fog can draw. |
| Rain splashes look fake, and land on an invisible plane | The plane was the ship asset's hidden helm trigger; fixed. Splashes are small crowns of thrown water now. |
| No ruins before the fog | Removed. |
| Foam fake and off the water | Foam is now part of the sea's own shading round ruins, rocks and hull, with a wake astern. The painted band and the ribbon are off. |
| Ship motion too little | Roughly doubled, with a slow swell; the captain's view leans with the deck. |
| Hit reactions and sounds | Ruins are solid: she is shouldered off, lurches, the view shakes, a thud and a new scrape play, the crew call it. |
| Half-pipe sea (16:22: much larger, partly fog-covered; 17:43: fix the deformed band) | 90 m to each shoulder, 11 m up there, level beyond it where the city stands, cloud walls on the shoulders. Hard over she holds 66 m out and 9 m up. The band was a temple base made of one hugely stretched block, hanging over lower water, plus a wrong water normal: all three fixed (review set 44). |

## Also new

- Ship view (`T`): a camera outside the ship that the mouse swings round her; good to steer by. Open sea and city only.
- The drowned city is a storm at dusk now, with lightning called every few seconds that lights the cloud.
- A colour grade; spray over the bow; the inside of the front is a grey blind instead of brown.
- Codex's cinematic maelstrom pass is merged and runs to the ending.

## Two things I got wrong during the day, both corrected

- I reported the deck lean working before it was; it and the camera shake had never reached the picture. Both do now.
- I reported that the engine's sharp volume renderer did not work in this project and advised holding off on VDB packs. It does work (my test volumes were beyond its 300 m default range). The sharp banks above are the result, and your tunnel pack should draw as intended. Notes for installing it are at the end of [VDB-ATMOSPHERE-HANDOFF.md](VDB-ATMOSPHERE-HANDOFF.md).

## Only you can judge these

1. **Sound.** The hull scrape and every other sound: I cannot listen. `-DCDumpAudio` writes the synthesised ones to `Saved\AudioDump`.
2. **Hands on the controls.** Steering in the big trough, walking the banked deck, `T` and the mouse orbit, the guns. Everything was tested with switches, never by hand.
3. **Whether the cloud city is what you meant.** It is brighter and busier than this morning's soft fog. `-DCNoHeroBanks` shows the soft version for comparison.
4. **The storm front from outside**, against `storm-front-portal.jpg`.

## Not done

- **Maelstrom pass** (your 16:22 message and `MAELSTROM-TIMED-BOSS-DESIGN.md`): read, not started, no maelstrom file claimed or touched. It is the first item for the next session.
- A tunnel of cloud inside the storm front: tried twice, hidden by the front's own fog. This is the job for the tunnel pack.
- Cloud faces: three attempts, nothing readable from the deck; off (`-DCFaces`).
- Crew are still mannequins.
- The water-plugin patches (current, foam, wake, trough) live in the engine folder; SETUP.md has how to apply and undo them.
