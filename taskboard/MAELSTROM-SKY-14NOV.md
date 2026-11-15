# Maelstrom volumetric cloud refinement — 14 November

## Coordinated neutral-sun diagnostic — 15 November

Opt-in flag `-DCMaelSunNeutral` alone changes only the finale's UDS `Sun Light Color` property to linear white (1, 1, 1). It uses the existing property capture, UDS refresh and exit/backward-seek restoration, targeting the rendered UDS sun rather than the director's inactive original light. Without the flag the colour remains (1, 0.94, 0.84). Sun direction/intensity, exposure, cloud material/composition, shaft tint and earlier phases are unchanged. Entry logs `MAEL_SKY neutral UDS sun base colour selected` when selected. `-DCOriginalMaelstromSky` bypasses this stage override as before.

Source inspection and `git diff --check` only; no build, editor or GPU launch was performed, per coordination. The sky source matched the integration branch before this small change. The integration owner will cherry-pick the diagnostic and run fixed matched captures. This is an A/B control, not a validated replacement lighting preset.

### Remaining warmth: read-only runtime diagnostic

The matched `CliffG2Falls` / `CliffG2Neutral` images remain broadly ochre after the neutral-property test. This does not identify whether UDS applies another sun colour/temperature multiplier, whether atmospheric attenuation is dominant, or whether a captured skylight or post-process source contributes. Do not compensate the proven grey-green cliff albedo. The explicit director grade has neutral channel gains at this stage's `Mist=0`, but that does not exclude other post-process sources.

Add `-DCMaelLightingDiagnostics` alongside `-DCMaelSunNeutral` for the next coordinated capture. It logs three `MAEL_LIGHT` samples about two seconds apart per stage entry, after stage lighting updates: the active sun's actual linear RGB, enabled temperature/Kelvin and temperature RGB, energy before atmosphere, CPU ground-transmittance/energy estimates, registered visible skylights with source/cubemap/realtime state, and candidate post-process volumes/camera blends with colour override bits, LUT and custom blendables. It neither recaptures the skylight nor changes any rendering settings. The atmospheric estimate is at planet-top ground, not the exact camera/pixel; PP entries are source settings, not the final blended grade. Skylight metadata alone cannot prove the captured irradiance is stale.

Interpretation: a warm live sun RGB despite a white UDS property points to UDS modulation; enabled temperature with a low blue multiplier identifies an additional direct-light tint; neutral pre-atmosphere energy with strongly blue-attenuating transmission points to atmosphere; neutral direct-light estimates require examining skylight/PP sources next. The measured result and a bounded comparison preset follow below. Installed UE 5.8 headers/source were checked for the logging APIs; no build, editor or GPU run was performed for this diagnostic commit.

### Measured atmosphere: opt-in ground-balance art preset

The integration owner's `CliffH2Three.log` samples 2/3 show actual sun RGB (1,1,1), 6500 K with temperature RGB (1.044155,0.983268,1.035693), and settled ground transmission (0.694727,0.511457,0.268807). Estimated ground energy is (5.531197,3.834607,2.122813): substantial warmth is introduced by the atmosphere despite near-neutral incident light. The active skylight is realtime, white, intensity 1.3 with no cubemap; logged PP colour controls are neutral. This supports a direct-light art correction, not cliff-albedo compensation.

New flag `-DCMaelSunGroundBalance` applies a fixed partial inverse-transmission tint to the existing stage sun colour. Exponent 0.85, minimum channel 0.4, maximum channel 1, intensity gain capped at 2. It is an art-direction preset for the existing 18-degree sun, not physical calibration or a live feedback loop. With `-DCMaelSunNeutral`, tint is approximately (0.44615,0.57881,1), gain 1.79742 and actual UDS intensity 13.70534 instead of 7.625. Estimated ground RGB becomes (4.43559,3.98940,3.81559), retaining mild warmth and the same estimated direct-light luminance (4.07171). This estimate does not guarantee identical rendered brightness at every pixel, including clouds and skylight capture.

The intensity correction uses the unmodified current director weather and its existing `cue * 0.75 + 2` mapping. It updates the UDS property and active sun, never multiplies an already-corrected light, and leaves phase cues/exposure consumers unchanged. The extra UDS intensity property is captured only when enabled; exit/backward seek releases intensity to the current outside weather, while colour uses existing saved-property restoration. Defaults and `-DCOriginalMaelstromSky` remain unchanged. Cloud density/material/bank placement/opening, sun angle, atmospheric coefficients, skylight fill and warm shaft bloom tint are unchanged. Native cloud illumination may change colour along with the sun.

