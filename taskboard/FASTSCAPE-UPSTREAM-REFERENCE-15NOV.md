# Pinned upstream escarpment reproduction

Owner: Codex cliff worker, branch `codex/fastscape-reference`, based on `cc82602`. Scope: new offline wrapper, narrow third-party notice and this handoff. No crater fitting, rescaling, temple constraints, Unreal, GPU, runtime materials or scene changes.

The official Fastscapelib 0.3.0 escarpment model now runs unchanged and exports actual native-scale terrain. Independent direct notebook replay matches the initial elevation, final elevation and upstream last-step drainage arrays **bitwise**. This establishes a reproducible upstream process model; it is not an approved game cliff or AAA asset.

## Exact source and isolated setup

Package `fastscapelib==0.3.0` was installed with `--target Saved/FastscapeReference/python_packages --no-deps --only-binary=:all:`. The Windows CPython 3.13 wheel is about 419 kB. Existing Python is Blender's standalone CPython 3.13.13, NumPy 2.3.4; its package installation and system Python were not modified.

The official release notebook and licence were downloaded from published revision `707fcc10976b0e729f1415fd16b0a161e1f746c3` and read before execution. The wrapper verifies exact file hashes and package version/location. Computational cells 4, 6, 8, 11 and 14 are compiled directly from the original notebook with no line rewriting. Imports are supplied directly; only matplotlib setup, plotting and empty cells are omitted.

See [Fastscapelib notice](../Tools/ThirdParty/Fastscapelib-NOTICE.md) for source attribution, GPL-3.0 licence link and hashes. The full licence, exact notebook and attribution accompany each Saved output bundle. No solver, binary or notebook implementation is vendored into the game.

Official reference: [Escarpment example](https://fastscapelib.readthedocs.io/en/latest/examples/escarpment_py.html). The pinned code uses **LOOPED periodic top/bottom boundaries**, despite a prose sentence saying reflective. We reproduce the code, not that inconsistent description. Left boundary is fixed-value; right boundary is core/free.

## Native model and measured result

- 101 × 201 nodes, 100 m spacing, native **20,000 m X × 10,000 m Y** domain. Identity XYZ frame, metres, no vertical exaggeration.
- Two initially almost-flat plateaus separated by a 400 m step, deterministic PCG64 seed 1234.
- `SingleFlowRouter`, `MSTSinkResolver`, then `MultiFlowRouter(1.1)`; stream-power erosion and ADI hillslope diffusion.
- `k_coef=1e-4`, drainage-area exponent 0.4, slope exponent 1, tolerance `1e-5`, diffusivity 0.01.
- 50 timesteps × 2,000 years = 100,000 model years, unchanged. Measured model execution: 0.554 s on this machine.
- Initial height range 0.0000461–400.999968 m; final 0.0020711–400.123036 m. Maximum net lowering 400.953068 m; 5,793 cells lowered more than 10 m. Left base level remains bitwise unchanged.

`terrain_arrays.npz` preserves the exact official final drainage array. That array was computed at the **input to the last model step**, as upstream does. A separately named `final_rerouted_drainage_area_m2` is calculated after the unchanged run for diagnostics; it is not substituted silently into the reference result. Minimum accumulated area is 10,000 m², consistent with the 100 m grid. No sediment transport, erosion-derived PBR mask or physical validation of a real catchment is claimed.

The physical mesh exports have 20,301 vertices / 40,000 triangles, one valid open manifold with 600 boundary edges and upward winding. Independent audit verifies exact array-to-XYZ correspondence, identity frame, native dimensions and fixed-left boundary. Report: `Cliff-Contact/Saved/WeatheredAudit/fastscape_official_escarpment.json`.

Array SHA256 values:

- Initial height: `ccfa96494ee2cda505af2c962e00241725a703f41472728e3a9f04b4719193a7`.
- Final height: `84dd324e74fbc11b37ba29511e444061c017cfe0906a50dacc5ef31788111a09`.
- Official last-step drainage: `63725eba4796b731ea94eb2042d6537c5c7448663f11f289f6aa6348b93707fd`.

## Reproduction

Run in `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`. The inspected exact inputs are `Saved/FastscapeReference/upstream/escarpment_py.ipynb` and `LICENSE`; their hashes are in the notice and checked by the wrapper. Download from the pinned raw GitHub URLs, not the moving latest branch, if those ignored files are absent.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B -m pip install --disable-pip-version-check --no-cache-dir --no-deps --only-binary=:all: --target Saved/FastscapeReference/python_packages fastscapelib==0.3.0
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/run_fastscape_escarpment_reference.py --dependencies Saved/FastscapeReference/python_packages --notebook Saved/FastscapeReference/upstream/escarpment_py.ipynb --license Saved/FastscapeReference/upstream/LICENSE --output Saved/FastscapeReference/ReplayOfficial
```

Use a fresh target for installation if the dependency folder already exists. The wrapper refuses to overwrite an output directory and writes only under Saved. It performs no downloads, package installs or source fitting itself.

Actual bundle: `Flow-Seams/Saved/FastscapeReference/OfficialEscarpment/` contains `baseline.npz`, `candidate.npz`, `terrain_arrays.npz`, `manifest.json`, exact upstream notebook, full licence and attribution. Renderer consumes the original triangles, not a resized crater proxy.

## Visual evidence and limits

Independent review files are under `Fitted-Waterfall/Saved/ImplicitCliffReview/UpstreamFastscape/`. The first `OfficialEscarpment` full-domain plan/oblique images are **invalid camera evidence**: the existing renderer's 10 km far clipping plane hid the 20 km terrain. The 7 km detail view was visible. Those files are retained; corrected full-domain captures use a Saved-only 100 km clipping setting in `OfficialEscarpmentLongClip`, with no source mesh scaling.

The worker, independent reviewer and parent inspected the corrected full-domain oblique/plan and 7 km detail frames. Connected scarp retreat, branching ravines and interlocking spurs are visible; these form a more coherent drainage structure than the rejected hand-cut pockets. The 100 m grid produces strong facets and steps in the detail view. This passes only the upstream-method proof: the geographically broad native terrain is not a ready 500 m cinematic cliff and must not be silently shrunk to one.

Final valid evidence is in `Fitted-Waterfall/Saved/ImplicitCliffReview/UpstreamFastscape/OfficialEscarpmentLongClip/`: `UPSTREAM REFERENCE 20x10km - oblique_fractured.png`, `UPSTREAM REFERENCE 20x10km - plan_fractured.png`, and `UPSTREAM REFERENCE - 7km detail_fractured.png`, with matched baseline images and `review.json`. Each mesh is a valid open manifold. The generic comparison's boundary-preservation flag is false because 500 of 600 outer edges move under the unchanged upstream mixed boundary conditions; this is expected model behavior, not a newly broken mesh. The fixed left boundary remains exact.

The next scheduled experiment can generate a fresh physically dimensioned 500 m escarpment with finer grid spacing and the real coast/datum. It must neither scale this 20 km mesh nor re-erode the rejected soft wall. This task performs no such adaptation. Exact temple supports and overhanging rock faces remain separate unsolved integration requirements; forcing many internal fixed pads into the initial scientific demonstration would repeat the pedestal problem instead of validating the method.
