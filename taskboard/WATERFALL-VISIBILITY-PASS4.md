# Waterfall native visibility diagnostic, pass 4

Owner: Codex waterfall worker, `codex/crater-volume-visibility4`. Owns waterfall runtime, importer and this note. Root owns Unreal builds, imports and captures. The cliff worker owns geology; do not regenerate a fitted cache until its candidate is accepted and newly exported.

The supported-contact cache imports correctly but remained nearly invisible with the opaque core hidden, including a forced-blocking streaming capture. The previous channel/transform code audit and density measurements are in `WATERFALL-WATER-CONTACT-PASS3.md`. Streaming is not an established cause. This pass adds direct diagnostics instead of changing density constants.

Implementation commit: `ec419fc`.

- The importer adds a zero-default `DiagnosticEmission` scalar multiplying the actual sampled SVT Attributes A.R. Normal rendering remains scene-lit with zero emission. Setting environment variable `DC_WATERFALL_MATERIALS_ONLY=1` rebuilds materials around the existing native assets without reimporting VDB data; the source manifest still selects the correct `Feed08` folder.
- `-DCFallDensityDiagnostic` enables that sampled-density emission and disables extinction. `-DCFallDiagnosticEmission=<value>` changes its diagnostic amplitude, default 1. The runtime reports an explicit error if the material lacks the parameter, preventing a stale material from being mistaken for an empty cache.
- `-DCFallHideCliffs` hides only the procedural cliff mesh and its rock/shadow-rock instances. `-DCFallVolumesOnly` hides the existing opaque water core/impact mesh. Original visibility is saved once and restored on cleanup, while respecting the encounter's hidden state after exit/early seek. Neither option changes sky or cloud actors.
- `-DCFallFixedFrame=0` holds a deterministic cache frame. The normal four-second manual playback is unchanged otherwise.
- `DC_FALL_NATIVE` logs the imported frame transform, actor-relative transform and diagnostic parameters at creation. `DC_FALL_COMPONENT` logs the native component frame transform, world scale, bounds, resolution, pivot and visibility after four update ticks, allowing the component to obtain its native frame first.

Root should first refresh materials using the current Supported3 manifest, then capture the same review camera/time with sampled-density emission, hidden core and fixed frame 0. Compare cliff-visible and `-DCFallHideCliffs` variants. Keep `ForceBlockingRequests=1` during these diagnostic captures to remove requested-mip residency ambiguity.

Expected Supported3 native transforms: Falling translation `(-1650,-7450,-650)` cm, Impact translation `(-3850,-1050,-1700)` cm; both have identity rotation and scale `(50,50,50)` cm per voxel. Actor world scale should remain `(1,1,1)`, and centroid pivot false. The actor's relative origin/yaw should match profile `FE78F5572EA074C287421091143D308E36DEE82F`. The current source geometry must remain unchanged for that comparison; a new cliff profile invalidates the fitted cache.

Interpretation: emission visible only after hiding cliffs indicates occlusion/fit; emission visible in both but lit density absent points to extinction/lighting; no emission even without cliffs points to native sampling/material/frame/bounds and needs runtime evidence before changing cache physics. An emission-only debug capture is not a quality improvement or a shippable effect.

Checked locally: importer parses, Git diff has no whitespace errors, and diagnostic calls were checked against installed UE headers. Worker did not build or run Unreal. Root's compile, native material refresh, captured visibility and cleanup checks are pending.

## Confirmed coefficient unit mismatch

Further tracing found a missing physical-unit conversion, implemented in `304923f`. The installed UE 5.8 live-shading renderer composes native `FrameTransform` into its local-to-world matrix (`HeterogeneousVolumesLiveShadingPipeline.cpp:1327-1344`), then transforms camera rays and bounds into voxel-instance space (`.usf:691-729`). `HeterogeneousVolumesRayMarchingTypes.ush:146-188` derives and limits step length in that same local space. `HeterogeneousVolumesRayMarchingUtils.ush:347-353,571` integrates `exp(-Extinction * local StepSize)`; `SampleExtinction` returns the material coefficient unchanged. `LocalToWorldScale` is used for world positions/depth, not as an extinction multiplier. There is no compensating centimetre factor in this path.

