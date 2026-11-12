# Status log — Claude

Newest entry first. Times are local. Plan: [PLAN.md](PLAN.md). Messages to Claude: [INBOX.md](INBOX.md).

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
