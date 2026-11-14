# Maelstrom trajectory and contact pass — 14 November

Branch: `codex/maelstrom-saturday`, based on `cf49b3b`. Isolated worktree: `E:/Projects/DirectorsCut/Maelstrom-Saturday`.

## Changes

- Replaced the nine-degree scenery drift with a continuous descending orbit: a lip pause, descent to about 1.18 km radius, then roughly 76 degrees of travel before the timed finale. The ship remains in its existing world frame; the crater follows the inverse route transform.
- Added lateral steering velocity, damping and a restoring current at the banks. Full held rudder settles approximately 71 m across the channel. Broad, uneven standing-wave banks define the path; the inner shoulder is low enough to retain the view into the abyss.
- Spouts and falling stones now use the same steering displacement as the route. The spout hit check uses its rendered position. Narrowed the spout and its oversized fog skirt so the visible hazard better matches the area to avoid.
- Swimming creatures stop outside a conservative mesh-bounds envelope, strike with a wake, then withdraw. Tentacle geometry and strikes stay outboard of the hull. Original spray particles, wake bursts and the installed water-impact sound accompany emergence and strikes. Particles remain in the moving basin frame.
- Prefer the existing installed UDIM `LeviathanSkin` material for the Claw Kaiju swimmers/finale, with fallback. Added original wet arm and lit wound materials; reduced the oversized unlit target bulbs.
- Replaced polar contour foam mapping with smaller warped planar patches, irregular crossing swells and crest masks that exclude channel shaping. Reduced the flat-ocean simulation's foam contribution. The live FFT detail bridge remains optional.
- Corrected the tilted basin's world-vertical height query. Hidden DRW ocean actors also have collision disabled during this stage, with their previous collision state restored on exit, so invisible water cannot block manual aim.
- Moved the stage tick before camera caching: the intended helm offset and abyss framing now appear. Preserved mouse look and the player's C-key override. Clear encounter hazards on exit.
- Removed the half-million-triangle water surface from dynamic shadow casting after the on-screen VSM marking overflow. Local landmarks still cast shadows. Full crater self-shadowing needs a cheaper proxy if required later.

## Ownership and installation

Only maelstrom/leviathan source, the water bridge, original generated materials, route tests and review tools are changed. No early-game, treadmill, crew, player, gunnery, HUD, global sky/weather or purchased source assets were edited.

This worktree shares installed content through junctions but has independent plugin binaries. Do not junction `Plugins` into an open editor's project: UnrealBuildTool may rebuild those DLLs. Do not run `Tools/build.sh` here; it names the main project explicitly.

Original material generator: `Tools/make_maelstrom_contact.py`. The water generator remains `Tools/make_maelstrom_materials.py`. Generated original materials are included. `Content/DC/Maelstrom/LeviathanSkin` and other purchased assets remain ignored; the existing skin generator is unchanged.

## Validation

- UE 5.8 `DirectorsCutEditor Win64 Development` builds successfully.
- UE automation `DirectorsCut.Maelstrom.RouteAndSteering` passes: route continuity, angular seam/inverse consistency, sufficient orbit, full-rudder containment on both sides, release damping and 30/120 fps agreement.
- Normal-speed held-starboard run: offset settled at 70.9 m; a deliberately unopposed swimmer performed its wake attack outside the hull and withdrew. Close-up posed mesh bounds put its nearest edge roughly 39 m from the ship centre during the attack. Water strikes no longer start deck fires; burning stones still do.
- Continuous double-speed encounter reached Ending at 450 seconds and the open-sea capture at 451 seconds, score 2450 and hull 40 with automatic batteries. This preceded the final spray texture, channel slope and spout-width refinements. It is a progression check, not a balance verdict.
- Normal-speed contact captures at 342.3–344.1 seconds confirm spray at the arm/water intersection. Replaced an obvious procedural chevron in the first spray material with smooth noise before the final captures.
- The continuous review caught black scene frames at 350/380 seconds. The arm shader used `pow` with a potentially negative base for squared distance; replaced it with multiplication to avoid invalid shader values. The repeated 350/380/430/440/451-second captures with waterspouts enabled render correctly after that correction.
- Final normal-speed run (`Saved/FiniteOverview.log`), starting at ride time 320: both swimmers sunk, 11 arms broken, timed defeat at 430, Ending at 450 and visible open sea at 451. Automatic batteries, no steering. Latest source/materials; this covers the remaining encounter and exit, not the whole game or the initial reveal. Samples were 38–72 fps during this 1600x900 run, with spout streaming still affecting render time; this is not a standalone 4K benchmark.
- New materials generate successfully. Initial continuous test reached the late encounter but the shared Zen cache stalled; subsequent runs use `-DDC=InstalledNoZenLocalFallback`. This is a test-process option, not a project-wide config change.

Offscreen helper: `Tools/review_maelstrom.ps1 -Continuous -Start 279`. Optional `-Steer -1` or `-Steer 1` holds rudder for a containment check; `-NoFFT` checks the fallback shader. Sound is off unless `-Sound` is supplied. Logs and screenshots are in `Saved/`.

## Remaining quality bar

This is a movement/contact correction, not a claim that the scene meets the concept's cinematic quality. The procedural arms still need stronger organic shape and animation, the crater's water/reflections need further art review in motion, and the purchased VDB spouts still require frame-time profiling. Audio is connected but not auditioned in these silent runs. The early-game balance and existing rigging/fire systems are unchanged.

Review captures: [orbit at 325 s](review/maelstrom-orbit-14nov/orbit-325.png), [orbit at 350 s](review/maelstrom-orbit-14nov/orbit-350.png), [water contact at 343.1 s](review/maelstrom-orbit-14nov/contact-343.png). These are evidence of the current state, not beauty shots. The contact image uses a test camera; it does not establish that the default cannon camera frames every target well.

[Finale at 440 s](review/maelstrom-orbit-14nov/finale-440.png) and [open-sea cut at 451 s](review/maelstrom-orbit-14nov/sea-exit-451.png). The finale plume is still too dense and dominant; improve its breakup and framing alongside the water/arm art pass. Some rim edges also show temporal breakup that needs a dedicated antialiasing/reflection comparison.

Next visual pass, in order:

1. Replace the smooth hooked arm silhouette with an articulated curl, asymmetric thickness and visible suction-cup relief. Keep the outboard clearance and actual water-intersection spray from this pass. The current arms still read like rubber tubes with hooked ends.
2. Diagnose large white patches with separate foam/reflection captures, then reduce distant high-frequency detail and introduce longer directional foam streaks. Preserve readable dark troughs and the descending route. Do not restore concentric contour bands.
3. Profile streaming of the spout volume frames at target resolution before adding more volumes. Narrowing its silhouette is not a streaming-performance fix.
4. Play with sound at normal speed: test manual cannon aim on both banks, helm-to-gun transitions, recovery from a missed hazard, and the timed exit. Automated firing and still captures cannot establish that the fight feels good.

Review this branch in its own editor project, or merge and rebuild the active project. The overnight `0748_69449a3_handover` package predates all these changes. No new packaged release was made by this pass. Existing editor-toolset Python initialization errors are still present in `-game` logs; the passing route test is not a claim that the entire log is error-free.
