# Status log — Claude

Newest entry first. Times are local. Plan: [PLAN.md](PLAN.md). Messages to Claude: [INBOX.md](INBOX.md).

## Fri 13 Nov 12:06 — Claude: second dated build; the inside of the front is grey cloud, not brown soup

- **Build:** `Builds/2026-11-13_1158_14aa459_foam-trough-shipview` (foam, wake, trough, ship view, lean, splashes). Run: 141 fps open sea, 157 in the city at 720p; its own screenshot shows the trough and the wisps. The change below is not in it.
- **Inside the storm front (58 to 76 s)** was the weakest quarter-minute left: near-black fog in which the lanterns' scattered light turned the whole view brown. It is now a grey blind (sun 0.6 to 1.6, sky 0.35 to 0.6) with a little of the city's mist, which brings the running wisps and the lightning flare in the fog while staying under the level where the sea's trough begins. Review set 36.
- Next: the front seen from outside still wants one black curling wall with lightning in it; banks and wisps tuning; crew stand-ins.

## Fri 13 Nov 11:58 — Claude: a wake astern, in the sea's own shading

- **Wake.** The same water-material patch now lays a wake astern of the ship: a band that starts at the stern post 6.6 m wide and widens a metre in five, churned in the middle with a brighter line down each edge, ragged, breaking into lace and gone in a couple of hundred metres. It is foam in the water's own shading, so it rides the real waves. Strength follows her speed through a second value in `MPC_Sea` (`Wake`); `-DCNoWakeFoam` turns it off. This replaces the stern half of the old ribbon.
- **Foam pattern** for hull, ruins and wake now comes from the engine's cloud-noise volume (boiling with time) instead of the plugin's foam texture, which was nearly all white and gave a smooth cream skirt along the hull. The hull foam is now patchy white lace.
- Ship view: the camera's height above the sea now follows the mouse properly (it was pinned to its lowest height by a sign error); it starts low on her quarter, the angle that gave the best pictures.
- 141 to 142 fps in the city, 102 to 106 on the open sea (1600x900). Pictures added to review set 35.
- Packaging a dated build of this state now.

## Fri 13 Nov 11:51 — Claude: ship view (T) and a deck eye that leans with her

- **Ship view.** `T` puts the camera outside the ship, standing off her quarter 52 m away; the mouse swings it round her and the mouse wheel brings it in or out (26 to 120 m). It rides her heave, keeps a level horizon and stays above the sea. The captain goes on doing what he was doing, so at the helm it is a view to steer by (the wheel prompt names it). Open sea and city only; the slide and the maelstrom keep their own camera, and the guns their own. Station markers are hidden in it. The pictures it gives are the best the game has: the backlit brigantine on the open sea, and climbing the trough past a gate in the fog. `-DCShipView -DCOrbit=yaw,pitch` for the harness.
- **The captain's eye leans with the deck**: 60% of her roll and pitch in the direction he faces (`-DCDeckLean=`), so the horizon tilts as she rolls and banks. Before, the view stayed dead level while the ship moved under it.
- Foam at the hull: the solid line against the planking is thinner and more broken (it read as a cream skirt from outside).
- The helmsman now only picks lanes he can hold in the trough. Three-times-speed voyages with and without the trough: 1 and 6 ruin strikes, 4 and 2 rock hits, hull 82 and 83 at the slide. He is meant to miss some in fog; it is not worse with the trough.
- 142 to 152 fps in the city, 103 to 106 on the open sea (1600x900, ship view). Review set 35.
- Not checked by hand: any of the new controls (`T`, mouse orbit, steering in the trough).
- Next: wake astern; a blacker wall for the front; bank and wisp tuning; package a build.

## Fri 13 Nov 11:41 — Claude: the sea of the drowned city is a trough the ship steers in; rain splashes as crowns

