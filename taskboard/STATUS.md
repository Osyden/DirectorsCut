# Status log — Claude

Newest entry first. Times are local. Plan: [PLAN.md](PLAN.md). Messages to Claude: [INBOX.md](INBOX.md).

## 14 Nov Codex: descending spiral prototype ready for review

- User authorized a bespoke banked spiral and ship-centred movement, retaining the crater scale. [Prototype and ownership](MAELSTROM-SPIRAL-PROTOTYPE-14NOV.md).
- Coordinated directly with **Improve Maelstrom sky visuals**. Water/route and sky/lighting remain separate; shared DCMaelstrom.cpp edits require function-level merging.
- Build and extended route tests pass. Full normal-speed scene review and held-port narrowing test reached open sea; a final short run verified the fix for a folded bank near the centre. Actual captures and remaining visual gaps are linked in the prototype note. This is geometry/movement work, not a finished water, boss or underwater-exit pass. Isolated branch only; no main merge or packaged build.

## 14 Nov Codex: concept critique, finer water scale and a leftward feeding current

- Generated three explicitly labelled paintovers of the game frame. A/B fail the owner's far-wave scale requirement; C is the useful starting direction, not a final approval. [Concepts, exact prompts, critique and implementation plan](MAELSTROM-WATER-DIRECTION-14NOV.md).
- Removed the giant crossing swell field and angular bowl buttresses; narrowed foam sources, reduced ordinary crest scale, reversed the route to enter a leftward descending current before the lip, and preserved starboard controls. Built a guarded bridge to the installed water simulation's actual vertex displacement with hull/distance bounds. Added dedicated crest-spray fading and smaller current-driven droplets.
- The user proposed a separate chat for clouds and sky lighting. This pass leaves that system unchanged; the new port-facing view requires a revised sun composition. Coordinate edits to the phase values in DCMaelstrom.cpp and the shared plume generator/material. Boss iteration remains deferred.
- Work remains isolated on `codex/maelstrom-saturday`; no main merge, packaged build or main-editor changes. Visual quality is still in progress; read the detailed note before integration.

## 14 Nov Codex: maelstrom environment checkpoint; boss deferred

- Continued on `codex/maelstrom-saturday` in the isolated project. [Changes, replay instructions, review captures and remaining quality gaps](MAELSTROM-SCENE-POLISH-14NOV.md).
- Integrated wave crests, advected foam source, crest spray, corrected tunnel winding/normals and clearer daylight atmosphere. Crater dimensions and route retained. A rejected detached-sheet experiment was removed.
- Editor build and material generation pass; native 1440p reveal/descent captures taken. This is not finalized, packaged or merged into the active project. Boss iteration remains on hold at the user's request.

## 14 Nov Codex: maelstrom orbit/contact pass, isolated review branch

- Branch `codex/maelstrom-saturday`, based on `cf49b3b`; project `E:/Projects/DirectorsCut/Maelstrom-Saturday`. [Changes, tests and remaining gaps](MAELSTROM-14NOV-CODEX.md).
- Continuous descending orbit, bounded lateral steering and matching hazard displacement. Swimmers stop outside the hull and withdraw after a wake attack; tentacles strike water outboard with contact spray. Smaller lit weak points replace the oversized glowing bulbs.
- Editor target builds; route/steering automation passes at 30 and 120 fps. Offscreen encounter and close-up checks are documented in the handoff. This is not a claim of reference-quality visuals or a packaged release.
- Early-game, player, gunnery, crew, HUD, treadmill and global sky/weather files untouched. Review/cherry-pick the branch before merging into the active project; the overnight packaged executable does not contain these changes.

## 14 Nov 07:54 Claude: END OF THE OVERNIGHT RUN. Claim on the maelstrom files released

- Last build, from the last source of the night: `Builds/2026-11-14_0748_69449a3_handover/Windows/DirectorsCut.exe`, run from its exe start to ending card at 2x with the bot captain, no errors (six giants killed, both spawnlings sunk, beaten on the clock; eleven blows on stone, two on the stem, hull 12 at the card).
- Hand-over for the owner: `taskboard/MORNING-14NOV.md`. For Codex: `taskboard/FOR-CODEX-14NOV.md`. Dev log notes: `taskboard/DEVLOG-NOTES-NIGHT-13-14NOV.md`.
- **The maelstrom files (`DCMaelstrom*`, `DCLeviathan`, `DCPortal`, `DCWaveCurl`) are Codex's again.**
- Open, most important first: nobody has played or heard any of it; the helmsman steers against the current and puts her on a rock head on about once a city; the maelstrom's water and foam; balance.

## 14 Nov 07:45 Claude: why she is still put on a rock head on; one fix tried and taken out

- The current's log at a stem strike (207 s) shows the crew helmsman with the wheel hard to port, steering for a gap he had chosen on that side, while the current was setting her to starboard at 10 m a second: the two held her on the rock between them.
- Tried: while the current is setting, the helmsman puts the wheel the same way. Two runs: 3 blows, none on the stem, hull 81; and 12 blows, one on the stem, hull 45. Too uneven to call, so it is **taken out**; the source is as it was at the last measured state (6 to 8 blows a city, about one on the stem, hull 70 to 83 at the slide).
- For whoever takes this next: make the helmsman and the current agree on one side (the helmsman's chosen lane should win when it is clear on the feelers, the current when it is not), and test over five or more runs at normal speed, because one run tells nothing.

## 14 Nov 07:30 Claude: the current looks 150 m ahead; no clear gain

- The keel's line now runs 150 m ahead (was 90) and the set is a little stronger close in. Three whole cities at normal speed with the bot: 6, 7 and 8 blows on stone, with one on the stem every time, hull 83, 70 and 71 at the slide. Against 8 and 11 blows before, that is fewer blows but she is still put on a rock head on about once a voyage. Kept, as no worse; the head-on case is not fixed.
- Not in the twenty-fifth build.

## 14 Nov 07:16 Claude: rock strikes measured at normal speed

- Two whole cities at normal speed with the bot captain, no window: 2 and 3 contacts with stone (8 and 11 blows in all), one contact on the stem, hull 71 and 73 at the slide; 3 and 5 giants killed, 6 and 3 of their rocks on deck.
- The double-speed whole-voyage runs overstate rock damage: a blow's strength is reckoned from the ship's speed times the test's time scale, so at 2x most first blows are at the maximum.
- Still open: she can be put on a rock head on. Next thing to try: start the current's set earlier (the keel's line is 90 m; at 15 m a second that is six seconds) and let it act on stone found by either bow line when the gap that side is less than her beam.

## 14 Nov 07:07 Claude: twenty-fifth build packaged and run: `2026-11-14_0701_ef7691f_morning`

- `Builds/2026-11-14_0701_ef7691f_morning/Windows/DirectorsCut.exe`. The whole voyage from the exe, no window, 1600x900, 2x with the bot captain: no errors, all six giants killed (one rock landed), both spawnlings sunk, beaten on the clock at 160.0 s, ending card.
- **But** this run had sixteen blows on stone in the city, one on the stem, and the hull was 10 at the card. The run before had five and none. So the current and the helmsman's steering are not reliable yet, and my earlier "none on the stem" was three good runs, not a fix. Corrected in the hand-over.
- This is the build to play: it is the twenty-fourth plus the narrower waterspout.

## 14 Nov 07:01 Claude: waterspout narrower

- At 4K the spout near the ship was a slab a third of the view wide. It is now 0.62 as wide for the same height (about 70 m through at 400 m tall): a column, with the bowl seen either side of it. `review/50-waterspout/20-narrower-304-to-312s.png`. Its strike width is unchanged (26 m either side).

## 14 Nov 06:59 Claude: 4K speed with everything in

- True 3840x2160, editor build, bot captain: open sea with the wall close 71 to 77 fps, entering the tunnel 56, inside 67, city 91 to 98 (74 with a giant throwing), maelstrom 55 to 61 with a spout up and 43 to 45 in single frames as it passes the ship.
- The dips are the render thread (29 to 49 ms) waiting on the spout's volume frames; the graphics card is at 10 to 13 ms. Marching the spout in coarser steps (`-DCTwisterStep=3`) gave 52 where it was 45; a slower playback rate (`-DCTwisterRate=14`) gave 66 to 74 earlier. Left at 24 pictures a second and step 1: the choice between smooth boil and frame rate at 4K is the owner's.

## 14 Nov 06:53 Claude: twenty-fourth build packaged and run: `2026-11-14_0646_28b4e25_crew-scene`

- `Builds/2026-11-14_0646_28b4e25_crew-scene/Windows/DirectorsCut.exe`. The whole voyage from the exe, no window, 1600x900, 2x with the bot captain: no errors; five giants killed, five of their rocks on deck, five grinding blows on stone and none on the stem; both spawnlings sunk; beaten on the clock at 160.0 s; ending card. Hull 23 at the card with nobody steering or giving an order.
- This is the build to play. `MORNING-14NOV.md` is rewritten against it, with the owner's 04:45 to 05:20 notes answered one by one.

## 14 Nov 06:46 Claude: the crew are told what is actually round them

- `DCVoice.cpp` `CrewReply` told the language model that a wall of storm cloud lay ahead for every phase it had no line for, the ending included; that the inside of the front was "nothing to be seen"; and that a clawed monster was rising out of the whirlpool. It now describes the pale bank at the start, the cloud tunnel, the city with its giants, the unseen thing in the maelstrom with its spouts and stones, and a calm golden sea at the end. First ending line after the change: "Captain, the storm is gone and we have made it!"
- Not in the twenty-third build.

## 14 Nov 06:44 Claude: the end of the voyage checked at normal speed

- From 425 s to the card at normal speed, without a window: the plume and the creature at 438 s, the stage left at 450, a clean sunlit sea with no storm wall or cloud volume left over, and the card ("SHE CAME THROUGH", hull, damage taken, arms broken, score, orders) at 458 to 468 s. 50 to 64 fps while the plume stands, 99 to 123 after.
- One oddity: in the ending a crew line said "that storm wall blocks our way". The crew's lines are made by the language model from what it is told of the scene; I have not looked at what it is told there.

## 14 Nov 06:42 Claude: looked for objects popping in ahead; the city from the bow

