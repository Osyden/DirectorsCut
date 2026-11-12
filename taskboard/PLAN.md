# Director's Cut — jam plan

Entry for the 2026 Epic AI MegaJam. Theme: **Director's Cut** — the one ambitious feature you always wanted in a game but never could ship. Ours: **an AI crew you direct by voice.**

Deadline: **Mon 16 Nov 2026, 01:59 GMT** (Sun 15 Nov 8:59 PM US Eastern), from secondary write-ups of the rules. Aim to submit by Sun 15 Nov 18:00 GMT.

Award targets: main jam entry, plus the **Render Royalty** modifier and the NVIDIA prize (qualify with the DLSS 4.5 plugin on stock UE 5.8.4).

## Pitch

You captain a brigantine. It sails from open sea into a rolling supernatural fog and the voyage becomes a ride: ruins and specters in the mist, then a burning hell channel, then a kilometre-wide storm whirlpool with a kraken in it. You cannot do everything yourself. You shout orders — "hard to starboard", "port guns, fire", "put that fire out" — and your AI crew answers and does it.

Length: 8–10 minutes, one sitting, no fail state that ends the ride early (the ship takes damage, the score and the ending shot change).

## Pillars

1. **Direct the crew.** Voice in, spoken acknowledgement out, visible action on deck. This is the theme feature and must work in every build from the greybox onward.
2. **A ride, not a sim.** The ship stays at the world origin. The world moves, rotates and transforms around it on a timeline.
3. **Cinematic weather.** Fog, lightning, rain, fire light and audio carry the look. Fog hides sparse content.

## Architecture

- **Ship at origin.** `BP_Brigantine` (purchased asset) never translates. A rocking component adds roll, pitch and heave from noise scaled by the current sea state.
- **Ride director** (`ADCRideDirector`, C++). Owns ride time, the current phase and a list of timed beats. Everything else reads from it. One float, `RideTime`, drives the whole show, so any moment can be jumped to for testing (`dc.Ride.Seek <seconds>`).
- **World treadmill.** Scenery, hazards and enemies spawn ahead and move toward and past the ship at "ship speed". Helm input shifts the lane offset of the treadmill, not the ship.
- **Environment state.** One struct (fog density and colour, sky, light, rain, wind, sea colour and roughness, lightning rate) blended between per-phase presets by the director.
- **Sea.** Flat plane with a wave material in phases 1–2; a tube mesh with scrolling water for the slide transitions; a giant funnel mesh rotating around the ship for the whirlpool. No buoyancy simulation.
- **Stations.** Common interface: a station has a state, can be operated by the player or by a crew member, and accepts commands.
- **Crew.** Each crew member is a plain state machine (idle, move to station, operate, respond). Commands arrive as structured intents. The crew works with no model running.

## The ride

| # | Phase | Time | What happens | Stations in play |
|---|---|---|---|---|
| 0 | Open sea | 0:00–1:00 | Calm, sunset. Tutorial: give two orders. Fog bank rolls in. | Helm |
| 1 | The fog | 1:00–4:00 | Dark mist, statues and ruins looming, specters, ghost ships on both sides. | Helm, cannons |
| T1 | The slide | 4:00–4:30 | Sea folds into a tunnel, ship loses control, drop. | none (hold on) |
| 2 | Hell channel | 4:30–7:00 | Lava glow, burning rocks to dodge, demons flinging fire, deck fires. | Helm, cannons, fire buckets |
| T2 | The breach | 7:00–7:20 | Tunnel opens into enormous space. | none |
| 3 | The maelstrom | 7:20–10:00 | Kilometre-wide whirlpool, storm, kraken tentacles strike the ship. | Cannons, repairs, helm |
| E | Ending | 10:00 | Kraken sinks or the ship is thrown clear. Score card. | — |

## Stations (in build order)

1. **Cannons.** Fixed side camera, mouse aims, guns track the cursor and fire in an arc. Port and starboard batteries. Crew can man the other side.
2. **Helm.** Left/right lane shifting to dodge. Crew can take it and follows "port", "starboard", "steady".
3. **Fire.** Fires start on deck in phase 2. Carry a bucket from the water source to the fire. Crew can be sent.
4. **Rigging repair.** Loose sail or rope events; hold to repair. Crew can be sent.
5. **Boarders.** Cut unless everything above is finished.

## AI crew pipeline

```
mic -> speech-to-text -> intent parser -> crew command -> state machine -> action
                                      \-> local LLM -> short spoken line -> TTS -> audio
```

- **Language model reads orders when it is up** (loose phrasing works); the keyword grammar below is the instant path for short orders and the fallback.
- **Intent parser.** Keyword and grammar matching turns text into `{crew, station, action, side}`. Fast, deterministic, works offline. Typed commands and number keys use the same path, so a judge without a mic can still play.
- **LLM for personality and fuzzy orders.** Local llama.cpp server (a sidecar process the game starts), small model. It writes the crew's one-line reply and resolves orders the grammar missed into the same intent format. The game never waits on it.
- **Speech.** STT: whisper.cpp server (CPU build, base.en). TTS: Piper, run once per line. LLM: llama.cpp server (Vulkan build). The game starts and stops them itself.
- **Models on hand:** `E:\AI\Oobabooga\user_data\models\Qwen3.5-4B-Q5_K_M.gguf` (3.1 GB, default) and `gemma-4-12b-it-Q5_K_M.gguf` (8.4 GB).
- **Fallbacks, in order:** no mic -> typed/keys; no LLM -> canned acknowledgement lines; no TTS -> subtitles only.

