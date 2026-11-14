# For Codex, Saturday 14 November: what Claude changed in the maelstrom overnight

Read with the STATUS entries of 13 Nov 23:38 onward. I worked on main in small commits. The claim on the maelstrom files ends when `taskboard/MORNING-14NOV.md` appears; after that they are yours again unless the owner says otherwise.

## Changes to your files

| File | Change | Switches |
| --- | --- | --- |
| `DCLeviathan.h/.cpp` | `FinaleAt` replaces `BreaksToDefeat`: the creature is beaten at that many seconds after the breach, with the low pain sound and a crew line. Broken arms still retract for their cycle, score 150 each and are counted on the director (`ArmsBroken`). `ShouldExitEncounter` is unchanged: 20 s after the defeat. | `-DCBossDiesAt=` |
| `DCMaelstrom.cpp` `BeginPlay` | Sets `Boss->FinaleAt = EndingStart - BreachStart - 20`. | |
| `DCMaelstrom.h/.cpp` | `FTwister`, `UpdateTwisters`, `ClearTwisters`: six scheduled waterspouts, called from `Tick` while the stage is active. | `-DCNoTwisters`, `-DCTwisterDensity=`, `-DCTwisterRate=`, `-DCTwisterBlocking`, `-DCTwisterLog` |
| `DCMaelstrom.cpp` `UpdateStage` | `Descent` is two-part: `Plunge` (0.42) of it between 9 and 29 s, the rest over the fight. | `-DCPlunge=` |
| `DCGunnery.cpp` | Ball and streak look; lower-deck muzzles 1.1 m further outboard; `-DCGunTestAt=`. | |
| `DCGame.cpp` | Results card line. | |
| `DCRideDirector.h/.cpp` | `DamageTaken`, `ArmsBroken`. | |
| `DCTreadmill.h/.cpp` | `bSteerForced`: `-DCSteer=` holds the helm against the crew. | |

## Burning stones (added 00:38)

`FBoulder`, `UpdateBoulders` in `DCMaelstrom`: five scheduled stones thrown from the abyss in a 5 s arc, dodged with the helm exactly as the spouts are, starting a deck fire through `ADCTroubles::StartFire` when they strike. `-DCNoBoulders`, `-DCBoulderAt=`. Plain glowing spheres; they want smoke, a splash and sound.

## How the spouts work, and what they lack

- Kept in ship space: `Ahead` and `Abeam` in cm on a line 20 degrees to starboard of the bow (where the helm view looks). The foot is put on `SampleSurfaceWorld`. `Abeam -= HelmTurn * 1100 * dt`, so the same helm value that turns your bearing slides the spout aside. This is an abstraction, not a route: your plan's spiral channel with a shared path description would replace it.
- Drawn with tunnel volume 01 of the pack through `M_VdbCloud`, rolled 90 degrees and spun. The height follows how far down the slope the foot is (260 to 620 m).
- Missing: disturbed water and spray at the foot, sound, a fallback when the pack is absent (the hazard then runs unseen), a visible tear in the sails (only the existing `TearRigging` trouble), and any dodge by the crew helmsman.

## Not started from your plan

The spiral channel and route frame, the lip and crest anticipation (only the faster descent is in), water shape and foam, boss anatomy and weak-point presentation, fireballs, adds and boarding, the finale's visible collapse. The existing water tunnel and glare at the exit are as you left them.

## Things I learned that bear on your plan

- Heterogeneous volumes: a material sees positions in voxels of the volume (`LocalPosition`), which is how `M_VdbCloud` cuts a floor. The pack's voxels are 0.42 cm at scale 1.
- `bIssueBlockingRequests` on an animated volume costs render-thread time in proportion to playback rate; non-blocking streams a coarser level when the volume is far or large on screen.
- `r.HeterogeneousVolumes.MaxTraceDistance` is set to 6 km by the front and by the spouts (only raised, never lowered). The old hero banks set 1.2 km but are off by default now.
- At 3x or 4x time scale the volumes play 3x or 4x as many frames and the frame rate drops by half; judge speed at normal time.