- Eight frames a second apart from over the bow at 130 to 137 s (`review/65-city-from-the-bow`): rocks, columns and a statue come up out of the mist ahead and pass; none appears from nothing between one frame and the next. With things now laid 650 m ahead and the sea mist over the water, I could not catch a pop in stills. It needs an eye in motion to be sure.
- The same frames are the nearest yet to `References/fog-city-reveal.jpg`: pale mist on the water, ruins in layers behind it, the statue under a dusk sky.

## 14 Nov 06:39 Claude: twenty-third build packaged and run: `2026-11-14_0631_15f5585_spray-helm-carcass`

- `Builds/2026-11-14_0631_15f5585_spray-helm-carcass/Windows/DirectorsCut.exe`. Run from the exe without a window at 1600x900: front, tunnel and the whole city at normal speed with the bot captain (88 to 116 fps): four giants killed, three rocks on deck, five grinding blows on stone and none on the stem, hull 83 at the slide; the maelstrom at 3x to the ending card, both spawnlings sunk, beaten on the clock at 160.0 s.
- This is the build to play. It has everything logged so far.

## 14 Nov 06:31 Claude: the creature is seen once, as it is beaten

- `DCMaelstrom::UpdatePlume`: when the clock beats it, the creature's body (the Claw Kaiju at 18 times, in the arms' dark hide, playing its defeated animation at half speed) heaves two thirds out of the water at the heart of the crater inside the plume, more than a kilometre off, holds, and is under again by 19 s. `-DCNoCarcass`, `-DCCarcassScale=`.
- `review/64-the-creature-at-the-end`: at 409 s a jagged shape stands at the plume's foot; by 416 it is going under. It reads as a pale broken mass in the spray rather than a clear animal: the plume and the distance hide most of it, which keeps the crater the biggest thing in the picture, as the design asks, but it is not a strong image yet. With `-DCNoPlume` it does not show at all (the same switch skips it).
- Not in the twenty-second build.

## 14 Nov 06:26 Claude: bow spray is sheets of white water, not a soft ball

- Owner: "the bow water splashes are not really looking good yet". Each plunge of the stem used to raise one soft ball of mist. It now also flings two ragged sheets (Easy Fog cards standing on edge, leaning away from the hull) up and out from the stem, one to each side; they rise fast, open out, are carried aft and fall back inside two seconds, and the ball of mist behind them is thinner. Without the fog pack the old ball is all there is.
- `review/63-bow-spray`: from above the forecastle, a burst at the stem that spreads into torn white and blows back. Not judged in motion or from the helm, where the forecastle hides most of it.
- Also looked for and not found: the rock outline in the cloud tunnel (frames at 62 to 80 s show none).

## 14 Nov 06:15 Claude: twenty-second build run; the helmsman was steering her on to the rocks; current strengthened

- **Build** `Builds/2026-11-14_0543_17df8b3_giants-bolts-current/Windows/DirectorsCut.exe`, run from the exe without a window at 1600x900: front, tunnel, city to 236 s at normal speed (90 to 127 fps), six giants sent, four killed, five rocks on deck; the maelstrom at 3x to the ending card, both spawnlings sunk. The giant's mesh and animations are in the packaged content.
- **Why she still struck stone, and on the stem**: a log of the current (`-DCCurrentLog`) showed the crew helmsman throwing the wheel hard over for every 2.5 m he was off his chosen lane; she carried her swing past each gap and on to the stone beyond, at 12 m a second across the channel, which no current could hold. He now steers for where she will be in a second and eases the wheel as he nears his lane (`DCCrew.cpp` `TickHelm`). The current itself is stronger (3.2 to 11 m a second, rising as the stone nears).
- One whole city at normal speed with the bot, after both changes: four contacts (ten blows), one of them on the stem, hull 71 at the slide with three giant rocks landed and five giants killed. Before tonight a lane rock was a 3-point hit that vanished; they are now grinding contacts. Not head-on-proof yet.
- These two changes are **not** in the twenty-second build.

## 14 Nov 05:42 Claude: stone giants on the banks of the city, throwing fire (owner's 05:20 request)

- `DCTreadmill::TickGiants`: six through the city (96, 118, 141, 164, 187, 208 s), port and starboard by turns. Each is Paragon's Rampage in the stone (Elemental) skin with its glowing cracks, 16 times life size (about 45 m), wading. He starts 460 m ahead and out beyond the bank, wades in to stand 82 m from the middle of the channel facing the ship, and the crew call him and name the battery. Once the ship is within 150 m he tears up a rock (the tell, 1.6 s), throws it burning in a three-second arc at the deck (2 damage and a deck fire, which someone must put out), and does it again every six seconds until she is past. He has 4 health and is an urgent target for that side's battery; killed, he plays his death and sinks, 200 points. Without the pack none are sent. `-DCNoGiants`, `-DCGiantAt=<ride time>`, `-DCGiantScale= -DCGiantSunk= -DCGiantYaw=`.
- Seen: `review/62-giants`. He reads well: dark stone, red-lit cracks, shards of rock round him, in the mist under the ruins.
- Run at 2x with the bot captain's guns manned: four of the six were killed (at about 145 m ahead), eight rocks landed on deck. With rocks in the lane as well the hull was 64 at the slide. The first throw now waits until the ship is within 150 m (was 210) so a manned battery usually gets him first. The balance wants a hand on it.
- Marchosias and the giant demons are not used: one creature, as asked.

## 14 Nov 05:27 Claude: the 04:45 and 05:00 fixes are built and run (editor closed at about 05:40)

