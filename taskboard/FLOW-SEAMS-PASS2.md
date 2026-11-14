# Basin flow and seam pass 2

Owner: Codex flow worker, `codex/crater-flow-seams`, isolated `Crater-Workers/Flow-Seams`, based on `989e064`.

Claim: `DCMaelstrom.cpp` basin height/mesh functions only, `WaterEdgeRadius` only in `DCMaelstromTerrain.cpp` (coordinated with cliff worker), water material generator, and a standalone CPU geometry probe. No cliff, waterfall, sky, boss or ride-route changes. Root owns all Unreal and GPU checks.

Actual failure reference: `taskboard/review/crater-iterations-14nov/lower-descent-c_0420.png`. Investigating the hanging upper-left water boundary and the central star-shaped distortion before further surface detail changes.

## Diagnosis and bounded corrections

The shoreline used `SecondsAtAngle` as though it were periodic. At the inlet, that route coordinate wraps from approximately -8s to341s. `WaterEdgeRadius` therefore jumped from1728m to1070m; both the water mesh's outer radius and the generated cliff toe consumed this discontinuity. The mesh connected those unlike edges with a huge stretched triangle.

`WaterEdgeRadius` now returns smoothly to the entry radius on the unused backward side of the inlet, over0.55radians. The playable river and complete spiral keep their existing shoreline and full-steer allowance. Both the water and cliff use the same repaired query. Falls just behind the inlet at-2.77/-2.63radians must derive their contact from that query, as the existing shared geometry does. Two additional previously rejected deterministic breaker packets now fit the extended non-playable return apron.

The second problem was in material coordinates. `q = delta - flow * age` backtraced an18m/s rotating field by up to257m even immediately around the pole. Nearby inner vertices consequently sampled widely separated phases, drawing a star/pinch in both the normals and vertex displacement. The root's matched normal diagnostics with and without live FFT both retained the defect, excluding live FFT as its source.

The basin now backtraces angle and radius analytically. Inside one full advection-cycle travel distance (257m), a finite angular-rate/inward-flow core keeps the origin fixed and well conditioned. A smooth transition returns to the prescribed outer spiral field by514m. The straight entrance/tunnel retains its original Cartesian backtrace. Explicit safe normalization also avoids `normalize(0)` at the pole. No noise, wave-height, cliff, sky, route timing or boss changes were made.

## Reproducible CPU evidence

`Tools/probe_maelstrom_geometry.py` extracts the production basin/shore/route/breaker function bodies and the actual HLSL `FLOW` expression into a temporary standalone C++ harness. It reads the installed engine's Perlin implementation into `Saved/GeometryProbe` only; no engine or vendor implementation is committed. A small math/vector shim runs the functions without Unreal. This is source-level CPU evidence, not a substitute for engine/GPU compilation.

The probe visits all688,128 angular neighbour pairs of the448x1536 basin grid, uses the deterministic breaker population, checks both sides of the inlet, preserves at least34.99m of full-route clearance, and bounds the protected core's source-coordinate stretch across five advection phases. It also checks the fixed pole and unchanged tunnel mapping.

| Measurement | Baseline989e064 | Corrected |
| --- | ---: | ---: |
| Inlet two-sided horizontal separation at±0.00001rad |657.942m |0.035m (ordinary angular separation) |
| Inlet vertical separation at those samples |427.108m |0.000m |
| Longest angular edge in complete basin grid |785.431m |12.543m |
| Minimum full-steer shoreline clearance |35.000m |35.000m |
| Worst sampled UV stretch at1.2m radius |16,924.5× |1.222× |
| Worst sampled UV stretch at250m radius |80.989× |1.222× |

The existing outer current still has strong strain near the terminal route angle: the probe reports about20.4× angular stretch at1000m in the worst phase, versus20.1× before. This patch fixes the geometric seam and the protected core's singularity; it does not claim a globally integrated fluid transport solver or remove the broad hydraulic bank profile.

```powershell
& 'E:/Utilities/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe' Tools/probe_maelstrom_geometry.py
& 'E:/Utilities/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe' Tools/probe_maelstrom_geometry.py --baseline 989e064
```

The baseline exits 1, catching the original defects; the corrected source exits 0. Python AST and `git diff --check` pass.

## Integration and visual acceptance still required

Rebuild the source target and regenerate `Tools/make_maelstrom_materials.py`. Inspect matched300/340/420 beauty and normal views, with particular attention to the former upper-left hanging boundary and the core at420. Confirm inlet/exit/backward seek, waterfall contact placement behind the inlet, and that the new core transition does not produce a visible band. CPU tests cannot establish those visual results. No Unreal process, material generation or GPU capture was run in this worker.