## Rendering

- Stock UE 5.8.4, Lumen with hardware ray tracing, volumetric fog, DLSS 4.5 plugin (v8.7.x for 5.8) for upscaling and frame generation.
- Dev GPU is an RTX PRO 6000 (96 GB). Ship a "Medium" preset too; judges will not have that card.
- NvRTX branch is **out of scope** (source build, hours of compile, packaging risk).
- In game: `F5` cycles the DLSS mode (Quality, Balanced, Performance, DLAA); `F6` toggles DLSS Frame Generation (off by default).

## Technical decisions

- **C++ module `DirectorsCut`** for director, treadmill, stations, crew, intent parser and sidecar client. Blueprints only for asset wiring and tuning. Text code is reviewable, diffable and mergeable; Blueprints are not.
- Our content lives under `Content/DC/`. Do not edit files inside `Content/Brigantine/`; subclass or wrap them.
- Main map: `Content/DC/Maps/L_Ride`.
- **Never commit** purchased assets (`Content/Brigantine`), model files, or NVIDIA plugin binaries. The repo is public. They are in `.gitignore`; see `SETUP.md` once written.

## Cut order (first to go)

Boarders -> rigging repair -> LLM fuzzy-order resolution -> hell-phase demons (keep rocks and fire) -> phase 0 tutorial polish. Never cut: voice order -> crew acts -> spoken reply; cannons; kraken finale; a packaged build.

## Milestones

Updated Thu 12 Nov, midday. The build ran far ahead of the original dates.

| Milestone | State |
|---|---|
| **M1 greybox** | Done Thu 12 Nov morning. |
| **M2 voice loop** | Done in unattended tests and in the packaged build. Live microphone and hands-on station play still need the owner. |
| **M3 all phases real** | Done, first pass: fog, slide tunnel, hell channel, maelstrom, kraken arms and body, fires, rigging, title and score cards. |
| **M4 polish and balance** | In progress. Needs the owner's eyes and ears: sound mix, pacing, what feels bad to play. |
| **M5 submit** | Build size decision, 30 to 60 second gameplay video, itch.io page. See `SUBMISSION.md`. Target: submitted by Sun 15 Nov 18:00 GMT; hard deadline Mon 16 Nov 01:59 GMT. |

## Where things are in the code

All game code is in `Source/DirectorsCut/`. The map `Content/DC/Maps/L_Ride` is empty; `ADCGameMode` builds everything at start.

| File | What it owns |
|---|---|
| `DCRideDirector` | Ride time, phases, per-phase environment values, sea mesh and its material parameters, lightning, lanterns, exposure, shake amount |
| `DCTreadmill` | Obstacles, scenery and ghost ships moving past the ship; lane offset from the helm; helmsman's lane search |
| `DCHazards` | Builders for rocks, ruins, statues, ghost ships |
| `DCCrew` | Keyword order grammar, crew members and their station behaviour, the crew subsystem that routes orders and warnings |
| `DCVoice` | Microphone capture, speech-to-text, language model (order reading and crew lines), text-to-speech, sidecar processes |
| `DCPlayer` | Captain: walking, taking stations, gun aiming, typed orders, hold-E jobs, camera feel, F5/F6 |
| `DCGunnery` | Broadsides: muzzle positions, ballistic shots, hits |
| `DCKraken`, `DCKrakenHead` | The arms that reach the ship; the body in the centre of the maelstrom |
| `DCTroubles` | Deck fires and loose rigging |
| `DCWeatherFX`, `DCAudio` | Rain and embers; synthesised wind, sea, rain, rumble, thunder |
| `DCGame` | Game mode (spawns everything, test flags), HUD |

Materials are generated, not hand-made: `Tools/make_*_material*.py`, run with `Tools/py.sh`. Test and build commands are in `SETUP.md`.

## Who does what

- **Claude:** primary builder. Owns the live Unreal editor session, `Source/`, `Content/DC/`, the sidecars and packaging.
- **Codex:** low on credits. Relay between the owner and the board; small reviews on request. Do not edit `Source/` or Unreal assets without claiming on the board first.
- **Owner:** away at work Thu 12 Nov. Leave messages in `INBOX.md`.

Progress log: `STATUS.md`. Messages to Claude: `INBOX.md`.

## Risks

- Editor restart may not bring the Unreal MCP server back (open board task). Mitigation: verify before the first restart; C++ and command-line builds do not need MCP.
- Packaging with sidecars and a multi-GB model. Mitigation: package at M1 and again at every milestone.
- Rules not yet read in full (pre-made assets disclosure, build size, model files). Owner to confirm or Claude to read the Official Rules.
