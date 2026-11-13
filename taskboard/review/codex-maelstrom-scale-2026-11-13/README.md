# Maelstrom scale correction — 13 November

These are actual UE 5.8 game captures from the isolated `codex/maelstrom-reveal` clone at 1600x900. They are not concept renders. [Critique and next art priorities](../../MAELSTROM-SCALE-CRITIQUE.md); [implementation and validation](../../MAELSTROM-HANDOFF.md).

- `01-reveal.png`: final visual pass, actual ride time about 284 s. Empty crater before the first tentacle peek at 292 s. The full boss body stays hidden throughout.
- `02-tentacles.png`: final visual pass, actual ride time about 355 s. No automatic firing in this seek review.
- `03-continuous-fight.png`: normal-speed continuous test at 345 s, before the final foam/lighting and tentacle surface corrections. The camera, basin profile and combat target positions are the same.
- `04-victory-exit.png`: that continuous test at 455 s, score 600 and hull 100%, with the ordinary sea and camera restored.

Seek filenames are not exact timestamps: the harness advances six seconds before its first shot and five seconds after later seeks. The continuous test does not repeatedly seek. Logs remain local under `Saved/MaelstromScaleFinal.log`, `Saved/MaelstromScaleUnopposed.log` and `Saved/MaelstromReview.log`.

The former [exposed-body view](../codex-maelstrom-2026-11-13/09-final-helm.png) is retained for comparison. The current pass improves the visible crater profile and removes that obstruction. It does not yet match the reference's water/spray complexity, irregular distant waterfall silhouette, composed light shafts, or central wheel-and-hands foreground. Packaging of this revision and hands-on control/audio review remain open.
