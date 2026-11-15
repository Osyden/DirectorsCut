# One measured lower waterfall attachment

Owner: Codex waterfall worker, `codex/implicit-exposure-review`. Offline CPU authoring only; no live actor, sky, cloud, water or Unreal asset changed.

## Current output for isolated native preview

Use `Crater-Workers/Fitted-Waterfall/Saved/CompressedWaterfall/RingPilotB`, not the earlier Sigma6 folders. The actual source is `Cliff-Contact/Saved/CompressedEscarpment/RingPilotB/candidate.npz`, SHA256 `18e7b77ce3540d52ebb459f40571e2980a6632dcba84073d6f6de502e2112046`. All support/clearance queries use those ring triangles. `compressed_chart_uvz_m` only locates the seed; it is not used as replacement geometry. Receiving water is the finite measured `Maelstrom-Saturday/Saved/StructuralCliff/mean_water.npz`, SHA256 `bd4514e69d97799d52a78c2100d39a5d85be6b02662036a2866d818333177ff2`.

- Actual lower outlet: 32.46 m drop, 34.35 m centreline, 2.60–3.78 m width. Source ring vertex 12860. Source arena metres `(-1058.7471,1168.1472,-89.7581)`; receiving point `(-1052.6556,1161.6619,-122.2193)`.
- Last 4.32 m is an authored ballistic jet with 6 m/s horizontal launch and 4 m/s initial downward speed, gravity 9.81 m/s². Thirteen separately supported lateral tracks are tested against the actual rock and finite water. They have different contact times, approximately 0.60–0.66 s.
- `support.npz` records original triangle IDs/barycentrics, path and lateral sections. Attached core triangles that bridge a sharp crease are subdivided and reprojected; a bounded clearance failure aborts generation.
- `core.npz` has 2,736 triangles; `receiving_foam.npz` has 590. Foam filaments sample actual sloping water, following the same prescribed 18 m/s route-tangent direction as the runtime exporter. These are static geometry/UV anchors, not a completed foam shader.
- `waterfall_core.foam.fbx` contains the two meshes and three diagnostic material slots: `WetCore`, `AeratedCore`, `ReceivingFoam`. UV0 is across-fall fraction / physical arc metres. Replace diagnostic stripes with the native animated water/whitewater materials; the stripes are visibly unsuitable as final art.
- `waterfall_profile.json` uses the existing `directorscut.waterfall_profile.v1` schema and explicit preview source ID **800**, not runtime feed 8. `attachment.json` records exact source hashes and physical transforms. No old feed-8 cache fits this source.

## Transform contract

The FBX is in the existing sector chart, metres, exported `axis_forward=-Y`, `axis_up=Z`, unit scale 1. Its documented chart origin/bases are in `attachment.json`. Account for the already-verified native FBX Y reflection before reconstructing arena coordinates; do not simply set its actor transform to the identity used by the full-ring mesh. The `.npz` mesh positions themselves are absolute arena metres.

VDB uses its own yaw-only physical centimetre frame. The exact `local_origin_arena_cm` and basis vectors are in the profile. X is minus sector U, Y is minus sector V (outward), Z is up. Keep native SVT voxel size 50 cm and `pivot_at_centroid=false`, component scale 1. Do **not** apply the FBX Y reflection to a VDB frame. The foot is locally `(0,1000,0)`; source/impact locations are in the profile, not inferred from volume bounds.

The initial cache is 48 frames at 12 fps, four seconds, 0.5 m voxels, FloatGrid density. `Volumes/manifest.json` records actual fixed bounds and per-frame hashes. Both sequences total 14,626,967 bytes (13.95 MiB), generated in 6.25 s offline. Falling resolution is 25×40×84; impact is 69×78×59. All 20,000 generated strands reach covered water; maximum measured sampled entry residual is 0.000023 m. Exact t=0 versus period and written/read VDB density differences are zero. These are CPU checks, not native rendering or GPU-cost claims.

The fitted generator retains its existing mass/voxel normalization, submerged-density mask and entry-synchronized impact. No whole new fluid solver was added. The profile's central guide and separate lateral tracks are not identical clocks; the VDB generator integrates travel speed on that guide, so the recorded static jet shape does not claim a validated native velocity field.

## Actual visual result and limitations

`Saved/CompressedWaterfall/RingPilotBVolumeReview/close.png` and `sector.png` render actual ring triangles, actual core, and VDB frame 12 using CPU Blender. The correctly labelled volume render supersedes the geometry-only picture's stale Sigma6 caption. No image generation or geometry exaggeration was used.

The lower fall is visible and correctly placed, but **not visually accepted**: the bare core reads as a straight striped ribbon, and the CPU volume is dark/soft under the fixed review sun. The volume uses actual float density and no emission; it does not predict Unreal's UNorm8 import or its lighting. The foam filaments are too thin/regular to count as finished receiving foam. Native material/lighting evaluation is required, followed by deliberate sheet breakup and splash detail.

The first longer ravine path was mostly hidden by its own walls. An attempted longer jet there failed whole-rock clearance and was rejected rather than forced through terrain. Horizontal projected widths also produced tall jagged strips; the current tool instead uses surface-tangent widths and nearest original triangles. Earlier Sigma6 caches and meshes are diagnostic failures, not integration candidates.

The current lower outlet does not provide the reference's epic rim waterfall. Root has requested a separate supported high-rim freefall with a 100–180 m drop and 10–20 m/s feeder momentum on the current ring. That is the next authoring task. The existing lower cache is useful for testing attachment and native rendering while the tall source is selected.

## Tall hero: final immutable native candidate

