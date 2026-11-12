# Director's Cut — jam plan

Entry for the 2026 Epic AI MegaJam. Theme: **Director's Cut** — the one ambitious feature you always wanted in a game but never could ship. Ours: **an AI crew you direct by voice.**

Deadline: roughly **Sun 15 Nov, late afternoon local** (owner said "80+ hours" at 09:30 on Thu 12 Nov). Exact time unconfirmed — treat Sun 15 Nov 12:00 as the hard internal deadline.

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

- **Intent parser first.** Keyword and grammar matching turns text into `{crew, station, action, side}`. Fast, deterministic, works offline. Typed commands and number keys use the same path, so a judge without a mic can still play.
- **LLM for personality and fuzzy orders.** Local llama.cpp server (a sidecar process the game starts), small model. It writes the crew's one-line reply and resolves orders the grammar missed into the same intent format. The game never waits on it.
- **Speech.** STT: whisper.cpp sidecar. TTS: a local engine (Piper or Kokoro) sidecar. All three are separate processes behind local HTTP, so the Unreal side is one small HTTP client.
- **Models on hand:** `E:\AI\Oobabooga\user_data\models\Qwen3.5-4B-Q5_K_M.gguf` (3.1 GB, default) and `gemma-4-12b-it-Q5_K_M.gguf` (8.4 GB).
- **Fallbacks, in order:** no mic -> typed/keys; no LLM -> canned acknowledgement lines; no TTS -> subtitles only.

## Rendering

- Stock UE 5.8.4, Lumen with hardware ray tracing, volumetric fog, DLSS 4.5 plugin (v8.7.x for 5.8) for upscaling and frame generation.
- Dev GPU is an RTX PRO 6000 (96 GB). Ship a "Medium" preset too; judges will not have that card.
- NvRTX branch is **out of scope** (source build, hours of compile, packaging risk).

## Technical decisions

- **C++ module `DirectorsCut`** for director, treadmill, stations, crew, intent parser and sidecar client. Blueprints only for asset wiring and tuning. Text code is reviewable, diffable and mergeable; Blueprints are not.
- Our content lives under `Content/DC/`. Do not edit files inside `Content/Brigantine/`; subclass or wrap them.
- Main map: `Content/DC/Maps/L_Ride`.
- **Never commit** purchased assets (`Content/Brigantine`), model files, or NVIDIA plugin binaries. The repo is public. They are in `.gitignore`; see `SETUP.md` once written.

## Cut order (first to go)

Boarders -> rigging repair -> LLM fuzzy-order resolution -> hell-phase demons (keep rocks and fire) -> phase 0 tutorial polish. Never cut: voice order -> crew acts -> spoken reply; cannons; kraken finale; a packaged build.

## Milestones

| When (local) | Milestone |
|---|---|
| Thu 12 Nov 20:00 | **M1 greybox:** C++ module builds; ship in `L_Ride`; director runs all phases with placeholder environment; typed order moves a crew member; first packaged build runs. |
| Fri 13 Nov 12:00 | **M2 voice loop:** mic -> order -> crew acts -> spoken reply. Cannons and helm playable by player and crew. DLSS on. |
| Sat 14 Nov 00:00 | **M3 all phases real:** fog, hell and maelstrom environments, kraken tentacles, fires. |
| Sat 14 Nov 18:00 | **M4 content lock.** Polish, audio, lighting, performance only. |
| Sun 15 Nov 06:00 | **M5 final package**, capture video, write submission answers (including NVIDIA questions). |

## Who does what

- **Claude:** primary builder. Owns the live Unreal editor session, `Source/`, `Content/DC/`, the sidecars and packaging.
- **Codex:** low on credits. Relay between the owner and the board; small reviews on request. Do not edit `Source/` or Unreal assets without claiming on the board first.
- **Owner:** away at work Thu 12 Nov. Leave messages in `INBOX.md`.

Progress log: `STATUS.md`. Messages to Claude: `INBOX.md`.

## Risks

- Editor restart may not bring the Unreal MCP server back (open board task). Mitigation: verify before the first restart; C++ and command-line builds do not need MCP.
- Packaging with sidecars and a multi-GB model. Mitigation: package at M1 and again at every milestone.
- Rules not yet read in full (pre-made assets disclosure, build size, model files). Owner to confirm or Claude to read the Official Rules.
