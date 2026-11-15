# Combined main candidate — 15 November

The subsequent [human playtest feedback pass](PLAYTEST-FIXES-15NOV.md) builds on this integration. Its handoff records the replacement candidate and current validation; the evidence below describes the earlier 14:27 baseline.

User authorized consolidating and pushing caldera, Kraken, ACE/crew and individual cannons into one main for final testing. Integration branch: `codex/final-main-integration`.

Source inputs: main `194a239` (crew portraits, purchased cast, ACE safeguards, individual guns, early gameplay and deck fire); caldera `a0d8e6f`; boss `2fef3c5`. The old `codex/ace-comparison` branch is already represented by newer main integration commits and must not be merged over them.

The combined fight uses one hit for obstacle joints and mouth charges, two for slam joints. Slams warn for 12 seconds, allowing recognition, an existing three-second reload, a second shot and up to three seconds of projectile travel. Perfect boss performance remains 1,000 points. Persistent severing, hull/fire/rigging consequences and timed ending are retained.

Local release content is frozen in `Final-Integration/Content/DC/Maelstrom/GeneratedCliffPreview`: active mesh `SM_CliffSector_264605a85967`, matching runtime markers/surface, scan/weather textures and bank materials. Owned scan folders `vefkehbva` and `vfmjadufa` are copied. Obsolete preview meshes and rejected lower-bank experiments are excluded. Licensed ship/sky/character/Kraken content remains local, outside the public repository. Plugins have independent build outputs; ACE keeps separate CUDA contexts.

`Tools/verify_release_content.ps1` now requires the Kraken mesh and five runtime clips, boss materials and matching installed caldera marker/import data, in addition to the existing voyage and crew assets. Kraken animation/mesh directories are explicitly cooked; caldera runtime data is staged as UFS.

Verification so far: 4,957 CPU replay assertions pass, including individual-cannon timing budgets, frame-rate-independent obstacle dodging, attack exclusivity, score limits and passive damage. Both isolated and main editor builds passed. The normal-speed combined fight interrupted all six attacks, severed both designated limbs and finished with 8 effective boss damage, zero ship damage and 1,000 boss points. Real player/crew individual guns were used; review-only aiming enabled, voice disabled. The earlier 11:39 crew build does not contain this integration.

The first combined cook exposed missing `LocalScans/cliff_geology` and `LocalScans/nordic_face_tile` dependencies of the cliff material. That candidate (`1421`) was stopped and rejected. Both folders are now frozen in main and the integration checkout; preflight checks all eleven referenced textures. Packaging also rejects material compilation failures instead of accepting a default material. The second candidate (`1424`) exposed two legacy DRW render-target calls missing the newer wave-roughness argument. `Tools/patch_drw_legacy_calls.ps1` adds narrow compatibility overloads to the installed plugin, retaining a backup and leaving current ocean calls intact. The packaging helper applies this idempotently.

Combined runtime source is main `3368536`. Fresh candidate: `E:/Projects/DirectorsCut/Builds/2026-11-15_1427_combined-main/Windows/DirectorsCut.exe`. Build/cook/stage/archive and sidecar staging succeeded with no material compilation failures. Size before runtime logs: 609 files, 16,030,052,550 bytes (14.93 GiB). Source, asset and release-file SHA256 manifests are in the candidate folder.

Additional checks passed: all seven Unreal automation suites (51 command cases, 20 regenerated synthetic speech round trips, cliff/water/route checks); native individual-cannon reservation, reload and real-hit verification; passive boss run with both collisions, both slams and both fireballs applied once, 23.4 cumulative damage. Packaged GPU ACE smoke passed all three crew lines and its actual ship/sky/HUD image was inspected.

Full packaged normal-speed voyage passed on 15 Nov at 14:38 GMT: level restart at 20 seconds, renewed LLM/STT readiness, crew ACE throughout, all phase transitions, all six boss interruptions, two persistent severed limbs, zero boss-phase ship damage and clean exit after the 465-second ending capture. Automated captain issued gun/fire/repair orders but did not steer; final hull 41%, total score 2,300. Captures of fog, caldera and restored open-sea ending were inspected. Evidence: candidate `Windows/DirectorsCut/Saved/CombinedFullVoyage.log` and `Saved/SceneReview/CombinedFullVoyage`; native checks remain in `Final-Integration/Saved`.

Launch the combined local package with `E:/Projects/DirectorsCut/Play DirectorsCut.cmd` or the same-named launcher in main. `-Boss` starts at the caldera for a focused test. `Saved/Release/Current.json` selects the tested candidate. Both older top-level early-gameplay shortcuts now point to this launcher; their original contents are retained under main `Saved/Release/LauncherBackups/20261115`. Earlier builds remain preserved.

User clarified that the itch account is now **Onibox** and explicitly wants local playtesting and fixes before any itch upload. No itch publishing is authorized at this stage.

Final acceptance still requires human normal-speed play, live microphone/mix review and a cold launch on another machine. Upload destination and jam submission remain tracked separately in `SUBMISSION-TRACKER.md`.

Observed polish item: after the packaged level restart, Unreal emits repeated `RecursiveApplyAdjuster failed` audio-mix messages. Crew ACE playback and gameplay continue; this is not a clean audio-mix acceptance. Review restart audio levels and the weather pack's sound-mix lifecycle before final submission. No crash or ACE fallback occurred during the completed voyage.

There are no pending merges for the accepted caldera/boss/crew/ACE/cannon checkpoints above. Further fixes should start from current main. Do not replace it with an older feature branch or silently add new terrain experiments to the frozen candidate.
