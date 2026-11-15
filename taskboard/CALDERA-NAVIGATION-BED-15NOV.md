# Inlet bed investigation — 15 November, 15:35

Review branch: `codex/maelstrom-saturday`, based on combined main `3368536`. **The deeper-bed trial is not accepted for integration.** The original installed terrain, support data and shoreline fog have been restored byte-for-byte. Main and its packaged local playable were never changed.

## What was tested

`Tools/deepen_caldera_navigation_bed.py` creates an isolated underwater clearance envelope from immutable source `6eac5bba828f84ef2196f3a6598c23829c503722eb056f7ad59269bd6ab444e7`. It samples the current route, full radial steering range and banking velocities, and only lowers submerged vertices. The trial changes no XY coordinates, topology, UVs, masks or exposed bank/lake shoreline faces. Already submerged lake-plane intersections under the river are allowed to move explicitly; their source/support/fog data were regenerated rather than relabelled.

The useful trial is `Saved/CalderaNavigationBed/WetFoot/candidate.npz`, SHA256 `56e8b93a890b01a1502fd4c968b03e8942aa4db866d7f152c4b58d35d13081d4`. Maximum lowering is 23.24m. The earlier `Saved/CalderaNavigationBed/candidate.npz` is a rejected collar-constrained trial; do not install it.

`Tools/probe_caldera_navigation_clearance.py` independently reconstructs production water triangles and queries the actual source triangles after the arena transform. Its 3,036 sampled descent/arrival states pass static terrain-hit clearance: minimum 0.624m to the recorded hull AABB, 3.546m to water inside that footprint, and 1.812m to water outside it in the sampled flatten area. Missing terrain hits are reported separately, not certified as clearance. At encounter 15, the nominal new-bed water clearance is 9.23m inside and 5.92m outside the hull footprint.

This is **not** a full dynamic clearance pass: the conservative wave sensitivity bound fails in some arrival/steering states. Transient hull jolts, exact hull collision, selected Nanite triangles and perspective camera rays are not certified. Separate late-orbit diagnostics also expose a pre-existing mismatch between the 220m `HeightAt` corridor and the narrower lower river ribbon. Do not expand this visual patch into a route change without the boss/integration owner.

Probe evidence: `Saved/CalderaNavigationClearance/WetFoot/report.json` and `contacts.npz`. The report records probe hash `acdbf7c75b546452c74e9df3ea1527864cb03d0a277ad9e8652fd8e50de7f326`; the subsequently committed script only makes the candidate argument explicit and corrects the native-validation disclaimer. Its geometry/clearance algorithm is unchanged.

## Native result: the visible patch remains

The trial was exported as FBX `98fec7e481eba1f4c18768a673edd54d3e0c4bab780e71197cbc56cdc035d904`, imported as `/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector_98fec7e481eb`, audited against all 1,720,320 SourceModel triangles, and installed with matching support/surface/fog data. SourceModel position error was below 0.038cm. Native logs confirmed that exact asset was loaded. The updated 160-contact fog report stayed within 40.88m of its surveyed contact geometry.

Despite those checks, **the pointed gray patches beside the ship are visibly unchanged**. Do not describe this as an art improvement or a fix for the photographed defect. Actual ride 285 is encounter 15 in the current integrated timeline, not encounter 10. The probe's predicted component centres match native logging within 2.64cm at encounter14.9932, which rules out an obvious metre-scale arena-frame error.

The native isolation sequence is retained in `Saved/SceneReview/`:

| Review | Observation |
| --- | --- |
| `CalderaNavigationBedShip` | Trial mesh visible; patch persists at ride 285; open sea and backward seek restore. |
| `CalderaNavigationNormalDiagnostic` | Water diagnostic emission surrounds a gray textured patch. |
| `CalderaNavigationOpaqueDiagnostic` | `ShowFlag.StaticMeshes 0` removes the patch. |
| `CalderaNavigationFrameHidden` / `CalderaNavigationRootHidden` | Hiding the ship helper, then the ship root and its children, does not remove it. |
| `CalderaNavigationNaniteHidden` | `ShowFlag.NaniteMeshes 0` removes both cliff and patch. |
| `CalderaNavigationNaniteDetail` | `r.Nanite.MaxPixelsPerEdge 0.1` does not fix it. |
| `CalderaNavigationProxyHidden` | `r.Nanite.ProxyRenderMode 2` does not fix it; coarse fallback is not established as the cause. |
| `CalderaNavigationCliffHidden` | The new opt-in `-DCCliffHideMesh` hides only the installed cliff mesh and removes the patch while leaving the water and other actors. This attributes it to that rendered cliff component. |

The ship-root and static-mesh hides were diagnostics only. They are not installed fixes. No production material, fog or visibility has been altered to conceal the artifact. The only C++ change in this checkpoint is the opt-in cliff-isolation flag; default rendering remains unchanged.

The imported fallback has 8,528 triangles, but that number alone is not evidence that fallback rasterization is active. Bank scan bounds and screen directions rule them out as the near-stern patch. Water topology has no interior boundary edges; its static flattening introduces no XY triangle flips. Actual FFT displacement is still a possible source of water coverage differences: the lateral cap occurs before rotation onto the slope. `WaveHeight=0` does not disable all displacement, so it would be an invalid isolation test.

## Restored baseline and next useful check

The accepted installed FBX remains `264605a859674b1288cc1f8feb7c6a534bb2b46ae9b75997688ddc87d9448640`; the source remains `6eac5bba828f84ef2196f3a6598c23829c503722eb056f7ad59269bd6ab444e7`. The unchanged shallow bank haze and dense waterfall-foot fog remain the delivered visual state. The experimental new mesh is still present locally but is not selected by the installed runtime marker.

Trial files and its exact runtime bundle are preserved under `Saved/CalderaNavigationBed/WetFootNative/`. Rollback evidence is in `Saved/CalderaNavigationBedRollback/`. Do not copy the trial mesh/runtime into main, package it, or overwrite the accepted marker automatically.

The next bounded investigation should identify the actual rendered cliff triangle under a known camera ray, and compare it with the source query at that same world point. Alternatively isolate final wave displacement while retaining hull flattening. Do not keep lowering the bed or enlarge fog from an unproven screen-space attribution. Preserve the bank-fog scope and the user's release testing time.

Editor and Game builds pass with the diagnostic flag; both Python tools compile. No new package or release/performance certification is claimed. Existing experimental editor-toolset startup errors and the non-Nanite VSM warning remain.

Final native `CalderaBankFogRestored` overview verifies the restored installed source, visible shallow shoreline haze, dense inlet-foot fog, open-sea transition and backward seek with the diagnostic flag absent. All eight installed runtime files match the original rollback bundle byte-for-byte. This confirms restoration, not a new visual improvement.

To rerun the independent probe explicitly:

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 1 --python-exit-code 1 --python Tools/probe_caldera_navigation_clearance.py -- --candidate Saved/CalderaNavigationBed/WetFoot/candidate.npz --output Saved/CalderaNavigationClearance/WetFoot
```

It is a CPU-only audit; it does not install its candidate or certify native rendering.
