# Morning hand-over, Saturday 14 November

**Draft, last brought up to date at 06:55. Claude is still working until 08:00 and still holds the claim on the maelstrom files until this line is removed.**

## Play this

`E:\projects\directorscut\Builds\2026-11-14_0701_ef7691f_morning\Windows\DirectorsCut.exe`

It was run from its exe without a window at 1600x900, the whole voyage from start to ending card at double speed with the bot captain: no errors, twelve frames taken on the way. It holds everything in the table below. Every earlier build (27 in all) is still in `Builds\` for the dev log; `taskboard/DEVLOG-NOTES-NIGHT-13-14NOV.md` says what each added.

## What changed overnight, in the order you meet it

| Where | Change | Seen in |
| --- | --- | --- |
| Open sea, 26 to 58 s | One storm front: a pale bank on the horizon with a rolling top that grows into a wall of the bought cloud volume, about 4 km long and 900 m high, standing on the water, with the tunnel as a lit opening through it. Sheet lightning flares in it four times as it closes. The sky pack's flat-topped storm slab no longer stands behind it. | `review/47`, `review/54`, `review/57` |
| 59 to 75 s | The ship sails through the cloud tunnel. The air inside is clear, so its walls and the light at the far end show; four bolts on cue light the walls from within. Stone is kept out of sight until the tunnel has passed it. | `review/47`, `review/54` |
| City, 76 to 240 s | The white self-lit banks are gone. A thin layer of mist lies over the water everywhere, with fog cards on it and over the channel's shoulders, and rags of mist run aft across the deck. The sea is grey and wind-torn. Lightning falls as visible forked bolts and lights the fog behind the ruins. Six stone giants wade up to the banks by turns and throw burning rock at the deck, which starts fires; the batteries can kill them. Rocks in the lane are solid: she grinds along them, and a set of the current carries her off stone dead ahead. The bow throws sheets of white water. | `review/48`, `review/51`, `review/53`, `review/59` to `review/63`, `review/65` |
| Guns | Shots are iron balls with a thin grey streak; the lower-deck guns fire from outside the hull. In the maelstrom, shots fired below the ship's level no longer burst on the hidden open sea beside her. | `review/49` |
| Maelstrom, from 270 s | The ship hangs on the lip for three seconds, then the bow drops and she goes down into the bowl. Six waterspouts and five burning stones come on a schedule, each with a tell and a crew call; the helm carries the ship clear. Two of the creature's young swim in on the beam, to be sunk by that side's guns. The creature is beaten on the clock at 160 s after the breach: a kilometre-high plume from the crater's heart, in which its body heaves up once and goes under. The results card shows damage taken and arms broken. | `review/50`, `review/52`, `review/55`, `review/58`, `review/64` |
| Crew | They are told what is actually round them in each part, so their lines fit (no "storm wall ahead" at the ending). The helmsman no longer swings her on to the rocks. | |

## Your notes of 04:45 to 05:20, one by one

| Note | State |
| --- | --- |
| Gap between the sea and the foot of the storm front | Fixed: the cloud's floor was up to 12 m over the water; it is at the water. |
| Objects popping in ahead | Laid 650 m ahead instead of 300 m. No pop found in stills; needs your eye in motion. |
| The ship clips through things | Lane rocks were not solid at all; they are now. Scenery was already solid. |
| Lightning is flashes, not streaks | Each bolt is now drawn as a forked streak. Thin, a third of a second. |
| Rock on the right showing through the fog tunnel | Stone hidden until the tunnel passes it; none seen in frames since. |
| Big objects: a current so she scrapes and cannot hit head on | In, and the helmsman fixed, but not reliable. At normal speed with the bot, two whole-city runs gave two and three contacts with stone (8 and 11 blows), one of them on the stem, and hull 71 and 73 at the slide. Double-speed test runs are worse because the blows are reckoned harder. She can still be put on a rock head on; it needs more work. |
| Creatures on the banks throwing fire | Stone giants (Rampage, stone skin, lit cracks), six of them. Marchosias and the giant demons unused. |
| Bow splashes | Reworked as sheets of white water. Judged from above the bow only. |
| Do not neglect the later game and the boss | Spawnlings, the guns' reach, the lip, the creature's one appearance and the ending were all worked on after that note. |

## Not verified by anyone

- Nothing has been seen in motion or heard by a person. Every judgement above is from still frames and logs.
- Whether the cloud wall at 18 pictures a second, the tunnel at 24 and the spouts at 24 show as a flicker.
- Steering round a spout, a stone or a rock by hand. One order to the helmsman ("Finch, hard to port") did clear a spout by 32 m in a test.
- Putting out a fire by hand or by order while giants are throwing.
- The port guns' muzzles; all new sound (wind rising with a spout, booms).
- The balance. With nobody steering or giving an order the hull is about 72 at the slide at normal speed, and was 23 and 10 at the ending card in two double-speed whole runs (which overstate the damage from rocks).

## Known faults

- The maelstrom's water is still striped and its foam is in contour lines; the reveal has no near crest hiding the drop. These are the first items in Codex's plan (`MAELSTROM-NEXT-PASS-PLAN.md`) and untouched.
- The creature at the end reads as a pale broken mass in the plume, not a clear animal.
- The spout is the tunnel volume stood on end: its top is darker than its foot.
- The burning stones are a rock with a tail of plain glowing balls; the giants' rocks are plain glowing rocks.
- The creature's young have no wake and do not come on deck.
- From far off the wall is one pale bank of one kind of cloud; a dark flat band shows right of the portal at about 56 s.
- The sea's white is in drifting patches, not on the wave crests.
- Without the bought packs installed the front has no tunnel or wall, the city has no cards, mist sheets or giants, and the spouts are a faint plain column.

## Speed

Packaged build at 1600x900, normal speed: open sea about 96 fps, wall and tunnel 88 to 100, city 98 to 116, slide about 100, maelstrom 65 to 73, ending 123. True 4K (DLSS at 67 percent, no frame generation, editor build, measured at about 06:57 with everything in): open sea with the wall close 71 to 77, entering the tunnel 56, inside it 67, city 91 to 98 (74 with a giant throwing), maelstrom 55 to 61 with a spout up, dipping to 43 to 45 in single frames as the spout passes the ship. Those dips are the render thread waiting on the spout's volume frames, not the graphics card (10 to 13 ms).

## For Codex

`taskboard/FOR-CODEX-14NOV.md` lists every change to the maelstrom files, the changes outside them since 04:45, and what is not started. After pulling, re-run both steps of `Tools/patch_drw_shorefoam.py`.

## Packs

In use, all git-ignored: Clouds Tunnel Loop (volume 01 only), Easy Fog, Paragon Rampage (stone skin, animations, one rock). Not used: FogArea, Ancient Marchosias. Nothing was bought. On the evidence of the tunnel volume, the same author's Clouds Storm and Water Tornado packs would replace the two things built from the tunnel (the wall and the spouts) with shapes made for the job.

## Also written for you

- `taskboard/ITCH_PAGE.md`: suggested changes, since the page text still describes Thursday's build.
- `E:\projects\directorscut\StoreShots\2026-11-14`: thirteen clean 4K frames without the HUD (taken at 02:50, before the giants and the sea smoke).
