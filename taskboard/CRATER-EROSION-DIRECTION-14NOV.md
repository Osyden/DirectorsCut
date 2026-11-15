# Crater landform reset, rim city and erosion diagnostic — 14 November

Owned branch: `codex/maelstrom-saturday`, integration project `E:/Projects/DirectorsCut/Maelstrom-Saturday`. Started at `0d49f2b`. The user's latest direction is a credible several-kilometre landscape and a realistic cliff generator, plus irregular columns/statues around the rim. Preserve the 3.2 km crater, 900 m basin depth, water boundary and ship route. Boss remains deferred; sky/cloud/light and early-game ownership remains separate.

## Decision: stop tuning the noisy radial wall

The existing wall is not reference quality. This pass tried irregular local fracture exposures and crease normals. Actual wide and close reviews still show a smooth brown wall; the close view adds horizontal triangular dark cuts. Those two candidate commits (`4bb5b0c`, `3339c57`) were reverted before the final checkpoint. Their geometry and diagnostic evidence remain in Git history. Do not mistake a passing geometry test for good geology.

The next terrain approach is an **offline, constrained landform plus erosion and real 3D cliff faces**:

1. Build a clean broad landform from the fixed crater shoreline, rim silhouette, river catchments and temple plateaus. Use large-scale ridges and hard/soft geological layers. Do not project the current folded wall and treat its artificial steps as mountains.
2. Run hydraulic erosion for connected drainage and sediment transport, and thermal erosion for loose debris. Use multiple physical feature scales, with explicit protected route/toe/plateau regions and real drainage boundaries. Erode after imposing the crater shape; distorting an already eroded mountain into a ring would distort its channels and scale.
3. Add genuinely three-dimensional joints, broken faces and overhangs to selected escarpments. Heightfields cannot represent these. Avoid a uniform course of blocks or the same cliff profile repeated around the ring.
4. Drive material blends from the same flow, sediment, slope, curvature and exposed-rock data. Use metre-scaled tileable stone detail, dry/wet roughness variation and restrained colour; broad lighting and silhouette must work without textures first. Installed Cliff01/03 textures were inspected and are mesh UV atlases, unsuitable as direct triplanar tiles.
5. Prove one approximately 500 m sector in plain clay, then with rock surfacing, from both the deck and close review camera. Only after visual acceptance, bake detailed static meshes for Nanite and connect the same authoritative surface to waterfall trajectories, land placement and exports. Keep any prototype out of the live surface provider until these agree.
6. Extend accepted sectors into an irregular continuous rim. Refit waterfall sheets, spray and impact foam against the final geometry. Keep distant details physically small; use silhouette, occlusion and atmospheric separation for distant scale. Coordinate lighting with the sky chat after the geometry proof.

