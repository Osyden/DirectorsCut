# Early gameplay: main integration and second polish session

## Main integration

Merge `5014304` integrates the first session through `e445dd1` into main, including fog-card, cannon, encounter, fire-response, navigation and bow-spray changes and their evidence. Main's prior committed head was `14ffeb6`; it was already an ancestor of the gameplay branch, so no competing committed code required resolution.

The committed taskboard README was retained from main. This deliberately lets the shared checkout keep its existing uncommitted ownership entries. All seven pre-existing modified/untracked main files were backed up outside Git and verified byte-for-byte after integration. No stash, other-chat commit, shared content save or editor shutdown was used. Other chats' maelstrom, ACE and pirate-cast work remains separate.

Both Development targets compiled successfully in the private runtime, and the actual main checkout's Development editor module also compiled and linked. Private runtime Source/Config and project descriptor use the merged code. The integration checks passed 10 cannon checks, 7 navigation checks and 29 rag/fire checks. A complete normal-speed route and its selected screenshots are recorded with the final integration evidence below.

The merged snapshot completed the normal-speed 0–246s route without scripted captain orders. It ended at 91.2% hull, score 1000, no active fires and no loose rigging. No stone contacts were logged; the 60–240s navigation window had three strong helm reversals and no successive reversals within four seconds. Different random layouts prevent treating the hull result as a controlled improvement over the previous run. Reviewed 236s/239s city/exit captures and the confirmed rag completion frame. The small exit-waterline/light transition remains a polish target, not a newly fixed result.

All four runtime processes exited normally with no fatal/assertion/Blueprint-runtime/material-compilation failure matches. Existing plugin and unavailable local-voice-service startup warnings remain. Source/Config file sets and SHA256 hashes, plus the project descriptor, matched the merged snapshot. These checks validate editor-runtime gameplay and both compiled targets; packaged validation is the next task.

Evidence: [cannons](review/early-gameplay/main-merge-15nov/cannons-results.txt), [navigation](review/early-gameplay/main-merge-15nov/navigation-results.txt), [rag](review/early-gameplay/main-merge-15nov/rag-results.txt), [route events](review/early-gameplay/main-merge-15nov/route-events.txt), [metrics](review/early-gameplay/main-merge-15nov/route-metrics.json), [236s](review/early-gameplay/main-merge-15nov/route-0236.png), [239s](review/early-gameplay/main-merge-15nov/route-0239.png), [fire out](review/early-gameplay/main-merge-15nov/rag-complete.png).

## Second session, 15 November, 05:40–09:40 London / UTC

Continue on `codex/early-gameplay` in `E:/Projects/DirectorsCut/Early-Gameplay`, based on the main integration. The existing automation is active every 30 minutes with an absolute end at 09:40 UTC. At the deadline, finish a safe checkpoint, pause the scheduler and report saved/pushed state and playable locations. Do not extend the session automatically.

Owned scope remains the early-phase weather/gameplay files described in [the first session](EARLY-GAMEPLAY-ITERATIONS.md). The shared main taskboard already records this ownership. Check current ownership before touching any additional file. Preserve other chats' dirty files and running editors. In particular, pirate-cast assets and maelstrom files remain with their owners.

1. Cook, stage and run a real Windows candidate from the isolated project. Keep old packages. Verify three-swat interaction, encounters and bow particles in packaged play; editor-only success does not establish package success.
2. Improve physical feedback at confirmed rag contact using the existing fire/smoke assets. Preserve three contacts, score once, interrupted-swing cancellation and crew buckets. Compare contact/recovery views.
3. Improve bow foam breakup and spray readability at gameplay distance. Preserve per-side water contact, dry-bow suppression and phase cleanup. Reject bright blades, solid rims or new edge-on sheets.
4. Diagnose the narrow bright exit-waterline patch separately from the fixed elevated fog-card band. Review a continuous approach, preserve layered city fog and avoid taking over the separately owned slide/maelstrom.
5. Recheck creature groups, fireball flight and statue charge/beam readability from both cannon and walking-deck views. Keep fair firing windows and meaningful player fire response. Add audio only after a real audition; metadata inspection alone is insufficient.

The private project is `C:/Users/ejobse/Documents/ChatGPT/DirectorsCut/pending-weather/DirectorsCut.uproject`. Its Content junction points to main: never save assets through it. Source/Config are physical copies; match the exact tested commit before building. Do not replace them or their binaries during its playtest or cook. Only stop processes owned by this session. Build outputs and raw logs stay outside Git, with selected screenshots and concise evidence committed on the owned branch.

## Current handoff

Main integration validation passed. The first new task is packaged-build validation. `Tools/package_isolated.ps1` in the gameplay worktree packages into a new output folder, records source/config hashes, refuses a busy runtime or existing output folder, and copies the licensed local AI sidecars only into the local build. Never publish raw licensed assets or those sidecars in the public source repository.

Local standalone editor-runtime launcher: `E:/Projects/DirectorsCut/Play Early Gameplay.cmd`. This is not the forthcoming packaged candidate. No new package is accepted until it has actually run.