The earlier pass-3 optical-depth estimates multiplied density by 50 cm per voxel and therefore described the intended physical coefficient, **not the old native material's actual opacity**. Before correction, expected unoccluded median native optical depth was approximately 0.0084 Falling / 0.00183 Impact at `.016/.004` (about 0.84% / 0.18% opacity), fifty times weaker in optical depth than intended. This is a confirmed coefficient mismatch, but it does not establish why the sampled-emission diagnostic below is absent. Native visibility must be proven separately.

The importer now declares `Density` as extinction per world centimetre and multiplies by a manifest-derived `VoxelSizeCm` before passing extinction to the native renderer. It applies the same conversion to diagnostic emission, which remains zero by default. Manifest size must be positive, finite and consistent with the generator configuration. Runtime verifies that all three native frame-scale axes are equal and match the material's voxel size; stale or anisotropic materials/assets are rejected with `DC_FALL_UNITS`. Scalar defaults are unchanged. The actor remains at unit world scale; non-unit actor scaling is outside this fitted-cache contract.

Re-run the materials-only importer after the correction. Emission diagnostic amplitude now has per-centimetre meaning too, so an existing amplitude of 1 produces a deliberately bright diagnostic. Do not use that setting as a quality target. Corrected lit and isolated captures, native transform logs and restoration checks remain root's next verification.

## First native diagnostic result

Root's `waterfall4-emission-open/shot_0300.png` uses the unconverted material, sampled emission 1, extinction 0, fixed frame 0, forced blocking and hidden cliffs/core. The waterfall is still absent. Runtime logs confirm the expected native translations, isotropic scale 50, component world scale 1, pivot false and visibility true. Cleanup reports restored component visibility. No missing diagnostic parameter error occurred. Thus neither a global placement transform error nor rock occlusion explains the entire failure, and unit correction alone is not yet an established rendering fix.

Follow-up diagnostic commit `0a9e3f6` adds `-DCFallConstantDensity=.05` under `-DCFallDensityDiagnostic`. This contributes known constant density independent of SVT samples, separating native material/render rejection from zero sampled data. `-DCFallDiagnosticExtinction=.001` optionally tests whether a nonzero extinction path is necessary. Defaults remain zero for both diagnostic additions. Critical material graph connections now fail explicitly if they cannot be created.

`DC_FALL_BINDING` records the actual MID-bound SVT, scalar values, actor hidden state, viewport heterogeneous-volume show flag and relevant enable/sparse/preshading/AVSM CVars. The engine's default direct live-shading march evaluates emission even at zero extinction; extinction-zero returns found in the source affect lighting-cache evaluation, not the direct emission integral. A nondefault AVSM/sparse pipeline remains a reason to verify the logged render state. Constant-density and corrected-material captures are pending; no further VDB was generated.

## Actual sampled density is now visible

Root's corrected lit capture `Saved/SceneReview/waterfall4-units-open/shot_0300.png` visibly renders the original cache, including its stepped path and receiving-water plume. Both opaque core and cliffs are hidden. Runtime uses `.016/.004` per-cm extinction, **zero emission and zero constant density**, with fixed frame 0. Logged bindings point to the correct native Falling/Impact frame-0 assets; `VoxelSizeCm=50`, world scale 1, show flag/HV/SVT enables are 1, and sparse-voxel/preshading/AVSM modes are 0. This establishes that actual sampled density works with the unit-converted lit material. It is not a uniform debug box or pre-existing atmosphere.