The subsequent supported high outlet is now generated at **`Saved/CompressedWaterfall/RingPilotBHero10`**. Use this for the tall native preview. Keep this folder immutable while root imports/captures it. `RingPilotBHero` without `10` is the rejected 12.5 m/s route-margin diagnostic.

- Same actual RingPilotB source hash. High source is canonical vertex **72791, column 552**, outside the parallel buttress experiment's columns 573–585. Lip arena metres approximately `(-1009.4793,1230.0542,10.1746)`; there is a four-metre supported feeder stub. Upstream rock is 0.81 m higher over four metres; the front drops 39.81 m over the next four metres.
- Ballistic horizontal speed **10 m/s**, initial downward speed 1 m/s, gravity 9.81 m/s². Actual free drop **148.313 m**, projection **53.978 m**, centre flight **5.39784 s**. Including the feeder, the overall export drop is 148.822 m and arc length 173.968 m. The source is eight metres wide. Thirteen independently supported tracks clear the whole actual rock mesh and reach finite water in 5.383–5.425 s; this does not mean every interpolated/secondary droplet has been independently tested against rock.
- `generate_waterfall_vdb.py` now recognizes the explicit `ballistic_launch` contract. Core particles use launch position/momentum plus gravity, with modest coherent lateral breakup; secondary droplets acquire drag after release. No 80 m volume is stretched. Fixed emission mass per time and accelerating travel reduce core density along the drop; the model is not a two-phase fluid solver or calibrated water mass in kilograms.
- FBX SHA256 **`09b849673b84120435b99018834cea12e6976ab6d6de971beefcf52a40bae1a0`**. Same three material slots and chart/FBX transform contract as above. Profile SHA1 **`6186786478027f8fa97b05c6c065ab30b8cd3b2c`**. VDB arena origin centimetres **`(-98226.29188380092,119544.30635117897,-13783.880187809968)`**, yaw **−135.3549236084079°**, scale 1. Use the exact JSON basis vectors rather than recomputing from rounded values.
- Falling and impact sequences: **48 frames at 12 fps, four seconds, 0.5 m voxels**, total **26.34 MiB**, actual generation **15.95 s**. Original generic particle output is unchanged (maximum position/mass residual 0); the original attached-profile regression differs only by floating arithmetic, maximum `3.55e-15`. Both checked at five phases including the loop boundary. Current hero cache manifest contains the complete contact, fixed-bound, roundtrip and periodicity measurements.
- The impact is **140.669 m** from the sampled current analytic route centreline. Subtracting 110 m steering allowance, an explicitly assumed 15 m ship halfwidth, and 4 m sheet halfwidth leaves **11.669 m**. **Spray is excluded**, and the ship's actual runtime envelope has not been verified. This is borderline local clearance, not a whole-route safety guarantee. At 12.5 m/s the same calculation was −2.003 m, which is why that candidate was rejected.

The actual corrected CPU views are **`Saved/CompressedWaterfall/RingPilotBHero10WorldVolumeReview/close.png`** and `sector.png`. They show the full physical drop without rescaling geometry. The source/trajectory now read as a tall detached fall, but the diagnostic remaining core fragments read as repeated white dashes, and the volume is not final water shading. The native worker supplies animated coverage materials; root will inspect the actual native capture before further changes. If the dashes survive, remove opaque fragments below roughly the first 25 m and let finer volume/spray carry the lower drop. Do not widen the impact before reviewing route clearance.

### Corrected CPU volume units

The initial CPU volume pictures in `*VolumeReview` without `World` used Blender's default **OBJECT** density space with a 0.01 centimetre-to-metre object scale. That preserves optical depth while shrinking the object, incorrectly making our already per-metre coefficients about 100 times too opaque. They are **superseded** and cannot establish that the cache is intrinsically an opaque dark tube.

The reviewer now explicitly sets `volume.render.space='WORLD'`, retaining the same actual VDB, fixed sun, no emission, and extinction 1.6/0.4 per metre (equivalent to native 0.016/0.004 per centimetre). Local Blender 5.3 reported default `OBJECT` and supported `OBJECT/WORLD`; the distinction is documented in [Blender's volume properties](https://docs.blender.org/manual/en/4.5/modeling/volumes/properties.html) and [volume release notes](https://developer.blender.org/docs/release_notes/2.83/volumes/). This is an offline reviewer correction; do not compensate native density constants for the old bad images. The corrected CPU render still reads original float VDB rather than native SVT quantization.

To reproduce the hero, use the attachment command below with `--hero-source-index 72791 --hero-speed 10 --width-m 8`, then generate VDB with `--width-m 8 --height-m 150 --voxel-m .5 --fps 12 --duration 4 --emission-rate 8000`. The root's native importer/runtime changes live with the other worker, not in this source commit.

Static/local limits: no upstream feeder mesh yet; no complete route or animated-wave collision guarantee; receiving coverage is resampled at 1 m for the VDB. Any ring geometry change invalidates the attachment and fitted cache. No terrain, support, or whole-scene acceptance is implied.

## Reproduce

Run Blender background CPU with `Tools/attach_compressed_cliff_waterfall.py -- --cliff <RingPilotB/candidate.npz> --water <mean_water.npz> --output <new Saved directory> --template-source-index 939 --width-m 4 --render`. The output directory must be new.

Then run bundled Python with `Tools/generate_waterfall_vdb.py --profile <output/waterfall_profile.json> --output <output/Volumes> --width-m 4 --height-m 40 --voxel-m .5 --fps 12 --duration 4 --emission-rate 5000`.

For an actual combined CPU frame: Blender background, eight threads, `Tools/review_compressed_waterfall.py -- --attachment <output> --output <new Saved review directory> --frame 12 --samples 32`. All generated meshes, caches and images stay ignored under Saved.
