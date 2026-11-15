# Historical terrain benchmark: independent audit

15 November 2026. Read-only CPU audit using existing original-triangle helpers. This Saved-only process benchmark is deliberately separate from ParentA/B integration proposals. No Unreal import, game change or new audit framework.

**The complete parent now has a useful branching landform hierarchy. This is method evidence, not approved game geometry.** Its historical shoreline is incompatible with current contact heights, it has not converged, and close faceting remains conspicuous.

## Exact evidence and reproduction

Artifacts: `Flow-Seams/Saved/FastscapeNaturalBenchmark/UniformHistoricalA`. Parent candidate SHA256 `28c3a39604f1a3f043c618e9443816c475c2a2c33d365c774e3874543f838af4`; crop candidate `3f241a59e09caf599130956afc079f7f648bc449baa5d4479d7baaa2d7114c95`.

The actual Saved script is `Flow-Seams/Saved/FastscapeNaturalBenchmark/run_uniform_parent.py`, SHA256 `9bfda3617afc72be7a3e9daf67ac432391738e63afd1910aca294e99db1271af`. Its bundled `generator_source.py` matches bytewise and the stored metadata hash is exact. Reproduce from the original script location: its `ROOT=parents[2]` depends on that directory layout; the deeper bundled copy is a source record, not a portable entry point. The helper copy and GPL-3.0 text accompany the artifacts. Fastscapelib remains an external offline dependency.

Only XY, domain, coastal-band mask and frame are copied from ParentA. The source-template hash and every declared copied field match. Independent PCG64 replay reproduces the complete initial -150 m plus 0..1 m perturbation field exactly; all fixed nodes are -150 m. Dry-land uplift is uniformly .001 m/year, zero elsewhere. No old elevations or Gaussian forcing enter the model.

The physical frame is unchanged: 551 x 251 samples at 2 m, a 500 x 1100 m parent array and 500 x 1088 m valid land. Crop rows `[0,253)` retain the physical 500 x 492 m valid footprint. No horizontal rescaling or vertical exaggeration is present. The model produces 250.264 m parent relief and 220.242 m crop relief.

## Deliberate boundary separation

All 1,311 coastal-band cells are flat historical receiving-water base levels, rather than the 251 modern external outlets. No extra inland or side sinks were added. Rainfall accumulation is restricted to active dry land: 504,092 m², excluding the 5,244 m² receiving band from the total 509,336 m² valid domain.

The historical target changes actual modern contact heights by -82.44 to +19.56 m. It therefore does **not** preserve current coastline elevations, route contact, waterfalls, neighbouring seams or temple foundations. The same XY sampling footprint does not erase those incompatibilities. Added inland rows remain authored proposal data, with closed lateral boundaries and no lateral halo.

Recomputed SingleFlow/MST routing matches the saved drainage array exactly. Every valid node reaches the declared band; zero receivers enter invalid padding. Outlet area sums to exactly 504,092 m². Of the 1,311 band cells, 251 receive nonzero dry-land drainage. Largest five catchments have 446,968, 16,520, 8,932, 5,608 and 4,632 m² contributing area. The largest still dominates at 88.67%; connected routing is not proof of a balanced artistic layout.

## Process regime and geometry

This run changes multiple conditions together: historical datum/band, initial field, uplift distribution, incision coefficient, diffusion, duration and rainfall policy. It does not isolate the modern sloping coast as the sole cause of previous failures.

The run reaches its 500 x 500 year ceiling. **It has not converged by its declared criterion**, which requires two consecutive 50-step windows with maximum height-change rate below 5% of uplift. The final maximum is 1.272 times uplift and median .102 times uplift; last-window stream-power removal equals 81.2% of added uplift. The surviving high terrain is still evolving, despite visibly developed ravines. The 250 kyr duration is a model limit, not calibrated geological history or maturity certification.

Independent bookkeeping gives 126,023,000 m³ uplift, 48,560,587.86 m³ stream-power removal, -197,893.38 m³ fixed-reservoir exchange and 77,264,518.76 m³ net gain. The combined residual is below 5e-8 m³; per-node identity error below 1.9e-12 m. This is geometric height-volume accounting, not transported stream sediment conservation.

Parent and crop remain one clean open manifold each, with 253,078 and 104,078 triangles, no degenerate/duplicate faces, nonmanifold edges, inconsistent winding or unused vertices. These are original physical heightfield triangles, with no overhang claim.

## Actual visual judgement

I inspected the review worker's `UniformHistoricalAWhole/WHOLE PARENT 500x1100m oblique_fractured.png` and `UniformHistoricalACrop/wall_fractured.png` under `Saved/ImplicitCliffReview/FastscapeParent`.

The whole view now has connected branching ravines and unequal interlocking spurs. It is substantially more terrain-like than the smooth authored mounds in ParentA/B. The close wall still has conspicuous planar/grid-aligned facet and shading steps, sharp sawtooth edges and repeated dark cuts. This needs deliberate cliff extraction/detail work after selecting the parent landform; it is not a finished rock surface.

The result supports further terrain-method evaluation, not native promotion. Current coast/route and supported temple composition must be designed and independently checked against any adapted result. Detailed audit evidence remains in the worker's ignored `Saved/WeatheredAudit/fastscape_uniform_historical_a.json`.

## Portable CLI: exact replay verified

The author promoted this precise benchmark into `Tools/generate_fastscape_historical_terrain.py`, with explicit `--input`, `--dependencies` and `--output` paths. The original Saved layout is no longer required. Its single replay is retained at `Flow-Seams/Saved/FastscapeNaturalBenchmark/PortableReplay`; generator SHA256 `5da7027f8767fd71d759cfba12abc6dc8ff8865dd4cb485f8702d42807bd2de8`.

Independent comparison confirms **all 40 numeric arrays** across terrain and the four parent/crop baseline/candidate files are bitwise identical to UniformHistoricalA, including shape and dtype. Settings, checkpoint numeric values, final metrics and diffusion ledger are also exact. Only provenance/timing metadata differs. This does not revise the model, improve its art, establish convergence or alter its dimensions.

The output bundles the exact domain template, CLI, four project helper modules under their import names and external GPL attribution/license. Their bytes match the referenced sources. Running the archived CLI's `--help` from a different working directory succeeds and resolves its imports; no second model run was performed by the auditor. Explicit flags for current coastline, route, temple and seam preservation, runtime integration, visual acceptance and convergence all remain false. The limitations and measured geometric incompatibilities above still apply.

Detailed replay evidence: audit worker `Saved/WeatheredAudit/fastscape_historical_portable_replay.json`. The portable foundation is reproducible; it is not a native game-scene approval.