For the integration owner's matched `CliffH2Three` review, keep all camera/time/terrain flags identical. Baseline flags: `-DCMaelSunNeutral -DCMaelLightingDiagnostics`. Comparison flags: `-DCMaelSunNeutral -DCMaelSunGroundBalance -DCMaelLightingDiagnostics`. Check all three settled diagnostic samples for tint/intensity persistence, then compare the rocks, waterfall whites, cloud highlights and warm rays. Include exit/backward-seek/re-entry verification; release logs `MAEL_SKY ground-balance released to current weather lux=...`. Do not promote this opt-in preset to defaults before visual review.

Validation for this addition: inspected the runtime evidence, source lifecycle and current TickUDS mapping; checked bounded tint/gain and luminance arithmetic; `git diff --check` passes. No build, editor or GPU launch, per integration coordination. Compilation, UDS tick-order behaviour and visual/restoration checks remain with the integration owner. Only sky-owned source/header and this handoff change.

This pass follows fa6b9c6 and preserves the selected bank anchors, wind, sun opening, sun direction and review camera. The clouds remain an animated, ray-marched three-dimensional density field. No photographic skybox or cellular erosion texture is used.

## Shape and lighting changes

The original engine hybrid volume noise now distorts each bank locally before its ellipsoidal bodies are evaluated. A varying, turbulent condensation floor trims the hanging spherical undersides. A narrower density transition, limited fine contour displacement and stronger erosion give the silhouettes more definition without moving the bank centres. Density is 0.035, erosion 0.28 and opening 0.23. Multiple-scattering contribution/occlusion are 0.32/0.65. The additional turbulence costs three volume-texture samples per density evaluation.

The sun stays at source azimuth -45 degrees and elevation 18 degrees. Sky fill is 1.3, sun energy 7.5, exposure -0.15 and sun colour linear RGB (1, 0.94, 0.84). This creates cooler blue-grey interiors and warmer highlights. Cloud self-shadow tracing distance increases to 4 km. Native cloud ambient occlusion is enabled at 4 km extent, strength 1, resolution scale 1 and aperture scale 0.4. Skylight scattering into fog is reduced to 0.15; directional scattering is at least 5. All newly changed skylight fields are captured and restored on exit/re-entry.

Native cloud shadow maps still shadow the atmosphere, fog and surfaces at full strength. The finale's fog remains at least 4 km deep, extinction 0.55 and scattering distribution 0.15. Full-resolution clouds retain view/shadow sample scales 4/3 and 3 km sample distance.

A selective screen-space shaft pass supplements that physical lighting: bloom 0.4, threshold 1.3, maximum brightness 1.2, first radial-blur distance 0.2. The higher threshold selects bright openings rather than lifting every cloud edge. The blur-distance console variable restores on exit along with the existing cloud rendering overrides. The old spotlight shafts remain disabled. **The clouds are volumetric; the additional visible radial fan is screen-space bloom, not an independently ray-marched beam simulation.**

