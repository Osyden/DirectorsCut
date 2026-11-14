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
