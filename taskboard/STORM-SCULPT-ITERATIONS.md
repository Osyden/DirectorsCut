# Storm-front visual iteration — 14 November

## Acceptance criteria

The approaching storm must read as a single enormous cloud mass. Its sides descend toward the sea and its leading face slopes into depth. Cloud folds feed the opening rather than outlining a separate arch. The throat contains mist and internal light, with no clear blue sky visible through it. Retain sea contact, progressive approach reveal and the later city transition. Review distant, approach, entry and exit screenshots; do not infer visual success from compilation.

## Baseline critique

The previous construction is fundamentally four unrelated volumes: three sideways tubes forming a rectangular wall and a forward-facing tube forming the arch. Moving/scaling them cannot remove their different silhouettes and intersections. The source tube is hollow, so it exposes the bright clear sky behind it. Enlarging it magnifies its coarse detail. Previous passes improved the opening timing and water seam but did not resolve this construction problem. It does not meet the reference quality.

## Iteration 1 — one density field

Replace the visible assembly with one heterogeneous volume containing a sloping, noise-eroded cloud mass and a gradually opening, tapered cavity. Warp the cloud noise around that same cavity so the surrounding clouds and lip have a shared flow. Add depth-dependent mist inside the cavity, becoming optically thick toward its far end, with restrained internal lightning. Keep the old assembly only as an explicit diagnostic fallback. This is a larger structural correction; compare screenshots before deciding whether it is an improvement.

Results and follow-up changes will be appended after rendering. “AAA” is the visual target, not a label granted by a passing build.

## Iteration 1 critique / iteration 2 changes

Rendered from the deck at 40, 49 and 55 seconds. Geometry is continuous and the distant sky no longer shines through, but the surface is a smooth muddy bank; the funnel is too hard to read within it. Reject as final art. Replace the poorly ranged stock-noise sampling with deterministic three-dimensional noise, strengthen the surface displacement and internal erosion, cool the scattering colour, and add restrained blue-grey fill deep inside the fog. Keep the unified shape and underwater overlap.

## Iteration 2 critique / iteration 3 changes

Colour improved, but the soft bank still fails the sculpted-cloud test. Increase dense-cloud extinction to expose surface relief rather than integrating through hundreds of metres of weak cloud. Reduce interior extinction while retaining an opaque depth buildup, and brighten the internal fog pocket to distinguish it from the exterior. Set resolution through the component setter after assigning the material and verify it at runtime; use full-resolution lighting-cache samples and smaller shadow steps on this one volume. Keep all renderer quality changes scoped to the front.

## Iteration 3 critique / iteration 4 changes

Runtime resolution is verified as 192×384×128. The third pass has better shadow relief and a mist-filled opening, but its puffs are far too large and smooth. Reject that scale of detail. Reduce the main billow wavelength from 220 m to 65 m, sharpen the leading density ramp, and add broad spiral relief across the surrounding face, not just around the cavity boundary. Increase the face slope so the upper mass recedes more visibly from the sea-level base.

## Iteration 4 critique / iteration 5 changes

The fourth pass is continuous and has no clear-sky hole, but reads as a finely textured bank. The outside silhouette is too uniform and the spiral relief is too shallow to read from the deck. Increase the curved relief from 140 to 400 m, give the shoulders a steeper asymmetric descent, and raise the central canopy within taller bounds. Widen the mouth to 540 m radius before tapering it into the mist. Use intermediate-scale billows rather than either the previous giant puffs or the fourth pass's fine grain.

## Iteration 5 critique / iteration 6 changes

The larger folds are readable, but their smooth surfaces resemble fabric rather than turbulent cloud. Keep the overall sweep and silhouette. Displace and distort those folds more strongly with the engine's cloud noise, sampling at the correct spatial scale (its tile contains about eight cells, unlike the procedural one-cell noise). Increase volume resolution to 256×512×256 while keeping world bounds unchanged, so the finer breakup survives baking. Check cost as well as appearance.

## Iteration 6 critique / iteration 7 changes

The stock texture produces grain but still leaves overly smooth, regular folds. Revert to predictable procedural noise and give each scale its own displacement amplitude. Distort the broad curls with the largest noise scale. Also extend the volume forward: the strongest curls can exceed the previous -160 m bound, flattening the leading edge. Keep a submerged lower bound. The sixth pass logged roughly 9 ms GPU at the 55-second 1280×720 capture on this machine; that is a diagnostic snapshot, not a representative performance benchmark.

## Iteration 7 critique / iteration 8 changes

The geometry is now connected and strongly curled, but the surfaces remain too solid and smooth. Use cellular noise for rounded billows at two scales, retain broad value noise for irregularity, and reduce dense-cloud extinction from 0.9 to 0.14 so the first occupied voxel does not dominate the surface. Give the interior mist stronger density variation; its uniform blue patch can visually suggest sky even though it is opaque fog.

## Iteration 8 critique / iteration 9 changes

Reject the cellular version: it loses the sculpted definition and costs substantially more (23 ms GPU at the approach capture). The existing animated VDB has better turbulent detail. Test it as a density-detail source sampled through curling coordinates inside the single shaped volume, rather than as separately transformed wall and arch actors. The slope, asymmetric silhouette, progressive opening and mist remain defined by the unified field.

## Iteration 9 correction

The first VDB-detail render failed: Unreal's volume component replaces the component bounds and transform with the imported texture frame on every tick. The runtime resolution log at spawn did not catch that later replacement. Disable that automatic component tick and explicitly stream the detail frame from ride time, preserving the storm's independent bounds. This also makes detail animation repeatable on seeks and restarts. Re-render before making an art judgement.

## Iteration 10 critique / iteration 11 changes