Hydraulic/thermal erosion and material masks are established workflows, not a guarantee of good art: [SideFX HeightField Erode](https://www.sidefx.com/docs/houdini/nodes/sop/heightfield_erode.html). Nanite is the intended rendering path for baked detailed meshes, with performance measured in our project: [Epic overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine). The local engine is 5.8; consult its APIs for implementation.

## Actual erosion experiment and its rejection

`Tools/generate_eroded_cliff_sector.py` is an original NumPy hydraulic/thermal erosion kernel. It preserves bedrock, bounds transport by available water, tracks sediment and checks mass, fixed cells and finite output. It exports flow, slope, curvature, erosion and deposition masks. This is an accelerated morphology tool with closed D8 boundaries, not a full fluid or fracture simulator. See `ERODED-CLIFF-KERNEL.md` for settings, tests and limitations.

`Tools/export_cliff_erosion_seed.py` exports a measured candidate sector from production C++ geometry. Source 8 at candidate `4bb5b0c` produced a 500 x 504 m domain at 2 m spacing (253 x 251 samples), 52,536 valid surface rays and 12,206 initially active erosion cells. Its raw 94 x 161 3D profiles, frame and exact provenance are preserved. Generated data stays under ignored `Saved/CliffErosion`.

The conversion deliberately selects the uppermost vertical intersection. 315 rays (0.60% of valid rays) have multiple surfaces; up to 28.81 m of vertical overhang span is lost. This is a measured limitation, not an acceptable final geometry conversion.

The first 256-step run cut about 13 m and deposited about 13 m locally: too strong. A bounded run was numerically safe, but 79% of cells cut by more than 1 m were within 4 m of the discarded-overhang band, although that band contains only 14.5% of active cells. The simulation mostly attacked projection defects; it did not establish convincing drainage gullies. CPU clay comparisons also retain the broad slab silhouette. **Do not import this result as finished terrain.** The next seed must be a clean geological landform. A protected-overhang variant is diagnostic only.

The seed records candidate geometry, not the restored runtime default. To reproduce that exact seed, use the exporter against a checkout of `4bb5b0c` and its matching dependencies; `--geometry-ref` validates the sampled position functions instead of silently accepting a different source.

## Retained runtime changes

- Rim layout now contains 50 irregular groups: nine sanctuaries, 16 colonnades, five arches, eight broken-column courts and 12 statues. There are paired districts and quiet gaps. Placement checks whole bounds against the plateau and samples actual rendered triangles, including flattened pads; unsupported groups are skipped with a warning. Existing licensed assets are referenced, not copied. `DCRuins` and early-game generation are untouched.
- Finite bank wave packets add approximately -1.27 to +1.45 m of relief outside the protected 125 m route corridor and 35 m shoreline band. Physical wavelengths are about 27–38 m, with near-zero area-weighted mean. This is bounded geometry improvement, not turbulent-water visual sign-off; the broad bank and specular repetition remain weak.
- Detached waterfall paths remain opt-in with `-DCDetachedFalls` and source 8 by default. They share gravity-driven endpoints with the core sampler and keep first water contact fixed. With restored cliff geometry, two accepted reaches have about 3.43 m and 9.54 m peak gaps, flight times 1.06 and 0.99 seconds, and zero final contact drift. Compact receiving sheets are also experimental. Constant material flow speed does not yet match physical acceleration, and the effect still reads as ribbons in close views. Do not enable it globally.
- The close waterfall camera now works without VDB actors. Old attached VDB data is skipped for the detached source. New trajectory timing must be included before regenerating a corresponding volume cache. No new cache was baked this pass.

## Review evidence and validation

- Rejected fracture/normal geometry: `Saved/SceneReview/crater5-city-bank-wide` (300/340/420/451) and `crater5-detached-close` (300). The former has detached falls OFF; the latter explicitly enabled them. The latter proves the test ran but fails visual acceptance.
- Final supported-city/restored-cliff review: `Saved/SceneReview/crater5-supported-city-final` (300/420/451). Review files are actual offscreen engine captures, not generated concepts. Requested seek times include settling before capture; they are not frame-rate measurements.
- Final Editor and Game targets: `Saved/CraterPass5AcceptedEditor.log`, `CraterPass5AcceptedGame.log`. Five maelstrom engine tests pass in `CraterPass5AcceptedAutomation.log`: cliff continuity, rock/water contact, detached trajectory, first receiving-water contact, route/steering.
- Production CPU bank/contact probes passed. After reverting the rejected cliff candidate, `Saved/GeometryProbe/Detached5Final.txt` confirms both detached reaches and all nine water contacts again. Erosion kernel tests and actual CPU previews are separately documented by their tools.
- Existing non-Nanite VSM marking overflow and unrelated Toolset Python startup messages remain. No new performance or motion-quality claim. Offscreen work preserves keyboard focus. No main merge, packaged build, purchase or cloud/light ownership changes.

Final actual review confirms all 50 landmark groups were supported, with no skip warnings. Columns and statues now punctuate the rim; the broad brown cliff and smooth bank are still poor. The 451 capture restores open sea with the crater absent. Retained evidence is in `taskboard/review/crater-pass5-14nov`: `current-city.png`, `current-bank.png`, `rejected-cliff-and-detached-close.png`, `candidate-raw-clay.png` and `rejected-projected-erosion.png`. The last two are CPU geometry diagnostics, not gameplay screenshots or concept art.

Eight final erosion tests pass, including exclusion of nonphysical padding from rain/transport. Exact measured candidate settings and the protected-overhang variant are in [the actual-sector audit](EROSION-ACTUAL-SECTOR-AUDIT.md). `Tools/review_eroded_cliff_sector.py` produces matched CPU clay views, maps and optional physical-coordinate OBJ files; it does not import game assets. Protected-overhang erosion also fails visual acceptance and still contains an isolated deposition spike. No erosion output is integrated into runtime terrain.

Next acceptance gate: an independently convincing clay sector from a clean landform, then material/scale proof in-engine. More wall noise, enlarged rock atlases or stronger fog are not substitutes.
