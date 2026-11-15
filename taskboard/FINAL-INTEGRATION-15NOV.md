# Combined main candidate — 15 November

User authorized consolidating and pushing caldera, Kraken, ACE/crew and individual cannons into one main for final testing. Integration branch: `codex/final-main-integration`.

Source inputs: main `194a239` (crew portraits, purchased cast, ACE safeguards, individual guns, early gameplay and deck fire); caldera `a0d8e6f`; boss `2fef3c5`. The old `codex/ace-comparison` branch is already represented by newer main integration commits and must not be merged over them.

The combined fight uses one hit for obstacle joints and mouth charges, two for slam joints. Slams warn for 12 seconds, allowing recognition, an existing three-second reload, a second shot and up to three seconds of projectile travel. Perfect boss performance remains 1,000 points. Persistent severing, hull/fire/rigging consequences and timed ending are retained.

Local release content is frozen in `Final-Integration/Content/DC/Maelstrom/GeneratedCliffPreview`: active mesh `SM_CliffSector_264605a85967`, matching runtime markers/surface, scan/weather textures and bank materials. Owned scan folders `vefkehbva` and `vfmjadufa` are copied. Obsolete preview meshes and rejected lower-bank experiments are excluded. Licensed ship/sky/character/Kraken content remains local, outside the public repository. Plugins have independent build outputs; ACE keeps separate CUDA contexts.

`Tools/verify_release_content.ps1` now requires the Kraken mesh and five runtime clips, boss materials and matching installed caldera marker/import data, in addition to the existing voyage and crew assets. Kraken animation/mesh directories are explicitly cooked; caldera runtime data is staged as UFS.

Verification so far: 4,957 CPU replay assertions pass, including individual-cannon timing budgets, frame-rate-independent obstacle dodging, attack exclusivity, score limits and passive damage. Native editor, combined package and packaged GPU validation are pending. The earlier 11:39 crew build does not contain this integration.

Final acceptance still requires human normal-speed play, live microphone/mix review and a cold launch on another machine. Upload destination and jam submission remain tracked separately in `SUBMISSION-TRACKER.md`.
