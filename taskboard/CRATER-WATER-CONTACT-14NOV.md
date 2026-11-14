# Crater waterfall contact and shading, 14 Nov

Continuation on `codex/maelstrom-saturday`, starting at `d04c743`. Three isolated workers handled core/shore contact, waterfall shading, and fitted-volume contact/export. The crater dimensions, descending route, supported temples, sky/cloud/light composition, early game and boss are outside this pass.

## Actual diagnosis and changes

The prior core was fitted to rendered cliff triangles but then pulled upward during its final six percent to reach a nominal foot. Removing that blend exposed a separate terrain defect: the low quadratic talus lay beneath the sea where the water mesh ended. Actual runtime measurements found source 8 entering sideways **6.61 m below** the mean water, and other sources 9.8–16.4 m below it. This was not an exporter-height error. We deliberately did not bake that profile.

Core paths now stop at their first **covered** receiving-water entry. Height extrapolated outside the basin does not count as water. Each branch checks 33 lateral tracks; the exporter resolves its nine tracks independently and uses the exact stable source angle. Surface impact and foam are separate from the unmodified core endpoint, so a submerged entry cannot silently turn into an upward curl. Impact sheets and droplets reject positions outside the receiving surface. The low-talus correction fits the actual rock to the water boundary, independent of the core offset, retaining the buried toe and all radial positions.

The waterfall material previously used a constant pale colour, roughness .75 and no ripple normal. It also used whole-width stationary dark bands, and a downstream-varying clock multiplier that compressed/reversed the pattern over time. The new material separates lit liquid and aerated patches, adds metre-scaled edges and small normal detail, and advects each branch at a constant speed. It has no emission. The first actual close render shows improved variation, but remains an attached stripe across oversized ledges; this is not waterfall visual acceptance.

The fitted VDB generator now requires a coverage grid and signed shoreline distance. It brackets each strand's first real water entry, synchronizes impact to that contact, terminates returning impact particles, and removes submerged density after reconstruction. It does not lift the core. Entry-depth statistics explicitly distinguish a surface hit from a submerged side entry. This remains an art-directed particle/volume model, not a liquid solver.

## Remaining visual problem and next implementation

The dominant close-up failure is still the waterfall bending over every giant shelf. A terrain-conforming offset necessarily preserves those bends. More opacity/noise tuning cannot create freefall or repair the geology.

Use feed 8 as the next bounded prototype:

1. Store an authoritative branch path with positions, metres, travel time, velocity, width and attached/freefall intervals. The core mesh and VDB exporter must consume the same data.
2. Detect real projecting lips, launch from the incoming downhill tangent, and integrate gravity in segments no longer than roughly one metre. Test the small local strip of actual cliff triangles and covered sea; take the earliest collision.
3. Replace the attached triangles between takeoff and landing. Preserve thin attached runs between falls. Check several lateral tracks so a wide sheet cannot intersect a rock that its centre misses.
4. Anchor landing spray and final surface foam at those actual contacts. Export the stored trajectories and invalidate the old fitted cache; do not independently reconstruct the former angle/V path.
5. Require visible air behind a detached reach, continuous takeoff/landing, no rock penetration, correct receiving-water contact, and matching export/mesh geometry. Inspect motion as well as the close and deck stills.

The cliffs themselves still have overly large, blocky shelves. Broad smooth hydraulic banks, harsh foam patches and non-Nanite shadow coverage also remain. The next geology change should be judged in a wide ship view and a close view; do not hide defects with additional fog.

## Validation and review

Editor and Game builds pass (`Saved/WaterfallShore3EditorBuild.log`, `WaterfallContact3GameBuild.log`). All four `DirectorsCut.Maelstrom` automation tests pass (`WaterfallContact3Tests.log`), including first-entry versus later upward-return cases. Material regeneration and actual GPU shader compilation pass; unrelated regenerated material assets were restored.

`Tools/probe_waterfall_contacts.py` extracts production functions into a temporary standalone C++ harness and samples the actual rendered cliff grid. It reports 9/9 valid rock/core contacts with less than 1 cm tolerance, 4.86–8.65 m core coverage margins and 34.7–100.1 cm of supporting rock at the water boundary. The worker's pre-correction baseline fails all nine. The live engine confirms central and lateral contacts within approximately 0.01 cm; these tiny residuals are numerical evidence, not a claim about fluid-simulation accuracy. The previous inlet/flow CPU probe also passes with unchanged 35 m steering clearance.

The corrected original cache uses profile `FE78F5572EA074C287421091143D308E36DEE82F`, source 8, a 230.768 m lower path and independent lateral contacts. Generated at 0.5 m voxels, 24 fps, 96 frames/four seconds: 132.43 MiB in 59 seconds. All 56,000 strands find covered water; all 192 stored frames contain no submerged density above the intentional invisible bounds anchors. Exact loop equality, bounds and VDB roundtrip checks pass. Native import passed: Falling 67x186x186, Impact 111x93x59, under `/Game/DC/Maelstrom/ProceduralWaterfalls/Feed08`. Reproduction data: `E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/WaterfallProfileSupported3.json` and `Saved/GeneratedWaterfallsSupported3` in that worker.

Actual close core captures before/after the shoreline correction are retained in `taskboard/review/crater-contact3-14nov/shading_before_shore.png` and `supported_shore_core.png`. The false toe curl is gone and the low shore joins the receiving water; the staircase silhouette is still plainly wrong. `wide_0300.png` and `descent_0420.png` preserve the combined ship views: thin distant falls, broad smooth banks and overly large bright foam/specular patches still fall well short of the reference. A successful contact regression is not visual sign-off.

The fitted combined view is `fitted_combined_03060.png`. Paired `fitted_core_hidden_0300.png` and `zero_density_core_hidden_0300.png` hide the surface core and impact mesh, with density .016/.004 versus zero. The visible wedge/mist at the foot persists at zero density and cannot be credited to our generated VDB. The generated volume is still too subtle to accept from these renders; native channel/pivot and optical-depth diagnostics should precede further density art tuning. Existing atmosphere remains active in both captures. Continuous-loop captures requested 305.8/306.0/306.2 actually landed at 305.8/306.2/306.4 due screenshot stalls; wide 300/340/420/451 captures landed at 306/345/425/456 after settling. These are review stills, not a motion/performance benchmark. Runtime logs confirm both procedural layers are cleared on exit.

Generated VDB/SVT data remain ignored and runtime use remains opt-in. No main merge or packaging. The non-Nanite VSM marking overflow still appears; no performance claim is made. Sky/cloud/lighting code was not changed. The wide review reached open sea and restored the sky controller.

The additional `waterfall-supported3-blocking-hidden` review explicitly confirmed `r.SparseVolumeTexture.Streaming.ForceBlockingRequests=1`; it still did not reveal a convincing falling volume. Do not label nonblocking streaming as the established cause. The native-source audit confirms the intended density A.R wiring and imported coordinate convention; UNorm8 loses approximately 5.1%/10.9% of Falling/Impact integrated density, insufficient by itself to explain an absent fall. Next isolate sampled density with a temporary diagnostic material and cliff occlusion removed, and inspect actual native frame transforms. Keep these diagnostics separate from production lighting/art changes. See the worker handoff for exact expected transforms and optical-depth estimates.