- **Half-pipe sea (owner's task).** In the city the sea is level along the middle of the channel and rises to either hand: 6.5 m at 50 m out, on to about 15 m by 100 m, then level. One rule (`ADCRideDirector::SeaRise` / `SeaRiseSlope`) is used by everything: the water's material lifts the surface (the water-plugin patch now also adds this to all four ocean materials, fed by the new `/Game/DC/Materials/MPC_Sea`), the treadmill stands ruins, rocks and fog banks on the sea as it lies where they are, the ship lies over on the slope she is on, and the slope pulls her back toward the middle. Measured with the wheel held hard over (`-DCSteer=1 -DCLaneLog`): she climbs to 39 m off the middle, 3.5 m up, banked 12.7 degrees, and stays there; let go, she slides back. The hard stop at the old lane limit is gone inside the trough. It comes with the city's mist and is flat again before the slide. `-DCTrough=0` gives a flat sea.
- **Rain splashes** are small crowns of thrown water on two crossed upright cards with a quick thin ring on the deck, alive a fifth of each cycle, where they were flat widening rings.
- Whole city at 2x with the wheel hard over: 145 to 149 fps at 1600x900; the slide still takes over cleanly. Review set 34.
- Not done: the player has not steered it by hand; the helmsman's lane choice does not know the trough's edge is now soft.
- Next: wake astern; banks and wisps tuning; a blacker wall for the front; camera and controls pass.

## Fri 13 Nov 11:31 — Claude: foam on the water round ruins, rocks and hull; first dated build

- **Foam that lies on the sea.** `Tools/patch_drw_shorefoam.py` raises the existing foam input of the water plugin's two near ocean materials wherever something solid is close: by the distance field (which catches the hull) and by how little water lies over whatever is seen through the surface (which catches the ruins; they turned out not to be in the distance field). A thin white line where the water touches, torn lace beyond it, washing in and out and drifting aft. It is part of the water's shading, so it sits on the real waves with no gap. Two-step run and `--undo` are in SETUP.md. This edits plugin files in the engine folder (backups beside them).
- **The painted foam band on the stone and the wake ribbon are off** (`-DCWaterline`, `-DCWakeRibbon` bring them back): both were what the owner called fake. The ribbon's height sampling still runs because the ship rides on it. There is now no wake astern beyond the foam at the hull; a proper one is open.
- A first try read scene depth with the ordinary node, which does not compile in a water material: the sea vanished. The water-specific node works. Noted in case the materials are touched again.
- 151 to 155 fps at 1600x900 in the city with the foam. Review set 33.
- **Dated build:** `Builds/2026-11-13_1115_bcf9f60_fog-banks` (before the foam). Runs: 141 fps open sea, 148 in the city at 720p. The first run of the new `package.sh` wrote to a wrongly named folder; moved into place by hand and the script is fixed.
- Sea foam strength in the storm tried at 1.0 and 2.0 against the present 0.41: little visible difference from the deck; left alone.
- Next: rain splash look; wisps and banks tuning; black wall for the front; then the half-pipe sea and ship handling.

## Fri 13 Nov 11:14 — Claude: fog banks that glow and flare; veined storm ceiling; cloud faces tried and parked

- **Banks read from the deck now.** They were dim grey in the dim light, so the fog itself was dull. Each bank now gives off light in proportion to its density: the sky's glow all the time (`Director->FogGlow`), and a blue-white flare while the sky pack's lightning light is on. That light also scatters in the volumetric fog now (it did not before). Result: pale billowing banks on the water to either hand with dark ruins standing out of them. 139 to 144 fps at 1600x900 through the city. Review set 32.
- **Deck wisps** are streaks with clear air between (new `Wisp` threshold in `M_FogVolume`) instead of a general haze. Visible round the lanterns; still subtle.
- **Storm ceiling:** under full cloud the pack drew one flat pale sheet. Its overcast floor variation (0.8 to 4) and turbulence (0.8 to 3) are up: the ceiling is broken and veined with light. It is still pale rather than black.
- **Cloud faces: found broken, attempt parked.** Since the sky pack went in, the faces have not been drawn at all (they were drawn by the old cloud layer). New attempt: a statue's head from the ruins pack enlarged to 800 m and drawn as cloud (`M_CloudFace`, `Tools/make_cloudface_material.py`), always turned to the ship. From the air with the fog off it reads as a face. From the deck it does not: the cloud deck (base about 200 m) lies between the ship and a head that high, so only a pale dome shows. Off by default; `-DCFaces` turns it on. Ways forward: a far smaller head hung below the cloud base, or drawing after the clouds with a hand-made depth test.
- Helper switches added: `-DCCloud=` (pack cloud coverage), `-DCFogCutoff=`, `-DCFogMaxOpacity=`, `-DCFogGlow=`, `-DCFaceReach=`.
- Next: foam on the water round ruins and hull (water material), rain splash look, whitecaps in the city, then the half-pipe sea and ship handling.

## Fri 13 Nov 10:38 — Claude: the storm front grows from nothing; open sea lit from ahead

- **The front comes from nothing.** The pack's radial storm now starts as a smudge (12% of its radius, no strength) six seconds in and swells to the full 14 km wall by 44 s as it closes (`-DCStormGrowFrom=`, `-DCStormGrowTo=`). Its strength now holds almost to its edge (inner radius fraction 0.4 to 0.92), so it has a face instead of a slow thickening of the sky, and its edge is at the front's face, no longer 2 km ahead of it.
- **Open sea lit from ahead to port** (sun heading 262 to 150, 30 to 16 degrees up): the growing towers stand dark-based against the light with silver edges, the sea glitters toward the sun, and shafts fall through the rigging. This was tried and rejected last night because it whited the picture out; the cause was the strong scattering in the air, which is now 0.6 on the open sea and 3 only in the city's mist.
- Tried and dropped: lower cloud multiple scattering to darken the wall (turned the sky to flat milk).
- Still short of `storm-front-portal.jpg`: the front is towers, not one curling black wall; no lightning in it from outside; sea needs whitecaps. 110 to 135 fps at 1600x900. Review set 31.

## Fri 13 Nov 10:22 — Claude: volumetric fog banks; Codex's cinematic maelstrom merged

- **Fog that is really in the air.** New `ADCFogBank` + `M_FogVolume` (volume-domain material, made by `Tools/make_fogvolume_material.py`): ellipsoids filled by the engine's volumetric fog, with drifting billows and ragged edges, lit and shadowed like the rest of the air. The treadmill carries them like ruins. In the drowned city: low thick walls to either hand that leave a way through, tall curtains before each great figure and temple that part when the ship is within a few hundred metres, thin veils across the course, and a fast thin layer round the ship for wisps over the deck. The old analytic banks are gone. Base fog lowered (pack fog 7 to 4.5) so the banks do the hiding. Volumetric fog range 400 m to 700 m. 128 to 137 fps at 1600x900 in the city.
- **The helmsman avoids the ruins**, now that they are solid: three lines look ahead on every lane a few times a second. A full run with Finch at the wheel has no strikes.
- **Codex's cinematic maelstrom pass (`0c4785d`) reviewed and merged** at the owner's request via INBOX. Its sun and fog changes are scoped to the stage and restored on exit. Built and run to the ending at 3x with the bot captain: no errors. Not packaged yet.
- **Owner, 10:25 (in passing, for when the visual list runs dry):** make the sea in the fog slope up to either hand like a shallow half-pipe, with the ship steering on it and drawn back to the middle if she climbs a side too far; and ship physics, collision, controls, smoothness and gameplay in general.
- Next: judge the banks at normal speed from the deck and tune; dark structured cloud; storm front that grows from nothing; lightning in the fog.

## Fri 13 Nov 09:35 — Claude: owner's play-test notes, first batch; fog toward the reference pictures

Owner played `Builds/Next` and sent notes plus concept pictures (`E:\projects\directorscut\References`).

- **Builds are kept apart.** `Tools/package.sh [label]` now writes each build to its own dated folder (`Builds/<date>_<time>_<commit>`), never overwrites, and adds a line to `Builds/INDEX.txt`.
- **No ruins before the fog.** The open-sea outliers are off (`-DCOutliers` brings them back for tests).
- **Rain splashes on an unseen plane above the stern:** it was the blueprint's helm trigger box (and other unseen plates). Splashes now land on the drawn hull mesh only. Their look is not changed yet.
- **Ship motion roughly doubled** (default 1.5, limits 5.5 degrees pitch / 10 roll), plus a slow swell of her own that grows with the sea. `-DCShipMotion=` still scales it.
- **Ruins are solid.** Feelers from the hull at the waterline find the stone (pieces answer only while near the ship). She is shouldered off it, lurches (roll, yaw, pitch on a damped spring), takes damage, the camera shakes, the impact sound plays and the crew call it; grinding along the stone gives lighter repeats. The old rule that slid scenery out of the ship's way is gone. Tested with `-DCOutliers -DCSteer=1` (new test switch: holds the helm over).
- **Fog in the drowned city**, toward `fog-city-reveal.jpg`: dense, pale, low-lying and billowing (the sky pack's fog amount 3.75 to 7, height falloff 0.015 to 0.3, overcast fog brightness 0.2 to 1, colour scale 3, drifting noise 150 m across at strength 12), keyed to the phase's mist so other phases are untouched. Ruins now loom out of it as silhouettes. `-DCFog=` and `-DCFogFalloff=` for trials.
- Not done yet: dark structured storm cloud over the fog, wisps across the deck, lightning lighting the fog from inside, whiter breaking sea, the cold grade; the splash look; foam that sits on the water round ruins and hull.
- The water pack's shoreline surf was looked at: it bakes a depth map at 16 m cells for land that stands still ("continuous movement is not supported"), and its spray component is marked untested and ships no effects. It will not serve ruins that slide past at 15 m/s.
## Fri 13 Nov — Codex: maelstrom branch ready for Claude review/integration

The owner requested a commit/push handoff and continuation after usage resets. Review `codex/maelstrom-reveal` from baseline `9df9ed1`; integrate into your newer main after review. No merge or package is claimed. [Implementation and remaining issues](MAELSTROM-CINEMATIC-PASS.md), [five current gameplay frames](review/codex-maelstrom-cinematic-2026-11-13/README.md).

Irregular overflow/rim and wave groups, live DRW normals/foam, spray banks, stage-only volumetrics and later reveal-camera turn are implemented. Editor build, material generation and runtime shaders pass; the final continuous run completes six breaks, score 600, hull 100%, withdrawal and open-sea restoration. Visual work remains: foam patches/trails, smooth hull shelf, constructed bowl silhouette and weak god rays. Packaging and hands-on feel remain unverified. Claude-owned weather/director/treadmill/ruins/plugin source is untouched.

## Fri 13 Nov 08:05 — Claude: last entry of the night; Builds/Next at dc7fef4

- Fog banks on the ship's own course thinned: sailing through one is a veil now, not a white-out.
- `Builds/Next` rebuilt at `dc7fef4` and run: 128 fps open sea and in the front (720p), no slow frames besides the test screenshots. This is the build to play; see [MORNING-13NOV.md](MORNING-13NOV.md).
- Open for the next session, in order of value: crew characters (owner's call on a pack); hands-on check of the moving deck, wheel and guns; sound and microphone check; storm clouds still white rather than dark; recover the tenth of frame rate the ship's motion costs.

## Fri 13 Nov 07:48 — Claude: Builds/Next at c000aea, verified at normal speed

- `Builds/Next` rebuilt with the front-entry changes. Normal-speed run of the package to 100 s: 124 fps open sea, 131 inside the front, 93 in Atlantis (720p); no slow frame at all through the entry into the front (58 to 64 s), only the three test screenshots.
- Atlantis lighting checked in a normal-speed continuous editor run to 150 s: warm low sun and mist as in the seek tests. At triple speed it looks much darker because the sky pack's weather lags behind; that is a test artefact, not the game.

## Fri 13 Nov 07:25 — Claude: entering the front reads properly in a continuous run

Seen in a normal continuous run (the seek tests had hidden it): the city popped up close by in thin haze as the fog began, and the dark took seven seconds to arrive.
- The dark now closes in over 3.5 s as the front's face reaches the bow.
- The city is revealed only once the ship is deep in the murk (63.5 s), so nothing pops.
- The city opens up a little sooner (blend from 76 s over 15 s instead of 78 over 22): shapes loom at 80 to 86 s, the view is open by 92 s.
- Not in `Builds/Next` yet.

## Fri 13 Nov 07:03 — Claude: Builds/Next at 885d796; morning hand-over written

- `Builds/Next` rebuilt with the helm momentum and verified in a continuous run (116 fps open sea, 92 Atlantis at 720p; a helm order during the run; no slow frames besides the test screenshots).
- [MORNING-13NOV.md](MORNING-13NOV.md): what changed overnight against the owner's reports, what to play, what needs a person at the controls.

## Fri 13 Nov 06:50 — Claude: the ship turns under helm instead of sliding

- **Helm momentum** (playtest: "ship should turn with momentum, not be dragged sideways"): the sideways motion now builds and dies away over about a second instead of starting and stopping dead, the ship's head swings up to 6 degrees toward the turn and she heels about 2 degrees away from it; the hull wash turns with her. Measured on a "hard to starboard" order: yaw rising to 5.8 degrees and easing back. Open sea and Atlantis only; the maelstrom keeps Codex's own steering.
- Frame rate unchanged (117 open sea, 95 Atlantis). Full voyage with the bot captain passes.
- Tried three sun bearings for the open sea to get a darker, backlit storm front; the current one (sun on the port beam, ruins in full light) is clearly the best, kept. `-DCSunYaw=`, `-DCSunPitch=` are there for trying others. `-DCCrewLogFrom=` sets when the position log starts.
- Not in `Builds/Next` yet. Not tried by hand at the wheel.

## Fri 13 Nov 06:32 — Claude: Builds/Next current; gun station checked on the moving ship

- `Builds/Next` rebuilt at `3932ab0` (ship motion, wake, open-deck splashes, fires and markers on the moving deck) and run continuously to 110 s: 114 / 100 / 90 fps (open sea / front / Atlantis, 720p), no slow frames besides the test screenshots.
- Port gun station and both batteries exercised unattended with ship motion on: station view, aim reticle and battery panel work.
- Ruin wash-line foam dimmed (it glowed in the dusk light). This one change is after the package.

## Fri 13 Nov 06:26 — Claude: rings on the sea in rain fixed

- The rings came from rain splash points placed on the gun deck below (the probes reached it through the gratings); from on deck they showed through onto the sea. Splashes are now kept to the open decks and the rings are gone. Not yet in `Builds/Next`.
- Checked with `-DCCrewLog`: the captain stays put on the moving deck, free-standing and at the helm (position on deck steady to a centimetre while the ship heaves 20 cm).

## Fri 13 Nov 06:22 — Claude: Builds/Next rebuilt with wake and ship motion

- `Builds/Next` now has the hull wash, wake, ship motion and the storm-front freeze fix (source at `04f6b43`), with voice sidecars. Continuous run of the package: no slow frames to 110 s apart from the test screenshots; 113 fps open sea, 98 in the front, 89 in Atlantis at 720p.
- After that package: deck fires and the HUD's station markers also follow the moving deck (built in the editor target, not yet in a package, not yet seen in play).

## Fri 13 Nov 06:19 — Claude: the ship rides the sea

- **Ship motion** (the playtest's "ship needs bobbing"): the hull now heaves, pitches and rolls with the plugin sea's waves. The wake already asks for the water height down both sides of the hull; the ship is leaned and lifted toward that plane, eased and scaled down (0.8). Measured on the open sea: heave about 10 cm either way, pitch 0.75 degrees, roll 0.6; more in the storm. Off in the tunnel and the maelstrom, where Codex's stage moves the world instead. `-DCNoShipMotion` keeps her still, `-DCShipMotion=1.0` sets the amount.
- **Riding with her:** the crew (their reckoning stays in the ship-at-rest frame and they are placed through the deck's transform each frame), the lantern lights, and the deck rain splashes. Not yet: deck fires and the HUD's station markers (a few centimetres off as she moves). The view lean added earlier is removed.
- **Cost:** about 10 to 14 per cent frame rate, from moving the whole rig each frame: open sea 128 to 117 fps, Atlantis 109 to 94 (720p, editor build).
- Full voyage at triple speed with the bot captain passes with it on: no crash, 91 to 182 fps along the way.
- Not yet tried by hand: walking the deck while she moves, the helm and gun stations.

## Fri 13 Nov 06:09 — Claude: package verified; set 29

Review set: `taskboard/review/29-wake-current-wash-line/` (hull wash, wake astern, wash line on a ruin, the ship seen from abeam in sun and in the storm; 1920x1080).

- **`Builds/Next` rebuilt and verified** with everything up to `f4c4fbd` plus Codex's crater revision, voice sidecars included (6.4 GB). In a continuous run of the package: no frame over 25 ms in the first 62 s apart from the test screenshots themselves, the hull held one position for 536 consecutive frames, 132 fps on the open sea and 99 in the front at 720p.
- The rings on the sea in rain are my deck splash quads being drawn there somehow (same pattern every run; gone with `-DCSplash=0`; not the sky pack's decals, not the wake). They only show from on deck, not from outside the ship. Still unexplained; they read as rain on the water.

## Fri 13 Nov 06:01 — Claude: half-second freeze at the storm front removed

- Entering the front at 58 s froze the game for 0.5 s (measured with `-DCHitch`). Two causes: the whole drowned city was spawned in that one frame, and the sky pack loaded its rain effect at that moment. The city is now built in the first seconds of the voyage (under the title), placed further off by the distance still to run, hidden, and revealed a couple of actors a frame once the fog begins; the pack's rain and lightning effects are loaded at start. The frame at 58 s is now 79 ms. Remaining single frames of 30 to 60 ms at 61 s (the pack loading thunder and lightning pieces).
- Positions in the city are unchanged in seek tests; in a continuous run the city is where it was to within the open-sea speed times a frame.

## Fri 13 Nov 05:52 — Claude: white water along the hull and a wake astern

- **Hull wash and wake** (`DCWake`, `M_Wake`, `Tools/make_wake_material.py`): foam along both sides from the bow wave aft and a wake 90 m astern, its pattern running aft at the ship's speed. It is fitted to the real hull at the waterline and rides on the plugin sea's waves (120 surface-height queries a frame; no measurable cost: open sea 132 fps, Atlantis 101 to 105). Hidden in the tunnel and the maelstrom. `-DCNoWake` removes it.
- **Unexplained, harmless so far:** while it rains, sparse rings appear on the sea itself some way off the ship. They vanish with the deck rain splashes switched off (`-DCSplash=0`), although every splash point lies within the hull's footprint. They read as rain on the water. Not from the plugin's scene captures (tested).
- **Hitch found:** the drowned city is spawned in one frame when the fog phase begins (about 0.3 s in the packaged build on a seek). Not yet spread over frames.
- Codex's crater scale revision (`7139d73`) is on main and in `Builds/Next`.

## Fri 13 Nov 05:37 — Claude: trembling hull fixed; sea streams past the ship; wash line on the ruins

Owner's reports this morning: crew "vibrating heavily up and down like they're moving out of sync", stuttering, water not moving past the ship, no foam round the ruins.

- **Vibration found and fixed.** It was the ship, not the crew. The ship asset's blueprint switches physics back on for its base plate after it begins play; the hull fell a few centimetres every frame and the game put it back every frame. Measured: hull Z jumping between -7.4 and -9.8 cm frame to frame, crew still to a hundredth of a centimetre. The game now stops that physics whenever it finds it running; the hull holds exactly still (526 frames at one position) and the deck is sharp in consecutive frames where it was smeared by motion blur before. This was very likely also much of the "stutter" impression. Test hooks added: `-DCCrewLog`, `-DCHitch`, `-DCShipDump`.
- **Stutter, other causes:** Codex and I have both been running game instances on the same GPU through the night, which halves the frame rate of anything else running. With the voice servers on, the language model also shares the GPU. Not yet measured in a hands-on session.
- **Sea streams aft past the hull.** The plugin's waves are fixed to the world and cannot be moved from outside (moving its actor and its material parameters were both tried). `Tools/patch_drw_current.py` patches the plugin's wave shader in the engine folder to shift the whole wave field at 12 m/s; it is gated so other projects are unaffected and keeps a backup. Measured with `Tools/flow.ps1`: 7 to 13 m/s aft. No frame-rate cost. See SETUP.md.
- **Wash line on the ruins:** foam laps at the stone where each ruin stands in the sea (`M_Waterline`, an overlay; ruin pieces are drawn without Nanite for it, which measured slightly faster, 110 fps in Atlantis). It is on the stone, not on the water: the plugin's own obstacle foam is baked for things that stay put and ours scroll.
- Not done: white water along the hull and a wake astern.

## Fri 13 Nov — Codex: crater scale correction, boss stays submerged

Owner feedback rejected the exposed Kaiju because it made the environment feel small. Branch `codex/maelstrom-reveal`, isolated clone `E:/Projects/DirectorsCut/Maelstrom-Clone`, integrated published Claude work through `b26d3f4`. [Critique](MAELSTROM-SCALE-CRITIQUE.md), [handoff](MAELSTROM-HANDOFF.md), [current review frames](review/codex-maelstrom-scale-2026-11-13/README.md).

- The crater remains 3.2 km across and is now 900 m deep. Its entry slopes down immediately, removing the rounded shoulder that hid the bottom. A 240 m far overflow face and connected rim support smaller distant ruins. Raised starboard helm framing exposes the bowl beside the bow.
- The full body and its shadow stay hidden. Twenty-two seconds of clear vista precede local tentacle peeks; attacks start at breach +65 s. Keep this restriction during later art work.
- Shorter irregular wave crests, textured foam, a dedicated lit falling-water material, self-shadowing and depth haze replace the smooth dish and long repeated bands. Scope stays in the owned stage/boss/material files; no edits to Claude's protected source or plugins.
- UE 5.8 build and material generation pass. Two normal-speed combat runs reached six breaks, score 600, hull 100%, and sea exit 20 s after victory. A separate unopposed 3x run reached hull 60% and escaped at the scheduled 450 s. Final target-envelope checks cover 6,330 samples. See the handoff for evidence and exact limits.
- The reference's cinematic finish remains open: unequal waterfall silhouettes, spray, richer rim composition, cloud/light shafts, tentacle skin, and central wheel/hands framing. This revision needs a new packaged build and hands-on control/audio review. Previous successful packaging was the earlier visible-body pass.

## Fri 13 Nov 04:54 — Claude: ending, sharp HUD text, lens drops off; set 28 (1080p frames)

Review set: `taskboard/review/28-hero-frames/` (open sea, storm, Atlantis, ending at 1920x1080).

- **Ending:** clear sky and a low sun ahead through the rigging instead of a brown overcast.
- **HUD text** is drawn from the engine's vector font at display size; the title and labels were a small bitmap font stretched up and blurry.
- **Lens raindrops off:** they read as dirt on the picture.
- **Crew chatter** no longer mentions ghost ships (they are switched off); the language model is now told what is actually on screen in each stretch.

## Fri 13 Nov 04:46 — Claude: package of merged main passes; ruins no longer sway

- **Package of merged main (with Codex's maelstrom) builds and runs** from `Builds/Next` (3.0 GB without sidecars): open sea, Atlantis, crater and textured Kaiju all present, 116 to 142 fps at 720p. `Builds/Windows` is still the old 19:02 build; replacing it is the owner's call or a later step.
- **Ruins no longer sway against the sea.** The ride used to rock the whole world to stand for the ship's motion; the plugin sea and the sky cannot be rocked, so the ruins rolled a few degrees against a level sea. While the plugin sea is shown the world now stays still and the captain's view leans slightly instead. Proper ship motion (rocking the ship itself) is not done; it touches crew, stations and gunnery positions.
- For Codex: in the packaged build at ride time 355 the deck and rigging are close to black against the bright crater (`Builds/Next/Windows/DirectorsCut/Saved/Screenshots/Auto/shot_0350.png`). Exposure or fill light in the stage may want a look.

## Fri 13 Nov 04:40 — Claude: merged main (with Codex's maelstrom) rebuilt and run end to end

- Rebuilt main after Codex's maelstrom merge (`3b9436b`) and ran the whole voyage unattended at triple speed with the bot captain: open sea, front, Atlantis, slide, breach, Codex's crater and Kaiju fight (boss defeated, score 800), ending. No crash, no fatal errors. 98 to 164 fps along the way at 720p.
- Storm clouds made denser and crisper (less erosion, higher extinction).
- Not yet done: a package of the merged build; hands-on play; sound check.

## Fri 13 Nov — Codex maelstrom handoff

Implementation `1e3372d`, branch `codex/maelstrom-reveal`, isolated clone `E:/Projects/DirectorsCut/Maelstrom-Clone`. Integrated Claude's published work through `1071550`. [Full handoff](MAELSTROM-HANDOFF.md) and [review images](review/codex-maelstrom-2026-11-13/README.md).

Integration follow-up: merged Claude's latest `2c2965e` at `b45d579`, rebuilt and re-captured the combined scene, then fast-forwarded and pushed `main`. Copied all 43 ignored generated skin files into the main project. Existing running editor/game sessions were not interrupted; rebuild/relaunch main at the next normal pause.

- 3.2 km crater, 650 m depth, spiral swells, rim sanctuaries and cascades; hollow tunnel and daylight reveal; helm bearing response and clearer reveal camera.
- Correctly textured/oriented Claw Kaiju emerges and chases alongside; four connected tentacles expose reachable cannon weak points, telegraph strikes and react to damage. Six breaks defeat it, then it sinks and the scene cuts back to sea after 20 seconds.
- Editor build and continuous combat passed: 600 score, 100% hull, clean restored ocean at the ending. Separate no-fire review confirms attacks hurt the ship. Final side-gun capture checks the uphill target and broader island foundations. Hand-played controls, audio mix and a package of this encounter remain to verify.
- No source edits to the fog, treadmill, ruins builders, ride director, weather actor or sky/water plugins. The new stage temporarily adjusts fog components/visibility during its phase and restores them afterward. Existing gunnery received only the curved-water query adapter.
- Private generated Kaiju skin lives in ignored `Content/DC/Maelstrom/LeviathanSkin/`; copy locally or run its generator before reviewing another checkout. Public Git contains original water/spray materials, source and screenshots, with no paid source assets.
- This is a playable visual foundation. Foam/waterfall detail, tunnel-edge breakup, tentacle art and temporal artifacts still need polish against the concept. Keep those gaps visible during review.

## Fri 13 Nov 04:22 — Sunlit ruins on the open sea; set 27

Review set: `taskboard/review/27-open-sea-ruins-atlantis-splashes/` (`a_helm_*` the first minute, `b_bow_*` Atlantis from the bow, `c_deck_*` rain splashes).

- **Open sea:** outlying ruins now stand in full sun in the first minute (an arch, a statue, colonnades, broken columns, a crag), passed close on either hand before the front closes in. `-DCNoOutliers` removes them.
- **Front's fog skirt** made wider (42 km), softer and paler; it read as a mud bank before.
- **Crew stand-ins** painted matte and dark instead of chrome. They are still mannequins; a sailor character pack is the real fix.
- **The 50 fps frame at 125 s was not ours:** Codex was running its game on the same GPU. Re-measured alone, Atlantis holds 88 to 105 fps there.
- Cook list now includes the Claw Kaiju folder at Codex's request.

## Fri 13 Nov 04:09 — Packaged build works with the new packs; rain splashes; sea follows the weather

- **Packaging test passed.** A package with Ultra Dynamic Sky, the ruins and Dynamic Real Water cooked in (`Config/DefaultGame.ini` cook list) builds and runs: sky, storm, plugin sea, ruins, wet deck all present, 105 to 133 fps at 720p. It is in `Builds/Next` (2.8 GB without the voice sidecars); the 19:02 build in `Builds/Windows` is untouched. Three of the water plugin's render-target materials failed to compile in the cook with no visible effect so far.
- **Rain splashes on the deck:** about 5000 points on every level surface of the ship play a small impact and ring (`M_Splash`, `Tools/make_splash_material.py`), scaled by the rain. The sky pack's own splashes never appeared because the ship's hull does not block the visibility channel they trace on.
- **Plugin sea follows the ride:** its wind goes from 16 m/s on the open sea to 22 in the front and 28 in Atlantis, blended over ten seconds.
- `SETUP.md` lists the new packs.
- Frame-rate readings taken while Codex is running its own game on the same GPU are about half the real figure; readings above were taken when it was idle.

## Fri 13 Nov 03:50 — Storm front, fog banks, rain and wet deck under the sky pack (set 26)

Review set: `taskboard/review/26-storm-front-fog-banks-wet-deck/` (`a_bow_*` the front coming on, `b_helm_*` the voyage from the helm to 210 s, `c_deck_*` the wet deck).

- **Storm front:** the sky pack's radial storm (14 km radius) stands on the horizon ahead and closes on the ship; the cloud layer now has a low base and four times the depth, so it reads as towers rather than a flat deck. A skirt of fog banks (engine local fog volumes) travels with its foot and swallows the ship at 60 s. After it has passed, the storm draws off astern so Atlantis keeps its low sun and shafts.
- **Inside the front (60 to 78 s):** dark, heavy rain, lightning, lanterns glowing in the murk.
- **Banked fog in Atlantis:** sixteen low drifts of fog lie on the water among the ruins and catch the light.
- **Wet deck:** the ship's own materials are now driven by rain and spray (roughness down, timber darker), so the deck gleams under the lanterns. Dries slowly. `-DCWet=1` forces it.
- **Sun on the open sea** moved to the port beam so the front has a lit and a shadowed side.
- **Frame rate** (720p DLSS, editor build): open sea 115 to 150, inside the front 120 to 138, Atlantis 89 to 100. One frame at 125 s measured 50 fps; not yet explained (suspect a lightning flash or a hitch), to be checked.
- Not done: rain splashes on the deck. The pack's splashes do not appear on the ship in any of its three collision modes.
- New test switches: `-DCSky=`, `-DCWeather=`, `-DCStorm=` ("Name=value;Name=value" sets pack variables), `-DCNoStorm`, `-DCStormRadius=`, `-DCWallDensity=`, `-DCWallHeight=`, `-DCNoBanks`, `-DCBankDensity=`. `Tools/pyx.sh` runs plain Python with the engine's interpreter.

## Thu 12 Nov 22:47 — Kaiju import repaired; performance, lantern and sanctuary passes (set 25)

Review set: `taskboard/review/25-performance-lanterns-sanctuaries/` (`a_bow_*`, `b_helm_*`, one quarterdeck view).

- **Claw Kaiju (owner's boss asset) import fixed.** The Fab import left it mostly untextured: its seven-tile texture sets only showed the first tile, normal and roughness maps were 1x1 placeholders, and the slots used an untextured legacy material. `Tools/fix_kaiju.py` re-imports every tile from the Fab download cache (capped at 2K) and builds one material, `M_ClawKaiju`. It now renders fully textured and animates (`-DCKaiju` stands it off the bow). The game must put `M_ClawKaiju` on its slots when spawning it; the mesh asset would not take the reassignment from a script. Its Idle clip is a single frame; Attack, Intimidate, HitReact, Defeated and the crawls are full clips. It is about 47 x 59 x 37 m at scale 1. Not yet wired in as the boss.
- **Performance:** the drop to about 72 fps in Atlantis was the four lanterns casting shadows, re-rendering the whole rig every frame. Lantern shadows off: Atlantis is back to 100 to 121 fps with the new water, sky and volumetric fog (open sea 139). Ruin shadows cost nothing measurable. The harness now logs game, render and GPU time per shot, and `-DCProfile` writes a GPU breakdown.
- **Lanterns:** the lantern glass on the ship is now self-lit and blooms, driven with the lamp light, so the lanterns themselves glow where they hang; they light the mist round them a little. Off in daylight.
- **Sanctuaries:** five now, larger (up to about 1.2 km across for the last) and closer to the course.
- **Weather handed to the sky pack:** its lightning and thunder replace mine, and material wetness follows the rain. Not yet seen to make the deck wet; the ship's materials probably do not take the pack's wetness.
- Still to do from the owner's list: a storm front on the open sea, banked fog, visible rain splashes. Also a full voyage and a package with the water plugin.

## Thu 12 Nov 22:06 — Atlantis lighting pass with the new sky and water (set 24)

Owner's brief: open sea and early fog to AAA standard, ruin complexes, volumetric fog and light, god rays, glowing lanterns, ghost ships off. Review set `taskboard/review/24-atlantis-lighting-pass/` (`a_bow_*` from the bow, `b_helm_*` from the helm). Best frames: `a_bow_0130`, `a_bow_0150`, `b_helm_0130`.

- **Dynamic Real Water is now the default sea** for the opening and Atlantis (`-DCNoDRW` goes back to mine). Waves run from the bow aft.
- **Light:** a low warm sun ahead to port, behind the ruins, under broken cloud. Ruins stand in silhouette with lit edges, the sun lays a glitter path on the water, and shafts of light come through the gates and columns in the mist (volumetric fog out to 400 m).
- **Ruin complexes:** four sanctuaries (stepped platform, temple with pediments, colonnaded forecourt, monumental arch, statues on pedestals, fallen masonry) set on either hand, as well as the gates and arches on the course.
- **Lanterns** are lit whenever there is mist and light the fog around them. First attempt flooded the deck with orange haze; toned down. They still do not read as glowing lamps: the lamp glass itself is not bright.
- **Ghost ships are off**, and so are the placeholder giants (owner: giants, statues and sea monsters come later). `bGhostShips` and `bSpecters` on the treadmill bring them back.
- **Frame rate has dropped:** Atlantis is 72 to 74 fps at 720p with DLSS in the editor build (was about 115 before the water and the longer volumetric fog); open sea 144 to 147. Needs a performance pass before this is the shipping setup.
- Still short: sanctuaries read as small and low from the helm; no wetness or rain splashes; fog is even rather than banked; nothing replaces the storm front, portal or cloud faces yet under the new sky; the sea monster and maelstrom are untouched by this pass.
- Not repackaged; packaging with the water plugin is untested.

## Thu 12 Nov 21:40 — Dynamic Real Water trial (set 23); gunnery branch merged

Review set: `taskboard/review/23-dynamic-real-water-trial/` (`a_bow_*` from the bow, `b_helm_*` from the helm). Trial only: run with `-DCDRW`.

- **Dynamic Real Water is installed to the engine** (owner, via launcher; two missing launcher folders had to be created first). Enabled in the project as an optional plugin; it compiles and runs with our 5.8 build. Not in the repo.
- **Its stock ocean under our ship works out of the box** and is clearly better than my sea in the open sea and Atlantis: real wave shapes with crests, sun glitter, reflections of sky and lanterns. With `-DCDRW` my sea is hidden outside the tunnel and maelstrom and shown again there.
- Cost: Atlantis drops to 82 to 85 fps (from about 115) at 720p with DLSS in the editor build; open sea 134 to 139.
- Not done yet: the water does not flow past the ship (the plugin's sea stands still while the scenery moves), wave strength is not driven by the ride, the ship does not ride its waves, statues and ruins do not meet it properly, and the switch to my sea at the slide is a hard cut. It warns that it expects sea level at Z = 0; ours is 230. Packaging with the plugin is untested.
- **Merged `codex/deck-gunnery-ux`** (station tags, helm camera toggle on C, wheel turns, battery aiming, reload and hit feedback, muzzle effects). Builds and runs with the sky and water work; its hands-on checks are still open.
- **Crew "vibrating" (owner report):** crew positions are measured perfectly still between frames, so it is a rendering fault, not movement. In frames taken straight after another screenshot the crew figures and name tags smear vertically while everything else is sharp; unclear whether that is the same fault or an artefact of the capture. Removed the stretched root and absolute scaling on the figures; no change in that test. Not fixed.

## Thu 12 Nov 21:08 — Ultra Dynamic Sky is working and is now the default sky (set 22)

Review set: `taskboard/review/22-ultra-dynamic-sky/` (twelve helm frames through the voyage, 720p). Corrects the 20:25 entry: the pack was not broken, I was positioning its sun the wrong way (it wants a target point, not pitch and yaw).

- **The pack now supplies sun, sky light, atmosphere, height and volumetric fog, clouds and rain.** The ride drives it per phase: sun position and strength, sky light, cloud cover, fog, rain, wind. The director keeps its own fixed exposure per phase.
- Open sea: real cumulus, blue sky, sunlit deck and sea. Atlantis: dark violet overcast with rain, the ruins in silhouette at several depths, lanterns warm on deck. Maelstrom: broken cloud with sun through it.
- Frame rate: 173 to 178 fps open sea, 113 to 123 in Atlantis, 130 to 138 in the maelstrom (720p, DLSS, editor build). Atlantis is about 20 fps faster than under my cloud layer.
- **Lost for now with the pack on:** the storm front wall and its mouth, the portal out of Atlantis, the cloud faces and the sea mist. They were drawn in my cloud layer, and the engine renders only one. `-DCNoUDS` brings the old sky and all of those back. They need rebuilding another way (the pack has a cloud painter and a radial storm actor to look at).
- Not yet used from the pack: its lightning (mine still fires), wet materials, rain splashes on the deck, screen droplets (switched on, not yet seen in a frame), its sounds.
- The opening is flat calm and plain now that the front is gone: nothing approaches. The Atlantis sky is an even overcast without structure.

The packaged build is still the 19:02 one.

## Thu 12 Nov 20:25 — Atlantis rebuilt from the ruins pack (set 21); Ultra Dynamic Sky wired in but not working yet

Owner's first playtest is in `PLAYTEST-1.md`. Fab packs in the project and ignored by git: AncientRuins, Kraken, UltraDynamicSky.

- **Atlantis is now built from Modular Ancient Ruins** (`DCRuins.cpp`): gateways across the course, the pack's four monumental arches, temples, colonnades, statues on plinths, broken columns and crags as lane obstacles. Review set `taskboard/review/21-atlantis-ruins-pack/`; best frames `a_bow_0100`, `a_bow_0125`. Falls back to the old placeholder shapes if the pack is missing.
- **Ultra Dynamic Sky:** the ride director can spawn the pack's sky and weather and drive sun position, cloud cover, fog, rain and wind from the ride phases. **It does not look right yet**: the sky comes up as a purple dusk with no clouds, and the sea glows bright cyan under the pack's auto exposure. So it is off by default; `-DCUDS` turns it on. With it on, the storm front, portal and cloud faces are not drawn (they live in the old cloud layer).
- Next on the sky: find why sun position and clouds do not apply (likely the sun/time settings need the pack's own update call), fix the sea's brightness under auto exposure, then decide how the front, portal and faces coexist with the pack's clouds.
- Not started from the playtest list: wheel camera, ship turning and momentum, cannon feedback, station markers, wetness.
- The packaged build is still the 19:02 one (no ruins).

## Thu 12 Nov 19:05 — packaged build current (includes faces and portal)

`E:\projects\directorscut\Builds\Windows\DirectorsCut.exe` (built 19:02) contains everything up to commit `f20e631`. Full unattended voyage in the packaged build at 5x with crew AI and stand-in captain: all phases in order, no errors, servers shut down on exit, 78% hull at the end, 132 to 168 fps at 720p with DLSS. Sound, microphone and hands-on play not verified.

## Thu 12 Nov 19:00 — Atlantis: faces and portal together, and seen from the helm (set 20)

Review set: `taskboard/review/20-atlantis-portal-and-faces/`. `a_bow_*` look ahead from the bow, clear of the rigging; `b_helm_*` are the helm at the same four moments. Best frames: `a_bow_0160`, `a_bow_0185`, `a_bow_0210`. No new inbox messages.

- **The faces now sit where the playable view sees them:** three heads 900 m to 1.3 km tall, low over the city on either side of the portal and one above it, staggered along the route so one is always coming into view. They have shadowed eye sockets, cheeks and mouth, and cold lit eyes.
- **From the bow this is the closest any frame has come to the fog-to-portal reference:** a luminous spiral tunnel among the ruins, sentinel statues in silhouette at several depths, pale faces looming out of the storm cloud on both sides, a ghost ship crossing.
- **From the helm** the faces show from about 3:00 onward to starboard of the mainmast (`b_helm_0185`, `0210`); before that the rigging and sails cover most of them. The helm view of this scene is much more cluttered than the bow view.

Still short:
- The faces read as skull-like masks; they are not the modelled, bearded god-faces of the reference.
- At 2:15 the scene is still flat pale haze with silhouettes (`a_bow_0135`); the layered cloud only shows once closer.
- Statues, ruins and temple are still simple placeholder shapes (owner's ruins pack pending).
- Grain in the cloud as before.

Suggestion for the owner, not acted on: the bow gives a far better view of the reveals than the wheel does. The player can already walk there. A prompt or a station at the bow ("lookout") might be worth it.

The packaged build (18:21) does not include the faces. Repackaging next.

## Thu 12 Nov 18:39 — cloud faces are volumetric now (set 19), but the helm does not see them yet

Review set: `taskboard/review/19-cloud-faces-volumetric/`. `a_bow_*` look up from just ahead of the bow; `b_helm_*` are the helm at the same stretch. Best frame: `a_bow_0150`. No new inbox messages.

- **The three faces over Atlantis are no longer built from spheres.** Each is a head about 500 to 700 m tall shaped out of the same volumetric cloud as the sky (skull, brow, nose, cheekbones, jaw, hollow eyes, parted mouth, hair and shoulders tearing away into cloud), looking down toward the ship, with cold burning eyes and a faint glow from inside that rises with the lightning.
- From the bow they read as pale spectral heads with lit eyes hanging in the overcast above the statues, with the portal's light below. That is close in spirit to the fog-city reference.
- **Problem: from the helm they are not in view.** In all four helm frames the faces are above the top of the frame or behind the sails. They need to sit lower and further off so the playable viewpoint sees them. Next job.
- The modelling is soft: the head reads as a smooth pale oval with eyes; nose and mouth do not show at this distance.
- The slide frame that came out electric blue in set 18 was a lightning flash; six consecutive frames of the slide are normal.
- Frame rate in Atlantis from the helm: 89 to 96 fps (720p, DLSS, editor build).

The packaged build (18:21) does not include the faces change.

## Thu 12 Nov 18:23 — packaged build is current again

`E:\projects\directorscut\Builds\Windows\DirectorsCut.exe` (5.3 GB) now contains everything up to commit `895ace0`: enlarged maelstrom with the descent, rebuilt leviathan head, water walls, volumetric storm front, sky, Atlantis mist and portal.

Verified by a full unattended voyage in the packaged build at 5x with the crew AI and stand-in captain: all phases in order, no errors, speech and language servers started and shut down on exit, 76% hull and 15 orders given at the end. The volumetric clouds render in the packaged build the same as in the editor build (the engine noise volume is cooked in). 132 to 175 fps at 720p with DLSS.

Not verified in the packaged build: sound (run with sound off), microphone, and hands-on play.

## Thu 12 Nov 18:19 — Atlantis portal is volumetric cloud now (set 18); full voyage passes; packaging

Review set: `taskboard/review/18-atlantis-portal-volumetric/` (eight helm frames from just inside the front to the slide). Best frame: `shot_0215` at full size. No new inbox messages.

- **The portal out of Atlantis is no longer rings of spheres.** It is a standing ring of storm cloud in the same volumetric layer as the front: a tunnel 300 m in radius with spiral arms winding into it and bright cloud closing its far end. From the helm it is a cold light behind the temple skyline from about two minutes out, with the statues in silhouette against the layered overcast.
- **The pale round blobs inside the front (set 17, `shot_0063`) are gone.** They were the old sphere portal seen through the cloud.
- **Full unattended voyage at 5x with crew AI and stand-in captain, on this code:** all phases in order, no errors, 73% hull at the end.
- Frame rate in Atlantis: 90 to 98 fps (720p, DLSS, editor build). This is the lowest point of the voyage.

Still short:
- In the last twenty seconds before the slide the portal is partly hidden by whatever statue or rock is in the lane (`shot_0236`, `0243`); the reference has it framed clear between statues.
- `shot_0248` (in the slide) came out bright electric blue with white flecks, unlike earlier captures of the same moment. Probably a lightning flash on the tunnel water; not yet confirmed.
- Cloud faces are still sphere-built (one is faintly visible top right in `shot_0215`).
- Grain as before.

Packaging a fresh build now; it will replace the 14:44 one. Result in the next entry.

## Thu 12 Nov 18:00 — clouds pass 4 (set 17): large storm structure found. This is the first set I would put next to the reference

Review set: `taskboard/review/17-storm-front-large-scale/` (thirteen helm frames through the voyage, normal speed, 720p). Best single frames for the storm front: `shot_0024` and `shot_0046`. No new inbox messages.

- **The cause of the small repeated lumps in sets 15 and 16:** one tile of the engine's noise volume holds about eight cells across, so every scale I had chosen was about six times too fine. Everything was small detail with no large form. All scales are now six times coarser: masses around 20 km, towers of 5 km, heads of a kilometre, wisps of a few hundred metres.
- **Result:** the front is now a range of large rolling towers with soft lit flanks and dark hollows, standing against a clear blue sky, with a top you can see. The mouth is a dark hole ringed by lit spiral arms at the foot of it (`shot_0046`).
- The sky above the front is nearly clear now, so the front stands as a wall instead of merging with a cloud deck, and the horizon haze is thinner. The face of the front stands nearly upright instead of leaning 1.5 km over the ship.
- The fog inside the front is a third as thick, so on entry the mouth's glow and the first Atlantis silhouettes show through (`shot_0053`).
- Atlantis overcast and mist banks picked up the same large structure (`shot_0110` to `0210`): heavier, layered sky over the statues.
- Frame rate: 165 to 172 fps open sea, 94 to 110 in the front and Atlantis, 134 to 151 maelstrom.

Still short, honestly:
- Fine grain on thin cloud, and faint diagonal streaks in the lit flanks at full size. Flicker over a sequence not yet checked.
- `shot_0063`, a few seconds inside the front, shows pale round blobs in blue murk. I have not identified them yet; possibly the sphere-built faces seen through the cloud. To fix next.
- No rain skirt or hard line where the front meets the sea.
- The mouth is cold-only; the reference has warm sun against cold lightning.
- Cloud faces and the Atlantis portal are still sphere-built.

The packaged build is still the 14:44 one.

## Thu 12 Nov 17:51 — clouds pass 3 (set 16): the front reads as a storm mass now; grain and scale still open

Review set: `taskboard/review/16-storm-clouds-pass-3/` (`a_sky_*` from above the masts, `b_helm_*` from the helm through the voyage; normal speed, 720p). No new inbox messages.

- **Why set 15's front looked like scattered cloud on empty sky:** I tinted the front red to see it. It was one continuous towering mass all along, but its shaded parts were exactly the colour of the sky behind, so only the sunlit parts showed. Storm cloud is now darker and bluer than the sky, and the open-sea sun is whiter, so the front is cream where lit and dark slate in its hollows.
- **Firmer body:** the face is denser with a sharper edge and less deep bays, and the finest scale only frays the edges.
- **Mouth:** half as large again (450 m radius), with spiral arms that show from the helm in the last 15 seconds before entry (`a_sky_0046`, `b_helm_0046`). Its glow and the sheet lightning are confined; the white column of set 15 is gone.
- **Grain:** tried the engine's full-resolution cloud mode: much worse, because it drops accumulation over frames. Kept the previous mode and raised samples per ray from 2.4 to 4 times default. Better, still visible on thin cloud in full-size frames.
- Frame rate: 158 to 181 fps open sea, 93 to 104 in the front and Atlantis, 134 to 151 maelstrom.

Still short of the storm-front reference, in my reading of the frames:
- From the helm the front fills the sky as a ceiling of cloud rather than standing as a wall with a visible top; the sense of height is weak until the last seconds.
- No clean line where the front meets the sea; the horizon haze hides its foot.
- The mouth is the right idea but small in frame until late, and cold-only: no warm light against it.
- Grain remains. Flicker over a frame sequence still not checked.
- The inside of the front (`b_helm_0058`) is plain grey.
- Atlantis: mist banks and overcast are in, but the layering is subtle; statues still fade into flat blue.

Next: height and sea contact of the front, inside of the front, then Atlantis layers, faces and portal.

## Thu 12 Nov 17:44 — 17:17 message read; clouds pass 2 (set 15). Better, not there yet

**Acknowledged: the 17:17 cloud quality message.** Agreed that set 14 was not at the level of the references. This entry is a second pass, and by my own reading of the frames it is still short of them. I am continuing on the clouds; not moving on.

Review set: `taskboard/review/15-storm-clouds-pass-2/`. `a_sky_*` look ahead from above the masts as the front closes, `b_helm_*` are the helm through the whole voyage. Normal game speed, 720p.

What changed, against the list of problems in the message:
- **Bubbly repeated shapes and weak large-to-small structure:** found the real cause of the earlier dead end. The engine's cloud noise volume is fine; its values sit in a narrow band (about 0.83 to 0.96), so thresholding it at 0.5 gave plain white. I measured its range in game and now read it at four scales (masses of 10 km and more, towers of 3 km, heads of 800 m, wisps of 200 m). The computed ball noise that made the bubbles is gone. Shapes are now irregular, soft-edged cumulus rather than rows of spheres.
- **Density and self-shadowing:** density now builds up over about half a kilometre in from the surface. With a hard edge the sun never reached inside and the front rendered as flat dark slate. The front has lit masses standing in front of a solid dark core, so the bays between towers are dark cloud.
- **Blown-out white flank:** that was sheet lightning inside the cloud adding up along the whole line of sight. It is five times weaker and fades at the edges. Sunlit cloud is also held to 60% of the engine default. One bright patch of lightning still shows in `a_sky_0024`.
- **Storm mouth:** the hole now has spiral arms of cloud winding into it and closes in the dark further in, instead of a bright hole. It reads as a small swirl at the foot of the front, visible in `a_sky_0044` and `b_helm_0044`.
- **Grain:** cloud sample count raised by half. Still visibly grainy in the full-size frames, especially on thin cloud. Not solved.
- **Atlantis:** banks of sea mist from the same volume (new `Mist` value per phase) and a textured overcast replace some of the flat haze. First step only.

Remaining gaps, as I see them in the frames:
- The front does not yet read as one towering wall with a clear top and a hard line where it meets the sea. It reads as a field of cumulus in front of a dark mass. Scale is weaker than in set 14 from the helm.
- Grain and softness: the clouds look low-resolution next to the ship. I have not yet checked a frame sequence for flicker.
- The mouth is too small and too pale to be the centrepiece the storm-front reference shows; no warm-against-cold light contrast yet.
- Lightning lights patches, but does not reveal structure inside the cloud.
- Open-sea clouds are all one warm tan; no cool shadow sides.
- Cloud faces and the portal out of Atlantis are still sphere-built.

Checks this tick:
- **Full unattended voyage at 5x with crew AI and stand-in captain (before the cloud changes, after the maelstrom enlargement):** all phases in order, no errors, hull 78% at the end. The language model handled orders and reports throughout.
- Frame rate now: 163 to 179 fps open sea, 95 to 107 in the front and Atlantis, 133 to 153 in the maelstrom (720p, DLSS, editor build). Atlantis is the tightest.

The packaged build is still the 14:44 one.

## Thu 12 Nov 17:02 — storm front is now a real volumetric cloud (set 14), first part of item 4

Review set: `taskboard/review/14-volumetric-storm-front/`. Row 1 looks ahead from above the masts as the front closes; rows 2 and 3 are the helm through the approach, the inside of the front, Atlantis, and two maelstrom frames. Normal game speed. No new inbox messages.

- **The storm front is no longer a wall of cloud spheres.** It is part of the engine's ray-marched volumetric cloud layer, with a new material (`M_StormCloud`, `Tools/make_stormcloud_material.py`): a band of cloud 2.5 km deep and 3 to 6 km tall, heaped into towers, eroded at the edges, lit by the sun from the side and shadowing itself. It starts 15 km off as a wall on the horizon and closes at about 250 m a second.
- **The portal is bored into its foot:** a tunnel mouth 300 m in radius with a torn, turning edge, glowing from inside and flaring with the lightning. It is dead ahead on the sea from the first seconds of the voyage.
- **The same volume is the sky everywhere else:** a broken deck over the open sea, low overcast inside the front and over Atlantis, towering cloud with the sun through it over the maelstrom. Cover and cloud base are set per phase.
- The open sea is now clear afternoon light instead of a hazy sunset, so the front can be seen from far off. The old sphere wall is switched off (`bBillows` on the fog front brings it back).
- Frame rate at 720p with DLSS on the editor build: 175 to 180 fps on the open sea, 101 to 113 in the fog and the first half of the maelstrom, about 150 after. Before this change the fog ran at about 150, so the clouds cost roughly a third there.
- Against the storm-front reference: a towering, billowing, self-shadowed front with a lit portal at its base and warm side light is there. Still short: no clean concentric spiral around the portal; the cloud is grainy close up; for a few seconds around 0:22 one flank of the front is blown out white; the lightning does not yet show inside the body of the cloud.
- **Not done in item 4:** the Atlantis fog is still flat haze, and the cloud faces and the portal out of Atlantis are still sphere-built. That is next.
- Dead end worth recording: the engine's cloud noise textures sampled as plain white from a generated material, so the noise is computed in the shader instead.

The packaged build is still the 14:44 one. A full unattended voyage has not been run since the maelstrom was enlarged.

## Thu 12 Nov 16:09 — leviathan head rebuilt; it now towers over the ship (set 13)

Review set: `taskboard/review/13-leviathan-head/` (six helm frames through the maelstrom, normal game speed). No new inbox messages.

- **The head is a modelled skull now, not a mask of simple shapes.** Skull, brow, long muzzle and lower jaw are lofted skins in the creature's own hide material, with a crest of flat spines, longer horns, fins behind the jaw, rows of teeth and a pair of fangs. Still generated in code.
- **It holds a three-quarter view to the ship** so the long jaw shows in profile, and throws its head back and opens wider for each roar.
- **Neck is longer** (about 920 m from the pit floor to the head), so the head stands well above the ship's horizon instead of level with it, and the centre of the maelstrom sits a little further to starboard, clear of the masts.
- **The weave pattern on distant water is gone.** Fine foam and ripple detail now fades out before it gets smaller than a pixel.
- Light shafts are a third weaker; they were washing out the creature.
- Against the reference: a colossal dragon-like head with open jaws over a vast bowl, rim and falls behind, sun through cloud. Still short: the body below the neck is hidden by the bow most of the time; the hide is one dark tone and needs lighter belly and wet highlights to read in haze; no water streaming off it; the arms round its base are still thin for the new scale.
- Correction: the times on my last three entries (16:05, 16:12, and "read 16:08" in the inbox) were a few minutes ahead of the clock. This one is from the clock.

Next: fog and clouds (item 4 of the 15:21 message), then a fresh packaged build.

## Thu 12 Nov 16:12 — 15:40 and 15:41 messages read: I cannot download the ruins pack from here

**Result: blocked, needs two minutes of the owner at the PC.** What I checked:

- The Epic Games Launcher is running and signed in on this PC, but its vault cache is empty: the pack has not been downloaded by anything yet.
- The engine's Fab plugin (UE 5.8) only starts a download from a click inside its own browser window after an interactive sign-in. It has no console command, commandlet or script call that fetches a library item. The Unreal MCP server drives the editor, so it has the same limit.
- The launcher has no command-line install for Fab library items either. I have no tool that can operate a window, and Codex reports the desktop is locked.
- The remaining way would be to take the launcher's saved sign-in token and call Epic's private download service directly. That is not a supported route and it means handling the owner's account credentials, so I have not done it and will not without the owner saying so in person.

**What the owner does when home:** Epic Games Launcher, Unreal Engine, Fab Library, find Modular Ancient Ruins, Add To Project, tick "Show all projects", choose DirectorsCut, and pick 5.4 as the asset version if it complains about 5.8. Then leave a line in the inbox. I will then check that it loads in 5.8, add its folder to `.gitignore` before anything else, and swap it in for the placeholder ruins, aqueducts and temple.

**Until then** nobody should run a blanket `git add Content`: the pack's folder is not ignored yet because its name is not known. I only ever stage `Content/DC`.

## Thu 12 Nov 16:05 — 15:21 corrections, first pass: tunnel sheet fixed, bowl and leviathan about three times larger, visible descent (set 12)

Review set: `taskboard/review/12-descent-and-scale/`. Row 1 is the tunnel and breach from the helm, rows 2 and 3 the maelstrom from the helm over its two and a half minutes, the last two frames are the same scene from 120 m above the ship to show the scale and the slope. All captured at normal game speed.

**1. The flat sheet on the left of the tunnel: found and fixed.** Codex's pointer was the right area. The sea mesh is a fan of triangles around the ship, so a few hundred metres ahead it has only one or two points across the width of the tunnel, and the rolled-up sea collapsed there into a flat wall standing on the ship's centre line. The helm is to starboard of that line, so the wall covered the port half of the view ahead. Now the tunnel opens out with distance before the mesh gets that coarse, the bowl no longer squeezes one side of it, and the tunnel is 60 m in radius instead of 32 m. The breach opens the two sides like a pair of doors (frames `a_helm_0274`, `0284`). Checked from the helm through the whole transition and from astern.
- Also found on the way: ghost ships and Atlantis scenery used to follow the ship into the tunnel. Ghost ships now founder when the slide begins and the city sinks behind the closing walls.
- Motion blur was smearing fast water near the ship into flat colour; it is now about a third of the engine default.
- My earlier screenshots of the tunnel were taken at six times game speed, which multiplied that blur. Sets from now on use normal speed for anything that moves fast.

**2. Leviathan.** The whole creature is 2.8 times larger: the head stands about 750 m above the floor of the pit and the skull is about 250 m long, five ship lengths. It rises out of the pit ahead of the bow with haze between it and the ship (`b_helm_0382`, `0400`, `0430`). Not done: the head is still built from simple shapes and reads as a mask; silhouette, jaw and body are the next job. The arms round its base did not scale well and are hard to pick out.

**3. Maelstrom.** The crater is 6.6 km across (was 2.2), the wall of falling water is 300 m high, and the centre is a funnel about 1 km deep. The ship now starts 1.2 km from the centre on a 15 degree slope and is drawn in to under 1 km on a 23 degree slope, so through the phase the far wall and the rim climb up the view and the creature comes up from below the bow. A mountainous swell lifts the ship about 100 m every half minute, which is when the deck sees furthest down the funnel. The frames from above (`c_above_0382`, `0430`) show what is there: spiral arms of foam winding into the pit and the creature standing in it.
- Against the reference: immense bowl, towering far rim, creature dominating the centre, sun and rays, water walls beside the ship are all there. Still short: from the helm the bow hides the steepest part of the view down, so the depth is clearer from the main deck rail than from the wheel; the water close to the ship is pale and flat; a faint weave pattern shows on distant water in the sun; the wave walls beside the ship still look a little like ice.

**4. Fog and clouds** not started; next after one more pass on the creature.

Not yet repackaged; the packaged build is still the 14:44 one.

## Thu 12 Nov 15:25 — 15:21 message read; water walls beside the ship (set 11)

**Acknowledged: the 15:21 corrections** (tunnel's flat left sheet, much larger leviathan, much larger maelstrom with a visible descent, cinematic fog and clouds). Working on them now in the owner's order: descent, bowl and leviathan scale first, with the tunnel sheet; fog and clouds after. Thanks to Codex for the seam pointer in `make_sea_material.py`; I will check it at the helm camera.

Finished just before the message arrived, review set `taskboard/review/11-water-walls/`:

- **Breaking waves now stand on both beams in the maelstrom** (`DCWaveCurl.cpp`, material `M_Wave`). They are their own geometry about 95 m out, 110 m tall to starboard and 75 m to port, hollow toward the ship with the lip curling over, torn ragged at the edge, with spindrift along the crest and sunlight through the thin water. They rise as the tunnel lets go and taper to nothing ahead, so the crater and the creature stay clear between them.
- Against the reference: the framing by water on both sides is now there at all times, without hiding the scene. Still short: they read a little like ice where the lit foam is broad; no spray reaches the deck from them; the first seconds (frame 298) still show the old tunnel wall as a flat dark sheet on the left, which is item 1 of the new message.
- These walls will be re-fitted once the bowl is enlarged and sloped (items 2 and 3).

## Thu 12 Nov 15:05 — water walls beside the ship: tried again, reverted; docs brought up to date

- **Tried to keep the curling water walls up through the maelstrom** (the reference has them framing the scene). Two strengths tested from the helm, six frames each. Both failed the same way as the first attempt: the wall is so close to the ship that it fills the left half of the view as flat dark teal, and at some moments the whole frame. The creature framed in the gap looks good for a few seconds, but most of the time the wall hides the crater, which is the thing the owner asked to see. Reverted; the build is unchanged from the last review set (10).
- What would make it work: walls that stand much further out (hundreds of metres, not tens) and curl over at the top, as separate geometry instead of the sea tube. Not done yet.
- `PLAN.md` now describes the actual route (no hell by default, Atlantis, portal, crater) and lists the newer source files. `ITCH_PAGE.md` no longer promises a channel of fire. `SUBMISSION.md` no longer mentions lava.
- **Hell routing, as asked:** the hell channel is out of the default route and nothing in the default voyage refers to it. The code, its hazards and its sound remain behind `-DCHell`. Owner to confirm it stays out.
- No new inbox messages.

## Thu 12 Nov 14:44 — packaged build is current

`E:\projects\directorscut\Builds\Windows\DirectorsCut.exe` (5.3 GB) now contains everything up to commit `e5b04ed`: the new route, Atlantis, cloud faces (repaired), portal, crater maelstrom with leviathan and mist, storm bell, recorded sound. Verified by a full unattended voyage at 5x with the crew AI running: all phases in order, a loosely worded order understood by the language model, no errors, servers shut down on exit, 148 to 177 fps at 720p, stand-in captain finished with 73% hull.

## Thu 12 Nov 14:40 — maelstrom pass 4: mist

Review set: `taskboard/review/10-maelstrom-mist/` (five helm frames from bursting out of the tunnel to the creature fully risen).

- Mist now hangs where the falls land all round the foot of the crater wall, and boils around the creature's base. It softens the hard line between wall and water and gives the far side depth.
- Light shafts from the sun now show across the deck in these frames.
- State of the maelstrom against its reference: position, crater, waterfalls, mist, teal foaming water, spray, sun and rays, the creature rising with arms around it are all in. Still short: the creature's head is built from simple shapes; the curling water walls beside the ship are only there for the first seconds; the deck is not wet; crew are mannequins.
- Packaging a fresh build now (Atlantis, faces, portal, mist).

## Thu 12 Nov 14:38 — the portal out of Atlantis

Review set: `taskboard/review/09-portal/` (six helm frames over the last minute of the fog, ending inside the tunnel).

- **There is now a portal ahead before the sea takes the ship** (`DCPortal.cpp`): eight rings of cloud billows with trailing arms, turning about the tunnel's axis at different rates, around a bright blue-white core that flares with the lightning. It stands on the ship's course at the distance covered by the time the slide begins, and it stays dead ahead whatever the helm does.
- From the helm it first shows as a glow behind the ruins about a minute out, then as a lit, churning opening among the temple buildings; the sea rolls up into the tunnel as the ship reaches it.
- Against the portal reference: light-at-the-end among ruins is there. The reference's clean concentric tunnel and the statues framing it left and right are not; the view is cluttered by whatever random rock or ruin is in the lane at that moment.
- Also: lightning no longer turns the Atlantis sea bright blue (dimmer flash, greyer water).

All four of the 14:03 references now have a first pass in the build: storm front (set 07), fog city and Atlantis (06), faces (08), portal (09). The maelstrom reference has had three (01 to 03).

## Thu 12 Nov 14:26 — packaged build runs the new route; cloud faces repaired

- **Packaged build (5.3 GB) ran the whole new route** at 5x with the stand-in captain: sea, fog at 0:51, portal at 4:00, maelstrom at 4:50, ending at 7:30, no errors, 156 to 188 fps at 720p. The stand-in captain finished with **77% hull** ("She came through"); the route is shorter and the maelstrom has four near arms now, so the ride is gentler than before.
- **Fixed a regression I caused:** roughening the cloud billows for the storm front also dissolved the faces in the clouds into speckle. The roughness is now a setting; the faces keep their modelling. Checked from the deck with lightning held on: a pale face with eye sockets and nose looks down from the upper right. Frames in `taskboard/review/08-cloud-faces/`.
- That packaged build was made before this fix, so its faces are speckled. Next package picks it up.

## Thu 12 Nov 14:20 — faces in the clouds; the storm front has a mouth and real cloud texture

Review set: `taskboard/review/07-storm-front/` (two exterior views like the storm-front reference, then helm views of the approach). Open `ext_shot_0046.jpg` first.

- **Faces in the clouds (`DCCloudFace.cpp`).** Three heads 400 to 600 m tall hang above the city, modelled from soft billows: brow, nose, cheekbones, hollow eye sockets, parted mouth, chin, streaming hair, and a beard on two of them. They are faint until lightning lights them from inside. In a test frame from the deck one reads clearly as a face looking down. They are stylised and mask-like, not the classical portraits in the references.
- **Storm front.** The wall now has a mouth where the ship goes in: six rings of billows spiralling back into the cloud around a cold glow that flares with the lightning. The billows got noise-eroded edges and mottling, which was the bigger gain: they no longer read as spheres. From outside the front is now a ragged, towering mass with light breaking through it and blue light low down in the mouth.
- Against the storm-front reference: scale, darkness over a sunlit sea, and the cold light inside are there. The clean spiral "eye" of the reference is not; the mouth reads as a lit hollow rather than a curling tube.

## Thu 12 Nov 14:13 — Atlantis in the fog; new route without the hell channel

Review set: `taskboard/review/06-atlantis-route/` (twelve helm frames from entering the cloud front to the maelstrom, plus `_contact.jpg`). `shot_0115.jpg` and `shot_0190.jpg` are the ones to open first.

- **The route now follows the owner's progression:** calm sea -> cloud front (closes over the bow at 1:00) -> Atlantis in the fog (to 4:00) -> portal tunnel (4:00 to 4:50) -> maelstrom (4:50 to 7:30) -> clear water. The hell channel is out of the default ride; `-DCHell` restores it. Verified by a full run-through.
- **The fog is staged:** thick and blind for the first half minute inside the front, then it thins to layered blue-grey haze you can see a kilometre or two into.
- **The drowned city is laid out along the course** (`ADCTreadmill::SpawnAtlantis`): six guardian colossi 110 to 160 m tall with spear and shield or trident and crown, flanking the course in pairs and turned toward the ship; three broken aqueducts crossing behind them; a terraced temple 700 m wide with colonnades, waterfalls, corner towers and a domed hall, far ahead to starboard and about 450 m off by the time the portal opens; three figures a kilometre tall standing in the cloud beyond.
- Lightning is more frequent here and lands bolts among the ruins.
- Against the references: the layering, scale, cold haze against warm lanterns, sentinels and temple are there. **Not there:** actual faces in the clouds (the cloud figures are faceless giants); the statues and temple are built from simple shapes and look like chess pieces close up; the temple's waterfalls are plain strips; flashes turn the sea too blue.

## Thu 12 Nov 14:07 — ACKNOWLEDGED: four more references (storm front, fog city, Atlantis, portal)

Read the 14:03 message and looked at all four images. The route the owner describes is: calm sea -> immense storm front -> fog-shrouded ruined Atlantis with faces and ghostly forms -> portal -> maelstrom.

**Hell routing, as asked:** today the ride goes fog -> slide -> **hell channel (2.5 minutes)** -> breach -> maelstrom. The hell channel is not on the owner's route. I am taking it out of the default ride and keeping the code: the slide and breach become the portal, leading straight from Atlantis into the maelstrom. `-DCHell` on the command line puts it back. The ride becomes about 7.5 minutes instead of 10. **Owner: say if you want hell kept in the default route.**

What the four images ask for that the build lacks, in the order I will do it:

1. **Atlantis in the fog:** sentinel statues with spear or trident, broken aqueducts, a terraced temple with waterfalls, at staged depths. The fog itself must thin out: the references show a kilometre or two of layered haze, and the build's fog hides everything past 150 m.
2. **Faces in the clouds.** I have no face art; first attempt will be colossal spectral heads and shoulders standing in the cloud, lit by lightning.
3. **Storm front as a curling tunnel** with lightning inside, not a flat wall.
4. **The portal:** a luminous tunnel opening straight ahead among the ruins.

## Thu 12 Nov 14:06 — DLSS evidence from the packaged build

Asked for in the inbox: checked the actual packaged build (repackaged with the new maelstrom and fog front, 5.3 GB) and captured comparable performance and image evidence. It is all in `taskboard/review/05-dlss-packaged/` with a README.

At 3840x2160 on this PC: DLAA (DLSS at native) 85 to 95 fps; **DLSS Quality, the game's default, 114 to 135 fps (about 40% faster)**; Performance 129 to 157; Ultra Performance 153 to 179. With DLSS off the engine's own upscaler runs at about the speed of DLSS Performance, so that row is not a native baseline. One run per mode, on a very fast GPU that is partly CPU-limited here, so read the gaps as indicative.

The folder has 1:1 crops of the same region of the frame in every mode, and whole frames scaled to 1080p.

Useful trick found on the way: this remote session's desktop is only 1600x900, but `-RenderOffscreen -ForceRes -ResX=3840 -ResY=2160` renders and screenshots true 4K.

## Thu 12 Nov 14:00 — maelstrom light and spray; the cloud front; NvRTX assessment

New review sets in the repo: `taskboard/review/03-maelstrom-light-spray/` and `taskboard/review/04-fog-front/` (full-size frames in ride order plus `_contact.jpg`).

**Maelstrom (priority 1), pass 3.** Added light shafts from the sun, cloud shadows moving over the water, and wind-blown spray across the deck. With these the helm view now has: teal water torn into white foam spirals, the waterfall rim across the horizon, the horned head with lit eyes framed by arching arms, spray in the air. Against the reference, what is still short: the sun and its rays are above the frame from the helm (hidden by the sail), so god rays do not read; the creature is blocky; no mist at the foot of the falls; deck is not wet.

**Fog front (priority 2), pass 1.** There is now a cloud front (`DCFogFront.cpp`): about 150 billows forming a wall 9 km wide and 1.2 km tall, dark at the foot, sunlit on top, in three shades so it has modelling, flickering from inside with lightning. It stands 4 km off at the start and closes over the bow at 60 seconds. The fog darkens just before it arrives so there is no bright gap as the ship goes in. Inside, the first things out of the murk are ruins, a statue, a ghost ship, and a pale giant walking on the water. What is weak: from a distance the front reads as heavy cloud more than a wall on the sea; the billows are plain spheres.

**NvRTX assessment (asked for in the inbox).**

- What is active now: **DLSS 4.5 Super Resolution** (on, Quality), **DLSS Frame Generation** (integrated, off by default, `F6`). Lighting is stock Unreal: Lumen with hardware ray tracing, volumetric fog and clouds, light shafts. None of RTXDI, ReSTIR GI, ReSTIR PT, Mega Geometry, LSS Hair or ACE is in use.
- NvRTX features need NVIDIA's fork of the engine. I could find a 5.7 branch mentioned publicly and nothing about 5.8, and NVIDIA's own material describes compiling it, not downloading a binary. The project is on stock 5.8.4.
- Cost of switching: download and compile an engine from source (hours on this PC), possibly move the project down to 5.7, rebuild the DLSS plugins against it, re-verify packaging and the Unreal MCP tooling, and re-tune the lighting, which would look different under RTXDI. Access needs a GitHub account linked to Epic, which I do not have, so I cannot start it unattended.
- What it would buy: RTXDI is a real fit for this game (many moving lights: lanterns, deck fires, lightning, ghost ships). ReSTIR PT would be for a path-traced beauty mode, not for play.
- **My recommendation: do not migrate.** DLSS 4.5 alone meets the listed requirement, three days remain, and the visual gaps the owner listed (composition, scale, atmosphere) are art-direction problems that the stock renderer can solve. If the owner wants it anyway, the low-risk way is a second copy of the project on the NvRTX branch purely to try RTXDI, keeping this one as the entry; the owner would need to do the GitHub download.

## Thu 12 Nov 13:48 — maelstrom reveal, pass 2: the crater and the leviathan

Screenshots for remote review are now in the repo: `taskboard/review/02-maelstrom-leviathan/` (six full-size frames from the helm, in ride order, plus `_contact.jpg`). `01-maelstrom-reveal/` is the earlier pass for comparison. Reference: `References/maelstrom-direction.jpg`.

Compared with the reference, from the same helm view:

| | Reference | Now | Still missing |
|---|---|---|---|
| Where the creature is | Ahead, slightly to starboard | Ahead, slightly to starboard | — |
| The space | Crater ringed by waterfalls, far rim about a kilometre off | Crater ringed by a 260 m wall of falling water, far rim about 1.6 km ahead | Mist at the foot of the falls; the wall is evenly streaked rather than broken into cascades |
| The entrance | Ship bursts out between curling walls of water | The tunnel opens ahead of the bow over about 15 seconds and the crater appears through the mouth | The walls are flat teal and do not stay in frame once out; I tried keeping them and they filled half the view, so they go |
| The creature | Sea dragon, jaws open, spines, tentacles around it | A leviathan on a thick neck with a horned head, lit eyes, a jaw that drops open on each roar, a ridge of spines, seven arms around its base | It is built from simple shapes and reads as blocky up close; smaller in frame than the reference |
| Water | Teal, white foam everywhere, spray | Teal, foam arms spiralling into the pit, churn at the wall | Spray and white water around the ship itself |
| Light | Sun through towering cloud, god rays | Daylight with the sun ahead; the ship is rim-lit | No visible god rays or dramatic cloud yet; sky is flat |
| Foreground | Wet deck, crew at work | Deck, lanterns, mannequin crew | Wet sheen; real crew models are the owner's to supply |

Changes that made this: the pit moved from the port beam to ahead of the bow; a crater rim was added to the sea material; the kraken's dome was replaced by a neck and head (`DCKrakenHead.cpp`); the maelstrom was relit from grey storm to daylight; the near arms were cut from six to four and thinned so they frame rather than block.

Next: sky, god rays and cloud for the maelstrom; spray near the ship; then the fog-front entrance.

## Thu 12 Nov 13:26 — ACKNOWLEDGED: new visual direction from the owner

Read both 13:23 inbox messages and the reference image (`References/maelstrom-direction.jpg`). Working order from now:

1. **Maelstrom reveal, composition and scale to match the reference.**
2. **Fog phase and cloud-front entrance:** light, statues, ruins, spectral forms.
3. **Hell phase is on hold.** (My last change before reading the message thinned the lava cracks; I am leaving hell there.)

What the reference shows that the build does not yet have, and what I will change:

| Reference | Build now | Plan |
|---|---|---|
| Creature dead ahead, slightly to starboard, seen from the helm | Body 620 m to port; you must turn to see it | Move the pit ahead of the bow |
| A crater: a ring of waterfalls pouring down sheer walls, far rim a kilometre off | Smooth funnel, no rim | Add a sheer drop at the rim, a waterfall wall mesh, white water where it falls |
| Curling walls of water close on both sides, framing the view like a tunnel mouth | Tunnel closes and opens everywhere at once | Make the tunnel open ahead of the ship while still wrapping it, so the ship bursts out of it |
| Bright: sun breaking through towering clouds, god rays, teal water, white spray | Dark grey storm | Relight the maelstrom; put the sun ahead; storm stays for the slide and breach |
| A sea dragon, jaws open, with spines and tentacles | A dome with two eyes and arms | Build a neck and head for it |

**NvRTX:** not attempted yet; assessment will follow in this log. Short version of the constraint: it needs NVIDIA's engine fork built from source, which needs a GitHub account linked to Epic to download, so I cannot fetch it unattended.

Before the message arrived this tick: specters (pale giants walking on the water in the fog) and demons wading in the hell channel were added, and the packaged build was re-verified with the bell and all ten recordings loading.

## Thu 12 Nov 13:14 — storm bell and recorded sound (from Codex's shortlist)

Picked up `AUDIO-SHORTLIST.md` from Codex, including the owner's request for a bell that rings by itself. (I committed that file and Codex's README row along with my own commit by accident; nothing private was in them.)

- **Storm bell is in.** The ship's own bell (it has a mesh, a strike sound and a swing animation in the vessel asset) tolls by itself from the moment the fog closes in: one quiet strike, 15 to 25 seconds of silence, then strikes 8 to 14 seconds apart, quickening to 4 to 8 seconds in the slide, breach and maelstrom, and stopping in clear water. Each strike is a separate event, pitched down a little, varied in strength, and the wind dips for a moment so it reads. After the second strike a crew member says "The bell, Captain. Nobody is ringing it." I used the ship's bell sound, not the church bells: it needed no audition to be the right kind of bell. Swapping in a church-bell strike is a one-line change once someone has picked a clean one by ear.
- **Ten recordings from the shortlist are in the game** (`Content/Sonniss`, git-ignored): calm and rough sea beds that crossfade with the weather, storm wind, rain, fire crackle, a long thunder roll mixed in with the synthesised cracks, a horn swell at the start of the slide, hell channel and maelstrom, a creature gurgle with each kraken roar, a pain yell when an arm is shot away, and a water impact under each hull strike. The synthesised versions remain as the fallback when the recordings are absent.
- `Tools/wavprep.ps1` converts a recording to 48 kHz 16-bit, trims it and makes it loop; `Tools/import_audio.py` imports the results.
- **None of this has been heard by anyone.** Levels are guesses. This is now the most useful thing for the owner to check.
- Not used from the shortlist: the P2 and P3 items (rigging foley, ruin sounds, death whistle, extinguish). Say if any of them matter.

## Thu 12 Nov 13:06 — fixed: Odessa had no voice; packaged build verified

- **Bug found and fixed: Odessa has never spoken until now.** Her voice file (the current revision of `en_GB-alba-medium`) does not load in the bundled Piper build, so her lines only ever appeared as subtitles. I missed the warning earlier because my log checks filtered on Finch. The original `v1.0.0` release of the same voice works; it is now in `Sidecars/` and in the packaged build, and she speaks in a test run. `SETUP.md` notes the version.
- **Packaged build (5.3 GB) ran a whole voyage** with the stand-in captain at 5x speed: every phase, loose-order reading, DLSS, sound start-up, clean shutdown of the AI servers. It finished at 27% hull (the editor run finished at 45%, so expect a spread between runs).

## Thu 12 Nov 13:01 — balance measured with a stand-in captain; restart keeps the AI loaded

- **Balance now has numbers.** `-DCBot` is a stand-in captain: it mans both batteries, pulls one gunner off for each fire or torn sail, and sends them back afterwards. On a full run it left the fog at 94% hull, the hell channel at 76% and finished at 45% ("Battered, but afloat"). An idle captain finishes at 0%. A player who also calls helm orders should beat the bot.
- **`R` to sail again is fast and safe:** the language and speech servers stay loaded across the restart (tested: second voyage had the crew AI ready within a second) and are shut down when the game quits.
- **Sunk ghost ships go down** by the stern instead of vanishing, and the crew cheers.
- The near-black fog frame from the last run is gone in this one.
- Packaging a fresh build now.

## Thu 12 Nov 12:52 — hostiles shoot back, more sound, fresh packaged build

- **Ghost ships now fire on us** if left alone: a glowing shot arcs onto the deck every 10 to 14 seconds per ship (hull damage; in the hell channel it can start a fire). At most two are alongside at once. Manned batteries sink them before they get a shot off, which is the reason to man the guns.
- **More synthesised sound:** a quiet low drone for mood, fire crackle while the deck burns, a thud and splinter for every blow to the hull, and a kraken roar as it rises, when it is up, and near the end. Rumble and thunder lost their inaudible sub-bass.
- **To audition the sounds without playing:** run with `-DCDumpAudio`; every sound is written to `Saved/AudioDump/*.wav` (already there now). I checked them numerically (none silent, none clipping) but still cannot hear them.
- **Packaged build refreshed and verified** before the two items above (5.2 GB, voice loop and DLSS working, about 4 GB of video memory at 1080p including the language model). It includes lanterns, lightning, loose-order reading and the new maelstrom.
- `taskboard/ITCH_PAGE.md` has draft page text and a controls table for the owner to edit.

## Thu 12 Nov 12:25 — the maelstrom is a pit with the kraken in it

- **Reshaped the maelstrom** into a narrower, steeper pit (600 m deep, 350 m radius, centred 620 m to port) and thinned the storm fog. From the deck the sea now falls away into the pit and the far wall climbs to a quarter of the way up the sky.
- **The kraken's body is now the centre of the view to port:** larger, standing higher out of the pit, eye lit, ringed by its tall arms. Before this it was there but hidden by fog and the lip of the funnel.
- Note for anyone testing: with `-DCTimeScale` the screenshot waits are also sped up, so use `-DCRunThrough` when something needs real ride time to develop (I lost a few runs to this).

## Thu 12 Nov 12:18 — finale beat, opening hints, DLSS keys, plan refreshed

- **Finale:** the last ten seconds of the maelstrom shake harder, then the screen whites out as the ship is flung clear and the calm sea fades in. Fires and loose rigging clear at the ending.
- **Opening:** if the captain has given no orders after 17 seconds the crew asks for them; at 42 seconds someone calls the fog.
- **`F5`** cycles the DLSS mode (Quality, Balanced, Performance, DLAA) and **`F6`** toggles DLSS Frame Generation, each confirmed in the subtitle log. Frame generation initialises without errors but I cannot judge how it looks; it stays off by default.
- **Frame rate on this PC:** 90 to 165 fps through the ride at 720p and about the same at 1440p, so it is limited by the CPU here, not the GPU. I have no slower machine to test on.
- **`PLAN.md` now matches reality:** milestone states, the real deadline, and a map of which source file owns what.

## Thu 12 Nov 12:10 — the crew understands loose orders

- **The language model now reads the captain's orders**, not just the crew's replies. Tested in game: "somebody get us away from those rocks" -> helm turns; "blow that spectre on our right out of the water" -> starboard battery fires; "stop shooting you fools" -> hold fire. ("Marlow the mast is coming apart" -> rigging repair was checked against the model directly, not in game.) Small talk gets an in-character answer instead of an action. About half a second per order.
- Short clear orders ("hard to port") still go through the instant keyword grammar, which is also the fallback when the model is not running.
- Crew warnings no longer invent directions (lower temperature, stricter prompt).

## Thu 12 Nov 12:03 — lanterns, lightning bolts, shake; lighter build verified

- **Packaged build re-verified at 5.2 GB** with the lighter AI servers: it started them itself, heard the recorded order in 420 ms, the crew answered and spoke, DLSS active.
- **Ship lanterns** light up as daylight goes (stern, mainmast pair, foremast), flickering. In the fog the deck is now lit warm against the blue murk.
- **Lightning has bolts:** a forked ribbon a few kilometres off for the length of each flash, then thunder. The flash itself is gentler so the sea does not turn bright blue.
- **Camera feel:** shake on hull strikes and thunder, a rumble and wider lens while the sea drops away in the slide.
- Test flag `-DCLightningTest` holds each flash for 4 seconds so bolts can be photographed.

## Thu 12 Nov 11:42 — readable HUD, lighter build, submission notes

- **HUD redone:** hull bar, phase name, score, fire and rigging alerts, subtitles on dark plates, a one-line key reminder, and title and score cards that scale with resolution. Debug numbers only show with `-DCDebug`.
- **Build is about 2.7 GB lighter and no longer needs an NVIDIA card for the crew AI.** The language model server now uses Vulkan and speech-to-text runs on the CPU with a smaller model. Measured here: same reply speed, an order registers in about half a second.
- **Submission notes are in `taskboard/SUBMISSION.md`:** deadline (Mon 16 Nov 01:59 GMT, from secondary sources), what to upload, the list of sourced assets, draft NVIDIA answers, and the build-size question. **Owner: itch.io normally limits files to about 1 GB; our build is about 5 GB.** Please check the rules or tell me which cut to make.
- **Gun station** checked with a scripted player: the camera now sits outboard of the shrouds with a clear field of fire. Ghost ships hold station on the ship instead of drifting into it.
- Fires now burn out on their own after 40 seconds; arms take more shots; the lookout only calls rocks that actually threaten the ship.
- `Tools/contact.ps1` tiles a run's screenshots into one sheet (`Saved/Screenshots/contact.png`).

## Thu 12 Nov 11:26 — whole ride plays start to finish; kraken head; balance

- **The ride now plays from title card to score card.** A full unattended run-through (4x speed, `-DCRunThrough`) passes through all seven phases and ends on the score card. `R` sails again, `Esc` quits.
- **Kraken body:** a mantle with two lit eyes rises out of the centre of the maelstrom, ringed by arms hundreds of metres tall; the eyes blaze in each lightning flash. Seen from the deck through the rain it is the finale image.
- **First balance pass, measured by run-throughs.** With the crew left idle the ship ends at 0% hull ("barely afloat"); with both batteries manned the kraken lands about half as many blows (30 instead of 67) and 33 arms are shot away. Fires left burning are the biggest single cost, so putting them out matters.
- **Helmsman is smarter:** he picks the clearest water he can see, and sees less far in thick fog. Rock strikes on autopilot dropped from 16 to 8 per ride.
- Earlier entries in this log carry times I guessed; they run up to 45 minutes fast. This one uses the clock.
- Next: look and readability pass (cards, HUD, foam, hell brightness), then real player-input testing needs the owner.

## Thu 12 Nov 11:45 — the crew talks back; fires and rigging

- **The crew now speaks up unprompted**, in their own words through the local LLM and voices: the lookout calls rocks with their bearing, ghost ships with their side, hull strikes, and each new phase. Example from a test run: "Captain, look port! Ghost ship in the fog!"
- **Typed orders:** press `Enter`, type, press `Enter`. Same path as voice.
- **Deck fires** start in the hell channel (flames, flickering light, hull damage while they burn). Order someone to "put out the fire" and they run fire watch; or stand at a fire and hold `E`.
- **Rigging tears loose** in the slide and the storm: the helm goes sluggish until someone is sent to "repair the rigging", or the captain holds `E` at the mainmast.
- With three crew and four jobs (helm, two batteries, fire or rigging) the captain has to keep re-assigning people, which is the game.
- Next: kraken head and an ending, an intro card, then a balance and look pass over the whole ride.

## Thu 12 Nov 11:20 — full packaged build verified; rain and embers

- **Packaged build with everything in it works.** `E:\projects\directorscut\Builds\Windows\DirectorsCut.exe` (7.8 GB including the local AI) was run unattended: it started its own LLM and speech servers, heard a recorded order, the crew acted and spoke, DLSS was active, and the servers shut down on exit. `Tools/package.sh` now copies `Sidecars/` into the build.
- **Rain and embers** without a particle system: thousands of camera-wrapped streaks from one mesh and material (`DCWeatherFX.cpp`, `Tools/make_rain_material.py`). Rain brightens in lightning; embers rise in the hell channel.
- The build is large mainly because of CUDA runtime libraries and the 3 GB language model. If the jam has a size limit, tell me and I will slim it (smaller model, CPU speech-to-text).
- Next: deck fires and rigging repair, crew warnings, typed orders, kraken head, intro and ending.

## Thu 12 Nov 11:00 — hazards have shapes; hell is lava

- **Fog phase now has what the pitch asked for:** noise-built sea stacks and drowned ruins to steer around, colossal robed statues looming off to the sides, and glowing spectral copies of our own ship coming alongside as targets. All built in code (`DCHazards.cpp`) plus two generated materials (`Tools/make_misc_materials.py`).
- **Hell channel:** the sea is dark crust with glowing cracks, walls curve up on both sides, jagged spires with ember-lit cracks stand in the lane, and the ghost ships burn orange.
- New test flag `-DCTimeScale=5` runs the ride fast so hazards arrive during a screenshot run.
- Next: rain and spray, deck fires and rigging repair, typed orders, kraken head, intro and ending, then a packaged build with everything in it.

## Thu 12 Nov 10:50 — storm look and sound

- **Maelstrom now reads as a storm at sea** from the deck: dark towering swell, cloud layer, a kraken arm sweeping in. The sea no longer washes through the hull or over the deck.
- **Open sea** has a sunset cloud layer.
- **Weather sound is in, synthesised in code** (no audio files): wind, sea wash, rain, a low roar for the hell channel, and thunder that follows each lightning flash after a delay. Cannon and impact sounds come from the vessel asset's own effects. **I cannot hear the result**, so the mix is untested; please listen when you are back and tell me what is too loud or wrong.
- Next: hazard shapes (rocks, ruins, ghost ships), rain and spray visuals, deck fires and rigging repair, typed orders, kraken head, intro and ending.

## Thu 12 Nov 10:45 — kraken and crew bodies

- **The kraken is in.** Six procedural arms rise around the ship in the maelstrom, sway, wind up and slam the deck (hull damage). No authored animation: each arm is a spine curve with a spring-driven tip, skinned in code, with a suckered underside from `M_Kraken` (`Tools/make_kraken_material.py`). Guns can target and kill arms; a killed arm thrashes and sinks, then another rises.
- **Crew have bodies.** They use the mannequins that ship with the vessel asset, with idle and walk animation, and face their work at a station. Still placeholders for real pirate characters.
- **Known look problems, to fix in the polish pass:** the maelstrom is too bright and its foam is blobby; the hell sea is a flat orange glow; open sea is dull.
- **Next up, in order:** storm look for the maelstrom, hazard shapes (rocks, ruins, ghost ships), rain/spray/fire, sound, deck fires and rigging repair, typed orders, kraken head, intro and ending.

## Thu 12 Nov 10:35 — sea, tunnel, whirlpool, playable stations

- **The sea is real now.** A polar grid mesh around the ship with a wave material (`Content/DC/Materials/M_Sea`, generated by `Tools/make_sea_material.py`). One material does every phase: calm water, dark fog water, glowing hell sea.
- **The slide works.** The same material rolls the sea up into a tunnel that closes over the masts and drops away ahead (`Tube` and `Drop` values per phase). First-person view inside it already looks like the pitch.
- **The maelstrom works as a shape.** The sea becomes a funnel 500 m deep centred 800 m to port; the whole sea is tilted so the water stays level under the ship and the horizon leans.
- **Player stations.** Walk to the wheel or a gun battery and press `E`. Helm: `A`/`D`. Guns: fixed side camera, mouse aims a marker on the water, click fires a rippled broadside with ballistic arcs. Crew gunners use the same guns.
- **Fog phase look** is dark and misty with hazards looming out of it; exposure is now fixed per phase.
- Every screenshot I take is in `Saved/Screenshots/Auto` (overwritten each run).
- Next: procedural kraken tentacles, then real hazard shapes (rocks, ruins, ghost ships), rain/spray/fire effects, and sound.

## Thu 12 Nov 10:20 — M1 done; voice loop and DLSS working

- **M1 is complete.** First packaged build (`E:\projects\directorscut\Builds\Windows\DirectorsCut.exe`, 1.5 GB) runs and plays the ride.
- **Voice loop works end to end** in an unattended test: a recorded "Finch, hard to starboard" wav -> whisper (about 320 ms) -> order parsed -> Finch takes the helm order -> local LLM writes his reply (about 100 ms) -> Piper speaks it from his position on deck. Example reply: "Hauling hard, aye!"
- **DLSS 4.5 plugin is enabled** (Super Resolution at Quality, frame generation supported but off by default). Log confirms `DLSS-SR=1 DLSS-RR=1` and the feature is created at runtime.
- Local AI lives in `Sidecars/` (llama.cpp b12317 + Qwen3.5-4B, whisper.cpp + small.en, Piper + three British voices). Details and re-download list in `SETUP.md`.
- **Needs the owner:** no microphone was detected on this PC (probably because the session is remote), so live push-to-talk is untested. Please try holding `V` and speaking when you are back. Keys `1`-`6` send quick orders without a mic.
- **Not done yet from M2:** the player cannot operate the cannons or helm personally yet; that is next, followed by the fog look and real hazards.

## Thu 12 Nov 10:10 — greybox runs

- **C++ module `DirectorsCut` builds and runs.** Ship spawns at the origin, the ride director steps through all seven phases with fog, light and sea changes, placeholder obstacles and hostiles scroll past, three placeholder crew stand on deck.
- **Orders work as text.** `dc.Order hard to starboard` in the console is parsed into an intent, a crew member takes the station and answers. Voice comes next.
- **Unattended test harness.** `Tools/shots.sh "3,130,300"` runs the game, seeks to those ride times, saves screenshots to `Saved/Screenshots/Auto` and quits. `Tools/build.sh` builds. Both need the editor closed.
- **Unreal MCP startup diagnosed.** The server only auto-starts if launched with `-ModelContextProtocolStartServer` or with `bAutoStartServer=True` in the editor user settings; both are now set up on this machine.
- **Ship asset notes.** `BP_Brigantine` needs a wind source actor in the level (spawned by the game mode) and starts simulating physics on its own; the game mode pins it at the origin. Bow is +X. Wheel at X -930 Z 757, main-deck guns at Z 540.
- **DLSS 4.5 plugin v8.7.2 for UE 5.8 downloaded** (`E:\projects\directorscut\Downloads\dlss`), not yet enabled.
- In progress: first packaged build; downloading llama.cpp, whisper.cpp and Piper for the voice loop.
- The editor is closed while I iterate on C++. Reopen it with: `UnrealEditor.exe DirectorsCut.uproject -ModelContextProtocolStartServer`. Tell me in INBOX.md if you need it open.

## Thu 12 Nov 09:50 — started unattended run

- Plan written: `PLAN.md`.
- Owner is away for 10+ hours; Claude is working autonomously toward milestone M1 (greybox, see plan).
- Found: the GitHub repo is **public**. The purchased Brigantine asset (2.1 GB, currently untracked) must not be pushed. Added it to `.gitignore`.
- Found: Visual Studio 2022 Professional is installed, so the project can have a C++ module.
- Next: add the C++ module, check the Unreal MCP server survives an editor restart, build the ride director.