With the frame transform corrected, the source detail is visibly stretched into long streaks and leaves large holes. Reject this mapping. Restore the cellular field, reduce its billow scale to 65 m and restore higher extinction. This tests whether the previous cellular pass failed because it was too thin, while keeping the silhouette, opening and interior closure fixed.

## Iteration 11 critique / iteration 12 changes

The denser cellular version retains useful small billows and connected sweeping folds. It is more convincing than the stretched VDB or smooth procedural versions. The triangular skyline and bright surrounding blue sky still isolate the storm as an object. Broaden and soften the asymmetric skyline, and gradually build ordinary high sky coverage over 32–58 seconds; keep the separate radial storm withheld until the existing handoff. Check that this does not bring back the pale waterline skirt. The interior capture is properly fog-filled, but the light is too uniform and bright at 69 seconds; inspect again under the revised overcast.

## Iteration 12 critique / iteration 13 changes

Overcast connects the front to the sky, but the existing low overcast brightness crushes deck detail and the cool scattering tint becomes strongly blue under this lighting. Neutralize cloud albedo, increase approach overcast fill and blend it back to the existing setting over 58–66 seconds. Reduce the steady internal glow and desaturate it; retain the pulse. Reduce this volume's ray step modestly to improve edge sampling. The sea contact remains intact.

## Iteration 13 critique / iteration 14 changes

The interior now reads as grey mist rather than a luminous blue patch. The exterior's broad lightning light can still bleach the whole front and its water reflection. Reduce that light to 0.65 during approach, restoring the previous 4.0 over 76–82 seconds; preserve the internal shader pulse. Remove rejected texture-streaming experiments and unused material dependencies. Align the city visibility gate with the mist's 1750 m far boundary. Run continuously from launch through the city, including multiple frames around lightning, to assess the final candidate in sequence.

## Iterations 14–15 critique / iteration 16 changes

The continuous run exposed a lighting failure hidden by flash-lit stills: between lightning strikes, the overcast front loses its fold detail in shadow. Entry and city handoff remain coherent. Enabling heterogeneous indirect lighting and changing stochastic filtering did not fix this in a diagnostic run; neither override is retained. Add a restrained cool fill from aft/port during approach, fading up over 34–50 seconds and away over 58–68 seconds. It should model the front and preserve deck visibility while leaving the later city's lighting unchanged. Compare both flash and non-flash frames.

## Iteration 16 critique / iteration 17 changes

The fill restores deck visibility, but its reduced volumetric scattering still leaves the cloud face too dark. Use normal scattering strength on this fill while keeping its intensity and time envelope unchanged. This is an adjustment to illumination of the existing field, not another shape change.

## Iteration 17 critique / iteration 18 changes

The ordinary, non-flash frame now shows cloud relief and deck detail. Broad weather-pack lightning still washes out the full face and water reflection even after reducing its configured intensity. During the front, use the material's internal pulses instead of the pack's global flashes and visible bolts; resume normal weather lightning at the city transition. Verify this candidate in the main checkout with a continuous run, including the opening and later city.

## Iteration 18 critique / iteration 19 changes

Main compiled and completed the continuous 0–100 second run. The close approach, opaque interior and city transition hold up, but the 34-second frame reveals another timing issue: large spiral relief advertises the portal even while the cavity radius is still closed. Ramp the broad curl relief and internal glow with the same opening progress as the cavity. Distant cloud should precede the recognisable funnel. Re-render the early, approach and interior views after this correction.

## Iteration 19 critique / iteration 20 changes

The second continuous main run confirms the transitions, but scaling relief alone does not sufficiently hide the distant spiral: the density-noise coordinates are still fully twisted before opening. Scale that twist with opening too. Explicitly blend the cavity mask from completely solid during the first 12% of opening progress, removing its residual soft centre at zero radius. Check the distant closed cloud, emerging opening, close approach and interior after this final timing correction.

## Final verification and remaining art gap

The final material and main C++ build were checked in normal continuous playback from 0 to 100 seconds at 1280×720. The distant front reads as a closed cloud bank at 34 seconds, then develops the broad curling opening during approach. The 55-second frame shows one connected shape with descending shoulders and a mist-filled throat. The interior is opaque grey fog at 69 seconds. Ruins emerge through the existing city haze at 80 and 100 seconds. The front quality override restores downsample 2 at 80 seconds, and the game exits normally. No fatal errors, assertions, Blueprint runtime errors or material-compilation failures were found; existing editor-toolset Python and UDS editor-utility initialization errors remain.

The seek-first diagnostic did not display the distant front on its first capture. Its cause was not established; the final evidence uses normal startup, which does display it. Logs and rejected candidates are in E:/projects/directorscut/storm-sculpt-review. The final log is main-final.log, and the source snapshots precede this revision.

Saved comparisons: [before, 55 s](review/storm-sculpt/before-55.png), [closed front, 34 s](review/storm-sculpt/closed-34.png), [emerging approach, 44 s](review/storm-sculpt/approach-44.png), [new approach, 55 s](review/storm-sculpt/approach-55.png), [inside, 69 s](review/storm-sculpt/inside-69.png), [city, 80 s](review/storm-sculpt/city-80.png), [city, 100 s](review/storm-sculpt/city-100.png).

This is a verified structural and lighting improvement, not a claim of AAA reference fidelity. Fine turbulence remains softer and grainier than the reference artwork, and the broad folds could use more irregular secondary sculpture. Final approach GPU snapshots ranged from roughly 11 to 23 ms on this machine at 1280×720; this is not a packaged or 4K performance benchmark. The next art improvement would be higher-quality, authored turbulence within the unified shape, followed by motion and target-hardware review. Do not return to overlapping wall and arch volumes to obtain that detail.
