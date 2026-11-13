# Maelstrom cinematic pass — 13 November

Baseline: `9df9ed1`. Branch: `codex/maelstrom-reveal`. This note covers the next visual revision; existing ownership boundaries remain unchanged.

**Status: ready for Claude's code review and integration from `codex/maelstrom-reveal`. The owner requested a commit/push handoff, with visual work resuming after usage resets. This revision is not merged into main or packaged.**

The current editor build and both material generators pass. Runtime shader compilation succeeds and eight live DRW cascades bind to the bowl material. The final normal-speed run completed at 09:32:56: six breaks, score 600, hull 100%, withdrawal, Ending and restored open sea. Evidence: local `Saved/MaelstromReview.log` and the committed [review frames](review/codex-maelstrom-cinematic-2026-11-13/README.md). Frame-rate readings varied during concurrent activity, so this is not a performance benchmark.

The latest frame has broken overflow sections, unequal drops, rock buttresses, live wave normals/foam and visible spray banks. It still falls short of the reference: the overall rim reads as a constructed bowl, foam includes conspicuous patches and thin contour-like trails, the hull shelf is too smooth, and god rays remain weak despite correctly enabled volumetric media. No cinematic-quality acceptance or packaged-build claim is made. These are the priorities for the next visual session.

## User feedback and diagnosis

The reveal still resembles a pool rather than the vast, violent crater in the reference. It needs an irregular silhouette, recognisable waves at several distances, convincing waterfall contact and light through spray. The submerged threat must preserve that scale instead of filling the view with a visible body.

The preceding shape forced every rim crest to the same elevation and every waterfall to the same 240 m drop. Its nearly continuous opaque white strip reinforced the uniform boundary. Short, similarly sized wave details made the surface busy without establishing large water masses. More small noise alone cannot correct those structural cues.

## Current shape changes

- The nominal 3.2 km basin remains 900 m deep. Shared angular rim functions now vary radius, crest elevation, overflow strength, drop and the join to the inner slope. The resulting drop ranges approximately 90–351 m. Basin, cascade, land and landmark placement use those local rim values.
- Overflow sections have unequal extent and gaps rather than a uniform white ring. Rock buttresses interrupt the coast. The inner power profile and outer cubic meet at the same height and tangent. Six domain-warped wave groups have distinct directions, patchy amplitudes and 34–211 m wavelengths; the final pass replaces an intermediate isotropic-noise surface that resembled terrain.
- A protected entry sector retains the existing route and water shape. New variation is zero within 0.35 radians of the entry centre and blends fully in by 0.85 radians. This preserves the corrected sightline into the abyss and the tested cannon area.
- The old Mist loop has been removed. The new Atmosphere actor supplies torn spray, local fog banks and three volumetric daylight openings. It selects the registered UDS sun, enables nonzero volumetric extinction and restores prior sun/fog settings on exit. The bowl and rim cast shadows; the stage sun is higher, and the reveal camera turns toward the abyss later to avoid the tunnel sidewall. The full body stays hidden.
- The runtime bridge binds the installed DRW ocean's live FFT derivative and accumulated-foam textures while its grid is hidden. Eight cascades were confirmed at runtime. It reads existing simulation settings, restores the material's fallback on exit, and changes no geometry or DRW settings. The CPU water/cannon queries remain unchanged.
- Basin vertex alpha carries the upstream overflow sector. Basin-only `OverflowFoam=1` connects spiral foam trails to the actual falls. Cascades use accelerating flow and stock temporal dithering to soften torn feet and sides. Portal/tunnel instances retain the fallback defaults.

## Integration notes for Claude

Review the branch diff from baseline `9df9ed1`; later main commits were deliberately left for your integration. Changes are confined to DCMaelstrom, the new DCMaelstromAtmosphere/WaterBridge files, their original material generators/assets, setup and handoff documents. No edits to your director, fog, treadmill, ruins builder, sky pack or plugin source. Do not replace your newer main with this older baseline.

Keep all five `/Game/DC/Maelstrom` materials with the new sources. The paid DRW textures/shader helper are referenced in place, not copied into Git. Review the dependency/fallback caveat below and in SETUP.md before cooking. No ignored Kaiju skin changes are needed for this pass. The existing scale handoff describes the preceding revision; this document supersedes its water/atmosphere details.

The runtime fallback handles a missing or unready ocean. It does not remove compile-time references already generated into a material: keep DRW installed for the live version. If the plugin is removed from a development checkout, regenerate a fallback-only material before compiling/cooking; a scalar set to zero cannot bypass a missing shader include. Existing packaging settings include both `/Game/DC` and `/DynamicRealWater`, but this revision has not been packaged or validated in a packaged build.

## Completed mathematical checks

Across **14,400 angular profiles**, the underlying outer slope remains monotone, with no cubic overshoot; join heights, tangents and the periodic seam match. Waves intentionally add smaller local crests to that underlying shape.

All **6,330 cannon-envelope samples**, including full helm deflection, remain within 0.08945 radians of the entry centre, entirely inside the unchanged sector. Maximum elevation is 9.9994°, maximum range 117.21 m, minimum joint clearance above water 5 m, and minimum sampled clear aiming-path gap 1.27 m. These are source/math checks, not proof of the new rendering or moving projectile impacts.

## Pending visual acceptance

Review full-size playable helm frames and a continuous passage after the material and Atmosphere changes land:

1. **Silhouette and depth:** unequal rim heights, bays and cascade lengths are readable; the abyss stays visible immediately after the breach. No continuous white band or oversized foreground object dominates the view.
2. **Water scale:** distinct near detail, mid-distance breaking waves and broad distant swells; no uniform ribs, tiled glitter or flat, featureless surface. Geometry, normals and foam must agree in motion.
3. **Cascade contact:** spray begins at actual waterfall feet, breaks into separated plumes and leaves depth visible. No floating haze belt, card edges or mist disconnected from the flow.
4. **Lighting:** the initial glare resolves promptly into readable water and architecture. God rays pass through cloud/spray with visible depth, without washing the basin into flat fog.
5. **Continuity:** confirm tunnel-to-ramp passage, cannon hits, tentacle withdrawal and open-sea restoration still work through normal playback.

The cinematic reference remains unmet until those frames and motion are judged. New-pass visual, continuous-runtime and packaging acceptance must be recorded separately after testing.
