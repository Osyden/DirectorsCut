# Inlet bed investigation — 15 November, 15:35

Review branch: `codex/maelstrom-saturday`, based on combined main `3368536`. **The deeper-bed trial is not accepted for integration.** Its byte-for-byte rollback below is historical. The later [bounded shoulder correction](CALDERA-INLET-TOE-15NOV.md) is now installed only in this review checkout; player verification and the16:17 unchanged overview rerun pass. Main and its packaged local playable were never changed.

## 16:15 follow-up: fog trial reverted, shoulder geometry corrected

Stronger early density in the existing impact volumes did not visibly conceal the pointed shoulder. Restored the original height/radial expressions and rebuilt both targets. The separate `Rounded` candidate lowers only the identified finite outer tips, preserves the wet corridor and upper banks, and regenerates source/support/shoreline data. Native player evidence removes the photographed patch. The external overview captured successfully but crashed during process shutdown, so full overview acceptance is pending. See the linked handoff for exact identities, validation and limitations; the old WetFoot candidate remains rejected.

## 15:44 correction: the photographed patch is distant shoreline

Exact viewport-deprojected rays now resolve the apparent contradiction. The gray points photographed beside the ship are **220-247m away on the lower inlet's exposed outer shoulder**, not the adjacent hull bed. Their screen placement was misread. They lie outside the previous near-hull sample rectangle and beyond the narrowed 160m river half-width. Both their triangles and ray intersections are identical in the accepted and WetFoot sources, so the deeper-bed trial could not change them.

| Pixel at 1600x900 | Terrain face | Camera distance | Beyond river edge | Above lake |
| --- | --- | --- | --- | --- |
| 400.5,780.5 | 999838 | 219.580m | 23.886m | 16.865m |
| 450.5,740.5 | 118173 | 246.591m | 26.052m | 6.028m |
| 600.5,780.5, control | Water first | Cataract at 15.636m | Within river | Water correctly occludes distant rock |

The two shoulder hits precede the lake water by 29.580m and 10.928m along their camera rays. This is ordinary geometric occlusion, not evidence of a Nanite or Single Layer Water depth defect. Source arrays confirm the same triangle coordinates in both candidates. The earlier CPU result remains valid for its near-hull footprint; it did not attribute these pixels.

`-DCCliffRayTime=15` logs the current viewport's actual deprojection rays and full arena transform around encounter 15. `Tools/probe_caldera_camera_rays.py` consumes that JSON, queries original source triangles and production flattened-water triangles, and reports exact face IDs, barycentrics, depths and previous sampling coverage. It also supports explicit camera input and separate candidate comparison. Native data: `Saved/CalderaCameraRays/native-285.json`; report: `Saved/CalderaCameraRays/Native285/report.json`; screenshot: `Saved/SceneReview/CalderaNativeRayProbe/shot_0279.png`. Encounter time is 15.0031433; viewport 0,0,1600,900. The CPU comparison took 2.34s. Synthetic intersection/backface/camera-axis and end-to-end checks also pass, separately from native evidence.

Both actual shoulder points lie inside existing impact-fog lobe 8 (42m radius), 7.82m and 23.66m from its centre. Their viewing rays traverse 36.43m and 63.00m of that volume before reaching rock. The scoped next visual test is stronger **early** impact spray at these existing contacts, preserving the accepted footprint and later density. The user explicitly requested dense fog obscuring this waterfall foot. No additional bed excavation or renderer work is justified by these pixels.

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
