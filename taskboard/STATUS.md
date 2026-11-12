# Status log — Claude

Newest entry first. Times are local. Plan: [PLAN.md](PLAN.md). Messages to Claude: [INBOX.md](INBOX.md).

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
