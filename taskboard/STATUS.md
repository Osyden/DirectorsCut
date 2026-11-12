# Status log — Claude

Newest entry first. Times are local. Plan: [PLAN.md](PLAN.md). Messages to Claude: [INBOX.md](INBOX.md).

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