Shape tests that closed the sun opening were discarded. More expensive per-pixel atmosphere sampling did not materially improve this view and is not enabled by this change. Implementation context: [Epic's native volumetric cloud shadowing documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine).

## Reproduce

Worktree `E:/Projects/DirectorsCut/Maelstrom-Sky`, branch `codex/maelstrom-sky`. UE 5.8 is at `E:/Utilities/UE_5.8`. Build Development Editor and Game targets. The checked-in material is `/Game/DC/Maelstrom/Sky/M_MaelstromCumulus`; `Tools/make_maelstrom_sky.py` rebuilds it from `/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128` and checks shader compilation. Run its commandlet with `-AllowCommandletRendering -RenderOffscreen`, not `-nullrhi`.

```powershell
./Tools/review_maelstrom_sky.ps1 -EnvironmentOnly -Native -Clean -Wait -ReviewName contour-fixed-hero -Shots 300 -Width 1920 -Height 1080 -ExtraArgs '-DCShotCam=0,-3500,3500,10,-45'
./Tools/review_maelstrom_sky.ps1 -Clean -Continuous -Wait -ReviewName contour-final-game -Start 268 -Shots '269,280,300,340,380,420,449,451' -Width 1920 -Height 1080
./Tools/review_maelstrom_sky.ps1 -Native -Clean -Wait -ReviewName contour-fixed-seek -Shots '300,150,310,451' -Width 1920 -Height 1080
```

The discrete hero request at 300 settles at actual ride time 306. Camera yaw -45 exactly matches the user's chosen c8eca71 screenshot. Runtime look-development switches include `-DCMaelCloudDensity`, `-DCMaelCloudErosion`, `-DCMaelCloudOpening`, `-DCMaelSkyFill`, `-DCMaelSunEnergy`, `-DCMaelSkyExposure`, `-DCMaelRayBloom`, `-DCMaelRayThreshold`, `-DCMaelSunScatter`, and new `-DCMaelFogFill`. `-DCMaelRayBloom=0` isolates native lighting. A 6 km cloud sample distance is a cheaper option via `-DCMaelCloudSampleDistance=6`.

## Validation

Development Editor and Game targets pass. The regenerated material compiles successfully. The final native 1920 x 1080 hero uses runtime defaults, with no lighting overrides. Its sampled GPU time was 18.6 ms on the workstation; this is not a target-hardware benchmark.

The continuous 268-451 gameplay run completed with DLSS Super Resolution at 67%, frame generation off, and encounter actors enabled. The first backward-seek run completed and restored skylight scattering/AO, the 29 UDS properties, and fog distance/extinction/phase. It exposed a UE 5.8 constructor-priority restriction on the new shaft-distance CVar. That override now uses a named temporary-priority entry and removes it on exit, preserving the original value and priority. Both targets rebuilt successfully; the corrected native hero logs active 0.20 and restored 0.10. The corrected native 300 -> 150 -> 310 -> 451 seek run passed (settled times 306, 155, 315, 456). Both exits log skylight scattering 1.00 / cloud AO disabled, fog distance 70000 / extinction 2.5 / phase 0.20, all 29 UDS properties restored, and shaft blur 0.10 restored from active 0.20. No Maelstrom shader failure, fatal error, assertion, or rejected CVar override occurred in the corrected reviews. `git diff --check` passes.

The long gameplay run had poor late-frame samples (52.3 ms GPU at 420). Fresh matched late-descent runs gave 11.1/11.6 ms at 420/430 with cloud AO and 13.4/12.7 ms with AO disabled. Those short samples do not reproduce the long-run slowdown or establish a causal AO cost. Long-run performance still needs profiling; do not present the hero timing as a gameplay frame-rate guarantee. Existing non-Nanite VSM overflow and experimental editor-toolset Python startup warnings remain outside this sky change.

## Scope and remaining gaps

Only sky-owned source/materials/tools and this handoff/task row change. No water, route, boss, shared director, terrain or vendor asset edits. Water baseline in these captures remains 73300a6 (local cherry-pick 558d457). The water chat's new 836fdb9 lower shoreline/cliff basin, e09fae6 and 182f543 are not included. Review the combined lighting after integration. Water 836fdb9 exposes `GetShorelinePoint(angle)` for relocating contact mist; existing contact plumes need that follow-up and must not be treated as validated against the new shoreline.

This commit follows c8eca71 / 76ff2cb / e7dee7a / fa6b9c6; fa6b9c6 removes the rejected photographic path and cellular texture. Integrate the final sky-owned file state or the complete series carefully. No main merge, push or packaged/cooked build is performed.

Unretouched captures: [final sky at 306](review/maelstrom-volumetric-contours-14nov/sky-306.png), [DLSS helm at 300](review/maelstrom-volumetric-contours-14nov/helm-300.png), [deep descent at 420](review/maelstrom-volumetric-contours-14nov/descent-420.png), and [restored ending at 451](review/maelstrom-volumetric-contours-14nov/ending-451.png). The continuous gameplay captures precede the shaft-distance priority fix; the final sky and [native re-entry at 315](review/maelstrom-volumetric-contours-14nov/helm-reentry-315.png) include it. The selected first pass is `taskboard/review/maelstrom-sky-14nov/sky-after.png`; the immediately previous pass is `taskboard/review/maelstrom-volumetric-return-14nov/sky-306.png`.

Cloud bases and contours are less spherical, the cool/warm separation is stronger and the ray fan is more visible. Bright-edge grain and some softness remain. Beam separation is still broader than the references, especially around the bright central opening; this is a further volumetric iteration, not a claim of AAA parity. Deep-descent helm views expose little sky because the route looks into the crater.