- Built clean. One run at 3x with the bot captain to 220 s and one at normal speed to 120 s, both without a window (`-RenderOffscreen`, so the test no longer takes the owner's keyboard).
- **Lightning bolt**: seen: a white forked streak from the cloud to the sea, not fogged out (`review/61-bolts`). It lasts a third of a second and is thin; 33 were drawn in the 3x run. Bolts now fall within about 20 degrees of dead ahead.
- **Rocks in the lane are stone**: at 3x the bot took three groups of strikes (115, 156 and 210 s; ten blows, hull 75), two of them on the stem. At normal speed there were none up to 120 s. The set of the current cannot clear a rock at three times speed; it needs judging at normal speed over the whole city, which I have not done yet.
- **Spawn distance 650 m**, **stone hidden until the cloud tunnel has passed it**, **front's foot on the water**: in the build; the tunnel rock and the popping have not been looked for in frames yet.

## 14 Nov 05:18 Claude: WRITTEN, NOT COMPILED, NOT COMMITTED: popping, clipping, current, tunnel rock, bolt material

The owner's Unreal editor has been open since 05:10, so nothing can be built. These edits are in the working tree only (`DCTreadmill.cpp/.h`, `DCFogFront.cpp`, `DCRideDirector.cpp/.h`). The next firing that finds the editor closed builds and tests them; until then the newest tested build is still `2026-11-14_0431_9a61a5e_torn-sea`.

- **Objects popping in ahead**: rocks, columns and statues were spawned 300 m ahead, inside what can be seen in the city. Now 650 m.
- **The ship clipping through things**: rocks in the lane were not stone at all: a circle that, when the ship reached it, took 3 hull and vanished. They are now felt by the hull's feelers like the ruins, so she is shouldered off and grinds. (Also: the fog cards were being given collision when near, because they are registered like scenery; they no longer are.)
- **A set of the current round big stone**: three lines run 90 m ahead from the bow (keel and each side). Stone on the keel's line sets her toward the clearer side at 1.8 to 6.2 m a second, harder the nearer it is, until the keel's line is free; stone on one side only is left, so she grazes it. `-DCNoCurrent`.
- **Rock showing through the cloud tunnel**: stone is kept out of sight until the tunnel's far end has gone by it.
- **Bolt material**: the drawn lightning bolts switched to `M_Bolt` (not fogged), thinner colour.

## 14 Nov 05:16 Claude: lightning bolts drawn (part built); the owner's editor is open, so no builds for now

- **Bolts**: the sky pack's "Flash Lightning" shows no streak in our fog. The director now draws one with every bolt it calls: a jagged line of white-hot rods from 750 m up to the sea with two forks, for a third of a second, ahead of the ship and inside the helm's view (`-DCNoBolts`, `-DCLightningLog` prints where). As built, it uses the glow material and is fogged out beyond a few hundred metres: one frame shows a faint streak. `M_Bolt` (`Tools/make_bolt_material.py`: additive, unlit, not fogged) is made and the source switched to it, **but that last step is not compiled**: the owner opened the Unreal editor at 05:10 and a build cannot run while it is open.
- Owner's second note (05:00, in INBOX): a rock on the right pops through the cloud tunnel as an outline; large objects should be given a little berth by the current so the ship scrapes them and cannot hit head on.
- Until the editor is closed I am writing the code for those without compiling it, and running nothing that opens a window: tonight's test windows were taking the owner's keyboard and mouse (tests now run with `-RenderOffscreen`).

## 14 Nov 04:56 Claude: owner's 04:45 notes (in INBOX); the front's foot on the water; sea smoke over the city

Owner, half asleep, at about 04:45: a gap between the sea and the bottom of the storm front; objects popping in ahead of the ship; collisions not always working, the ship clips through some things; lightning is flashes, not streaks; keep going but do not neglect the later game and the boss.

- **Gap under the front**: my floor for the cloud was 12 m over the water for the wall pieces and 2.5 m for the tunnel, with a fade above that. Both are now at the water with a short fade.
- **Sea smoke** (`DCTreadmill.cpp`, `-DCSeaSmoke=<density>`, 0 off): one thin layer of mist 1.2 km by 1 km and about 7 m deep that goes with the ship, so the water out to the ruins lies under pale mist everywhere, as in the reference, and the ruins stand out of it. 113 to 124 fps in the city at 1280x720. `review/60-sea-smoke-and-foot`.
- Three of tonight's test runs have odd camera positions in them (and one a gun firing): the test window takes the keyboard and mouse while the owner is typing. Not a fault in the game.
- Next, in this order: lightning bolts that can be seen, objects popping in, the hull passing through things, then back to the boss.

## 14 Nov 04:39 Claude: twenty-first build packaged and run: `2026-11-14_0431_9a61a5e_torn-sea`

- `Builds/2026-11-14_0431_9a61a5e_torn-sea/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: a clean open sea at 30 s, wall and portal at 56, a flash in the tunnel at 63.5, the torn sea in the city at 120 and 150 (94 to 114 fps), no material failed to compile; the whole maelstrom at 3x with the bot captain from 262 s: both spawnlings sunk, 7 arms broken, beaten on the clock at 160.0 s, exit at 450.01, ending card.
- This is the build to play. It has everything logged so far, including the plain stand-in column for spouts.

## 14 Nov 04:31 Claude: wind-torn white on the sea in the city

- The reference's sea is torn white; ours was dark and glossy. `Tools/patch_drw_shorefoam.py` (the water plugin patch; re-run both steps after pulling) now adds drifting patches of the shore's foam lace over the open water, as much as the fourth number of `MPC_Sea`'s `Trough` says. The game sets that from the wave height: nothing on the open sea (checked at 30 s: clean), full over the city. `-DCWhitecaps=0..1` fixes it.
- Result at 90 to 200 s: the sea is greyer and broken, with streaks of foam on the swells; it is not the reference's breaking crests with spray. `review/59-whitecaps`. 87 to 125 fps at 1280x720, no change.
- What did not work: putting the white only on wave crests. The surface height the material reads did not pick the crests out at any threshold I tried, so the white is in patches by position, not by wave.
- Not in the twentieth build.

## 14 Nov 03:59 Claude: the city against `References/fog-city-reveal.jpg`, and two things tried

Side by side with the 4K frame at 120 s:
- **Like it**: dusk-blue palette, warm lanterns as the only warmth, wet deck, ruins standing as shapes in pale fog, lightning behind them.
- **Unlike it**: (1) the reference's sea is torn white, with breaking crests and spray blowing off them; ours is dark and glossy with no white on it. (2) Its fog is everywhere and is the brightest thing in the picture, with the far city fading into it in layers; ours is patches with clear dark air between. (3) It has figures working the deck; ours has mannequins standing. (4) Faces in the cloud (parked).
- **Tried, no gain**: the fog cards at 2.2 times density (`-DCFogCards=2.2`) mostly thickened the rags across the deck, which then catch the lantern light as a yellow smear; the water plugin's foam strength at 1.2 against 0.45 (`-DCSeaFoam=1.2`) made no visible difference to the sea in the city. Both left as they were.
- **So the next real step for the city** is whitecaps and blown spray on the sea (the plugin's foam does not answer; it wants its own layer, as the wake and shore foam have) and a continuous pale mist layer low over the whole channel, not more patches.

## 14 Nov 03:47 Claude: foam disc not found again; dev log notes written

- The solid disc of foam round shallow broken columns (seen from the air on 13 Nov) did not show in four aerial frames over the city at 104 to 176 s, and it cannot be seen from the deck. Fixing it means re-patching the water plugin's material; not done for a fault the player does not see.
- The helmsman striking rocks in the city with nobody giving orders is left as it is: hull about 83 by the slide, and steering is the captain's job.
- `taskboard/DEVLOG-NOTES-NIGHT-13-14NOV.md`: the night's builds against what each added and where its pictures are, with the things worth telling, for the owner's dev log.

## 14 Nov 03:42 Claude: a plain column stands in for the waterspout where the cloud pack is not installed

- Without `Content/Clouds_Tunnel_Loop` a spout was unseen but still struck. It is now a pale translucent column (the engine cylinder with `M_Cloud`) with the same white water at its foot if Easy Fog is there. Seen with `-DCNoCloudPack` (which makes the spouts behave as if the pack were absent): a faint grey-white pillar, readable against the bowl, poor against the sky. The plume and the front have no stand-in.
- Not in the twentieth build; it changes nothing where the pack is installed.

## 14 Nov 03:41 Claude: twentieth build packaged and run: `2026-11-14_0334_0739c01_over-the-lip`

- `Builds/2026-11-14_0334_0739c01_over-the-lip/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: the bank at 44 s, wall and portal at 56, a flash in the tunnel at 63.5, the city at 120 (95 to 113 fps); over the lip at normal speed from 262 s, frames at 281 to 310 (74 to 108 fps); the rest of the maelstrom at 3x with the bot captain: both spawnlings sunk, 8 arms broken, beaten on the clock at 160.0 s, exit at 450.00, ending card.
- This is the build to play. It has everything logged so far.

## 14 Nov 03:34 Claude: over the lip: she hangs three seconds, then the bow drops

- `DCMaelstrom::UpdateStage`: the descent now starts at 12 s after the breach (was 9), so for three seconds after the tunnel is gone she rides level on the lip. Then the whole bowl tips up ahead of her by 6 degrees between 12 and 17 s and settles back by 32 s (`-DCLipTip=<degrees>`); from the deck the far rim climbs and the bowl fills the lower half of the view, which is the bow dropping. The wider field of view follows the same seconds.
- Checked from outside at 282 and 288 s: the hull sits on the water, no water through the deck. `SampleSurfaceWorld` goes through the arena's transform, so spouts, stones and spawnlings stand on the tipped surface.
- `review/58-over-the-lip`. Not judged in motion; my first try had the sign wrong (the bow rose). Not in the nineteenth build.

## 14 Nov 03:29 Claude: the dark bar by the portal is the underside of the piece over the mouth

- `-DCNoFrontOver` leaves that piece out: the bar goes, but open sky shows over the tunnel between two towers of cloud. So the bar is its cut underside in shadow. Its cut now fades over 70 voxels (was 30) from a little lower; the band is softer but still there at 56 s, reading as a dark flat cloud base. Left at that.
- Not in the nineteenth build.

## 14 Nov 03:24 Claude: dark level bar right of the portal: one guess tried, not it

- At 54 to 57 s a dark, level band shows in the wall to the right of the portal. Guess: the right-hand wall piece being set 90 m back let the cut underside of the piece over the mouth show. Setting it back only 15 m did not remove the band; reverted. Still there. The next thing to try is the over-mouth piece's floor (raise it or soften it further), or leaving that piece out.

## 14 Nov 03:22 Claude: nineteenth build packaged and run: `2026-11-14_0315_e71ea99_rolling-top`

- `Builds/2026-11-14_0315_e71ea99_rolling-top/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: the bank at 44 s, wall and portal at 56, a flash in the tunnel at 63.5, the city at 120 (94 to 114 fps), no material failed to compile; the whole maelstrom at 3x with the bot captain from 262 s: both spawnlings sunk, 8 arms broken, beaten on the clock at 160.0 s, exit at 450.03, ending card.
- This is the build to play. It has everything logged so far.

## 14 Nov 03:14 Claude: the wall's top rolls; no longer one level line from far off

- `M_VdbCloud` has a `Crown`: nothing above a ceiling that rises and falls along the volume's length (two sines, phase per piece). The three wall pieces use it (ceiling at 118 of 160 voxels, rolling 30 either way; `-DCFrontWallCrown=0` turns it off, `-DCFrontWallCrownRoll=`). From 38 to 54 s the bank on the horizon is now hills and saddles of cloud, not a mesa. The tunnel, spouts and plume do not use it. `review/57-one-front/10-rolling-top-32-to-57s.png`.
- A slip on the way: my first version read a fourth channel a vector parameter does not have, the material failed to compile, and the game drew **no wall and no tunnel at all** without any error on screen. Caught in the frames; the log line is "Failed to compile Material". Worth a grep after any material change.
- Not in the eighteenth build.

## 14 Nov 03:07 Claude: eighteenth build packaged and run: `2026-11-14_0301_5018846_one-front`

- `Builds/2026-11-14_0301_5018846_one-front/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: the bank on the horizon at 32 and 44 s with no slab behind it, wall and portal at 56, a flash in the tunnel at 63.5, the city at 120 (93 to 104 fps); the whole maelstrom at 3x with the bot captain from 262 s: both spawnlings sunk (172 and 135 m off), 7 arms broken, beaten on the clock at 160.0 s, exit at 450.02, ending card.
- This is the build to play. It has everything logged so far, including `-DCNoHUD`.

## 14 Nov 03:01 Claude: one storm front on the open sea, not two

- Frames at 20 to 50 s still showed what the owner first complained of: the sky pack's radial storm as a flat-topped slab with streaked sides, standing behind and above the new wall of cloud volumes, each cutting across the other.
- When the wall exists (`Director->bCloudWall`, set by the fog front), the pack's storm now has no strength until 50 s and comes to full by 61 s (`-DCStormLateFrom= -DCStormLateTo=`), so on the open sea there is only the wall: a thin pale bank on the horizon from about 26 s that grows into the wall by 54 s. The pack's storm then comes in over the ship with the wall as the dark overhead and the rain. Without the pack installed the old behaviour is kept.
- The cloud volumes are now marched out to 10 km (were 6), so the bank is seen earlier. 74 to 101 fps at 1280x720 from 20 to 70 s.
- `review/57-one-front`. Not in the seventeenth build.

## 14 Nov 02:56 Claude: clean 4K pictures for the store page and dev log

- `-DCNoHUD` draws no HUD (the crew's name tags, which are in the world, still show).
- Thirteen true 3840x2160 frames without the HUD are in `E:/projects/directorscut/StoreShots/2026-11-14` (outside the repository, 147 MB): the wall and portal at 56 and 57.7 s (the second on a flash), the tunnel mouth, two flashes inside the tunnel, the city at 90, 100.1 (a flash), 120 and 150 s, and four of the maelstrom with a spout and a burning stone. A contact sheet of the first nine is in `review/56-store-shots`.
- Not in the seventeenth build (the switch only).

## 14 Nov 02:51 Claude: the seventeenth build run from start to ending card at normal speed

- From the packaged exe at 1600x900, bot captain, no sound, 0 to 470 s without time scaling: no errors, 23 frames taken on the way. 110 fps on the open sea, 75 to 86 through the wall and tunnel, 111 to 154 in the city, 102 to 105 in the slide, 65 to 73 in the maelstrom, 131 at the ending.
- In the maelstrom, with nobody steering: all six spouts and all five stones struck, both spawnlings were sunk by the manned batteries (165 and 150 m off), 11 arms broken, beaten on the clock at 160.0 s, hull 36 at the card.

## 14 Nov 02:41 Claude: seventeenth build packaged and run: `2026-11-14_0235_0a152ce_guns-reach-down-slope`

- `Builds/2026-11-14_0235_0a152ce_guns-reach-down-slope/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: wall and portal at 56 s, a flash in the tunnel at 63.5, the city at 120 (81 to 111 fps); the whole maelstrom at 3x with the bot captain from 262 s: port spawnling sunk 160 m off, starboard one sunk 151 m off, 8 arms broken, beaten on the clock at 160.0 s, plume, exit at 450.01, ending card, hull 50.
- This is the build to play. It has everything logged so far.

## 14 Nov 02:35 Claude: found why shots to starboard never reached anything below the ship in the maelstrom

- `-DCGunLog` showed every ball fired at the starboard spawnling ending at exactly sea level, 8 to 15 m from the ship's side, as a "hit". The open sea (`BP_DynamicRealWater`) is hidden in the maelstrom but its flat sheet still stops the shot's line trace, so anything fired down the bowl's slope burst on an unseen plane beside the hull. This is older than tonight: it will have stopped the captain's own shots at anything below the ship's level too.
- `DCGunnery.cpp`: in the maelstrom a trace hit on a level surface at sea level is passed through. With that the starboard spawnling was sunk 167 m off in the first run.
- Not in the sixteenth build.

## 14 Nov 02:31 Claude: sixteenth build packaged and run: `2026-11-14_0226_201abae_spawnlings-sinkable`

- `Builds/2026-11-14_0226_201abae_spawnlings-sinkable/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: wall and portal at 56 s, a flash in the tunnel at 63.5, the city at 120 (82 to 105 fps); the whole maelstrom at 3x with the bot captain from 262 s: port spawnling sunk 160 m off, starboard one reached the hull, 5 arms broken, beaten on the clock at 160.0 s, plume, exit at 450.02, ending card, hull 37.
- This is the build to play. It has everything logged so far.

## 14 Nov 02:25 Claude: why the starboard battery never sank a spawnling, and what was done

- The battery was firing at it (five broadsides in one run), but the guns do not lead a target: to starboard the creature is far down the slope (100 m below the ship at 176 m off), the balls take two or three seconds, and at 13 m a second it had moved 25 to 40 m on by the time they fell. To port it comes down from the rim, nearer and level, and was hit.
- Now: it crosses the open water at 13 m a second and comes up the last 200 m to the hull at 5, which is the stretch the batteries reach; its target is a little larger (1500, was 900) and is marked urgent, so a battery takes it before a nearer arm joint (`ADCTreadmill::AddExternalHostile` has a fourth argument, `FindHostile` honours it). They are sent at 92 and 108 s after the breach and take 48 s from sight to hull.
- Two runs with the bot's guns manned: port sunk at 176 and 166 m off, starboard sunk both times but only 7 m off the hull. The starboard battery still mostly misses down the slope; I have not fixed its aim.
- `-DCGunLog` logs where each shot ended.
- Not in the fifteenth build.

## 14 Nov 02:18 Claude: a sunk spawnling rolls and goes down; batteries open fire at 200 m

- The guns' target is now an unseen actor riding with the spawnling. When they destroy it the creature plays its defeated animation, rolls and sinks over four seconds instead of vanishing (`review/55-spawnling/20-sunk-390-to-396s.png`: whole at 390 s, claws only at 391.5, gone by 392.5).
- Crew batteries open fire on a hostile inside 200 m (was 130; `DCCrew.cpp` `TickCannons`, one number, Codex's file). In four runs with the bot's guns manned the port spawnling was sunk each time, between 27 and 123 m off. The starboard one reached the hull each time: I have not found out why that battery does not sink it.
- Not in the fifteenth build.

## 14 Nov 02:13 Claude: fifteenth build packaged and run: `2026-11-14_0207_974b398_spawnlings`

- `Builds/2026-11-14_0207_974b398_spawnlings/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: wall and portal at 56 s, a flash in the tunnel at 63.5, the city at 120 (83 to 105 fps); the whole maelstrom at 3x with the bot captain from 262 s: the first spawnling sunk 21 m off, the second reached the hull, 8 arms broken, beaten on the clock at 160.0 s, plume, exit at 450.03, ending card, hull 46.
- This is the build to play. It has everything logged so far.

## 14 Nov 02:07 Claude: maelstrom damage lowered; second spawnling comes to starboard

- With the spawnlings added, a voyage on which nobody steers or gives an order ended at hull 0 before the finale. Now a spout or a stone does 3 (was 5), a spawnling at the hull 5 (was 8) and it has 3 health (was 5). The same passive run through the maelstrom alone now ends at 28 from 100. The creature's arms still do 2.5 a strike.
- The second spawnling was also coming to port; it now comes to starboard.
- In two runs with the bot captain's guns manned the batteries sank one spawnling of four. They only fire inside 130 m, which leaves them about ten seconds.

## 14 Nov 02:02 Claude: the creature's young swim in on the beam, to be sunk before they board

- `DCMaelstrom::UpdateSpawnlings`: two in the later fight (100 s to port, 123 s to starboard after the breach). Each is the Claw Kaiju mesh at 0.6 (about 28 m), crawling, in the dark hide the creature's arms have (its own materials drew it pale jade). It starts 300 m off the beam with a crew call naming the battery, comes on at 13 m a second, and is a hostile on the treadmill with 5 health, so a manned battery on that side fires at it inside 130 m and the captain can fire at it from the guns sooner. Sunk: the treadmill's 100 points and "It is going under!". At the hull: 8 damage, the rigging torn, a deck fire, "It is on the hull!". `-DCNoSpawnlings`, `-DCSpawnlingAt=`, `-DCSpawnlingScale= -DCSpawnlingSunk= -DCSpawnlingYaw=`.
- Checked: with no guns manned it reaches the hull at 23 s and the fire starts; with the bot captain's guns manned the first was sunk 12 m off, at the last moment. `review/55-spawnling`.
- It is not a boarding fight: nothing comes on deck. It dies by vanishing; it has no wake and no sound of its own. Which way it faces was judged from one small picture.
- Not in the fourteenth build.

## 14 Nov 01:53 Claude: the burning stone has a rock at its head

- The stone's head is now a rock from the ruins pack, eight metres through, with the glowing tail behind it (a glowing ball as before if the pack is absent). Tried the deck fires' flame cones for the tail: in the maelstrom's daylight they are a flat pale yellow and hide the rock, so the tail is glowing balls again.
- Not in the fourteenth build.

## 14 Nov 01:49 Claude: fourteenth build packaged and run: `2026-11-14_0143_b4a71b8_spout-sound-cut`

- `Builds/2026-11-14_0143_b4a71b8_spout-sound-cut/Windows/DirectorsCut.exe`. Run from the exe at 1600x900 without sound: wall and portal at 56 s, a flash in the tunnel at 63.5, the city at 120 (82 to 103 fps); the whole maelstrom at 3x with the bot captain from 262 s: 11 strikes, 10 arms broken, beaten on the clock at 160.0 s, plume, exit at 450.01, ending card.
- This is the build to play. It has everything logged so far, including the hazards' sound and the spout fix.

## 14 Nov 01:41 Claude: the rising spout no longer shows as a pale box; white water under it comes first

- Seen from above in its first seconds the spout was a faint slab with straight sides: the volume has thin haze right out to the edges of its box, which shows when the cloud is thin. `M_VdbCloud` has a new `Cut` (density below it counts as empty; 0 leaves the volume as it is). Spouts and the plume use 0.1 (`-DCTwisterCut=`); the front's wall and tunnel stay at 0 and look as before (checked at 56 and 62 s).
- The dome of white water under a spout is denser and wider and is there within a second and a half, before the column has risen.
- Not in the thirteenth build.

## 14 Nov 01:36 Claude: sound for the spouts, the stones and the plume (written, not heard)

- `DCAudio`: `SpoutRoar` (0 to 1) adds to the wind bed; `Boom(volume)` plays the recorded water impact pitched down with one of the synthesised thunders. The maelstrom sets the roar from the nearest spout (full when it is on the ship, nothing beyond 450 m) and holds it at 0.8 while the plume stands; a stone coming down and the plume going up each make a boom.
- One run with the sound device on (about 40 s, from 296 s, a spout, a stone, the plume): no errors. **It played through the speakers at about 01:40; I should not have run it with sound at night and have not again.** Nobody has heard whether it is right.
- Not in the thirteenth build.

## 14 Nov 01:33 Claude: thirteenth build packaged and run: `2026-11-14_0128_f26af10_helm-order-dodge`

- `Builds/2026-11-14_0128_f26af10_helm-order-dodge/Windows/DirectorsCut.exe`. Run from the exe at 1600x900: wall and portal at 56 s, a flash in the tunnel at 63.5, the city at 120 (81 to 104 fps); then the whole maelstrom at 3x with the bot captain from 262 s: 11 spouts and stones struck (the bot does not turn), 8 arms broken, "beaten on the clock at 160.0 s", plume, stage exited at 450.00, ending card. No script errors.
- This is the build to play: it is the twelfth plus the faster sideways carry for helm orders and the wider view over the lip.

## 14 Nov 01:28 Claude: the view opens out over the lip; 4K frames of the front looked at

- `DCMaelstrom::UpdateView`: the field of view widens by 9 degrees between 8 and 13 s after the breach and closes again by 30 s, over the same seconds as the quicker descent. A small thing; not judged in motion.
- Full-size 4K frames at 56 s (wall and portal) and 64 s (inside the tunnel on a flash) hold up: no voxel edges, no box sides. At 4K with the raised playback rates: 68 to 75 fps on the open sea with the wall close, 57 entering the tunnel, 74 to 75 inside, 96 in the city.
- Packaging a thirteenth build with the helm-order change and this.

## 14 Nov 01:24 Claude: one order to the helmsman clears a waterspout

- A helm order lasts 4 s. At the old rate that slid a spout about 36 m aside against a 38 m strike width, so one order did not reliably clear it. The helm now carries the ship across at 15 m a second in the maelstrom (was 11) and the spout's strike half-width is 26 m (was 38).
- Checked through the real order path, not the test switch: "Finch, hard to port" given about 8 s after the spout rose, with the rigging loose, and the spout passed 32 m clear. (`-DCOrdersLate=Finch,_steady;Finch,_steady;Finch,_hard_to_port` from `-DCMaelstromStart=296`; the two "steady" orders are only there to delay the third.)
- Not in the twelfth build.

## 14 Nov 01:22 Claude: twelfth build packaged and run: `2026-11-14_0116_26370c7_tunnel-lightning`

- `Builds/2026-11-14_0116_26370c7_tunnel-lightning/Windows/DirectorsCut.exe`. Run from the exe at 1600x900: sheet lightning in the wall at 50.7 s, the portal at 56, flashes inside the tunnel at 60.9, 63.5 and 66.4 (the statues showing in the cloud), the city at 120 (68 to 112 fps; the 68 is the frame of a flash); from 296 s a detailed spout, the strike, the creature beaten on the clock (`-DCBossDiesAt=50` for the test) and its plume (67 to 87 fps).
- This is the build to play. It has everything logged up to 01:22.

## 14 Nov 01:22 Claude: sheet lightning in the wall as it comes on

- Four more bolts on cue at 45, 50.5, 54.5 and 57.6 s, while the wall closes. The wall's pieces flare from within at about a third of the tunnel's strength (at the tunnel's strength the whole wall went pale blue from end to end). Frames at 50.7 and 57.7 s show the wall lit blue-white with the portal dark in it. It is one even flare over each piece, not a local one: Codex's review asks for a flash with a source and fall-off, and this is not that.
- One frame at 50.6 s in an earlier run still showed the sepia border with the bloom strength at 0; it did not recur in the next run. Unexplained.

## 14 Nov 01:16 Claude: lightning on cue inside the cloud tunnel; sepia border on flashes gone

- With the tunnel's air kept clear (Mist 0.15) the director's chance bolts, which need Mist above 0.3, never fell there, so the tunnel was ten seconds of dim grey. Four bolts now fall on cue at 60.8, 63.4, 66.3 and 69.5 s (`DCRideDirector.cpp`, by the `-DCLightningAt` hook). Each flash lights the tunnel's rolling walls blue-white from within through the material's flare; the third shows the first statues standing in the cloud. `review/54-lightning-in-the-tunnel`.
- The sky pack's flash bloom laid a sepia border round the whole picture on every flash; its strength is now 0 (was 0.5).

## 14 Nov 01:12 Claude: lights inside the cloud tunnel tried and left off; playback rates

- **Lights in the tunnel**: two point lights travelling with the tunnel (far end and under the crown, `-DCFrontTunnelLight=<strength>`). At full strength they white the inside out; at an eighth they turn the dark tunnel into pale even haze and put a sheen on the deck, because they light the air more than the cloud. Off by default. The dark tunnel with the pale opening ahead reads better.
- **Playback**: wall pieces now 18 pictures a second (were 12), spouts 24, tunnel 24. At 4K doubling the front's rates cost about 15 fps (54 to 74), so I stopped at these. Whether 18 shows as a flicker in the far wall needs an eye on it in motion.
- A walk through 8 to 256 s at normal speed, 25 frames: nothing broken; the helmsman still strikes rocks in the city (hull 81 by the slide).

## 14 Nov 00:52 Claude: eleventh build; spouts keep their detail at 4K; a plume marks the creature's end

- **Build** `Builds/2026-11-14_0041_fbabe67_spouts-stones-wisps/Windows/DirectorsCut.exe` (7.7 GB), run from the exe at 1600x900: wall and portal at 56 s, tunnel at 62, city at 120 (98 to 112 fps); spout, a burning stone striking and the second spout from 296 s (69 to 73 fps). It has the stones, the lightning fix, the deck wisps and the quicker descent. **Its spouts are soft at 4K** (see next line); the fix is after it.
- **Spout at 4K**: with non-blocking streaming the spout was a smooth grey slab with straight sides at 3840x2160. Now blocking again but played at 14 frames a second (the column's own spin carries the motion): detailed and turbulent, 66 to 74 fps at 4K while a cook was running in the background.
- **The end** (`DCMaelstrom::UpdatePlume`): when the clock beats the creature, a column of water a kilometre high goes up from the heart of the crater in three seconds, stands, and falls back over the next sixteen while the arms let go; the deck jumps. Seen at the right edge of the helm view at 411 to 420 s with `-DCBossDiesAt=140`. `-DCNoPlume`. No sound of its own beyond the creature's cry.

## 14 Nov 00:42 Claude: 4K speed with the night's work

True 3840x2160 (`-RenderOffscreen -ForceRes`, DLSS at 67 percent, no frame generation), from the editor build: open sea with the wall coming 82 to 88 fps, entering the tunnel 74, inside it 85 to 89, city with cards and wisps 97, maelstrom with a spout up 68 to 72 (55 in the frame the spout is raised). Tried and not kept: turning the fog off for the cloud volumes makes the far wall dark and detailed but it then reads as a cliff pasted on the horizon with its ends showing; with the fog it is a pale bank, which is what a far storm looks like.

## 14 Nov 00:34 Claude: rags of mist across the deck; white water at the spout's foot

- **Deck wisps** (`DCTreadmill::TickDeckWisps`): six upright Easy Fog cards, 9 to 22 m, come in over the bow and run aft past the masts at 7 to 13 m a second, over and over, while the mist is up (Mist above 0.45, so the city, not the tunnel). They fade near the eye and at each end of the run. Where they pass the lanterns they catch the warm light. `-DCNoDeckWisps`. Frames half a second apart in `review/53-deck-wisps` (the pictures were taken at a density a third higher than is now set). Not judged in motion.
- **Spout foot**: an Easy Fog card lies on the slope under each spout as a dome of white water. From the deck it is mostly hidden by the rail; from above it is faint.

## 14 Nov 00:28 Claude: burning stones from the deep, with deck fires

- `DCMaelstrom::UpdateBoulders`: five on a schedule (86, 104, 110, 126, 141 s after the breach, between the spouts). Each leaves the water 520 m off the starboard bow with a crew call ("Fire from the deep! ... Turn her, Captain!"), comes over in a 160 m arc for 5 s as a glowing stone with a tail of fire and a light, and falls where the ship was. The helm carries her out from under it at the same rate as for the spouts. Struck: 5 damage and a fire at the nearest deck point (the existing fire trouble, so the crew or the captain must put it out). Clear: 100 points and a burst on the water. `-DCNoBoulders`, `-DCBoulderAt=<s>` throws an extra one.
- Checked: helm amidships it strikes and "FIRE ON DECK" shows with flames by the wheel; `-DCSteer=-1` it falls 56 m clear. Spout damage lowered from 6 to 5.
- With the bot captain, who never turns, from 262 s: all six spouts and all five stones strike, 10 arms broken, hull 100 to 27 over the maelstrom. There is no sinking: the hull stops at 0 and the card says "BARELY AFLOAT".
- Look: the stone is a plain glowing ball with a tail of smaller balls; no smoke, no splash column, no sound of its own. `review/52-fire-from-the-deep`.

## 14 Nov 00:20 Claude: lightning no longer whites out the city; storm growth measured smooth

- **Lightning**: the sky pack's flash light at its own strength turned near stone and the deck pure white for about half a second (frames at 100.1 to 100.6 s with `-DCLightningAt=100`). `Lightning Flash Light Intensity` is now 4: the fog behind the ruins lights up and they stand as shapes against it. One of yesterday's three leftovers.
- **Storm growth in steps** (owner's note of 13 Nov): with the redraw-every-frame settings, sixteen frames a tenth of a second apart from a fixed camera at 30 to 31.5 s change evenly (mean difference 2.1 to 2.65 a frame, no flat stretches followed by jumps). I did not capture the old behaviour for comparison.

## 14 Nov 00:13 Claude: cloud lining at the maelstrom tunnel's mouth tried and left off

- `DCMaelstrom::UpdateThroat` puts the tunnel volume (scale 220) inside the last of the water tunnel with 40 m standing out of its mouth. Result at 256 to 286 s: hardly seen inside the dark tunnel, a white-out at the mouth (271 s), then two lumps of cloud with a cut foot hanging behind the ship (273 to 275 s), and 53 to 64 fps. Off unless `-DCThroat`. The exit needs shaping in the water tunnel and the lip, as in Codex's plan, not a cloud over it.

## 14 Nov 00:10 Claude: tenth build packaged and run: `2026-11-14_0006_68b9d2c_storm-wall-spouts`

- `Builds/2026-11-14_0006_68b9d2c_storm-wall-spouts/Windows/DirectorsCut.exe`, 7.7 GB (1.3 GB more than the last: tunnel volume 01 and the fog cards).
- Run from the packaged exe at 1600x900: the cloud wall with its portal at 56 s, inside the tunnel at 60 and 64 s, the city with fog cards at 90 s (90 to 109 fps); from 262 s the slide, the breach and a waterspout at 304 and 309 s (84 to 109 fps). Tunnel, 68 fog cards and spout all present in the packaged content.
- Not in this build: the cloud lining at the maelstrom tunnel's mouth (written after it was started).

## 14 Nov 00:06 Claude: quicker first descent; spout cost found; whole voyage run; packaging

- **Descent**: 42 percent of the maelstrom's descent is now made between 9 and 29 s after the breach (`-DCPlunge=`), so the rim and its falls climb away overhead as the ship clears the tunnel. It was one even glide over the whole fight.
- **Cost of the spout**: the volume's blocking streaming requests cost the render thread 8 to 15 ms a frame at 45 frames a second of playback. Spouts now stream without blocking at 30: 73 to 77 fps at 1280x720 with a spout up, against 58 to 63 blocking. For the front's wall, non-blocking made the far cloud soft, so the front stays blocking (82 to 99 fps there). My earlier "33 to 57 fps in the maelstrom" was a 3x and 4x time-scale run; at normal speed the stage is 74 to 91 fps without a spout.
- **Whole voyage** at 3x with the bot captain, 0 to 468 s: no script errors, front, city, slide, maelstrom, six spouts (all struck: the bot does not turn), "beaten on the clock at 160.0 s", ending card "Hull 33%, Damage taken 67, Arms broken 10, Score 3050".
- **Packaged builds**: `MI_VdbTunnel01` (`Tools/make_vdb_instances.py`) pulls tunnel volume 01 alone into the cook; Easy Fog's material, mesh and texture folders are on the always-cook list. Build `storm-wall-spouts` is packaging now; not yet run.

## 13 Nov 23:52 Claude: maelstrom fight ends on the clock; waterspouts to steer round

- **Timed finale** (`DCLeviathan`, `DCMaelstrom::BeginPlay`): the creature is beaten at `FinaleAt` seconds after the breach (default 20 s before the ending phase, so 160 s; `-DCBossDiesAt=`), whatever the guns have done. The six-break early exit is gone. Each broken arm scores 150 and is counted; the results card shows hull, damage taken (repairs do not reduce it), arms broken, score, orders. Checked at 4x from 262 s: "beaten on the clock at 160.1 s", stage exited at 450.06, card correct.
- **Waterspouts** (`DCMaelstrom::UpdateTwisters`): six on a schedule (30, 50, 78, 96, 118, 134 s after the breach, none in the last 20 s before the finale). Each rises 420 m off on a line 20 degrees to starboard of the bow, stands 5 s as the tell with a crew call ("Waterspout dead ahead! Turn to port, Captain!"), then comes up the slope at 36 m a second. The helm slides it aside (11 m a second hard over); if its core crosses the ship she takes 6 damage and the rigging tears, otherwise 100 points. It is the bought tunnel volume stood on end and spinning, 260 to 620 m tall. Without the pack the hazard still runs, unseen (to fix: a fallback column). `-DCNoTwisters`, `-DCTwisterDensity=`, `-DCTwisterLog`.
- Checked: helm amidships, both first spouts strike; `-DCSteer=1`, both pass clear (110 m and 76 m off). `-DCSteer=` now holds the helm against the crew as well. Frames in `review/50-waterspout`.
- **Seen and not fixed**: the maelstrom runs at 33 to 57 fps at 1280x720 with the render thread at 20 to 30 ms (it did before the spouts); the bowl water is still striped; the spout's top is darker than its foot and it has no disturbed water at its foot; Finch does not dodge by himself (the captain must order it or take the wheel, which is the design).
- Next: the reveal (cloud tunnel over the exit, lip, drop), then fire and boarding hazards if time allows.

## 13 Nov 23:38 Claude: CLAIM on the maelstrom files for the night

Owner authorised this at 16:22 on 13 Nov and queued it again at 22:30. Until the morning hand-over I am editing `DCMaelstrom.*`, `DCLeviathan.*` and, if needed, `DCMaelstromAtmosphere.*`, `DCPortal.*`, `DCWaveCurl.*`, on main, in small commits. Codex: please do not edit these before reading `taskboard/MORNING-14NOV.md` (written at 08:00) or the STATUS entries above this one. I am following `MAELSTROM-TIMED-BOSS-DESIGN.md` and your `MAELSTROM-NEXT-PASS-PLAN.md`, in the order: timed finale and score, one telegraphed tornado to steer round, then the reveal.

## 13 Nov 23:36 Claude: cannon shot is an iron ball; lower guns fire from outside the hull

Owner's note: the guns fired a salvo of wooden bolts from behind the camera, with the flash behind the camera too.

- **Bolts**: each shot was drawn with a 2.5 m orange rod behind it. Now a dark iron ball (30 cm) with a thin grey streak (`DCGunnery.cpp`). `review/49-cannon-shot`.
- **Flash and shot inside the ship**: the five lower-deck muzzles were reckoned about a metre inside the hull (Y 372 to 466 where the side is at up to 567), so their flashes and balls started inside her. They are moved 1.1 m outboard. The four deck guns were already at their barrels' mouths (checked from the gun station).
- Test hook `-DCGunTestAt=<seconds>` fires a captain's starboard volley; with `-DCStation=starboard -DCTenths` it can be photographed.
- Not checked: the port side, the lower guns' flashes seen from outside, and whether this is all of what the owner saw (their build's gun camera may differ). DCGunnery is Codex's file by the README; the change is small and listed here for them.

## 13 Nov 23:26 Claude: clear air inside the tunnel

- The front-interior phase (58 s) had thick fog (density 0.035, Mist 0.44) from when the inside of the front was meant to be blind. It hid the tunnel's walls: after 61 s the picture was even grey. Now density 0.004 and Mist 0.15, so from 61 to 70 s the ship runs down a dark rolling tunnel toward the light at its far end, and the first ruins show at about 70 s. `review/47-front-wall-with-portal/10-inside-tunnel-61-to-86s.png`. 86 to 118 fps at 1280x720.
- Unjudged: the tunnel is dark; at normal speed it may want a little more exposure or lightning inside.

## 13 Nov 23:20 Claude: Easy Fog cards in the city; front skirt off; tunnel has no light of its own

- **Fog cards** (`DCTreadmill.cpp`, after the deck wisps): about 60 Easy Fog cards on the treadmill through the city: wide sheets (140 to 380 m) lying 2 to 4 m over the sea, and slanted rags of scud over the channel's shoulders. Lit by the scene only (the pack's emissive is set to 0), fade near the eye and at stone. Absent pack: nothing laid. `-DCNoFogCards`, `-DCFogCards=<density scale>`. Frames 78 to 200 s in `review/48-fog-cards-city`; 117 to 134 fps at 1280x720 in the city.
- **Front skirt off** when the cloud wall exists (`-DCFrontSkirt` brings it back): it laid a pale belt across the foot of the wall. The floor fade of the wall pieces is now 1.5 voxels, which removes the bright strip under them.
- **Tunnel glow 0**: nothing in the front lights itself now except the lightning flare (Codex's review point).
- Owner at about 23:15: "Always just keep going."
- Not done: deck wisps from cards (they need to move with the ship), far-off look of the wall (pale, straight top at 40 to 50 s), judging the storm growth for steps.

## 13 Nov 23:11 Claude: the storm front is a wall of the tunnel volume with the portal through it

- **Wall** (`DCFogFront.cpp`, end of `BuildTunnel`): three more instances of the same looping volume, about five times the tunnel's size and laid across the course like a roll cloud (about 4 km long, 1.1 km through, 900 m high, lower quarter under the sea): one either side of the mouth, one over it with its foot cut away above the tunnel's crown. Unequal sizes, a few degrees of turn and one set back so the top is not a level line. They travel with the tunnel's mouth and take its density and lightning flare. `-DCNoFrontWall`, `-DCFrontWallScale= -DCFrontWallDensity= -DCFrontWallGap=`.
- **Tint**: low blue-grey albedo (0.34, 0.42, 0.58; `-DCFrontTint=r,g,b`) so the low sun no longer turns the cloud to sand.
- Frames 24 to 59 s in `review/47-front-wall-with-portal`: a band on the horizon at 34 s, a towering wall with a lit opening at sea level by 56 s. 82 to 99 fps at 1280x720 with all four volumes in view.
- **Owner at about 23:08**: "Good progress, keep going, your intuition on what to improve next is right."
- **Still wrong**: far off (40 to 50 s) the wall is a pale band with a nearly straight top; a dark level bar shows right of the portal at 56 s; the sky pack's storm slab still stands behind and above the wall; inside the tunnel the light is still the open-sea sun; no Easy Fog wisps yet; the jerky formation fix is still unjudged.

## 13 Nov 23:03 Claude: cloud tunnel on the course as the mouth of the front; white banks in the city off

Owner's play notes tonight: the fog and cloud systems cancel and paste over each other; white self-lit fog in the city looks terrible and shows statues as outlines; the tunnel passed through the water; the tunnel must be the opening in the front and line up with it; the front itself fights the other clouds.

- **Tunnel on the course** (`DCFogFront.cpp`, `BuildTunnel/TickTunnel`): tunnel 1 at scale 300 rides at the face of the front, its mouth reaches the bow at about 59.8 s and it draws aft over 16 s. Absent pack: no tunnel, nothing else changes. Switches `-DCFrontTunnel=N` (0 off), `-DCFrontTunnelScale= -DCFrontTunnelSink= -DCFrontTunnelFloor= -DCFrontTunnelSoft= -DCFrontTunnelRate= -DCFrontTunnelTemp= -DCFrontTunnelDensity=`.
- **Own material** `M_VdbCloud` (`Tools/make_vdbcloud_material.py`): plays the volume with a floor just above the sea (the pack's material let it pass through the water), a storm-grey tint, a faint cold light in the folds and a lightning flare. It holds no pack data; the game sets the volume on an instance.
- **White banks off**: the sharp self-shadowed banks I made the default yesterday are what the owner saw as white, self-lit and pasted over the ruins. They do not take the local fog that darkens the scenery in front of them. Now off unless `-DCHeroBanks`; the soft banks are back.
- Frames 46 to 80 s in `review/46-tunnel-on-course`. 91 to 116 fps at 1280x720 through the tunnel.
- **Not right yet**: from the open sea the tunnel is a small ring standing alone, not an opening in a wall; the front is still the sky pack's slab. Next: build the wall of the front from the same volumes laid across the course (a roll cloud) with the tunnel through it, and dim the sun inside.
- Tried and left off: `r.HeterogeneousVolumes.SupportOverlappingVolumes 1` and `r.Translucency.HeterogeneousVolumes 1` made no visible difference at 54 to 76 s.

## 13 Nov 22:36 Claude: bought cloud tunnel draws in the game at ship scale; night queue set

- Owner installed three packs (all git-ignored): `Content/Clouds_Tunnel_Loop` (ten looping 201-frame volumes, 13 GB), `Content/EasyFog` (lit flowing fog cards), `Content/FogArea` (fog box through the fog grid).
- Trial switch `-DCVdbTunnel=N` (1 to 10) with `-DCVdbScale= -DCVdbAt=x,y,z -DCVdbYaw= -DCVdbRate= -DCVdbDensity=`. The pack's voxels are 0.42 cm, so scale 300 gives a tunnel about 850 m long, 240 m wide and 200 m high. Tunnel 1 at that scale: 128 fps at 1280x720 with the ship inside it. Frames in `review/45-vdb-tunnel-first`. As shipped it is brown cloud with teal light; both are material parameters and need retinting.
- Written and built, not yet judged in frames: the radial storm redraws every frame (it moved in steps: `Movement Update Fraction`, `Alpha Update Threshold`, `Tick Interval`), and the fog skirt under the front fades in instead of switching on.
- Owner's play-test faults and the night's order of work: `taskboard/NIGHT-QUEUE-13NOV.md`. Scheduler runs every 20 minutes until 08:00 on 14 Nov.

## Fri 13 Nov 18:29 — Claude: ninth build verified with the band fix; run ends

- **Build:** `Builds/2026-11-13_1824_4c3cc16_no-bands` runs: 133 fps open sea, 96 and 97 in the city at 1600x900. Its own frame at 150 s (the pose of the owner's complaint) shows ruins, cloud and dark sea, no sheet or slab. This is the build to play; [EVENING-13NOV.md](EVENING-13NOV.md) is updated.
- The scheduled run is over (it was to stop at 18:15; it ran on to finish the owner's 17:43 task). Scheduler cancelled.
- Queue for the next session, in order: the three remaining defects listed in the entry below; the maelstrom pass (claim the files here first); the owner's VDB pack once installed.

## Fri 13 Nov 18:24 — Claude: the "deformed band" on the half-pipe: three causes found, all fixed

Owner's 17:43 message (via Codex and a note from the other session): the broad stretched grey band on the left shoulder in `review/43-cloud-city/shot_0150`. Diagnosed with matched captures (trough on and off, fog and banks off, lit from the air). It was not one thing:

1. **It was mostly stone, not water.** Seen from the air with the fog off, the band is the stepped base of a temple: one stone block stretched to 44 by 30 m and then by the temple's scale, its face drawn out about a hundredfold. From the deck that is a smooth streaked sheet. The bases are now floors of many blocks near their own proportions (`RuinFloor`, one instanced component a floor; an unseen single block still stands in for collision, since making every block solid cost 12 ms a frame and 30 strikes a voyage).
2. **The base hung in the air.** The sea went on rising beyond the trough's shoulder, and a wide ruin was set at the height under its middle, so its near edge stood metres above the lower water beside the channel: a tilted slab over the ship. The trough now rises from the level middle to the shoulder (90 m out, 11 m up) and is **level again beyond it**, so the city stands on a level sea and nothing hangs. A steady pull past the middle of the slope brings the ship back, since the slope itself now eases toward the shoulder. Hard over she holds 66 m out, 9 m up.
3. **Codex's finding was right too:** the patch lifted the water without leaning its normal, so the slope was shaded as level water at a grazing angle. The normal now leans with the trough in all four ocean materials.

- Same pose before and after is in review set 44, with six frames through the city: no sheets, no slabs.
- Voyage at 3x with the bot captain after the fix: city 93 to 105 fps at 1600x900 (the block floors cost a little), five light strikes, no rock hits, hull 89, slide reached.
- **Two of my builds this hour silently failed** (a comment I pasted swallowed a declaration; I was reading only the last line of the build output), so several captures in between were of the old binary. Caught when the next build printed the error. The numbers above are from a build that succeeded.
- Audit of the same captures for like defects: (a) a solid white disc of foam round a clump of broken columns standing in shallow water, seen from the air: the shore foam fills in wherever the stone lies just under the surface; not fixed. (b) Frames caught mid-lightning in the city are still very bright with the sharp banks. (c) Cloud walls end in fairly straight tops at some angles. Not fixed today.

## Fri 13 Nov 17:24 — Claude: 4K re-measured with the cloud city

- True 3840x2160, DLSS 67%, editor build, 3x run: 94 fps open sea, 89 inside the front, 77, 78, 82 and 91 through the city. The lower volume resolution that helped at 1600x900 does little at 4K, where the cost is in the pixels marched.
- Tried: marching the sharp banks at a quarter of the screen resolution instead of half (`r.HeterogeneousVolumes.DownsampleFactor 4`): 79, 88, 88 in the city and no difference I could see in a frame scaled to 1600x900. Not made the default this late in the run; it is the first thing to try if 4K speed matters more than it does now.

## Fri 13 Nov 17:03 — Claude: eighth build verified; lightning with the sharp banks checked; evening hand-over written

- **Build:** `Builds/2026-11-13_1645_2a0837e_cloud-city` runs: 132 fps open sea, 123 and 118 in the city at 1600x900. This is the build to play.
- Lightning with the sharp banks captured in tenths of a second: the cloud brightens and settles, no white-out (picture in review set 43).
- [EVENING-13NOV.md](EVENING-13NOV.md) is the hand-over for the owner.

## Fri 13 Nov 16:45 — Claude: seventh build; sharp banks made cheaper; the curtains before the great figures are sharp cloud too

- **Build:** `Builds/2026-11-13_1630_7323c3e_big-trough` runs (133 fps open sea, 95 in the city at 1600x900). It has the big trough and the sharp walls at their first, costlier setting. The two changes below are after it.
- **Cheaper:** the sharp banks at 48 cells a side look the same from the deck as at 64 or 96 (compared side by side) and bring the city back from 91 to 109 fps in the same frame. Now the default (`-DCHeroRes=`).
- **Curtains:** the tall banks in front of each colossus and temple are sharp cloud as well, and still thin away as the ship comes up. Nine frames through the city (review set 43): towers of lit cloud to either hand and ahead, ruins in silhouette between them, the channel dark and open. 98 to 131 fps at 1600x900. Frame 150 s is the nearest thing yet to `fog-city-reveal.jpg`.
- Still soft-fog only: the outer fields, the veils across the course, the deck mist.

## Fri 13 Nov 16:30 — Claude: owner's 16:22 message: half-pipe much larger; maelstrom pass authorised but not begun

- **INBOX 16:22 (owner via Codex):** the half-pipe must be substantially larger and partly fog-covered; and when current work runs out I may take a maelstrom visuals and gameplay pass ([MAELSTROM-TIMED-BOSS-DESIGN.md](MAELSTROM-TIMED-BOSS-DESIGN.md): timed boss death, hazards, weak spots, rollercoaster reveal). Read. **I have not claimed or touched any maelstrom file.** With about an hour of this run left it is not the time to open a stage that large; it is the first item for the next session, to be claimed here before any edit.
- **Half-pipe enlarged:** the channel is 90 m to its shoulder each side (was 50) and the sea rises 14 m there (was 6.5), on to about 31 m beyond. The helm has half as much authority again inside it and the stop is further out. Wheel hard over: she now climbs to 79 m off the middle, 10 m up, banked 18 degrees, and holds (`-DCSteer=1 -DCLaneLog`). The sharp cloud walls stand on the upper slopes, so the shoulders are partly hidden and show through the gaps. Picture in review set 42.
- Whole voyage at 3x with the bot captain after the change: every phase, ending reached, no ruin strikes, four rock hits, hull 79. 96 to 117 fps in the city at 1600x900.
- Not judged: whether the larger trough and its fog read as "immense" from the deck in motion, and how it feels to steer. Needs the owner.

## Fri 13 Nov 16:25 — Claude: sharp banks on leaving the front; tunnel tried again with them

- Leaving the front (66 to 92 s) the new banks stood out stark white against the flat grey light, like cut paper. They are now dimmer until the city's dusk has settled. The stretch reads as towering banks of cloud on a storm sea with the first ruins behind them. Review set 42.
- The tunnel inside the front (`-DCTunnel`) was tried again with the sharp banks: still nothing to see, because the front's own thick fog hides anything more than a few tens of metres off. A tunnel there needs that fog thinned and the city held back by other means. Left off; this remains the job for the owner's tunnel pack or a rework of the front's interior.

## Fri 13 Nov 16:10 — Claude: sixth build has the sharp banks and they survive the cook; billows enlarged

- **Build:** `Builds/2026-11-13_1602_2c8fffe_sharp-banks` runs: 130 fps open sea, 124 in the city at 1600x900, and its own screenshot shows the cloud walls to either hand with the channel between. The heterogeneous-volume material cooks.
- After it: the frosty speckle was the material's own fine pattern, not the renderer (three renderer settings made no difference). Billows are now about two and a half times larger, which reads as rolling cloud and less as cotton. 121 to 124 fps in the city at 1600x900 in the frames checked (one at 90 while a bank filled the view). Deck mist eased back (0.07): at 0.1 it browned the foreground.
- Note for whoever tests: straight after the material is regenerated the banks are missing for one run while shaders compile.

## Fri 13 Nov 16:02 — Claude: correction on heterogeneous volumes: they work; the fog walls are now sharp, self-shadowed cloud

- **I was wrong at 14:35.** Heterogeneous volumes draw in this project. My test volumes were beyond the engine's 300 m march limit for them, and my material assumed world positions where the engine gives the volume's own. Found by putting a plain one 40 m from the ship. The VDB hand-off carries the correction; the advice there to hold off on packs because "the renderer does not work" is withdrawn.
- **The fog walls along the course are now heterogeneous volumes** (`M_HeteroCloud`, `ADCFogBank::bHero`): ray-marched at their own resolution, shadowing themselves, lit by sun and lightning. From the deck they are crisp rolling cloud with bright crowns and dark undersides, the first fog in the game that looks sculpted. Review set 41 (set 40 is the same view before).
- **Cost: about a fifth of the frame rate.** City 97 to 123 fps at 1600x900 (was 120 to 138); true 4K, DLSS 67%: 77 to 86 (was 90 to 101). `-DCNoHeroBanks` returns to the old banks; `-DCHeroRes=` (96) trades sharpness for speed.
- Rough edges seen: a frosty speckle in the banks at some angles; one capture mid-lightning was washed white (flare on these banks since cut to a quarter, not re-captured).
- Not in a packaged build yet; the cook has not been tried with this material.

## Fri 13 Nov 15:25 — Claude: fifth dated build; deck wisps looked at again

- **Build:** `Builds/2026-11-13_1521_328e18c_sculpted-banks` runs (137 fps open sea, 126 city, 720p). It has the modelled banks.
- **Wisps over the deck:** a close look across the deck at 100 s showed none at all; they were too thin and dim for the dusk. Raised (density 0.05 to 0.1, more of the noise counts as fog). At 0.16 the deck had a visible warm mist round the lanterns, but it read as general haze, not running streaks, and began to go brown, so it is set between. Honest state: mist on deck yes, distinct wisps streaming across it no. Streaks finer than the fog grid's cells cannot be drawn in it. Not in the build above.

## Fri 13 Nov 15:06 — Claude: a cloud tunnel from fog banks tried in the front; not good enough, off

- Rings of banks round the course inside the front (`-DCTunnel`): invisible in the front's thick fog, mere side patches when that fog is thinned, and thinning it shows the city too early. Off by default. Recorded in the VDB hand-off as the one shape the present system cannot make, which is what the owner's tunnel pack is for.

## Fri 13 Nov 14:47 — Claude: fog banks modelled (lit tops, dark folds, edges)

- Answering "sculpted, not thicker": `M_FogVolume` now gives each billow an edge (density climbs quickly from nothing to full) and shades the bank's own glow by whether a point is open to the sky or under more cloud, and by its height. Banks read as rolling forms with bright tops and dark hollows. 120 to 138 fps at 1600x900. Review set 40. Not in a packaged build.
- Heterogeneous volume: three more settings tried, still not drawn; noted in the VDB hand-off. A finer fog grid (`-DCFogGrid=4,256`) costs a tenth of the frame rate; kept as a switch only.

## Fri 13 Nov 14:31 — Claude: owner's message on fog and the VDB pack; heterogeneous volumes tried and do not draw; fourth build

- **INBOX:** the owner is still not satisfied with the fog and has acquired a looping cloud-tunnel VDB pack (not installed; they will install it at home). Read [VDB-ATMOSPHERE-HANDOFF.md](VDB-ATMOSPHERE-HANDOFF.md); my findings are appended at its end.
- **Tried the renderer such a pack needs, without the pack:** one heterogeneous volume beside the course (`-DCHeteroTest`). It registers and reports visible, and does not draw, under every combination I tried. Cause not found. This is the first thing to settle when the pack is installed (open its demo map in this project), and a reason not to buy more packs before it is.
- The softness the owner dislikes is the fog grid's own: the banks cannot be sharper than it is, whatever is sampled into them.
- **Build:** `Builds/2026-11-13_1406_606526a_camera-and-strikes` runs (131 fps open sea, 134 city, 720p). It has everything up to the 4K check, including the working lean and shake and the strike scrape.

## Fri 13 Nov 14:06 — Claude: frame rate at 4K with everything in

- True 3840x2160 (`-RenderOffscreen -ForceRes`, DLSS at 67%, editor build, 3x run): 98 fps open sea at 30 s, 96 under the front, 100 inside it, 101 and 98 in the city, 90 late in the city. This morning's hand-over gave 84 to 102 for the same test, so the day's fog banks, foam, trough, grade and lightning have cost nothing measurable. Five frames scaled to 1080p are in review set 39.
- A first run at "3840x2160" without those switches silently rendered at the desktop's 1600x900 and read 107 to 121; discarded.
- Fog curtains now part when the ship is about 150 to 250 m from what they hide (it was nearer 400 m, further than anything can be seen in this fog).
- Packaging a fourth dated build with the camera fixes and the strike sound.

## Fri 13 Nov 13:45 — Claude: note for Codex; city into slide checked; less brown in the tunnel

- [FOR-CODEX-13NOV.md](FOR-CODEX-13NOV.md) lists what changed today that reaches the maelstrom stage (working, stronger camera shake above all). Taskboard row updated: the cinematic pass is merged.
- City into the slide watched frame by frame at 2x (215 to 268 s): the dusk city hands over cleanly, no long frames.
- In the slide's tunnel the lanterns' halo in the thick dark air was turning the picture brown, as it did in the front. The halo shrinks with `Env.Tube` (0.45 to 0.12). This is the director's lantern code, not the maelstrom's.

## Fri 13 Nov 13:30 — Claude: a hull strike now jolts the eye and has a sound of its own

- **Shake.** Now that the shake reaches the picture, a strike was measured at the eye: barely a degree. Shake angles are up by half and a strike adds more of it; the same strikes now roll the view up to 4 degrees for a moment (`-DCOutliers -DCSteer=1 -DCCrewLog -DCCrewLogFrom=22`). Thunder and the maelstrom's blows use the same shake and are stronger by the same half; Codex should know.
- **Sound.** Every strike on stone now also plays a scrape: a 1.7 s groan that grips and slips with grit over it, two variants, synthesised like the other effects (`DCAudio`, on the director's new `OnHullStrike`). `-DCDumpAudio` writes them to `Saved/AudioDump/hull_scrape_*.wav`; they measure as sustained and brighter than the thud (rms 0.14 to 0.16 against 0.09 to 0.11). **Nobody has heard them.** I cannot listen; this needs the owner's ears.

## Fri 13 Nov 13:25 — Claude: correction: the deck lean was not working, nor was the camera shake; both are now

- Measuring the captain on the banked deck (`-DCSteer=1 -DCCrewLog -DCCrewLogFrom=114 -DCRunThrough`), the camera's roll read 0.00 while the ship lay over 10 to 17 degrees. The lean I reported at 11:50 was not reaching the picture: the camera takes its direction from the controller, which overwrites any rotation set on the component. The tilted horizon in that screenshot was the trough's sea, not the camera. The same line carried the camera shake from hull strikes and thunder, so **that had never shown either**, since it was written.
- Both now go in as the camera's additive offset. Measured after: roll -5.4, -6.8, -8.5 degrees at the eye as she banks (half the deck's; `-DCDeckLean=`). The shake from a strike has still not been seen by anyone: it wants a hand at the controls, or a capture during a strike.
- The captain keeps his footing on the banked deck: his place on the planking moved 8 cm in two seconds with the ship rolled 17 degrees and heaving 1.2 m.
- Not in any packaged build yet.

## Fri 13 Nov 13:10 — Claude: third dated build; more fog to either hand

- **Build:** `Builds/2026-11-13_1300_734c009_dusk-city` (dusk city, lightning on cue, grade, grey front interior, bow spray). Runs: 140 fps open sea, 142 in the city at 720p. Whole voyage in the editor build at 3x with the bot captain: every phase, ending reached, hull 80, no errors.
- **Banks seen from above** were separate puffs with clear water between. The walls along the channel are bigger and less eaten away, and a second, outer row of wide low fields lies beyond them (300 to 460 m off the course) so the city stands knee-deep in fog wherever the eye goes. 137 to 152 fps at 1600x900. Picture in review set 38. Not in the build above.
- Open sea checked again at 38 and 50 s from the deck: the front is a dark tower dead ahead with lit edges, about 35 degrees up at ten seconds out. Good, but it does not fill the sky like `storm-front-portal.jpg`. Left as it is.

## Fri 13 Nov 12:58 — Claude: spray over the bow

- When the stem drives down into a sea faster than 70 cm/s, a cloud of spray is thrown up at the bow, carried aft over the forecastle and gone in two and a half seconds (`DCWeatherFX::TickBowSpray`). It is a small volume of real mist in the same material as the fog banks, so it is lit by the sky, the lightning and the lanterns. About 17 a voyage, heavier in the city's sea than on the open sea. `-DCBowSpray=` sets the plunge that throws it, `-DCNoBowSpray` turns it off, `-DCSprayLog` writes each one.
- Honest reading of the captures (sequence in review set 38): from outside it is a soft pale cloud at the bow, more sea-smoke than hard white spray; from the helm, 35 m back in the fog, it is hard to pick out. It is not the sheet of white water of the owner's pictures. That needs particles or a sprite sheet, which the project has none of.

## Fri 13 Nov 12:49 — Claude: the drowned city is a storm at dusk

- Set against `fog-city-reveal.jpg` and `fog-to-maelstrom-portal.jpg`, the city was still far too light: a grey day. Both pictures are a storm at dusk in which the fog is the brightest thing and the lanterns the only warmth. The city's sun is now 0.8 (was 3) and its sky light 0.5 (was 1: the 0.55 it was meant to have had been swallowed by a comment on the same line since the day it was written). The fog banks keep their own glow, so they now stand out pale against dark stone and a near-black sea, and each lightning flash lifts the whole scene. This is the closest the game has been to the owner's pictures. Review set 38 (nine frames, 70 to 236 s; two caught mid-flash).
- New test switch `-DCEnv="SunIntensity=0.8;Exposure=-1"` fixes any weather value for a run; it is how this was found without a build per try.
- 117 to 145 fps at 1600x900 through the city.
- Still missing against those pictures: a black sculpted cloud ceiling (ours is dim and veined, not sculpted), the faces, heavy breaking seas with spray, crew who are people.

## Fri 13 Nov 12:26 — Claude: a grade; flash toned down again; faces still parked

- **Grade.** There was none. Now: a darkened frame edge, a trace of grain and fringe throughout; in the mist of the front and the city the colour is drained a little (saturation 0.86), shadows pushed toward blue, blacks a little deeper and bloom up (0.9 to 1.5) so the lanterns are the one warm thing and glow in the fog; the open sea stays warm with slightly more contrast. `-DCNoGrade` for comparison (picture in review set 37).
- Lightning in the fog: a second capture, inside a bank, was still too white. The sky pack adds its own bloom on a flash (2.0, now 0.5); flare and scattering are down again.
- Cloud faces: tried at ruin scale under the cloud base with the sea fog applied (heads 145 to 210 m, 300 m off the course). Nothing readable from the deck in four captures. Still off (`-DCFaces`). Not spending more on it today.

## Fri 13 Nov 12:12 — Claude: lightning on cue, lighting the fog without whiting the picture out

- **Lightning on the director's cue.** Besides the sky pack's own chance flashes, the director now calls a bolt down somewhere ahead of the ship every 3.5 to 8 seconds in the front and over the city (the pack's `Flash Lightning`). Each lights the banks from inside.
- **First look at an actual flash** (`-DCLightningAt=104 -DCRunThrough -DCTenths`): the whole view went white for a second. Three things were stacking: the flare in the banks, the same flare in the wisps round the ship, and the flash light's scattering in the air. Now the banks flare at about a third of the first value, the wisps take an eighth of that, and the light's scattering is 1.5 where it was 6. A flash now brightens the fog and the bank it is behind and dies away over a second. Review set 37 is the sequence in tenths of a second.
- `-DCLightningLog` writes every bolt and flash.

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
