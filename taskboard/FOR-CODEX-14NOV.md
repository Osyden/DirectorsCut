# For Codex, Saturday 14 November: what Claude changed in the maelstrom overnight

Read with the STATUS entries of 13 Nov 23:38 onward. I worked on main in small commits. The claim on the maelstrom files ends when `taskboard/MORNING-14NOV.md` no longer opens with a line saying it is a draft (08:00 at the latest); after that they are yours again unless the owner says otherwise.

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

## Burning stones (added 00:28)

`FBoulder`, `UpdateBoulders` in `DCMaelstrom`: five scheduled stones thrown from the abyss in a 5 s arc, dodged with the helm exactly as the spouts are, starting a deck fire through `ADCTroubles::StartFire` when they strike. `-DCNoBoulders`, `-DCBoulderAt=`. Plain glowing spheres; they want smoke, a splash and sound.

## The creature's young (added 02:02)

`FSpawnling`, `UpdateSpawnlings` in `DCMaelstrom`: two scheduled adds on the beam, registered with `ADCTreadmill::AddExternalHostile` so the existing batteries and scoring handle them; at the hull they do damage, tear the rigging and start a fire. No boarding on deck. The mesh is the Claw Kaiju with `M_Kraken` on every slot, because `M_ClawKaiju` and its instances draw pale jade in the game (worth knowing before you show any of the boss's body).

## Shots stopped by the hidden sea (found 02:35)

In the maelstrom the hidden `BP_DynamicRealWater` sheet at sea level still blocked the gunnery line trace, so balls fired below the ship's level burst a few metres off her side. `DCGunnery.cpp` now passes a level hit at sea level while the maelstrom is active. If you place targets down the slope, they can now be reached.

## Over the lip (added 03:34)

`UpdateStage`: `Descent` starts at 12 s; the arena is pitched about the ship by up to 6 degrees between 12 and 32 s (`Over` quaternion applied to both its location and rotation). Anything that assumes the arena is only yawed should be checked; `SampleSurfaceWorld` is fine. There is still no near crest hiding the drop.

## The creature at the end (added 06:31)

`UpdatePlume` also raises the Claw Kaiju body (`Carcass`, scale 18, `M_Kraken` on every slot, Defeated at half rate) at the crater's centre for the 19 s of the plume. Your `ADCLeviathan::Body` stays hidden as you left it; this is a separate actor.

## The end (added 00:52)

`UpdatePlume` in `DCMaelstrom`: from `FinaleAt` for 19 s a kilometre-high column of the same volume stands on the crater's centre (`Arena` location on `SampleSurfaceWorld`), and `Director->Trauma` is kicked once. It is the only visible mark of the defeat besides the arms withdrawing; your plan's collapse of the current and opening of the path is not done.

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
