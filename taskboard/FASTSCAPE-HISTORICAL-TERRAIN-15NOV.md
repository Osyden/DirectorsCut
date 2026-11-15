# Portable historical terrain benchmark

Owner: Codex cliff worker, branch `codex/fastscape-parent-landform`. New tool: `Tools/generate_fastscape_historical_terrain.py`. It promotes the exact Saved-only uniform-uplift diagnostic to explicit input, dependency and output arguments. No numerical model changes, parameter variants, game imports or live scene changes accompany this refactor.

## Actual result and limits

At native 500 × 1,100 m / 2 m sampling, this historical benchmark develops connected branching valleys and unequal interlocking ridges. Root, worker and independent reviewer inspected its whole-parent and 500 m crop clay. It is useful primary-process evidence, stronger than the rejected smooth authored-parent domes. Angular/terraced channel edges and repeated steep planes remain. **It is not an accepted arena cliff or a converged geological model.**

The model uses a uniform historical receiving band at −150 m, uniform active-land uplift of 0.001 m/model-year, SingleFlow + MST, SPL `K=2e-4`, area exponent 0.4, slope exponent 1, and valid-edge diffusion 0.001 m²/year. Initial PCG64 seed1234 relief is 0–1 m. It runs 500 × 500 = 250,000 model years. Native output is −150 to +100.264 m, relief250.264 m; no scaling or cap. The final maximum elevation-change rate is 1.272 times uplift, so the declared convergence criterion fails. Last-window SPL removal is 81.2% of uplift; elapsed time does not imply maturity.

All1,311 flat historical wet-band nodes are flow baselevels. Rainfall is applied only to504,092 m² active dry land; total valid physical area is509,336 m². The largest outlet receives88.6679% of dry-land area. This is a dominant drainage basin, not proof of many equally developed catchments. Side/back boundaries are closed. Historical datum and uniform uplift both differ from the previous authored-parent models; this is **not** an isolated single-variable causal comparison.

The current whirlpool coast, ship route, outer seams and temple supports are explicitly **not preserved or approved**. This historical terrain is a possible precursor to a later crater-cut experiment, which must separately resolve those constraints. No detailed hard-rock fractures, overhangs, sediment PBR masks or native Nanite result are claimed.

## Exact replay and evidence

One replay after the CLI refactor is in `Flow-Seams/Saved/FastscapeNaturalBenchmark/PortableReplay`. All40 numeric arrays across `terrain.npz`, parent baseline/candidate and crop baseline/candidate match the original `UniformHistoricalA` **bitwise**, including dtype, height, drainage, frame, vertices and triangle indices. Containers differ in metadata/provenance/timing, as expected. The original diagnostic remains immutable.

Actual clay: `Fitted-Waterfall/Saved/ImplicitCliffReview/FastscapeParent/UniformHistoricalAWhole` and `UniformHistoricalACrop`. The latter uses the original 500 m cameras without modern mean water. Independent original audit: `Cliff-Contact/Saved/WeatheredAudit/fastscape_uniform_historical_a.json`. Numeric conservation, physical frame, fresh seed/forcing and routing checks pass independently of visual acceptance.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_fastscape_historical_terrain.py --input Saved/FastscapeParent/ParentA/parent_terrain.npz --dependencies Saved/FastscapeReference/python_packages --output Saved/FastscapeNaturalBenchmark/MyReplay
```

Run from the worker checkout, or use absolute arguments from any working directory. Input is a physical domain template; only XY/frame/valid-domain/coastal-band fields are read, not old heights or Gaussian forcing. The tool validates its exact551 × 251 / 2 m benchmark contract and checks Fastscapelib0.3.0 loads from the supplied isolated dependency directory. It never installs or downloads packages. Choose a fresh ignored Saved output directory.

Every output bundle includes the exact CLI source, imported project helpers under their module names, input template, full external GPL licence, attribution, manifest and convergence checkpoints. Thus the archived `generator_source.py` can also be invoked with explicit paths; it no longer depends on the original Saved folder depth. Fastscapelib remains external and is not linked or vendored into Unreal. See [licence/attribution notice](../Tools/ThirdParty/Fastscapelib-NOTICE.md).

Next work is an explicit, separately reviewed crater-cut/placement experiment using the natural parent geometry. Do not stretch this terrain around the rim, overwrite measured contacts, or add surface detail and call it finished.
