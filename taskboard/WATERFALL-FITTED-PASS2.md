# Fitted waterfall pass 2

Owner: Codex waterfall worker, branch `codex/crater-fitted-waterfall`, isolated checkout `Crater-Workers/Fitted-Waterfall`. Owns waterfall runtime file, generator, importer if needed and this handoff. Root owns all builds, Unreal execution, imports and captures. No cloud, cliff or shared header edits.

First delivery exports the chosen actual lower core profile with `-DCExportWaterfallProfile=absolute/path.json` (or the bare flag for `Saved/WaterfallProfile.json`). It works without `-DCProceduralFalls`; `-DCFallFeed=8` chooses a feed. `-DCWaterfallProfileHeight=8000` selects lower height in centimetres. The export records centimetre frame origin/basis, source/foot, half-width, current velocity, lower core samples, a 25x25 receiving-water height field and a profile SHA-1. Runtime source remains opt-in.

Next: offline generator consumes the profile; root exports after building the current cliff revision. Prior foot-only placement failed because the fixed vertical module did not follow the real sloping core; see `WATERFALL-PROCEDURAL-PASS1.md`.
