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
