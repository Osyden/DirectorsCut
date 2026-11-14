# Dev log notes: the night of 13 to 14 November (Claude)

Raw material for a dev log, in the order things happened. Every build named here is still in `E:\projects\directorscut\Builds`; every picture set is in `taskboard/review`. Times are local.

## The problem at 20:00

The owner played Friday evening's build and said the clouds and fog were still the main fault: two cloud systems cutting across each other, white self-lit fog pasted over the statues, a storm front that formed in jerks. They had installed a pack of looping cloud volumes (a tunnel) and a pack of fog cards.

## What the pictures show, build by build

| Build | What is new in it | Pictures |
| --- | --- | --- |
| `2026-11-13_1824_4c3cc16_no-bands` | Friday evening's state: the sky pack's storm slab as the front, sharp white banks in the city. | `review/43`, `review/44` |
| `2026-11-14_0006_68b9d2c_storm-wall-spouts` | The bought tunnel volume used four times over: once as the tunnel into the fog, three times enlarged and laid across the course as a wall. White banks off, fog cards on the water, iron cannonballs, the first waterspouts, the boss beaten on a clock. | `review/45` to `review/50` |
| `2026-11-14_0041_fbabe67_spouts-stones-wisps` | Burning stones, mist running across the deck, lightning that lights the fog instead of whiting out the stone. | `review/51` to `review/53` |
| `2026-11-14_0116_26370c7_tunnel-lightning` | Lightning on cue inside the tunnel and in the wall; a plume when the creature is beaten. | `review/54` |
| `2026-11-14_0207_974b398_spawnlings` | The creature's young swim in on the beam. | `review/55` |
| `2026-11-14_0235_0a152ce_guns-reach-down-slope` | The guns can hit things below the ship in the maelstrom. | |
| `2026-11-14_0301_5018846_one-front` | One storm front on the open sea instead of two. | `review/57` |
| `2026-11-14_0315_e71ea99_rolling-top` | The far bank's top rolls. | `review/57` |
| `2026-11-14_0334_0739c01_over-the-lip` | The ship hangs on the maelstrom's lip, then the bow drops. | `review/58` |

Clean 4K frames without the HUD: `E:\projects\directorscut\StoreShots\2026-11-14`.

## Things worth telling

- **One asset, four jobs.** The pack is a tunnel of cloud about 50 m long. Scaled 300 times it is the tunnel the ship sails through. Scaled 1400 times and turned across the course it is a storm wall four kilometres long. Stood on end and spun it is a waterspout, and a kilometre high it is the plume when the creature goes down.
- **The pack's own material let the cloud pass through the sea.** A material for these volumes sees position in the volume's own voxels, so a few lines cut the cloud off just above the water. The same material later got a way to drop the thin haze that showed the volume's box, and a rolling ceiling so a long wall is not a mesa.
- **The white fog the owner hated was mine.** The sharp self-shadowed banks I had made the default on Friday do not take the fog that darkens the scenery in front of them, so they sat bright behind dark statues like cut-outs. Turning them off was the fix.
- **Two storms in one sky.** The new wall stood in front of the sky pack's storm and each cut across the other. The pack's storm now waits until the wall is on the ship and comes in over her as the dark overhead and the rain.
- **Clearing the air made the tunnel visible and then made it dull.** Thick fog inside the front hid the tunnel's walls. Clear air showed them, but stopped the chance lightning, which needed mist. Four bolts on cue brought it back.
- **A bug older than the night.** The batteries never sank anything to starboard in the maelstrom. A shot log showed every ball ending at exactly sea level a few metres from the hull: the hidden open sea still stopped them. Anything below the ship's level had been out of reach all along.
- **Blocking streaming.** Asking the engine to wait for each frame of a volume costs render-thread time in proportion to playback speed; not waiting gives a soft, coarse volume at 4K. Spouts play at 24 pictures a second with waiting; the wall at 18.
- **Mistakes.** A material that failed to compile silently drew no storm at all; two test runs printed times an hour ahead in the log until I checked the clock; one test at 01:40 was run with the sound on.

## Tried and dropped

- Rolling cloud lining the mouth of the maelstrom's water tunnel: a white-out at the exit and lumps hanging behind the ship.
- Lights inside the cloud tunnel: they lit the air into pale haze instead of the walls.
- Flame cones for the burning stone's tail: flat pale yellow in daylight.
- Turning the fog off for the cloud volumes: the far wall became a dark cliff pasted on the horizon.
