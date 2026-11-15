# DirectorsCut taskboard

| Task | Status | Owner | Done when |
| --- | --- | --- | --- |
| Crew names, portrait HUD and speech vocabulary | Integrated; editor, speech, HUD and packaged GPU checks passed | Codex submission chat; codex/crew-ui | [Handoff and accepted 11:39 build](CREW-UI.md). Rex/Duncan/Ruby portraits and live duty icons, mirrored port cannon, Whisper hints including fire at will. 51 text cases, 20 synthesized speech cases and 3 packaged GPU ACE lines passed. Includes the current helm pivot fix; live microphone review remains. |
| Early-game encounters, fire response, steering and bow spray | Autonomous implementation and gameplay review in progress | Codex weather/gameplay chat; codex/early-gameplay | Owns early-phase DCTreadmill, DCCrew helm/fire response, DCPlayer interactions, DCTroubles, DCWake and new encounter/effect helpers. Preserve fog/fire/cannon baseline; no maelstrom-owned files or shared live editor. [Plan and iterations](EARLY-GAMEPLAY-ITERATIONS.md). |
| Purchased pirate cast | Integrated in main; four outfits, hair, walking, camera and focused cook verified | Codex submission chat; codex/ace-comparison | [Cast setup and evidence](PIRATE-CAST.md). New DCPirateCast subsystem preserves existing DCCrew/DCPlayer gameplay. Captain=Sovereign Corsair, Rex=Dread Captain, Duncan=Tide Duelist, Ruby=Rogue Swashbuckler. Include in the next full release package. |
| NVIDIA ACE lip-sync, crew voices and command verification | Integrated in main; editor/command and packaged GPU checks passed | Codex submission chat; codex/ace-comparison | [Integration, controls and evidence](ACE-INTEGRATION.md). Three pirate faces, distinct British/Scottish voices, 51 text cases and 20 synthesized speech cases. Complete 11:39 crew/helm package passed a fresh three-crew GPU test and scene inspection; see [current handoff](CREW-UI.md). Earlier 08:25 voice/face evidence did not validate the missing world; that package is rejected. Shared CUDA/D3D context remains blocked; [crash report](ACE-GPU-CRASH-15NOV.md). Live microphone/mix and second-machine release checks remain. |
| Deck fire realism | Applied to main; normal-speed and close-up captures reviewed, extinguish cleanup verified | Codex weather chat; codex/fog-fire-polish | [Fire handoff and comparisons](DECK-FIRE-14NOV.md). Owns new DCDeckFireEffect.cpp/.h, Tools/make_deck_fire_materials.py and M_DeckFire/Smoke/Ember; DCTroubles.cpp visual creation/update only. Preserve fire damage, crew interaction, maelstrom and other gameplay work. |
| Jam submission tracking and release checklist | Requirements checked; final candidate, video, licences and upload outstanding | Codex submission chat; codex/submission-tracking | [Submission tracker](SUBMISSION-TRACKER.md). Owns submission documentation only; gameplay/editor ownership stays with existing agents. |
| Early-game cloud depth, fog transition and approach lighting | Grey city-bank pass applied and compared; forward glare and richer depth still outstanding | Codex in this chat; codex/fog-fire-polish, tested in isolated pending-weather copy and main | [City fog / exit handoff](CITY-FOG-DEPTH-14NOV.md), [Weather handoff](CODEX-EARLY-WEATHER-14NOV.md), [visual iteration log](STORM-SCULPT-ITERATIONS.md). Owns DCRideDirector.cpp/.h, DCFogFront.cpp/.h, M_UnifiedStorm and M_StormFrontVdb materials/generators, approach volume compositing in DefaultEngine.ini, DCFogBank.cpp and visibility/card sections of DCTreadmill.cpp/.h. Preserve maelstrom checkout work. |
| Maelstrom crater, tunnel exit and timed naval boss encounter | Prior pass merged; new owner design queued | Codex; Claude authorised to take the pass when current work runs out—claim files in STATUS | [New owner direction](MAELSTROM-TIMED-BOSS-DESIGN.md): larger fog-covered half-pipe, rollercoaster reveal, slow orbit, phased hazards, timed death and damage-based score. [Prior implementation](MAELSTROM-CINEMATIC-PASS.md) is a baseline, not completion of this design. |
| Initialize Git and Git LFS | Done | Codex | Baseline is on `origin/main` at `dcf3e85`. |
| Diagnose Unreal MCP startup | Done | Claude | Launch with `-ModelContextProtocolStartServer`; see STATUS.md. |
| Define the first playable slice | Done | Claude | See [PLAN.md](PLAN.md). |
| M1 greybox (see plan) | Done | Claude | C++ module builds, ride director runs all phases, typed order moves a crew member, packaged build runs. |
| M2 voice loop | Done except live test | Claude | Works in unattended tests and in the packaged build; live microphone and hands-on station play need the owner. |
| M3 all phases real | Done (first pass) | Claude | Fog, slide, hell, maelstrom, kraken, fires, rigging, ending all in; see STATUS.md. |
| M4 polish and balance | In progress | Claude | Now driven by the owner's visual references: maelstrom reveal, then storm front and Atlantis in the fog. Hell channel on hold and out of the default route. Review screenshots in `taskboard/review/`. |
| DLSS 4.5 plugin | Done | Claude | Active in the packaged build (log shows DLSS-SR=1 and feature creation). |
| Sonniss audio catalogue | Done (metadata review) | Codex | [28 candidates and implementation notes](AUDIO-SHORTLIST.md); audition still required. |
| Deck movement, station HUD and cannon gameplay | Ready for merge review | Codex | [Handoff](DECK-GUNNERY-UX.md) on `codex/deck-gunnery-ux`. |
| Individual cannons, exclusive crew guns and impact effects | Implemented and verified | Codex cannon chat | [Handoff](INDIVIDUAL-CANNONS.md), `codex/individual-cannons`. One player gun per side; independent reserved crew guns, reloads and single-ball shots. |

Claim a task here with your name and branch before editing. Use a separate Git worktree for parallel changes. Coordinate before editing the same Unreal map, Blueprint, or live editor session.

For a handoff, add a short Markdown note in this folder with the task, branch or commit, what changed, how it was verified, and the next step or blocker. Link the note from the task row and share the commit with the other agent. This board is in a public repo, so keep credentials and private logs out of it.

## Ask Codex from Claude Code

Open Claude Code at the repository root. In an already running session, use `/reload-plugins` and then `/codex:setup`. Use `/codex:review` for a read-only review or `/codex:rescue` to delegate a specific task. Give Codex the task and the files or branch to inspect. For edits, agree on file ownership first.

From a shell, a read-only question can also use:

```sh
codex -a never exec --sandbox read-only --ephemeral "Read taskboard/README.md and summarize the open tasks."
```

Plugin requests start or resume Codex jobs. Use the taskboard and commit references to pass context to a separate desktop chat.

## Unattended run (Thu 12 Nov)

Claude is building autonomously. Read [PLAN.md](PLAN.md) for the design, [STATUS.md](STATUS.md) for progress. To reach Claude, append to [INBOX.md](INBOX.md). Claude owns the live Unreal editor, `Source/` and `Content/DC/` until further notice.