Visual critique: the fall is now conspicuous but soft, warm and strongly ladder-shaped around the old shelf geometry. Its repeated source channels and under-ledge path are artificial. Native visibility is solved for this diagnostic view, not visual quality. The upcoming accepted cliff geometry needs a new profile/cache, and subsequent flow should detach ballistically across overhangs instead of adhering to every horizontal shelf. Keep opt-in; review the lit combined cliff/core view, motion and performance before any default enablement. The earlier zero-extinction emission-only result remains unexplained and should not overrule this explicit zero-emission sampled-density proof.

## Fresh fit to accepted geology pass 4

After root accepted the revised cliff candidate, it exported `Saved/WaterfallProfileGeology4.json`, SHA1 `4E08EA4E6096EB9D73269BC6C8FCA36973DACE1E`. Source 8 has 350 finite rows and nine independent lateral contacts. Its lower 80 m reach now follows a 156.121 m path, down from 230.768 m with the old shelves. Central contact remains 0.0044 cm below mean water and the final 50 centreline steps have no upward motion.

The newly fitted cache is ignored/local at `Crater-Workers/Fitted-Waterfall/Saved/GeneratedWaterfallsGeology4`. Settings are unchanged: 0.5 m voxels, 24 fps, four seconds, 96 frames per sequence, 14,000 strands/sec, seed 54117. No physics or density tuning occurred. Generation took 53.08 CPU seconds and produced 101.72 MiB. All 56,000 strands reach covered water; maximum contact-depth residual is 0.018 mm, and sampled positive-mass particles never lie beneath covered water. Flight now lasts 6.44–8.71 seconds because the revised path is shorter.

| Sequence | Resolution XYZ | Native frame translation / voxel-centre minimum (cm) | Voxel-centre maximum (cm) | VDB bytes |
| --- | --- | --- | --- | --- |
| Falling | 67 x 247 x 186 | (-1650, -10450, -650) | (1650, 1850, 8600) | 79,223,421 |
| Impact | 111 x 96 x 58 | (-3850, -1100, -1650) | (1650, 3650, 1200) | 27,440,117 |

Both sequences have exact period equality and VDB roundtrip equality, stable bounds and zero clipped source mass. Wrap-frame density change remains below the interior p95 for both. All 192 stored frames were read back: zero submerged voxels above the intentional `1e-8` bounds anchors. Diagnostic side projections now show a continuous descending path without the huge old shelf folds; they are density projections, not proof of convincing whitewater. Root will import this new manifest/cache and review corrected lit units at default density with the cliff/core visible. Older Supported3 data is no longer a valid fit to the new face geometry.

## Combined default-density capture: still not visually accepted

Inspected root's `Saved/SceneReview/waterfall4-combined/shot_0300.png` against the same new geometry without volume density in `Saved/SceneReview/crater4-cliff-core/shot_0300.png`. This is a qualitative still comparison; the outputs have different resolutions. The fresh VDB adds a visible soft cream-coloured layer around the fall, especially its lower reach, and a broader tan plume at the landing. Native volume visibility is confirmed in the combined scene as well as the isolated diagnostic.

The dominant shape remains a bright, almost constant-width strip attached to the rock. Most sharp white detail comes from the opaque core, while the volume softens its outline rather than creating convincing detached sheets, fragmented streams or wind-blown spray. The impact reads as dusty fog instead of crisp aerated water, and its warm smooth envelope partly obscures the landing. The large old shelf folds are gone in **both** captures because of the geology change; that improvement should not be attributed to the VDB. The exposed rock also still shows broad planar/block-like facets.

The landing appears connected to the receiving surface, but one still does not validate motion, splash timing or performance. Keep opt-in and do not call this a finished waterfall. The next visual pass should separate attached wet flow from detached broken sheets and local airborne spray, concentrate density around real breakup/impact events, and give the landing a sharper splash/foam response. A larger or brighter continuous fog coating will preserve the current artificial strip. No code, density, lighting or cache changes were made for this critique.
